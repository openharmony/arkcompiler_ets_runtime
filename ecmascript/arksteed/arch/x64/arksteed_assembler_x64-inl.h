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

constexpr x64::Register X64_SCRATCH_REGISTER = x64::r10;
constexpr x64::DoubleRegister X64_SCRATCH_DOUBLE_REGISTER = x64::xmm15;

// Common registers
constexpr x64::Register kReturnRegister = x64::rax;
constexpr x64::Register kFramePointerRegister = x64::rbp;
constexpr x64::Register kStackPointerRegister = x64::rsp;

// =============================================================================
// ScratchRegisterScope - x64 Implementation
// =============================================================================

class ScratchRegisterScope {
public:
    explicit ScratchRegisterScope() = default;

    ~ScratchRegisterScope()
    {
#ifndef NDEBUG
        if (gprAcquiredByMe) {
            s_gprAcquired = false;
        }
        if (fprAcquiredByMe) {
            s_fprAcquired = false;
        }
#endif
    }

    x64::Register AcquireScratch()
    {
#ifndef NDEBUG
        ASSERT(!s_gprAcquired);
        gprAcquiredByMe = true;
        s_gprAcquired = true;
#endif
        return X64_SCRATCH_REGISTER;
    }

    x64::DoubleRegister AcquireDoubleScratch()
    {
#ifndef NDEBUG
        ASSERT(!s_fprAcquired);
        fprAcquiredByMe = true;
        s_fprAcquired = true;
#endif
        return X64_SCRATCH_DOUBLE_REGISTER;
    }

private:
#ifndef NDEBUG
    static inline bool s_gprAcquired = false;
    static inline bool s_fprAcquired = false;

    bool gprAcquiredByMe = false;
    bool fprAcquiredByMe = false;
#endif
};

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
    ScratchRegisterScope scope;
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
