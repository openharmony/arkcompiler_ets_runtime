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
#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MOP_SPLIT_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MOP_SPLIT_H

#include "aarch64_cg.h"
#include "aarch64_isa.h"
#include "cg_irbuilder.h"
#include "cgbb.h"
#include "common_utils.h"

namespace maplebe {
// Supply a new reg operand for insn split process, which type is kRegTyInt for immediate.
// Before regalloc: create a new virtual reg;
// After regalloc: use R16 to be a temporary physical reg.
inline RegOperand *GetSplitBaseReg(bool isAfterRegAlloc, bool is64Bits, OperandBuilder *opndBuilder)
{
    RegOperand *resOpnd = nullptr;
    if (!isAfterRegAlloc) {
        resOpnd = &opndBuilder->CreateVReg((is64Bits ? k64BitSize : k32BitSize), kRegTyInt);
    } else {
        resOpnd = &opndBuilder->CreatePReg(R16, (is64Bits ? k64BitSize : k32BitSize), kRegTyInt);
    }
    return resOpnd;
}

// Judging valid range of the immediate by passing in bitLen & forPair parameter, return the closest valid value to
// ofstVal, getting the remainder simultaneously. The valid value will be input in new memopnd, and the remainder
// will be input in add insn.
inline ImmOperand &SplitGetRemained(const MemOperand &memOpnd, uint32 bitLen, int64 ofstVal, bool forPair,
                                    OperandBuilder *opndBuilder)
{
    // opndVal == Q0 * 32760(16380) + R0
    // R0 == Q1 * 8(4) + R1
    // ADDEND == Q0 * 32760(16380) + R1
    // NEW_OFFSET = Q1 * 8(4)
    // we want to generate two instructions:
    // ADD TEMP_REG, X29, ADDEND
    // LDR/STR TEMP_REG, [ TEMP_REG, #NEW_OFFSET ]
    int32 maxPimm = 0;
    if (!forPair) {
        maxPimm = MemOperand::GetMaxPIMM(bitLen);
    } else {
        maxPimm = MemOperand::GetMaxPairPIMM(bitLen);
    }
    DEBUG_ASSERT(maxPimm != 0, "get max pimm failed");
    int64 q0 = ofstVal / maxPimm + (ofstVal < 0 ? -1 : 0);
    int64 addend = q0 * maxPimm;
    uint64 r0 = static_cast<uint64>(ofstVal - addend);
    uint64 alignment = static_cast<uint64>(static_cast<int64>(MemOperand::GetImmediateOffsetAlignment(bitLen)));
    auto q1 = r0 >> alignment;
    auto r1 = static_cast<int64>(r0 & ((1u << alignment) - 1));
    auto remained = static_cast<int64>(q1 << alignment);
    addend = addend + r1;
    if (addend > 0) {
        uint64 suffixClear = 0xfff;
        if (forPair) {
            suffixClear = 0xff;
        }
        int64 remainedTmp = remained + static_cast<int64>(static_cast<uint64>(addend) & suffixClear);
        if (!MemOperand::IsPIMMOffsetOutOfRange(static_cast<int32>(remainedTmp), bitLen) &&
            ((static_cast<uint64>(remainedTmp) & ((1u << alignment) - 1)) == 0)) {
            addend = static_cast<int64>(static_cast<uint64>(addend) & ~suffixClear);
        }
    }
    ImmOperand &immAddend = opndBuilder->CreateImm(k64BitSize, addend, true);
    if (memOpnd.GetOffsetImmediate()->GetVary() == kUnAdjustVary) {
        immAddend.SetVary(kUnAdjustVary);
    }
    return immAddend;
}

// Split Add Insn add reg, reg, #imm, steps as follows:
// If #imm value range 0 ~ 2^24 - 1, insn will be split into add reg, reg, #imm(, LSL 12)
// If #imm value out of range 2^24, insn will be split as follows:
// add  x0, x1, #imm    ====>    mov  x2, #imm
//                               add  x0, x1, x2
inline void AddInsnSplit(Insn *insn, bool is64Bits, bool isAfterRegAlloc, InsnBuilder *insnBuilder,
                         OperandBuilder *opndBuilder)
{
    if (insn->VerifySelf()) {
        return;
    }
    Operand *opnd0 = &insn->GetOperand(kInsnFirstOpnd);
    Operand *opnd1 = &insn->GetOperand(kInsnSecondOpnd);
    Operand *opnd2 = &insn->GetOperand(kInsnThirdOpnd);
    ImmOperand *oldImmOpnd = static_cast<ImmOperand *>(opnd2);
    ImmOperand *immOpnd =
        &opndBuilder->CreateImm(oldImmOpnd->GetSize(), oldImmOpnd->GetValue(), oldImmOpnd->IsSignedValue());
    immOpnd->SetVary(oldImmOpnd->GetVary());
    insn->SetOperand(kInsnThirdOpnd, *immOpnd);
    MOperator mOpCode = MOP_undef;
    if (immOpnd->IsNegative()) {
        immOpnd->Negate();
        mOpCode = is64Bits ? MOP_xsubrri12 : MOP_wsubrri12;
        insn->SetMOP(AArch64CG::kMd[mOpCode]);
        if (!insn->VerifySelf()) {
            insn->SplitSelf(isAfterRegAlloc, insnBuilder, opndBuilder);
        }
        return;
    }
    BB *bb = insn->GetBB();
    // lower 24 bits has 1, higher bits are all 0
    if (immOpnd->IsInBitSize(kMaxImmVal24Bits, 0)) {
        // lower 12 bits and higher 12 bits both has 1
        Operand *newOpnd1 = opnd1;
        if (!(immOpnd->IsInBitSize(kMaxImmVal12Bits, 0) || immOpnd->IsInBitSize(kMaxImmVal12Bits, kMaxImmVal12Bits))) {
            // process higher 12 bits
            ImmOperand &immOpnd2 = opndBuilder->CreateImm(
                immOpnd->GetSize(), static_cast<int64>(static_cast<uint64>(immOpnd->GetValue()) >> kMaxImmVal12Bits),
                immOpnd->IsSignedValue());
            mOpCode = is64Bits ? MOP_xaddrri24 : MOP_waddrri24;
            RegOperand *tmpRes = GetSplitBaseReg(isAfterRegAlloc, is64Bits, opndBuilder);
            BitShiftOperand &shiftopnd = opndBuilder->CreateBitShift(BitShiftOperand::kLSL, k12BitSize, k64BitSize);
            Insn &newInsn = insnBuilder->BuildInsn(mOpCode, *tmpRes, *opnd1, immOpnd2, shiftopnd);
            newInsn.SetBB(bb);
            DEBUG_ASSERT(newInsn.VerifySelf(), "immOpnd2 appears invalid");
            (void)bb->InsertInsnBefore(*insn, newInsn);
            // get lower 12 bits value
            immOpnd->ModuloByPow2(kMaxImmVal12Bits);
            newOpnd1 = tmpRes;
        }
        // process lower 12 bits value
        mOpCode = is64Bits ? MOP_xaddrri12 : MOP_waddrri12;
        // It`s worth noting that if immOpnd->IsInBitSize(12, 12) returns true, gcc assembler can compile correctly,
        // so we pass immOpnd directly as a parameter.
        Insn &newInsn = insnBuilder->BuildInsn(mOpCode, *opnd0, *newOpnd1, *immOpnd);
        newInsn.SetBB(bb);
        DEBUG_ASSERT(newInsn.VerifySelf(), "immOpnd appears invalid");
        bb->ReplaceInsn(*insn, newInsn);
        return;
    } else {
        // load into register
        int64 immVal = immOpnd->GetValue();
        int32 tail0bitNum = AArch64isa::GetTail0BitNum(immVal);
        int32 head0bitNum = AArch64isa::GetHead0BitNum(immVal);
        const int32 bitNum = (k64BitSizeInt - head0bitNum) - tail0bitNum;
        RegOperand *movOpnd = GetSplitBaseReg(isAfterRegAlloc, is64Bits, opndBuilder);
        regno_t regNO0 = static_cast<RegOperand *>(opnd1)->GetRegisterNumber();
        // The content of the next if code block is when immvalue can be moved in one insn, we do next:
        // add  x0, x1, #imm1    ====>    mov  x2, #imm2
        //                                add  x0, x1, x2, LSL <shift>
        // #imm2 = #imm1 >> shift
        // addrrrs do not support sp
        if (bitNum <= k16BitSizeInt && regNO0 != RSP) {
            int64 newImm =
                static_cast<int64>((static_cast<uint64>(immVal) >> static_cast<uint32>(tail0bitNum)) & 0xFFFF);
            ImmOperand &immOpnd1 = opndBuilder->CreateImm(k16BitSize, newImm, false);
            mOpCode = is64Bits ? MOP_xmovri64 : MOP_wmovri32;
            Insn &movInsn = insnBuilder->BuildInsn(mOpCode, *movOpnd, immOpnd1);
            movInsn.SetBB(bb);
            (void)bb->InsertInsnBefore(*insn, movInsn);
            mOpCode = is64Bits ? MOP_xaddrrrs : MOP_waddrrrs;
            // bitLen means bitshiftopnd size: 64bits -> 6, 32bits ->5
            uint32 bitLen = is64Bits ? k6BitSize : k5BitSize;
            BitShiftOperand &bitShiftOpnd =
                opndBuilder->CreateBitShift(BitShiftOperand::kLSL, static_cast<uint32>(tail0bitNum), bitLen);
            Insn &newInsn = insnBuilder->BuildInsn(mOpCode, *opnd0, *opnd1, *movOpnd, bitShiftOpnd);
            newInsn.SetBB(bb);
            bb->ReplaceInsn(*insn, newInsn);
            return;
        }
        mOpCode = is64Bits ? MOP_xmovri64 : MOP_wmovri32;
        Insn &movInsn = insnBuilder->BuildInsn(mOpCode, *movOpnd, *immOpnd);
        movInsn.SetBB(bb);
        mOpCode = is64Bits ? MOP_xaddrrr : MOP_waddrrr;
        Insn &newInsn = insnBuilder->BuildInsn(mOpCode, *opnd0, *opnd1, *movOpnd);
        newInsn.SetBB(bb);
        (void)bb->InsertInsnBefore(*insn, movInsn);
        // If #imm of mov is invalid, split mov insn
        if (!movInsn.VerifySelf()) {
            movInsn.SplitSelf(isAfterRegAlloc, insnBuilder, opndBuilder);
        }
        bb->ReplaceInsn(*insn, newInsn);
    }
}

// Split Sub Insn sub reg, reg, #imm, the same split steps as add
inline void SubInsnSplit(Insn *insn, bool is64Bits, bool isAfterRegAlloc, InsnBuilder *insnBuilder,
                         OperandBuilder *opndBuilder)
{
    if (insn->VerifySelf()) {
        return;
    }
    Operand *opnd0 = &insn->GetOperand(kInsnFirstOpnd);
    Operand *opnd1 = &insn->GetOperand(kInsnSecondOpnd);
    Operand *opnd2 = &insn->GetOperand(kInsnThirdOpnd);
    ImmOperand *oldImmOpnd = static_cast<ImmOperand *>(opnd2);
    ImmOperand *immOpnd =
        &opndBuilder->CreateImm(oldImmOpnd->GetSize(), oldImmOpnd->GetValue(), oldImmOpnd->IsSignedValue());
    immOpnd->SetVary(oldImmOpnd->GetVary());
    insn->SetOperand(kInsnThirdOpnd, *immOpnd);
    MOperator mOpCode = MOP_undef;
    if (immOpnd->IsNegative()) {
        immOpnd->Negate();
        mOpCode = is64Bits ? MOP_xaddrri12 : MOP_waddrri12;
        insn->SetMOP(AArch64CG::kMd[mOpCode]);
        if (!insn->VerifySelf()) {
            insn->SplitSelf(isAfterRegAlloc, insnBuilder, opndBuilder);
        }
        return;
    }
    BB *bb = insn->GetBB();
    int64 higher12BitVal = static_cast<int64>(static_cast<uint64>(immOpnd->GetValue()) >> kMaxImmVal12Bits);
    if (immOpnd->IsInBitSize(kMaxImmVal24Bits, 0) && higher12BitVal + 1 <= kMaxPimm8) {
        // SUB Wd|WSP, Wn|WSP, #imm{, shift} ; 32-bit general registers
        // SUB Xd|SP,  Xn|SP,  #imm{, shift} ; 64-bit general registers
        // imm : 0 ~ 4095, shift: none, LSL #0, or LSL #12
        // aarch64 assembly takes up to 24-bits, if the lower 12 bits is all 0
        // large offset is treated as sub (higher 12 bits + 4096) + add
        // it gives opportunities for combining add + ldr due to the characteristics of aarch64's load/store
        bool isSplitSub = false;
        Operand *newOpnd1 = opnd1;
        if (!(immOpnd->IsInBitSize(kMaxImmVal12Bits, 0) || immOpnd->IsInBitSize(kMaxImmVal12Bits, kMaxImmVal12Bits))) {
            isSplitSub = true;
            // process higher 12 bits
            ImmOperand &immOpnd2 =
                opndBuilder->CreateImm(immOpnd->GetSize(), higher12BitVal + 1, immOpnd->IsSignedValue());
            mOpCode = is64Bits ? MOP_xsubrri24 : MOP_wsubrri24;
            RegOperand *resOpnd = GetSplitBaseReg(isAfterRegAlloc, is64Bits, opndBuilder);
            BitShiftOperand &shiftopnd = opndBuilder->CreateBitShift(BitShiftOperand::kLSL, k12BitSize, k64BitSize);
            Insn &newInsn = insnBuilder->BuildInsn(mOpCode, *resOpnd, *opnd1, immOpnd2, shiftopnd);
            newInsn.SetBB(bb);
            (void)bb->InsertInsnBefore(*insn, newInsn);
            immOpnd->ModuloByPow2(kMaxImmVal12Bits);
            immOpnd->SetValue(static_cast<int64>(kMax12UnsignedImm) - immOpnd->GetValue());
            newOpnd1 = resOpnd;
        }
        // process lower 12 bits
        mOpCode = isSplitSub ? (is64Bits ? MOP_xaddrri12 : MOP_waddrri12) : (is64Bits ? MOP_xsubrri12 : MOP_wsubrri12);
        Insn &newInsn = insnBuilder->BuildInsn(mOpCode, *opnd0, *newOpnd1, *immOpnd);
        newInsn.SetBB(bb);
        bb->ReplaceInsn(*insn, newInsn);
        return;
    } else {
        // load into register
        int64 immVal = immOpnd->GetValue();
        int32 tail0bitNum = AArch64isa::GetTail0BitNum(immVal);
        int32 head0bitNum = AArch64isa::GetHead0BitNum(immVal);
        const int32 bitNum = (k64BitSizeInt - head0bitNum) - tail0bitNum;
        RegOperand *movOpnd = GetSplitBaseReg(isAfterRegAlloc, is64Bits, opndBuilder);
        // The content of the next if code block is when immvalue can be moved in one insn, we do next:
        // sub  x0, x1, #imm1    ====>    mov  x2, #imm2
        //                                sub  x0, x1, x2, LSL <shift>
        // #imm2 = #imm1 >> shift
        // subrrrs supports sp, so do not need to check whether regNo is RSP
        if (bitNum <= k16BitSizeInt) {
            int64 newImm =
                static_cast<int64>((static_cast<uint64>(immVal) >> static_cast<uint32>(tail0bitNum)) & 0xFFFF);
            ImmOperand &immOpnd1 = opndBuilder->CreateImm(k16BitSize, newImm, false);
            mOpCode = is64Bits ? MOP_xmovri64 : MOP_wmovri32;
            Insn &movInsn = insnBuilder->BuildInsn(mOpCode, *movOpnd, immOpnd1);
            movInsn.SetBB(bb);
            (void)bb->InsertInsnBefore(*insn, movInsn);
            mOpCode = is64Bits ? MOP_xsubrrrs : MOP_wsubrrrs;
            // bitLen means bitshiftopnd size: 64bits -> 6, 32bits ->5
            uint32 bitLen = is64Bits ? k6BitSize : k5BitSize;
            BitShiftOperand &bitShiftOpnd =
                opndBuilder->CreateBitShift(BitShiftOperand::kLSL, static_cast<uint32>(tail0bitNum), bitLen);
            Insn &newInsn = insnBuilder->BuildInsn(mOpCode, *opnd0, *opnd1, *movOpnd, bitShiftOpnd);
            newInsn.SetBB(bb);
            bb->ReplaceInsn(*insn, newInsn);
            return;
        }
        mOpCode = is64Bits ? MOP_xmovri64 : MOP_wmovri32;
        Insn &movInsn = insnBuilder->BuildInsn(mOpCode, *movOpnd, *immOpnd);
        movInsn.SetBB(bb);
        mOpCode = is64Bits ? MOP_xsubrrr : MOP_wsubrrr;
        Insn &newInsn = insnBuilder->BuildInsn(mOpCode, *opnd0, *opnd1, *movOpnd);
        newInsn.SetBB(bb);
        (void)bb->InsertInsnBefore(*insn, movInsn);
        // If #imm of mov is invalid, split mov insn
        if (!movInsn.VerifySelf()) {
            movInsn.SplitSelf(isAfterRegAlloc, insnBuilder, opndBuilder);
        }
        bb->ReplaceInsn(*insn, newInsn);
    }
}

// adds & subs updates the condition flags based on the result, so it cannot be split into multiple instructions
// of the same type, because we don`t know when the result is out of range and changes flags. The solution is:
// adds/subs  x0, x1, #imm    ====>    mov        x2, #imm
//                                     adds/subs  x0, x1, x2
// isAdds: true -> adds, false -> subs
inline void AddsSubsInsnSplit(Insn *insn, bool isAdds, bool is64Bits, bool isAfterRegAlloc, InsnBuilder *insnBuilder,
                              OperandBuilder *opndBuilder)
{
    if (insn->VerifySelf()) {
        return;
    }
    Operand *opnd0 = &insn->GetOperand(kInsnFirstOpnd);
    Operand *opnd1 = &insn->GetOperand(kInsnSecondOpnd);
    Operand *opnd2 = &insn->GetOperand(kInsnThirdOpnd);
    Operand *opnd3 = &insn->GetOperand(kInsnFourthOpnd);
    ImmOperand *immOpnd = static_cast<ImmOperand *>(opnd3);
    MOperator mOpCode = MOP_undef;
    BB *bb = insn->GetBB();
    RegOperand *movOpnd = GetSplitBaseReg(isAfterRegAlloc, is64Bits, opndBuilder);
    mOpCode = is64Bits ? MOP_xmovri64 : MOP_wmovri32;
    Insn &movInsn = insnBuilder->BuildInsn(mOpCode, *movOpnd, *immOpnd);
    movInsn.SetBB(bb);
    mOpCode = isAdds ? (is64Bits ? MOP_xaddsrrr : MOP_waddsrrr) : (is64Bits ? MOP_xsubsrrr : MOP_wsubsrrr);
    Insn &newInsn = insnBuilder->BuildInsn(mOpCode, *opnd0, *opnd1, *opnd2, *movOpnd);
    newInsn.SetBB(bb);
    (void)bb->InsertInsnBefore(*insn, movInsn);
    // If #imm of mov is invalid, split mov first.
    if (!movInsn.VerifySelf()) {
        movInsn.SplitSelf(isAfterRegAlloc, insnBuilder, opndBuilder);
    }
    (void)bb->InsertInsnBefore(*insn, newInsn);
    bb->RemoveInsn(*insn);
}

// Split Add/Sub Insn with BitShiftOperand (LSL 12), steps as follows:
// add/sub  x0, x1, #imm, LSL 12   ====>   add/sub  x0, x1, #newimm   ====>   Add/SubInsnSplit
// isAdd: true -> add, false -> sub
inline void AddSubWithLslSplit(Insn *insn, bool isAdd, bool is64Bits, bool isAfterRegAlloc, InsnBuilder *insnBuilder,
                               OperandBuilder *opndBuilder)
{
    if (insn->VerifySelf()) {
        return;
    }
    uint32 size = is64Bits ? k64BitSize : k32BitSize;
    ImmOperand &immOpnd = static_cast<ImmOperand &>(insn->GetOperand(kInsnThirdOpnd));
    // add/sub x0, x1, #imm, LSL 12  ====>  imm value range can be split: 2^12 ~ 2^52 - 1
    // add/sub w0, w1, #imm, LSL 12  ====>  imm value range can be split: 2^12 ~ 2^20 - 1
    // If imm is out of range, insn will not be split
    if (!immOpnd.IsInBitSize((size - k12BitSize), k0BitSize)) {
        return;
    }
    BB *bb = insn->GetBB();
    ImmOperand &newImmOpnd =
        opndBuilder->CreateImm(size, static_cast<int64>(static_cast<uint64>(immOpnd.GetValue()) << k12BitSize));
    MOperator mOpCode = is64Bits ? (isAdd ? MOP_xaddrri12 : MOP_xsubrri12) : (isAdd ? MOP_waddrri12 : MOP_wsubrri12);
    Insn &newInsn = insnBuilder->BuildInsn(mOpCode, insn->GetOperand(kInsnFirstOpnd), insn->GetOperand(kInsnSecondOpnd),
                                           newImmOpnd);
    newInsn.SetBB(bb);
    bb->ReplaceInsn(*insn, newInsn);
    if (!newInsn.VerifySelf()) {
        newInsn.SplitSelf(isAfterRegAlloc, insnBuilder, opndBuilder);
    }
}

// Split memoperand with invalid offset value to a new valid memoperand and add insn with remainder.
inline MemOperand &MemOfstSplitWithAdd(const MemOperand &memOpnd, uint32 bitLen, bool isAfterRegAlloc, Insn *insn,
                                       bool forPair, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    DEBUG_ASSERT((memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi), "expect kAddrModeBOi memOpnd");
    DEBUG_ASSERT(memOpnd.IsIntactIndexed(), "expect intactIndexed memOpnd");
    BB *bb = insn->GetBB();
    OfstOperand *ofstOpnd = memOpnd.GetOffsetImmediate();
    int64 ofstVal = ofstOpnd->GetOffsetValue();
    RegOperand *resOpnd = GetSplitBaseReg(isAfterRegAlloc, true, opndBuilder);
    ImmOperand &immAddend = SplitGetRemained(memOpnd, bitLen, ofstVal, forPair, opndBuilder);
    int64 remained = (ofstVal - immAddend.GetValue());
    RegOperand *origBaseReg = memOpnd.GetBaseRegister();
    DEBUG_ASSERT(origBaseReg != nullptr, "nullptr check");
    // Provide add insn to split offset, where add insn is 64-bit explicitly
    Insn &addInsn = insnBuilder->BuildInsn(MOP_xaddrri12, *resOpnd, *origBaseReg, immAddend);
    addInsn.SetBB(bb);
    bb->InsertInsnBefore(*insn, addInsn);
    if (!addInsn.VerifySelf()) {
        addInsn.SplitSelf(isAfterRegAlloc, insnBuilder, opndBuilder);
    }
    ImmOperand &remainedOpnd = opndBuilder->CreateImm(k32BitSize, remained, true);
    MemOperand &newMemOpnd = opndBuilder->CreateMem(bitLen, *resOpnd, remainedOpnd);
    newMemOpnd.SetStackMem(memOpnd.IsStackMem());
    return newMemOpnd;
}

// Split a load/store insn with invalid offset value to a new valid insn and add insn.
// idx: memOperand index of insn
inline void LoadStoreInsnSplit(Insn *insn, uint32 idx, bool forPair, bool isAfterRegAlloc, InsnBuilder *insnBuilder,
                               OperandBuilder *opndBuilder)
{
    if (insn->VerifySelf()) {
        return;
    }
    MemOperand &memOpnd = static_cast<MemOperand &>(insn->GetOperand(idx));
    if (!(memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi && memOpnd.IsIntactIndexed())) {
        return;
    }
    uint32 bitLen = insn->GetDesc()->GetOpndDes(idx)->GetSize();
    MemOperand &newMemOpnd =
        MemOfstSplitWithAdd(memOpnd, bitLen, isAfterRegAlloc, insn, forPair, insnBuilder, opndBuilder);
    insn->SetOperand(idx, newMemOpnd);
}

// ccmp updates the condition flags based on the result, so it cannot be split into multiple instructions
// of the same type, because we don`t know when the result is out of range and changes flags. The solution is:
// ccmp  <Xn>, #<imm>, #<nzcv>, <cond>    ====>    mov   <Xm>, #<imm>
//                                                 ccmp  <Xn>, <Xm>, #<nzcv>, <cond>
inline void CondCompareInsnSplit(Insn *insn, bool is64Bits, bool isAfterRegAlloc, InsnBuilder *insnBuilder,
                                 OperandBuilder *opndBuilder)
{
    if (insn->VerifySelf()) {
        return;
    }
    ImmOperand &immOpnd = static_cast<ImmOperand &>(insn->GetOperand(kInsnSecondOpnd));
    MOperator mOpCode = MOP_undef;
    BB *bb = insn->GetBB();
    RegOperand *movOpnd = GetSplitBaseReg(isAfterRegAlloc, is64Bits, opndBuilder);
    mOpCode = is64Bits ? MOP_xmovri64 : MOP_wmovri32;
    Insn &movInsn = insnBuilder->BuildInsn(mOpCode, *movOpnd, immOpnd);
    movInsn.SetBB(bb);
    bb->InsertInsnBefore(*insn, movInsn);
    if (!movInsn.VerifySelf()) {
        movInsn.SplitSelf(isAfterRegAlloc, insnBuilder, opndBuilder);
    }
    mOpCode = is64Bits ? MOP_xccmprric : MOP_wccmprric;
    insn->SetMOP(AArch64CG::kMd[mOpCode]);
    insn->SetOperand(kInsnSecondOpnd, *movOpnd);
}

// split mov w0, #imm to mov and movk
inline void MOP_wmovri32Split(Insn *curInsn, bool /* isAfterRegAlloc */, InsnBuilder *insnBuilder,
                              OperandBuilder *opndBuilder)
{
    // If higher 32bits of immVal have 1, we will truncate and keep lower 32 bits.
    int64 immVal = static_cast<int64>(
        static_cast<uint64>(static_cast<ImmOperand &>(curInsn->GetOperand(kInsnSecondOpnd)).GetValue()) &
        0x00000000FFFFFFFFULL);
    ImmOperand &immOpnd = opndBuilder->CreateImm(k64BitSize, immVal, true);
    curInsn->SetOperand(kInsnSecondOpnd, immOpnd);
    if (curInsn->VerifySelf()) {
        return;
    }
    RegOperand &destReg = static_cast<RegOperand &>(curInsn->GetOperand(kInsnFirstOpnd));
    auto *bb = curInsn->GetBB();
    uint64 chunkVal0 = static_cast<uint64>(immVal) & 0x0000FFFFULL;
    ImmOperand &src0 = opndBuilder->CreateImm(k16BitSize, static_cast<int64>(chunkVal0), false);
    Insn &movInsn = insnBuilder->BuildInsn(MOP_wmovri32, destReg, src0);
    movInsn.SetBB(bb);
    (void)bb->InsertInsnBefore(*curInsn, movInsn);
    uint64 chunkVal1 = (static_cast<uint64>(immVal) >> k16BitSize) & 0x0000FFFFULL;
    ImmOperand &src16 = opndBuilder->CreateImm(k16BitSize, static_cast<int64>(chunkVal1), false);
    BitShiftOperand *lslOpnd = &opndBuilder->CreateBitShift(BitShiftOperand::kLSL, static_cast<uint32>(k16BitSize), 6);
    Insn &movkInsn = insnBuilder->BuildInsn(MOP_wmovkri16, destReg, src16, *lslOpnd);
    movkInsn.SetBB(bb);
    (void)bb->InsertInsnBefore(*curInsn, movkInsn);
    bb->RemoveInsn(*curInsn);
}

// split mov x0, #imm to movz/movn and movk
inline void MOP_xmovri64Split(Insn *curInsn, bool /* isAfterRegAlloc */, InsnBuilder *insnBuilder,
                              OperandBuilder *opndBuilder)
{
    if (curInsn->VerifySelf()) {
        return;
    }
    RegOperand &destReg = static_cast<RegOperand &>(curInsn->GetOperand(kInsnFirstOpnd));
    int64 immVal = static_cast<ImmOperand &>(curInsn->GetOperand(kInsnSecondOpnd)).GetValue();
    bool useMovz = BetterUseMOVZ(static_cast<uint64>(immVal));
    bool useMovk = false;
    // get lower 32 bits of the immediate
    uint64 chunkLval = static_cast<uint64>(immVal) & 0xFFFFFFFFULL;
    // get upper 32 bits of the immediate
    uint64 chunkHval = (static_cast<uint64>(immVal) >> k32BitSize) & 0xFFFFFFFFULL;
    int32 maxLoopTime = 4;
    if (chunkLval == chunkHval) {
        // compute lower 32 bits, and then copy to higher 32 bits, so only 2 chunks need be processed
        maxLoopTime = 2;
    }
    uint64 sa = 0;
    auto *bb = curInsn->GetBB();
    for (int64 i = 0; i < maxLoopTime; ++i, sa += k16BitSize) {
        // create an imm opereand which represents the i-th 16-bit chunk of the immediate
        uint64 chunkVal = (static_cast<uint64>(immVal) >> sa) & 0x0000FFFFULL;
        if (useMovz ? (chunkVal == 0) : (chunkVal == 0x0000FFFFULL)) {
            continue;
        }
        ImmOperand &src16 = opndBuilder->CreateImm(k16BitSize, static_cast<int64>(chunkVal), false);
        BitShiftOperand *lslOpnd = &opndBuilder->CreateBitShift(BitShiftOperand::kLSL, static_cast<uint32>(sa), 6);
        Insn *newInsn = nullptr;
        if (!useMovk) {
            // use movz or movn
            if (!useMovz) {
                src16.BitwiseNegate();
            }
            MOperator mOpCode = useMovz ? MOP_xmovzri16 : MOP_xmovnri16;
            newInsn = &insnBuilder->BuildInsn(mOpCode, destReg, src16, *lslOpnd);
            newInsn->SetBB(bb);
            useMovk = true;
        } else {
            newInsn = &insnBuilder->BuildInsn(MOP_xmovkri16, destReg, src16, *lslOpnd);
            newInsn->SetBB(bb);
        }
        (void)bb->InsertInsnBefore(*curInsn, *newInsn);
    }
    constexpr int32 loopTime = 2;
    if (maxLoopTime == loopTime) {
        // copy lower 32 bits to higher 32 bits
        ImmOperand &immOpnd = opndBuilder->CreateImm(k8BitSize, k32BitSize, false);
        Insn &insn = insnBuilder->BuildInsn(MOP_xbfirri6i6, destReg, destReg, immOpnd, immOpnd);
        insn.SetBB(bb);
        (void)bb->InsertInsnBefore(*curInsn, insn);
    }
    bb->RemoveInsn(*curInsn);
}

inline void MOP_xaddrri24Split(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    AddSubWithLslSplit(insn, true, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_xaddrri12Split(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    AddInsnSplit(insn, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_xaddsrri12Split(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    AddsSubsInsnSplit(insn, true, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_waddrri24Split(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    AddSubWithLslSplit(insn, true, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_waddrri12Split(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    AddInsnSplit(insn, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_waddsrri12Split(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    AddsSubsInsnSplit(insn, true, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_xsubrri24Split(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    AddSubWithLslSplit(insn, false, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_xsubrri12Split(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    SubInsnSplit(insn, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_xsubsrri12Split(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    AddsSubsInsnSplit(insn, false, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_wsubrri24Split(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    AddSubWithLslSplit(insn, false, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_wsubrri12Split(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    SubInsnSplit(insn, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_wsubsrri12Split(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    AddsSubsInsnSplit(insn, false, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_wldrsbSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_xldrsbSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_wldrbSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_wldrshSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_xldrshSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_xldrswSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_wldrhSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_wldrSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_xldrSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_bldrSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_hldrSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_sldrSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_dldrSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_qldrSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_wldpSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnThirdOpnd, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_xldpSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnThirdOpnd, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_xldpswSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnThirdOpnd, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_sldpSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnThirdOpnd, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_dldpSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnThirdOpnd, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_qldpSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnThirdOpnd, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_wccmpriicSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    CondCompareInsnSplit(insn, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_xccmpriicSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    CondCompareInsnSplit(insn, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_wstrbSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_wstrhSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_wstrSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_xstrSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_sstrSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_dstrSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_qstrSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnSecondOpnd, false, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_wstpSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnThirdOpnd, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_xstpSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnThirdOpnd, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_sstpSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnThirdOpnd, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_dstpSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnThirdOpnd, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_qstpSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder, OperandBuilder *opndBuilder)
{
    LoadStoreInsnSplit(insn, kInsnThirdOpnd, true, isAfterRegAlloc, insnBuilder, opndBuilder);
}

inline void MOP_assert_nonnullSplit(Insn *insn, bool isAfterRegAlloc, InsnBuilder *insnBuilder,
                                    OperandBuilder *opndBuilder)
{
    MOP_wldrSplit(insn, isAfterRegAlloc, insnBuilder, opndBuilder);
}
} /* namespace maplebe */
#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_MOP_SPLIT_H */
