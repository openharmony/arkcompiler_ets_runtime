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

#include "aarch64_ico.h"
#include "ico.h"
#include "cg.h"
#include "cg_option.h"
#include "aarch64_isa.h"
#include "aarch64_insn.h"
#include "aarch64_cgfunc.h"

/*
 * This phase implements if-conversion optimization,
 * which tries to convert conditional branches into cset/csel instructions
 */
namespace maplebe {
void AArch64IfConversionOptimizer::InitOptimizePatterns()
{
    singlePassPatterns.emplace_back(memPool->New<AArch64ICOIfThenElsePattern>(*cgFunc));
    singlePassPatterns.emplace_back(memPool->New<AArch64ICOSameCondPattern>(*cgFunc));
    singlePassPatterns.emplace_back(memPool->New<AArch64ICOMorePredsPattern>(*cgFunc));
}

/* build ccmp Insn */
Insn *AArch64ICOPattern::BuildCcmpInsn(ConditionCode ccCode, const Insn *cmpInsn) const
{
    Operand &opnd0 = cmpInsn->GetOperand(kInsnFirstOpnd);
    Operand &opnd1 = cmpInsn->GetOperand(kInsnSecondOpnd);
    Operand &opnd2 = cmpInsn->GetOperand(kInsnThirdOpnd);
    /* ccmp has only int opnd */
    if (!static_cast<RegOperand &>(opnd1).IsOfIntClass()) {
        return nullptr;
    }
    AArch64CGFunc *func = static_cast<AArch64CGFunc *>(cgFunc);
    uint32 nzcv = GetNZCV(ccCode, false);
    if (nzcv == k16BitSize) {
        return nullptr;
    }
    ImmOperand &opnd3 = func->CreateImmOperand(PTY_u8, nzcv);
    CondOperand &cond = static_cast<AArch64CGFunc *>(cgFunc)->GetCondOperand(ccCode);
    uint32 dSize = opnd1.GetSize();
    bool isIntTy = opnd2.IsIntImmediate();
    MOperator mOpCode = isIntTy ? (dSize == k64BitSize ? MOP_xccmpriic : MOP_wccmpriic)
                                : (dSize == k64BitSize ? MOP_xccmprric : MOP_wccmprric);
    /* cmp opnd2 in the range 0-4095, ccmp opnd2 in the range 0-31 */
    if (isIntTy && static_cast<RegOperand &>(opnd2).GetRegisterNumber() >= k32BitSize) {
        return nullptr;
    }
    return &cgFunc->GetInsnBuilder()->BuildInsn(mOpCode, opnd0, opnd1, opnd2, opnd3, cond);
}

/* Rooted ccCode resource NZCV */
uint32 AArch64ICOPattern::GetNZCV(ConditionCode ccCode, bool inverse)
{
    switch (ccCode) {
        case CC_EQ:
            return inverse ? k4BitSize : k0BitSize;
        case CC_HS:
            return inverse ? k2BitSize : k0BitSize;
        case CC_MI:
            return inverse ? k8BitSize : k0BitSize;
        case CC_VS:
            return inverse ? k1BitSize : k0BitSize;
        case CC_VC:
            return inverse ? k0BitSize : k1BitSize;
        case CC_LS:
            return inverse ? k4BitSize : k2BitSize;
        case CC_LO:
            return inverse ? k0BitSize : k2BitSize;
        case CC_NE:
            return inverse ? k0BitSize : k4BitSize;
        case CC_HI:
            return inverse ? k2BitSize : k4BitSize;
        case CC_PL:
            return inverse ? k0BitSize : k8BitSize;
        default:
            return k16BitSize;
    }
}

Insn *AArch64ICOPattern::BuildCmpInsn(const Insn &condBr) const
{
    AArch64CGFunc *func = static_cast<AArch64CGFunc *>(cgFunc);
    RegOperand &reg = static_cast<RegOperand &>(condBr.GetOperand(0));
    PrimType ptyp = (reg.GetSize() == k64BitSize) ? PTY_u64 : PTY_u32;
    ImmOperand &numZero = func->CreateImmOperand(ptyp, 0);
    Operand &rflag = func->GetOrCreateRflag();
    MOperator mopCode = (reg.GetSize() == k64BitSize) ? MOP_xcmpri : MOP_wcmpri;
    Insn &cmpInsn = func->GetInsnBuilder()->BuildInsn(mopCode, rflag, reg, numZero);
    return &cmpInsn;
}

bool AArch64ICOPattern::IsSetInsn(const Insn &insn, Operand *&dest, std::vector<Operand *> &src) const
{
    MOperator mOpCode = insn.GetMachineOpcode();
    if ((mOpCode >= MOP_xmovrr && mOpCode <= MOP_xvmovd) || cgFunc->GetTheCFG()->IsAddOrSubInsn(insn)) {
        dest = &(insn.GetOperand(0));
        for (uint32 i = 1; i < insn.GetOperandSize(); ++i) {
            (void)src.emplace_back(&(insn.GetOperand(i)));
        }
        return true;
    }
    dest = nullptr;
    src.clear();
    return false;
}

ConditionCode AArch64ICOPattern::Encode(MOperator mOp, bool inverse) const
{
    switch (mOp) {
        case MOP_bmi:
            return inverse ? CC_PL : CC_MI;
        case MOP_bvc:
            return inverse ? CC_VS : CC_VC;
        case MOP_bls:
            return inverse ? CC_HI : CC_LS;
        case MOP_blt:
            return inverse ? CC_GE : CC_LT;
        case MOP_ble:
            return inverse ? CC_GT : CC_LE;
        case MOP_beq:
            return inverse ? CC_NE : CC_EQ;
        case MOP_bne:
            return inverse ? CC_EQ : CC_NE;
        case MOP_blo:
            return inverse ? CC_HS : CC_LO;
        case MOP_bpl:
            return inverse ? CC_MI : CC_PL;
        case MOP_bhs:
            return inverse ? CC_LO : CC_HS;
        case MOP_bvs:
            return inverse ? CC_VC : CC_VS;
        case MOP_bhi:
            return inverse ? CC_LS : CC_HI;
        case MOP_bgt:
            return inverse ? CC_LE : CC_GT;
        case MOP_bge:
            return inverse ? CC_LT : CC_GE;
        case MOP_wcbnz:
            return inverse ? CC_EQ : CC_NE;
        case MOP_xcbnz:
            return inverse ? CC_EQ : CC_NE;
        case MOP_wcbz:
            return inverse ? CC_NE : CC_EQ;
        case MOP_xcbz:
            return inverse ? CC_NE : CC_EQ;
        default:
            return kCcLast;
    }
}

Insn *AArch64ICOPattern::BuildCondSet(const Insn &branch, RegOperand &reg, bool inverse) const
{
    ConditionCode ccCode = Encode(branch.GetMachineOpcode(), inverse);
    DEBUG_ASSERT(ccCode != kCcLast, "unknown cond, ccCode can't be kCcLast");
    AArch64CGFunc *func = static_cast<AArch64CGFunc *>(cgFunc);
    CondOperand &cond = func->GetCondOperand(ccCode);
    Operand &rflag = func->GetOrCreateRflag();
    MOperator mopCode = (reg.GetSize() == k64BitSize) ? MOP_xcsetrc : MOP_wcsetrc;
    return &func->GetInsnBuilder()->BuildInsn(mopCode, reg, cond, rflag);
}

Insn *AArch64ICOPattern::BuildCondSel(const Insn &branch, MOperator mOp, RegOperand &dst, RegOperand &src1,
                                      RegOperand &src2) const
{
    ConditionCode ccCode = Encode(branch.GetMachineOpcode(), false);
    DEBUG_ASSERT(ccCode != kCcLast, "unknown cond, ccCode can't be kCcLast");
    CondOperand &cond = static_cast<AArch64CGFunc *>(cgFunc)->GetCondOperand(ccCode);
    Operand &rflag = static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreateRflag();
    return &cgFunc->GetInsnBuilder()->BuildInsn(mOp, dst, src1, src2, cond, rflag);
}

void AArch64ICOIfThenElsePattern::GenerateInsnForImm(const Insn &branchInsn, Operand &ifDest, Operand &elseDest,
                                                     RegOperand &destReg, std::vector<Insn *> &generateInsn)
{
    ImmOperand &imm1 = static_cast<ImmOperand &>(ifDest);
    ImmOperand &imm2 = static_cast<ImmOperand &>(elseDest);
    bool inverse = imm1.IsZero() && imm2.IsOne();
    if (inverse || (imm2.IsZero() && imm1.IsOne())) {
        Insn *csetInsn = BuildCondSet(branchInsn, destReg, inverse);
        DEBUG_ASSERT(csetInsn != nullptr, "build a insn failed");
        generateInsn.emplace_back(csetInsn);
    } else if (imm1.GetValue() == imm2.GetValue()) {
        bool destIsIntTy = destReg.IsOfIntClass();
        MOperator mOp = destIsIntTy ? ((destReg.GetSize() == k64BitSize ? MOP_xmovri64 : MOP_wmovri32))
                                    : ((destReg.GetSize() == k64BitSize ? MOP_xdfmovri : MOP_wsfmovri));
        Insn &tempInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, destReg, imm1);
        generateInsn.emplace_back(&tempInsn);
    } else {
        bool destIsIntTy = destReg.IsOfIntClass();
        uint32 dSize = destReg.GetSize();
        bool isD64 = dSize == k64BitSize;
        MOperator mOp = destIsIntTy ? ((destReg.GetSize() == k64BitSize ? MOP_xmovri64 : MOP_wmovri32))
                                    : ((destReg.GetSize() == k64BitSize ? MOP_xdfmovri : MOP_wsfmovri));
        RegOperand *tempTarIf = nullptr;
        if (imm1.IsZero()) {
            tempTarIf = &cgFunc->GetZeroOpnd(dSize);
        } else {
            tempTarIf = cgFunc->GetTheCFG()->CreateVregFromReg(destReg);
            Insn &tempInsnIf = cgFunc->GetInsnBuilder()->BuildInsn(mOp, *tempTarIf, imm1);
            generateInsn.emplace_back(&tempInsnIf);
        }

        RegOperand *tempTarElse = nullptr;
        if (imm2.IsZero()) {
            tempTarElse = &cgFunc->GetZeroOpnd(dSize);
        } else {
            tempTarElse = cgFunc->GetTheCFG()->CreateVregFromReg(destReg);
            Insn &tempInsnElse = cgFunc->GetInsnBuilder()->BuildInsn(mOp, *tempTarElse, imm2);
            generateInsn.emplace_back(&tempInsnElse);
        }

        bool isIntTy = destReg.IsOfIntClass();
        MOperator mOpCode = isIntTy ? (isD64 ? MOP_xcselrrrc : MOP_wcselrrrc)
                                    : (isD64 ? MOP_dcselrrrc : (dSize == k32BitSize ? MOP_scselrrrc : MOP_hcselrrrc));
        Insn *cselInsn = BuildCondSel(branchInsn, mOpCode, destReg, *tempTarIf, *tempTarElse);
        CHECK_FATAL(cselInsn != nullptr, "build a csel insn failed");
        generateInsn.emplace_back(cselInsn);
    }
}

RegOperand *AArch64ICOIfThenElsePattern::GenerateRegAndTempInsn(Operand &dest, const RegOperand &destReg,
                                                                std::vector<Insn *> &generateInsn) const
{
    RegOperand *reg = nullptr;
    if (!dest.IsRegister()) {
        bool destIsIntTy = destReg.IsOfIntClass();
        bool isDest64 = destReg.GetSize() == k64BitSize;
        MOperator mOp =
            destIsIntTy ? (isDest64 ? MOP_xmovri64 : MOP_wmovri32) : (isDest64 ? MOP_xdfmovri : MOP_wsfmovri);
        reg = cgFunc->GetTheCFG()->CreateVregFromReg(destReg);
        ImmOperand &tempSrcElse = static_cast<ImmOperand &>(dest);
        if (tempSrcElse.IsZero()) {
            return &cgFunc->GetZeroOpnd(destReg.GetSize());
        }
        Insn &tempInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, *reg, tempSrcElse);
        generateInsn.emplace_back(&tempInsn);
        return reg;
    } else {
        return (static_cast<RegOperand *>(&dest));
    }
}

void AArch64ICOIfThenElsePattern::GenerateInsnForReg(const Insn &branchInsn, Operand &ifDest, Operand &elseDest,
                                                     RegOperand &destReg, std::vector<Insn *> &generateInsn)
{
    RegOperand *tReg = static_cast<RegOperand *>(&ifDest);
    RegOperand *eReg = static_cast<RegOperand *>(&elseDest);

    /* mov w0, w1   mov w0, w1  --> mov w0, w1 */
    if (eReg->GetRegisterNumber() == tReg->GetRegisterNumber()) {
        uint32 dSize = destReg.GetSize();
        bool srcIsIntTy = tReg->IsOfIntClass();
        bool destIsIntTy = destReg.IsOfIntClass();
        MOperator mOp;
        if (dSize == k64BitSize) {
            mOp = srcIsIntTy ? (destIsIntTy ? MOP_xmovrr : MOP_xvmovdr) : (destIsIntTy ? MOP_xvmovrd : MOP_xvmovd);
        } else {
            mOp = srcIsIntTy ? (destIsIntTy ? MOP_wmovrr : MOP_xvmovsr) : (destIsIntTy ? MOP_xvmovrs : MOP_xvmovs);
        }
        Insn &tempInsnIf = cgFunc->GetInsnBuilder()->BuildInsn(mOp, destReg, *tReg);
        generateInsn.emplace_back(&tempInsnIf);
    } else {
        uint32 dSize = destReg.GetSize();
        bool isIntTy = destReg.IsOfIntClass();
        MOperator mOpCode =
            isIntTy ? (dSize == k64BitSize ? MOP_xcselrrrc : MOP_wcselrrrc)
                    : (dSize == k64BitSize ? MOP_dcselrrrc : (dSize == k32BitSize ? MOP_scselrrrc : MOP_hcselrrrc));
        Insn *cselInsn = BuildCondSel(branchInsn, mOpCode, destReg, *tReg, *eReg);
        CHECK_FATAL(cselInsn != nullptr, "build a csel insn failed");
        generateInsn.emplace_back(cselInsn);
    }
}

Operand *AArch64ICOIfThenElsePattern::GetDestReg(const std::map<Operand *, std::vector<Operand *>> &destSrcMap,
                                                 const RegOperand &destReg) const
{
    Operand *dest = nullptr;
    for (const auto &destSrcPair : destSrcMap) {
        DEBUG_ASSERT(destSrcPair.first->IsRegister(), "opnd must be register");
        RegOperand *destRegInMap = static_cast<RegOperand *>(destSrcPair.first);
        DEBUG_ASSERT(destRegInMap != nullptr, "nullptr check");
        if (destRegInMap->GetRegisterNumber() == destReg.GetRegisterNumber()) {
            if (destSrcPair.second.size() > 1) {
                dest = destSrcPair.first;
            } else {
                dest = destSrcPair.second[0];
            }
            break;
        }
    }
    return dest;
}

bool AArch64ICOIfThenElsePattern::BuildCondMovInsn(BB &cmpBB, const BB &bb,
                                                   const std::map<Operand *, std::vector<Operand *>> &ifDestSrcMap,
                                                   const std::map<Operand *, std::vector<Operand *>> &elseDestSrcMap,
                                                   bool elseBBIsProcessed, std::vector<Insn *> &generateInsn)
{
    Insn *branchInsn = cgFunc->GetTheCFG()->FindLastCondBrInsn(cmpBB);
    FOR_BB_INSNS_CONST(insn, (&bb))
    {
        if (!insn->IsMachineInstruction() || insn->IsBranch()) {
            continue;
        }
        Operand *dest = nullptr;
        std::vector<Operand *> src;

        if (!IsSetInsn(*insn, dest, src)) {
            DEBUG_ASSERT(false, "insn check");
        }
        DEBUG_ASSERT(dest->IsRegister(), "register check");
        RegOperand *destReg = static_cast<RegOperand *>(dest);

        Operand *elseDest = GetDestReg(elseDestSrcMap, *destReg);
        Operand *ifDest = GetDestReg(ifDestSrcMap, *destReg);

        if (elseBBIsProcessed) {
            if (elseDest != nullptr) {
                continue;
            }
            elseDest = dest;
            DEBUG_ASSERT(ifDest != nullptr, "null ptr check");
            if (!bb.GetLiveOut()->TestBit(destReg->GetRegisterNumber())) {
                continue;
            }
        } else {
            DEBUG_ASSERT(elseDest != nullptr, "null ptr check");
            if (ifDest == nullptr) {
                if (!bb.GetLiveOut()->TestBit(destReg->GetRegisterNumber())) {
                    continue;
                }
                ifDest = dest;
            }
        }

        /* generate cset or csel instruction */
        DEBUG_ASSERT(ifDest != nullptr, "null ptr check");
        if (ifDest->IsIntImmediate() && elseDest->IsIntImmediate()) {
            GenerateInsnForImm(*branchInsn, *ifDest, *elseDest, *destReg, generateInsn);
        } else {
            RegOperand *tReg = GenerateRegAndTempInsn(*ifDest, *destReg, generateInsn);
            RegOperand *eReg = GenerateRegAndTempInsn(*elseDest, *destReg, generateInsn);
            if ((tReg->GetRegisterType() != eReg->GetRegisterType()) ||
                (tReg->GetRegisterType() != destReg->GetRegisterType())) {
                return false;
            }
            GenerateInsnForReg(*branchInsn, *tReg, *eReg, *destReg, generateInsn);
        }
    }

    return true;
}

bool AArch64ICOIfThenElsePattern::CheckHasSameDest(std::vector<Insn *> &lInsn, std::vector<Insn *> &rInsn) const
{
    for (size_t i = 0; i < lInsn.size(); ++i) {
        if (cgFunc->GetTheCFG()->IsAddOrSubInsn(*lInsn[i])) {
            bool hasSameDest = false;
            for (size_t j = 0; j < rInsn.size(); ++j) {
                RegOperand *rDestReg = static_cast<RegOperand *>(&rInsn[j]->GetOperand(0));
                RegOperand *lDestReg = static_cast<RegOperand *>(&lInsn[i]->GetOperand(0));
                if (lDestReg->GetRegisterNumber() == rDestReg->GetRegisterNumber()) {
                    hasSameDest = true;
                    break;
                }
            }
            if (!hasSameDest) {
                return false;
            }
        }
    }
    return true;
}

bool AArch64ICOIfThenElsePattern::CheckModifiedRegister(Insn &insn,
                                                        std::map<Operand *, std::vector<Operand *>> &destSrcMap,
                                                        std::vector<Operand *> &src, Operand &dest, const Insn *cmpInsn,
                                                        const Operand *flagOpnd) const
{
    /* src was modified in this blcok earlier */
    for (auto srcOpnd : src) {
        if (srcOpnd->IsRegister()) {
            RegOperand &srcReg = static_cast<RegOperand &>(*srcOpnd);
            for (const auto &destSrcPair : destSrcMap) {
                DEBUG_ASSERT(destSrcPair.first->IsRegister(), "opnd must be register");
                RegOperand *mapSrcReg = static_cast<RegOperand *>(destSrcPair.first);
                if (mapSrcReg->GetRegisterNumber() == srcReg.GetRegisterNumber()) {
                    return false;
                }
            }
        }
    }

    /* dest register was modified earlier in this block */
    DEBUG_ASSERT(dest.IsRegister(), "opnd must be register");
    RegOperand &destReg = static_cast<RegOperand &>(dest);
    for (const auto &destSrcPair : destSrcMap) {
        DEBUG_ASSERT(destSrcPair.first->IsRegister(), "opnd must be register");
        RegOperand *mapSrcReg = static_cast<RegOperand *>(destSrcPair.first);
        if (mapSrcReg->GetRegisterNumber() == destReg.GetRegisterNumber()) {
            return false;
        }
    }

    /* src register is modified later in this block, will not be processed */
    for (auto srcOpnd : src) {
        if (srcOpnd->IsRegister()) {
            RegOperand &srcReg = static_cast<RegOperand &>(*srcOpnd);
            if (destReg.IsOfFloatOrSIMDClass() && srcReg.GetRegisterNumber() == RZR) {
                return false;
            }
            for (Insn *tmpInsn = &insn; tmpInsn != nullptr; tmpInsn = tmpInsn->GetNext()) {
                Operand *tmpDest = nullptr;
                std::vector<Operand *> tmpSrc;
                if (IsSetInsn(*tmpInsn, tmpDest, tmpSrc) && tmpDest->Equals(*srcOpnd)) {
                    DEBUG_ASSERT(tmpDest->IsRegister(), "opnd must be register");
                    RegOperand *tmpDestReg = static_cast<RegOperand *>(tmpDest);
                    if (srcReg.GetRegisterNumber() == tmpDestReg->GetRegisterNumber()) {
                        return false;
                    }
                }
            }
        }
    }

    /* add/sub insn's dest register does not exist in cmp insn. */
    if (cgFunc->GetTheCFG()->IsAddOrSubInsn(insn)) {
        RegOperand &insnDestReg = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
        if (flagOpnd) {
            RegOperand &cmpReg = static_cast<RegOperand &>(cmpInsn->GetOperand(kInsnFirstOpnd));
            if (insnDestReg.GetRegisterNumber() == cmpReg.GetRegisterNumber()) {
                return false;
            }
        } else {
            RegOperand &cmpReg1 = static_cast<RegOperand &>(cmpInsn->GetOperand(kInsnSecondOpnd));
            if (cmpInsn->GetOperand(kInsnThirdOpnd).IsRegister()) {
                RegOperand &cmpReg2 = static_cast<RegOperand &>(cmpInsn->GetOperand(kInsnThirdOpnd));
                if (insnDestReg.GetRegisterNumber() == cmpReg1.GetRegisterNumber() ||
                    insnDestReg.GetRegisterNumber() == cmpReg2.GetRegisterNumber()) {
                    return false;
                }
            } else {
                if (insnDestReg.GetRegisterNumber() == cmpReg1.GetRegisterNumber()) {
                    return false;
                }
            }
        }
    }
    return true;
}

bool AArch64ICOIfThenElsePattern::CheckCondMoveBB(BB *bb, std::map<Operand *, std::vector<Operand *>> &destSrcMap,
                                                  std::vector<Operand *> &destRegs, std::vector<Insn *> &setInsn,
                                                  Operand *flagOpnd, Insn *cmpInsn) const
{
    if (bb == nullptr) {
        return false;
    }
    FOR_BB_INSNS(insn, bb)
    {
        if (!insn->IsMachineInstruction() || insn->IsBranch()) {
            continue;
        }
        Operand *dest = nullptr;
        std::vector<Operand *> src;

        if (!IsSetInsn(*insn, dest, src)) {
            return false;
        }
        DEBUG_ASSERT(dest != nullptr, "null ptr check");
        DEBUG_ASSERT(src.size() != 0, "null ptr check");

        if (!dest->IsRegister()) {
            return false;
        }

        for (auto srcOpnd : src) {
            if (!(srcOpnd->IsConstImmediate()) && !srcOpnd->IsRegister()) {
                return false;
            }
        }

        if (flagOpnd != nullptr) {
            RegOperand *flagReg = static_cast<RegOperand *>(flagOpnd);
            regno_t flagRegNO = flagReg->GetRegisterNumber();
            if (bb->GetLiveOut()->TestBit(flagRegNO)) {
                return false;
            }
        }

        if (!CheckModifiedRegister(*insn, destSrcMap, src, *dest, cmpInsn, flagOpnd)) {
            return false;
        }

        (void)destSrcMap.insert(std::make_pair(dest, src));
        destRegs.emplace_back(dest);
        (void)setInsn.emplace_back(insn);
    }
    return true;
}

/* Convert conditional branches into cset/csel instructions */
bool AArch64ICOIfThenElsePattern::DoOpt(BB &cmpBB, BB *ifBB, BB *elseBB, BB &joinBB)
{
    Insn *condBr = cgFunc->GetTheCFG()->FindLastCondBrInsn(cmpBB);
    DEBUG_ASSERT(condBr != nullptr, "nullptr check");
    Insn *cmpInsn = FindLastCmpInsn(cmpBB);
    Operand *flagOpnd = nullptr;
    /* for cbnz and cbz institution */
    if (cgFunc->GetTheCFG()->IsCompareAndBranchInsn(*condBr)) {
        Operand &opnd0 = condBr->GetOperand(0);
        if (opnd0.IsRegister() && static_cast<RegOperand &>(opnd0).GetRegisterNumber() == RZR) {
            return false;
        }
        cmpInsn = condBr;
        flagOpnd = &(opnd0);
    }

    /* tbz will not be optimized */
    MOperator mOperator = condBr->GetMachineOpcode();
    if (mOperator == MOP_xtbz || mOperator == MOP_wtbz || mOperator == MOP_xtbnz || mOperator == MOP_wtbnz) {
        return false;
    }
    if (cmpInsn == nullptr) {
        return false;
    }

    std::vector<Operand *> ifDestRegs;
    std::vector<Insn *> ifSetInsn;
    std::vector<Operand *> elseDestRegs;
    std::vector<Insn *> elseSetInsn;

    std::map<Operand *, std::vector<Operand *>> ifDestSrcMap;
    std::map<Operand *, std::vector<Operand *>> elseDestSrcMap;

    if (!CheckCondMoveBB(elseBB, elseDestSrcMap, elseDestRegs, elseSetInsn, flagOpnd, cmpInsn) ||
        (ifBB != nullptr && !CheckCondMoveBB(ifBB, ifDestSrcMap, ifDestRegs, ifSetInsn, flagOpnd, cmpInsn))) {
        return false;
    }

    if (!CheckHasSameDest(ifSetInsn, elseSetInsn) || !CheckHasSameDest(elseSetInsn, ifSetInsn)) {
        return false;
    }

    size_t count = elseDestRegs.size();

    for (size_t i = 0; i < ifDestRegs.size(); ++i) {
        bool foundInElse = false;
        for (size_t j = 0; j < elseDestRegs.size(); ++j) {
            RegOperand *elseDestReg = static_cast<RegOperand *>(elseDestRegs[j]);
            RegOperand *ifDestReg = static_cast<RegOperand *>(ifDestRegs[i]);
            if (ifDestReg->GetRegisterNumber() == elseDestReg->GetRegisterNumber()) {
                if (cgFunc->GetTheCFG()->IsAddOrSubInsn(*ifSetInsn[i]) &&
                    cgFunc->GetTheCFG()->IsAddOrSubInsn(*elseSetInsn[j])) {
                    return false;
                }
                foundInElse = true;
                break;
            }
        }
        if (foundInElse) {
            continue;
        } else {
            ++count;
        }
    }
    if (count > kThreshold) {
        return false;
    }

    /* generate insns */
    std::vector<Insn *> elseGenerateInsn;
    std::vector<Insn *> ifGenerateInsn;
    bool elseBBProcessResult = false;
    if (elseBB != nullptr) {
        elseBBProcessResult = BuildCondMovInsn(cmpBB, *elseBB, ifDestSrcMap, elseDestSrcMap, false, elseGenerateInsn);
    }
    bool ifBBProcessResult = false;
    if (ifBB != nullptr) {
        ifBBProcessResult = BuildCondMovInsn(cmpBB, *ifBB, ifDestSrcMap, elseDestSrcMap, true, ifGenerateInsn);
    }
    if (!elseBBProcessResult || (ifBB != nullptr && !ifBBProcessResult)) {
        return false;
    }

    /* insert insn */
    if (cgFunc->GetTheCFG()->IsCompareAndBranchInsn(*condBr)) {
        Insn *innerCmpInsn = BuildCmpInsn(*condBr);
        cmpBB.InsertInsnBefore(*condBr, *innerCmpInsn);
        cmpInsn = innerCmpInsn;
    }

    if (elseBB != nullptr) {
        cmpBB.SetKind(elseBB->GetKind());
    } else {
        DEBUG_ASSERT(ifBB != nullptr, "ifBB should not be nullptr");
        cmpBB.SetKind(ifBB->GetKind());
    }

    for (auto setInsn : ifSetInsn) {
        if (cgFunc->GetTheCFG()->IsAddOrSubInsn(*setInsn)) {
            (void)cmpBB.InsertInsnBefore(*cmpInsn, *setInsn);
        }
    }

    for (auto setInsn : elseSetInsn) {
        if (cgFunc->GetTheCFG()->IsAddOrSubInsn(*setInsn)) {
            (void)cmpBB.InsertInsnBefore(*cmpInsn, *setInsn);
        }
    }

    /* delete condBr */
    cmpBB.RemoveInsn(*condBr);
    /* Insert goto insn after csel insn. */
    if (cmpBB.GetKind() == BB::kBBGoto || cmpBB.GetKind() == BB::kBBIf) {
        if (elseBB != nullptr) {
            (void)cmpBB.InsertInsnAfter(*cmpBB.GetLastInsn(), *elseBB->GetLastInsn());
        } else {
            DEBUG_ASSERT(ifBB != nullptr, "ifBB should not be nullptr");
            (void)cmpBB.InsertInsnAfter(*cmpBB.GetLastInsn(), *ifBB->GetLastInsn());
        }
    }

    /* Insert instructions in branches after cmpInsn */
    for (auto itr = elseGenerateInsn.rbegin(); itr != elseGenerateInsn.rend(); ++itr) {
        (void)cmpBB.InsertInsnAfter(*cmpInsn, **itr);
    }
    for (auto itr = ifGenerateInsn.rbegin(); itr != ifGenerateInsn.rend(); ++itr) {
        (void)cmpBB.InsertInsnAfter(*cmpInsn, **itr);
    }

    /* Remove branches and merge join */
    if (ifBB != nullptr) {
        cgFunc->GetTheCFG()->RemoveBB(*ifBB);
    }
    if (elseBB != nullptr) {
        cgFunc->GetTheCFG()->RemoveBB(*elseBB);
    }

    if (cmpBB.GetKind() != BB::kBBIf && cmpBB.GetNext() == &joinBB &&
        !maplebe::CGCFG::InLSDA(joinBB.GetLabIdx(), cgFunc->GetEHFunc()) &&
        cgFunc->GetTheCFG()->CanMerge(cmpBB, joinBB)) {
        maplebe::CGCFG::MergeBB(cmpBB, joinBB, *cgFunc);
        keepPosition = true;
    }
    return true;
}

/*
 * Find IF-THEN-ELSE or IF-THEN basic block pattern,
 * and then invoke DoOpt(...) to finish optimize.
 */
bool AArch64ICOIfThenElsePattern::Optimize(BB &curBB)
{
    if (curBB.GetKind() != BB::kBBIf) {
        return false;
    }
    BB *ifBB = nullptr;
    BB *elseBB = nullptr;
    BB *joinBB = nullptr;

    BB *thenDest = CGCFG::GetTargetSuc(curBB);
    BB *elseDest = curBB.GetNext();
    CHECK_FATAL(thenDest != nullptr, "then_dest is null in ITEPattern::Optimize");
    CHECK_FATAL(elseDest != nullptr, "else_dest is null in ITEPattern::Optimize");
    /* IF-THEN-ELSE */
    if (thenDest->NumPreds() == 1 && thenDest->NumSuccs() == 1 && elseDest->NumSuccs() == 1 &&
        elseDest->NumPreds() == 1 && thenDest->GetSuccs().front() == elseDest->GetSuccs().front()) {
        ifBB = thenDest;
        elseBB = elseDest;
        joinBB = thenDest->GetSuccs().front();
    } else if (elseDest->NumPreds() == 1 && elseDest->NumSuccs() == 1 && elseDest->GetSuccs().front() == thenDest) {
        /* IF-THEN */
        ifBB = nullptr;
        elseBB = elseDest;
        joinBB = thenDest;
    } else {
        /* not a form we can handle */
        return false;
    }
    DEBUG_ASSERT(elseBB != nullptr, "elseBB should not be nullptr");
    if (CGCFG::InLSDA(elseBB->GetLabIdx(), cgFunc->GetEHFunc()) || CGCFG::InSwitchTable(elseBB->GetLabIdx(), *cgFunc)) {
        return false;
    }

    if (ifBB != nullptr &&
        (CGCFG::InLSDA(ifBB->GetLabIdx(), cgFunc->GetEHFunc()) || CGCFG::InSwitchTable(ifBB->GetLabIdx(), *cgFunc))) {
        return false;
    }
    return DoOpt(curBB, ifBB, elseBB, *joinBB);
}

/* If( cmp || cmp ) then
 * or
 * If( cmp && cmp ) then */
bool AArch64ICOSameCondPattern::Optimize(BB &secondIfBB)
{
    if (secondIfBB.GetKind() != BB::kBBIf || secondIfBB.NumPreds() != 1) {
        return false;
    }
    BB *firstIfBB = secondIfBB.GetPrev();
    BB *nextBB = firstIfBB->GetNext();
    CHECK_FATAL(nextBB != nullptr, "nextBB is null in AArch64ICOSameCondPattern::Optimize");
    /* firstIfBB's nextBB is secondIfBB */
    if (firstIfBB == nullptr || firstIfBB->GetKind() != BB::kBBIf || nextBB->GetId() != secondIfBB.GetId()) {
        return false;
    }
    return DoOpt(firstIfBB, secondIfBB);
}

bool AArch64ICOPattern::CheckMop(MOperator mOperator) const
{
    switch (mOperator) {
        case MOP_beq:
        case MOP_bne:
        case MOP_blt:
        case MOP_ble:
        case MOP_bgt:
        case MOP_bge:
        case MOP_blo:
        case MOP_bls:
        case MOP_bhs:
        case MOP_bhi:
        case MOP_bpl:
        case MOP_bmi:
        case MOP_bvc:
        case MOP_bvs:
            return true;
        default:
            return false;
    }
}

/* branchInsn1 is firstIfBB's LastCondBrInsn
 * branchInsn2 is secondIfBB's LastCondBrInsn
 *
 * Limitations: branchInsn1 is the same as branchInsn2
 * */
bool AArch64ICOSameCondPattern::DoOpt(BB *firstIfBB, BB &secondIfBB)
{
    Insn *branchInsn1 = cgFunc->GetTheCFG()->FindLastCondBrInsn(*firstIfBB);
    DEBUG_ASSERT(branchInsn1 != nullptr, "nullptr check");
    Insn *cmpInsn1 = FindLastCmpInsn(*firstIfBB);
    MOperator mOperator1 = branchInsn1->GetMachineOpcode();
    Insn *branchInsn2 = cgFunc->GetTheCFG()->FindLastCondBrInsn(secondIfBB);
    DEBUG_ASSERT(branchInsn2 != nullptr, "nullptr check");
    Insn *cmpInsn2 = FindLastCmpInsn(secondIfBB);
    MOperator mOperator2 = branchInsn2->GetMachineOpcode();
    if (cmpInsn1 == nullptr || cmpInsn2 == nullptr) {
        return false;
    }

    /* tbz and cbz will not be optimized */
    if (mOperator1 != mOperator2 || !CheckMop(mOperator1)) {
        return false;
    }

    /* two BB has same branch */
    std::vector<LabelOperand *> labelOpnd1 = GetLabelOpnds(*branchInsn1);
    std::vector<LabelOperand *> labelOpnd2 = GetLabelOpnds(*branchInsn2);
    if (labelOpnd1.size() != 1 || labelOpnd1.size() != 1 ||
        labelOpnd1[0]->GetLabelIndex() != labelOpnd2[0]->GetLabelIndex()) {
        return false;
    }

    /* secondifBB only has branchInsn and cmpInsn */
    FOR_BB_INSNS_REV(insn, &secondIfBB)
    {
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        if (insn != branchInsn2 && insn != cmpInsn2) {
            return false;
        }
    }

    /* build ccmp Insn */
    ConditionCode ccCode = Encode(branchInsn1->GetMachineOpcode(), true);
    DEBUG_ASSERT(ccCode != kCcLast, "unknown cond, ccCode can't be kCcLast");
    Insn *ccmpInsn = BuildCcmpInsn(ccCode, cmpInsn2);
    if (ccmpInsn == nullptr) {
        return false;
    }

    /* insert ccmp Insn */
    firstIfBB->InsertInsnBefore(*branchInsn1, *ccmpInsn);

    /* Remove secondIfBB */
    BB *nextBB = secondIfBB.GetNext();
    cgFunc->GetTheCFG()->RemoveBB(secondIfBB);
    firstIfBB->PushFrontSuccs(*nextBB);
    nextBB->PushFrontPreds(*firstIfBB);
    return true;
}
/*
 * find the preds all is ifBB
 */
bool AArch64ICOMorePredsPattern::Optimize(BB &curBB)
{
    if (curBB.GetKind() != BB::kBBGoto) {
        return false;
    }
    for (BB *preBB : curBB.GetPreds()) {
        if (preBB->GetKind() != BB::kBBIf) {
            return false;
        }
    }
    for (BB *succsBB : curBB.GetSuccs()) {
        if (succsBB->GetKind() != BB::kBBFallthru) {
            return false;
        }
        if (succsBB->NumPreds() > k2BitSize) {
            return false;
        }
    }
    Insn *gotoBr = curBB.GetLastMachineInsn();
    DEBUG_ASSERT(gotoBr != nullptr, "gotoBr should not be nullptr");
    CHECK_FATAL(gotoBr->GetOperandSize() > 1 && gotoBr->GetOperandSize()  < UINT32_MAX, "value overflow");
    auto &gotoLabel = static_cast<LabelOperand &>(gotoBr->GetOperand(gotoBr->GetOperandSize() - 1));
    for (BB *preBB : curBB.GetPreds()) {
        Insn *condBr = cgFunc->GetTheCFG()->FindLastCondBrInsn(*preBB);
        DEBUG_ASSERT(condBr != nullptr, "nullptr check");
        CHECK_FATAL(condBr->GetOperandSize() > 1 && condBr->GetOperandSize()  < UINT32_MAX, "value overflow");
        Operand &condBrLastOpnd = condBr->GetOperand(condBr->GetOperandSize() - 1);
        DEBUG_ASSERT(condBrLastOpnd.IsLabelOpnd(), "label Operand must be exist in branch insn");
        auto &labelOpnd = static_cast<LabelOperand &>(condBrLastOpnd);
        if (labelOpnd.GetLabelIndex() != curBB.GetLabIdx()) {
            return false;
        }
        if (gotoLabel.GetLabelIndex() != preBB->GetNext()->GetLabIdx()) {
            /* do not if convert if 'else' clause present */
            return false;
        }
    }
    return DoOpt(curBB);
}

/* this BBGoto only has mov Insn and Branch */
bool AArch64ICOMorePredsPattern::CheckGotoBB(BB &gotoBB, std::vector<Insn *> &movInsn) const
{
    FOR_BB_INSNS(insn, &gotoBB)
    {
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        if (insn->IsMove()) {
            movInsn.push_back(insn);
            continue;
        }
        if (insn->GetId() != gotoBB.GetLastInsn()->GetId()) {
            return false;
        } else if (!insn->IsBranch()) { /* last Insn is Branch */
            return false;
        }
    }
    return true;
}

/* this BBGoto only has mov Insn */
bool AArch64ICOMorePredsPattern::MovToCsel(std::vector<Insn *> &movInsn, std::vector<Insn *> &cselInsn,
                                           const Insn &branchInsn) const
{
    Operand &branchOpnd0 = branchInsn.GetOperand(kInsnFirstOpnd);
    regno_t branchRegNo;
    if (branchOpnd0.IsRegister()) {
        branchRegNo = static_cast<RegOperand &>(branchOpnd0).GetRegisterNumber();
    }
    for (Insn *insn : movInsn) {
        /* use mov build csel */
        Operand &opnd0 = insn->GetOperand(kInsnFirstOpnd);
        Operand &opnd1 = insn->GetOperand(kInsnSecondOpnd);
        ConditionCode ccCode = AArch64ICOPattern::Encode(branchInsn.GetMachineOpcode(), false);
        DEBUG_ASSERT(ccCode != kCcLast, "unknown cond, ccCode can't be kCcLast");
        CondOperand &cond = static_cast<AArch64CGFunc *>(cgFunc)->GetCondOperand(ccCode);
        Operand &rflag = static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreateRflag();
        RegOperand &regOpnd0 = static_cast<RegOperand &>(opnd0);
        RegOperand &regOpnd1 = static_cast<RegOperand &>(opnd1);
        /* movInsn's opnd1 is Immediate */
        if (opnd1.IsImmediate()) {
            return false;
        }
        /* opnd0 and opnd1 hsa same type and size */
        if (regOpnd0.GetSize() != regOpnd1.GetSize() || (regOpnd0.IsOfIntClass() != regOpnd1.IsOfIntClass())) {
            return false;
        }
        /* The branchOpnd0 cannot be modified for csel. */
        regno_t movRegNo0 = static_cast<RegOperand &>(opnd0).GetRegisterNumber();
        if (branchOpnd0.IsRegister() && branchRegNo == movRegNo0) {
            return false;
        }
        uint32 dSize = regOpnd0.GetSize();
        bool isIntTy = regOpnd0.IsOfIntClass();
        MOperator mOpCode =
            isIntTy ? (dSize == k64BitSize ? MOP_xcselrrrc : MOP_wcselrrrc)
                    : (dSize == k64BitSize ? MOP_dcselrrrc : (dSize == k32BitSize ? MOP_scselrrrc : MOP_hcselrrrc));
        cselInsn.emplace_back(&cgFunc->GetInsnBuilder()->BuildInsn(mOpCode, opnd0, opnd1, opnd0, cond, rflag));
    }
    if (cselInsn.size() < 1) {
        return false;
    }
    return true;
}

bool AArch64ICOMorePredsPattern::DoOpt(BB &gotoBB)
{
    std::vector<Insn *> movInsn;
    std::vector<std::vector<Insn *>> presCselInsn;
    std::vector<BB *> presBB;
    Insn *branchInsn = gotoBB.GetLastMachineInsn();
    if (branchInsn == nullptr || !branchInsn->IsUnCondBranch()) {
        return false;
    }
    /* get preds's new label */
    std::vector<LabelOperand *> labelOpnd = GetLabelOpnds(*branchInsn);
    if (labelOpnd.size() != 1) {
        return false;
    }
    if (!CheckGotoBB(gotoBB, movInsn)) {
        return false;
    }
    /* Check all preBB, Exclude gotoBBs that cannot be optimized. */
    for (BB *preBB : gotoBB.GetPreds()) {
        Insn *condBr = cgFunc->GetTheCFG()->FindLastCondBrInsn(*preBB);
        DEBUG_ASSERT(condBr != nullptr, "nullptr check");

        /* tbz/cbz will not be optimized */
        MOperator mOperator = condBr->GetMachineOpcode();
        if (!CheckMop(mOperator)) {
            return false;
        }
        std::vector<Insn *> cselInsn;
        if (!MovToCsel(movInsn, cselInsn, *condBr)) {
            return false;
        }
        if (cselInsn.size() < 1) {
            return false;
        }
        presCselInsn.emplace_back(cselInsn);
        presBB.emplace_back(preBB);
    }
    /* modifies presBB */
    for (size_t i = 0; i < presCselInsn.size(); ++i) {
        BB *preBB = presBB[i];
        Insn *condBr = cgFunc->GetTheCFG()->FindLastCondBrInsn(*preBB);
        std::vector<Insn *> cselInsn = presCselInsn[i];
        /* insert csel insn */
        for (Insn *csel : cselInsn) {
            preBB->InsertInsnBefore(*condBr, *csel);
        }
        /* new condBr */
        CHECK_FATAL(condBr->GetOperandSize() > 1 && condBr->GetOperandSize() < UINT32_MAX, "value overflow");
        condBr->SetOperand(condBr->GetOperandSize() - 1, *labelOpnd[0]);
    }
    /* Remove branches and merge gotoBB */
    cgFunc->GetTheCFG()->RemoveBB(gotoBB);
    return true;
}

} /* namespace maplebe */
