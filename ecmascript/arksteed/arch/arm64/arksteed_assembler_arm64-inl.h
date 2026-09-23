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

// Common registers
constexpr aarch64::Register kReturnRegister = aarch64::x0;
constexpr aarch64::Register kFramePointerRegister = aarch64::fp;
constexpr aarch64::Register kStackPointerRegister = aarch64::sp;
constexpr aarch64::Register kLinkRegister = aarch64::x30;

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
    return ((imm & IMM12_MASK) == 0) && ((imm >> 12U) <= IMM12_MASK);
}

inline bool FitsUnscaledImmediateOffset(const aarch64::MemoryOperand &operand)
{
    if (!operand.IsImmediateOffset()) {
        return false;
    }
    int64_t imm = operand.GetImmediate().Value();
    return imm >= -256 && imm <= 255;  // -256, 255: 9-bit signed immediate range for ARM64 load/store
}

inline aarch64::MemoryOperand ArkSteedAssembler::MaterializeAddress(const aarch64::MemoryOperand &operand)
{
    ASSERT(operand.IsImmediateOffset());
    ASSERT(operand.GetAddrMode() == aarch64::AddrMode::OFFSET);

    int64_t imm = operand.GetImmediate().Value();
    aarch64::Register base = operand.GetRegBase();
    if (imm == 0) {
        return aarch64::MemoryOperand(base, 0, aarch64::AddrMode::OFFSET);
    }

    TemporaryRegisterScope scope(this);
    aarch64::Register scratch = scope.AcquireScratch();
    uint64_t magnitude = imm < 0 ? static_cast<uint64_t>(-(imm + 1)) + 1U : static_cast<uint64_t>(imm);
    if (imm > 0 && FitsAddSubImmediate(magnitude)) {
        EmitAddSubImmediate(scratch, base, magnitude, AddSubImmediateOp::ADD);
    } else if (imm < 0 && FitsAddSubImmediate(magnitude)) {
        EmitAddSubImmediate(scratch, base, magnitude, AddSubImmediateOp::SUB);
    } else {
        assembler_.Mov(scratch, aarch64::Immediate(imm));
        assembler_.Add(scratch, base, aarch64::Operand(scratch));
    }
    return aarch64::MemoryOperand(scratch, 0, aarch64::AddrMode::OFFSET);
}

inline void ArkSteedAssembler::LoadRegisterWithOperand(const aarch64::Register &dst, const aarch64::MemoryOperand &src)
{
    if (FitsScaledImmediateOffset(src, !dst.IsW())) {
        assembler_.Ldr(dst, src);
        return;
    }

    if (FitsUnscaledImmediateOffset(src)) {
        assembler_.Ldur(dst, src);
        return;
    }

    assembler_.Ldr(dst, MaterializeAddress(src));
}

inline void ArkSteedAssembler::StoreRegisterWithOperand(const aarch64::Register &src, const aarch64::MemoryOperand &dst)
{
    if (FitsScaledImmediateOffset(dst, !src.IsW())) {
        assembler_.Str(src, dst);
        return;
    }

    if (FitsUnscaledImmediateOffset(dst)) {
        assembler_.Stur(src, dst);
        return;
    }

    assembler_.Str(src, MaterializeAddress(dst));
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
            LoadRegisterWithOperand(dst.W(), src);
            break;
        case MachineRepresentation::Tagged:
        case MachineRepresentation::Word64:
            LoadRegisterWithOperand(dst, src);
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
            return StoreRegisterWithOperand(src.W(), dst);
        case MachineRepresentation::Tagged:
        case MachineRepresentation::Word64:
            return StoreRegisterWithOperand(src, dst);
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
    constexpr ArkSteedRegister argRegisters[8] = {aarch64::x0, aarch64::x1, aarch64::x2, aarch64::x3,
                                                  aarch64::x4, aarch64::x5, aarch64::x6, aarch64::x7};
    return argRegisters[i];
}

#endif

}  // namespace panda::ecmascript::arksteed

#endif  // ECMASCRIPT_ARKSTEED_ARCH_ARM64_ASSEMBLER_ARM64_INL_H
