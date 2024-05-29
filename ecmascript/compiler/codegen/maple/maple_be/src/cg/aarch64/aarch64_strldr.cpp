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

#include "aarch64_strldr.h"
#include "aarch64_reaching.h"
#include "aarch64_cgfunc.h"
#include "common_utils.h"

namespace maplebe {
using namespace maple;

static MOperator SelectMovMop(bool isFloatOrSIMD, bool is64Bit)
{
    return isFloatOrSIMD ? (is64Bit ? MOP_xvmovd : MOP_xvmovs) : (is64Bit ? MOP_xmovrr : MOP_wmovrr);
}

void AArch64StoreLoadOpt::Run()
{
    DoStoreLoadOpt();
}

void AArch64StoreLoadOpt::GenerateMoveLiveInsn(RegOperand &resRegOpnd, RegOperand &srcRegOpnd, Insn &ldrInsn,
                                               Insn &strInsn, short memSeq)
{
    MOperator movMop = SelectMovMop(resRegOpnd.IsOfFloatOrSIMDClass(), resRegOpnd.GetSize() == k64BitSize);
    Insn *movInsn = nullptr;
    if (str2MovMap[&strInsn][memSeq] != nullptr && !cgFunc.IsAfterRegAlloc()) {
        Insn *movInsnOfStr = str2MovMap[&strInsn][memSeq];
        auto &vregOpnd = static_cast<RegOperand &>(movInsnOfStr->GetOperand(kInsnFirstOpnd));
        movInsn = &cgFunc.GetInsnBuilder()->BuildInsn(movMop, resRegOpnd, vregOpnd);
    } else {
        movInsn = &cgFunc.GetInsnBuilder()->BuildInsn(movMop, resRegOpnd, srcRegOpnd);
    }
    if (&resRegOpnd == &srcRegOpnd && cgFunc.IsAfterRegAlloc()) {
        ldrInsn.GetBB()->RemoveInsn(ldrInsn);
        cgFunc.GetRD()->InitGenUse(*ldrInsn.GetBB(), false);
        return;
    }
    movInsn->SetId(ldrInsn.GetId());
    ldrInsn.GetBB()->ReplaceInsn(ldrInsn, *movInsn);
    if (CG_DEBUG_FUNC(cgFunc)) {
        LogInfo::MapleLogger() << "replace ldrInsn:\n";
        ldrInsn.Dump();
        LogInfo::MapleLogger() << "with movInsn:\n";
        movInsn->Dump();
    }
    /* Add comment. */
    MapleString newComment = ldrInsn.GetComment();
    if (strInsn.IsStorePair()) {
        newComment += ";  stp-load live version.";
    } else {
        newComment += ";  str-load live version.";
    }
    movInsn->SetComment(newComment);
    cgFunc.GetRD()->InitGenUse(*ldrInsn.GetBB(), false);
}

void AArch64StoreLoadOpt::GenerateMoveDeadInsn(RegOperand &resRegOpnd, RegOperand &srcRegOpnd, Insn &ldrInsn,
                                               Insn &strInsn, short memSeq)
{
    Insn *newMovInsn = nullptr;
    RegOperand *vregOpnd = nullptr;

    if (str2MovMap[&strInsn][memSeq] == nullptr) {
        RegType regTy = srcRegOpnd.IsOfFloatOrSIMDClass() ? kRegTyFloat : kRegTyInt;
        regno_t vRegNO = cgFunc.NewVReg(regTy, srcRegOpnd.GetSize() <= k32BitSize ? k4ByteSize : k8ByteSize);
        /* generate a new vreg, check if the size of DataInfo is big enough */
        if (vRegNO >= cgFunc.GetRD()->GetRegSize(*strInsn.GetBB())) {
            cgFunc.GetRD()->EnlargeRegCapacity(vRegNO);
        }
        vregOpnd = &cgFunc.CreateVirtualRegisterOperand(vRegNO);
        MOperator newMop = SelectMovMop(resRegOpnd.IsOfFloatOrSIMDClass(), resRegOpnd.GetSize() == k64BitSize);
        newMovInsn = &cgFunc.GetInsnBuilder()->BuildInsn(newMop, *vregOpnd, srcRegOpnd);
        newMovInsn->SetId(strInsn.GetId() + memSeq + 1);
        strInsn.GetBB()->InsertInsnAfter(strInsn, *newMovInsn);
        str2MovMap[&strInsn][memSeq] = newMovInsn;
        /* update DataInfo */
        cgFunc.GetRD()->UpdateInOut(*strInsn.GetBB(), true);
    } else {
        newMovInsn = str2MovMap[&strInsn][memSeq];
        vregOpnd = &static_cast<RegOperand &>(newMovInsn->GetOperand(kInsnFirstOpnd));
    }
    MOperator movMop = SelectMovMop(resRegOpnd.IsOfFloatOrSIMDClass(), resRegOpnd.GetSize() == k64BitSize);
    Insn &movInsn = cgFunc.GetInsnBuilder()->BuildInsn(movMop, resRegOpnd, *vregOpnd);
    movInsn.SetId(ldrInsn.GetId());
    ldrInsn.GetBB()->ReplaceInsn(ldrInsn, movInsn);
    if (CG_DEBUG_FUNC(cgFunc)) {
        LogInfo::MapleLogger() << "replace ldrInsn:\n";
        ldrInsn.Dump();
        LogInfo::MapleLogger() << "with movInsn:\n";
        movInsn.Dump();
    }

    /* Add comment. */
    MapleString newComment = ldrInsn.GetComment();
    if (strInsn.IsStorePair()) {
        newComment += ";  stp-load die version.";
    } else {
        newComment += ";  str-load die version.";
    }
    movInsn.SetComment(newComment);
    cgFunc.GetRD()->InitGenUse(*ldrInsn.GetBB(), false);
}

bool AArch64StoreLoadOpt::HasMemBarrier(const Insn &ldrInsn, const Insn &strInsn) const
{
    if (!cgFunc.GetMirModule().IsCModule()) {
        return false;
    }
    const Insn *currInsn = strInsn.GetNext();
    while (currInsn != &ldrInsn) {
        if (currInsn == nullptr) {
            return false;
        }
        if (currInsn->IsMachineInstruction() && currInsn->IsCall()) {
            return true;
        }
        currInsn = currInsn->GetNext();
    }
    return false;
}

/*
 * Transfer: store wzr, [MEM]
 *           ... // May exist branches.
 *           load  x200, [MEM]
 *        ==>
 *        OPT_VERSION_STP_ZERO / OPT_VERSION_STR_ZERO:
 *            store wzr, [MEM]
 *            ... // May exist branches. if x100 not dead here.
 *            mov   x200, wzr
 *
 *  Params:
 *    stInsn: indicate store insn.
 *    strSrcIdx: index of source register operand of store insn. (wzr in this example)
 *    memUseInsnSet: insns using memOperand
 */
void AArch64StoreLoadOpt::DoLoadZeroToMoveTransfer(const Insn &strInsn, short strSrcIdx,
                                                   const InsnSet &memUseInsnSet) const
{
    /* comment for strInsn should be only added once */
    for (auto *ldrInsn : memUseInsnSet) {
        /* Currently we don't support useInsn is ldp insn. */
        if (!ldrInsn->IsLoad() || ldrInsn->GetDefRegs().size() > 1) {
            continue;
        }
        if (HasMemBarrier(*ldrInsn, strInsn)) {
            continue;
        }
        /* ldr reg, [mem], the index of [mem] is 1 */
        InsnSet defInsnForUseInsns = cgFunc.GetRD()->FindDefForMemOpnd(*ldrInsn, 1);
        /* If load has multiple definition, continue. */
        if (defInsnForUseInsns.size() > 1) {
            continue;
        }

        auto &resOpnd = ldrInsn->GetOperand(0);
        auto &srcOpnd = strInsn.GetOperand(static_cast<uint32>(strSrcIdx));

        if (resOpnd.GetSize() != srcOpnd.GetSize()) {
            return;
        }
        RegOperand &resRegOpnd = static_cast<RegOperand &>(resOpnd);
        MOperator movMop = SelectMovMop(resRegOpnd.IsOfFloatOrSIMDClass(), resRegOpnd.GetSize() == k64BitSize);
        Insn &movInsn = cgFunc.GetInsnBuilder()->BuildInsn(movMop, resOpnd, srcOpnd);
        movInsn.SetId(ldrInsn->GetId());
        ldrInsn->GetBB()->ReplaceInsn(*ldrInsn, movInsn);

        /* Add comment. */
        MapleString newComment = ldrInsn->GetComment();
        newComment += ",  str-load zero version";
        movInsn.SetComment(newComment);
    }
}

bool AArch64StoreLoadOpt::CheckStoreOpCode(MOperator opCode) const
{
    switch (opCode) {
        case MOP_wstr:
        case MOP_xstr:
        case MOP_sstr:
        case MOP_dstr:
        case MOP_wstp:
        case MOP_xstp:
        case MOP_sstp:
        case MOP_dstp:
        case MOP_wstrb:
        case MOP_wstrh:
            return true;
        default:
            return false;
    }
}

void AArch64StoreLoadOpt::MemPropInit()
{
    propMode = kUndef;
    amount = 0;
    removeDefInsn = false;
}

bool AArch64StoreLoadOpt::CheckReplaceReg(Insn &defInsn, Insn &currInsn, InsnSet &replaceRegDefSet,
                                          regno_t replaceRegNo)
{
    if (replaceRegDefSet.empty()) {
        return true;
    }
    if (defInsn.GetBB() == currInsn.GetBB()) {
        /* check replace reg def between defInsn and currInsn */
        Insn *tmpInsn = defInsn.GetNext();
        while (tmpInsn != nullptr && tmpInsn != &currInsn) {
            if (replaceRegDefSet.find(tmpInsn) != replaceRegDefSet.end()) {
                return false;
            }
            tmpInsn = tmpInsn->GetNext();
        }
    } else {
        regno_t defRegno = static_cast<RegOperand &>(defInsn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
        if (defRegno == replaceRegNo) {
            auto *defLoop = loopInfo.GetBBLoopParent(defInsn.GetBB()->GetId());
            auto defLoopId = defLoop ? defLoop->GetHeader().GetId() : 0;
            auto *curLoop = loopInfo.GetBBLoopParent(currInsn.GetBB()->GetId());
            auto curLoopId = curLoop ? curLoop->GetHeader().GetId() : 0;
            if (defLoopId != curLoopId) {
                return false;
            }
        }
        AArch64ReachingDefinition *a64RD = static_cast<AArch64ReachingDefinition *>(cgFunc.GetRD());
        if (a64RD->HasRegDefBetweenInsnGlobal(replaceRegNo, defInsn, currInsn)) {
            return false;
        }
    }

    if (replaceRegDefSet.size() == 1 && *replaceRegDefSet.begin() == &defInsn) {
        /* lsl x1, x1, #3    <-----should be removed after replace MemOperand of ldrInsn.
         * ldr x0, [x0,x1]   <-----should be single useInsn for x1
         */
        InsnSet newRegUseSet = cgFunc.GetRD()->FindUseForRegOpnd(defInsn, replaceRegNo, true);
        if (newRegUseSet.size() != k1BitSize) {
            return false;
        }
        removeDefInsn = true;
    }
    return true;
}

bool AArch64StoreLoadOpt::CheckDefInsn(Insn &defInsn, Insn &currInsn)
{
    if (defInsn.GetOperandSize() < k2ByteSize) {
        return false;
    }
    for (uint32 i = kInsnSecondOpnd; i < defInsn.GetOperandSize(); i++) {
        Operand &opnd = defInsn.GetOperand(i);
        if (defInsn.IsMove() && opnd.IsRegister() && !cgFunc.IsSPOrFP(static_cast<RegOperand &>(opnd))) {
            return false;
        }
        if (opnd.IsRegister()) {
            RegOperand &a64OpndTmp = static_cast<RegOperand &>(opnd);
            regno_t replaceRegNo = a64OpndTmp.GetRegisterNumber();
            InsnSet newRegDefSet = cgFunc.GetRD()->FindDefForRegOpnd(currInsn, replaceRegNo, true);
            if (!CheckReplaceReg(defInsn, currInsn, newRegDefSet, replaceRegNo)) {
                return false;
            }
        }
    }
    return true;
}

bool AArch64StoreLoadOpt::CheckNewAmount(const Insn &insn, uint32 newAmount)
{
    MOperator mOp = insn.GetMachineOpcode();
    switch (mOp) {
        case MOP_wstrb:
        case MOP_wldrsb:
        case MOP_xldrsb:
        case MOP_wldrb: {
            return newAmount == 0;
        }
        case MOP_wstrh:
        case MOP_wldrsh:
        case MOP_xldrsh:
        case MOP_wldrh: {
            return (newAmount == 0) || (newAmount == k1BitSize);
        }
        case MOP_wstr:
        case MOP_sstr:
        case MOP_wldr:
        case MOP_sldr:
        case MOP_xldrsw: {
            return (newAmount == 0) || (newAmount == k2BitSize);
        }
        case MOP_qstr:
        case MOP_qldr: {
            return (newAmount == 0) || (newAmount == k4BitSize);
        }
        default: {
            return (newAmount == 0) || (newAmount == k3ByteSize);
        }
    }
}

bool AArch64StoreLoadOpt::CheckNewMemOffset(const Insn &insn, MemOperand *newMemOpnd, uint32 opndIdx)
{
    AArch64CGFunc &a64CgFunc = static_cast<AArch64CGFunc &>(cgFunc);
    if ((newMemOpnd->GetOffsetImmediate() != nullptr) &&
        !a64CgFunc.IsOperandImmValid(insn.GetMachineOpcode(), newMemOpnd, opndIdx)) {
        return false;
    }
    auto newAmount = newMemOpnd->ShiftAmount();
    if (!CheckNewAmount(insn, newAmount)) {
        return false;
    }
    /* is ldp or stp, addrMode must be BOI */
    if ((opndIdx == kInsnThirdOpnd) && (newMemOpnd->GetAddrMode() != MemOperand::kAddrModeBOi)) {
        return false;
    }
    return true;
}

MemOperand *AArch64StoreLoadOpt::SelectReplaceExt(const Insn &defInsn, RegOperand &base, bool isSigned)
{
    MemOperand *newMemOpnd = nullptr;
    RegOperand *newOffset = static_cast<RegOperand *>(&defInsn.GetOperand(kInsnSecondOpnd));
    CHECK_FATAL(newOffset != nullptr, "newOffset is null!");
    /* defInsn is extend, currMemOpnd is same extend or shift */
    bool propExtend = (propMode == kPropShift) || ((propMode == kPropSignedExtend) && isSigned) ||
                      ((propMode == kPropUnsignedExtend) && !isSigned);
    if (propMode == kPropOffset) {
        newMemOpnd = static_cast<AArch64CGFunc &>(cgFunc).CreateMemOperand(MemOperand::kAddrModeBOrX, k64BitSize, base,
                                                                           *newOffset, 0, isSigned);
    } else if (propExtend) {
        newMemOpnd = static_cast<AArch64CGFunc &>(cgFunc).CreateMemOperand(MemOperand::kAddrModeBOrX, k64BitSize, base,
                                                                           *newOffset, amount, isSigned);
    } else {
        return nullptr;
    }
    return newMemOpnd;
}

MemOperand *AArch64StoreLoadOpt::HandleArithImmDef(RegOperand &replace, Operand *oldOffset, int64 defVal)
{
    if (propMode != kPropBase) {
        return nullptr;
    }
    OfstOperand *newOfstImm = nullptr;
    if (oldOffset == nullptr) {
        newOfstImm = &static_cast<AArch64CGFunc &>(cgFunc).CreateOfstOpnd(static_cast<uint64>(defVal), k32BitSize);
    } else {
        auto *ofstOpnd = static_cast<OfstOperand *>(oldOffset);
        CHECK_FATAL(ofstOpnd != nullptr, "oldOffsetOpnd is null");
        newOfstImm = &static_cast<AArch64CGFunc &>(cgFunc).CreateOfstOpnd(
            static_cast<uint64>(defVal + ofstOpnd->GetValue()), k32BitSize);
    }
    CHECK_FATAL(newOfstImm != nullptr, "newOffset is null!");
    return static_cast<AArch64CGFunc &>(cgFunc).CreateMemOperand(MemOperand::kAddrModeBOi, k64BitSize, replace, nullptr,
                                                                 newOfstImm, nullptr);
}

/*
 * limit to adjacent bb to avoid ra spill.
 */
bool AArch64StoreLoadOpt::IsAdjacentBB(Insn &defInsn, Insn &curInsn) const
{
    if (defInsn.GetBB() == curInsn.GetBB()) {
        return true;
    }
    for (auto *bb : defInsn.GetBB()->GetSuccs()) {
        if (bb == curInsn.GetBB()) {
            return true;
        }
        if (bb->IsSoloGoto()) {
            BB *tragetBB = CGCFG::GetTargetSuc(*bb);
            if (tragetBB == curInsn.GetBB()) {
                return true;
            }
        }
    }
    return false;
}

bool AArch64StoreLoadOpt::CanDoMemProp(const Insn *insn)
{
    if (!cgFunc.GetMirModule().IsCModule()) {
        return false;
    }
    if (!insn->IsMachineInstruction()) {
        return false;
    }
    if (insn->GetMachineOpcode() == MOP_qstr) {
        return false;
    }

    if (insn->IsLoad() || insn->IsStore()) {
        if (insn->IsAtomic()) {
            return false;
        }
        // It is not desired to propagate on 128bit reg with immediate offset
        // which may cause linker to issue misalignment error
        if (insn->IsAtomic() || insn->GetOperand(0).GetSize() == k128BitSize) {
            return false;
        }
        MemOperand *currMemOpnd = static_cast<MemOperand *>(insn->GetMemOpnd());
        return currMemOpnd != nullptr;
    }
    return false;
}

void AArch64StoreLoadOpt::SelectPropMode(const MemOperand &currMemOpnd)
{
    MemOperand::AArch64AddressingMode currAddrMode = currMemOpnd.GetAddrMode();
    switch (currAddrMode) {
        case MemOperand::kAddrModeBOi: {
            if (!currMemOpnd.IsPreIndexed() && !currMemOpnd.IsPostIndexed()) {
                propMode = kPropBase;
            }
            break;
        }
        case MemOperand::kAddrModeBOrX: {
            propMode = kPropOffset;
            amount = currMemOpnd.ShiftAmount();
            if (currMemOpnd.GetExtendAsString() == "LSL") {
                if (amount != 0) {
                    propMode = kPropShift;
                }
                break;
            } else if (currMemOpnd.SignedExtend()) {
                propMode = kPropSignedExtend;
            } else if (currMemOpnd.UnsignedExtend()) {
                propMode = kPropUnsignedExtend;
            }
            break;
        }
        default:
            propMode = kUndef;
    }
}

/*
 * Optimize: store x100, [MEM]
 *           ... // May exist branches.
 *           load  x200, [MEM]
 *        ==>
 *        OPT_VERSION_STP_LIVE / OPT_VERSION_STR_LIVE:
 *           store x100, [MEM]
 *           ... // May exist branches. if x100 not dead here.
 *           mov   x200, x100
 *        OPT_VERSION_STP_DIE / OPT_VERSION_STR_DIE:
 *           store x100, [MEM]
 *           mov x9000(new reg), x100
 *           ... // May exist branches. if x100 dead here.
 *           mov   x200, x9000
 *
 *  Note: x100 may be wzr/xzr registers.
 */
void AArch64StoreLoadOpt::DoStoreLoadOpt()
{
    AArch64CGFunc &a64CgFunc = static_cast<AArch64CGFunc &>(cgFunc);
    if (a64CgFunc.IsIntrnCallForC()) {
        return;
    }
    FOR_ALL_BB(bb, &a64CgFunc) {
        FOR_BB_INSNS_SAFE(insn, bb, next) {
            MOperator mOp = insn->GetMachineOpcode();
            if (CanDoMemProp(insn)) {
                MemProp(*insn);
            }
            if (a64CgFunc.GetMirModule().IsCModule() && cgFunc.GetRD()->OnlyAnalysisReg()) {
                continue;
            }
            if (!insn->IsMachineInstruction() || !insn->IsStore() || !CheckStoreOpCode(mOp) ||
                (a64CgFunc.GetMirModule().IsCModule() && !a64CgFunc.IsAfterRegAlloc()) ||
                (!a64CgFunc.GetMirModule().IsCModule() && a64CgFunc.IsAfterRegAlloc())) {
                continue;
            }
            if (insn->IsStorePair()) {
                ProcessStrPair(*insn);
                continue;
            }
            ProcessStr(*insn);
        }
    }
}

/*
 * PropBase:
 *   add/sub x1, x2, #immVal1
 *   ...(no def of x2)
 *   ldr/str x0, [x1, #immVal2]
 *   ======>
 *   add/sub x1, x2, #immVal1
 *   ...
 *   ldr/str x0, [x2, #(immVal1 + immVal2)/#(-immVal1 + immVal2)]
 *
 * PropOffset:
 *   sxtw x2, w2
 *   lsl x1, x2, #1~3
 *   ...(no def of x2)
 *   ldr/str x0, [x0, x1]
 *   ======>
 *   sxtw x2, w2
 *   lsl x1, x2, #1~3
 *   ...
 *   ldr/str x0, [x0, w2, sxtw 1~3]
 */
void AArch64StoreLoadOpt::MemProp(Insn &insn)
{
    MemPropInit();
    MemOperand *currMemOpnd = static_cast<MemOperand *>(insn.GetMemOpnd());
    SelectPropMode(*currMemOpnd);
    bool memReplaced = false;

    if (propMode == kUndef) {
        return;
    }

    /* if prop success, find more prop chance */
    if (memReplaced) {
        MemProp(insn);
    }
}

/*
 * Assume stack(FP) will not be varied out of pro/epi log
 * PreIndex:
 *   add/sub x1, x1 #immVal1
 *   ...(no def/use of x1)
 *   ldr/str x0, [x1]
 *   ======>
 *   ldr/str x0, [x1, #immVal1]!
 *
 * PostIndex:
 *   ldr/str x0, [x1]
 *   ...(no def/use of x1)
 *   add/sub x1, x1, #immVal1
 *   ======>
 *   ldr/str x0, [x1],  #immVal1
 */
void AArch64StoreLoadOpt::StrLdrIndexModeOpt(Insn &currInsn)
{
    auto *curMemopnd = static_cast<MemOperand *>(currInsn.GetMemOpnd());
    DEBUG_ASSERT(curMemopnd != nullptr, " get memopnd failed");
    /* one instruction cannot define one register twice */
    if (!CanDoIndexOpt(*curMemopnd) || currInsn.IsRegDefined(curMemopnd->GetBaseRegister()->GetRegisterNumber())) {
        return;
    }
    MemOperand *newMemopnd = SelectIndexOptMode(currInsn, *curMemopnd);
    if (newMemopnd != nullptr) {
        currInsn.SetMemOpnd(newMemopnd);
    }
}

bool AArch64StoreLoadOpt::CanDoIndexOpt(const MemOperand &MemOpnd)
{
    if (MemOpnd.GetAddrMode() != MemOperand::kAddrModeBOi || !MemOpnd.IsIntactIndexed()) {
        return false;
    }
    DEBUG_ASSERT(MemOpnd.GetOffsetImmediate() != nullptr, " kAddrModeBOi memopnd have no offset imm");
    if (!MemOpnd.GetOffsetImmediate()->IsImmOffset()) {
        return false;
    }
    if (cgFunc.IsSPOrFP(*MemOpnd.GetBaseRegister())) {
        return false;
    }
    OfstOperand *a64Ofst = MemOpnd.GetOffsetImmediate();
    if (a64Ofst == nullptr) {
        return false;
    }
    return a64Ofst->GetValue() == 0;
}

int64 AArch64StoreLoadOpt::GetOffsetForNewIndex(Insn &defInsn, Insn &insn, regno_t baseRegNO, uint32 memOpndSize)
{
    bool subMode = defInsn.GetMachineOpcode() == MOP_wsubrri12 || defInsn.GetMachineOpcode() == MOP_xsubrri12;
    bool addMode = defInsn.GetMachineOpcode() == MOP_waddrri12 || defInsn.GetMachineOpcode() == MOP_xaddrri12;
    if (addMode || subMode) {
        DEBUG_ASSERT(static_cast<RegOperand &>(defInsn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber() == baseRegNO,
                     "check def opnd");
        auto &srcOpnd = static_cast<RegOperand &>(defInsn.GetOperand(kInsnSecondOpnd));
        if (srcOpnd.GetRegisterNumber() == baseRegNO && defInsn.GetBB() == insn.GetBB()) {
            int64 offsetVal = static_cast<ImmOperand &>(defInsn.GetOperand(kInsnThirdOpnd)).GetValue();
            if (!MemOperand::IsSIMMOffsetOutOfRange(offsetVal, memOpndSize == k64BitSize, insn.IsLoadStorePair())) {
                return subMode ? -offsetVal : offsetVal;
            }
        }
    }
    return kMaxPimm8; /* simm max value cannot excced pimm max value */
};

MemOperand *AArch64StoreLoadOpt::SelectIndexOptMode(Insn &insn, const MemOperand &curMemOpnd)
{
    AArch64ReachingDefinition *a64RD = static_cast<AArch64ReachingDefinition *>(cgFunc.GetRD());
    DEBUG_ASSERT((a64RD != nullptr), "check a64RD!");
    regno_t baseRegisterNO = curMemOpnd.GetBaseRegister()->GetRegisterNumber();
    auto &a64cgFunc = static_cast<AArch64CGFunc &>(cgFunc);
    /* pre index */
    InsnSet regDefSet = a64RD->FindDefForRegOpnd(insn, baseRegisterNO, true);
    if (regDefSet.size() == k1BitSize) {
        Insn *defInsn = *regDefSet.begin();
        int64 defOffset = GetOffsetForNewIndex(*defInsn, insn, baseRegisterNO, curMemOpnd.GetSize());
        if (defOffset < kMaxPimm8) {
            InsnSet tempCheck;
            (void)a64RD->FindRegUseBetweenInsn(baseRegisterNO, defInsn->GetNext(), insn.GetPrev(), tempCheck);
            if (tempCheck.empty() && (defInsn->GetBB() == insn.GetBB())) {
                auto &newMem = a64cgFunc.CreateMemOpnd(*curMemOpnd.GetBaseRegister(), defOffset, curMemOpnd.GetSize());
                DEBUG_ASSERT(newMem.GetOffsetImmediate() != nullptr, "need offset for memopnd in this case");
                newMem.SetIndexOpt(MemOperand::kPreIndex);
                insn.GetBB()->RemoveInsn(*defInsn);
                return &newMem;
            }
        }
    }
    /* post index */
    std::vector<Insn *> refDefVec =
        a64RD->FindRegDefBetweenInsn(baseRegisterNO, &insn, insn.GetBB()->GetLastInsn(), true);
    if (!refDefVec.empty()) {
        Insn *defInsn = refDefVec.back();
        int64 defOffset = GetOffsetForNewIndex(*defInsn, insn, baseRegisterNO, curMemOpnd.GetSize());
        if (defOffset < kMaxPimm8) {
            InsnSet tempCheck;
            (void)a64RD->FindRegUseBetweenInsn(baseRegisterNO, insn.GetNext(), defInsn->GetPrev(), tempCheck);
            if (tempCheck.empty() && (defInsn->GetBB() == insn.GetBB())) {
                auto &newMem = a64cgFunc.CreateMemOpnd(*curMemOpnd.GetBaseRegister(), defOffset, curMemOpnd.GetSize());
                DEBUG_ASSERT(newMem.GetOffsetImmediate() != nullptr, "need offset for memopnd in this case");
                newMem.SetIndexOpt(MemOperand::kPostIndex);
                insn.GetBB()->RemoveInsn(*defInsn);
                return &newMem;
            }
        }
    }
    return nullptr;
}

void AArch64StoreLoadOpt::ProcessStrPair(Insn &insn)
{
    const short memIndex = 2;
    short regIndex = 0;
    Operand &opnd = insn.GetOperand(memIndex);
    auto &memOpnd = static_cast<MemOperand &>(opnd);
    RegOperand *base = memOpnd.GetBaseRegister();
    if ((base == nullptr) || !(cgFunc.GetRD()->IsFrameReg(*base))) {
        return;
    }
    if (cgFunc.IsAfterRegAlloc() && !insn.IsSpillInsn()) {
        return;
    }
    DEBUG_ASSERT(memOpnd.GetIndexRegister() == nullptr, "frame MemOperand must not be exist register index");
    InsnSet memUseInsnSet;
    for (int i = 0; i != kMaxMovNum; ++i) {
        memUseInsnSet.clear();
        if (i == 0) {
            regIndex = 0;
            memUseInsnSet = cgFunc.GetRD()->FindUseForMemOpnd(insn, memIndex);
        } else {
            regIndex = 1;
            memUseInsnSet = cgFunc.GetRD()->FindUseForMemOpnd(insn, memIndex, true);
        }
        if (memUseInsnSet.empty()) {
            return;
        }
        auto &regOpnd = static_cast<RegOperand &>(insn.GetOperand(static_cast<uint32>(regIndex)));
        if (regOpnd.GetRegisterNumber() == RZR) {
            DoLoadZeroToMoveTransfer(insn, regIndex, memUseInsnSet);
        }
    }
}

void AArch64StoreLoadOpt::ProcessStr(Insn &insn)
{
    /* str x100, [mem], mem index is 1, x100 index is 0; */
    const short memIndex = 1;
    const short regIndex = 0;
    Operand &opnd = insn.GetOperand(memIndex);
    auto &memOpnd = static_cast<MemOperand &>(opnd);
    RegOperand *base = memOpnd.GetBaseRegister();
    if ((base == nullptr) || !(cgFunc.GetRD()->IsFrameReg(*base))) {
        return;
    }

    if (cgFunc.IsAfterRegAlloc() && !insn.IsSpillInsn()) {
        return;
    }
    DEBUG_ASSERT(memOpnd.GetIndexRegister() == nullptr, "frame MemOperand must not be exist register index");

    InsnSet memUseInsnSet = cgFunc.GetRD()->FindUseForMemOpnd(insn, memIndex);
    if (memUseInsnSet.empty()) {
        return;
    }

    auto *regOpnd = static_cast<RegOperand *>(&insn.GetOperand(regIndex));
    CHECK_NULL_FATAL(regOpnd);
    if (regOpnd->GetRegisterNumber() == RZR) {
        DoLoadZeroToMoveTransfer(insn, regIndex, memUseInsnSet);
    }
    if (cgFunc.IsAfterRegAlloc() && insn.IsSpillInsn()) {
        InsnSet newmemUseInsnSet = cgFunc.GetRD()->FindUseForMemOpnd(insn, memIndex);
        if (newmemUseInsnSet.empty()) {
            insn.GetBB()->RemoveInsn(insn);
        }
    }
}
} /* namespace maplebe */
