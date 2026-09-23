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

#ifndef ECMASCRIPT_ARKSTEED_ARCH_X64_ASSEMBLER_X64_INL_H
#define ECMASCRIPT_ARKSTEED_ARCH_X64_ASSEMBLER_X64_INL_H

#include "ecmascript/arksteed/arksteed_assembler.h"

namespace panda::ecmascript::arksteed {
#if defined(PANDA_TARGET_AMD64)

// =============================================================================
// x64 Platform Constants
// =============================================================================

// Common registers
constexpr x64::Register kReturnRegister = x64::rax;
constexpr x64::Register kFramePointerRegister = x64::rbp;
constexpr x64::Register kStackPointerRegister = x64::rsp;

x64::Operand ArkSteedAssembler::GetStackSlot(const AllocatedState &operand)
{
    return x64::Operand(x64::rbp, GetFramePointerOffsetForStackSlot(operand.GetIndex(), operand.GetRepresentation()));
}
x64::Operand ArkSteedAssembler::ToMemOperand(const InstructionOperand &operand)
{
    return GetStackSlot(AllocatedState::Cast(operand));
}
x64::Operand ArkSteedAssembler::GetCallArgSlot(int32_t slotIndex)
{
    return x64::Operand(x64::rsp, slotIndex * FRAME_SLOT_SIZE);
}

template <typename Dest, typename Source>
void ArkSteedAssembler::MoveRepr(MachineRepresentation repr, Dest dst, Source src)
{
    switch (repr) {
        case MachineRepresentation::Word32:
            return assembler_.Movl(src, dst);
        case MachineRepresentation::Word64:
        case MachineRepresentation::Tagged:
            return assembler_.Movq(src, dst);
        default:
            UNREACHABLE();
    }
}
template <>
inline void ArkSteedAssembler::MoveRepr(MachineRepresentation repr, MemoryOperand dst, MemoryOperand src)
{
    TemporaryRegisterScope scope(this);
    ArkSteedRegister scratch = scope.AcquireScratch();
    MoveRepr(repr, scratch, src);
    MoveRepr(repr, dst, scratch);
}

constexpr ArkSteedRegister ArkSteedAssembler::GetParameterRegister(int i)
{
    ASSERT(i >= 0 && i < NUM_ARG_REGISTERS);
    constexpr ArkSteedRegister argRegisters[6] = {x64::rdi, x64::rsi, x64::rdx, x64::rcx, x64::r8, x64::r9};
    return argRegisters[i];
}

#endif
}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ARCH_X64_ASSEMBLER_X64_INL_H
