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

#include "aarch64_validbit_opt.h"
#include "aarch64_cg.h"

namespace maplebe {
void AArch64ValidBitOpt::DoOpt(BB &bb, Insn &insn)
{
    MOperator curMop = insn.GetMachineOpcode();
    switch (curMop) {
        case MOP_wandrri12:
        case MOP_xandrri13: {
            Optimize<AndValidBitPattern>(bb, insn);
            break;
        }
        case MOP_xuxtb32:
        case MOP_xuxth32:
        case MOP_wubfxrri5i5:
        case MOP_xubfxrri6i6:
        case MOP_wsbfxrri5i5:
        case MOP_xsbfxrri6i6: {
            Optimize<ExtValidBitPattern>(bb, insn);
            break;
        }
        case MOP_wcsetrc:
        case MOP_xcsetrc: {
            Optimize<CmpCsetVBPattern>(bb, insn);
            break;
        }
        case MOP_bge:
        case MOP_blt: {
            Optimize<CmpBranchesPattern>(bb, insn);
            break;
        }
        default:
            break;
    }
}

void AArch64ValidBitOpt::SetValidBits(Insn &insn)
{
    MOperator mop = insn.GetMachineOpcode();
    switch (mop) {
        case MOP_wcsetrc:
        case MOP_xcsetrc: {
            auto &dstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
            dstOpnd.SetValidBitsNum(k1BitSize);
            break;
        }
        case MOP_wmovri32:
        case MOP_xmovri64: {
            Operand &srcOpnd = insn.GetOperand(kInsnSecondOpnd);
            DEBUG_ASSERT(srcOpnd.IsIntImmediate(), "must be ImmOperand");
            auto &immOpnd = static_cast<ImmOperand &>(srcOpnd);
            auto &dstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
            dstOpnd.SetValidBitsNum(GetImmValidBit(immOpnd.GetValue(), dstOpnd.GetSize()));
            break;
        }
        case MOP_xmovrr:
        case MOP_wmovrr: {
            auto &srcOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
            if (!srcOpnd.IsVirtualRegister()) {
                break;
            }
            if (srcOpnd.GetRegisterNumber() == RZR) {
                srcOpnd.SetValidBitsNum(k1BitSize);
            }
            auto &dstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
            if (!(dstOpnd.GetSize() == k64BitSize && srcOpnd.GetSize() == k32BitSize) &&
                !(dstOpnd.GetSize() == k32BitSize && srcOpnd.GetSize() == k64BitSize)) {
                dstOpnd.SetValidBitsNum(srcOpnd.GetValidBitsNum());
            }
            break;
        }
        case MOP_wlsrrri5:
        case MOP_xlsrrri6:
        case MOP_wasrrri5:
        case MOP_xasrrri6: {
            Operand &opnd = insn.GetOperand(kInsnThirdOpnd);
            DEBUG_ASSERT(opnd.IsIntImmediate(), "must be ImmOperand");
            uint32 shiftBits = static_cast<uint32>(static_cast<ImmOperand &>(opnd).GetValue());
            auto &dstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
            auto &srcOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
            if ((static_cast<uint32>(srcOpnd.GetValidBitsNum()) - shiftBits) <= 0) {
                dstOpnd.SetValidBitsNum(k1BitSize);
            } else {
                dstOpnd.SetValidBitsNum(srcOpnd.GetValidBitsNum() - shiftBits);
            }
            break;
        }
        case MOP_wlslrri5:
        case MOP_xlslrri6: {
            Operand &opnd = insn.GetOperand(kInsnThirdOpnd);
            DEBUG_ASSERT(opnd.IsIntImmediate(), "must be ImmOperand");
            uint32 shiftBits = static_cast<uint32>(static_cast<ImmOperand &>(opnd).GetValue());
            auto &dstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
            auto &srcOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
            uint32 newVB = ((srcOpnd.GetValidBitsNum() + shiftBits) > srcOpnd.GetSize())
                               ? srcOpnd.GetSize()
                               : (srcOpnd.GetValidBitsNum() + shiftBits);
            dstOpnd.SetValidBitsNum(newVB);
            break;
        }
        case MOP_xuxtb32:
        case MOP_xuxth32: {
            auto &dstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
            auto &srcOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
            uint32 srcVB = srcOpnd.GetValidBitsNum();
            uint32 newVB = dstOpnd.GetValidBitsNum();
            newVB = (mop == MOP_xuxtb32) ? ((srcVB < k8BitSize) ? srcVB : k8BitSize) : newVB;
            newVB = (mop == MOP_xuxth32) ? ((srcVB < k16BitSize) ? srcVB : k16BitSize) : newVB;
            dstOpnd.SetValidBitsNum(newVB);
            break;
        }
        case MOP_wldrb:
        case MOP_wldrh: {
            auto &dstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
            uint32 newVB = (mop == MOP_wldrb) ? k8BitSize : k16BitSize;
            dstOpnd.SetValidBitsNum(newVB);
            break;
        }
        case MOP_wandrrr:
        case MOP_xandrrr: {
            auto &dstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
            uint32 src1VB = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetValidBitsNum();
            uint32 src2VB = static_cast<RegOperand &>(insn.GetOperand(kInsnThirdOpnd)).GetValidBitsNum();
            uint32 newVB = (src1VB <= src2VB ? src1VB : src2VB);
            dstOpnd.SetValidBitsNum(newVB);
            break;
        }
        case MOP_wandrri12:
        case MOP_xandrri13: {
            auto &dstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
            auto &immOpnd = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
            uint32 src1VB = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetValidBitsNum();
            uint32 src2VB = GetImmValidBit(immOpnd.GetValue(), dstOpnd.GetSize());
            uint32 newVB = (src1VB <= src2VB ? src1VB : src2VB);
            dstOpnd.SetValidBitsNum(newVB);
            break;
        }
        case MOP_wiorrrr:
        case MOP_xiorrrr: {
            auto &dstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
            uint32 src1VB = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetValidBitsNum();
            uint32 src2VB = static_cast<RegOperand &>(insn.GetOperand(kInsnThirdOpnd)).GetValidBitsNum();
            uint32 newVB = (src1VB >= src2VB ? src1VB : src2VB);
            dstOpnd.SetValidBitsNum(newVB);
            break;
        }
        case MOP_wiorrri12:
        case MOP_xiorrri13: {
            auto &dstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
            auto &immOpnd = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
            uint32 src1VB = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetValidBitsNum();
            uint32 src2VB = GetImmValidBit(immOpnd.GetValue(), dstOpnd.GetSize());
            uint32 newVB = (src1VB >= src2VB ? src1VB : src2VB);
            dstOpnd.SetValidBitsNum(newVB);
            break;
        }
        default:
            break;
    }
}

bool AArch64ValidBitOpt::SetPhiValidBits(Insn &insn)
{
    Operand &defOpnd = insn.GetOperand(kInsnFirstOpnd);
    DEBUG_ASSERT(defOpnd.IsRegister(), "expect register");
    auto &defRegOpnd = static_cast<RegOperand &>(defOpnd);
    Operand &phiOpnd = insn.GetOperand(kInsnSecondOpnd);
    DEBUG_ASSERT(phiOpnd.IsPhi(), "expect phiList");
    auto &phiList = static_cast<PhiOperand &>(phiOpnd);
    int32 maxVB = -1;
    for (auto phiOpndIt : phiList.GetOperands()) {
        if (phiOpndIt.second != nullptr) {
            maxVB = (maxVB < static_cast<int32>(phiOpndIt.second->GetValidBitsNum()))
                        ? static_cast<int32>(phiOpndIt.second->GetValidBitsNum())
                        : maxVB;
        }
    }
    if (maxVB >= static_cast<int32>(k0BitSize) && static_cast<uint32>(maxVB) != defRegOpnd.GetValidBitsNum()) {
        defRegOpnd.SetValidBitsNum(static_cast<uint32>(maxVB));
        return true;
    }
    return false;
}

static bool IsZeroRegister(const Operand &opnd)
{
    if (!opnd.IsRegister()) {
        return false;
    }
    const RegOperand *regOpnd = static_cast<const RegOperand *>(&opnd);
    return regOpnd->GetRegisterNumber() == RZR;
}

bool AndValidBitPattern::CheckImmValidBit(int64 andImm, uint32 andImmVB, int64 shiftImm) const
{
    if ((__builtin_ffs(static_cast<int>(andImm)) - 1 == shiftImm) &&
        ((andImm >> shiftImm) == ((1 << (andImmVB - shiftImm)) - 1))) {
        return true;
    }
    return false;
}

bool AndValidBitPattern::CheckCondition(Insn &insn)
{
    MOperator mOp = insn.GetMachineOpcode();
    if (mOp == MOP_wandrri12) {
        newMop = MOP_wmovrr;
    } else if (mOp == MOP_xandrri13) {
        newMop = MOP_xmovrr;
    }
    if (newMop == MOP_undef) {
        return false;
    }
    CHECK_FATAL(insn.GetOperand(kInsnFirstOpnd).IsRegister(), "must be register!");
    CHECK_FATAL(insn.GetOperand(kInsnSecondOpnd).IsRegister(), "must be register!");
    CHECK_FATAL(insn.GetOperand(kInsnThirdOpnd).IsImmediate(), "must be imm!");
    desReg = static_cast<RegOperand *>(&insn.GetOperand(kInsnFirstOpnd));
    srcReg = static_cast<RegOperand *>(&insn.GetOperand(kInsnSecondOpnd));
    auto &andImm = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
    int64 immVal = andImm.GetValue();
    uint32 validBit = srcReg->GetValidBitsNum();
    if (validBit == k8BitSize && immVal == 0xFF) {
        return true;
    } else if (validBit == k16BitSize && immVal == 0xFFFF) {
        return true;
    }
    /* and R287[32], R286[64], #255 */
    if ((desReg->GetSize() < srcReg->GetSize()) && (srcReg->GetValidBitsNum() > desReg->GetSize())) {
        return false;
    }
    InsnSet useInsns = GetAllUseInsn(*desReg);
    if (useInsns.size() == 1) {
        Insn *useInsn = *useInsns.begin();
        MOperator useMop = useInsn->GetMachineOpcode();
        if (useMop != MOP_wasrrri5 && useMop != MOP_xasrrri6 && useMop != MOP_wlsrrri5 && useMop != MOP_xlsrrri6) {
            return false;
        }
        Operand &shiftOpnd = useInsn->GetOperand(kInsnThirdOpnd);
        CHECK_FATAL(shiftOpnd.IsImmediate(), "must be immediate");
        int64 shiftImm = static_cast<ImmOperand &>(shiftOpnd).GetValue();
        uint32 andImmVB = ValidBitOpt::GetImmValidBit(andImm.GetValue(), desReg->GetSize());
        if ((srcReg->GetValidBitsNum() == andImmVB) && CheckImmValidBit(andImm.GetValue(), andImmVB, shiftImm)) {
            return true;
        }
    }
    return false;
}

void AndValidBitPattern::Run(BB &bb, Insn &insn)
{
    if (!CheckCondition(insn)) {
        return;
    }
    Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, *desReg, *srcReg);
    bb.ReplaceInsn(insn, newInsn);
    /* update ssa info */
    ssaInfo->ReplaceInsn(insn, newInsn);
    if (desReg->GetSize() < srcReg->GetSize()) {
        ssaInfo->InsertSafePropInsn(newInsn.GetId());
    }
    /* dump pattern info */
    if (CG_VALIDBIT_OPT_DUMP) {
        std::vector<Insn *> prevs;
        prevs.emplace_back(&insn);
        DumpAfterPattern(prevs, &insn, &newInsn);
    }
}

bool ExtValidBitPattern::CheckCondition(Insn &insn)
{
    Operand &dstOpnd = insn.GetOperand(kInsnFirstOpnd);
    Operand &srcOpnd = insn.GetOperand(kInsnSecondOpnd);
    MOperator mOp = insn.GetMachineOpcode();
    switch (mOp) {
        case MOP_xuxtb32:
        case MOP_xuxth32: {
            CHECK_FATAL(dstOpnd.IsRegister(), "must be register");
            CHECK_FATAL(srcOpnd.IsRegister(), "must be register");
            if (static_cast<RegOperand &>(dstOpnd).GetValidBitsNum() !=
                static_cast<RegOperand &>(srcOpnd).GetValidBitsNum()) {
                return false;
            }
            newMop = MOP_wmovrr;
            break;
        }
        case MOP_wubfxrri5i5:
        case MOP_xubfxrri6i6:
        case MOP_wsbfxrri5i5:
        case MOP_xsbfxrri6i6: {
            Operand &immOpnd1 = insn.GetOperand(kInsnThirdOpnd);
            Operand &immOpnd2 = insn.GetOperand(kInsnFourthOpnd);
            CHECK_FATAL(immOpnd1.IsImmediate(), "must be immediate");
            CHECK_FATAL(immOpnd2.IsImmediate(), "must be immediate");
            int64 lsb = static_cast<ImmOperand &>(immOpnd1).GetValue();
            int64 width = static_cast<ImmOperand &>(immOpnd2).GetValue();
            if (lsb != 0 || static_cast<RegOperand &>(srcOpnd).GetValidBitsNum() > width) {
                return false;
            }
            if ((mOp == MOP_wsbfxrri5i5 || mOp == MOP_xsbfxrri6i6) &&
                width != static_cast<RegOperand &>(srcOpnd).GetSize()) {
                return false;
            }
            if (mOp == MOP_wubfxrri5i5 || mOp == MOP_wsbfxrri5i5) {
                newMop = MOP_wmovrr;
            } else if (mOp == MOP_xubfxrri6i6 || mOp == MOP_xsbfxrri6i6) {
                newMop = MOP_xmovrr;
            }
            break;
        }
        default:
            return false;
    }
    newDstOpnd = &static_cast<RegOperand &>(dstOpnd);
    newSrcOpnd = &static_cast<RegOperand &>(srcOpnd);
    return true;
}

void ExtValidBitPattern::Run(BB &bb, Insn &insn)
{
    if (!CheckCondition(insn)) {
        return;
    }
    MOperator mOp = insn.GetMachineOpcode();
    switch (mOp) {
        case MOP_xuxtb32:
        case MOP_xuxth32: {
            insn.SetMOP(AArch64CG::kMd[newMop]);
            break;
        }
        case MOP_wubfxrri5i5:
        case MOP_xubfxrri6i6:
        case MOP_wsbfxrri5i5:
        case MOP_xsbfxrri6i6: {
            Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, *newDstOpnd, *newSrcOpnd);
            bb.ReplaceInsn(insn, newInsn);
            /* update ssa info */
            ssaInfo->ReplaceInsn(insn, newInsn);
            /* dump pattern info */
            if (CG_VALIDBIT_OPT_DUMP) {
                std::vector<Insn *> prevs;
                prevs.emplace_back(&insn);
                DumpAfterPattern(prevs, &insn, &newInsn);
            }
            break;
        }
        default:
            return;
    }
}

bool CmpCsetVBPattern::IsContinuousCmpCset(const Insn &curInsn)
{
    auto &csetDstReg = static_cast<RegOperand &>(curInsn.GetOperand(kInsnFirstOpnd));
    CHECK_FATAL(csetDstReg.IsSSAForm(), "dstOpnd must be ssa form");
    VRegVersion *dstVersion = ssaInfo->FindSSAVersion(csetDstReg.GetRegisterNumber());
    DEBUG_ASSERT(dstVersion != nullptr, "find vRegVersion failed");
    for (auto useDUInfoIt : dstVersion->GetAllUseInsns()) {
        if (useDUInfoIt.second == nullptr) {
            continue;
        }
        Insn *useInsn = useDUInfoIt.second->GetInsn();
        if (useInsn == nullptr) {
            continue;
        }
        MOperator useMop = useInsn->GetMachineOpcode();
        if (useMop == MOP_wcmpri || useMop == MOP_xcmpri) {
            auto &ccDstReg = static_cast<RegOperand &>(useInsn->GetOperand(kInsnFirstOpnd));
            CHECK_FATAL(ccDstReg.IsSSAForm(), "dstOpnd must be ssa form");
            VRegVersion *ccDstVersion = ssaInfo->FindSSAVersion(ccDstReg.GetRegisterNumber());
            DEBUG_ASSERT(ccDstVersion != nullptr, "find vRegVersion failed");
            for (auto ccUseDUInfoIt : ccDstVersion->GetAllUseInsns()) {
                if (ccUseDUInfoIt.second == nullptr) {
                    continue;
                }
                Insn *ccUseInsn = ccUseDUInfoIt.second->GetInsn();
                if (ccUseInsn == nullptr) {
                    continue;
                }
                MOperator ccUseMop = ccUseInsn->GetMachineOpcode();
                if (ccUseMop == MOP_wcsetrc || ccUseMop == MOP_xcsetrc) {
                    return true;
                }
            }
        }
    }
    return false;
}

bool CmpCsetVBPattern::OpndDefByOneValidBit(const Insn &defInsn)
{
    if (defInsn.IsPhi()) {
        return (static_cast<RegOperand &>(cmpInsn->GetOperand(kInsnSecondOpnd)).GetValidBitsNum() == k1BitSize) ||
               (static_cast<RegOperand &>(cmpInsn->GetOperand(kInsnSecondOpnd)).GetValidBitsNum() == k0BitSize);
    }
    MOperator defMop = defInsn.GetMachineOpcode();
    switch (defMop) {
        case MOP_wcsetrc:
        case MOP_xcsetrc:
            return true;
        case MOP_wmovri32:
        case MOP_xmovri64: {
            Operand &defOpnd = defInsn.GetOperand(kInsnSecondOpnd);
            DEBUG_ASSERT(defOpnd.IsIntImmediate(), "expects ImmOperand");
            auto &defConst = static_cast<ImmOperand &>(defOpnd);
            int64 defConstValue = defConst.GetValue();
            return (defConstValue == 0 || defConstValue == 1);
        }
        case MOP_xmovrr:
        case MOP_wmovrr:
            return IsZeroRegister(defInsn.GetOperand(kInsnSecondOpnd));
        case MOP_wlsrrri5:
        case MOP_xlsrrri6: {
            Operand &opnd2 = defInsn.GetOperand(kInsnThirdOpnd);
            DEBUG_ASSERT(opnd2.IsIntImmediate(), "expects ImmOperand");
            auto &opndImm = static_cast<ImmOperand &>(opnd2);
            int64 shiftBits = opndImm.GetValue();
            return ((defMop == MOP_wlsrrri5 && shiftBits == (k32BitSize - 1)) ||
                    (defMop == MOP_xlsrrri6 && shiftBits == (k64BitSize - 1)));
        }
        default:
            return false;
    }
}

bool CmpCsetVBPattern::CheckCondition(Insn &csetInsn)
{
    MOperator curMop = csetInsn.GetMachineOpcode();
    if (curMop != MOP_wcsetrc && curMop != MOP_xcsetrc) {
        return false;
    }
    /* combine [continuous cmp & cset] first, to eliminate more insns */
    if (IsContinuousCmpCset(csetInsn)) {
        return false;
    }
    RegOperand &ccReg = static_cast<RegOperand &>(csetInsn.GetOperand(kInsnThirdOpnd));
    regno_t ccRegNo = ccReg.GetRegisterNumber();
    cmpInsn = GetDefInsn(ccReg);
    CHECK_NULL_FATAL(cmpInsn);
    MOperator mop = cmpInsn->GetMachineOpcode();
    if ((mop != MOP_wcmpri) && (mop != MOP_xcmpri)) {
        return false;
    }
    VRegVersion *ccRegVersion = ssaInfo->FindSSAVersion(ccRegNo);
    if (ccRegVersion->GetAllUseInsns().size() > k1BitSize) {
        return false;
    }
    Operand &cmpSecondOpnd = cmpInsn->GetOperand(kInsnThirdOpnd);
    CHECK_FATAL(cmpSecondOpnd.IsIntImmediate(), "expects ImmOperand");
    auto &cmpConst = static_cast<ImmOperand &>(cmpSecondOpnd);
    cmpConstVal = cmpConst.GetValue();
    /* get ImmOperand, must be 0 or 1 */
    if ((cmpConstVal != 0) && (cmpConstVal != k1BitSize)) {
        return false;
    }
    Operand &cmpFirstOpnd = cmpInsn->GetOperand(kInsnSecondOpnd);
    CHECK_FATAL(cmpFirstOpnd.IsRegister(), "cmpFirstOpnd must be register!");
    RegOperand &cmpReg = static_cast<RegOperand &>(cmpFirstOpnd);
    Insn *defInsn = GetDefInsn(cmpReg);
    if (defInsn == nullptr) {
        return false;
    }
    if (defInsn->GetMachineOpcode() == MOP_wmovrr || defInsn->GetMachineOpcode() == MOP_xmovrr) {
        auto &srcOpnd = static_cast<RegOperand &>(defInsn->GetOperand(kInsnSecondOpnd));
        if (!srcOpnd.IsVirtualRegister()) {
            return false;
        }
    }
    return ((cmpReg.GetValidBitsNum() == k1BitSize) || (cmpReg.GetValidBitsNum() == k0BitSize) ||
            OpndDefByOneValidBit(*defInsn));
}

void CmpCsetVBPattern::Run(BB &bb, Insn &csetInsn)
{
    if (!CheckCondition(csetInsn)) {
        return;
    }
    Operand &csetFirstOpnd = csetInsn.GetOperand(kInsnFirstOpnd);
    Operand &cmpFirstOpnd = cmpInsn->GetOperand(kInsnSecondOpnd);
    auto &cond = static_cast<CondOperand &>(csetInsn.GetOperand(kInsnSecondOpnd));
    Insn *newInsn = nullptr;

    /* cmpFirstOpnd == 1 */
    if ((cmpConstVal == 0 && cond.GetCode() == CC_NE) || (cmpConstVal == 1 && cond.GetCode() == CC_EQ)) {
        MOperator mopCode = (cmpFirstOpnd.GetSize() == k64BitSize) ? MOP_xmovrr : MOP_wmovrr;
        newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(mopCode, csetFirstOpnd, cmpFirstOpnd);
    } else if ((cmpConstVal == 1 && cond.GetCode() == CC_NE) || (cmpConstVal == 0 && cond.GetCode() == CC_EQ)) {
        /* cmpFirstOpnd == 0 */
        MOperator mopCode = (cmpFirstOpnd.GetSize() == k64BitSize) ? MOP_xeorrri13 : MOP_weorrri12;
        ImmOperand &one = static_cast<AArch64CGFunc *>(cgFunc)->CreateImmOperand(1, k8BitSize, false);
        newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(mopCode, csetFirstOpnd, cmpFirstOpnd, one);
    }
    if (newInsn == nullptr) {
        return;
    }
    bb.ReplaceInsn(csetInsn, *newInsn);
    ssaInfo->ReplaceInsn(csetInsn, *newInsn);
    if (CG_VALIDBIT_OPT_DUMP && (newInsn != nullptr)) {
        std::vector<Insn *> prevInsns;
        prevInsns.emplace_back(cmpInsn);
        prevInsns.emplace_back(&csetInsn);
        DumpAfterPattern(prevInsns, newInsn, nullptr);
    }
}

void CmpBranchesPattern::SelectNewMop(MOperator mop)
{
    switch (mop) {
        case MOP_bge: {
            newMop = is64Bit ? MOP_xtbnz : MOP_wtbnz;
            break;
        }
        case MOP_blt: {
            newMop = is64Bit ? MOP_xtbz : MOP_wtbz;
            break;
        }
        default:
            break;
    }
}

bool CmpBranchesPattern::CheckCondition(Insn &insn)
{
    MOperator curMop = insn.GetMachineOpcode();
    if (curMop != MOP_bge && curMop != MOP_blt) {
        return false;
    }
    auto &ccReg = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    prevCmpInsn = GetDefInsn(ccReg);
    if (prevCmpInsn == nullptr) {
        return false;
    }
    MOperator cmpMop = prevCmpInsn->GetMachineOpcode();
    if (cmpMop != MOP_wcmpri && cmpMop != MOP_xcmpri) {
        return false;
    }
    is64Bit = (cmpMop == MOP_xcmpri);
    auto &cmpUseOpnd = static_cast<RegOperand &>(prevCmpInsn->GetOperand(kInsnSecondOpnd));
    auto &cmpImmOpnd = static_cast<ImmOperand &>(prevCmpInsn->GetOperand(kInsnThirdOpnd));
    int64 cmpImmVal = cmpImmOpnd.GetValue();
    newImmVal = ValidBitOpt::GetLogValueAtBase2(cmpImmVal);
    if (newImmVal < 0 || cmpUseOpnd.GetValidBitsNum() != (newImmVal + 1)) {
        return false;
    }
    SelectNewMop(curMop);
    if (newMop == MOP_undef) {
        return false;
    }
    return true;
}

void CmpBranchesPattern::Run(BB &bb, Insn &insn)
{
    if (!CheckCondition(insn)) {
        return;
    }
    auto *aarFunc = static_cast<AArch64CGFunc *>(cgFunc);
    auto &labelOpnd = static_cast<LabelOperand &>(insn.GetOperand(kInsnSecondOpnd));
    ImmOperand &newImmOpnd = aarFunc->CreateImmOperand(newImmVal, k8BitSize, false);
    Insn &newInsn =
        cgFunc->GetInsnBuilder()->BuildInsn(newMop, prevCmpInsn->GetOperand(kInsnSecondOpnd), newImmOpnd, labelOpnd);
    bb.ReplaceInsn(insn, newInsn);
    /* update ssa info */
    ssaInfo->ReplaceInsn(insn, newInsn);
    /* dump pattern info */
    if (CG_VALIDBIT_OPT_DUMP) {
        std::vector<Insn *> prevs;
        prevs.emplace_back(prevCmpInsn);
        DumpAfterPattern(prevs, &insn, &newInsn);
    }
}
} /* namespace maplebe */
