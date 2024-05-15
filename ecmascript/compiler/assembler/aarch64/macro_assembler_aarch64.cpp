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

#include "ecmascript/compiler/assembler/aarch64/macro_assembler_aarch64.h"
#include "ecmascript/js_function.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
void MacroAssemblerAArch64::Move(const StackSlotOperand &dstStackSlot, Immediate value)
{
    aarch64::Register baseReg = (dstStackSlot.IsFrameBase()) ? aarch64::Register(aarch64::FP) :
                                                               aarch64::Register(aarch64::SP);
    aarch64::MemoryOperand dstOpnd(baseReg, static_cast<int64_t>(dstStackSlot.GetOffset()));
    assembler.Mov(LOCAL_SCOPE_REGISTER, aarch64::Immediate(value.GetValue()));
    assembler.Stur(LOCAL_SCOPE_REGISTER, dstOpnd);
}

void MacroAssemblerAArch64::Move(const StackSlotOperand &dstStackSlot,
                                 const StackSlotOperand &srcStackSlot)
{
    aarch64::Register dstBaseReg = (dstStackSlot.IsFrameBase()) ? aarch64::Register(aarch64::FP) :
                                                                  aarch64::Register(aarch64::SP);
    aarch64::MemoryOperand dstOpnd(dstBaseReg, static_cast<int64_t>(dstStackSlot.GetOffset()));
    aarch64::Register srcBaseReg = (srcStackSlot.IsFrameBase()) ? aarch64::Register(aarch64::FP) :
                                                                  aarch64::Register(aarch64::SP);
    aarch64::MemoryOperand srcOpnd(srcBaseReg, static_cast<int64_t>(srcStackSlot.GetOffset()));
    assembler.Ldur(LOCAL_SCOPE_REGISTER, srcOpnd);
    assembler.Stur(LOCAL_SCOPE_REGISTER, dstOpnd);
}

void MacroAssemblerAArch64::Cmp(const StackSlotOperand &stackSlot, Immediate value)
{
    aarch64::Register baseReg = (stackSlot.IsFrameBase()) ? aarch64::Register(aarch64::FP) :
                                                            aarch64::Register(aarch64::SP);
    aarch64::MemoryOperand opnd(baseReg, static_cast<int64_t>(stackSlot.GetOffset()));
    aarch64::Operand immOpnd = aarch64::Immediate(value.GetValue());
    assembler.Ldur(LOCAL_SCOPE_REGISTER, opnd);
    assembler.Cmp(LOCAL_SCOPE_REGISTER, immOpnd);
}

void MacroAssemblerAArch64::Bind(JumpLabel &label)
{
    assembler.Bind(&label);
}

void MacroAssemblerAArch64::Jz(JumpLabel &label)
{
    assembler.B(aarch64::EQ, &label);
}

void MacroAssemblerAArch64::Jnz(JumpLabel &label)
{
    assembler.B(aarch64::NE, &label);
}

void MacroAssemblerAArch64::Jump(JumpLabel &label)
{
    assembler.B(&label);
}

void MacroAssemblerAArch64::CallBuiltin(Address funcAddress,
                                        const std::vector<MacroParameter> &parameters)
{
    for (size_t i = 0; i < parameters.size(); ++i) {
        auto param = parameters[i];
        if (i == PARAM_REGISTER_COUNT) {
            std::cout << "not support aarch64 baseline stack parameter " << std::endl;
            std::abort();
            break;
        }
        MovParameterIntoParamReg(param, registerParamVec[i]);
    }
    assembler.Mov(LOCAL_SCOPE_REGISTER, aarch64::Immediate(funcAddress));
    assembler.Blr(LOCAL_SCOPE_REGISTER);
}

void MacroAssemblerAArch64::SaveReturnRegister(const StackSlotOperand &dstStackSlot)
{
    aarch64::Register baseReg = (dstStackSlot.IsFrameBase()) ? aarch64::Register(aarch64::FP) :
                                                               aarch64::Register(aarch64::SP);
    aarch64::MemoryOperand dstOpnd(baseReg, static_cast<int64_t>(dstStackSlot.GetOffset()));
    assembler.Stur(RETURN_REGISTER, dstOpnd);
}

void MacroAssemblerAArch64::MovParameterIntoParamReg(MacroParameter param, aarch64::Register paramReg)
{
    if (std::holds_alternative<BaselineSpecialParameter>(param)) {
        auto specialParam = std::get<BaselineSpecialParameter>(param);
        switch (specialParam) {
            case BaselineSpecialParameter::GLUE: {
                assembler.Mov(paramReg, GLUE_REGISTER);
                break;
            }
            case BaselineSpecialParameter::PROFILE_TYPE_INFO: {
                assembler.Ldur(LOCAL_SCOPE_REGISTER,
                               aarch64::MemoryOperand(aarch64::Register(aarch64::X29),
                                                      static_cast<int64_t>(FUNCTION_OFFSET_FROM_SP)));
                assembler.Ldr(paramReg,
                              aarch64::MemoryOperand(LOCAL_SCOPE_REGISTER, JSFunction::PROFILE_TYPE_INFO_OFFSET));
                break;
            }
            case BaselineSpecialParameter::SP: {
                assembler.Mov(paramReg, aarch64::Register(aarch64::X29));
                break;
            }
            case BaselineSpecialParameter::HOTNESS_COUNTER: {
                assembler.Ldur(LOCAL_SCOPE_REGISTER, aarch64::MemoryOperand(aarch64::Register(aarch64::X29),
                    static_cast<int64_t>(FUNCTION_OFFSET_FROM_SP)));
                assembler.Ldr(LOCAL_SCOPE_REGISTER,
                              aarch64::MemoryOperand(LOCAL_SCOPE_REGISTER, JSFunctionBase::METHOD_OFFSET));
                assembler.Ldr(paramReg,
                              aarch64::MemoryOperand(LOCAL_SCOPE_REGISTER, Method::LITERAL_INFO_OFFSET));
                break;
            }
            default: {
                std::cout << "not supported other BaselineSpecialParameter currently" << std::endl;
                std::abort();
            }
        }
        return;
    }
    if (std::holds_alternative<int8_t>(param)) {
        int16_t num = std::get<int8_t>(param);
        assembler.Mov(paramReg, aarch64::Immediate(static_cast<int64_t>(num)));
        return;
    }
    if (std::holds_alternative<int16_t>(param)) {
        int16_t num = std::get<int16_t>(param);
        assembler.Mov(paramReg, aarch64::Immediate(static_cast<int64_t>(num)));
        return;
    }
    if (std::holds_alternative<int32_t>(param)) {
        int32_t num = std::get<int32_t>(param);
        assembler.Mov(paramReg, aarch64::Immediate(static_cast<int64_t>(num)));
        return;
    }
    if (std::holds_alternative<int64_t>(param)) {
        int64_t num = std::get<int64_t>(param);
        assembler.Mov(paramReg, aarch64::Immediate(static_cast<int64_t>(num)));
        return;
    }
    if (std::holds_alternative<StackSlotOperand>(param)) {
        StackSlotOperand stackSlotOpnd = std::get<StackSlotOperand>(param);
        aarch64::Register dstBaseReg = (stackSlotOpnd.IsFrameBase()) ? aarch64::Register(aarch64::FP) :
                                                                       aarch64::Register(aarch64::SP);
        aarch64::MemoryOperand paramOpnd(dstBaseReg, static_cast<int64_t>(stackSlotOpnd.GetOffset()));
        assembler.Ldur(paramReg, paramOpnd);
        return;
    }
    std::cout << "not supported other type of aarch64 baseline parameters currently" << std::endl;
    std::abort();
}

}  // namespace panda::ecmascript::kungfu
