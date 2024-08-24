/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/compiler/baseline/baseline_assembler.h"
#include "ecmascript/compiler/assembler/x64/macro_assembler_x64.h"
#include "ecmascript/compiler/assembler/aarch64/macro_assembler_aarch64.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
BaselineAssembler::BaselineAssembler(const std::string &tripleStr) : stackOffsetDescriptor(0)
{
    NativeAreaAllocator allocator;
    Chunk chunk(&allocator);
    if (tripleStr.compare(TARGET_X64) == 0) {
        macroAssembler = new MacroAssemblerX64();
    } else if (tripleStr.compare(TARGET_AARCH64) == 0) {
        macroAssembler = new MacroAssemblerAArch64();
        static_cast<MacroAssemblerAArch64*>(macroAssembler)->SetBaselineFlag();
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
};

void BaselineAssembler::Move(VirtualRegister interpreterDestReg, Immediate value)
{
    StackSlotOperand stackSlotOperand(StackSlotOperand::BaseRegister::FRAME_REGISTER,
        stackOffsetDescriptor.GetVregOffset(interpreterDestReg));

    macroAssembler->Move(stackSlotOperand, value);
}

void BaselineAssembler::Move(SpecialRegister destReg, Immediate value)
{
    StackSlotOperand stackSlotOperand(StackSlotOperand::BaseRegister::FRAME_REGISTER,
        stackOffsetDescriptor.GetSpecialRegisterOffset(destReg));

    macroAssembler->Move(stackSlotOperand, value);
}

void BaselineAssembler::Move(SpecialRegister destReg, SpecialRegister srcReg)
{
    StackSlotOperand srcStackSlotOpnd(StackSlotOperand::BaseRegister::FRAME_REGISTER,
        stackOffsetDescriptor.GetSpecialRegisterOffset(srcReg));

    StackSlotOperand dstStackSlotOpnd(StackSlotOperand::BaseRegister::FRAME_REGISTER,
        stackOffsetDescriptor.GetSpecialRegisterOffset(destReg));
    macroAssembler->Move(dstStackSlotOpnd, srcStackSlotOpnd);
}

void BaselineAssembler::Move(VirtualRegister interpreterDestReg, VirtualRegister interpreterSrcReg)
{
    StackSlotOperand srcStackSlotOpnd(StackSlotOperand::BaseRegister::FRAME_REGISTER,
        stackOffsetDescriptor.GetVregOffset(interpreterSrcReg));

    StackSlotOperand dstStackSlotOpnd(StackSlotOperand::BaseRegister::FRAME_REGISTER,
        stackOffsetDescriptor.GetVregOffset(interpreterDestReg));

    macroAssembler->Move(dstStackSlotOpnd, srcStackSlotOpnd);
}

void BaselineAssembler::Move(SpecialRegister destReg, VirtualRegister interpreterSrcReg)
{
    StackSlotOperand srcStackSlotOpnd(StackSlotOperand::BaseRegister::FRAME_REGISTER,
        stackOffsetDescriptor.GetVregOffset(interpreterSrcReg));

    StackSlotOperand dstStackSlotOpnd(StackSlotOperand::BaseRegister::FRAME_REGISTER,
        stackOffsetDescriptor.GetSpecialRegisterOffset(destReg));

    macroAssembler->Move(dstStackSlotOpnd, srcStackSlotOpnd);
}

void BaselineAssembler::Move(VirtualRegister interpreterDestReg, SpecialRegister srcReg)
{
    StackSlotOperand srcStackSlotOpnd(StackSlotOperand::BaseRegister::FRAME_REGISTER,
        stackOffsetDescriptor.GetSpecialRegisterOffset(srcReg));

    StackSlotOperand dstStackSlotOpnd(StackSlotOperand::BaseRegister::FRAME_REGISTER,
        stackOffsetDescriptor.GetVregOffset(interpreterDestReg));

    macroAssembler->Move(dstStackSlotOpnd, srcStackSlotOpnd);
}

void BaselineAssembler::Cmp(SpecialRegister reg, Immediate value)
{
    StackSlotOperand stackSlotOperand(StackSlotOperand::BaseRegister::FRAME_REGISTER,
        stackOffsetDescriptor.GetSpecialRegisterOffset(reg));

    macroAssembler->Cmp(stackSlotOperand, value);
}

void BaselineAssembler::SaveResultIntoAcc()
{
    StackSlotOperand dstStackSlotOpnd(StackSlotOperand::BaseRegister::FRAME_REGISTER,
        stackOffsetDescriptor.GetSpecialRegisterOffset(SpecialRegister::ACC_REGISTER));

    macroAssembler->SaveReturnRegister(dstStackSlotOpnd);
}

void BaselineAssembler::CallBuiltin(Address funcAddress,
                                    const std::vector<BaselineParameter> &parameters)
{
    std::vector<MacroParameter> macroParameters;
    for (const auto &param : parameters) {
        if (std::holds_alternative<BaselineSpecialParameter>(param)) {
            switch (std::get<BaselineSpecialParameter>(param)) {
                case BaselineSpecialParameter::ACC: {
                    StackSlotOperand slotOpnd(StackSlotOperand::BaseRegister::FRAME_REGISTER,
                        stackOffsetDescriptor.GetSpecialRegisterOffset(SpecialRegister::ACC_REGISTER));
                    macroParameters.emplace_back(slotOpnd);
                    break;
                }
                case BaselineSpecialParameter::ENV: {
                    StackSlotOperand slotOpnd(StackSlotOperand::BaseRegister::FRAME_REGISTER,
                        stackOffsetDescriptor.GetSpecialRegisterOffset(SpecialRegister::ENV));
                    macroParameters.emplace_back(slotOpnd);
                    break;
                }
                default:
                    macroParameters.emplace_back(std::get<BaselineSpecialParameter>(param));
            }
        } else if (std::holds_alternative<int8_t>(param)) {
            macroParameters.emplace_back(std::get<int8_t>(param));
        } else if (std::holds_alternative<int16_t>(param)) {
            macroParameters.emplace_back(std::get<int16_t>(param));
        } else if (std::holds_alternative<int32_t>(param)) {
            macroParameters.emplace_back(std::get<int32_t>(param));
        } else if (std::holds_alternative<int64_t>(param)) {
            macroParameters.emplace_back(std::get<int64_t>(param));
        } else {
            VirtualRegister vReg = std::get<VirtualRegister>(param);
            StackSlotOperand slotOpnd(StackSlotOperand::BaseRegister::FRAME_REGISTER,
                stackOffsetDescriptor.GetVregOffset(vReg));
            macroParameters.emplace_back(slotOpnd);
        }
    }
    macroAssembler->CallBuiltin(funcAddress, macroParameters);
}

}  // namespace panda::ecmascript::kungfu
