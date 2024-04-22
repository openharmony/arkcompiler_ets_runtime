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
#include "ecmascript/compiler/assembler/aarch64/assembler_aarch64.h"

namespace panda::ecmascript::kungfu {
using namespace panda::ecmascript;
void MacroAssemblerAArch64::Move(const StackSlotOperand &dstStackSlot, Immediate value)
{
    (void)dstStackSlot;
    (void)value;
}

void MacroAssemblerAArch64::Move(const StackSlotOperand &dstStackSlot,
                                 const StackSlotOperand &srcStackSlot)
{
    (void)dstStackSlot;
    (void)srcStackSlot;
}

void MacroAssemblerAArch64::Cmp(const StackSlotOperand &stackSlot, Immediate value)
{
    (void)stackSlot;
    (void)value;
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
    (void)funcAddress;
    (void)parameters;
}

void MacroAssemblerAArch64::SaveReturnRegister(const StackSlotOperand &dstStackSlot)
{
    (void)dstStackSlot;
}

}  // namespace panda::ecmascript::kungfu
