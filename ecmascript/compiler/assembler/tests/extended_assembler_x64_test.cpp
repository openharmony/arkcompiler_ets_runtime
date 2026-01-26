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

#include "ecmascript/compiler/assembler/x64/extended_assembler_x64.h"

#include <iostream>
#include <ostream>

#include "ecmascript/compiler/assembler/x64/macro_assembler_x64.h"
#include "ecmascript/compiler/codegen/llvm/llvm_codegen.h"
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
using namespace panda::ecmascript::kungfu;
using namespace panda::ecmascript::x64;

class ExtendedAssemblerX64Test : public testing::Test {
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

    EcmaVM *instance {nullptr};
    JSThread *thread {nullptr};
    EcmaHandleScope *scope {nullptr};
    Chunk *chunk_ {nullptr};
};

// Test for PushGhcCalleeSaveRegisters
// Tests pushq r10, pushq r11, pushq r12, pushq r13, pushq r15
HWTEST_F_L0(ExtendedAssemblerX64Test, PushGhcCalleeSaveRegisters)
{
    ExtendedAssembler masm(chunk_, nullptr);

    masm.PushGhcCalleeSaveRegisters();

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);  // Should generate code

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for PopCppCalleeSaveNoReservedRegisters
// Tests popq rbx, addq $8, rsp, popq r14, popq r13, popq r12
HWTEST_F_L0(ExtendedAssemblerX64Test, PopCppCalleeSaveNoReservedRegisters)
{
    ExtendedAssembler masm(chunk_, nullptr);

    masm.PopCppCalleeSaveNoReservedRegisters();

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);  // Should generate code

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for UpdateCalleeSaveRegisters
// Tests addq $0x28, rsp
HWTEST_F_L0(ExtendedAssemblerX64Test, UpdateCalleeSaveRegisters)
{
    ExtendedAssembler masm(chunk_, nullptr);

    masm.UpdateCalleeSaveRegisters();

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);  // Should generate code

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for PushCppCalleeSaveRegisters
// Tests pushq r12, pushq r13, pushq r14, pushq r15, pushq rbx
HWTEST_F_L0(ExtendedAssemblerX64Test, PushCppCalleeSaveRegisters)
{
    ExtendedAssembler masm(chunk_, nullptr);

    masm.PushCppCalleeSaveRegisters();

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);  // Should generate code

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for PopCppCalleeSaveRegisters
// Tests popq rbx, popq r15, popq r14, popq r13, popq r12
HWTEST_F_L0(ExtendedAssemblerX64Test, PopCppCalleeSaveRegisters)
{
    ExtendedAssembler masm(chunk_, nullptr);

    masm.PopCppCalleeSaveRegisters();

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);  // Should generate code

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for PopGhcCalleeSaveRegisters
// Tests popq r15, popq r13, popq r12, popq r11, popq r10
HWTEST_F_L0(ExtendedAssemblerX64Test, PopGhcCalleeSaveRegisters)
{
    ExtendedAssembler masm(chunk_, nullptr);

    masm.PopGhcCalleeSaveRegisters();

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);  // Should generate code

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for PushAlignBytes
HWTEST_F_L0(ExtendedAssemblerX64Test, PushAlignBytes)
{
    ExtendedAssembler masm(chunk_, nullptr);

    masm.PushAlignBytes();

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);  // Should generate code

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for PopAlignBytes
HWTEST_F_L0(ExtendedAssemblerX64Test, PopAlignBytes)
{
    ExtendedAssembler masm(chunk_, nullptr);

    masm.PopAlignBytes();

    size_t size = masm.GetCurrentPosition();
    ASSERT_GT(size, 0U);  // Should generate code

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Comprehensive test combining all three target functions
HWTEST_F_L0(ExtendedAssemblerX64Test, ComprehensiveTest)
{
    ExtendedAssembler masm(chunk_, nullptr);

    // Test PushGhcCalleeSaveRegisters
    masm.PushGhcCalleeSaveRegisters();
    size_t size1 = masm.GetCurrentPosition();
    ASSERT_GT(size1, 0U);

    // Test UpdateCalleeSaveRegisters
    masm.UpdateCalleeSaveRegisters();
    size_t size2 = masm.GetCurrentPosition();
    ASSERT_GT(size2, size1);

    // Test PopCppCalleeSaveNoReservedRegisters
    masm.PopCppCalleeSaveNoReservedRegisters();
    size_t size3 = masm.GetCurrentPosition();
    ASSERT_GT(size3, size2);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size3);
}

// Test for register availability functions
HWTEST_F_L0(ExtendedAssemblerX64Test, RegisterAvailability)
{
    ExtendedAssembler masm(chunk_, nullptr);

    // Test AvailableRegister1 (should be r10)
    Register reg1 = masm.AvailableRegister1();
    ASSERT_EQ(reg1, r10);

    // Test AvailableRegister2 (should be r11)
    Register reg2 = masm.AvailableRegister2();
    ASSERT_EQ(reg2, r11);

    // Test ReserveRegister (should be r15)
    Register reg3 = masm.ReserveRegister();
    ASSERT_EQ(reg3, r15);

    // Test TempRegister (should be rax)
    {
        TempRegisterScope scope(&masm);
        Register tempReg = masm.TempRegister();
        ASSERT_EQ(tempReg, rax);
    }

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), masm.GetCurrentPosition());
}

// Test that the functions generate valid x86_64 instructions
HWTEST_F_L0(ExtendedAssemblerX64Test, InstructionGeneration)
{
    ExtendedAssembler masm(chunk_, nullptr);

    // Generate code for all the target functions
    masm.PushGhcCalleeSaveRegisters();
    masm.UpdateCalleeSaveRegisters();
    masm.PopCppCalleeSaveNoReservedRegisters();

    size_t totalSize = masm.GetCurrentPosition();
    ASSERT_GT(totalSize, 0U);

    // Verify the generated code is executable by disassembling
    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), totalSize);
}

}  // namespace panda::test