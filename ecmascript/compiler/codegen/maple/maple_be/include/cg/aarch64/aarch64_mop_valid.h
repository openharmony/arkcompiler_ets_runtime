/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MOP_VALID_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MOP_VALID_H
#include "aarch64_imm_valid.h"
#include "aarch64_isa.h"
#include "mempool_allocator.h"
#include "operand.h"

namespace maplebe {
// Immediate verification for a byte from/to memory. simm: -256 ~ 255; pimm: 0 ~ 4095.
inline bool StrLdr8Valid(Operand *o)
{
    return (static_cast<MemOperand *>(o)->GetAddrMode() == MemOperand::kAddrModeLo12Li) ||
           StrLdrInsnSignedOfstValid(AArch64isa::GetMemOpndOffsetValue(o), k0ByteSize,
                                     static_cast<MemOperand *>(o)->IsIntactIndexed());
}

// Immediate verification for half word from/to memory. simm: -256 ~ 255; pimm: 0 ~ 8190, multiple of 2.
inline bool StrLdr16Valid(Operand *o)
{
    MemOperand *memOpnd = static_cast<MemOperand *>(o);
    if (memOpnd->GetAddrMode() == MemOperand::kAddrModeLo12Li) {
        uint8 symAlign = 0;
        const MIRSymbol *sym = memOpnd->GetSymbol();
        if (sym) {
            symAlign = 1U << sym->GetSymbolAlign();
        }
        return ((symAlign + static_cast<uint64>(AArch64isa::GetMemOpndOffsetValue(o))) &
                static_cast<int64>(k1BitSize)) == static_cast<int64>(k0BitSize);
    }
    return StrLdrInsnSignedOfstValid(AArch64isa::GetMemOpndOffsetValue(o), k1ByteSize, memOpnd->IsIntactIndexed());
}

// Immediate verification for a word from/to memory. simm: -256 ~ 255; pimm: 0 ~ 16380, multiple of 4.
inline bool StrLdr32Valid(Operand *o)
{
    MemOperand *memOpnd = static_cast<MemOperand *>(o);
    if (memOpnd->GetAddrMode() == MemOperand::kAddrModeLo12Li) {
        uint8 symAlign = 0;
        const MIRSymbol *sym = memOpnd->GetSymbol();
        if (sym) {
            symAlign = 1U << sym->GetSymbolAlign();
        }
        return ((symAlign + static_cast<uint64>(AArch64isa::GetMemOpndOffsetValue(o))) &
                static_cast<int64>(k3BitSize)) == static_cast<int64>(k0BitSize);
    }
    return StrLdrInsnSignedOfstValid(AArch64isa::GetMemOpndOffsetValue(o), k2ByteSize, memOpnd->IsIntactIndexed());
}

// Immediate verification: value range -256 ~ 252, multiple of 4.
inline bool StrLdr32PairValid(Operand *o)
{
    constexpr uint64 sImmValidOffset = 3;
    int64 value = AArch64isa::GetMemOpndOffsetValue(o);
    if ((value <= kMaxSimm32Pair) && (value >= kMinSimm32)) {
        return (static_cast<uint64>(value) & sImmValidOffset) > 0 ? false : true;
    }
    return false;
}

// Immediate verification for 2 words from/to memory. simm: -256 ~ 255; pimm: 0 ~ 32760, multiple of 8.
inline bool StrLdr64Valid(Operand *o)
{
    MemOperand *memOpnd = static_cast<MemOperand *>(o);
    if (memOpnd->GetAddrMode() == MemOperand::kAddrModeLo12Li) {
        uint8 symAlign = 0;
        const MIRSymbol *sym = memOpnd->GetSymbol();
        if (sym) {
            symAlign = 1U << sym->GetSymbolAlign();
        }
        return ((symAlign + static_cast<uint64>(AArch64isa::GetMemOpndOffsetValue(o))) &
                static_cast<int64>(k7BitSize)) == static_cast<int64>(k0BitSize);
    }
    return StrLdrInsnSignedOfstValid(AArch64isa::GetMemOpndOffsetValue(o), k3ByteSize, memOpnd->IsIntactIndexed());
}

// Immediate verification: value range -512 ~ 504, multiple of 8.
inline bool StrLdr64PairValid(Operand *o)
{
    constexpr uint64 sImmValidOffset = 7;
    int64 value = AArch64isa::GetMemOpndOffsetValue(o);
    if (value <= kMaxSimm64Pair && (value >= kMinSimm64)) {
        return (static_cast<uint64>(value) & sImmValidOffset) > 0 ? false : true;
    }
    return false;
}

// Immediate verification for 4 words from/to memory. simm: -256 ~ 255; pimm: 0 ~ 65520, multiple of 16.
inline bool StrLdr128Valid(Operand *o)
{
    MemOperand *memOpnd = static_cast<MemOperand *>(o);
    if (memOpnd->GetAddrMode() == MemOperand::kAddrModeLo12Li) {
        uint8 symAlign = 0;
        const MIRSymbol *sym = memOpnd->GetSymbol();
        if (sym && sym->IsConst()) {
            symAlign = 1U << sym->GetSymbolAlign();
        }
        return ((symAlign + static_cast<uint64>(AArch64isa::GetMemOpndOffsetValue(o))) &
                static_cast<int64>(k15BitSize)) == static_cast<int64>(k0BitSize);
    }
    return StrLdrInsnSignedOfstValid(AArch64isa::GetMemOpndOffsetValue(o), k4ByteSize, memOpnd->IsIntactIndexed());
}

// Immediate verification: value range -1024 ~ 1008, multiple of 16.
inline bool StrLdr128PairValid(Operand *o)
{
    int64 value = AArch64isa::GetMemOpndOffsetValue(o);
    if (value < k1024BitSize && (value >= kNegative1024BitSize)) {
        return (static_cast<uint64>(value) & 0xf) > 0 ? false : true;
    }
    return false;
}

// Load-Acquire & Store-Release & Load/Store-Exclusive offset value must be #0 if not absent
inline bool IsOfstZero(Operand *o)
{
    int64 value = AArch64isa::GetMemOpndOffsetValue(o);
    if (value == static_cast<int64>(k0BitSize)) {
        return true;
    }
    return false;
}

inline bool MOP_wmovri32Valid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr)
                           ? IsSingleInstructionMovable32(static_cast<ImmOperand *>(opnds[kInsnSecondOpnd])->GetValue())
                           : true;
    return checkSecond;
}

inline bool MOP_xmovri64Valid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr)
                           ? IsSingleInstructionMovable64(static_cast<ImmOperand *>(opnds[kInsnSecondOpnd])->GetValue())
                           : true;
    return checkSecond;
}

inline bool MOP_xaddrrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? BitShift6BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xaddsrrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFifth = (opnds[kInsnFifthOpnd] != nullptr)
                          ? BitShift6BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFifthOpnd])->GetValue())
                          : true;
    return checkFifth;
}

inline bool MOP_xxwaddrrreValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? ExtendShift0To4Valid(static_cast<ExtendShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xaddrri24Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? LeftShift12Valid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkThird && checkFourth;
}

inline bool MOP_xaddrri12Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_xaddsrri12Valid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_waddrrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? BitShift5BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_waddsrrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFifth = (opnds[kInsnFifthOpnd] != nullptr)
                          ? BitShift5BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFifthOpnd])->GetValue())
                          : true;
    return checkFifth;
}

inline bool MOP_wwwaddrrreValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? ExtendShift0To4Valid(static_cast<ExtendShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_waddrri24Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? LeftShift12Valid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkThird && checkFourth;
}

inline bool MOP_waddrri12Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_waddsrri12Valid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xsubrrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? BitShift6BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xsubsrrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFifth = (opnds[kInsnFifthOpnd] != nullptr)
                          ? BitShift6BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFifthOpnd])->GetValue())
                          : true;
    return checkFifth;
}

inline bool MOP_xsubrri24Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? LeftShift12Valid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkThird && checkFourth;
}

inline bool MOP_xsubsrri24Valid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    bool checkFifth = (opnds[kInsnFifthOpnd] != nullptr)
                          ? LeftShift12Valid(static_cast<BitShiftOperand *>(opnds[kInsnFifthOpnd])->GetValue())
                          : true;
    return checkFourth && checkFifth;
}

inline bool MOP_xsubrri12Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_xsubsrri12Valid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_wsubrrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? BitShift5BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_wsubsrrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFifth = (opnds[kInsnFifthOpnd] != nullptr)
                          ? BitShift5BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFifthOpnd])->GetValue())
                          : true;
    return checkFifth;
}

inline bool MOP_wsubrri24Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? LeftShift12Valid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkThird && checkFourth;
}

inline bool MOP_wsubsrri24Valid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    bool checkFifth = (opnds[kInsnFifthOpnd] != nullptr)
                          ? LeftShift12Valid(static_cast<BitShiftOperand *>(opnds[kInsnFifthOpnd])->GetValue())
                          : true;
    return checkFourth && checkFifth;
}

inline bool MOP_wsubrri12Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_wsubsrri12Valid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xxwsubrrreValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? ExtendShift0To4Valid(static_cast<ExtendShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_wwwsubrrreValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? ExtendShift0To4Valid(static_cast<ExtendShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xandrrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? BitShift6BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xandrri13Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm13BitMaskValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_wandrrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? BitShift5BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_wandrri12Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitMaskValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_xiorrrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? BitShift6BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xiorrri13Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm13BitMaskValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_wiorrrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? BitShift5BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_wiorrri12Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitMaskValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_xeorrrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? BitShift6BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xeorrri13Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm13BitMaskValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_weorrrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? BitShift5BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_weorrri12Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitMaskValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_wubfxrri5i5Valid(const MapleVector<Operand *> &opnds)
{
    if ((opnds[kInsnThirdOpnd] != nullptr) && (opnds[kInsnFourthOpnd] != nullptr)) {
        int64 lsb = static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue();
        int64 width = static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue();
        return Lsb5BitValid(lsb) && Width5BitValid(width, lsb);
    } else {
        bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                              ? Lsb5BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                              : true;
        bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                               ? Width5BitOnlyValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                               : true;
        return checkThird && checkFourth;
    }
}

inline bool MOP_xubfxrri6i6Valid(const MapleVector<Operand *> &opnds)
{
    if ((opnds[kInsnThirdOpnd] != nullptr) && (opnds[kInsnFourthOpnd] != nullptr)) {
        int64 lsb = static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue();
        int64 width = static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue();
        return Lsb6BitValid(lsb) && Width6BitValid(width, lsb);
    } else {
        bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                              ? Lsb6BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                              : true;
        bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                               ? Width6BitOnlyValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                               : true;
        return checkThird && checkFourth;
    }
}

inline bool MOP_wsbfxrri5i5Valid(const MapleVector<Operand *> &opnds)
{
    if ((opnds[kInsnThirdOpnd] != nullptr) && (opnds[kInsnFourthOpnd] != nullptr)) {
        int64 lsb = static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue();
        int64 width = static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue();
        return Lsb5BitValid(lsb) && Width5BitValid(width, lsb);
    } else {
        bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                              ? Lsb5BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                              : true;
        bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                               ? Width5BitOnlyValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                               : true;
        return checkThird && checkFourth;
    }
}

inline bool MOP_xsbfxrri6i6Valid(const MapleVector<Operand *> &opnds)
{
    if ((opnds[kInsnThirdOpnd] != nullptr) && (opnds[kInsnFourthOpnd] != nullptr)) {
        int64 lsb = static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue();
        int64 width = static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue();
        return Lsb6BitValid(lsb) && Width6BitValid(width, lsb);
    } else {
        bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                              ? Lsb6BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                              : true;
        bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                               ? Width6BitOnlyValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                               : true;
        return checkThird && checkFourth;
    }
}

inline bool MOP_wubfizrri5i5Valid(const MapleVector<Operand *> &opnds)
{
    if ((opnds[kInsnThirdOpnd] != nullptr) && (opnds[kInsnFourthOpnd] != nullptr)) {
        int64 lsb = static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue();
        int64 width = static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue();
        return Lsb5BitValid(lsb) && Width5BitValid(width, lsb);
    } else {
        bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                              ? Lsb5BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                              : true;
        bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                               ? Width5BitOnlyValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                               : true;
        return checkThird && checkFourth;
    }
}

inline bool MOP_xubfizrri6i6Valid(const MapleVector<Operand *> &opnds)
{
    if ((opnds[kInsnThirdOpnd] != nullptr) && (opnds[kInsnFourthOpnd] != nullptr)) {
        int64 lsb = static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue();
        int64 width = static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue();
        return Lsb6BitValid(lsb) && Width6BitValid(width, lsb);
    } else {
        bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                              ? Lsb6BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                              : true;
        bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                               ? Width6BitOnlyValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                               : true;
        return checkThird && checkFourth;
    }
}

inline bool MOP_wsbfizrri5i5Valid(const MapleVector<Operand *> &opnds)
{
    if ((opnds[kInsnThirdOpnd] != nullptr) && (opnds[kInsnFourthOpnd] != nullptr)) {
        int64 lsb = static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue();
        int64 width = static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue();
        return Lsb5BitValid(lsb) && Width5BitValid(width, lsb);
    } else {
        bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                              ? Lsb5BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                              : true;
        bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                               ? Width5BitOnlyValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                               : true;
        return checkThird && checkFourth;
    }
}

inline bool MOP_xsbfizrri6i6Valid(const MapleVector<Operand *> &opnds)
{
    if ((opnds[kInsnThirdOpnd] != nullptr) && (opnds[kInsnFourthOpnd] != nullptr)) {
        int64 lsb = static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue();
        int64 width = static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue();
        return Lsb6BitValid(lsb) && Width6BitValid(width, lsb);
    } else {
        bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                              ? Lsb6BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                              : true;
        bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                               ? Width6BitOnlyValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                               : true;
        return checkThird && checkFourth;
    }
}

inline bool MOP_wbfirri5i5Valid(const MapleVector<Operand *> &opnds)
{
    if ((opnds[kInsnThirdOpnd] != nullptr) && (opnds[kInsnFourthOpnd] != nullptr)) {
        int64 lsb = static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue();
        int64 width = static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue();
        return Lsb5BitValid(lsb) && Width5BitValid(width, lsb);
    } else {
        bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                              ? Lsb5BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                              : true;
        bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                               ? Width5BitOnlyValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                               : true;
        return checkThird && checkFourth;
    }
}

inline bool MOP_xbfirri6i6Valid(const MapleVector<Operand *> &opnds)
{
    if ((opnds[kInsnThirdOpnd] != nullptr) && (opnds[kInsnFourthOpnd] != nullptr)) {
        int64 lsb = static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue();
        int64 width = static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue();
        return Lsb6BitValid(lsb) && Width6BitValid(width, lsb);
    } else {
        bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                              ? Lsb6BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                              : true;
        bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                               ? Width6BitOnlyValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                               : true;
        return checkThird && checkFourth;
    }
}

inline bool MOP_xlslrri6Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? BitShift6BitValidImm(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_wlslrri5Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? BitShift5BitValidImm(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_xasrrri6Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? BitShift6BitValidImm(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_wasrrri5Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? BitShift5BitValidImm(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_xlsrrri6Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? BitShift6BitValidImm(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_wlsrrri5Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? BitShift5BitValidImm(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_wtstri32Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitMaskValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_xtstri64Valid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm13BitMaskValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_wextrrrri5Valid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? Lsb5BitValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xextrrrri6Valid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? Lsb6BitValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_winegrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? BitShift5BitValid(static_cast<BitShiftOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_xinegrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? BitShift6BitValid(static_cast<BitShiftOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_wldrsbValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr8Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_xldrsbValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr8Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wldrbValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr8Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wldrshValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr16Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_xldrshValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr16Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_xldrswValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr32Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wldrhValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr16Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wldrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr32Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_xldrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr64Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_bldrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr8Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_hldrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr16Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_sldrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr32Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_dldrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr64Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_qldrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr128Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wldpValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? StrLdr32PairValid(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_xldpValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? StrLdr64PairValid(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_xldpswValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? StrLdr32PairValid(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_sldpValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? StrLdr32PairValid(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_dldpValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? StrLdr64PairValid(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_qldpValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? StrLdr128PairValid(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_wldarbValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wldarhValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wldarValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_xldarValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wmovkri16Valid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr)
                           ? Imm16BitValid(static_cast<ImmOperand *>(opnds[kInsnSecondOpnd])->GetValue())
                           : true;
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? ImmShift32Valid(static_cast<BitShiftOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkSecond && checkThird;
}

inline bool MOP_xmovkri16Valid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr)
                           ? Imm16BitValid(static_cast<ImmOperand *>(opnds[kInsnSecondOpnd])->GetValue())
                           : true;
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? ImmShift64Valid(static_cast<BitShiftOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkSecond && checkThird;
}

inline bool MOP_wmovzri16Valid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr)
                           ? Imm16BitValid(static_cast<ImmOperand *>(opnds[kInsnSecondOpnd])->GetValue())
                           : true;
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? ImmShift32Valid(static_cast<BitShiftOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkSecond && checkThird;
}

inline bool MOP_xmovzri16Valid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr)
                           ? Imm16BitValid(static_cast<ImmOperand *>(opnds[kInsnSecondOpnd])->GetValue())
                           : true;
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? ImmShift64Valid(static_cast<BitShiftOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkSecond && checkThird;
}

inline bool MOP_wmovnri16Valid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr)
                           ? Imm16BitValid(static_cast<ImmOperand *>(opnds[kInsnSecondOpnd])->GetValue())
                           : true;
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? ImmShift32Valid(static_cast<BitShiftOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkSecond && checkThird;
}

inline bool MOP_xmovnri16Valid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr)
                           ? Imm16BitValid(static_cast<ImmOperand *>(opnds[kInsnSecondOpnd])->GetValue())
                           : true;
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? ImmShift64Valid(static_cast<BitShiftOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkSecond && checkThird;
}

inline bool MOP_wldxrbValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wldxrhValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wldxrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_xldxrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wldaxrbValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wldaxrhValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wldaxrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_xldaxrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wldaxpValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? IsOfstZero(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_xldaxpValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? IsOfstZero(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_wcmpriValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_wcmprrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? BitShift5BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_wwcmprreValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? ExtendShift0To4Valid(static_cast<ExtendShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xcmpriValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_xcmprrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? BitShift6BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xwcmprreValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? ExtendShift0To4Valid(static_cast<ExtendShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_wccmpriicValid(const MapleVector<Operand *> &opnds)
{
    // Is a five bit unsigned (positive) immediate, range 0 to 31
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? BitShift5BitValidImm(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? Nzcv4BitValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkThird && checkFourth;
}

inline bool MOP_wccmprricValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? Nzcv4BitValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xccmpriicValid(const MapleVector<Operand *> &opnds)
{
    // Is a five bit unsigned (positive) immediate, range 0 to 31
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? BitShift5BitValidImm(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? Nzcv4BitValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkThird && checkFourth;
}

inline bool MOP_xccmprricValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? Nzcv4BitValid(static_cast<ImmOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_wcmnriValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_wcmnrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? BitShift5BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_wwcmnrreValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? ExtendShift0To4Valid(static_cast<ExtendShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xcmnriValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr)
                          ? Imm12BitValid(static_cast<ImmOperand *>(opnds[kInsnThirdOpnd])->GetValue())
                          : true;
    return checkThird;
}

inline bool MOP_xcmnrrsValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? BitShift6BitValid(static_cast<BitShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_xwcmnrreValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr)
                           ? ExtendShift0To4Valid(static_cast<ExtendShiftOperand *>(opnds[kInsnFourthOpnd])->GetValue())
                           : true;
    return checkFourth;
}

inline bool MOP_wtbnzValid(const MapleVector<Operand *> &opnds)
{
    // Is the bit number to be tested, in the range 0 to 63
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr)
                           ? BitShift5BitValidImm(static_cast<ImmOperand *>(opnds[kInsnSecondOpnd])->GetValue())
                           : true;
    return checkSecond;
}

inline bool MOP_xtbnzValid(const MapleVector<Operand *> &opnds)
{
    // Is the bit number to be tested, in the range 0 to 63
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr)
                           ? BitShift6BitValidImm(static_cast<ImmOperand *>(opnds[kInsnSecondOpnd])->GetValue())
                           : true;
    return checkSecond;
}

inline bool MOP_wtbzValid(const MapleVector<Operand *> &opnds)
{
    // Is the bit number to be tested, in the range 0 to 31
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr)
                           ? BitShift5BitValidImm(static_cast<ImmOperand *>(opnds[kInsnSecondOpnd])->GetValue())
                           : true;
    return checkSecond;
}

inline bool MOP_xtbzValid(const MapleVector<Operand *> &opnds)
{
    // Is the bit number to be tested, in the range 0 to 63
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr)
                           ? BitShift6BitValidImm(static_cast<ImmOperand *>(opnds[kInsnSecondOpnd])->GetValue())
                           : true;
    return checkSecond;
}

inline bool MOP_wstrbValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr8Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wstrhValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr16Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wstrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr32Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_xstrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr64Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_sstrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr32Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_dstrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr64Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_qstrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? StrLdr128Valid(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wstpValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? StrLdr32PairValid(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_xstpValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? StrLdr64PairValid(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_sstpValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? StrLdr32PairValid(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_dstpValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? StrLdr64PairValid(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_qstpValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? StrLdr128PairValid(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_wstlrbValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wstlrhValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wstlrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_xstlrValid(const MapleVector<Operand *> &opnds)
{
    bool checkSecond = (opnds[kInsnSecondOpnd] != nullptr) ? IsOfstZero(opnds[kInsnSecondOpnd]) : true;
    return checkSecond;
}

inline bool MOP_wstxrbValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? IsOfstZero(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_wstxrhValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? IsOfstZero(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_wstxrValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? IsOfstZero(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_xstxrValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? IsOfstZero(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_wstlxrbValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? IsOfstZero(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_wstlxrhValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? IsOfstZero(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_wstlxrValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? IsOfstZero(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_xstlxrValid(const MapleVector<Operand *> &opnds)
{
    bool checkThird = (opnds[kInsnThirdOpnd] != nullptr) ? IsOfstZero(opnds[kInsnThirdOpnd]) : true;
    return checkThird;
}

inline bool MOP_wstlxpValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr) ? IsOfstZero(opnds[kInsnFourthOpnd]) : true;
    return checkFourth;
}

inline bool MOP_xstlxpValid(const MapleVector<Operand *> &opnds)
{
    bool checkFourth = (opnds[kInsnFourthOpnd] != nullptr) ? IsOfstZero(opnds[kInsnFourthOpnd]) : true;
    return checkFourth;
}

inline bool MOP_brkValid(const MapleVector<Operand *> &opnds)
{
    bool checkFirst = (opnds[kInsnFirstOpnd] != nullptr)
                          ? Imm16BitValidImm(static_cast<ImmOperand *>(opnds[kInsnFirstOpnd])->GetValue())
                          : true;
    return checkFirst;
}

inline bool MOP_assert_nonnullValid(const MapleVector<Operand *> &opnds)
{
    return MOP_wldrValid(opnds);
}
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MOP_VALID_H */
