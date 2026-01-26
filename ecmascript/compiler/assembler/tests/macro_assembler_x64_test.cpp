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

#include "ecmascript/compiler/assembler/x64/macro_assembler_x64.h"

#include <iostream>
#include <ostream>

#include "ecmascript/compiler/assembler/x64/extended_assembler_x64.h"
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

class MacroAssemblerX64Test : public testing::Test {
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

    void DisassembleChunk(const char *triple, MacroAssemblerX64 *masm, std::ostream &os)
    {
        LLVMDisasmContextRef dcr = LLVMCreateDisasm(triple, nullptr, 0, nullptr, SymbolLookupCallback);
        uint8_t *byteSp = masm->GetBegin();
        size_t numBytes = masm->GetBufferCurrentSize();
        unsigned pc = 0;
        constexpr size_t outStringSize = 100;
        constexpr uint32_t hexFieldWidth = 8;
        char outString[outStringSize];
        while (numBytes > 0) {
            size_t instSize = LLVMDisasmInstruction(dcr, byteSp, numBytes, pc, outString, outStringSize);
            if (instSize == 0) {
                constexpr size_t wordSize = sizeof(uint32_t);
                os << std::setw(hexFieldWidth) << std::setfill('0') << std::hex << pc << ":" << std::setw(hexFieldWidth)
                   << *reinterpret_cast<uint32_t *>(byteSp) << "maybe constant" << std::endl;
                pc += wordSize;
                byteSp += wordSize;
                numBytes -= wordSize;
            }
            os << std::setw(hexFieldWidth) << std::setfill('0') << std::hex << pc << ":" << std::setw(hexFieldWidth)
               << *reinterpret_cast<uint32_t *>(byteSp) << " " << outString << std::endl;
            pc += instSize;
            byteSp += instSize;
            numBytes -= instSize;
        }
        LLVMDisasmDispose(dcr);
    }

    EcmaVM *instance {nullptr};
    JSThread *thread {nullptr};
    EcmaHandleScope *scope {nullptr};
    Chunk *chunk_ {nullptr};
};

// Test for Move(const StackSlotOperand &dstStackSlot, Immediate value)
HWTEST_F_L0(MacroAssemblerX64Test, MoveStackSlotImmediate)
{
    MacroAssemblerX64 masm;
    StackSlotOperand dstSlot(StackSlotOperand::BaseRegister::STACK_REGISTER, 0x10);
    kungfu::Immediate immValue(0x12345678);

    masm.Move(dstSlot, immValue);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Move(const StackSlotOperand &dstStackSlot, const StackSlotOperand &srcStackSlot)
HWTEST_F_L0(MacroAssemblerX64Test, MoveStackSlotToStackSlot)
{
    MacroAssemblerX64 masm;
    StackSlotOperand srcSlot(StackSlotOperand::BaseRegister::STACK_REGISTER, 0x8);
    StackSlotOperand dstSlot(StackSlotOperand::BaseRegister::STACK_REGISTER, 0x10);

    masm.Move(dstSlot, srcSlot);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Move with FrameBase register
HWTEST_F_L0(MacroAssemblerX64Test, MoveWithFrameBase)
{
    MacroAssemblerX64 masm;
    StackSlotOperand dstSlot(StackSlotOperand::BaseRegister::FRAME_REGISTER, 0x20);
    kungfu::Immediate immValue(0xDEADBEEF);

    masm.Move(dstSlot, immValue);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Cmp(const StackSlotOperand &stackSlot, Immediate value)
HWTEST_F_L0(MacroAssemblerX64Test, CmpStackSlotImmediate)
{
    MacroAssemblerX64 masm;
    StackSlotOperand slot(StackSlotOperand::BaseRegister::STACK_REGISTER, 0x10);
    kungfu::Immediate immValue(0xABCD);

    masm.Cmp(slot, immValue);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Bind, Jz, Jnz, Jump
HWTEST_F_L0(MacroAssemblerX64Test, BindAndBranch)
{
    MacroAssemblerX64 masm;
    JumpLabel label1;
    JumpLabel label2;

    // Test Bind
    masm.Bind(label1);
    ASSERT_TRUE(label1.IsBound());

    // Test Jump
    masm.Jump(label2);

    // Bind another label
    masm.Bind(label2);
    ASSERT_TRUE(label2.IsBound());

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jz (jump if zero)
HWTEST_F_L0(MacroAssemblerX64Test, JzTest)
{
    MacroAssemblerX64 masm;
    JumpLabel label;

    masm.Jz(label);
    ASSERT_TRUE(label.IsLinked());

    masm.Bind(label);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for Jnz (jump if not zero)
HWTEST_F_L0(MacroAssemblerX64Test, JnzTest)
{
    MacroAssemblerX64 masm;
    JumpLabel label;

    masm.Jnz(label);
    ASSERT_TRUE(label.IsLinked());

    masm.Bind(label);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for SaveReturnRegister
HWTEST_F_L0(MacroAssemblerX64Test, SaveReturnRegister)
{
    MacroAssemblerX64 masm;
    StackSlotOperand dstSlot(StackSlotOperand::BaseRegister::STACK_REGISTER, 0x8);

    masm.SaveReturnRegister(dstSlot);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for CallBuiltin with immediate parameter (indirectly tests MovParameterIntoParamReg)
HWTEST_F_L0(MacroAssemblerX64Test, CallBuiltinWithImmediate)
{
    MacroAssemblerX64 masm;
    // Use a simple address for testing
    Address testFuncAddr = 0x12345678ABCD;
    std::vector<MacroParameter> params;
    params.push_back(static_cast<int32_t>(42));  // Tests int32_t path in MovParameterIntoParamReg

    masm.CallBuiltin(testFuncAddr, params);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test CallBuiltin with int8_t parameter
HWTEST_F_L0(MacroAssemblerX64Test, CallBuiltinWithInt8Param)
{
    MacroAssemblerX64 masm;
    Address testFuncAddr = 0x12345678ABCD;
    std::vector<MacroParameter> params;
    params.push_back(static_cast<int8_t>(-5));  // Tests int8_t path

    masm.CallBuiltin(testFuncAddr, params);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test CallBuiltin with int16_t parameter
HWTEST_F_L0(MacroAssemblerX64Test, CallBuiltinWithInt16Param)
{
    MacroAssemblerX64 masm;
    Address testFuncAddr = 0x12345678ABCD;
    std::vector<MacroParameter> params;
    params.push_back(static_cast<int16_t>(1000));  // Tests int16_t path

    masm.CallBuiltin(testFuncAddr, params);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test CallBuiltin with int64_t parameter
HWTEST_F_L0(MacroAssemblerX64Test, CallBuiltinWithInt64Param)
{
    MacroAssemblerX64 masm;
    Address testFuncAddr = 0x12345678ABCD;
    std::vector<MacroParameter> params;
    params.push_back(static_cast<int64_t>(0x1234567890ABCDEF));  // Tests int64_t path

    masm.CallBuiltin(testFuncAddr, params);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test CallBuiltin with StackSlotOperand parameter
HWTEST_F_L0(MacroAssemblerX64Test, CallBuiltinWithStackSlotParam)
{
    MacroAssemblerX64 masm;
    Address testFuncAddr = 0x12345678ABCD;
    std::vector<MacroParameter> params;
    StackSlotOperand slot(StackSlotOperand::BaseRegister::STACK_REGISTER, 0x20);
    params.push_back(slot);  // Tests StackSlotOperand path

    masm.CallBuiltin(testFuncAddr, params);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test CallBuiltin with multiple parameters (tests all parameter registers)
HWTEST_F_L0(MacroAssemblerX64Test, CallBuiltinWithMultipleParams)
{
    MacroAssemblerX64 masm;
    Address testFuncAddr = 0x12345678ABCD;
    std::vector<MacroParameter> params;
    params.push_back(static_cast<int32_t>(1));
    params.push_back(static_cast<int32_t>(2));
    params.push_back(static_cast<int32_t>(3));
    params.push_back(static_cast<int32_t>(4));
    params.push_back(static_cast<int32_t>(5));
    params.push_back(static_cast<int32_t>(6));  // 6 params fill all param registers

    masm.CallBuiltin(testFuncAddr, params);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test CallBuiltin with BaselineSpecialParameter::GLUE (indirectly tests MovParameterIntoParamReg)
HWTEST_F_L0(MacroAssemblerX64Test, CallBuiltinWithGlueParam)
{
    MacroAssemblerX64 masm;
    Address testFuncAddr = 0x12345678ABCD;
    std::vector<MacroParameter> params;
    params.push_back(BaselineSpecialParameter::GLUE);  // Tests GLUE path

    masm.CallBuiltin(testFuncAddr, params);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test CallBuiltin with BaselineSpecialParameter::SP (indirectly tests MovParameterIntoParamReg)
HWTEST_F_L0(MacroAssemblerX64Test, CallBuiltinWithSpParam)
{
    MacroAssemblerX64 masm;
    Address testFuncAddr = 0x12345678ABCD;
    std::vector<MacroParameter> params;
    params.push_back(BaselineSpecialParameter::SP);  // Tests SP path

    masm.CallBuiltin(testFuncAddr, params);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test CallBuiltin with FrameBase StackSlotOperand
HWTEST_F_L0(MacroAssemblerX64Test, CallBuiltinWithFrameBaseStackSlot)
{
    MacroAssemblerX64 masm;
    Address testFuncAddr = 0x12345678ABCD;
    std::vector<MacroParameter> params;
    StackSlotOperand slot(StackSlotOperand::BaseRegister::FRAME_REGISTER, 0x30);
    params.push_back(slot);  // Tests FrameBase StackSlotOperand path

    masm.CallBuiltin(testFuncAddr, params);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Comprehensive test combining multiple operations
HWTEST_F_L0(MacroAssemblerX64Test, ComprehensiveTest)
{
    MacroAssemblerX64 masm;

    JumpLabel start;
    JumpLabel end;
    JumpLabel skip;

    masm.Bind(start);

    // Move immediate to stack slot
    StackSlotOperand slot1(StackSlotOperand::BaseRegister::STACK_REGISTER, 0x10);
    masm.Move(slot1, kungfu::Immediate(0x100));

    // Compare with immediate
    masm.Cmp(slot1, kungfu::Immediate(0x100));

    // Conditional jump
    masm.Jz(end);
    masm.Jnz(skip);

    // Bind skip label
    masm.Bind(skip);

    // Save return register
    StackSlotOperand slot2(StackSlotOperand::BaseRegister::STACK_REGISTER, 0x18);
    masm.SaveReturnRegister(slot2);

    // Unconditional jump to end
    masm.Jump(end);

    // Bind end label
    masm.Bind(end);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for CallBuiltin with PROFILE_TYPE_INFO parameter
HWTEST_F_L0(MacroAssemblerX64Test, CallBuiltinWithProfileTypeInfoParam)
{
    MacroAssemblerX64 masm;
    Address testFuncAddr = 0x12345678ABCD;
    std::vector<MacroParameter> params;
    params.push_back(BaselineSpecialParameter::PROFILE_TYPE_INFO);

    masm.CallBuiltin(testFuncAddr, params);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

// Test for CallBuiltin with HOTNESS_COUNTER parameter
HWTEST_F_L0(MacroAssemblerX64Test, CallBuiltinWithHotnessCounterParam)
{
    MacroAssemblerX64 masm;
    Address testFuncAddr = 0x12345678ABCD;
    std::vector<MacroParameter> params;
    params.push_back(BaselineSpecialParameter::HOTNESS_COUNTER);

    masm.CallBuiltin(testFuncAddr, params);

    size_t size = masm.GetBufferCurrentSize();
    ASSERT_GT(size, 0);

    ecmascript::kungfu::LLVMAssembler::Disassemble(nullptr, TARGET_X64,
                                                   masm.GetBegin(), size);
}

}  // namespace panda::test