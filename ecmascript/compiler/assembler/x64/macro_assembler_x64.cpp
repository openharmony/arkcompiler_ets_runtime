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

#include "ecmascript/compiler/assembler/x64/macro_assembler_x64.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;

void MacroAssemblerX64::Move(const StackSlotOperand &dstStackSlot, Immediate value)
{
    x64::Register baseReg = (dstStackSlot.IsFrameBase()) ? x64::rbp : x64::rsp;
    x64::Operand dstOpnd(baseReg, dstStackSlot.GetOffset());
    assembler.Movabs(value.GetValue(), LOCAL_SCOPE_REGISTER);
    assembler.Movq(LOCAL_SCOPE_REGISTER, dstOpnd);
}

void MacroAssemblerX64::Move(const StackSlotOperand &dstStackSlot,
                             const StackSlotOperand &srcStackSlot)
{
    x64::Register dstBaseReg = (dstStackSlot.IsFrameBase()) ? x64::rbp : x64::rsp;
    x64::Register srcBaseReg = (srcStackSlot.IsFrameBase()) ? x64::rbp : x64::rsp;
    x64::Operand srcOpnd(srcBaseReg, srcStackSlot.GetOffset());
    x64::Operand dstOpnd(dstBaseReg, dstStackSlot.GetOffset());
    assembler.Movq(srcOpnd, LOCAL_SCOPE_REGISTER);
    assembler.Movq(LOCAL_SCOPE_REGISTER, dstOpnd);
}

void MacroAssemblerX64::Cmp(const StackSlotOperand &stackSlot, Immediate value)
{
    x64::Register baseReg = (stackSlot.IsFrameBase()) ? x64::rbp : x64::rsp;
    x64::Operand opnd(baseReg, stackSlot.GetOffset());
    assembler.Movq(opnd, LOCAL_SCOPE_REGISTER);
    assembler.Cmp(x64::Immediate(value.GetValue()), LOCAL_SCOPE_REGISTER);
}

void MacroAssemblerX64::Bind(JumpLabel &label)
{
    assembler.Bind(&label);
}

void MacroAssemblerX64::Jz(JumpLabel &label)
{
    assembler.Jz(&label);
}

void MacroAssemblerX64::Jnz(JumpLabel &label)
{
    assembler.Jnz(&label);
}

void MacroAssemblerX64::Jump(JumpLabel &label)
{
    assembler.Jmp(&label);
}

void MacroAssemblerX64::SaveReturnRegister(const StackSlotOperand &dstStackSlot)
{
    x64::Register dstBaseReg = (dstStackSlot.IsFrameBase()) ? x64::rbp : x64::rsp;
    x64::Operand dstOpnd(dstBaseReg, dstStackSlot.GetOffset());
    assembler.Movq(RETURN_REGISTER, dstOpnd);
}

void MacroAssemblerX64::MovParameterIntoParamReg(MacroParameter param, x64::Register paramReg)
{
    if (std::holds_alternative<BaselineSpecialParameter>(param)) {
        auto specialParam = std::get<BaselineSpecialParameter>(param);
        switch (specialParam) {
            case BaselineSpecialParameter::GLUE: {
                assembler.Movq(GLUE_REGISTER, paramReg);
                break;
            }
            case BaselineSpecialParameter::PROFILE_TYPE_INFO: {
                assembler.Movq(x64::Operand(x64::rbp, FUNCTION_OFFSET_FROM_SP), LOCAL_SCOPE_REGISTER);
                assembler.Movq(
                    x64::Operand(LOCAL_SCOPE_REGISTER, JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET),
                    LOCAL_SCOPE_REGISTER);
                assembler.Movq(x64::Operand(LOCAL_SCOPE_REGISTER, ProfileTypeInfoCell::VALUE_OFFSET), paramReg);
                break;
            }
            case BaselineSpecialParameter::SP: {
                assembler.Movq(x64::rbp, paramReg);
                break;
            }
            case BaselineSpecialParameter::HOTNESS_COUNTER: {
                assembler.Movq(x64::Operand(x64::rbp, FUNCTION_OFFSET_FROM_SP), LOCAL_SCOPE_REGISTER);
                assembler.Movq(
                    x64::Operand(LOCAL_SCOPE_REGISTER, JSFunctionBase::METHOD_OFFSET), LOCAL_SCOPE_REGISTER);
                assembler.Movzwq(x64::Operand(LOCAL_SCOPE_REGISTER, Method::LITERAL_INFO_OFFSET), paramReg);
                break;
            }
            default: {
                std::cout << "not supported BaselineSpecialParameter currently" << std::endl;
                std::abort();
            }
        }
        return;
    }
    if (std::holds_alternative<int8_t>(param)) {
        int16_t num = std::get<int8_t>(param);
        assembler.Movq(panda::ecmascript::x64::Immediate(static_cast<int32_t>(num)), paramReg);
        return;
    }
    if (std::holds_alternative<int16_t>(param)) {
        int16_t num = std::get<int16_t>(param);
        assembler.Movq(panda::ecmascript::x64::Immediate(static_cast<int32_t>(num)), paramReg);
        return;
    }
    if (std::holds_alternative<int32_t>(param)) {
        int32_t num = std::get<int32_t>(param);
        assembler.Movq(panda::ecmascript::x64::Immediate(num), paramReg);
        return;
    }
    if (std::holds_alternative<int64_t>(param)) {
        int64_t num = std::get<int64_t>(param);
        assembler.Movabs(num, paramReg);
        return;
    }
    if (std::holds_alternative<StackSlotOperand>(param)) {
        StackSlotOperand stackSlotOpnd = std::get<StackSlotOperand>(param);
        x64::Register dstBaseReg = (stackSlotOpnd.IsFrameBase()) ? x64::rbp : x64::rsp;
        x64::Operand paramOpnd(dstBaseReg, stackSlotOpnd.GetOffset());
        assembler.Movq(paramOpnd, paramReg);
        return;
    }
    std::cout << "not supported other type of baseline parameters currently" << std::endl;
    std::abort();
}

void MacroAssemblerX64::CallBuiltin(Address funcAddress,
                                    const std::vector<MacroParameter> &parameters)
{
    for (size_t i = 0; i < parameters.size(); ++i) {
        auto param = parameters[i];
        if (i == PARAM_REGISTER_COUNT) {
            std::cout << "should not pass parameters on the stack" << std::endl;
            std::abort();
        }
        MovParameterIntoParamReg(param, registerParamVec[i]);
    }
    assembler.Movabs(static_cast<uint64_t>(funcAddress), LOCAL_SCOPE_REGISTER);
    assembler.Callq(LOCAL_SCOPE_REGISTER);
}

}  // namespace panda::ecmascript::kungfu
