/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include "ecmascript/compiler/assembler/x64/assembler_x64.h"

#include <ostream>

#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64.h"
#include "ecmascript/compiler/assembler/x64/extended_assembler_x64.h"
#include "ecmascript/compiler/codegen/llvm/llvm_codegen.h"
#include "ecmascript/compiler/trampoline/x64/common_call.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/dyn_chunk.h"
#include "ecmascript/tests/test_helper.h"

#include "llvm-c/Analysis.h"
#include "llvm-c/Core.h"
#include "llvm-c/Disassembler.h"
#include "llvm-c/ExecutionEngine.h"
#include "llvm-c/Target.h"


namespace panda::test {
using namespace panda::ecmascript;
using namespace panda::ecmascript::x64;

class AssemblerX64Test : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        InitializeLLVM(TARGET_X64);
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
        chunk_ = thread->GetEcmaVM()->GetChunk();
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    static const char *SymbolLookupCallback([[maybe_unused]] void *disInfo, [[maybe_unused]] uint64_t referenceValue,
                                            uint64_t *referenceType, [[maybe_unused]] uint64_t referencePC,
                                            [[maybe_unused]] const char **referenceName)
    {
        *referenceType = LLVMDisassembler_ReferenceType_InOut_None;
        return nullptr;
    }

    void InitializeLLVM(std::string triple)
    {
        if (triple.compare(TARGET_X64) == 0) {
            LLVMInitializeX86TargetInfo();
            LLVMInitializeX86TargetMC();
            LLVMInitializeX86Disassembler();
            /* this method must be called, ohterwise "Target does not support MC emission" */
            LLVMInitializeX86AsmPrinter();
            LLVMInitializeX86AsmParser();
            LLVMInitializeX86Target();
        } else if (triple.compare(TARGET_AARCH64) == 0) {
            LLVMInitializeAArch64TargetInfo();
            LLVMInitializeAArch64TargetMC();
            LLVMInitializeAArch64Disassembler();
            LLVMInitializeAArch64AsmPrinter();
            LLVMInitializeAArch64AsmParser();
            LLVMInitializeAArch64Target();
        } else {
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
        }
    }

    void DisassembleChunk(const char *triple, Assembler *assemlber, std::ostream &os)
    {
        LLVMDisasmContextRef dcr = LLVMCreateDisasm(triple, nullptr, 0, nullptr, SymbolLookupCallback);
        uint8_t *byteSp = assemlber->GetBegin();
        size_t numBytes = assemlber->GetCurrentPosition();
        unsigned pc = 0;
        const char outStringSize = 100;
        char outString[outStringSize];
        while (numBytes > 0) {
            size_t InstSize = LLVMDisasmInstruction(dcr, byteSp, numBytes, pc, outString, outStringSize);
            if (InstSize == 0) {
                // 8 : 8 means width of the pc offset and instruction code
                os << std::setw(8) << std::setfill('0') << std::hex << pc << ":" << std::setw(8)
                   << *reinterpret_cast<uint32_t *>(byteSp) << "maybe constant" << std::endl;
                pc += 4; // 4 pc length
                byteSp += 4; // 4 sp offset
                numBytes -= 4; // 4 num bytes
            }
            // 8 : 8 means width of the pc offset and instruction code
            os << std::setw(8) << std::setfill('0') << std::hex << pc << ":" << std::setw(8)
               << *reinterpret_cast<uint32_t *>(byteSp) << " " << outString << std::endl;
            pc += InstSize;
            byteSp += InstSize;
            numBytes -= InstSize;
        }
        LLVMDisasmDispose(dcr);
    }

    EcmaVM *instance {nullptr};
    JSThread *thread {nullptr};
    EcmaHandleScope *scope {nullptr};
    Chunk *chunk_ {nullptr};
};

#define __ masm.
HWTEST_F_L0(AssemblerX64Test, Emit)
{
    x64::AssemblerX64 masm(chunk_);
    Label lable1;

    size_t current = 0;
    __ Pushq(rbp);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x55U);
    __ Pushq(0);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x6AU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    __ Popq(rbp);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x5DU);
    __ Bind(&lable1);
    __ Movq(rcx, rbx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x89U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xCBU);
    __ Movq(Operand(rsp, 0x40U), rbx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x8BU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x5CU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x24U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x40U);
    __ Jmp(&lable1);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xEBU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xF6U);
    __ Ret();
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xC3U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), masm.GetCurrentPosition());
}

HWTEST_F_L0(AssemblerX64Test, Emit1)
{
    x64::AssemblerX64 masm(chunk_);

    size_t current = 0;
    __ Movl(Operand(rax, 0x38), rax);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x8BU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x40U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x38U);

    // 41 89 f6 movl    %esi, %r14d
    __ Movl(rsi, r14);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x41U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x89U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xF6U);

    // movzbq  (%rcx), %rax
    __ Movzbq(Operand(rcx, 0), rax);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0FU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xB6U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x01U);

    // 48 ba 02 00 00 00 00 00 00 00   movabs $0x2,%rdx
    __ Movabs(0x2, rdx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xBAU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x02U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);

    __ Movq(0x5, rdx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xBAU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x05U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);

    // 49 89 e0        mov    %rsp,%r8
    __ Movq(rsp, r8);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x49U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x89U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xE0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), masm.GetCurrentPosition());
}

HWTEST_F_L0(AssemblerX64Test, Emit2)
{
    x64::AssemblerX64 masm(chunk_);

    size_t current = 0;
    // 81 fa ff ff ff 09       cmpl    $0x9ffffff,%edx
    __ Cmpl(0x9FFFFFF, rdx);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x81U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFAU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x09U);

    // 39 cb   cmpl    %ecx,%ebx
    __ Cmpl(rcx, rbx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x39U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xCBU);

    // 48 83 fa 00     cmp    $0x0,%rdx
    __ Cmp(0x0, rdx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x83U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFAU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);

    // 4c 39 D8 cmpq    %r11, %rax
    __ Cmpq(r11, rax);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x4CU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x39U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xD8U);


    // 0f ba e0 08     bt     $0x8,%eax
    __ Btl(0x8, rax);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0FU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xBAU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xE0U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x08U);
    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), masm.GetCurrentPosition());
}

HWTEST_F_L0(AssemblerX64Test, Emit3)
{
    x64::AssemblerX64 masm(chunk_);
    size_t current = 0;

    // cmovbe  %ebx, %ecx
    __ CMovbe(rbx, rcx);
    uint32_t value = masm.GetU8(current++);

    ASSERT_EQ(value, 0x0FU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x46U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xCBU);

    // testb   $0x1, %r14b
    __ Testb(0x1, r14);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x41U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xF6U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xC6U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x01U);

    // 48 f6 c4 0f testq   $15, %rsp
    __ Testq(15, rsp);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x40U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xF6U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xC4U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0FU);

    // andq    $ASM_JS_METHOD_NUM_VREGS_MASK, %r11
    __ Andq(0xfffffff, r11);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x49U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x81U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xE3U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0FU);

    // andl 0xfffffff, %eax
    __ Andl(0xfffffff, rax);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x25U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0FU);

    // and     %rax, %rdx
    __ And(rax, rdx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x21U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xC2U);
    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), masm.GetCurrentPosition());
}

HWTEST_F_L0(AssemblerX64Test, Emit4)
{
    x64::AssemblerX64 masm(chunk_);
    size_t current = 0;

    // 4a 8d 0c f5 00 00 00 00 leaq    0x0(,%r14,8),%rcx
    __ Leaq(Operand(r14, Scale::Times8, 0), rcx);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x4AU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x8DU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0CU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xF5U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x00U);

    // 8d 90 ff ff ff fc       leal    -0x3000001(%rax),%edx
    __ Leal(Operand(rax, -50331649), rdx);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x8DU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x90U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFFU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xFCU);

    // c1 e0 18        shl    $0x18,%eax
    __ Shll(0x18, rax);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xC1U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xE0U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x18U);

    // shrq    $ASM_JS_METHOD_NUM_ARGS_START_BIT(32), %r11
    __ Shrq(32, r11);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x49U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xC1U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xEBU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x20U);

    // int3
    __ Int3();
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xCCU);
    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), masm.GetCurrentPosition());
}

// Test for Subl with immediate value in 8-bit range
HWTEST_F_L0(AssemblerX64Test, SublImm8)
{
    x64::AssemblerX64 masm(chunk_);
    size_t current = 0;

    // 83 e8 05     subl    $0x5,%eax
    __ Subl(5, rax);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x83U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xE8U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x05U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), masm.GetCurrentPosition());
}

// Test for Subl with rax register (special encoding)
HWTEST_F_L0(AssemblerX64Test, SublRax)
{
    x64::AssemblerX64 masm(chunk_);
    size_t current = 0;

    // 2d 78 56 34 12     subl    $0x12345678,%eax
    __ Subl(0x12345678, rax);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x2DU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x78U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x56U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x34U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x12U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), masm.GetCurrentPosition());
}

// Test for Subl with non-rax register (general encoding)
// ModR/M for mode=3, reg=5 (sub), rm=rcx(1): 0b11_101_001 = 0xE9
HWTEST_F_L0(AssemblerX64Test, SublNonRax)
{
    x64::AssemblerX64 masm(chunk_);
    size_t current = 0;

    // 81 e9 78 56 34 12     subl $0x12345678,%ecx
    // ModR/M: mode=3(11), reg=5(101), rm=rcx(001) = 0b11001001 = 0xE9
    __ Subl(0x12345678, rcx);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x81U);  // opcode
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xE9U);  // ModR/M: mode=3, reg=5, rm=rcx
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x78U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x56U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x34U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x12U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), masm.GetCurrentPosition());
}

// Test for Jmp with 8-bit offset (short jump)
HWTEST_F_L0(AssemblerX64Test, JmpImm8)
{
    x64::AssemblerX64 masm(chunk_);
    size_t current = 0;

    // eb 10        jmp     10 <label>
    __ Jmp(0x10);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xEBU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x10U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), masm.GetCurrentPosition());
}

// Test for Jmp with 32-bit offset (long jump)
HWTEST_F_L0(AssemblerX64Test, JmpImm32)
{
    x64::AssemblerX64 masm(chunk_);
    size_t current = 0;

    // e9 78 56 34 12     jmp     12345678 <label>
    __ Jmp(0x12345678);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xE9U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x78U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x56U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x34U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x12U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), masm.GetCurrentPosition());
}

// Test for Or with immediate value in 8-bit range
HWTEST_F_L0(AssemblerX64Test, OrImm8)
{
    x64::AssemblerX64 masm(chunk_);
    size_t current = 0;

    // 48 83 c8 0f     rex.W orl     $0xf,%eax
    __ Or(0xF, rax);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);  // REX.W prefix
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x83U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xC8U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0FU);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), masm.GetCurrentPosition());
}

// Test for Or with rax register (special encoding)
HWTEST_F_L0(AssemblerX64Test, OrRax)
{
    x64::AssemblerX64 masm(chunk_);
    size_t current = 0;

    // 48 0d 78 56 34 12     rex.W orl     $0x12345678,%eax
    __ Or(0x12345678, rax);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);  // REX.W prefix
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x0DU);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x78U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x56U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x34U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x12U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), masm.GetCurrentPosition());
}

// Test for Or with non-rax register (general encoding)
HWTEST_F_L0(AssemblerX64Test, OrNonRax)
{
    x64::AssemblerX64 masm(chunk_);
    size_t current = 0;

    // 48 81 c9 78 56 34 12     rex.W orl     $0x12345678,%ecx
    __ Or(0x12345678, rcx);
    uint32_t value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x48U);  // REX.W prefix
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x81U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0xC9U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x78U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x56U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x34U);
    value = masm.GetU8(current++);
    ASSERT_EQ(value, 0x12U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), masm.GetCurrentPosition());
}

// Test for Jz (jump if zero) - tests EmitJz internally
HWTEST_F_L0(AssemblerX64Test, JzTest)
{
    x64::AssemblerX64 masm(chunk_);
    Label label;

    // Bind label first to test bound label path (calls EmitJz)
    __ Bind(&label);
    __ Movq(rax, rbx);  // Some instruction

    // Create another label for Jz
    Label jmpLabel;
    __ Jz(&jmpLabel);  // This tests Jz with unbound label (rel8 path)
    __ Bind(&jmpLabel);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jz with bound label (tests EmitJz with short offset)
HWTEST_F_L0(AssemblerX64Test, JzBoundLabel)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;

    // Emit some instructions first to create distance
    __ Movq(rax, rbx);  // 3 bytes
    __ Movq(rcx, rdx);  // 3 bytes
    __ Movq(r8, r9);    // 3 bytes
    // Total 9 bytes, offset should be within 8-bit range

    __ Bind(&target);
    __ Movq(r10, r11);  // 3 bytes at target

    // Now Jz to already bound label - tests EmitJz
    Label jmpHere;
    __ Bind(&jmpHere);
    __ Jz(&target);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jle (jump if less or equal) - tests EmitJle internally
HWTEST_F_L0(AssemblerX64Test, JleTest)
{
    x64::AssemblerX64 masm(chunk_);
    Label label;

    // Bind label first to test bound label path (calls EmitJle)
    __ Bind(&label);
    __ Movq(rax, rbx);  // Some instruction

    // Create another label for Jle
    Label jmpLabel;
    __ Jle(&jmpLabel);  // This tests Jle with unbound label (rel8 path)
    __ Bind(&jmpLabel);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jle with bound label (tests EmitJle with short offset)
HWTEST_F_L0(AssemblerX64Test, JleBoundLabel)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;

    // Emit some instructions first to create distance
    __ Movq(rax, rbx);  // 3 bytes
    __ Movq(rcx, rdx);  // 3 bytes
    __ Movq(r8, r9);    // 3 bytes
    // Total 9 bytes, offset should be within 8-bit range

    __ Bind(&target);
    __ Movq(r10, r11);  // 3 bytes at target

    // Now Jle to already bound label - tests EmitJle
    Label jmpHere;
    __ Bind(&jmpHere);
    __ Jle(&target);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for multiple conditional jumps
HWTEST_F_L0(AssemblerX64Test, ConditionalJumps)
{
    x64::AssemblerX64 masm(chunk_);
    Label label1;
    Label label2;
    Label label3;

    __ Bind(&label1);
    __ Cmp(rax, rbx);
    __ Jz(&label2);   // Jump if zero
    __ Jle(&label3);  // Jump if less or equal
    __ Jmp(&label1);  // Unconditional jump

    __ Bind(&label2);
    __ Movq(rax, rcx);

    __ Bind(&label3);
    __ Movq(rbx, rdx);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jmp with Distance::Near and IsLinkedNear path
// This test covers the if (target->IsLinkedNear()) branch in Jmp
HWTEST_F_L0(AssemblerX64Test, JmpNearLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // Jump to end (2 bytes: EB xx)
    __ Jmp(&end, Distance::Near);
    // Jump to same target again - tests IsLinkedNear() == true path
    __ Jmp(&target, Distance::Near);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jz with Distance::Near and IsLinkedNear path
HWTEST_F_L0(AssemblerX64Test, JzNearLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // Jump to end (2 bytes: 74 xx)
    __ Jz(&end, Distance::Near);
    // Jump to same target again - tests IsLinkedNear() == true path
    __ Jz(&target, Distance::Near);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jnz with Distance::Near and IsLinkedNear path
HWTEST_F_L0(AssemblerX64Test, JnzNearLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // Jump to end (2 bytes: 75 xx)
    __ Jnz(&end, Distance::Near);
    // Jump to same target again - tests IsLinkedNear() == true path
    __ Jnz(&target, Distance::Near);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Je with Distance::Near and IsLinkedNear path
HWTEST_F_L0(AssemblerX64Test, JeNearLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // Jump to end (2 bytes: 74 xx)
    __ Je(&end, Distance::Near);
    // Jump to same target again - tests IsLinkedNear() == true path
    __ Je(&target, Distance::Near);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jne with Distance::Near and IsLinkedNear path
HWTEST_F_L0(AssemblerX64Test, JneNearLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // Jump to end (2 bytes: 75 xx)
    __ Jne(&end, Distance::Near);
    // Jump to same target again - tests IsLinkedNear() == true path
    __ Jne(&target, Distance::Near);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Ja with Distance::Near and IsLinkedNear path
HWTEST_F_L0(AssemblerX64Test, JaNearLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // Jump to end (2 bytes: 77 xx)
    __ Ja(&end, Distance::Near);
    // Jump to same target again - tests IsLinkedNear() == true path
    __ Ja(&target, Distance::Near);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jb with Distance::Near and IsLinkedNear path
HWTEST_F_L0(AssemblerX64Test, JbNearLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // Jump to end (2 bytes: 72 xx)
    __ Jb(&end, Distance::Near);
    // Jump to same target again - tests IsLinkedNear() == true path
    __ Jb(&target, Distance::Near);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jae with Distance::Near and IsLinkedNear path
HWTEST_F_L0(AssemblerX64Test, JaeNearLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // Jump to end (2 bytes: 73 xx)
    __ Jae(&end, Distance::Near);
    // Jump to same target again - tests IsLinkedNear() == true path
    __ Jae(&target, Distance::Near);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jg with Distance::Near and IsLinkedNear path
HWTEST_F_L0(AssemblerX64Test, JgNearLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // Jump to end (2 bytes: 7F xx)
    __ Jg(&end, Distance::Near);
    // Jump to same target again - tests IsLinkedNear() == true path
    __ Jg(&target, Distance::Near);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jge with Distance::Near and IsLinkedNear path
HWTEST_F_L0(AssemblerX64Test, JgeNearLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // Jump to end (2 bytes: 7D xx)
    __ Jge(&end, Distance::Near);
    // Jump to same target again - tests IsLinkedNear() == true path
    __ Jge(&target, Distance::Near);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jbe with Distance::Near and IsLinkedNear path
HWTEST_F_L0(AssemblerX64Test, JbeNearLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // Jump to end (2 bytes: 76 xx)
    __ Jbe(&end, Distance::Near);
    // Jump to same target again - tests IsLinkedNear() == true path
    __ Jbe(&target, Distance::Near);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jnb with Distance::Near and IsLinkedNear path
HWTEST_F_L0(AssemblerX64Test, JnbNearLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // Jump to end (2 bytes: 73 xx)
    __ Jnb(&end, Distance::Near);
    // Jump to same target again - tests IsLinkedNear() == true path
    __ Jnb(&target, Distance::Near);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jmp with Distance::Far and IsLinked path
HWTEST_F_L0(AssemblerX64Test, JmpFarLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // First jump with Far distance - sets pos_ through LinkTo()
    __ Jmp(&end, Distance::Far);
    // Second jump to same target - tests IsLinked() == true path
    __ Jmp(&target, Distance::Far);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jz with Distance::Far and IsLinked path
HWTEST_F_L0(AssemblerX64Test, JzFarLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // First jump with Far distance - sets pos_ through LinkTo()
    __ Jz(&end, Distance::Far);
    // Second jump to same target - tests IsLinked() == true path
    __ Jz(&target, Distance::Far);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jnz with Distance::Far and IsLinked path
HWTEST_F_L0(AssemblerX64Test, JnzFarLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // First jump with Far distance - sets pos_ through LinkTo()
    __ Jnz(&end, Distance::Far);
    // Second jump to same target - tests IsLinked() == true path
    __ Jnz(&target, Distance::Far);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Je with Distance::Far and IsLinked path
HWTEST_F_L0(AssemblerX64Test, JeFarLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // First jump with Far distance - sets pos_ through LinkTo()
    __ Je(&end, Distance::Far);
    // Second jump to same target - tests IsLinked() == true path
    __ Je(&target, Distance::Far);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jne with Distance::Far and IsLinked path
HWTEST_F_L0(AssemblerX64Test, JneFarLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // First jump with Far distance - sets pos_ through LinkTo()
    __ Jne(&end, Distance::Far);
    // Second jump to same target - tests IsLinked() == true path
    __ Jne(&target, Distance::Far);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Ja with Distance::Far and IsLinked path
HWTEST_F_L0(AssemblerX64Test, JaFarLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // First jump with Far distance - sets pos_ through LinkTo()
    __ Ja(&end, Distance::Far);
    // Second jump to same target - tests IsLinked() == true path
    __ Ja(&target, Distance::Far);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jb with Distance::Far and IsLinked path
HWTEST_F_L0(AssemblerX64Test, JbFarLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // First jump with Far distance - sets pos_ through LinkTo()
    __ Jb(&end, Distance::Far);
    // Second jump to same target - tests IsLinked() == true path
    __ Jb(&target, Distance::Far);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jae with Distance::Far and IsLinked path
HWTEST_F_L0(AssemblerX64Test, JaeFarLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // First jump with Far distance - sets pos_ through LinkTo()
    __ Jae(&end, Distance::Far);
    // Second jump to same target - tests IsLinked() == true path
    __ Jae(&target, Distance::Far);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jg with Distance::Far and IsLinked path
HWTEST_F_L0(AssemblerX64Test, JgFarLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // First jump with Far distance - sets pos_ through LinkTo()
    __ Jg(&end, Distance::Far);
    // Second jump to same target - tests IsLinked() == true path
    __ Jg(&target, Distance::Far);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jge with Distance::Far and IsLinked path
HWTEST_F_L0(AssemblerX64Test, JgeFarLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // First jump with Far distance - sets pos_ through LinkTo()
    __ Jge(&end, Distance::Far);
    // Second jump to same target - tests IsLinked() == true path
    __ Jge(&target, Distance::Far);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jbe with Distance::Far and IsLinked path
HWTEST_F_L0(AssemblerX64Test, JbeFarLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // First jump with Far distance - sets pos_ through LinkTo()
    __ Jbe(&end, Distance::Far);
    // Second jump to same target - tests IsLinked() == true path
    __ Jbe(&target, Distance::Far);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jnb with Distance::Far and IsLinked path
HWTEST_F_L0(AssemblerX64Test, JnbFarLinked)
{
    x64::AssemblerX64 masm(chunk_);
    Label target;
    Label end;

    // First jump with Far distance - sets pos_ through LinkTo()
    __ Jnb(&end, Distance::Far);
    // Second jump to same target - tests IsLinked() == true path
    __ Jnb(&target, Distance::Far);

    // Bind the target
    __ Bind(&target);
    __ Bind(&end);

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

#undef __
}  // namespace panda::test
