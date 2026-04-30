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

#ifndef ECMASCRIPT_ARKSTEED_ARCH_ARM64_ASSEMBLER_ARM64_INL_H
#define ECMASCRIPT_ARKSTEED_ARCH_ARM64_ASSEMBLER_ARM64_INL_H

#include "ecmascript/arksteed/arksteed_assembler.h"

namespace panda::ecmascript::arksteed {
#if defined(PANDA_TARGET_ARM64)
// =============================================================================
// ARM64 Platform Constants
// =============================================================================

// Keep these scratch registers in sync with the x16/x17 exclusions in
// GetAllocatableGeneralRegisters() in arksteed_regalloc_types.h.
constexpr aarch64::Register kScratchRegister = aarch64::x16;
constexpr aarch64::Register kScratchRegister2 = aarch64::x17;
constexpr aarch64::DoubleRegister kScratchDoubleRegister = aarch64::d30;
constexpr aarch64::DoubleRegister kScratchDoubleRegister2 = aarch64::d31;

// Common registers
constexpr aarch64::Register kReturnRegister = aarch64::x0;
constexpr aarch64::Register kFramePointerRegister = aarch64::fp;
constexpr aarch64::Register kStackPointerRegister = aarch64::sp;
constexpr aarch64::Register kLinkRegister = aarch64::x30;

// =============================================================================
// ScratchRegisterScope - ARM64 Implementation
// =============================================================================

class ScratchRegisterScope {
public:
    explicit ScratchRegisterScope() = default;

    ~ScratchRegisterScope()
    {
        ASSERT(s_gprAcquired_ >= gprAcquiredByMe_);
        ASSERT(s_fprAcquired_ >= fprAcquiredByMe_);
        if (gprAcquiredByMe_ > 0) {
            s_gprAcquired_ -= gprAcquiredByMe_;
        }
        if (fprAcquiredByMe_ > 0) {
            s_fprAcquired_ -= fprAcquiredByMe_;
        }
        gprAcquiredByMe_ = 0;
        fprAcquiredByMe_ = 0;
    }

    aarch64::Register AcquireScratch()
    {
        ASSERT(s_gprAcquired_ < 2);  // 2: only two scratch GPRs available
        ASSERT(gprAcquiredByMe_ < 2);  // 2: only two scratch GPRs available
        gprAcquiredByMe_++;
        s_gprAcquired_++;
        return s_gprAcquired_ == 1 ? kScratchRegister : kScratchRegister2;
    }

    aarch64::DoubleRegister AcquireDoubleScratch()
    {
        ASSERT(s_fprAcquired_ < 2);  // 2: only two scratch FPRs available
        ASSERT(fprAcquiredByMe_ < 2);  // 2: only two scratch FPRs available
        fprAcquiredByMe_++;
        s_fprAcquired_++;
        return s_fprAcquired_ == 1 ? kScratchDoubleRegister : kScratchDoubleRegister2;
    }

private:
    uint8_t gprAcquiredByMe_ = 0;
    uint8_t fprAcquiredByMe_ = 0;
    static inline uint8_t s_gprAcquired_ = 0;
    static inline uint8_t s_fprAcquired_ = 0;
};

inline bool IsNegativeImmediateOffset(const aarch64::MemoryOperand &operand)
{
    return operand.IsImmediateOffset() && operand.GetAddrMode() == aarch64::AddrMode::OFFSET &&
           operand.GetImmediate().Value() < 0;
}

inline bool FitsScaledImmediateOffset(const aarch64::MemoryOperand &operand, bool is64Bit)
{
    if (!operand.IsImmediateOffset() || operand.GetAddrMode() != aarch64::AddrMode::OFFSET) {
        return false;
    }
    int64_t imm = operand.GetImmediate().Value();
    int64_t scale = is64Bit ? static_cast<int64_t>(sizeof(int64_t)) : static_cast<int64_t>(sizeof(int32_t));
    // 4095: max 12-bit unsigned immediate (0xFFF) for ARM64 load/store
    return imm >= 0 && (imm % scale) == 0 && (imm / scale) <= 4095;
}

inline bool FitsAddSubImmediate(uint64_t imm)
{
    constexpr uint64_t IMM12_MASK = (1ULL << 12U) - 1;
    if (imm <= IMM12_MASK) {
        return true;
    }
    return ((imm & IMM12_MASK) == 0) && ((imm & ~IMM12_MASK) <= IMM12_MASK);
}

inline bool FitsUnscaledImmediateOffset(const aarch64::MemoryOperand &operand)
{
    if (!operand.IsImmediateOffset()) {
        return false;
    }
    int64_t imm = operand.GetImmediate().Value();
    return imm >= -256 && imm <= 255;  // -256, 255: 9-bit signed immediate range for ARM64 load/store
}

inline aarch64::MemoryOperand MaterializeAddress(aarch64::AssemblerAarch64 &assembler,
                                                 const aarch64::MemoryOperand &operand)
{
    ASSERT(operand.IsImmediateOffset());
    ASSERT(operand.GetAddrMode() == aarch64::AddrMode::OFFSET);

    int64_t imm = operand.GetImmediate().Value();
    aarch64::Register base = operand.GetRegBase();
    if (imm == 0) {
        return aarch64::MemoryOperand(base, 0, aarch64::AddrMode::OFFSET);
    }

    ScratchRegisterScope scope;
    aarch64::Register scratch = scope.AcquireScratch();
    if (imm > 0 && FitsAddSubImmediate(static_cast<uint64_t>(imm))) {
        assembler.Add(scratch, base, aarch64::Operand(aarch64::Immediate(imm)));
    } else if (imm < 0 && FitsAddSubImmediate(static_cast<uint64_t>(-imm))) {
        assembler.Sub(scratch, base, aarch64::Operand(aarch64::Immediate(-imm)));
    } else {
        assembler.Mov(scratch, aarch64::Immediate(imm));
        assembler.Add(scratch, base, aarch64::Operand(scratch));
    }
    return aarch64::MemoryOperand(scratch, 0, aarch64::AddrMode::OFFSET);
}

inline void LoadRegisterWithOperand(aarch64::AssemblerAarch64 &assembler, const aarch64::Register &dst,
                                    const aarch64::MemoryOperand &src)
{
    if (FitsScaledImmediateOffset(src, !dst.IsW())) {
        assembler.Ldr(dst, src);
        return;
    }

    if (FitsUnscaledImmediateOffset(src)) {
        assembler.Ldur(dst, src);
        return;
    }

    assembler.Ldr(dst, MaterializeAddress(assembler, src));
}

inline void StoreRegisterWithOperand(aarch64::AssemblerAarch64 &assembler, const aarch64::Register &src,
                                     const aarch64::MemoryOperand &dst)
{
    if (FitsScaledImmediateOffset(dst, !src.IsW())) {
        assembler.Str(src, dst);
        return;
    }

    if (FitsUnscaledImmediateOffset(dst)) {
        assembler.Stur(src, dst);
        return;
    }

    assembler.Str(src, MaterializeAddress(assembler, dst));
}

aarch64::MemoryOperand ArkSteedAssembler::GetStackSlot(const AllocatedState &operand)
{
    return aarch64::MemoryOperand(aarch64::fp,
                                  GetFramePointerOffsetForStackSlot(operand.GetIndex(), operand.GetRepresentation()));
}

aarch64::MemoryOperand ArkSteedAssembler::ToMemOperand(const InstructionOperand &operand)
{
    return GetStackSlot(AllocatedState::Cast(operand));
}

aarch64::MemoryOperand ArkSteedAssembler::GetCallArgSlot(int32_t slotIndex)
{
    return aarch64::MemoryOperand(aarch64::sp, slotIndex * FRAME_SLOT_SIZE);
}

template <>
inline void ArkSteedAssembler::MoveRepr(MachineRepresentation repr, ArkSteedRegister dst, ArkSteedRegister src)
{
    Move(dst, src);
}

template <>
inline void ArkSteedAssembler::MoveRepr(MachineRepresentation repr, ArkSteedRegister dst, MemoryOperand src)
{
    switch (repr) {
        case MachineRepresentation::Word32:
            LoadRegisterWithOperand(assembler_, dst.W(), src);
            break;
        case MachineRepresentation::Tagged:
        case MachineRepresentation::Word64:
            LoadRegisterWithOperand(assembler_, dst, src);
            break;
        default:
            UNREACHABLE();
    }
}

template <>
inline void ArkSteedAssembler::MoveRepr(MachineRepresentation repr, MemoryOperand dst, ArkSteedRegister src)
{
    switch (repr) {
        case MachineRepresentation::Word32:
            return StoreRegisterWithOperand(assembler_, src.W(), dst);
        case MachineRepresentation::Tagged:
        case MachineRepresentation::Word64:
            return StoreRegisterWithOperand(assembler_, src, dst);
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
    constexpr ArkSteedRegister argRegisters[8] = {
        aarch64::x0, aarch64::x1, aarch64::x2, aarch64::x3,
        aarch64::x4, aarch64::x5, aarch64::x6, aarch64::x7
    };
    return argRegisters[i];
}

#endif

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ARCH_ARM64_ASSEMBLER_ARM64_INL_H
