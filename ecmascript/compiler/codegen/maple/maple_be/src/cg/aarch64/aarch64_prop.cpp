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

#include "aarch64_prop.h"
#include "aarch64_isa.h"
#include "aarch64_cg.h"
#include "aarch64_reg_coalesce.h"
#include <climits>

namespace maplebe {

#define PROP_DUMP CG_DEBUG_FUNC(cgFunc)

bool MayOverflow(const ImmOperand &value1, const ImmOperand &value2, bool is64Bit, bool isAdd, bool isSigned)
{
    if (value1.GetVary() || value2.GetVary()) {
        return false;
    }
    int64 cstA = value1.GetValue();
    int64 cstB = value2.GetValue();
    if (isAdd) {
        int64 res = static_cast<int64>(static_cast<uint64>(cstA) + static_cast<uint64>(cstB));
        if (!isSigned) {
            return static_cast<uint64>(res) < static_cast<uint64>(cstA);
        }
        auto rightShiftNumToGetSignFlag = (is64Bit ? 64 : 32) - 1;
        return (static_cast<uint64>(res) >> rightShiftNumToGetSignFlag !=
                static_cast<uint64>(cstA) >> rightShiftNumToGetSignFlag) &&
               (static_cast<uint64>(res) >> rightShiftNumToGetSignFlag !=
                static_cast<uint64>(cstB) >> rightShiftNumToGetSignFlag);
    } else {
        /* sub */
        if (!isSigned) {
            return cstA < cstB;
        }
        int64 res = static_cast<int64>(static_cast<uint64>(cstA) - static_cast<uint64>(cstB));
        auto rightShiftNumToGetSignFlag = (is64Bit ? 64 : 32) - 1;
        return (static_cast<uint64>(cstA) >> rightShiftNumToGetSignFlag !=
                static_cast<uint64>(cstB) >> rightShiftNumToGetSignFlag) &&
               (static_cast<uint64>(res) >> rightShiftNumToGetSignFlag !=
                static_cast<uint64>(cstA) >> rightShiftNumToGetSignFlag);
    }
}

bool AArch64Prop::IsInLimitCopyRange(VRegVersion *toBeReplaced)
{
    uint32 baseID = toBeReplaced->GetDefInsnInfo()->GetInsn()->GetId();
    MapleUnorderedMap<uint32, DUInsnInfo *> &useList = toBeReplaced->GetAllUseInsns();
    for (auto it : useList) {
        if (it.second->GetInsn()->GetId() - baseID > k16BitSize) {
            return false;
        }
    }
    return true;
}

void AArch64Prop::CopyProp()
{
    PropOptimizeManager optManager;
    optManager.Optimize<CopyRegProp>(*cgFunc, GetSSAInfo(), GetRegll());
    optManager.Optimize<ValidBitNumberProp>(*cgFunc, GetSSAInfo());
    optManager.Optimize<RedundantPhiProp>(*cgFunc, GetSSAInfo());
}

void AArch64Prop::TargetProp(Insn &insn)
{
    A64ConstProp a64ConstProp(*memPool, *cgFunc, *GetSSAInfo(), insn);
    a64ConstProp.DoOpt();
    A64StrLdrProp a64StrLdrProp(*memPool, *cgFunc, *GetSSAInfo(), insn, *GetDce());
    a64StrLdrProp.DoOpt();
}

void A64ConstProp::DoOpt()
{
    if (curInsn->GetMachineOpcode() == MOP_wmovri32 || curInsn->GetMachineOpcode() == MOP_xmovri64) {
        Operand &destOpnd = curInsn->GetOperand(kInsnFirstOpnd);
        CHECK_FATAL(destOpnd.IsRegister(), "must be reg operand");
        auto &destReg = static_cast<RegOperand &>(destOpnd);
        if (destReg.IsSSAForm()) {
            VRegVersion *destVersion = ssaInfo->FindSSAVersion(destReg.GetRegisterNumber());
            DEBUG_ASSERT(destVersion != nullptr, "find Version failed");
            Operand &constOpnd = curInsn->GetOperand(kInsnSecondOpnd);
            CHECK_FATAL(constOpnd.IsImmediate(), "must be imm operand");
            auto &immOperand = static_cast<ImmOperand &>(constOpnd);
            bool isZero = immOperand.IsZero();
            for (auto useDUInfoIt : destVersion->GetAllUseInsns()) {
                if (isZero) {
                    ZeroRegProp(*useDUInfoIt.second, *destVersion->GetSSAvRegOpnd());
                    destVersion->CheckDeadUse(*useDUInfoIt.second->GetInsn());
                }
                (void)ConstProp(*useDUInfoIt.second, immOperand);
            }
        }
    }
}

void A64ConstProp::ZeroRegProp(DUInsnInfo &useDUInfo, RegOperand &toReplaceReg)
{
    auto *useInsn = useDUInfo.GetInsn();
    const InsnDesc *md = &AArch64CG::kMd[(useInsn->GetMachineOpcode())];
    /* special case */
    bool isSpecficCase = useInsn->GetMachineOpcode() == MOP_wbfirri5i5 || useInsn->GetMachineOpcode() == MOP_xbfirri6i6;
    isSpecficCase &=
        (useDUInfo.GetOperands().size() == 1) && (useDUInfo.GetOperands().begin()->first == kInsnSecondOpnd);
    if (useInsn->IsStore() || md->IsCondDef() || isSpecficCase) {
        RegOperand &zeroOpnd = cgFunc->GetZeroOpnd(toReplaceReg.GetSize());
        for (auto &opndIt : useDUInfo.GetOperands()) {
            if (useInsn->IsStore() && opndIt.first != 0) {
                return;
            }
            Operand &opnd = useInsn->GetOperand(opndIt.first);
            A64ReplaceRegOpndVisitor replaceRegOpndVisitor(*cgFunc, *useInsn, opndIt.first, toReplaceReg, zeroOpnd);
            opnd.Accept(replaceRegOpndVisitor);
            useDUInfo.ClearDU(opndIt.first);
        }
    }
}

MOperator A64ConstProp::GetReversalMOP(MOperator arithMop)
{
    switch (arithMop) {
        case MOP_waddrri12:
            return MOP_wsubrri12;
        case MOP_xaddrri12:
            return MOP_xsubrri12;
        case MOP_xsubrri12:
            return MOP_xaddrri12;
        case MOP_wsubrri12:
            return MOP_waddrri12;
        default:
            CHECK_FATAL(false, "NYI");
            break;
    }
    return MOP_undef;
}

MOperator A64ConstProp::GetRegImmMOP(MOperator regregMop, bool withLeftShift)
{
    switch (regregMop) {
        case MOP_xaddrrrs:
        case MOP_xaddrrr: {
            return withLeftShift ? MOP_xaddrri24 : MOP_xaddrri12;
        }
        case MOP_waddrrrs:
        case MOP_waddrrr: {
            return withLeftShift ? MOP_waddrri24 : MOP_waddrri12;
        }
        case MOP_xsubrrrs:
        case MOP_xsubrrr: {
            return withLeftShift ? MOP_xsubrri24 : MOP_xsubrri12;
        }
        case MOP_wsubrrrs:
        case MOP_wsubrrr: {
            return withLeftShift ? MOP_wsubrri24 : MOP_wsubrri12;
        }
        case MOP_xandrrrs:
            return MOP_xandrri13;
        case MOP_wandrrrs:
            return MOP_wandrri12;
        case MOP_xeorrrrs:
            return MOP_xeorrri13;
        case MOP_weorrrrs:
            return MOP_weorrri12;
        case MOP_xiorrrrs:
        case MOP_xbfirri6i6:
            return MOP_xiorrri13;
        case MOP_wiorrrrs:
        case MOP_wbfirri5i5:
            return MOP_wiorrri12;
        case MOP_xmovrr: {
            return MOP_xmovri64;
        }
        case MOP_wmovrr: {
            return MOP_wmovri32;
        }
        default:
            CHECK_FATAL(false, "NYI");
            break;
    }
    return MOP_undef;
}

MOperator A64ConstProp::GetFoldMopAndVal(int64 &newVal, int64 constVal, const Insn &arithInsn)
{
    MOperator arithMop = arithInsn.GetMachineOpcode();
    MOperator newMop = MOP_undef;
    switch (arithMop) {
        case MOP_waddrrr:
        case MOP_xaddrrr: {
            newVal = constVal + constVal;
            newMop = (arithMop == MOP_waddrrr) ? MOP_wmovri32 : MOP_xmovri64;
            break;
        }
        case MOP_waddrrrs:
        case MOP_xaddrrrs: {
            auto &shiftOpnd = static_cast<BitShiftOperand &>(arithInsn.GetOperand(kInsnFourthOpnd));
            uint32 amount = shiftOpnd.GetShiftAmount();
            BitShiftOperand::ShiftOp sOp = shiftOpnd.GetShiftOp();
            switch (sOp) {
                case BitShiftOperand::kLSL: {
                    newVal = constVal + static_cast<int64>((static_cast<unsigned>(constVal) << amount));
                    break;
                }
                case BitShiftOperand::kLSR: {
                    newVal = constVal + (static_cast<unsigned>(constVal) >> amount);
                    break;
                }
                case BitShiftOperand::kASR: {
                    newVal = constVal + (constVal >> amount);
                    break;
                }
                default:
                    CHECK_FATAL(false, "NYI");
                    break;
            }
            newMop = (arithMop == MOP_waddrrrs) ? MOP_wmovri32 : MOP_xmovri64;
            break;
        }
        case MOP_wsubrrr:
        case MOP_xsubrrr: {
            newVal = 0;
            newMop = (arithMop == MOP_wsubrrr) ? MOP_wmovri32 : MOP_xmovri64;
            break;
        }
        case MOP_wsubrrrs:
        case MOP_xsubrrrs: {
            auto &shiftOpnd = static_cast<BitShiftOperand &>(arithInsn.GetOperand(kInsnFourthOpnd));
            uint32 amount = shiftOpnd.GetShiftAmount();
            BitShiftOperand::ShiftOp sOp = shiftOpnd.GetShiftOp();
            switch (sOp) {
                case BitShiftOperand::kLSL: {
                    newVal = constVal - static_cast<int64>((static_cast<unsigned>(constVal) << amount));
                    break;
                }
                case BitShiftOperand::kLSR: {
                    newVal = constVal - (static_cast<unsigned>(constVal) >> amount);
                    break;
                }
                case BitShiftOperand::kASR: {
                    newVal = constVal - (constVal >> amount);
                    break;
                }
                default:
                    CHECK_FATAL(false, "NYI");
                    break;
            }
            newMop = (arithMop == MOP_wsubrrrs) ? MOP_wmovri32 : MOP_xmovri64;
            break;
        }
        default:
            DEBUG_ASSERT(false, "this case is not supported currently");
            break;
    }
    return newMop;
}

void A64ConstProp::ReplaceInsnAndUpdateSSA(Insn &oriInsn, Insn &newInsn) const
{
    ssaInfo->ReplaceInsn(oriInsn, newInsn);
    oriInsn.GetBB()->ReplaceInsn(oriInsn, newInsn);
    /* dump insn replacement here */
}

bool A64ConstProp::MovConstReplace(DUInsnInfo &useDUInfo, ImmOperand &constOpnd)
{
    Insn *useInsn = useDUInfo.GetInsn();
    MOperator curMop = useInsn->GetMachineOpcode();
    if (useDUInfo.GetOperands().size() == 1) {
        MOperator newMop = GetRegImmMOP(curMop, false);
        Operand &destOpnd = useInsn->GetOperand(kInsnFirstOpnd);
        if (constOpnd.IsSingleInstructionMovable(destOpnd.GetSize())) {
            auto useOpndInfoIt = useDUInfo.GetOperands().begin();
            uint32 useOpndIdx = useOpndInfoIt->first;
            DEBUG_ASSERT(useOpndIdx == kInsnSecondOpnd, "invalid instruction in ssa form");
            if (useOpndIdx == kInsnSecondOpnd) {
                Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, destOpnd, constOpnd);
                ReplaceInsnAndUpdateSSA(*useInsn, newInsn);
                return true;
            }
        }
    } else {
        DEBUG_ASSERT(false, "invalid instruction in ssa form");
    }
    return false;
}

/* support add now */
bool A64ConstProp::ArithmeticConstReplace(DUInsnInfo &useDUInfo, ImmOperand &constOpnd, ArithmeticType aT)
{
    Insn *useInsn = useDUInfo.GetInsn();
    MOperator curMop = useInsn->GetMachineOpcode();
    if (useDUInfo.GetOperands().size() == 1) {
        MOperator newMop = GetRegImmMOP(curMop, false);
        auto useOpndInfoIt = useDUInfo.GetOperands().begin();
        uint32 useOpndIdx = useOpndInfoIt->first;
        CHECK_FATAL(useOpndIdx == kInsnSecondOpnd || useOpndIdx == kInsnThirdOpnd, "check this insn");
        Insn *newInsn = nullptr;
        auto &tempImm = static_cast<ImmOperand &>(*constOpnd.Clone(*constPropMp));
        if (tempImm.GetSize() == k64BitSize && useInsn->GetOperandSize(useOpndIdx) == k32BitSize) {
            uint64 newVal = static_cast<uint64>(tempImm.GetValue()) & UINT32_MAX;
            tempImm.SetValue(static_cast<int64>(newVal));
        }
        if (static_cast<AArch64CGFunc *>(cgFunc)->IsOperandImmValid(newMop, &constOpnd, kInsnThirdOpnd)) {
            if (useOpndIdx == kInsnThirdOpnd) {
                newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(newMop, useInsn->GetOperand(kInsnFirstOpnd),
                                                               useInsn->GetOperand(kInsnSecondOpnd), constOpnd);
            } else if (useOpndIdx == kInsnSecondOpnd && aT == kAArch64Add) { /* swap operand due to legality in aarch */
                newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(newMop, useInsn->GetOperand(kInsnFirstOpnd),
                                                               useInsn->GetOperand(kInsnThirdOpnd), constOpnd);
            }
        }
        /* try aggressive opt in aarch64 add and sub */
        if (newInsn == nullptr && (aT == kAArch64Add || aT == kAArch64Sub)) {
            auto *tempImm = static_cast<ImmOperand *>(constOpnd.Clone(*constPropMp));
            /* try aarch64 imm shift mode, aarch64 data processing instructions have 12 bits of space for values */
            tempImm->SetValue(tempImm->GetValue() >> 12);
            if (static_cast<AArch64CGFunc *>(cgFunc)->IsOperandImmValid(newMop, tempImm, kInsnThirdOpnd) &&
                CGOptions::GetInstance().GetOptimizeLevel() < CGOptions::kLevel0) {
                DEBUG_ASSERT(false, "NIY");
            }
            auto *zeroImm = &(static_cast<AArch64CGFunc *>(cgFunc)->CreateImmOperand(0, constOpnd.GetSize(), true));
            /* value in immOpnd is signed */
            if (MayOverflow(*zeroImm, constOpnd, constOpnd.GetSize() == k64BitSize, false, true)) {
                return false;
            }
            /* (constA - var) can not reversal to (var + (-constA)) */
            if (useOpndIdx == kInsnSecondOpnd && aT == kAArch64Sub) {
                return false;
            }
            /* Addition and subtraction reversal */
            tempImm->SetValue(-constOpnd.GetValue());
            newMop = GetReversalMOP(newMop);
            if (static_cast<AArch64CGFunc *>(cgFunc)->IsOperandImmValid(newMop, tempImm, kInsnThirdOpnd)) {
                auto *cgImm = static_cast<ImmOperand *>(tempImm->Clone(*cgFunc->GetMemoryPool()));
                newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(newMop, useInsn->GetOperand(kInsnFirstOpnd),
                                                               useInsn->GetOperand(kInsnSecondOpnd), *cgImm);
                if (useOpndIdx == kInsnSecondOpnd) { /* swap operand due to legality in aarch */
                    newInsn->SetOperand(kInsnSecondOpnd, useInsn->GetOperand(kInsnThirdOpnd));
                }
            }
        }
        if (newInsn != nullptr) {
            ReplaceInsnAndUpdateSSA(*useInsn, *newInsn);
            return true;
        }
    } else if (useDUInfo.GetOperands().size() == kOpndNum2) {
        /* only support add & sub now */
        int64 newValue = 0;
        MOperator newMop = GetFoldMopAndVal(newValue, constOpnd.GetValue(), *useInsn);
        bool isSigned = (newValue < 0);
        auto *tempImm = static_cast<ImmOperand *>(constOpnd.Clone(*constPropMp));
        tempImm->SetValue(newValue);
        tempImm->SetSigned(isSigned);
        if (tempImm->IsSingleInstructionMovable()) {
            auto *newImmOpnd = static_cast<ImmOperand *>(tempImm->Clone(*cgFunc->GetMemoryPool()));
            auto &newInsn =
                cgFunc->GetInsnBuilder()->BuildInsn(newMop, useInsn->GetOperand(kInsnFirstOpnd), *newImmOpnd);
            ReplaceInsnAndUpdateSSA(*useInsn, newInsn);
            return true;
        } else {
            CHECK_FATAL(false, "invalid immediate");
        }
    } else {
        DEBUG_ASSERT(false, "invalid instruction in ssa form");
    }
    return false;
}

bool A64ConstProp::ArithmeticConstFold(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd, ArithmeticType aT)
{
    Insn *useInsn = useDUInfo.GetInsn();
    if (useDUInfo.GetOperands().size() == 1) {
        Operand &existedImm = useInsn->GetOperand(kInsnThirdOpnd);
        DEBUG_ASSERT(existedImm.IsImmediate(), "must be");
        Operand &destOpnd = useInsn->GetOperand(kInsnFirstOpnd);
        bool is64Bit = destOpnd.GetSize() == k64BitSize;
        ImmOperand *foldConst = CanDoConstFold(constOpnd, static_cast<ImmOperand &>(existedImm), aT, is64Bit);
        if (foldConst != nullptr) {
            MOperator newMop = is64Bit ? MOP_xmovri64 : MOP_wmovri32;
            Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, destOpnd, *foldConst);
            ReplaceInsnAndUpdateSSA(*useInsn, newInsn);
            return true;
        }
    }
    return false;
}

bool A64ConstProp::ShiftConstReplace(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd)
{
    Insn *useInsn = useDUInfo.GetInsn();
    MOperator curMop = useInsn->GetMachineOpcode();
    if (useDUInfo.GetOperands().size() == 1) {
        auto useOpndInfoIt = useDUInfo.GetOperands().begin();
        uint32 useOpndIdx = useOpndInfoIt->first;
        if (useOpndIdx == kInsnThirdOpnd) {
            auto &shiftBit = static_cast<BitShiftOperand &>(useInsn->GetOperand(kInsnFourthOpnd));
            int64 val = constOpnd.GetValue();
            if (shiftBit.GetShiftOp() == BitShiftOperand::kLSL) {
                val = val << shiftBit.GetShiftAmount();
            } else if (shiftBit.GetShiftOp() == BitShiftOperand::kLSR) {
                val = val >> shiftBit.GetShiftAmount();
            } else if (shiftBit.GetShiftOp() == BitShiftOperand::kASR) {
                val = static_cast<int64>((static_cast<uint64>(val)) >> shiftBit.GetShiftAmount());
            } else {
                CHECK_FATAL(false, "shift type is not defined");
            }
            auto *newImm = static_cast<ImmOperand *>(constOpnd.Clone(*constPropMp));
            newImm->SetValue(val);
            MOperator newMop = GetRegImmMOP(curMop, false);
            if (static_cast<AArch64CGFunc *>(cgFunc)->IsOperandImmValid(newMop, newImm, kInsnThirdOpnd)) {
                auto *cgNewImm = static_cast<ImmOperand *>(constOpnd.Clone(*cgFunc->GetMemoryPool()));
                Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, useInsn->GetOperand(kInsnFirstOpnd),
                                                                    useInsn->GetOperand(kInsnSecondOpnd), *cgNewImm);
                ReplaceInsnAndUpdateSSA(*useInsn, newInsn);
                return true;
            }
        }
    }
    return false;
}

bool A64ConstProp::ConstProp(DUInsnInfo &useDUInfo, ImmOperand &constOpnd)
{
    MOperator curMop = useDUInfo.GetInsn()->GetMachineOpcode();
    switch (curMop) {
        case MOP_xmovrr:
        case MOP_wmovrr: {
            return MovConstReplace(useDUInfo, constOpnd);
        }
        case MOP_xsubrrr:
        case MOP_wsubrrr: {
            return ArithmeticConstReplace(useDUInfo, constOpnd, kAArch64Sub);
        }
        case MOP_xaddrrr:
        case MOP_waddrrr: {
            return ArithmeticConstReplace(useDUInfo, constOpnd, kAArch64Add);
        }
        case MOP_waddrri12:
        case MOP_xaddrri12: {
            return ArithmeticConstFold(useDUInfo, constOpnd, kAArch64Add);
        }
        case MOP_xsubrri12:
        case MOP_wsubrri12: {
            return ArithmeticConstFold(useDUInfo, constOpnd, kAArch64Sub);
        }
        case MOP_xiorrrrs:
        case MOP_wiorrrrs:
        case MOP_xeorrrrs:
        case MOP_weorrrrs:
        case MOP_xandrrrs:
        case MOP_wandrrrs:
        case MOP_xaddrrrs:
        case MOP_waddrrrs:
        case MOP_wsubrrrs:
        case MOP_xsubrrrs: {
            return ShiftConstReplace(useDUInfo, constOpnd);
        }
        case MOP_wbfirri5i5:
        case MOP_xbfirri6i6: {
            return BitInsertReplace(useDUInfo, constOpnd);
        }
        default:
            break;
    }
    return false;
}

bool A64ConstProp::BitInsertReplace(DUInsnInfo &useDUInfo, const ImmOperand &constOpnd)
{
    Insn *useInsn = useDUInfo.GetInsn();
    MOperator curMop = useInsn->GetMachineOpcode();
    if (useDUInfo.GetOperands().size() == 1) {
        auto useOpndInfoIt = useDUInfo.GetOperands().begin();
        uint32 useOpndIdx = useOpndInfoIt->first;
        if (useOpndIdx == kInsnSecondOpnd) {
            auto &lsbOpnd = static_cast<ImmOperand &>(useInsn->GetOperand(kInsnThirdOpnd));
            auto &widthOpnd = static_cast<ImmOperand &>(useInsn->GetOperand(kInsnFourthOpnd));
            auto val = static_cast<uint64>(constOpnd.GetValue());
            /* bfi width in the range [1 -64] */
            auto width = static_cast<uint64>(widthOpnd.GetValue());
            /*  bit number of the lsb of the destination bitfield */
            auto lsb = static_cast<uint64>(lsbOpnd.GetValue());
            val = val & ((1U << width) - 1U);
            if (static_cast<uint64>(__builtin_popcountl(val)) == width) {
                val = val << lsb;
                MOperator newMop = GetRegImmMOP(curMop, false);
                Operand &newOpnd = cgFunc->CreateImmOperand(PTY_i64, static_cast<int64>(val));
                if (static_cast<AArch64CGFunc *>(cgFunc)->IsOperandImmValid(newMop, &newOpnd, kInsnThirdOpnd)) {
                    Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, useInsn->GetOperand(kInsnFirstOpnd),
                                                                        useInsn->GetOperand(kInsnFirstOpnd), newOpnd);
                    ReplaceInsnAndUpdateSSA(*useInsn, newInsn);
                    return true;
                }
            }
        }
    }
    return false;
}

ImmOperand *A64ConstProp::CanDoConstFold(const ImmOperand &value1, const ImmOperand &value2, ArithmeticType aT,
                                         bool is64Bit)
{
    auto *tempImm = static_cast<ImmOperand *>(value1.Clone(*constPropMp));
    int64 newVal = 0;
    bool isSigned = value1.IsSignedValue();
    if (value1.IsSignedValue() != value2.IsSignedValue()) {
        isSigned = false;
    }
    if (MayOverflow(value1, value2, is64Bit, aT == kAArch64Add, isSigned)) {
        return nullptr;
    }
    switch (aT) {
        case kAArch64Add: {
            newVal = value1.GetValue() + value2.GetValue();
            break;
        }
        case kAArch64Sub: {
            newVal = value1.GetValue() - value2.GetValue();
            break;
        }
        default:
            return nullptr;
    }
    if (!is64Bit && isSigned && (newVal > INT_MAX || newVal < INT_MIN)) {
        return nullptr;
    }
    if (!is64Bit && !isSigned && (newVal > UINT_MAX || newVal < 0)) {
        return nullptr;
    }
    if (newVal < 0) {
        tempImm->SetSigned();
    }
    tempImm->SetValue(newVal);
    if (value2.GetVary() == kUnAdjustVary) {
        tempImm->SetVary(kUnAdjustVary);
    }
    bool canBeMove = tempImm->IsSingleInstructionMovable(k64BitSize);
    return canBeMove ? static_cast<ImmOperand *>(tempImm->Clone(*cgFunc->GetMemoryPool())) : nullptr;
}

void A64StrLdrProp::DoOpt()
{
    DEBUG_ASSERT(curInsn != nullptr, "not input insn");
    bool tryOptAgain = false;
    do {
        tryOptAgain = false;
        MemOperand *currMemOpnd = StrLdrPropPreCheck(*curInsn);
        if (currMemOpnd != nullptr && memPropMode != kUndef) {
            /* can be changed to recursive propagation */
            if (ReplaceMemOpnd(*currMemOpnd, nullptr)) {
                tryOptAgain = true;
            }
            replaceVersions.clear();
        }
    } while (tryOptAgain);
}

bool A64StrLdrProp::ReplaceMemOpnd(const MemOperand &currMemOpnd, const Insn *defInsn)
{
    auto GetDefInsn = [&defInsn, this](const RegOperand &regOpnd, std::vector<Insn *> &allUseInsns) -> void {
        if (regOpnd.IsSSAForm() && defInsn == nullptr) {
            VRegVersion *replacedV = ssaInfo->FindSSAVersion(regOpnd.GetRegisterNumber());
            if (replacedV->GetDefInsnInfo() != nullptr) {
                for (auto it : replacedV->GetAllUseInsns()) {
                    allUseInsns.emplace_back(it.second->GetInsn());
                }
                defInsn = replacedV->GetDefInsnInfo()->GetInsn();
            }
        }
    };
    RegOperand *replacedReg = nullptr;
    std::vector<Insn *> allUseInsns;
    std::vector<MemOperand *> newMemOpnds;
    bool doFullReplaceProp = true; /* due to register pressure, do not do partial prop */
    if (memPropMode == kPropBase) {
        replacedReg = currMemOpnd.GetBaseRegister();
    } else {
        Operand *offset = currMemOpnd.GetOffset();
        DEBUG_ASSERT(offset->IsRegister(), "must be");
        replacedReg = static_cast<RegOperand *>(offset);
    }
    CHECK_FATAL(replacedReg != nullptr, "check this insn");
    GetDefInsn(*replacedReg, allUseInsns);
    if (defInsn != nullptr) {
        for (auto useInsn : allUseInsns) {
            MemOperand *oldMemOpnd = StrLdrPropPreCheck(*useInsn, memPropMode);
            if (CheckSameReplace(*replacedReg, oldMemOpnd)) {
                MemOperand *newMemOpnd = SelectReplaceMem(*defInsn, *oldMemOpnd);
                if (newMemOpnd != nullptr) {
                    uint32 opndIdx = GetMemOpndIdx(oldMemOpnd, *useInsn);
                    if (CheckNewMemOffset(*useInsn, newMemOpnd, opndIdx)) {
                        newMemOpnds.emplace_back(newMemOpnd);
                        continue;
                    }
                }
            }
            doFullReplaceProp = false;
            break;
        }
    } else {
        doFullReplaceProp = false;
    }
    if (doFullReplaceProp) {
        for (size_t i = 0; i < newMemOpnds.size(); ++i) {
            DoMemReplace(*replacedReg, *newMemOpnds[i], *allUseInsns[i]);
        }
        return true;
    }
    return false;
}

bool A64StrLdrProp::CheckSameReplace(const RegOperand &replacedReg, const MemOperand *memOpnd) const
{
    if (memOpnd != nullptr && memPropMode != kUndef) {
        if (memPropMode == kPropBase) {
            return replacedReg.GetRegisterNumber() == memOpnd->GetBaseRegister()->GetRegisterNumber();
        } else {
            Operand *offset = memOpnd->GetOffset();
            DEBUG_ASSERT(offset->IsRegister(), "must be");
            return replacedReg.GetRegisterNumber() == static_cast<RegOperand *>(offset)->GetRegisterNumber();
        }
    }
    return false;
}

uint32 A64StrLdrProp::GetMemOpndIdx(MemOperand *newMemOpnd, const Insn &insn) const
{
    uint32 opndIdx = kInsnMaxOpnd;
    if (insn.IsLoadPair() || insn.IsStorePair()) {
        DEBUG_ASSERT(newMemOpnd->GetOffsetImmediate() != nullptr, "unexpect insn");
        opndIdx = kInsnThirdOpnd;
    } else {
        opndIdx = kInsnSecondOpnd;
    }
    return opndIdx;
}

void A64StrLdrProp::DoMemReplace(const RegOperand &replacedReg, MemOperand &newMem, Insn &useInsn)
{
    VRegVersion *replacedV = ssaInfo->FindSSAVersion(replacedReg.GetRegisterNumber());
    DEBUG_ASSERT(replacedV != nullptr, "must in ssa form");
    uint32 opndIdx = GetMemOpndIdx(&newMem, useInsn);
    replacedV->RemoveUseInsn(useInsn, opndIdx);
    if (replacedV->GetAllUseInsns().empty()) {
        (void)cgDce->RemoveUnuseDef(*replacedV);
    }
    for (auto &replaceit : replaceVersions) {
        replaceit.second->AddUseInsn(*ssaInfo, useInsn, opndIdx);
    }
    useInsn.SetOperand(opndIdx, newMem);
}

MemOperand *A64StrLdrProp::StrLdrPropPreCheck(const Insn &insn, MemPropMode prevMod)
{
    memPropMode = kUndef;
    if (insn.IsLoad() || insn.IsStore()) {
        if (insn.IsAtomic()) {
            return nullptr;
        }
        auto *currMemOpnd = static_cast<MemOperand *>(insn.GetMemOpnd());
        if (currMemOpnd != nullptr) {
            memPropMode = SelectStrLdrPropMode(*currMemOpnd);
            if (prevMod != kUndef) {
                if (prevMod != memPropMode) {
                    memPropMode = prevMod;
                    return nullptr;
                }
            }
            return currMemOpnd;
        }
    }
    return nullptr;
}

MemPropMode A64StrLdrProp::SelectStrLdrPropMode(const MemOperand &currMemOpnd)
{
    MemOperand::AArch64AddressingMode currAddrMode = currMemOpnd.GetAddrMode();
    MemPropMode innerMemPropMode = kUndef;
    switch (currAddrMode) {
        case MemOperand::kAddrModeBOi: {
            if (!currMemOpnd.IsPreIndexed() && !currMemOpnd.IsPostIndexed()) {
                innerMemPropMode = kPropBase;
            }
            break;
        }
        case MemOperand::kAddrModeBOrX: {
            innerMemPropMode = kPropOffset;
            auto amount = currMemOpnd.ShiftAmount();
            if (currMemOpnd.GetExtendAsString() == "LSL") {
                if (amount != 0) {
                    innerMemPropMode = kPropShift;
                }
                break;
            } else if (currMemOpnd.SignedExtend()) {
                innerMemPropMode = kPropSignedExtend;
            } else if (currMemOpnd.UnsignedExtend()) {
                innerMemPropMode = kPropUnsignedExtend;
            }
            break;
        }
        default:
            innerMemPropMode = kUndef;
    }
    return innerMemPropMode;
}

MemOperand *A64StrLdrProp::SelectReplaceMem(const Insn &defInsn, const MemOperand &currMemOpnd)
{
    MemOperand *newMemOpnd = nullptr;
    Operand *offset = currMemOpnd.GetOffset();
    RegOperand *base = currMemOpnd.GetBaseRegister();
    MOperator opCode = defInsn.GetMachineOpcode();
    constexpr uint32 kValidShiftAmount = 12;
    switch (opCode) {
        case MOP_xsubrri12:
        case MOP_wsubrri12: {
            RegOperand *replace = GetReplaceReg(static_cast<RegOperand &>(defInsn.GetOperand(kInsnSecondOpnd)));
            if (replace != nullptr) {
                auto &immOpnd = static_cast<ImmOperand &>(defInsn.GetOperand(kInsnThirdOpnd));
                int64 defVal = -(immOpnd.GetValue());
                newMemOpnd = HandleArithImmDef(*replace, offset, defVal, currMemOpnd.GetSize());
            }
            break;
        }
        case MOP_xaddrri12:
        case MOP_waddrri12: {
            RegOperand *replace = GetReplaceReg(static_cast<RegOperand &>(defInsn.GetOperand(kInsnSecondOpnd)));
            if (replace != nullptr) {
                auto &immOpnd = static_cast<ImmOperand &>(defInsn.GetOperand(kInsnThirdOpnd));
                int64 defVal = immOpnd.GetValue();
                newMemOpnd = HandleArithImmDef(*replace, offset, defVal, currMemOpnd.GetSize());
            }
            break;
        }
        case MOP_xaddrri24:
        case MOP_waddrri24: {
            RegOperand *replace = GetReplaceReg(static_cast<RegOperand &>(defInsn.GetOperand(kInsnSecondOpnd)));
            if (replace != nullptr) {
                auto &immOpnd = static_cast<ImmOperand &>(defInsn.GetOperand(kInsnThirdOpnd));
                auto &shiftOpnd = static_cast<BitShiftOperand &>(defInsn.GetOperand(kInsnFourthOpnd));
                CHECK_FATAL(shiftOpnd.GetShiftAmount() == kValidShiftAmount, "invalid shiftAmount");
                int64 defVal = (immOpnd.GetValue() << shiftOpnd.GetShiftAmount());
                newMemOpnd = HandleArithImmDef(*replace, offset, defVal, currMemOpnd.GetSize());
            }
            break;
        }
        case MOP_xsubrri24:
        case MOP_wsubrri24: {
            RegOperand *replace = GetReplaceReg(static_cast<RegOperand &>(defInsn.GetOperand(kInsnSecondOpnd)));
            if (replace != nullptr) {
                auto &immOpnd = static_cast<ImmOperand &>(defInsn.GetOperand(kInsnThirdOpnd));
                auto &shiftOpnd = static_cast<BitShiftOperand &>(defInsn.GetOperand(kInsnFourthOpnd));
                CHECK_FATAL(shiftOpnd.GetShiftAmount() == kValidShiftAmount, "invalid shiftAmount");
                int64 defVal = -(immOpnd.GetValue() << shiftOpnd.GetShiftAmount());
                newMemOpnd = HandleArithImmDef(*replace, offset, defVal, currMemOpnd.GetSize());
            }
            break;
        }
        case MOP_xaddrrr:
        case MOP_waddrrr:
        case MOP_dadd:
        case MOP_sadd: {
            if (memPropMode == kPropBase) {
                auto *ofstOpnd = static_cast<ImmOperand *>(offset);
                if (!ofstOpnd->IsZero()) {
                    break;
                }

                RegOperand *replace = GetReplaceReg(static_cast<RegOperand &>(defInsn.GetOperand(kInsnSecondOpnd)));
                RegOperand *newOfst = GetReplaceReg(static_cast<RegOperand &>(defInsn.GetOperand(kInsnThirdOpnd)));

                if (replace != nullptr && newOfst != nullptr) {
                    newMemOpnd = static_cast<AArch64CGFunc *>(cgFunc)->CreateMemOperand(
                        MemOperand::kAddrModeBOrX, currMemOpnd.GetSize(), *replace, newOfst, nullptr, nullptr);
                }
            }
            break;
        }
        case MOP_xaddrrrs:
        case MOP_waddrrrs: {
            if (memPropMode == kPropBase) {
                auto *ofstOpnd = static_cast<ImmOperand *>(offset);
                if (!ofstOpnd->IsZero()) {
                    break;
                }
                RegOperand *newBaseOpnd = GetReplaceReg(static_cast<RegOperand &>(defInsn.GetOperand(kInsnSecondOpnd)));
                RegOperand *newIndexOpnd = GetReplaceReg(static_cast<RegOperand &>(defInsn.GetOperand(kInsnThirdOpnd)));
                auto &shift = static_cast<BitShiftOperand &>(defInsn.GetOperand(kInsnFourthOpnd));
                if (shift.GetShiftOp() != BitShiftOperand::kLSL) {
                    break;
                }
                if (newBaseOpnd != nullptr && newIndexOpnd != nullptr) {
                    newMemOpnd = static_cast<AArch64CGFunc *>(cgFunc)->CreateMemOperand(
                        MemOperand::kAddrModeBOrX, currMemOpnd.GetSize(), *newBaseOpnd, *newIndexOpnd,
                        shift.GetShiftAmount(), false);
                }
            }
            break;
        }
        case MOP_xadrpl12: {
            if (memPropMode == kPropBase) {
                if (currMemOpnd.GetSize() >= k128BitSize) {
                    // We can not be sure that the page offset is 16-byte aligned
                    break;
                }
                auto *ofstOpnd = static_cast<ImmOperand *>(offset);
                CHECK_FATAL(ofstOpnd != nullptr, "oldOffset is null!");
                int64 val = ofstOpnd->GetValue();
                auto *offset1 = static_cast<StImmOperand *>(&defInsn.GetOperand(kInsnThirdOpnd));
                CHECK_FATAL(offset1 != nullptr, "offset1 is null!");
                val += offset1->GetOffset();
                OfstOperand *newOfsetOpnd =
                    &static_cast<AArch64CGFunc *>(cgFunc)->CreateOfstOpnd(static_cast<uint64>(val), k32BitSize);
                CHECK_FATAL(newOfsetOpnd != nullptr, "newOfsetOpnd is null!");
                const MIRSymbol *addr = offset1->GetSymbol();
                /* do not guarantee rodata alignment at Os */
                if (CGOptions::OptimizeForSize() && addr->IsReadOnly()) {
                    break;
                }
                RegOperand *replace = GetReplaceReg(static_cast<RegOperand &>(defInsn.GetOperand(kInsnSecondOpnd)));
                if (replace != nullptr) {
                    newMemOpnd = static_cast<AArch64CGFunc *>(cgFunc)->CreateMemOperand(
                        MemOperand::kAddrModeLo12Li, currMemOpnd.GetSize(), *replace, nullptr, newOfsetOpnd, addr);
                }
            }
            break;
        }
        /* do this in const prop ? */
        case MOP_wmovri32:
        case MOP_xmovri64: {
            if (memPropMode == kPropOffset) {
                auto *imm = static_cast<ImmOperand *>(&defInsn.GetOperand(kInsnSecondOpnd));
                OfstOperand *newOffset = &static_cast<AArch64CGFunc *>(cgFunc)->CreateOfstOpnd(
                    static_cast<uint64>(imm->GetValue()), k32BitSize);
                CHECK_FATAL(newOffset != nullptr, "newOffset is null!");
                newMemOpnd = static_cast<AArch64CGFunc *>(cgFunc)->CreateMemOperand(
                    MemOperand::kAddrModeBOi, currMemOpnd.GetSize(), *base, nullptr, newOffset, nullptr);
            }
            break;
        }
        case MOP_xlslrri6:
        case MOP_wlslrri5: {
            auto *imm = static_cast<ImmOperand *>(&defInsn.GetOperand(kInsnThirdOpnd));
            RegOperand *newOfst = GetReplaceReg(static_cast<RegOperand &>(defInsn.GetOperand(kInsnSecondOpnd)));
            if (newOfst != nullptr) {
                uint32 shift = static_cast<uint32>(static_cast<int32>(imm->GetValue()));
                if (memPropMode == kPropOffset) {
                    if (shift < k4ByteSize) {
                        newMemOpnd = static_cast<AArch64CGFunc *>(cgFunc)->CreateMemOperand(
                            MemOperand::kAddrModeBOrX, currMemOpnd.GetSize(), *base, *newOfst, shift);
                    }
                } else if (memPropMode == kPropShift) {
                    shift += currMemOpnd.ShiftAmount();
                    if (shift < k4ByteSize) {
                        newMemOpnd = static_cast<AArch64CGFunc *>(cgFunc)->CreateMemOperand(
                            MemOperand::kAddrModeBOrX, currMemOpnd.GetSize(), *base, *newOfst, shift);
                    }
                }
            }
            break;
        }
        case MOP_xsxtw64: {
            newMemOpnd = SelectReplaceExt(defInsn, *base, static_cast<uint32>(currMemOpnd.ShiftAmount()), true,
                                          currMemOpnd.GetSize());
            break;
        }
        case MOP_xuxtw64: {
            newMemOpnd = SelectReplaceExt(defInsn, *base, static_cast<uint32>(currMemOpnd.ShiftAmount()), false,
                                          currMemOpnd.GetSize());
            break;
        }
        default:
            break;
    }
    return newMemOpnd;
}

RegOperand *A64StrLdrProp::GetReplaceReg(RegOperand &a64Reg)
{
    if (a64Reg.IsSSAForm()) {
        regno_t ssaIndex = a64Reg.GetRegisterNumber();
        replaceVersions[ssaIndex] = ssaInfo->FindSSAVersion(ssaIndex);
        DEBUG_ASSERT(replaceVersions.size() <= 2, "CHECK THIS CASE IN A64PROP");  // size <= 2 in A64PROP
        return &a64Reg;
    }
    return nullptr;
}

MemOperand *A64StrLdrProp::HandleArithImmDef(RegOperand &replace, Operand *oldOffset, int64 defVal,
                                             uint32 memSize) const
{
    if (memPropMode != kPropBase) {
        return nullptr;
    }
    OfstOperand *newOfstImm = nullptr;
    if (oldOffset == nullptr) {
        newOfstImm = &static_cast<AArch64CGFunc *>(cgFunc)->CreateOfstOpnd(static_cast<uint64>(defVal), k32BitSize);
    } else {
        auto *ofstOpnd = static_cast<OfstOperand *>(oldOffset);
        CHECK_FATAL(ofstOpnd != nullptr, "oldOffsetOpnd is null");
        newOfstImm = &static_cast<AArch64CGFunc *>(cgFunc)->CreateOfstOpnd(
            static_cast<uint64>(defVal + ofstOpnd->GetValue()), k32BitSize);
    }
    CHECK_FATAL(newOfstImm != nullptr, "newOffset is null!");
    return static_cast<AArch64CGFunc *>(cgFunc)->CreateMemOperand(MemOperand::kAddrModeBOi, memSize, replace, nullptr,
                                                                  newOfstImm, nullptr);
}

MemOperand *A64StrLdrProp::SelectReplaceExt(const Insn &defInsn, RegOperand &base, uint32 amount, bool isSigned,
                                            uint32 memSize)
{
    MemOperand *newMemOpnd = nullptr;
    RegOperand *newOfst = GetReplaceReg(static_cast<RegOperand &>(defInsn.GetOperand(kInsnSecondOpnd)));
    if (newOfst == nullptr) {
        return nullptr;
    }
    /* defInsn is extend, currMemOpnd is same extend or shift */
    bool propExtend = (memPropMode == kPropShift) || ((memPropMode == kPropSignedExtend) && isSigned) ||
                      ((memPropMode == kPropUnsignedExtend) && !isSigned);
    if (memPropMode == kPropOffset) {
        newMemOpnd = static_cast<AArch64CGFunc *>(cgFunc)->CreateMemOperand(MemOperand::kAddrModeBOrX, memSize, base,
                                                                            *newOfst, 0, isSigned);
    } else if (propExtend) {
        newMemOpnd = static_cast<AArch64CGFunc *>(cgFunc)->CreateMemOperand(MemOperand::kAddrModeBOrX, memSize, base,
                                                                            *newOfst, amount, isSigned);
    } else {
        return nullptr;
    }
    return newMemOpnd;
}

bool A64StrLdrProp::CheckNewMemOffset(const Insn &insn, MemOperand *newMemOpnd, uint32 opndIdx) const
{
    auto *a64CgFunc = static_cast<AArch64CGFunc *>(cgFunc);
    if ((newMemOpnd->GetOffsetImmediate() != nullptr) &&
        !a64CgFunc->IsOperandImmValid(insn.GetMachineOpcode(), newMemOpnd, opndIdx)) {
        return false;
    }
    auto newAmount = static_cast<uint32>(newMemOpnd->ShiftAmount());
    if (!AArch64StoreLoadOpt::CheckNewAmount(insn, newAmount)) {
        return false;
    }
    /* is ldp or stp, addrMode must be BOI */
    if ((opndIdx == kInsnThirdOpnd) && (newMemOpnd->GetAddrMode() != MemOperand::kAddrModeBOi)) {
        return false;
    }
    return true;
}

void AArch64Prop::PropPatternOpt()
{
    PropOptimizeManager optManager;
    optManager.Optimize<ExtendMovPattern>(*cgFunc, GetSSAInfo());
    optManager.Optimize<ExtendShiftPattern>(*cgFunc, GetSSAInfo());
    optManager.Optimize<FpSpConstProp>(*cgFunc, GetSSAInfo());
    optManager.Optimize<A64PregCopyPattern>(*cgFunc, GetSSAInfo());
}

bool ExtendShiftPattern::IsSwapInsn(const Insn &insn) const
{
    MOperator op = insn.GetMachineOpcode();
    switch (op) {
        case MOP_xaddrrr:
        case MOP_waddrrr:
        case MOP_xiorrrr:
        case MOP_wiorrrr:
            return true;
        default:
            return false;
    }
}

void ExtendShiftPattern::SetExMOpType(const Insn &use)
{
    MOperator op = use.GetMachineOpcode();
    switch (op) {
        case MOP_xaddrrr:
        case MOP_xxwaddrrre:
        case MOP_xaddrrrs: {
            exMOpType = kExAdd;
            break;
        }
        case MOP_waddrrr:
        case MOP_wwwaddrrre:
        case MOP_waddrrrs: {
            exMOpType = kEwAdd;
            break;
        }
        case MOP_xsubrrr:
        case MOP_xxwsubrrre:
        case MOP_xsubrrrs: {
            exMOpType = kExSub;
            break;
        }
        case MOP_wsubrrr:
        case MOP_wwwsubrrre:
        case MOP_wsubrrrs: {
            exMOpType = kEwSub;
            break;
        }
        case MOP_xcmnrr:
        case MOP_xwcmnrre:
        case MOP_xcmnrrs: {
            exMOpType = kExCmn;
            break;
        }
        case MOP_wcmnrr:
        case MOP_wwcmnrre:
        case MOP_wcmnrrs: {
            exMOpType = kEwCmn;
            break;
        }
        case MOP_xcmprr:
        case MOP_xwcmprre:
        case MOP_xcmprrs: {
            exMOpType = kExCmp;
            break;
        }
        case MOP_wcmprr:
        case MOP_wwcmprre:
        case MOP_wcmprrs: {
            exMOpType = kEwCmp;
            break;
        }
        default: {
            exMOpType = kExUndef;
        }
    }
}

void ExtendShiftPattern::SetLsMOpType(const Insn &use)
{
    MOperator op = use.GetMachineOpcode();
    switch (op) {
        case MOP_xaddrrr:
        case MOP_xaddrrrs: {
            lsMOpType = kLxAdd;
            break;
        }
        case MOP_waddrrr:
        case MOP_waddrrrs: {
            lsMOpType = kLwAdd;
            break;
        }
        case MOP_xsubrrr:
        case MOP_xsubrrrs: {
            lsMOpType = kLxSub;
            break;
        }
        case MOP_wsubrrr:
        case MOP_wsubrrrs: {
            lsMOpType = kLwSub;
            break;
        }
        case MOP_xcmnrr:
        case MOP_xcmnrrs: {
            lsMOpType = kLxCmn;
            break;
        }
        case MOP_wcmnrr:
        case MOP_wcmnrrs: {
            lsMOpType = kLwCmn;
            break;
        }
        case MOP_xcmprr:
        case MOP_xcmprrs: {
            lsMOpType = kLxCmp;
            break;
        }
        case MOP_wcmprr:
        case MOP_wcmprrs: {
            lsMOpType = kLwCmp;
            break;
        }
        case MOP_xeorrrr:
        case MOP_xeorrrrs: {
            lsMOpType = kLxEor;
            break;
        }
        case MOP_weorrrr:
        case MOP_weorrrrs: {
            lsMOpType = kLwEor;
            break;
        }
        case MOP_xinegrr:
        case MOP_xinegrrs: {
            lsMOpType = kLxNeg;
            replaceIdx = kInsnSecondOpnd;
            break;
        }
        case MOP_winegrr:
        case MOP_winegrrs: {
            lsMOpType = kLwNeg;
            replaceIdx = kInsnSecondOpnd;
            break;
        }
        case MOP_xiorrrr:
        case MOP_xiorrrrs: {
            lsMOpType = kLxIor;
            break;
        }
        case MOP_wiorrrr:
        case MOP_wiorrrrs: {
            lsMOpType = kLwIor;
            break;
        }
        default: {
            lsMOpType = kLsUndef;
        }
    }
}

void ExtendShiftPattern::SelectExtendOrShift(const Insn &def)
{
    MOperator op = def.GetMachineOpcode();
    switch (op) {
        case MOP_xsxtb32:
        case MOP_xsxtb64:
            extendOp = ExtendShiftOperand::kSXTB;
            break;
        case MOP_xsxth32:
        case MOP_xsxth64:
            extendOp = ExtendShiftOperand::kSXTH;
            break;
        case MOP_xsxtw64:
            extendOp = ExtendShiftOperand::kSXTW;
            break;
        case MOP_xuxtb32:
            extendOp = ExtendShiftOperand::kUXTB;
            break;
        case MOP_xuxth32:
            extendOp = ExtendShiftOperand::kUXTH;
            break;
        case MOP_xuxtw64:
            extendOp = ExtendShiftOperand::kUXTW;
            break;
        case MOP_wlslrri5:
        case MOP_xlslrri6:
            shiftOp = BitShiftOperand::kLSL;
            break;
        case MOP_xlsrrri6:
        case MOP_wlsrrri5:
            shiftOp = BitShiftOperand::kLSR;
            break;
        case MOP_xasrrri6:
        case MOP_wasrrri5:
            shiftOp = BitShiftOperand::kASR;
            break;
        default: {
            extendOp = ExtendShiftOperand::kUndef;
            shiftOp = BitShiftOperand::kUndef;
        }
    }
}

/* Optimize ExtendShiftPattern:
 * ==========================================================
 *           nosuffix  LSL   LSR   ASR      extrn   (def)
 * nosuffix |   F    | LSL | LSR | ASR |    extrn  |
 * LSL      |   F    | LSL |  F  |  F  |    extrn  |
 * LSR      |   F    |  F  | LSR |  F  |     F     |
 * ASR      |   F    |  F  |  F  | ASR |     F     |
 * exten    |   F    |  F  |  F  |  F  |exten(self)|
 * (use)
 * ===========================================================
 */
constexpr uint32 kExtenAddShiftNum = 5;
ExtendShiftPattern::SuffixType optTable[kExtenAddShiftNum][kExtenAddShiftNum] = {
    {ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kLSL, ExtendShiftPattern::kLSR, ExtendShiftPattern::kASR,
     ExtendShiftPattern::kExten},
    {ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kLSL, ExtendShiftPattern::kNoSuffix,
     ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kExten},
    {ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kLSR,
     ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kNoSuffix},
    {ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kNoSuffix,
     ExtendShiftPattern::kASR, ExtendShiftPattern::kNoSuffix},
    {ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kNoSuffix,
     ExtendShiftPattern::kNoSuffix, ExtendShiftPattern::kExten}};

/* Check whether ExtendShiftPattern optimization can be performed. */
ExtendShiftPattern::SuffixType ExtendShiftPattern::CheckOpType(const Operand &lastOpnd) const
{
    /* Assign values to useType and defType. */
    uint32 useType = ExtendShiftPattern::kNoSuffix;
    uint32 defType = shiftOp;
    if (extendOp != ExtendShiftOperand::kUndef) {
        defType = ExtendShiftPattern::kExten;
    }
    if (lastOpnd.IsOpdShift()) {
        BitShiftOperand lastShiftOpnd = static_cast<const BitShiftOperand &>(lastOpnd);
        useType = lastShiftOpnd.GetShiftOp();
    } else if (lastOpnd.IsOpdExtend()) {
        ExtendShiftOperand lastExtendOpnd = static_cast<const ExtendShiftOperand &>(lastOpnd);
        useType = ExtendShiftPattern::kExten;
        /* two insn is exten and exten ,value is exten(oneself) */
        if (useType == defType && extendOp != lastExtendOpnd.GetExtendOp()) {
            return ExtendShiftPattern::kNoSuffix;
        }
    }
    return optTable[useType][defType];
}

constexpr uint32 kExMopTypeSize = 9;
constexpr uint32 kLsMopTypeSize = 15;

MOperator exMopTable[kExMopTypeSize] = {MOP_undef,    MOP_xxwaddrrre, MOP_wwwaddrrre, MOP_xxwsubrrre, MOP_wwwsubrrre,
                                        MOP_xwcmnrre, MOP_wwcmnrre,   MOP_xwcmprre,   MOP_wwcmprre};
MOperator lsMopTable[kLsMopTypeSize] = {MOP_undef,    MOP_xaddrrrs, MOP_waddrrrs, MOP_xsubrrrs, MOP_wsubrrrs,
                                        MOP_xcmnrrs,  MOP_wcmnrrs,  MOP_xcmprrs,  MOP_wcmprrs,  MOP_xeorrrrs,
                                        MOP_weorrrrs, MOP_xinegrrs, MOP_winegrrs, MOP_xiorrrrs, MOP_wiorrrrs};
/* new Insn extenType:
 * =====================
 * (useMop)   (defMop) (newmop)
 * | nosuffix |  all  | all|
 * | exten    |  ex   | ex |
 * |  ls      |  ex   | ls |
 * |  asr     |  !asr | F  |
 * |  !asr    |  asr  | F  |
 * (useMop)   (defMop)
 * =====================
 */
void ExtendShiftPattern::ReplaceUseInsn(Insn &use, const Insn &def, uint32 amount)
{
    AArch64CGFunc &a64CGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    uint32 lastIdx = use.GetOperandSize() - k1BitSize;
    Operand &lastOpnd = use.GetOperand(lastIdx);
    ExtendShiftPattern::SuffixType optType = CheckOpType(lastOpnd);
    Operand *shiftOpnd = nullptr;
    if (optType == ExtendShiftPattern::kNoSuffix) {
        return;
    } else if (optType == ExtendShiftPattern::kExten) {
        replaceOp = exMopTable[exMOpType];
        if (amount > k4BitSize) {
            return;
        }
        shiftOpnd = &a64CGFunc.CreateExtendShiftOperand(extendOp, amount, static_cast<int32>(k64BitSize));
    } else {
        replaceOp = lsMopTable[lsMOpType];
        if (amount >= k32BitSize) {
            return;
        }
        shiftOpnd = &a64CGFunc.CreateBitShiftOperand(shiftOp, amount, static_cast<int32>(k64BitSize));
    }
    if (replaceOp == MOP_undef) {
        return;
    }

    Insn *replaceUseInsn = nullptr;
    Operand &firstOpnd = use.GetOperand(kInsnFirstOpnd);
    Operand *secondOpnd = &use.GetOperand(kInsnSecondOpnd);
    if (replaceIdx == kInsnSecondOpnd) { /* replace neg insn */
        secondOpnd = &def.GetOperand(kInsnSecondOpnd);
        replaceUseInsn = &cgFunc.GetInsnBuilder()->BuildInsn(replaceOp, firstOpnd, *secondOpnd, *shiftOpnd);
    } else {
        Operand &thirdOpnd = def.GetOperand(kInsnSecondOpnd);
        replaceUseInsn = &cgFunc.GetInsnBuilder()->BuildInsn(replaceOp, firstOpnd, *secondOpnd, thirdOpnd, *shiftOpnd);
    }
    use.GetBB()->ReplaceInsn(use, *replaceUseInsn);
    if (PROP_DUMP) {
        LogInfo::MapleLogger() << ">>>>>>> In ExtendShiftPattern : <<<<<<<\n";
        LogInfo::MapleLogger() << "=======ReplaceInsn :\n";
        use.Dump();
        LogInfo::MapleLogger() << "=======NewInsn :\n";
        replaceUseInsn->Dump();
    }
    /* update ssa info */
    optSsaInfo->ReplaceInsn(use, *replaceUseInsn);
    newInsn = replaceUseInsn;
    optSuccess = true;
}

/*
 * pattern1:
 * UXTB/UXTW X0, W1              <---- def x0
 * ....                          <---- (X0 not used)
 * AND/SUB/EOR X0, X1, X0        <---- use x0
 * ======>
 * AND/SUB/EOR X0, X1, W1 UXTB/UXTW
 *
 * pattern2:
 * LSL/LSR X0, X1, #8
 * ....(X0 not used)
 * AND/SUB/EOR X0, X1, X0
 * ======>
 * AND/SUB/EOR X0, X1, X1 LSL/LSR #8
 */
void ExtendShiftPattern::Optimize(Insn &insn)
{
    uint32 amount = 0;
    uint32 offset = 0;
    uint32 lastIdx = insn.GetOperandSize() - k1BitSize;
    Operand &lastOpnd = insn.GetOperand(lastIdx);
    if (lastOpnd.IsOpdShift()) {
        auto &lastShiftOpnd = static_cast<BitShiftOperand &>(lastOpnd);
        amount = lastShiftOpnd.GetShiftAmount();
    } else if (lastOpnd.IsOpdExtend()) {
        auto &lastExtendOpnd = static_cast<ExtendShiftOperand &>(lastOpnd);
        amount = lastExtendOpnd.GetShiftAmount();
    }
    if (shiftOp != BitShiftOperand::kUndef) {
        auto &immOpnd = static_cast<ImmOperand &>(defInsn->GetOperand(kInsnThirdOpnd));
        offset = static_cast<uint32>(immOpnd.GetValue());
    }
    amount += offset;

    ReplaceUseInsn(insn, *defInsn, amount);
}

void ExtendShiftPattern::DoExtendShiftOpt(Insn &insn)
{
    if (!CheckAllOpndCondition(insn)) {
        return;
    }
    Optimize(*curInsn);
    if (optSuccess) {
        DoExtendShiftOpt(*newInsn);
    }
}

void ExtendShiftPattern::SwapOpnd(Insn &insn)
{
    Insn *swapInsn =
        &cgFunc.GetInsnBuilder()->BuildInsn(insn.GetMachineOpcode(), insn.GetOperand(kInsnFirstOpnd),
                                            insn.GetOperand(kInsnThirdOpnd), insn.GetOperand(kInsnSecondOpnd));
    insn.GetBB()->ReplaceInsn(insn, *swapInsn);
    optSsaInfo->ReplaceInsn(insn, *swapInsn);
    curInsn = swapInsn;
    replaceIdx = kInsnThirdOpnd;
}

bool ExtendShiftPattern::CheckAllOpndCondition(Insn &insn)
{
    Init();
    SetLsMOpType(insn);
    SetExMOpType(insn);
    curInsn = &insn;
    if (IsSwapInsn(insn)) {
        if (CheckCondition(insn)) {
            return true;
        }
        Init();
        SetLsMOpType(insn);
        SetExMOpType(insn);
        replaceIdx = kInsnSecondOpnd;
        if (CheckCondition(insn)) {
            SwapOpnd(insn);
            return true;
        }
    } else {
        return CheckCondition(insn);
    }
    return false;
}

/* check and set:
 * exMOpType, lsMOpType, extendOp, shiftOp, defInsn
 */
bool ExtendShiftPattern::CheckCondition(Insn &insn)
{
    if ((exMOpType == kExUndef) && (lsMOpType == kLsUndef)) {
        return false;
    }
    auto &regOperand = static_cast<RegOperand &>(insn.GetOperand(replaceIdx));
    regno_t regNo = regOperand.GetRegisterNumber();
    VRegVersion *useVersion = optSsaInfo->FindSSAVersion(regNo);
    defInsn = FindDefInsn(useVersion);
    if (!defInsn || (useVersion->GetAllUseInsns().size() > 1)) {
        return false;
    }
    SelectExtendOrShift(*defInsn);
    /* defInsn must be shift or extend */
    if ((extendOp == ExtendShiftOperand::kUndef) && (shiftOp == BitShiftOperand::kUndef)) {
        return false;
    }
    Operand &defSrcOpnd = defInsn->GetOperand(kInsnSecondOpnd);
    CHECK_FATAL(defSrcOpnd.IsRegister(), "defSrcOpnd must be register!");
    auto &regDefSrc = static_cast<RegOperand &>(defSrcOpnd);
    if (regDefSrc.IsPhysicalRegister()) {
        return false;
    }
    /*
     * has Implict cvt
     *
     * avoid cases as following:
     *   lsr  x2, x2, #8
     *   ubfx w2, x2, #0, #32                lsr  x2, x2, #8
     *   eor  w0, w0, w2           ===>      eor  w0, w0, x2     ==\=>  eor w0, w0, w2, LSR #8
     *
     * the truncation causes the wrong value by shift right
     * shift left does not matter
     */
    if (useVersion->HasImplicitCvt() && shiftOp != BitShiftOperand::kUndef) {
        return false;
    }
    if ((shiftOp == BitShiftOperand::kLSR || shiftOp == BitShiftOperand::kASR) &&
        (defSrcOpnd.GetSize() > regOperand.GetSize())) {
        return false;
    }
    regno_t defSrcRegNo = regDefSrc.GetRegisterNumber();
    /* check regDefSrc */
    VRegVersion *replaceUseV = optSsaInfo->FindSSAVersion(defSrcRegNo);
    CHECK_FATAL(replaceUseV != nullptr, "useVRegVersion must not be null based on ssa");
    if (replaceUseV->GetAllUseInsns().size() > 1) {
        return false;
    }
    return true;
}

void ExtendShiftPattern::Init()
{
    replaceOp = MOP_undef;
    extendOp = ExtendShiftOperand::kUndef;
    shiftOp = BitShiftOperand::kUndef;
    defInsn = nullptr;
    newInsn = nullptr;
    replaceIdx = kInsnThirdOpnd;
    optSuccess = false;
    exMOpType = kExUndef;
    lsMOpType = kLsUndef;
}

void ExtendShiftPattern::Run()
{
    if (!cgFunc.GetMirModule().IsCModule()) {
        return;
    }
    FOR_ALL_BB_REV(bb, &cgFunc)
    {
        FOR_BB_INSNS_REV(insn, bb)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            DoExtendShiftOpt(*insn);
        }
    }
}

void ExtendMovPattern::Run()
{
    if (!cgFunc.GetMirModule().IsCModule()) {
        return;
    }
    FOR_ALL_BB(bb, &cgFunc)
    {
        FOR_BB_INSNS(insn, bb)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            if (!CheckCondition(*insn)) {
                continue;
            }
            Optimize(*insn);
        }
    }
}

bool ExtendMovPattern::CheckSrcReg(regno_t srcRegNo, uint32 validNum)
{
    InsnSet srcDefSet;
    VRegVersion *useVersion = optSsaInfo->FindSSAVersion(srcRegNo);
    CHECK_FATAL(useVersion != nullptr, "useVRegVersion must not be null based on ssa");
    DUInsnInfo *defInfo = useVersion->GetDefInsnInfo();
    if (defInfo == nullptr) {
        return false;
    }
    Insn *insn = defInfo->GetInsn();
    srcDefSet.insert(insn);
    /* reserve insn set for non ssa version. */
    for (auto defInsn : srcDefSet) {
        CHECK_FATAL((defInsn != nullptr), "defInsn is null!");
        MOperator mOp = defInsn->GetMachineOpcode();
        switch (mOp) {
            case MOP_wiorrri12:
            case MOP_weorrri12: {
                /* check immVal if mop is OR */
                ImmOperand &imm = static_cast<ImmOperand &>(defInsn->GetOperand(kInsnThirdOpnd));
                uint32 bitNum = static_cast<uint32>(imm.GetValue());
                if ((bitNum >> validNum) != 0) {
                    return false;
                }
                break;
            }
            case MOP_wandrri12: {
                /* check defSrcReg */
                RegOperand &defSrcRegOpnd = static_cast<RegOperand &>(defInsn->GetOperand(kInsnSecondOpnd));
                regno_t defSrcRegNo = defSrcRegOpnd.GetRegisterNumber();
                if (!CheckSrcReg(defSrcRegNo, validNum)) {
                    return false;
                }
                break;
            }
            case MOP_wandrrr: {
                /* check defSrcReg */
                RegOperand &defSrcRegOpnd1 = static_cast<RegOperand &>(defInsn->GetOperand(kInsnSecondOpnd));
                RegOperand &defSrcRegOpnd2 = static_cast<RegOperand &>(defInsn->GetOperand(kInsnThirdOpnd));
                regno_t defSrcRegNo1 = defSrcRegOpnd1.GetRegisterNumber();
                regno_t defSrcRegNo2 = defSrcRegOpnd2.GetRegisterNumber();
                if (!CheckSrcReg(defSrcRegNo1, validNum) && !CheckSrcReg(defSrcRegNo2, validNum)) {
                    return false;
                }
                break;
            }
            case MOP_wiorrrr:
            case MOP_weorrrr: {
                /* check defSrcReg */
                RegOperand &defSrcRegOpnd1 = static_cast<RegOperand &>(defInsn->GetOperand(kInsnSecondOpnd));
                RegOperand &defSrcRegOpnd2 = static_cast<RegOperand &>(defInsn->GetOperand(kInsnThirdOpnd));
                regno_t defSrcRegNo1 = defSrcRegOpnd1.GetRegisterNumber();
                regno_t defSrcRegNo2 = defSrcRegOpnd2.GetRegisterNumber();
                if (!CheckSrcReg(defSrcRegNo1, validNum) || !CheckSrcReg(defSrcRegNo2, validNum)) {
                    return false;
                }
                break;
            }
            case MOP_wldrb: {
                if (validNum != k8BitSize) {
                    return false;
                }
                break;
            }
            case MOP_wldrh: {
                if (validNum != k16BitSize) {
                    return false;
                }
                break;
            }
            default:
                return false;
        }
    }
    return true;
}

bool ExtendMovPattern::BitNotAffected(const Insn &insn, uint32 validNum)
{
    RegOperand &firstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    RegOperand &secondOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    regno_t desRegNo = firstOpnd.GetRegisterNumber();
    regno_t srcRegNo = secondOpnd.GetRegisterNumber();
    VRegVersion *useVersion = optSsaInfo->FindSSAVersion(desRegNo);
    CHECK_FATAL(useVersion != nullptr, "useVRegVersion must not be null based on ssa");
    DUInsnInfo *defInfo = useVersion->GetDefInsnInfo();
    if (defInfo == nullptr) {
        return false;
    }
    if (!CheckSrcReg(srcRegNo, validNum)) {
        return false;
    }
    replaceMop = MOP_wmovrr;
    return true;
}

bool ExtendMovPattern::CheckCondition(Insn &insn)
{
    MOperator mOp = insn.GetMachineOpcode();
    switch (mOp) {
        case MOP_xuxtb32:
            return BitNotAffected(insn, k8BitSize);
        case MOP_xuxth32:
            return BitNotAffected(insn, k16BitSize);
        default:
            return false;
    }
}

/* No initialization required */
void ExtendMovPattern::Init()
{
    replaceMop = MOP_undef;
}

void ExtendMovPattern::Optimize(Insn &insn)
{
    insn.SetMOP(AArch64CG::kMd[replaceMop]);
}

void CopyRegProp::Run()
{
    FOR_ALL_BB(bb, &cgFunc)
    {
        FOR_BB_INSNS(insn, bb)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            Init();
            if (!CheckCondition(*insn)) {
                continue;
            }
            Optimize(*insn);
        }
    }
}

bool CopyRegProp::IsValidCopyProp(const RegOperand &dstReg, const RegOperand &srcReg) const
{
    DEBUG_ASSERT(destVersion != nullptr, "find destVersion failed");
    DEBUG_ASSERT(srcVersion != nullptr, "find srcVersion failed");
    LiveInterval *dstll = nullptr;
    LiveInterval *srcll = nullptr;
    if (destVersion->GetOriginalRegNO() == srcVersion->GetOriginalRegNO()) {
        return true;
    }
    regno_t dstRegNO = dstReg.GetRegisterNumber();
    regno_t srcRegNO = srcReg.GetRegisterNumber();
    for (auto useDUInfoIt : destVersion->GetAllUseInsns()) {
        if (useDUInfoIt.second == nullptr) {
            continue;
        }
        Insn *useInsn = (useDUInfoIt.second)->GetInsn();
        if (useInsn == nullptr) {
            continue;
        }

        dstll = regll->GetLiveInterval(dstRegNO);
        srcll = regll->GetLiveInterval(srcRegNO);
        static_cast<AArch64LiveIntervalAnalysis *>(regll)->CheckInterference(*dstll, *srcll);
        BB *useBB = useInsn->GetBB();
        if (dstll->IsConflictWith(srcRegNO) &&
            /* support override value when the version is not transphi */
            (((useBB->IsInPhiDef(srcRegNO) || useBB->IsInPhiList(srcRegNO)) && useBB->HasCriticalEdge()) ||
             useBB->IsInPhiList(dstRegNO))) {
            return false;
        }
    }
    if (dstll && srcll) {
        regll->CoalesceLiveIntervals(*dstll, *srcll);
    }
    return true;
}

bool CopyRegProp::CheckCondition(Insn &insn)
{
    if (Globals::GetInstance()->GetTarget()->IsEffectiveCopy(insn)) {
        MOperator mOp = insn.GetMachineOpcode();
        if (mOp == MOP_xmovrr || mOp == MOP_wmovrr || mOp == MOP_xvmovs || mOp == MOP_xvmovd) {
            Operand &destOpnd = insn.GetOperand(kInsnFirstOpnd);
            Operand &srcOpnd = insn.GetOperand(kInsnSecondOpnd);
            DEBUG_ASSERT(destOpnd.IsRegister() && srcOpnd.IsRegister(), "must be");
            auto &destReg = static_cast<RegOperand &>(destOpnd);
            auto &srcReg = static_cast<RegOperand &>(srcOpnd);
            if (srcReg.GetRegisterNumber() == RZR) {
                insn.SetMOP(AArch64CG::kMd[mOp == MOP_xmovrr ? MOP_xmovri64 : MOP_wmovri32]);
                insn.SetOperand(kInsnSecondOpnd, cgFunc.CreateImmOperand(PTY_u64, 0));
            }
            if (destReg.IsSSAForm() && srcReg.IsSSAForm()) {
                /* case for ExplicitExtendProp  */
                if (destReg.GetSize() != srcReg.GetSize()) {
                    VaildateImplicitCvt(destReg, srcReg, insn);
                    return false;
                }
                if (destReg.GetValidBitsNum() >= srcReg.GetValidBitsNum()) {
                    destReg.SetValidBitsNum(srcReg.GetValidBitsNum());
                } else {
                    MapleVector<uint32> &propInsns = optSsaInfo->GetSafePropInsns();
                    if (std::find(propInsns.begin(), propInsns.end(), insn.GetId()) == propInsns.end()) {
                        CHECK_FATAL(false, "do not support explicit extract bit in mov");
                        return false;
                    }
                }
                destVersion = optSsaInfo->FindSSAVersion(destReg.GetRegisterNumber());
                DEBUG_ASSERT(destVersion != nullptr, "find Version failed");
                srcVersion = optSsaInfo->FindSSAVersion(srcReg.GetRegisterNumber());
                DEBUG_ASSERT(srcVersion != nullptr, "find Version failed");
                if (!IsValidCopyProp(destReg, srcReg)) {
                    return false;
                }
                return true;
            } else {
                /* should be eliminated by ssa peep */
            }
        }
    }
    return false;
}

void CopyRegProp::Optimize(Insn &insn)
{
    optSsaInfo->ReplaceAllUse(destVersion, srcVersion);
    if (cgFunc.IsExtendReg(destVersion->GetSSAvRegOpnd()->GetRegisterNumber())) {
        cgFunc.InsertExtendSet(srcVersion->GetSSAvRegOpnd()->GetRegisterNumber());
    }
}

void CopyRegProp::VaildateImplicitCvt(RegOperand &destReg, const RegOperand &srcReg, Insn &movInsn)
{
    DEBUG_ASSERT(movInsn.GetMachineOpcode() == MOP_xmovrr || movInsn.GetMachineOpcode() == MOP_wmovrr,
                 "NIY explicit CVT");
    if (destReg.GetSize() == k64BitSize && srcReg.GetSize() == k32BitSize) {
        movInsn.SetMOP(AArch64CG::kMd[MOP_xuxtw64]);
    } else if (destReg.GetSize() == k32BitSize && srcReg.GetSize() == k64BitSize) {
        movInsn.SetMOP(AArch64CG::kMd[MOP_xubfxrri6i6]);
        movInsn.AddOperand(cgFunc.CreateImmOperand(PTY_i64, 0));
        movInsn.AddOperand(cgFunc.CreateImmOperand(PTY_i64, k32BitSize));
    } else {
        CHECK_FATAL(false, " unknown explicit integer cvt,  need implement in ssa prop ");
    }
    destReg.SetValidBitsNum(k32BitSize);
}

void RedundantPhiProp::Run()
{
    FOR_ALL_BB(bb, &cgFunc)
    {
        for (auto phiIt : bb->GetPhiInsns()) {
            Init();
            if (!CheckCondition(*phiIt.second)) {
                continue;
            }
            Optimize(*phiIt.second);
        }
    }
}

void RedundantPhiProp::Optimize(Insn &insn)
{
    optSsaInfo->ReplaceAllUse(destVersion, srcVersion);
}

bool RedundantPhiProp::CheckCondition(Insn &insn)
{
    DEBUG_ASSERT(insn.IsPhi(), "must be phi insn here");
    auto &phiOpnd = static_cast<PhiOperand &>(insn.GetOperand(kInsnSecondOpnd));
    if (phiOpnd.IsRedundancy()) {
        auto &phiDestReg = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
        destVersion = optSsaInfo->FindSSAVersion(phiDestReg.GetRegisterNumber());
        DEBUG_ASSERT(destVersion != nullptr, "find Version failed");
        uint32 srcRegNO = phiOpnd.GetOperands().begin()->second->GetRegisterNumber();
        srcVersion = optSsaInfo->FindSSAVersion(srcRegNO);
        DEBUG_ASSERT(srcVersion != nullptr, "find Version failed");
        return true;
    }
    return false;
}

bool ValidBitNumberProp::CheckCondition(Insn &insn)
{
    /* extend to all shift pattern in future */
    RegOperand *destOpnd = nullptr;
    RegOperand *srcOpnd = nullptr;
    if (insn.GetMachineOpcode() == MOP_xuxtw64) {
        destOpnd = &static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
        srcOpnd = &static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    }
    if (insn.GetMachineOpcode() == MOP_xubfxrri6i6) {
        destOpnd = &static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
        srcOpnd = &static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
        auto &lsb = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
        auto &width = static_cast<ImmOperand &>(insn.GetOperand(kInsnFourthOpnd));
        if ((lsb.GetValue() != 0) || (width.GetValue() != k32BitSize)) {
            return false;
        }
    }
    if (destOpnd != nullptr && destOpnd->IsSSAForm() && srcOpnd != nullptr && srcOpnd->IsSSAForm()) {
        destVersion = optSsaInfo->FindSSAVersion(destOpnd->GetRegisterNumber());
        DEBUG_ASSERT(destVersion != nullptr, "find Version failed");
        srcVersion = optSsaInfo->FindSSAVersion(srcOpnd->GetRegisterNumber());
        DEBUG_ASSERT(srcVersion != nullptr, "find Version failed");
        if (destVersion->HasImplicitCvt()) {
            return false;
        }
        for (auto destUseIt : destVersion->GetAllUseInsns()) {
            Insn *useInsn = destUseIt.second->GetInsn();
            if (useInsn->GetMachineOpcode() == MOP_xuxtw64) {
                return false;
            }
            /* if srcOpnd upper 32 bits are valid, it can not prop to mop_x */
            if (srcOpnd->GetSize() == k64BitSize && destOpnd->GetSize() == k64BitSize) {
                const auto *useMD = useInsn->GetDesc();
                for (auto opndUseIt : destUseIt.second->GetOperands()) {
                    const OpndDesc *useProp = useMD->opndMD[opndUseIt.first];
                    if (useProp->GetSize() == k64BitSize) {
                        return false;
                    }
                }
            }
        }
        srcVersion->SetImplicitCvt();
        return true;
    }
    return false;
}

void ValidBitNumberProp::Optimize(Insn &insn)
{
    optSsaInfo->ReplaceAllUse(destVersion, srcVersion);
    cgFunc.InsertExtendSet(srcVersion->GetSSAvRegOpnd()->GetRegisterNumber());
}

void ValidBitNumberProp::Run()
{
    FOR_ALL_BB(bb, &cgFunc)
    {
        FOR_BB_INSNS(insn, bb)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            Init();
            if (!CheckCondition(*insn)) {
                continue;
            }
            Optimize(*insn);
        }
    }
}

void FpSpConstProp::Run()
{
    FOR_ALL_BB(bb, &cgFunc)
    {
        FOR_BB_INSNS(insn, bb)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            Init();
            if (!CheckCondition(*insn)) {
                continue;
            }
            Optimize(*insn);
        }
    }
}

bool FpSpConstProp::CheckCondition(Insn &insn)
{
    std::set<uint32> defRegs = insn.GetDefRegs();
    auto &a64CGFunc = static_cast<AArch64CGFunc &>(cgFunc);
    if (defRegs.size() <= 1) {
        if (insn.ScanReg(RSP)) {
            fpSpBase = &a64CGFunc.GetOrCreatePhysicalRegisterOperand(RSP, k64BitSize, kRegTyInt);
            /* not safe due to varied sp in alloca */
            if (cgFunc.HasVLAOrAlloca()) {
                return false;
            }
        }
        if (insn.ScanReg(RFP)) {
            DEBUG_ASSERT(fpSpBase == nullptr, " unexpect for both sp fp using ");
            fpSpBase = &a64CGFunc.GetOrCreatePhysicalRegisterOperand(RFP, k64BitSize, kRegTyInt);
        }
        if (fpSpBase == nullptr) {
            return false;
        }
        if (insn.GetMachineOpcode() == MOP_xaddrri12) {
            aT = kAArch64Add;
            if (GetValidSSAInfo(insn.GetOperand(kInsnFirstOpnd))) {
                shiftOpnd = &static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
                return true;
            }
        } else if (insn.GetMachineOpcode() == MOP_xsubrri12) {
            aT = kAArch64Sub;
            if (GetValidSSAInfo(insn.GetOperand(kInsnFirstOpnd))) {
                shiftOpnd = &static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
                return true;
            }
        }
    }
    return false;
}

bool FpSpConstProp::GetValidSSAInfo(Operand &opnd)
{
    if (opnd.IsRegister()) {
        auto &regOpnd = static_cast<RegOperand &>(opnd);
        if (regOpnd.IsSSAForm()) {
            replaced = optSsaInfo->FindSSAVersion(regOpnd.GetRegisterNumber());
            DEBUG_ASSERT(replaced != nullptr, "find ssa version failed in FpSpConstProp");
            return true;
        }
    }
    return false;
}

int64 FpSpConstProp::ArithmeticFold(int64 valInUse, ArithmeticType useAT) const
{
    int64 valInDef = shiftOpnd->GetValue();
    int64 returnVal = 0;
    CHECK_FATAL(aT == kAArch64Add || aT == kAArch64Sub, "unsupport sp/fp arthimetic  in aarch64");
    if (useAT == aT) {
        returnVal = valInUse + valInDef;
    } else {
        returnVal = valInUse - valInDef;
    }
    return returnVal;
}

void FpSpConstProp::PropInMem(DUInsnInfo &useDUInfo, Insn &useInsn)
{
    MOperator useMop = useInsn.GetMachineOpcode();
    if (useInsn.IsAtomic()) {
        return;
    }
    if (useInsn.IsStore() || useInsn.IsLoad()) {
        if (useDUInfo.GetOperands().size() == 1) {
            auto useOpndIt = useDUInfo.GetOperands().begin();
            if (useOpndIt->first == kInsnSecondOpnd || useOpndIt->first == kInsnThirdOpnd) {
                DEBUG_ASSERT(useOpndIt->second == 1, "multiple use in memory opnd");
                auto *a64memOpnd = static_cast<MemOperand *>(useInsn.GetMemOpnd());
                if (a64memOpnd->IsIntactIndexed() && a64memOpnd->GetAddrMode() == MemOperand::kAddrModeBOi) {
                    auto *ofstOpnd = static_cast<OfstOperand *>(a64memOpnd->GetOffsetImmediate());
                    CHECK_FATAL(ofstOpnd != nullptr, "oldOffsetOpnd is null");
                    int64 newVal = ArithmeticFold(ofstOpnd->GetValue(), kAArch64Add);
                    auto *newOfstImm =
                        &static_cast<AArch64CGFunc &>(cgFunc).CreateOfstOpnd(static_cast<uint64>(newVal), k64BitSize);
                    if (ofstOpnd->GetVary() == kUnAdjustVary || shiftOpnd->GetVary() == kUnAdjustVary) {
                        newOfstImm->SetVary(kUnAdjustVary);
                    }
                    auto *newMem = static_cast<AArch64CGFunc &>(cgFunc).CreateMemOperand(
                        MemOperand::kAddrModeBOi, a64memOpnd->GetSize(), *fpSpBase, nullptr, newOfstImm, nullptr);
                    if (static_cast<AArch64CGFunc &>(cgFunc).IsOperandImmValid(useMop, newMem, useOpndIt->first)) {
                        useInsn.SetMemOpnd(newMem);
                        useDUInfo.DecreaseDU(useOpndIt->first);
                        replaced->CheckDeadUse(useInsn);
                    }
                }
            }
        } else {
            /*
             * case : store stack location on stack
             * add x1, sp, #8
             *  ...
             * store x1 [x1, #16]
             * not prop , not benefit to live range yet
             */
            return;
        }
    }
}

void FpSpConstProp::PropInArith(DUInsnInfo &useDUInfo, Insn &useInsn, ArithmeticType curAT)
{
    if (useDUInfo.GetOperands().size() == 1) {
        auto &a64cgFunc = static_cast<AArch64CGFunc &>(cgFunc);
        MOperator useMop = useInsn.GetMachineOpcode();
        DEBUG_ASSERT(useDUInfo.GetOperands().begin()->first == kInsnSecondOpnd, "NIY");
        DEBUG_ASSERT(useDUInfo.GetOperands().begin()->second == 1, "multiple use in add/sub");
        auto &curVal = static_cast<ImmOperand &>(useInsn.GetOperand(kInsnThirdOpnd));
        ImmOperand &newVal =
            a64cgFunc.CreateImmOperand(ArithmeticFold(curVal.GetValue(), curAT), curVal.GetSize(), false);
        if (newVal.GetValue() < 0) {
            newVal.Negate();
            useMop = A64ConstProp::GetReversalMOP(useMop);
        }
        if (curVal.GetVary() == kUnAdjustVary || shiftOpnd->GetVary() == kUnAdjustVary) {
            newVal.SetVary(kUnAdjustVary);
        }
        if (static_cast<AArch64CGFunc &>(cgFunc).IsOperandImmValid(useMop, &newVal, kInsnThirdOpnd)) {
            Insn &newInsn =
                cgFunc.GetInsnBuilder()->BuildInsn(useMop, useInsn.GetOperand(kInsnFirstOpnd), *fpSpBase, newVal);
            useInsn.GetBB()->ReplaceInsn(useInsn, newInsn);
            optSsaInfo->ReplaceInsn(useInsn, newInsn);
        }
    } else {
        CHECK_FATAL(false, "NYI");
    }
}

void FpSpConstProp::PropInCopy(DUInsnInfo &useDUInfo, Insn &useInsn, MOperator oriMop)
{
    if (useDUInfo.GetOperands().size() == 1) {
        DEBUG_ASSERT(useDUInfo.GetOperands().begin()->first == kInsnSecondOpnd, "NIY");
        DEBUG_ASSERT(useDUInfo.GetOperands().begin()->second == 1, "multiple use in add/sub");
        auto &newVal = *static_cast<ImmOperand *>(shiftOpnd->Clone(*cgFunc.GetMemoryPool()));
        Insn &newInsn =
            cgFunc.GetInsnBuilder()->BuildInsn(oriMop, useInsn.GetOperand(kInsnFirstOpnd), *fpSpBase, newVal);
        useInsn.GetBB()->ReplaceInsn(useInsn, newInsn);
        optSsaInfo->ReplaceInsn(useInsn, newInsn);
    } else {
        CHECK_FATAL(false, "NYI");
    }
}

void FpSpConstProp::Optimize(Insn &insn)
{
    for (auto &useInsnInfo : replaced->GetAllUseInsns()) {
        Insn *useInsn = useInsnInfo.second->GetInsn();
        MOperator useMop = useInsn->GetMachineOpcode();
        PropInMem(*useInsnInfo.second, *useInsn);
        switch (useMop) {
            case MOP_xmovrr:
            case MOP_wmovrr:
                PropInCopy(*useInsnInfo.second, *useInsn, insn.GetMachineOpcode());
                break;
            case MOP_xaddrri12:
                PropInArith(*useInsnInfo.second, *useInsn, kAArch64Add);
                break;
            case MOP_xsubrri12:
                PropInArith(*useInsnInfo.second, *useInsn, kAArch64Sub);
                break;
            default:
                break;
        }
    }
}

bool A64PregCopyPattern::DFSFindValidDefInsns(Insn *curDefInsn, RegOperand *lastPhiDef,
                                              std::unordered_map<uint32, bool> &visited)
{
    if (curDefInsn == nullptr) {
        return false;
    }
    /*
     * avoid the case as following:
     * R113 and R117 define each other.
     *                               [BB5]     ----------------------------
     *                    phi: R113, (R111<4>, R117<9>)                   |
     *                              /     \                               |
     *                             /       \                              |
     *                        [BB6]  ----  [BB7]                          |
     *            add R116, R113, #4     phi: R117, (R113<5>, R116<6>)    |
     *                                     /   \                          |
     *                                    /     \                         |
     *                                 [BB8]    [BB28]                    |
     *                                   /                                |
     *                                  /                                 |
     *                                 [BB9] ------ [BB5]                 |
     *                             mov R1, R117  --------------------------
     *
     * but the cases as following is right:
     * (1)
     *                              [BB124]
     *                       add R339, R336, #345      --------  is found twice
     *                              /    \
     *                             /      \
     *                            /     [BB125]
     *                            \        /
     *                             \      /
     *                              [BB56]
     *                       phi: R370, (R339<124>, R339<125>)
     *                                |
     *                                |
     *                             [BB61]
     *                           mov R0, R370
     * (2)
     *                                [BB17]
     *                       phi: R242, (R241<14>, R218<53>)    ------- is found twice
     *                              /          \
     *                             /            \
     *                            /           [BB26]     [BB32]
     *                            \               \      /
     *                             \               [BB27]
     *                              \      phi: R273, (R242<26>, R320<32>)
     *                           [BB25]           /
     *                                \        [BB42]
     *                                 \       /
     *                                  [BB43]
     *                        phi: R321, (R242<25>, R273<42>)
     *                                    |
     *                                  [BB47]
     *                               mov R0, R321
     */
    if (visited[curDefInsn->GetId()] && curDefInsn->IsPhi() && lastPhiDef != nullptr) {
        auto &curPhiOpnd = static_cast<PhiOperand &>(curDefInsn->GetOperand(kInsnSecondOpnd));
        for (auto &curPhiListIt : curPhiOpnd.GetOperands()) {
            auto &curUseOpnd = static_cast<RegOperand &>(*curPhiListIt.second);
            if (&curUseOpnd == lastPhiDef) {
                return false;
            }
        }
    }
    if (visited[curDefInsn->GetId()]) {
        return true;
    }
    visited[curDefInsn->GetId()] = true;
    if (!curDefInsn->IsPhi()) {
        CHECK_FATAL(curDefInsn->IsMachineInstruction(), "expect valid insn");
        (void)validDefInsns.emplace_back(curDefInsn);
        return true;
    }
    auto &phiOpnd = static_cast<PhiOperand &>(curDefInsn->GetOperand(kInsnSecondOpnd));
    for (auto &phiListIt : phiOpnd.GetOperands()) {
        auto &useOpnd = static_cast<RegOperand &>(*phiListIt.second);
        VRegVersion *useVersion = optSsaInfo->FindSSAVersion(useOpnd.GetRegisterNumber());
        Insn *defInsn = FindDefInsn(useVersion);
        if (defInsn == nullptr) {
            return false;
        }
        lastPhiDef = &static_cast<RegOperand &>(curDefInsn->GetOperand(kInsnFirstOpnd));
        if (!DFSFindValidDefInsns(defInsn, lastPhiDef, visited)) {
            return false;
        }
    }
    return true;
}

bool A64PregCopyPattern::CheckMultiUsePoints(const Insn *defInsn) const
{
    Operand &dstOpnd = defInsn->GetOperand(kInsnFirstOpnd);
    CHECK_FATAL(dstOpnd.IsRegister(), "dstOpnd must be register");
    VRegVersion *defVersion = optSsaInfo->FindSSAVersion(static_cast<RegOperand &>(dstOpnd).GetRegisterNumber());
    /* use: (phi) or (mov preg) */
    for (auto &useInfoIt : defVersion->GetAllUseInsns()) {
        DUInsnInfo *useInfo = useInfoIt.second;
        CHECK_FATAL(useInfo, "get useDUInfo failed");
        Insn *useInsn = useInfo->GetInsn();
        CHECK_FATAL(useInsn, "get useInsn failed");
        if (!useInsn->IsPhi() && useInsn->GetMachineOpcode() != MOP_wmovrr &&
            useInsn->GetMachineOpcode() != MOP_xmovrr) {
            return false;
        }
        if ((useInsn->GetMachineOpcode() == MOP_wmovrr || useInsn->GetMachineOpcode() == MOP_xmovrr) &&
            !static_cast<RegOperand &>(useInsn->GetOperand(kInsnFirstOpnd)).IsPhysicalRegister()) {
            return false;
        }
    }
    return true;
}

bool A64PregCopyPattern::CheckPhiCaseCondition(Insn &curInsn, Insn &defInsn)
{
    std::unordered_map<uint32, bool> visited;
    RegOperand *lastPhiDef =
        (defInsn.IsPhi() ? &static_cast<RegOperand &>(defInsn.GetOperand(kInsnFirstOpnd)) : nullptr);
    if (!DFSFindValidDefInsns(&defInsn, lastPhiDef, visited)) {
        return false;
    }
    if (!CheckValidDefInsn(validDefInsns[0])) {
        return false;
    }
    MOperator defMop = validDefInsns[0]->GetMachineOpcode();
    uint32 defOpndNum = validDefInsns[0]->GetOperandSize();
    for (size_t i = 1; i < validDefInsns.size(); ++i) {
        if (defMop != validDefInsns[i]->GetMachineOpcode()) {
            return false;
        }
        if (!CheckMultiUsePoints(validDefInsns[i])) {
            return false;
        }
        for (uint32 idx = 0; idx < defOpndNum; ++idx) {
            if (validDefInsns[0]->OpndIsDef(idx) && validDefInsns[i]->OpndIsDef(idx)) {
                continue;
            }
            Operand &opnd1 = validDefInsns[0]->GetOperand(idx);
            Operand &opnd2 = validDefInsns[i]->GetOperand(idx);
            if (!opnd1.Equals(opnd2) && differIdx == -1) {
                differIdx = static_cast<int>(idx);
                if (!validDefInsns[0]->GetOperand(static_cast<uint32>(differIdx)).IsRegister() ||
                    !validDefInsns[i]->GetOperand(static_cast<uint32>(differIdx)).IsRegister()) {
                    return false;
                }
                auto &differOpnd1 =
                    static_cast<RegOperand &>(validDefInsns[0]->GetOperand(static_cast<uint32>(differIdx)));
                auto &differOpnd2 =
                    static_cast<RegOperand &>(validDefInsns[1]->GetOperand(static_cast<uint32>(differIdx)));
                /* avoid cc reg */
                if (!differOpnd1.IsOfIntClass() || !differOpnd2.IsOfIntClass() || differOpnd1.IsPhysicalRegister() ||
                    differOpnd2.IsPhysicalRegister()) {
                    return false;
                }
                VRegVersion *differVersion1 = optSsaInfo->FindSSAVersion(differOpnd1.GetRegisterNumber());
                VRegVersion *differVersion2 = optSsaInfo->FindSSAVersion(differOpnd2.GetRegisterNumber());
                if (!differVersion1 || !differVersion2) {
                    return false;
                }
                if (differVersion1->GetOriginalRegNO() != differVersion2->GetOriginalRegNO()) {
                    return false;
                }
                differOrigNO = differVersion1->GetOriginalRegNO();
            } else if (!opnd1.Equals(opnd2) && idx != static_cast<uint32>(differIdx)) {
                return false;
            }
        }
        if (differIdx <= 0) {
            return false;
        }
    }
    return true;
}

bool A64PregCopyPattern::CheckUselessDefInsn(const Insn *defInsn) const
{
    Operand &dstOpnd = defInsn->GetOperand(kInsnFirstOpnd);
    CHECK_FATAL(dstOpnd.IsRegister(), "dstOpnd must be register");
    VRegVersion *defVersion = optSsaInfo->FindSSAVersion(static_cast<RegOperand &>(dstOpnd).GetRegisterNumber());
    if (defVersion->GetAllUseInsns().size() == 1) {
        return true;
    }
    /*
     * avoid the case as following
     * In a loop:
     *                      [BB43]
     *            phi: R356, (R345<42>, R377<63>)
     *                     /           \
     *                    /             \
     *                 [BB44]            \
     *             add R377, R356, #1    /
     *             mov R1, R377         /
     *                  bl             /
     *                     \          /
     *                      \        /
     *                        [BB63]
     */
    for (auto &useInfoIt : defVersion->GetAllUseInsns()) {
        DUInsnInfo *useInfo = useInfoIt.second;
        CHECK_FATAL(useInfo, "get useDUInfo failed");
        Insn *useInsn = useInfo->GetInsn();
        CHECK_FATAL(useInsn, "get useInsn failed");
        if (useInsn->IsPhi()) {
            auto &phiDefOpnd = static_cast<RegOperand &>(useInsn->GetOperand(kInsnFirstOpnd));
            uint32 opndNum = defInsn->GetOperandSize();
            for (uint32 i = 0; i < opndNum; ++i) {
                if (defInsn->OpndIsDef(i)) {
                    continue;
                }
                Operand &opnd = defInsn->GetOperand(i);
                if (opnd.IsRegister() &&
                    static_cast<RegOperand &>(opnd).GetRegisterNumber() == phiDefOpnd.GetRegisterNumber()) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool A64PregCopyPattern::CheckValidDefInsn(const Insn *defInsn)
{
    const auto *md = defInsn->GetDesc();
    CHECK_FATAL(md != nullptr, "expect valid AArch64MD");
    /* this pattern applies to all basicOps */
    if (md->IsMove() || md->IsStore() || md->IsLoad() || md->IsLoadStorePair() || md->IsCall() || md->IsDMB() ||
        md->IsVectorOp() || md->IsCondDef() || md->IsCondBranch() || md->IsUnCondBranch()) {
        return false;
    }
    uint32 opndNum = defInsn->GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = defInsn->GetOperand(i);
        if (!opnd.IsRegister() && !opnd.IsImmediate() && !opnd.IsOpdShift() && !opnd.IsOpdExtend()) {
            return false;
        }
        if (opnd.IsRegister()) {
            auto &regOpnd = static_cast<RegOperand &>(opnd);
            if (cgFunc.IsSPOrFP(regOpnd) || regOpnd.IsPhysicalRegister() ||
                (!regOpnd.IsOfIntClass() && !regOpnd.IsOfFloatOrSIMDClass())) {
                return false;
            }
        }
    }
    return true;
}

bool A64PregCopyPattern::CheckCondition(Insn &insn)
{
    MOperator curMop = insn.GetMachineOpcode();
    if (curMop != MOP_xmovrr && curMop != MOP_wmovrr) {
        return false;
    }
    auto &dstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    if (!dstOpnd.IsPhysicalRegister()) {
        return false;
    }
    regno_t useRegNO = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber();
    VRegVersion *useVersion = optSsaInfo->FindSSAVersion(useRegNO);
    Insn *defInsn = FindDefInsn(useVersion);
    if (defInsn == nullptr) {
        return false;
    }
    Operand &defDstOpnd = defInsn->GetOperand(kInsnFirstOpnd);
    /* avoid inline-asm */
    if (!defDstOpnd.IsRegister()) {
        return false;
    }
    if (!CheckMultiUsePoints(defInsn)) {
        return false;
    }
    if (defInsn->IsPhi()) {
        isCrossPhi = true;
        firstPhiInsn = defInsn;
        return CheckPhiCaseCondition(insn, *defInsn);
    } else {
        if (!CheckValidDefInsn(defInsn)) {
            return false;
        }
        if (!CheckUselessDefInsn(defInsn)) {
            return false;
        }
        (void)validDefInsns.emplace_back(defInsn);
    }
    return true;
}

Insn &A64PregCopyPattern::CreateNewPhiInsn(std::unordered_map<uint32, RegOperand *> &newPhiList, Insn *curInsn)
{
    CHECK_FATAL(!newPhiList.empty(), "empty newPhiList");
    RegOperand *differOrigOpnd = cgFunc.GetVirtualRegisterOperand(differOrigNO);
    CHECK_FATAL(differOrigOpnd != nullptr, "get original opnd default");
    PhiOperand &phiList = optSsaInfo->CreatePhiOperand();
    for (auto &it : newPhiList) {
        phiList.InsertOpnd(it.first, *it.second);
    }
    Insn &phiInsn = cgFunc.GetCG()->BuildPhiInsn(*differOrigOpnd, phiList);
    optSsaInfo->CreateNewInsnSSAInfo(phiInsn);
    BB *bb = curInsn->GetBB();
    (void)bb->InsertInsnBefore(*curInsn, phiInsn);
    /* <phiDef-ssaRegNO, phiInsn> */
    bb->AddPhiInsn(static_cast<RegOperand &>(phiInsn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber(), phiInsn);
    return phiInsn;
}

/*
 * Check whether the required phi is available, do not insert phi repeatedly.
 */
RegOperand *A64PregCopyPattern::CheckAndGetExistPhiDef(Insn &phiInsn, std::vector<regno_t> &validDifferRegNOs) const
{
    MapleMap<regno_t, Insn *> &phiInsns = phiInsn.GetBB()->GetPhiInsns();
    for (auto &phiIt : phiInsns) {
        auto &def = static_cast<RegOperand &>(phiIt.second->GetOperand(kInsnFirstOpnd));
        VRegVersion *defVersion = optSsaInfo->FindSSAVersion(def.GetRegisterNumber());
        /*
         * if the phi of the change point has been created (according to original regNO), return the phiDefOpnd.
         * But, there is a problem: the phiDefOpnd of the same original regNO is not the required phi.
         * For example: (in parentheses is the original regNO)
         *           add R110(R80), R106(R80), #1              add R122(R80), R118(R80), #1
         *                                       \            /
         *                                        \          /
         *                              (1) phi: R123(R80), [R110, R122]
         *                                  mov R0, R123
         *           It will return R123 of phi(1) because the differOrigNO is 80, but that's not what we want,
         *           we need to create a new phi(2): R140(R80), [R106, R118].
         *           so we need to check whether all phiOpnds have correct ssaRegNO.
         */
        if (defVersion->GetOriginalRegNO() == differOrigNO) {
            auto &phiOpnd = static_cast<PhiOperand &>(phiIt.second->GetOperand(kInsnSecondOpnd));
            if (phiOpnd.GetOperands().size() == validDifferRegNOs.size()) {
                bool exist = true;
                for (auto &phiListIt : phiOpnd.GetOperands()) {
                    if (std::find(validDifferRegNOs.begin(), validDifferRegNOs.end(),
                                  static_cast<RegOperand *>(phiListIt.second)->GetRegisterNumber()) ==
                        validDifferRegNOs.end()) {
                        exist = false;
                        break;
                    }
                }
                if (exist) {
                    return &static_cast<RegOperand &>(phiIt.second->GetOperand(kInsnFirstOpnd));
                }
            }
        }
    }
    return nullptr;
}

RegOperand &A64PregCopyPattern::DFSBuildPhiInsn(Insn *curInsn, std::unordered_map<uint32, RegOperand *> &visited)
{
    CHECK_FATAL(curInsn, "curInsn must not be null");
    if (visited[curInsn->GetId()] != nullptr) {
        return *visited[curInsn->GetId()];
    }
    if (!curInsn->IsPhi()) {
        return static_cast<RegOperand &>(curInsn->GetOperand(static_cast<uint32>(differIdx)));
    }
    std::unordered_map<uint32, RegOperand *> differPhiList;
    std::vector<regno_t> validDifferRegNOs;
    auto &phiOpnd = static_cast<PhiOperand &>(curInsn->GetOperand(kInsnSecondOpnd));
    for (auto &phiListIt : phiOpnd.GetOperands()) {
        auto &useOpnd = static_cast<RegOperand &>(*phiListIt.second);
        VRegVersion *useVersion = optSsaInfo->FindSSAVersion(useOpnd.GetRegisterNumber());
        Insn *defInsn = FindDefInsn(useVersion);
        CHECK_FATAL(defInsn != nullptr, "get defInsn failed");
        RegOperand &phiDefOpnd = DFSBuildPhiInsn(defInsn, visited);
        (void)differPhiList.emplace(phiListIt.first, &phiDefOpnd);
        (void)validDifferRegNOs.emplace_back(phiDefOpnd.GetRegisterNumber());
    }
    /*
     * The phi in control flow may already exists.
     * For example:
     *                              [BB26]                         [BB45]
     *                          add R191, R103, R187           add R166, R103, R164
     *                                              \         /
     *                                               \       /
     *                                                [BB27]
     *                                      phi: R192, (R191<26>, R166<45>)  ------ curInsn
     *                                      phi: R194, (R187<26>, R164<45>)  ------ the phi witch we need already exists
     *                                                 /                            validDifferRegNOs : [187, 164]
     *                                                /
     *                   [BB28]                    [BB46]
     *              add R215, R103, R211           /
     *                                  \         /
     *                                   \       /
     *                                    [BB29]
     *                            phi: R216, (R215<28>, R192<46>)
     *                            phi: R218, (R211<28>, R194<46>)  ------ the phi witch we need already exists
     *                            mov R0, R216                            validDifferRegNOs : [211, 194]
     */
    RegOperand *existPhiDef = CheckAndGetExistPhiDef(*curInsn, validDifferRegNOs);
    if (existPhiDef == nullptr) {
        Insn &phiInsn = CreateNewPhiInsn(differPhiList, curInsn);
        visited[curInsn->GetId()] = &static_cast<RegOperand &>(phiInsn.GetOperand(kInsnFirstOpnd));
        existPhiDef = &static_cast<RegOperand &>(phiInsn.GetOperand(kInsnFirstOpnd));
    }
    return *existPhiDef;
}

void A64PregCopyPattern::Optimize(Insn &insn)
{
    Insn *defInsn = *validDefInsns.begin();
    MOperator newMop = defInsn->GetMachineOpcode();
    Operand &dstOpnd = insn.GetOperand(kInsnFirstOpnd);
    Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(newMop, AArch64CG::kMd[newMop]);
    uint32 opndNum = defInsn->GetOperandSize();
    newInsn.ResizeOpnds(opndNum);
    if (!isCrossPhi) {
        for (uint32 i = 0; i < opndNum; ++i) {
            if (defInsn->OpndIsDef(i)) {
                newInsn.SetOperand(i, dstOpnd);
            } else {
                newInsn.SetOperand(i, defInsn->GetOperand(i));
            }
        }
    } else {
        std::vector<regno_t> validDifferRegNOs;
        for (Insn *vdInsn : validDefInsns) {
            auto &vdOpnd = static_cast<RegOperand &>(vdInsn->GetOperand(static_cast<uint32>(differIdx)));
            (void)validDifferRegNOs.emplace_back(vdOpnd.GetRegisterNumber());
        }
        RegOperand *differPhiDefOpnd = CheckAndGetExistPhiDef(*firstPhiInsn, validDifferRegNOs);
        if (differPhiDefOpnd == nullptr) {
            std::unordered_map<uint32, RegOperand *> visited;
            differPhiDefOpnd = &DFSBuildPhiInsn(firstPhiInsn, visited);
        }
        CHECK_FATAL(differPhiDefOpnd, "get differPhiDefOpnd failed");
        for (uint32 i = 0; i < opndNum; ++i) {
            if (defInsn->OpndIsDef(i)) {
                newInsn.SetOperand(i, dstOpnd);
            } else if (i == static_cast<uint32>(differIdx)) {
                newInsn.SetOperand(i, *differPhiDefOpnd);
            } else {
                newInsn.SetOperand(i, defInsn->GetOperand(i));
            }
        }
    }
    insn.GetBB()->ReplaceInsn(insn, newInsn);
    /* update ssa info */
    optSsaInfo->ReplaceInsn(insn, newInsn);

    if (PROP_DUMP) {
        LogInfo::MapleLogger() << ">>>>>>> In A64PregCopyPattern : <<<<<<<\n";
        LogInfo::MapleLogger() << "======= ReplaceInsn :\n";
        insn.Dump();
        LogInfo::MapleLogger() << "======= NewInsn :\n";
        newInsn.Dump();
    }
}

void A64PregCopyPattern::Run()
{
    FOR_ALL_BB(bb, &cgFunc)
    {
        FOR_BB_INSNS(insn, bb)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            Init();
            if (!CheckCondition(*insn)) {
                continue;
            }
            Optimize(*insn);
        }
    }
    validDefInsns.clear();
    validDefInsns.shrink_to_fit();
}

void A64ReplaceRegOpndVisitor::Visit(RegOperand *v)
{
    (void)v;
    insn->SetOperand(idx, *newReg);
}
void A64ReplaceRegOpndVisitor::Visit(MemOperand *a64memOpnd)
{
    bool changed = false;
    CHECK_FATAL(a64memOpnd->IsIntactIndexed(), "NYI post/pre index model");
    StackMemPool tempMemPool(memPoolCtrler, "temp mempool for A64ReplaceRegOpndVisitor");
    auto *cpyMem = a64memOpnd->Clone(tempMemPool);
    if (cpyMem->GetBaseRegister() != nullptr &&
        cpyMem->GetBaseRegister()->GetRegisterNumber() == oldReg->GetRegisterNumber()) {
        cpyMem->SetBaseRegister(*static_cast<RegOperand *>(newReg));
        changed = true;
    }
    if (cpyMem->GetIndexRegister() != nullptr &&
        cpyMem->GetIndexRegister()->GetRegisterNumber() == oldReg->GetRegisterNumber()) {
        CHECK_FATAL(!changed, "base reg is equal to index reg");
        cpyMem->SetIndexRegister(*newReg);
        changed = true;
    }
    if (changed) {
        insn->SetMemOpnd(&static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreateMemOpnd(*cpyMem));
    }
}
void A64ReplaceRegOpndVisitor::Visit(ListOperand *v)
{
    for (auto &it : v->GetOperands()) {
        if (it->GetRegisterNumber() == oldReg->GetRegisterNumber()) {
            it = newReg;
        }
    }
}
void A64ReplaceRegOpndVisitor::Visit(PhiOperand *v)
{
    for (auto &it : v->GetOperands()) {
        if (it.second->GetRegisterNumber() == oldReg->GetRegisterNumber()) {
            it.second = newReg;
        }
    }
    auto &phiDest = static_cast<RegOperand &>(insn->GetOperand(kInsnFirstOpnd));
    if (phiDest.GetValidBitsNum() > v->GetLeastCommonValidBit()) {
        phiDest.SetValidBitsNum(v->GetLeastCommonValidBit());
    }
}
}  // namespace maplebe
