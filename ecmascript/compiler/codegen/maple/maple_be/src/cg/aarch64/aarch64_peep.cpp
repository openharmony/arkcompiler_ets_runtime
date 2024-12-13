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

#include "aarch64_peep.h"
#include "cg.h"
#include "mpl_logging.h"
#include "common_utils.h"
#include "cg_option.h"
#include "cg_irbuilder.h"
#include "aarch64_cg.h"

namespace maplebe {
#define CG_PEEP_DUMP CG_DEBUG_FUNC(*cgFunc)

static bool IsZeroRegister(const Operand &opnd)
{
    if (!opnd.IsRegister()) {
        return false;
    }
    const RegOperand *regOpnd = static_cast<const RegOperand *>(&opnd);
    return regOpnd->GetRegisterNumber() == RZR;
}

void AArch64CGPeepHole::Run()
{
    FOR_ALL_BB(bb, cgFunc)
    {
        FOR_BB_INSNS_SAFE(insn, bb, nextInsn)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            DoNormalOptimize(*bb, *insn);
        }
    }
}

void AArch64CGPeepHole::DoNormalOptimize(BB &bb, Insn &insn)
{
    MOperator thisMop = insn.GetMachineOpcode();
    manager = peepMemPool->New<PeepOptimizeManager>(*cgFunc, bb, insn);
    switch (thisMop) {
        /*
         * e.g.
         * execute before & after RA: manager->NormalPatternOpt<>(true)
         * execute before RA: manager->NormalPatternOpt<>(!cgFunc->IsAfterRegAlloc())
         * execute after RA: manager->NormalPatternOpt<>(cgFunc->IsAfterRegAlloc())
         */
        case MOP_wmovrr:
        case MOP_xmovrr:
        case MOP_xvmovs:
        case MOP_xvmovd: {
            manager->NormalPatternOpt<RemoveMovingtoSameRegPattern>(cgFunc->IsAfterRegAlloc());
            break;
        }
        case MOP_wstrb:
        case MOP_wldrb: {
            // only strb ldrb can do this pattern, other patterns still need to be done, so there is no break here.
            break;
        }
        case MOP_wstrh:
        case MOP_wldrh:
        case MOP_xldr:
        case MOP_xstr:
        case MOP_wldr:
        case MOP_wstr:
        case MOP_dldr:
        case MOP_dstr:
        case MOP_sldr:
        case MOP_sstr:
        case MOP_qldr:
        case MOP_qstr: {
            manager->NormalPatternOpt<CombineContiLoadAndStorePattern>(cgFunc->IsAfterRegAlloc());
            if (!manager->OptSuccess()) {
                manager->NormalPatternOpt<ContiLDRorSTRToSameMEMPattern>(cgFunc->IsAfterRegAlloc());
            }
            break;
        }
        case MOP_xvmovrv:
        case MOP_xvmovrd: {
            manager->NormalPatternOpt<FmovRegPattern>(cgFunc->IsAfterRegAlloc());
            break;
        }
        case MOP_xsbfxrri6i6: {
            manager->NormalPatternOpt<SbfxOptPattern>(cgFunc->IsAfterRegAlloc());
            break;
        }
        case MOP_wcbz:
        case MOP_xcbz:
        case MOP_wcbnz:
        case MOP_xcbnz: {
            manager->NormalPatternOpt<CbnzToCbzPattern>(cgFunc->IsAfterRegAlloc());
            break;
        }
        default:
            break;
    }
}
/* ======== CGPeepPattern End ======== */

void AArch64PeepHole::InitOpts()
{
    optimizations.resize(kPeepholeOptsNum);
    optimizations[kEliminateSpecifcSXTOpt] = optOwnMemPool->New<EliminateSpecifcSXTAArch64>(cgFunc);
    optimizations[kEliminateSpecifcUXTOpt] = optOwnMemPool->New<EliminateSpecifcUXTAArch64>(cgFunc);
    optimizations[kCsetCbzToBeqOpt] = optOwnMemPool->New<CsetCbzToBeqOptAArch64>(cgFunc);
    optimizations[kAndCmpBranchesToCsetOpt] = optOwnMemPool->New<AndCmpBranchesToCsetAArch64>(cgFunc);
    optimizations[kAndCmpBranchesToTstOpt] = optOwnMemPool->New<AndCmpBranchesToTstAArch64>(cgFunc);
    optimizations[kAndCbzBranchesToTstOpt] = optOwnMemPool->New<AndCbzBranchesToTstAArch64>(cgFunc);
    optimizations[kZeroCmpBranchesOpt] = optOwnMemPool->New<ZeroCmpBranchesAArch64>(cgFunc);
    optimizations[kCselZeroOneToCsetOpt] = optOwnMemPool->New<CselZeroOneToCsetOpt>(cgFunc);
    optimizations[kAndCmpCsetEorCbzOpt] = optOwnMemPool->New<AndCmpCsetEorCbzOpt>(cgFunc);
    optimizations[kAddLdrOpt] = optOwnMemPool->New<AddLdrOpt>(cgFunc);
    optimizations[kCsetEorOpt] = optOwnMemPool->New<CsetEorOpt>(cgFunc);
    optimizations[kMoveCmpOpt] = optOwnMemPool->New<MoveCmpOpt>(cgFunc);
}

void AArch64PeepHole::Run(BB &bb, Insn &insn)
{
    MOperator thisMop = insn.GetMachineOpcode();
    switch (thisMop) {
        case MOP_xsxtb32:
        case MOP_xsxth32:
        case MOP_xsxtb64:
        case MOP_xsxth64:
        case MOP_xsxtw64: {
            (static_cast<EliminateSpecifcSXTAArch64 *>(optimizations[kEliminateSpecifcSXTOpt]))->Run(bb, insn);
            break;
        }
        case MOP_xuxtb32:
        case MOP_xuxth32:
        case MOP_xuxtw64: {
            (static_cast<EliminateSpecifcUXTAArch64 *>(optimizations[kEliminateSpecifcUXTOpt]))->Run(bb, insn);
            break;
        }
        case MOP_wcbnz:
        case MOP_xcbnz: {
            (static_cast<CsetCbzToBeqOptAArch64 *>(optimizations[kCsetCbzToBeqOpt]))->Run(bb, insn);
            break;
        }
        case MOP_wcbz:
        case MOP_xcbz: {
            (static_cast<CsetCbzToBeqOptAArch64 *>(optimizations[kCsetCbzToBeqOpt]))->Run(bb, insn);
            break;
        }
        case MOP_xandrrr:
        case MOP_wandrrr:
        case MOP_wandrri12:
        case MOP_xandrri13: {
            (static_cast<AndCmpCsetEorCbzOpt *>(optimizations[kAndCmpCsetEorCbzOpt]))->Run(bb, insn);
            (static_cast<AndCmpBranchesToTstAArch64 *>(optimizations[kAndCmpBranchesToTstOpt]))->Run(bb, insn);
            (static_cast<AndCbzBranchesToTstAArch64 *>(optimizations[kAndCbzBranchesToTstOpt]))->Run(bb, insn);
            break;
        }
        case MOP_wcsetrc:
        case MOP_xcsetrc: {
            (static_cast<CsetEorOpt *>(optimizations[kCsetEorOpt]))->Run(bb, insn);
            (static_cast<AndCmpBranchesToCsetAArch64 *>(optimizations[kAndCmpBranchesToCsetOpt]))->Run(bb, insn);
            break;
        }
        case MOP_xmovri64:
        case MOP_wmovri32: {
            static_cast<MoveCmpOpt *>(optimizations[kMoveCmpOpt])->Run(bb, insn);
            break;
        }
        case MOP_xaddrrr: {
            (static_cast<AddLdrOpt *>(optimizations[kAddLdrOpt]))->Run(bb, insn);
            break;
        }
        case MOP_wcselrrrc:
        case MOP_xcselrrrc: {
            (static_cast<CselZeroOneToCsetOpt *>(optimizations[kCselZeroOneToCsetOpt]))->Run(bb, insn);
            break;
        }
        default:
            break;
    }
    if (&insn == bb.GetLastInsn()) {
        (static_cast<ZeroCmpBranchesAArch64 *>(optimizations[kZeroCmpBranchesOpt]))->Run(bb, insn);
    }
}

void AArch64PeepHole0::InitOpts()
{
    optimizations.resize(kPeepholeOptsNum);
    optimizations[kRemoveIdenticalLoadAndStoreOpt] = optOwnMemPool->New<RemoveIdenticalLoadAndStoreAArch64>(cgFunc);
    optimizations[kCmpCsetOpt] = optOwnMemPool->New<CmpCsetAArch64>(cgFunc);
    optimizations[kComplexMemOperandOptAdd] = optOwnMemPool->New<ComplexMemOperandAddAArch64>(cgFunc);
    optimizations[kRemoveSxtBeforeStrOpt] = optOwnMemPool->New<RemoveSxtBeforeStrAArch64>(cgFunc);
    optimizations[kRedundantMovAArch64Opt] = optOwnMemPool->New<RedundantMovAArch64>(cgFunc);
    optimizations[kRemoveMovingtoSameRegOpt] = optOwnMemPool->New<RemoveMovingtoSameRegAArch64>(cgFunc);
    optimizations[kEnhanceStrLdrAArch64Opt] = optOwnMemPool->New<EnhanceStrLdrAArch64>(cgFunc);
    optimizations[kAddImmZeroToMov] = optOwnMemPool->New<AddImmZeroToMov>(cgFunc);
}

void AArch64PeepHole0::Run(BB &bb, Insn &insn)
{
    MOperator thisMop = insn.GetMachineOpcode();
    switch (thisMop) {
        case MOP_wcmpri:
        case MOP_xcmpri: {
            (static_cast<CmpCsetAArch64 *>(optimizations[kCmpCsetOpt]))->Run(bb, insn);
            break;
        }
        case MOP_xaddrrr: {
            (static_cast<ComplexMemOperandAddAArch64 *>(optimizations[kComplexMemOperandOptAdd]))->Run(bb, insn);
            break;
        }
        case MOP_xaddrri12: {
            (static_cast<AddImmZeroToMov *>(optimizations[kAddImmZeroToMov]))->Run(bb, insn);
            break;
        }
        case MOP_wstrh:
        case MOP_wstrb: {
            (static_cast<RemoveSxtBeforeStrAArch64 *>(optimizations[kRemoveSxtBeforeStrOpt]))->Run(bb, insn);
            break;
        }
        case MOP_wmovrr:
        case MOP_xmovrr:
        case MOP_xvmovs:
        case MOP_xvmovd: {
            (static_cast<RedundantMovAArch64 *>(optimizations[kRedundantMovAArch64Opt]))->Run(bb, insn);
            (static_cast<RemoveMovingtoSameRegAArch64 *>(optimizations[kRemoveMovingtoSameRegOpt]))->Run(bb, insn);
            break;
        }
        case MOP_xldr:
        case MOP_xstr:
        case MOP_wldr:
        case MOP_wstr:
        case MOP_dldr:
        case MOP_dstr:
        case MOP_sldr:
        case MOP_sstr: {
            if (thisMop == MOP_wstr || thisMop == MOP_xstr) {
                (static_cast<RemoveIdenticalLoadAndStoreAArch64 *>(optimizations[kRemoveIdenticalLoadAndStoreOpt]))
                    ->Run(bb, insn);
            }
            (static_cast<EnhanceStrLdrAArch64 *>(optimizations[kEnhanceStrLdrAArch64Opt]))->Run(bb, insn);
            break;
        }
        default:
            break;
    }
}

bool RemoveIdenticalLoadAndStoreAArch64::IsMemOperandsIdentical(const Insn &insn1, const Insn &insn2) const
{
    regno_t regNO1 = static_cast<RegOperand &>(insn1.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
    regno_t regNO2 = static_cast<RegOperand &>(insn2.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
    if (regNO1 != regNO2) {
        return false;
    }
    /* Match only [base + offset] */
    auto &memOpnd1 = static_cast<MemOperand &>(insn1.GetOperand(kInsnSecondOpnd));
    if (memOpnd1.GetAddrMode() != MemOperand::kAddrModeBOi || !memOpnd1.IsIntactIndexed()) {
        return false;
    }
    auto &memOpnd2 = static_cast<MemOperand &>(insn2.GetOperand(kInsnSecondOpnd));
    if (memOpnd2.GetAddrMode() != MemOperand::kAddrModeBOi || !memOpnd1.IsIntactIndexed()) {
        return false;
    }
    Operand *base1 = memOpnd1.GetBaseRegister();
    Operand *base2 = memOpnd2.GetBaseRegister();
    if (!((base1 != nullptr) && base1->IsRegister()) || !((base2 != nullptr) && base2->IsRegister())) {
        return false;
    }

    regno_t baseRegNO1 = static_cast<RegOperand *>(base1)->GetRegisterNumber();
    /* First insn re-write base addr   reg1 <- [ reg1 + offset ] */
    if (baseRegNO1 == regNO1) {
        return false;
    }

    regno_t baseRegNO2 = static_cast<RegOperand *>(base2)->GetRegisterNumber();
    if (baseRegNO1 != baseRegNO2) {
        return false;
    }

    return memOpnd1.GetOffsetImmediate()->GetOffsetValue() == memOpnd2.GetOffsetImmediate()->GetOffsetValue();
}

void RemoveIdenticalLoadAndStoreAArch64::Run(BB &bb, Insn &insn)
{
    Insn *nextInsn = insn.GetNextMachineInsn();
    if (nextInsn == nullptr) {
        return;
    }
    MOperator mop1 = insn.GetMachineOpcode();
    MOperator mop2 = nextInsn->GetMachineOpcode();
    if ((mop1 == MOP_wstr && mop2 == MOP_wstr) || (mop1 == MOP_xstr && mop2 == MOP_xstr)) {
        if (IsMemOperandsIdentical(insn, *nextInsn)) {
            bb.RemoveInsn(insn);
        }
    } else if ((mop1 == MOP_wstr && mop2 == MOP_wldr) || (mop1 == MOP_xstr && mop2 == MOP_xldr)) {
        if (IsMemOperandsIdentical(insn, *nextInsn)) {
            bb.RemoveInsn(*nextInsn);
        }
    }
}

bool RemoveMovingtoSameRegPattern::CheckCondition(Insn &insn)
{
    DEBUG_ASSERT(insn.GetOperand(kInsnFirstOpnd).IsRegister(), "expects registers");
    DEBUG_ASSERT(insn.GetOperand(kInsnSecondOpnd).IsRegister(), "expects registers");
    auto &reg1 = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    auto &reg2 = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    /* remove mov x0,x0 when it cast i32 to i64 */
    if ((reg1.GetRegisterNumber() == reg2.GetRegisterNumber()) && (reg1.GetSize() >= reg2.GetSize())) {
        return true;
    }
    return false;
}

void RemoveMovingtoSameRegPattern::Run(BB &bb, Insn &insn)
{
    /* remove mov x0,x0 when it cast i32 to i64 */
    if (CheckCondition(insn)) {
        bb.RemoveInsn(insn);
    }
}

void RedundantMovAArch64::Run(BB &bb, Insn &insn)
{
    auto *prevInsn = insn.GetPreviousMachineInsn();
    if (prevInsn == nullptr) {
        return;
    }
    auto *prevOpndDes = prevInsn->GetDesc()->GetOpndDes(kInsnFirstOpnd);
    auto *srcOpndDesc  = insn.GetDesc()->GetOpndDes(kInsnSecondOpnd);
    if (!prevOpndDes->IsRegDef() || prevOpndDes->IsUse() || prevOpndDes->GetSize() > srcOpndDesc->GetSize()) {
        return;
    }
    auto &srcOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnSecondOpnd));
    auto &prevDestOpnd = static_cast<RegOperand&>(prevInsn->GetOperand(kInsnFirstOpnd));
    if (srcOpnd.GetRegisterNumber() != prevDestOpnd.GetRegisterNumber()) {
        return;
    }
    if (IfOperandIsLiveAfterInsn(srcOpnd, insn)) {
        return;
    }
    auto &desOpnd = static_cast<RegOperand&>(insn.GetOperand(kInsnFirstOpnd));
    prevInsn->SetOperand(kInsnFirstOpnd, desOpnd);
    bb.RemoveInsn(insn);
}

void RemoveMovingtoSameRegAArch64::Run(BB &bb, Insn &insn)
{
    DEBUG_ASSERT(insn.GetOperand(kInsnFirstOpnd).IsRegister(), "expects registers");
    DEBUG_ASSERT(insn.GetOperand(kInsnSecondOpnd).IsRegister(), "expects registers");
    auto &reg1 = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    auto &reg2 = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    /* remove mov x0,x0 when it cast i32 to i64 */
    if ((reg1.GetRegisterNumber() == reg2.GetRegisterNumber()) && (reg1.GetSize() >= reg2.GetSize())) {
        bb.RemoveInsn(insn);
    }
}

bool EnhanceStrLdrAArch64::CheckOperandIsDeadFromInsn(const RegOperand &regOpnd, Insn &insn)
{
    for (uint32 i = 0; i < insn.GetOperandSize(); ++i) {
        auto &opnd = insn.GetOperand(i);
        if (!insn.GetDesc()->GetOpndDes(i)->IsRegDef()) {
            continue;
        }
        // regOpnd is redefined at curInsn
        if (static_cast<RegOperand &>(opnd).GetRegisterNumber() == regOpnd.GetRegisterNumber()) {
            return true;
        }
    }
    return !IfOperandIsLiveAfterInsn(regOpnd, insn);
}

ImmOperand *EnhanceStrLdrAArch64::GetInsnAddOrSubNewOffset(Insn &insn, ImmOperand &offset)
{
    int64 val = 0;
    VaryType vary = offset.GetVary();
    auto mOp = insn.GetMachineOpcode();
    if (mOp == MOP_xaddrri12 || mOp == MOP_xsubrri12) {
        auto &immOpnd = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
        val = immOpnd.GetValue();
        CHECK_FATAL(!(vary == kUnAdjustVary && immOpnd.GetVary() == kUnAdjustVary), "NIY, can not deal this case!");
        vary = immOpnd.GetVary();
    } else {
        auto &immOpnd = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
        auto &shiftOpnd = static_cast<BitShiftOperand &>(insn.GetOperand(kInsnFourthOpnd));
        CHECK_FATAL(shiftOpnd.GetShiftAmount() == 12, "invalid shiftAmount"); // 12: invalid shiftAmount
        val = (immOpnd.GetValue() << shiftOpnd.GetShiftAmount());
    }

    if (mOp == MOP_xsubrri12 || mOp == MOP_xsubrri24) {
        val = -val;
    }
    val += offset.GetValue();
    auto &newImm = static_cast<AArch64CGFunc &>(cgFunc).GetOrCreateOfstOpnd(val, k64BitSize);
    newImm.SetVary(vary);
    return &newImm;
}

void EnhanceStrLdrAArch64::OptimizeAddrBOI(Insn &insn, MemOperand &memOpnd, Insn &prevInsn)
{
    auto *oriBase = memOpnd.GetBaseRegister();
    auto *oriOffset = memOpnd.GetOffsetOperand();
    auto &defOpnd = static_cast<RegOperand &>(prevInsn.GetOperand(kInsnFirstOpnd));
    if (defOpnd.GetRegisterNumber() != oriBase->GetRegisterNumber() || !CheckOperandIsDeadFromInsn(defOpnd, insn)) {
        return;
    }
    auto *newBase = static_cast<RegOperand *>(&prevInsn.GetOperand(kInsnSecondOpnd));
    auto *newOffset = GetInsnAddOrSubNewOffset(prevInsn, *memOpnd.GetOffsetOperand());
    if (newOffset->GetValue() < 0) {
        return;  // obj dump cannot deal str	x19, [x29,#-16]
    }

    memOpnd.SetBaseRegister(*newBase);
    memOpnd.SetOffsetOperand(*newOffset);
    if (!static_cast<AArch64CGFunc &>(cgFunc).IsOperandImmValid(insn.GetMachineOpcode(), &memOpnd, kInsnSecondOpnd)) {
        // If new offset is invalid, undo it
        memOpnd.SetBaseRegister(*oriBase);
        memOpnd.SetOffsetOperand(*oriOffset);
        return;
    }
    memOpnd.SetAddrMode(MemOperand::kAddrModeBOi);
    prevInsn.GetBB()->RemoveInsn(prevInsn);
}

void EnhanceStrLdrAArch64::OptimizeAddrBOrXShiftExtend(Insn &insn, MemOperand &memOpnd, Insn &shiftExtendInsn)
{
    auto mOp = shiftExtendInsn.GetMachineOpcode();
    if (mOp != MOP_xuxtw64 && mOp != MOP_xsxtw64 && mOp != MOP_xlslrri6) {
        return;
    }
    auto *oriIndex = memOpnd.GetIndexRegister();
    auto &defOpnd = static_cast<RegOperand &>(shiftExtendInsn.GetOperand(kInsnFirstOpnd));
    if (defOpnd.GetRegisterNumber() != oriIndex->GetRegisterNumber() || !CheckOperandIsDeadFromInsn(defOpnd, insn)) {
        return;
    }
    auto &newIndex = static_cast<RegOperand &>(shiftExtendInsn.GetOperand(kInsnSecondOpnd));
    bool isSigned = (mOp == MOP_xsxtw64);
    uint32 shift = 0;
    if (mOp == MOP_xlslrri6) {
        shift = static_cast<uint32>(static_cast<ImmOperand &>(shiftExtendInsn.GetOperand(kInsnThirdOpnd)).GetValue());
    }
    const uint32 regSize = insn.GetDesc()->GetOpndDes(kInsnFirstOpnd)->GetSize();
    // lsl extend insn shift amount can only be 0 or 1(16-bit def opnd) or 2(32-bit def opnd) or
    // 3(64-bit def opnd) or 4(128-bit def opnd) in ldr/str insn, and in this pattern we only have
    // 32-bit & 64-bit situation now
    if ((shift == k0BitSize) || (regSize == k32BitSize && shift == k2BitSize) ||
        (regSize == k64BitSize && shift == k3BitSize)) {
        auto *newMemOpnd = static_cast<AArch64CGFunc &>(cgFunc).CreateMemOperand(
            MemOperand::kAddrModeBOrX, memOpnd.GetSize(), *memOpnd.GetBaseRegister(), newIndex, shift, isSigned);
        insn.SetOperand(kInsnSecondOpnd, *newMemOpnd);
        shiftExtendInsn.GetBB()->RemoveInsn(shiftExtendInsn);
    }
}

void EnhanceStrLdrAArch64::OptimizeAddrBOrX(Insn &insn, MemOperand &memOpnd, Insn &prevInsn)
{
    if (memOpnd.GetOffsetOperand()->GetValue() != 0 || memOpnd.GetOffsetOperand()->GetVary() == kUnAdjustVary) {
        return;
    }
    auto *oriBase = memOpnd.GetBaseRegister();
    auto &defOpnd = static_cast<RegOperand &>(prevInsn.GetOperand(kInsnFirstOpnd));
    if (defOpnd.GetRegisterNumber() != oriBase->GetRegisterNumber() || !CheckOperandIsDeadFromInsn(defOpnd, insn)) {
        return;
    }
    auto *newBase = static_cast<RegOperand *>(&prevInsn.GetOperand(kInsnSecondOpnd));
    auto *newIndex = static_cast<RegOperand *>(&prevInsn.GetOperand(kInsnThirdOpnd));

    memOpnd.SetBaseRegister(*newBase);
    memOpnd.SetIndexRegister(*newIndex);
    memOpnd.SetAddrMode(MemOperand::kAddrModeBOrX);
    auto *prevShiftExtendInsn = prevInsn.GetPreviousMachineInsn();
    if (prevShiftExtendInsn != nullptr) {
        OptimizeAddrBOrXShiftExtend(insn, memOpnd, *prevShiftExtendInsn);
    }
    prevInsn.GetBB()->RemoveInsn(prevInsn);
}

void EnhanceStrLdrAArch64::OptimizeWithAddrrrs(Insn &insn, MemOperand &memOpnd, Insn &addInsn)
{
    if (memOpnd.GetOffsetOperand()->GetValue() != 0 || memOpnd.GetOffsetOperand()->GetVary() != kNotVary) {
        return;
    }
    auto *oriBase = memOpnd.GetBaseRegister();
    auto &defOpnd = static_cast<RegOperand &>(addInsn.GetOperand(kInsnFirstOpnd));
    if (defOpnd.GetRegisterNumber() != oriBase->GetRegisterNumber() || !CheckOperandIsDeadFromInsn(defOpnd, insn)) {
        return;
    }
    auto &newBase = static_cast<RegOperand &>(addInsn.GetOperand(kInsnSecondOpnd));
    auto &newIndex = static_cast<RegOperand &>(addInsn.GetOperand(kInsnThirdOpnd));
    auto &shift = static_cast<BitShiftOperand &>(addInsn.GetOperand(kInsnFourthOpnd));
    if (shift.GetShiftOp() != BitShiftOperand::kLSL) {
        return;
    }
    auto *newMemOpnd = static_cast<AArch64CGFunc &>(cgFunc).CreateMemOperand(
        MemOperand::kAddrModeBOrX, memOpnd.GetSize(), newBase, newIndex, shift.GetShiftAmount());
    insn.SetOperand(kInsnSecondOpnd, *newMemOpnd);
    addInsn.GetBB()->RemoveInsn(addInsn);
}

void EnhanceStrLdrAArch64::Run(BB &bb, Insn &insn)
{
    Operand &opnd = insn.GetOperand(kInsnSecondOpnd);
    CHECK_FATAL(opnd.IsMemoryAccessOperand(), "Unexpected operand in EnhanceStrLdrAArch64");
    auto &memOpnd = static_cast<MemOperand &>(opnd);
    if (memOpnd.GetAddrMode() != MemOperand::kAddrModeBOi || !memOpnd.GetOffsetImmediate()->IsImmOffset()) {
        return;
    }

    auto *prev = insn.GetPreviousMachineInsn();
    while (prev != nullptr) {
        if (prev->GetMachineOpcode() == MOP_xmovrr) {
            auto &defOpnd = static_cast<RegOperand &>(prev->GetOperand(kInsnFirstOpnd));
            if (defOpnd.GetRegisterNumber() != memOpnd.GetBaseRegister()->GetRegisterNumber() ||
                !CheckOperandIsDeadFromInsn(defOpnd, insn)) {
                return;
            }
            memOpnd.SetBaseRegister(static_cast<RegOperand &>(prev->GetOperand(kInsnSecondOpnd)));
            auto *tmpInsn = prev;
            prev = prev->GetPreviousMachineInsn();
            tmpInsn->GetBB()->RemoveInsn(*tmpInsn);
            continue;
        }
        break;
    }
    if (prev == nullptr) {
        return;
    }
    auto prevMop = prev->GetMachineOpcode();
    if (prevMop == MOP_xaddrri12 || prevMop == MOP_xsubrri12 || prevMop == MOP_xaddrri24 || prevMop == MOP_xsubrri24) {
        OptimizeAddrBOI(insn, memOpnd, *prev);
    } else if (prevMop == MOP_xaddrrr) {
        OptimizeAddrBOrX(insn, memOpnd, *prev);
    } else if (prevMop == MOP_xaddrrrs) {
        OptimizeWithAddrrrs(insn, memOpnd, *prev);
    }
}

bool IsSameRegisterOperation(const RegOperand &desMovOpnd, const RegOperand &uxtDestOpnd, const RegOperand &uxtFromOpnd)
{
    return ((desMovOpnd.GetRegisterNumber() == uxtDestOpnd.GetRegisterNumber()) &&
            (uxtDestOpnd.GetRegisterNumber() == uxtFromOpnd.GetRegisterNumber()));
}

bool CombineContiLoadAndStorePattern::IsRegNotSameMemUseInInsn(const Insn &checkInsn, const Insn &curInsn,
                                                               regno_t curBaseRegNO, bool isCurStore, int64 curBaseOfst,
                                                               int64 curMemRange) const
{
    uint32 opndNum = checkInsn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = checkInsn.GetOperand(i);
        if (opnd.IsList()) {
            auto &listOpnd = static_cast<const ListOperand &>(opnd);
            for (auto &listElem : listOpnd.GetOperands()) {
                auto *regOpnd = static_cast<RegOperand *>(listElem);
                DEBUG_ASSERT(regOpnd != nullptr, "parameter operand must be RegOperand");
                if (curBaseRegNO == regOpnd->GetRegisterNumber()) {
                    return true;
                }
            }
        } else if (opnd.IsMemoryAccessOperand()) {
            auto &memOperand = static_cast<MemOperand &>(opnd);
            RegOperand *checkBaseReg = memOperand.GetBaseRegister();
            // Check memory overlap
            if ((isCurStore || checkInsn.IsStore()) && checkBaseReg != nullptr &&
                memOperand.GetAddrMode() == MemOperand::kAddrModeBOi && memOperand.GetOffsetImmediate() != nullptr) {
                // If memInsn is split with x16, we need to find the actual base register
                int64 checkOffset = memOperand.GetOffsetImmediate()->GetOffsetValue();
                regno_t checkRegNO = checkBaseReg->GetRegisterNumber();
                if (checkRegNO == R16) {
                    const Insn *prevInsn = checkInsn.GetPrev();
                    // Before cgaggressiveopt, the def and use of R16 must be adjacent, and the def of R16 must be
                    // addrri, otherwise, the process is conservative and the mem insn that can be combined is not
                    // search forward.
                    if (prevInsn == nullptr || prevInsn->GetMachineOpcode() != MOP_xaddrri12 ||
                        static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd)).GetRegisterNumber() != R16) {
                        return true;
                    }
                    checkOffset += static_cast<ImmOperand &>(prevInsn->GetOperand(kInsnThirdOpnd)).GetValue();
                }
                auto checkMemRange = static_cast<int64>(checkInsn.GetMemoryByteSize());
                //      curOfst          curOfst+curMemRange
                // |______|_/_/_/_/_/_/_/_/_/_/_|____________|
                if ((curBaseOfst >= checkOffset && curBaseOfst < (checkOffset + checkMemRange)) ||
                    (checkOffset >= curBaseOfst && checkOffset < (curBaseOfst + curMemRange))) {
                    return true;
                }
            }
        } else if (opnd.IsConditionCode()) {
            auto &rflagOpnd = static_cast<RegOperand &>(cgFunc->GetOrCreateRflag());
            if (rflagOpnd.GetRegisterNumber() == curBaseRegNO) {
                return true;
            }
        } else if (opnd.IsRegister()) {
            if (!isCurStore && static_cast<RegOperand &>(opnd).GetRegisterNumber() == curBaseRegNO) {
                return true;
            }
        }
    }
    return false;
}

std::vector<Insn *> CombineContiLoadAndStorePattern::FindPrevStrLdr(Insn &insn, regno_t destRegNO, regno_t memBaseRegNO,
                                                                    int64 baseOfst) const
{
    std::vector<Insn *> prevContiInsns;
    for (Insn *curInsn = insn.GetPrev(); curInsn != nullptr; curInsn = curInsn->GetPrev()) {
        if (!curInsn->IsMachineInstruction()) {
            continue;
        }
        if (curInsn->IsRegDefined(memBaseRegNO)) {
            return prevContiInsns;
        }
        DEBUG_ASSERT(insn.GetOperand(kInsnSecondOpnd).IsMemoryAccessOperand(), "invalid mem insn");
        auto baseMemRange = static_cast<int64>(insn.GetMemoryByteSize());
        if (IsRegNotSameMemUseInInsn(*curInsn, insn, memBaseRegNO, insn.IsStore(), static_cast<int32>(baseOfst),
                                     baseMemRange)) {
            return prevContiInsns;
        }
        // record continuous STD/LDR insn
        if (!curInsn->IsLoadStorePair() &&
            ((insn.IsStore() && curInsn->IsStore()) || (insn.IsLoad() && curInsn->IsLoad()))) {
            auto *memOperand = static_cast<MemOperand *>(curInsn->GetMemOpnd());
            /* do not combine ldr r0, label */
            if (memOperand != nullptr) {
                auto *baseRegOpnd = static_cast<RegOperand *>(memOperand->GetBaseRegister());
                DEBUG_ASSERT(baseRegOpnd == nullptr || !baseRegOpnd->IsVirtualRegister(),
                             "physical register has not been allocated?");
                if (memOperand->GetAddrMode() == MemOperand::kAddrModeBOi &&
                    baseRegOpnd->GetRegisterNumber() == memBaseRegNO) {
                    prevContiInsns.emplace_back(curInsn);
                }
            }
        }
        /* check insn that changes the data flow */
        /* ldr x8, [x21, #8]
         * call foo()
         * ldr x9, [x21, #16]
         * although x21 is a calleeSave register, there is no guarantee data in memory [x21] is not changed
         */
        if (curInsn->IsCall() || curInsn->GetMachineOpcode() == MOP_asm) {
            return prevContiInsns;
        }
        /* Check regOpnd for mem access:
         * 1. if the destRegNO is RZR, we do not need to check define and use for destRegNO between PREVINSN and INSN;
         * 2. for load insn, we forbid both use and define destRegNO between PREVINSN and INSN;
         * 3. for store insn, we only forbit define destRegNO between PREVINSN and INSN;
         * e.g.1
         * ldr x2, [sp, #16]
         * add x3, x1, #5  &  add x1, x3, #5  ---\-->  all [x1] use and define can not across
         * ldr x1, [sp, #8]
         * e.g.2
         * str x2, [sp, #16]
         * add x1, x3, #5   ---\--->  only [x1] define can not across
         * str x1, [sp, #8]
         */
        /* store opt should not cross call due to stack args */
        if (destRegNO != RZR &&
            ((insn.IsLoad() && curInsn->ScanReg(destRegNO)) || (insn.IsStore() && curInsn->IsRegDefined(destRegNO)))) {
            return prevContiInsns;
        }
        if (curInsn->ScanReg(destRegNO)) {
            return prevContiInsns;
        }
    }
    return prevContiInsns;
}

bool CombineContiLoadAndStorePattern::CheckCondition(Insn &insn)
{
    MOperator mop = insn.GetMachineOpcode();
    if (mop == MOP_wldrb || mop == MOP_wldrh) {
        return false;
    }
    auto *curMemOpnd = static_cast<MemOperand *>(insn.GetMemOpnd());
    DEBUG_ASSERT(curMemOpnd != nullptr, "get mem operand failed");
    if (!doAggressiveCombine || curMemOpnd->GetAddrMode() != MemOperand::kAddrModeBOi) {
        return false;
    }
    return true;
}

/* Combining 2 STRs into 1 stp or 2 LDRs into 1 ldp */
void CombineContiLoadAndStorePattern::Run(BB &bb, Insn &insn)
{
    if (!CheckCondition(insn)) {
        return;
    }

    auto *curMemOpnd = static_cast<MemOperand *>(insn.GetMemOpnd());
    CHECK_FATAL(curMemOpnd != nullptr, "nullptr check");
    DEBUG_ASSERT(curMemOpnd->GetAddrMode() == MemOperand::kAddrModeBOi, "invalid continues mem insn");
    OfstOperand *curOfstOpnd = curMemOpnd->GetOffsetImmediate();
    int64 curOfstVal = curOfstOpnd ? curOfstOpnd->GetOffsetValue() : 0;

    auto *baseRegOpnd = static_cast<RegOperand *>(curMemOpnd->GetBaseRegister());
    DEBUG_ASSERT(baseRegOpnd == nullptr || !baseRegOpnd->IsVirtualRegister(),
                 "physical register has not been allocated?");
    auto &curDestOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    std::vector<Insn *> prevContiInsnVec =
        FindPrevStrLdr(insn, curDestOpnd.GetRegisterNumber(), baseRegOpnd->GetRegisterNumber(), curOfstVal);
    for (auto prevContiInsn : prevContiInsnVec) {
        DEBUG_ASSERT(prevContiInsn != nullptr, "get previous consecutive instructions failed");
        auto *prevMemOpnd = static_cast<MemOperand *>(prevContiInsn->GetMemOpnd());
        DEBUG_ASSERT(prevMemOpnd->GetAddrMode() == MemOperand::kAddrModeBOi, "invalid continues mem insn");
        OfstOperand *prevOfstOpnd = prevMemOpnd->GetOffsetImmediate();
        int64 prevOfstVal = prevOfstOpnd ? prevOfstOpnd->GetOffsetValue() : 0;
        auto &prevDestOpnd = static_cast<RegOperand &>(prevContiInsn->GetOperand(kInsnFirstOpnd));
        if (prevDestOpnd.GetRegisterType() != curDestOpnd.GetRegisterType()) {
            continue;
        }

        MemOperand *combineMemOpnd = (curOfstVal < prevOfstVal) ? curMemOpnd : prevMemOpnd;
        if (IsValidNormalLoadOrStorePattern(insn, *prevContiInsn, *curMemOpnd, curOfstVal, prevOfstVal)) {
            // Process normal mem pair
            MOperator newMop = GetMopPair(insn.GetMachineOpcode(), true);
            Insn *combineInsn =
                GenerateMemPairInsn(newMop, curDestOpnd, prevDestOpnd, *combineMemOpnd, curOfstVal < prevOfstVal);
            DEBUG_ASSERT(combineInsn != nullptr, "create combineInsn failed");
            bb.InsertInsnAfter(*prevContiInsn, *combineInsn);
            if (!(static_cast<AArch64CGFunc &>(*cgFunc).IsOperandImmValid(
                newMop, combineMemOpnd, isPairAfterCombine ? kInsnThirdOpnd : kInsnSecondOpnd))) {
                if (FindUseX16AfterInsn(*prevContiInsn)) {
                    // Do not combine Insns when x16 was used after curInsn
                    bb.RemoveInsn(*combineInsn);
                    return;
                }
                SPLIT_INSN(combineInsn, cgFunc);
            }
            RemoveInsnAndKeepComment(bb, insn, *prevContiInsn);
            SetCurrInsn(combineInsn);
            optSuccess = true;
            return;
        } else if (IsValidStackArgLoadOrStorePattern(insn, *prevContiInsn, *curMemOpnd, *prevMemOpnd, curOfstVal,
                                                     prevOfstVal)) {
            // Process stack-arg mem pair
            regno_t curDestRegNo = curDestOpnd.GetRegisterNumber();
            regno_t prevDestRegNo = prevDestOpnd.GetRegisterNumber();
            RegOperand &newDest = static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreatePhysicalRegisterOperand(
                static_cast<AArch64reg>(curDestRegNo), k64BitSize, curDestOpnd.GetRegisterType());
            RegOperand &newPrevDest = static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreatePhysicalRegisterOperand(
                static_cast<AArch64reg>(prevDestRegNo), k64BitSize, prevDestOpnd.GetRegisterType());
            MOperator newMop = (curDestOpnd.GetRegisterType() == kRegTyInt) ? MOP_xstp : MOP_dstp;
            if (!(static_cast<AArch64CGFunc &>(*cgFunc).IsOperandImmValid(newMop, combineMemOpnd, kInsnThirdOpnd))) {
                return;
            }
            Insn *combineInsn =
                GenerateMemPairInsn(newMop, newDest, newPrevDest, *combineMemOpnd, curOfstVal < prevOfstVal);
            bb.InsertInsnAfter(*prevContiInsn, *combineInsn);
            RemoveInsnAndKeepComment(bb, insn, *prevContiInsn);
            SetCurrInsn(combineInsn);
            optSuccess = true;
            return;
        }
    }
}

bool CombineContiLoadAndStorePattern::FindUseX16AfterInsn(const Insn &curInsn) const
{
    for (Insn *cursor = curInsn.GetNext(); cursor != nullptr; cursor = cursor->GetNext()) {
        if (!cursor->IsMachineInstruction()) {
            continue;
        }
        for (uint32 defRegNo : cursor->GetDefRegs()) {
            if (defRegNo == R16) {
                return false;
            }
        }
        if ((!cursor->IsLoad() && !cursor->IsStore() && !cursor->IsLoadStorePair()) || cursor->IsAtomic()) {
            continue;
        }
        const InsnDesc *md = &AArch64CG::kMd[cursor->GetMachineOpcode()];
        if (cursor->IsLoadLabel() || md->IsLoadAddress()) {
            continue;
        }
        uint32 memIdx = (cursor->IsLoadStorePair() ? kInsnThirdOpnd : kInsnSecondOpnd);
        auto &curMemOpnd = static_cast<MemOperand &>(cursor->GetOperand(memIdx));
        RegOperand *baseOpnd = curMemOpnd.GetBaseRegister();
        if (baseOpnd != nullptr && baseOpnd->GetRegisterNumber() == R16) {
            return true;
        }
    }
    return false;
}

Insn *CombineContiLoadAndStorePattern::GenerateMemPairInsn(MOperator newMop, RegOperand &curDestOpnd,
                                                           RegOperand &prevDestOpnd, MemOperand &combineMemOpnd,
                                                           bool isCurDestFirst)
{
    DEBUG_ASSERT(newMop != MOP_undef, "invalid MOperator");
    Insn *combineInsn = nullptr;
    if (isPairAfterCombine) {  // for ldr/str --> ldp/stp
        combineInsn = (isCurDestFirst)
                          ? &cgFunc->GetInsnBuilder()->BuildInsn(newMop, curDestOpnd, prevDestOpnd, combineMemOpnd)
                          : &cgFunc->GetInsnBuilder()->BuildInsn(newMop, prevDestOpnd, curDestOpnd, combineMemOpnd);
    } else {  // for strb/strh --> strh/str, curDestOpnd == preDestOpnd
        combineInsn = &cgFunc->GetInsnBuilder()->BuildInsn(newMop, curDestOpnd, combineMemOpnd);
        combineMemOpnd.SetSize(newMop == MOP_wstrh ? maplebe::k16BitSize : maplebe::k32BitSize);
    }
    return combineInsn;
}

bool CombineContiLoadAndStorePattern::IsValidNormalLoadOrStorePattern(const Insn &insn, const Insn &prevInsn,
                                                                      const MemOperand &memOpnd, int64 curOfstVal,
                                                                      int64 prevOfstVal)
{
    if (memOpnd.IsStackArgMem()) {
        return false;
    }
    DEBUG_ASSERT(insn.GetOperand(kInsnFirstOpnd).IsRegister(), "unexpect operand");
    DEBUG_ASSERT(prevInsn.GetOperand(kInsnFirstOpnd).IsRegister(), "unexpect operand");
    auto &curDestOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    auto &prevDestOpnd = static_cast<RegOperand &>(prevInsn.GetOperand(kInsnFirstOpnd));
    if (prevDestOpnd.GetRegisterType() != curDestOpnd.GetRegisterType() ||
        curDestOpnd.GetSize() != prevDestOpnd.GetSize()) {
        return false;
    }
    uint32 memSize = insn.GetMemoryByteSize();
    uint32 prevMemSize = prevInsn.GetMemoryByteSize();
    if (memSize != prevMemSize) {
        return false;
    }

    int64 diffVal = std::abs(curOfstVal - prevOfstVal);
    if ((memSize == k1ByteSize && diffVal == k1BitSize) || (memSize == k2ByteSize && diffVal == k2BitSize) ||
        (memSize == k4ByteSize && diffVal == k4BitSize) || (memSize == k8ByteSize && diffVal == k8BitSize)) {
        MOperator curMop = insn.GetMachineOpcode();
        DEBUG_ASSERT(curMop != MOP_wldrb && curMop != MOP_wldrh, "invalid mem insn that cannot be combined");
        if (curMop == MOP_wstrb || curMop == MOP_wstrh) {
            isPairAfterCombine = false;
        }

        regno_t destRegNO = curDestOpnd.GetRegisterNumber();
        regno_t prevDestRegNO = prevDestOpnd.GetRegisterNumber();
        if (destRegNO == RZR && prevDestRegNO == RZR) {
            return true;
        }

        if (insn.IsLoad() && destRegNO == prevDestRegNO) {
            return false;
        }

        if ((curMop == MOP_wstrb || curMop == MOP_wstrh) && (destRegNO != RZR || prevDestRegNO != RZR)) {
            return false;
        }

        return true;
    }

    return false;
}

bool CombineContiLoadAndStorePattern::IsValidStackArgLoadOrStorePattern(const Insn &curInsn, const Insn &prevInsn,
                                                                        const MemOperand &curMemOpnd,
                                                                        const MemOperand &prevMemOpnd, int64 curOfstVal,
                                                                        int64 prevOfstVal) const
{
    if (!curInsn.IsStore()) {
        return false;
    }
    if (!curMemOpnd.IsStackArgMem() || !prevMemOpnd.IsStackArgMem()) {
        return false;
    }
    auto &curDestOpnd = static_cast<RegOperand &>(curInsn.GetOperand(kInsnFirstOpnd));
    auto &prevDestOpnd = static_cast<RegOperand &>(prevInsn.GetOperand(kInsnFirstOpnd));
    uint32 memSize = curInsn.GetMemoryByteSize();
    uint32 prevMemSize = prevInsn.GetMemoryByteSize();
    auto diffVal = std::abs(curOfstVal - prevOfstVal);
    if ((memSize == k4ByteSize || memSize == k8ByteSize) && (prevMemSize == k4ByteSize || prevMemSize == k8ByteSize) &&
        (diffVal == k8BitSize) && (curDestOpnd.GetValidBitsNum() == memSize * k8BitSize) &&
        (prevDestOpnd.GetValidBitsNum() == prevMemSize * k8BitSize)) {
        return true;
    }
    return false;
}

void CombineContiLoadAndStorePattern::RemoveInsnAndKeepComment(BB &bb, Insn &insn, Insn &prevInsn) const
{
    /* keep the comment */
    Insn *nn = prevInsn.GetNextMachineInsn();
    std::string newComment = "";
    MapleString comment = insn.GetComment();
    if (comment.c_str() != nullptr && strlen(comment.c_str()) > 0) {
        newComment += comment.c_str();
    }
    comment = prevInsn.GetComment();
    if (comment.c_str() != nullptr && strlen(comment.c_str()) > 0) {
        newComment = newComment + "  " + comment.c_str();
    }
    if (newComment.c_str() != nullptr && strlen(newComment.c_str()) > 0) {
        DEBUG_ASSERT(nn != nullptr, "nn should not be nullptr");
        nn->SetComment(newComment);
    }
    bb.RemoveInsn(insn);
    bb.RemoveInsn(prevInsn);
}

void EliminateSpecifcSXTAArch64::Run(BB &bb, Insn &insn)
{
    MOperator thisMop = insn.GetMachineOpcode();
    Insn *prevInsn = insn.GetPrev();
    while (prevInsn != nullptr && !prevInsn->GetMachineOpcode()) {
        prevInsn = prevInsn->GetPrev();
    }
    if (prevInsn == nullptr) {
        return;
    }
    auto &regOpnd0 = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    auto &regOpnd1 = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    if (&insn != bb.GetFirstInsn() && regOpnd0.GetRegisterNumber() == regOpnd1.GetRegisterNumber() &&
        prevInsn->IsMachineInstruction()) {
        if (prevInsn->GetMachineOpcode() == MOP_wmovri32 || prevInsn->GetMachineOpcode() == MOP_xmovri64) {
            auto &dstMovOpnd = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
            if (dstMovOpnd.GetRegisterNumber() != regOpnd1.GetRegisterNumber()) {
                return;
            }
            Operand &opnd = prevInsn->GetOperand(kInsnSecondOpnd);
            if (opnd.IsIntImmediate()) {
                auto &immOpnd = static_cast<ImmOperand &>(opnd);
                int64 value = immOpnd.GetValue();
                if (thisMop == MOP_xsxtb32) {
                    /* value should in range between -127 and 127 */
                    if (value >= static_cast<int64>(0xFFFFFFFFFFFFFF80) && value <= 0x7F &&
                        immOpnd.IsSingleInstructionMovable(regOpnd0.GetSize())) {
                        bb.RemoveInsn(insn);
                    }
                } else if (thisMop == MOP_xsxth32) {
                    /* value should in range between -32678 and 32678 */
                    if (value >= static_cast<int64>(0xFFFFFFFFFFFF8000) && value <= 0x7FFF &&
                        immOpnd.IsSingleInstructionMovable(regOpnd0.GetSize())) {
                        bb.RemoveInsn(insn);
                    }
                } else {
                    uint64 flag = 0xFFFFFFFFFFFFFF80; /* initialize the flag with fifty-nine 1s at top */
                    if (thisMop == MOP_xsxth64) {
                        flag = 0xFFFFFFFFFFFF8000; /* specify the flag with forty-nine 1s at top in this case */
                    } else if (thisMop == MOP_xsxtw64) {
                        flag = 0xFFFFFFFF80000000; /* specify the flag with thirty-three 1s at top in this case */
                    }
                    if (!(static_cast<uint64>(value) & flag) &&
                        immOpnd.IsSingleInstructionMovable(regOpnd0.GetSize())) {
                        auto *aarch64CGFunc = static_cast<AArch64CGFunc *>(&cgFunc);
                        RegOperand &dstOpnd = aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(
                            static_cast<AArch64reg>(dstMovOpnd.GetRegisterNumber()), k64BitSize,
                            dstMovOpnd.GetRegisterType());
                        prevInsn->SetOperand(kInsnFirstOpnd, dstOpnd);
                        prevInsn->SetMOP(AArch64CG::kMd[MOP_xmovri64]);
                        bb.RemoveInsn(insn);
                    }
                }
            }
        } else if (prevInsn->GetMachineOpcode() == MOP_wldrsb) {
            auto &dstMovOpnd = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
            if (dstMovOpnd.GetRegisterNumber() != regOpnd1.GetRegisterNumber()) {
                return;
            }
            if (thisMop == MOP_xsxtb32) {
                bb.RemoveInsn(insn);
            }
        } else if (prevInsn->GetMachineOpcode() == MOP_wldrsh) {
            auto &dstMovOpnd = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
            if (dstMovOpnd.GetRegisterNumber() != regOpnd1.GetRegisterNumber()) {
                return;
            }
            if (thisMop == MOP_xsxth32) {
                bb.RemoveInsn(insn);
            }
        }
    }
}

void EliminateSpecifcUXTAArch64::Run(BB &bb, Insn &insn)
{
    MOperator thisMop = insn.GetMachineOpcode();
    Insn *prevInsn = insn.GetPreviousMachineInsn();
    if (prevInsn == nullptr) {
        return;
    }
    auto &regOpnd0 = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    auto &regOpnd1 = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    if (prevInsn->IsCall() && prevInsn->GetIsCallReturnUnsigned() &&
        regOpnd0.GetRegisterNumber() == regOpnd1.GetRegisterNumber() &&
        (regOpnd1.GetRegisterNumber() == R0 || regOpnd1.GetRegisterNumber() == V0)) {
        uint32 retSize = prevInsn->GetRetSize();
        if (retSize > 0 &&
            ((thisMop == MOP_xuxtb32 && retSize <= k1ByteSize) || (thisMop == MOP_xuxth32 && retSize <= k2ByteSize) ||
             (thisMop == MOP_xuxtw64 && retSize <= k4ByteSize))) {
            bb.RemoveInsn(insn);
        }
        return;
    }
    if (&insn == bb.GetFirstInsn() || regOpnd0.GetRegisterNumber() != regOpnd1.GetRegisterNumber() ||
        !prevInsn->IsMachineInstruction()) {
        return;
    }
    if (cgFunc.GetMirModule().GetSrcLang() == kSrcLangC && prevInsn->IsCall() && prevInsn->GetIsCallReturnSigned()) {
        return;
    }
    if (thisMop == MOP_xuxtb32) {
        if (prevInsn->GetMachineOpcode() == MOP_wmovri32 || prevInsn->GetMachineOpcode() == MOP_xmovri64) {
            auto &dstMovOpnd = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
            if (!IsSameRegisterOperation(dstMovOpnd, regOpnd1, regOpnd0)) {
                return;
            }
            Operand &opnd = prevInsn->GetOperand(kInsnSecondOpnd);
            if (opnd.IsIntImmediate()) {
                auto &immOpnd = static_cast<ImmOperand &>(opnd);
                int64 value = immOpnd.GetValue();
                /* check the top 56 bits of value */
                if (!(static_cast<uint64>(value) & 0xFFFFFFFFFFFFFF00)) {
                    bb.RemoveInsn(insn);
                }
            }
        } else if (prevInsn->GetMachineOpcode() == MOP_wldrb) {
            auto &dstOpnd = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
            if (dstOpnd.GetRegisterNumber() != regOpnd1.GetRegisterNumber()) {
                return;
            }
            bb.RemoveInsn(insn);
        }
    } else if (thisMop == MOP_xuxth32) {
        if (prevInsn->GetMachineOpcode() == MOP_wmovri32 || prevInsn->GetMachineOpcode() == MOP_xmovri64) {
            auto &dstMovOpnd = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
            if (!IsSameRegisterOperation(dstMovOpnd, regOpnd1, regOpnd0)) {
                return;
            }
            Operand &opnd = prevInsn->GetOperand(kInsnSecondOpnd);
            if (opnd.IsIntImmediate()) {
                auto &immOpnd = static_cast<ImmOperand &>(opnd);
                int64 value = immOpnd.GetValue();
                if (!(static_cast<uint64>(value) & 0xFFFFFFFFFFFF0000)) {
                    bb.RemoveInsn(insn);
                }
            }
        } else if (prevInsn->GetMachineOpcode() == MOP_wldrh) {
            auto &dstOpnd = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
            if (dstOpnd.GetRegisterNumber() != regOpnd1.GetRegisterNumber()) {
                return;
            }
            bb.RemoveInsn(insn);
        }
    } else {
        /* this_mop == MOP_xuxtw64 */
        if (prevInsn->GetMachineOpcode() == MOP_wmovri32 || prevInsn->GetMachineOpcode() == MOP_wldrsb ||
            prevInsn->GetMachineOpcode() == MOP_wldrb || prevInsn->GetMachineOpcode() == MOP_wldrsh ||
            prevInsn->GetMachineOpcode() == MOP_wldrh || prevInsn->GetMachineOpcode() == MOP_wldr) {
            auto &dstOpnd = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
            if (!IsSameRegisterOperation(dstOpnd, regOpnd1, regOpnd0)) {
                return;
            }
            /* 32-bit ldr does zero-extension by default, so this conversion can be skipped */
            bb.RemoveInsn(insn);
        }
    }
}

bool FmovRegPattern::CheckCondition(Insn &insn)
{
    nextInsn = insn.GetNextMachineInsn();
    if (nextInsn == nullptr) {
        return false;
    }
    prevInsn = insn.GetPreviousMachineInsn();
    if (prevInsn == nullptr) {
        return false;
    }
    auto &curSrcOpnd = insn.GetOperand(kInsnSecondOpnd);
    auto &prevSrcOpnd = prevInsn->GetOperand(kInsnSecondOpnd);
    if (!curSrcOpnd.IsRegister() || !prevSrcOpnd.IsRegister()) {
        return false;
    }
    auto &curSrcRegOpnd = static_cast<RegOperand&>(curSrcOpnd);
    auto &prevSrcRegOpnd = static_cast<RegOperand&>(prevSrcOpnd);
    /* same src freg */
    if (curSrcRegOpnd.GetRegisterNumber() != prevSrcRegOpnd.GetRegisterNumber()) {
        return false;
    }
    return true;
}

void FmovRegPattern::Run(BB &bb, Insn &insn)
{
    if (!CheckCondition(insn)) {
        return;
    }
    MOperator thisMop = insn.GetMachineOpcode();
    MOperator prevMop = prevInsn->GetMachineOpcode();
    MOperator newMop;
    uint32 doOpt = 0;
    if (prevMop == MOP_xvmovrv && thisMop == MOP_xvmovrv) {
        doOpt = k32BitSize;
        newMop = MOP_wmovrr;
    } else if (prevMop == MOP_xvmovrd && thisMop == MOP_xvmovrd) {
        doOpt = k64BitSize;
        newMop = MOP_xmovrr;
    }
    if (doOpt == 0) {
        return;
    }
    auto &curDstRegOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    regno_t curDstReg = curDstRegOpnd.GetRegisterNumber();
    /* optimize case 1 */
    auto &prevDstRegOpnd = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
    regno_t prevDstReg = prevDstRegOpnd.GetRegisterNumber();
    auto *aarch64CGFunc = static_cast<AArch64CGFunc *>(cgFunc);
    RegOperand &dst =
        aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(curDstReg), doOpt, kRegTyInt);
    RegOperand &src =
        aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(prevDstReg), doOpt, kRegTyInt);
    Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(newMop, dst, src);
    bb.InsertInsnBefore(insn, newInsn);
    bb.RemoveInsn(insn);
    RegOperand &newOpnd =
        aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(prevDstReg), doOpt, kRegTyInt);
    uint32 opndNum = nextInsn->GetOperandSize();
    for (uint32 opndIdx = 0; opndIdx < opndNum; ++opndIdx) {
        Operand &opnd = nextInsn->GetOperand(opndIdx);
        if (opnd.IsMemoryAccessOperand()) {
            auto &memOpnd = static_cast<MemOperand &>(opnd);
            Operand *base = memOpnd.GetBaseRegister();
            if (base != nullptr) {
                if (base->IsRegister()) {
                    auto *reg = static_cast<RegOperand *>(base);
                    if (reg->GetRegisterNumber() == curDstReg) {
                        memOpnd.SetBaseRegister(newOpnd);
                    }
                }
            }
            Operand *offset = memOpnd.GetIndexRegister();
            if (offset != nullptr) {
                if (offset->IsRegister()) {
                    auto *reg = static_cast<RegOperand *>(offset);
                    if (reg->GetRegisterNumber() == curDstReg) {
                        memOpnd.SetIndexRegister(newOpnd);
                    }
                }
            }
        } else if (opnd.IsRegister()) {
            /* Check if it is a source operand. */
            auto *regProp = nextInsn->GetDesc()->opndMD[opndIdx];
            if (regProp->IsUse()) {
                auto &reg = static_cast<RegOperand &>(opnd);
                if (reg.GetRegisterNumber() == curDstReg) {
                    nextInsn->SetOperand(opndIdx, newOpnd);
                }
            }
        }
    }
}

bool SbfxOptPattern::CheckCondition(Insn &insn)
{
    nextInsn = insn.GetNextMachineInsn();
    if (nextInsn == nullptr) {
        return false;
    }
    auto &curDstRegOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    auto &lsb = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
    auto &width = static_cast<ImmOperand &>(insn.GetOperand(kInsnFourthOpnd));
    if (lsb.GetValue() != 0 || width.GetValue() < k32BitSize) {
        return false;
    }
    uint32 opndNum = nextInsn->GetOperandSize();
    const InsnDesc *md = nextInsn->GetDesc();
    for (uint32 opndIdx = 0; opndIdx < opndNum; ++opndIdx) {
        Operand &opnd = nextInsn->GetOperand(opndIdx);
        /* Check if it is a source operand. */
        if (opnd.IsMemoryAccessOperand() || opnd.IsList()) {
            return false;
        } else if (opnd.IsRegister()) {
            auto &reg = static_cast<RegOperand &>(opnd);
            auto *regProp = md->opndMD[opndIdx];
            if (reg.GetRegisterNumber() == curDstRegOpnd.GetRegisterNumber()) {
                if (nextInsn->GetOperandSize(opndIdx) != k32BitSize) {
                    return false;
                }
                if (regProp->IsDef()) {
                    toRemove = true;
                } else {
                    (void)cands.emplace_back(opndIdx);
                }
            }
        }
    }
    return cands.size() != 0;
}

void SbfxOptPattern::Run(BB &bb, Insn &insn)
{
    if (!CheckCondition(insn)) {
        return;
    }
    auto &srcRegOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    RegOperand &newReg = static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreatePhysicalRegisterOperand(
        static_cast<AArch64reg>(srcRegOpnd.GetRegisterNumber()), k32BitSize, srcRegOpnd.GetRegisterType());
    // replace use point of opnd in nextInsn
    for (auto i : cands) {
        nextInsn->SetOperand(i, newReg);
    }
    if (toRemove) {
        bb.RemoveInsn(insn);
    }
}

bool CbnzToCbzPattern::CheckCondition(Insn &insn)
{
    MOperator curMop = insn.GetMachineOpcode();
    if (curMop != MOP_wcbnz && curMop != MOP_xcbnz) {
        return false;
    }
    /* reg has to be R0, since return value is in R0 */
    auto &regOpnd0 = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    if (regOpnd0.GetRegisterNumber() != R0) {
        return false;
    }
    nextBB = insn.GetBB()->GetNext();
    /* Make sure nextBB can only be reached by bb */
    if (nextBB->GetPreds().size() > 1) {
        return false;
    }
    /* Next insn should be a mov R0 = 0 */
    movInsn = nextBB->GetFirstMachineInsn();
    if (movInsn == nullptr) {
        return false;
    }
    MOperator movInsnMop = movInsn->GetMachineOpcode();
    if (movInsnMop != MOP_wmovri32 && movInsnMop != MOP_xmovri64) {
        return false;
    }
    auto &movDest = static_cast<RegOperand &>(movInsn->GetOperand(kInsnFirstOpnd));
    if (movDest.GetRegisterNumber() != R0) {
        return false;
    }
    auto &movImm = static_cast<ImmOperand &>(movInsn->GetOperand(kInsnSecondOpnd));
    if (movImm.GetValue() != 0) {
        return false;
    }
    Insn *nextBrInsn = movInsn->GetNextMachineInsn();
    if (nextBrInsn == nullptr) {
        return false;
    }
    if (nextBrInsn->GetMachineOpcode() != MOP_xuncond) {
        return false;
    }
    /* Is nextBB branch to the return-bb? */
    if (nextBB->GetSuccs().size() != 1) {
        return false;
    }
    return true;
}

void CbnzToCbzPattern::Run(BB &bb, Insn &insn)
{
    if (!CheckCondition(insn)) {
        return;
    }
    MOperator thisMop = insn.GetMachineOpcode();
    BB *targetBB = nullptr;
    auto it = bb.GetSuccsBegin();
    if (*it == nextBB) {
        ++it;
    }
    targetBB = *it;
    /* Make sure when nextBB is empty, targetBB is fallthru of bb. */
    if (targetBB != nextBB->GetNext()) {
        return;
    }
    BB *nextBBTarget = *(nextBB->GetSuccsBegin());
    if (nextBBTarget->GetKind() != BB::kBBReturn) {
        return;
    }
    /* Control flow looks nice, instruction looks nice */
    DEBUG_ASSERT(brInsn != nullptr, "brInsn should not be nullptr");
    Operand &brTarget = brInsn->GetOperand(kInsnFirstOpnd);
    insn.SetOperand(kInsnSecondOpnd, brTarget);
    if (thisMop == MOP_wcbnz) {
        insn.SetMOP(AArch64CG::kMd[MOP_wcbz]);
    } else {
        insn.SetMOP(AArch64CG::kMd[MOP_xcbz]);
    }
    nextBB->RemoveInsn(*movInsn);
    nextBB->RemoveInsn(*brInsn);
    /* nextBB is now a fallthru bb, not a goto bb */
    nextBB->SetKind(BB::kBBFallthru);
    /*
     * fix control flow, we have bb, nextBB, targetBB, nextBB_target
     * connect bb -> nextBB_target erase targetBB
     */
    it = bb.GetSuccsBegin();
    CHECK_FATAL(it != bb.GetSuccsEnd(), "succs is empty.");
    if (*it == targetBB) {
        bb.EraseSuccs(it);
        bb.PushFrontSuccs(*nextBBTarget);
    } else {
        ++it;
        bb.EraseSuccs(it);
        bb.PushBackSuccs(*nextBBTarget);
    }
    for (auto targetBBIt = targetBB->GetPredsBegin(); targetBBIt != targetBB->GetPredsEnd(); ++targetBBIt) {
        if (*targetBBIt == &bb) {
            targetBB->ErasePreds(targetBBIt);
            break;
        }
    }
    for (auto nextIt = nextBBTarget->GetPredsBegin(); nextIt != nextBBTarget->GetPredsEnd(); ++nextIt) {
        if (*nextIt == nextBB) {
            nextBBTarget->ErasePreds(nextIt);
            break;
        }
    }
    nextBBTarget->PushBackPreds(bb);

    /* nextBB has no target, originally just branch target */
    nextBB->EraseSuccs(nextBB->GetSuccsBegin());
    DEBUG_ASSERT(nextBB->GetSuccs().empty(), "peep: branch target incorrect");
    /* Now make nextBB fallthru to targetBB */
    nextBB->PushFrontSuccs(*targetBB);
    targetBB->PushBackPreds(*nextBB);
}

bool ContiLDRorSTRToSameMEMPattern::HasImplicitSizeUse(const Insn &insn) const
{
    if (insn.GetOperandSize(kInsnFirstOpnd) != prevInsn->GetOperandSize(kInsnFirstOpnd)) {
        return true;
    }
    if (loadAfterStore) {
        // To avoid the optimization as following:
        // str w10, [sp, #8]
        // ldr w10, [sp, #8]     ---\-->  can not be removed
        // ...
        // str x10, [x1, #16]
        auto &defOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
        for (Insn *cursor = insn.GetNext(); cursor != nullptr; cursor = cursor->GetNext()) {
            if (!cursor->IsMachineInstruction()) {
                continue;
            }
            uint32 opndNum = cursor->GetOperandSize();
            for (uint32 i = 0; i < opndNum; ++i) {
                if (cursor->OpndIsDef(i)) {
                    continue;
                }
                if (!cursor->GetOperand(i).IsRegister()) {
                    continue;
                }
                auto &useOpnd = static_cast<RegOperand &>(cursor->GetOperand(i));
                if (useOpnd.GetRegisterNumber() == defOpnd.GetRegisterNumber() &&
                    insn.GetOperandSize(kInsnFirstOpnd) != cursor->GetOperandSize(i)) {
                    return true;
                }
            }
        }
    }
    return false;
}

void CsetCbzToBeqOptAArch64::Run(BB &bb, Insn &insn)
{
    Insn *insn1 = insn.GetPreviousMachineInsn();
    if (insn1 == nullptr) {
        return;
    }
    /* prevInsn must be "cset" insn */
    MOperator opCode1 = insn1->GetMachineOpcode();
    if (opCode1 != MOP_xcsetrc && opCode1 != MOP_wcsetrc) {
        return;
    }

    auto &tmpRegOp1 = static_cast<RegOperand &>(insn1->GetOperand(kInsnFirstOpnd));
    regno_t baseRegNO1 = tmpRegOp1.GetRegisterNumber();
    auto &tmpRegOp2 = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    regno_t baseRegNO2 = tmpRegOp2.GetRegisterNumber();
    if (baseRegNO1 != baseRegNO2) {
        return;
    }
    /* If the reg will be used later, we shouldn't optimize the cset insn here */
    if (IfOperandIsLiveAfterInsn(tmpRegOp2, insn)) {
        return;
    }
    MOperator opCode = insn.GetMachineOpcode();
    bool reverse = (opCode == MOP_xcbz || opCode == MOP_wcbz);
    Operand &rflag = static_cast<AArch64CGFunc *>(&cgFunc)->GetOrCreateRflag();
    auto &label = static_cast<LabelOperand &>(insn.GetOperand(kInsnSecondOpnd));
    auto &cond = static_cast<CondOperand &>(insn1->GetOperand(kInsnSecondOpnd));
    MOperator jmpOperator = SelectMOperator(cond.GetCode(), reverse);
    CHECK_FATAL((jmpOperator != MOP_undef), "unknown condition code");
    Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(jmpOperator, rflag, label);
    bb.RemoveInsn(*insn1);
    bb.ReplaceInsn(insn, newInsn);
}

MOperator CsetCbzToBeqOptAArch64::SelectMOperator(ConditionCode condCode, bool inverse) const
{
    switch (condCode) {
        case CC_NE:
            return inverse ? MOP_beq : MOP_bne;
        case CC_EQ:
            return inverse ? MOP_bne : MOP_beq;
        case CC_MI:
            return inverse ? MOP_bpl : MOP_bmi;
        case CC_PL:
            return inverse ? MOP_bmi : MOP_bpl;
        case CC_VS:
            return inverse ? MOP_bvc : MOP_bvs;
        case CC_VC:
            return inverse ? MOP_bvs : MOP_bvc;
        case CC_HI:
            return inverse ? MOP_bls : MOP_bhi;
        case CC_LS:
            return inverse ? MOP_bhi : MOP_bls;
        case CC_GE:
            return inverse ? MOP_blt : MOP_bge;
        case CC_LT:
            return inverse ? MOP_bge : MOP_blt;
        case CC_HS:
            return inverse ? MOP_blo : MOP_bhs;
        case CC_LO:
            return inverse ? MOP_bhs : MOP_blo;
        case CC_LE:
            return inverse ? MOP_bgt : MOP_ble;
        case CC_GT:
            return inverse ? MOP_ble : MOP_bgt;
        case CC_CS:
            return inverse ? MOP_bcc : MOP_bcs;
        default:
            return MOP_undef;
    }
}

bool ContiLDRorSTRToSameMEMPattern::CheckCondition(Insn &insn)
{
    prevInsn = insn.GetPrev();
    while (prevInsn != nullptr && (prevInsn->GetMachineOpcode() == 0 || !prevInsn->IsMachineInstruction()) &&
           prevInsn != insn.GetBB()->GetFirstMachineInsn()) {
        prevInsn = prevInsn->GetPrev();
    }
    if (!insn.IsMachineInstruction() || prevInsn == nullptr) {
        return false;
    }
    MOperator thisMop = insn.GetMachineOpcode();
    MOperator prevMop = prevInsn->GetMachineOpcode();
    /*
     * store regB, RegC, offset
     * load regA, RegC, offset
     */
    if ((thisMop == MOP_xldr && prevMop == MOP_xstr) || (thisMop == MOP_wldr && prevMop == MOP_wstr) ||
        (thisMop == MOP_dldr && prevMop == MOP_dstr) || (thisMop == MOP_sldr && prevMop == MOP_sstr)) {
        loadAfterStore = true;
    }
    /*
     * load regA, RegC, offset
     * load regB, RegC, offset
     */
    if ((thisMop == MOP_xldr || thisMop == MOP_wldr || thisMop == MOP_dldr || thisMop == MOP_sldr) &&
        prevMop == thisMop) {
        loadAfterLoad = true;
    }
    if (!loadAfterStore && !loadAfterLoad) {
        return false;
    }
    if (HasImplicitSizeUse(insn)) {
        return false;
    }
    DEBUG_ASSERT(insn.GetOperand(kInsnSecondOpnd).IsMemoryAccessOperand(), "expects mem operands");
    DEBUG_ASSERT(prevInsn->GetOperand(kInsnSecondOpnd).IsMemoryAccessOperand(), "expects mem operands");
    return true;
}

void ContiLDRorSTRToSameMEMPattern::Run(BB &bb, Insn &insn)
{
    if (!CheckCondition(insn)) {
        return;
    }
    MOperator thisMop = insn.GetMachineOpcode();
    auto &memOpnd1 = static_cast<MemOperand &>(insn.GetOperand(kInsnSecondOpnd));
    MemOperand::AArch64AddressingMode addrMode1 = memOpnd1.GetAddrMode();
    if (addrMode1 != MemOperand::kAddrModeBOi || (!memOpnd1.IsIntactIndexed())) {
        return;
    }

    auto *base1 = static_cast<RegOperand *>(memOpnd1.GetBaseRegister());
    DEBUG_ASSERT(base1 == nullptr || !base1->IsVirtualRegister(), "physical register has not been allocated?");
    OfstOperand *offset1 = memOpnd1.GetOffsetImmediate();

    auto &memOpnd2 = static_cast<MemOperand &>(prevInsn->GetOperand(kInsnSecondOpnd));
    MemOperand::AArch64AddressingMode addrMode2 = memOpnd2.GetAddrMode();
    if (addrMode2 != MemOperand::kAddrModeBOi || (!memOpnd2.IsIntactIndexed())) {
        return;
    }

    auto *base2 = static_cast<RegOperand *>(memOpnd2.GetBaseRegister());
    DEBUG_ASSERT(base2 == nullptr || !base2->IsVirtualRegister(), "physical register has not been allocated?");
    OfstOperand *offset2 = memOpnd2.GetOffsetImmediate();

    if (base1 == nullptr || base2 == nullptr || offset1 == nullptr || offset2 == nullptr) {
        return;
    }

    auto &reg1 = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    auto &reg2 = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
    int64 offsetVal1 = offset1->GetOffsetValue();
    int64 offsetVal2 = offset2->GetOffsetValue();
    if (base1->GetRegisterNumber() != base2->GetRegisterNumber() || reg1.GetRegisterType() != reg2.GetRegisterType() ||
        reg1.GetSize() != reg2.GetSize() || offsetVal1 != offsetVal2) {
        return;
    }
    if (loadAfterStore && reg1.GetRegisterNumber() != reg2.GetRegisterNumber()) {
        /* replace it with mov */
        MOperator newOp = MOP_undef;
        if (reg1.GetRegisterType() == kRegTyInt) {
            newOp = (insn.GetOperandSize(kInsnFirstOpnd) == k32BitSizeInt) ? MOP_wmovrr : MOP_xmovrr;
        } else if (reg1.GetRegisterType() == kRegTyFloat) {
            newOp = (insn.GetOperandSize(kInsnFirstOpnd) == k32BitSizeInt) ? MOP_xvmovs : MOP_xvmovd;
        }
        Insn *nextInsn = insn.GetNext();
        while (nextInsn != nullptr && !nextInsn->IsMachineInstruction() && nextInsn != bb.GetLastMachineInsn()) {
            nextInsn = nextInsn->GetNext();
        }
        bool moveSameReg = false;
        if (nextInsn && nextInsn->GetIsSpill() && !IfOperandIsLiveAfterInsn(reg1, *nextInsn)) {
            MOperator nextMop = nextInsn->GetMachineOpcode();
            if ((thisMop == MOP_xldr && nextMop == MOP_xstr) || (thisMop == MOP_wldr && nextMop == MOP_wstr) ||
                (thisMop == MOP_dldr && nextMop == MOP_dstr) || (thisMop == MOP_sldr && nextMop == MOP_sstr)) {
                nextInsn->Insn::SetOperand(kInsnFirstOpnd, reg2);
                moveSameReg = true;
            }
        }
        if (!moveSameReg) {
            bb.InsertInsnAfter(*prevInsn, cgFunc->GetInsnBuilder()->BuildInsn(newOp, reg1, reg2));
        }
        SetCurrInsn(insn.GetNextMachineInsn());
        optSuccess = true;
        bb.RemoveInsn(insn);
    } else if (reg1.GetRegisterNumber() == reg2.GetRegisterNumber() &&
               base1->GetRegisterNumber() != reg2.GetRegisterNumber()) {
        SetCurrInsn(insn.GetNextMachineInsn());
        optSuccess = true;
        bb.RemoveInsn(insn);
    }
}

void CselZeroOneToCsetOpt::Run(BB &bb, Insn &insn)
{
    Operand &trueValueOp = insn.GetOperand(kInsnSecondOpnd);
    Operand &falseValueOp = insn.GetOperand(kInsnThirdOpnd);
    Operand *trueTempOp = nullptr;
    Operand *falseTempOp = nullptr;

    /* find fixed value in BB */
    if (!trueValueOp.IsIntImmediate()) {
        trueMovInsn = FindFixedValue(trueValueOp, bb, trueTempOp, insn);
    }
    if (!falseValueOp.IsIntImmediate()) {
        falseMovInsn = FindFixedValue(falseValueOp, bb, falseTempOp, insn);
    }

    DEBUG_ASSERT(trueTempOp != nullptr, "trueTempOp should not be nullptr");
    DEBUG_ASSERT(falseTempOp != nullptr, "falseTempOp should not be nullptr");
    /* csel to cset */
    if ((trueTempOp->IsIntImmediate() || IsZeroRegister(*trueTempOp)) &&
        (falseTempOp->IsIntImmediate() || IsZeroRegister(*falseTempOp))) {
        ImmOperand *imm1 = static_cast<ImmOperand *>(trueTempOp);
        ImmOperand *imm2 = static_cast<ImmOperand *>(falseTempOp);
        bool inverse = imm1->IsOne() && (imm2->IsZero() || IsZeroRegister(*imm2));
        if (inverse || ((imm1->IsZero() || IsZeroRegister(*imm1)) && imm2->IsOne())) {
            Operand &reg = insn.GetOperand(kInsnFirstOpnd);
            CondOperand &condOperand = static_cast<CondOperand &>(insn.GetOperand(kInsnFourthOpnd));
            MOperator mopCode = (reg.GetSize() == k64BitSize) ? MOP_xcsetrc : MOP_wcsetrc;
            /* get new cond  ccCode */
            ConditionCode ccCode = inverse ? condOperand.GetCode() : GetReverseCC(condOperand.GetCode());
            if (ccCode == kCcLast) {
                return;
            }
            AArch64CGFunc *func = static_cast<AArch64CGFunc *>(cgFunc);
            CondOperand &cond = func->GetCondOperand(ccCode);
            Operand &rflag = func->GetOrCreateRflag();
            Insn &csetInsn = func->GetInsnBuilder()->BuildInsn(mopCode, reg, cond, rflag);
            if (CGOptions::DoCGSSA() && CGOptions::GetInstance().GetOptimizeLevel() < CGOptions::kLevel0) {
                CHECK_FATAL(false, "check this case in ssa opt");
            }
            insn.GetBB()->ReplaceInsn(insn, csetInsn);
        }
    }
}

Insn *CselZeroOneToCsetOpt::FindFixedValue(Operand &opnd, BB &bb, Operand *&tempOp, const Insn &insn) const
{
    tempOp = &opnd;
    bool alreadyFindCsel = false;
    bool isRegDefined = false;
    regno_t regno = static_cast<RegOperand &>(opnd).GetRegisterNumber();
    FOR_BB_INSNS_REV(defInsn, &bb)
    {
        if (!defInsn->IsMachineInstruction() || defInsn->IsBranch()) {
            continue;
        }
        /* find csel */
        if (defInsn->GetId() == insn.GetId()) {
            alreadyFindCsel = true;
        }
        /* find def defined */
        if (alreadyFindCsel) {
            isRegDefined = defInsn->IsRegDefined(regno);
        }
        /* if def defined is movi do this opt */
        if (isRegDefined) {
            MOperator thisMop = defInsn->GetMachineOpcode();
            if (thisMop == MOP_wmovri32 || thisMop == MOP_xmovri64) {
                if (&defInsn->GetOperand(kInsnFirstOpnd) == &opnd) {
                    tempOp = &(defInsn->GetOperand(kInsnSecondOpnd));
                    return defInsn;
                }
            } else {
                return nullptr;
            }
        }
    }
    return nullptr;
}

void AndCmpCsetEorCbzOpt::Run(BB &bb, Insn &insn)
{
    if (insn.GetMachineOpcode() != MOP_wandrri12) {
        return;
    }
    RegOperand &andInsnFirstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    RegOperand &andInsnSecondOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    ImmOperand &andInsnThirdOpnd = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
    if (andInsnFirstOpnd.GetRegisterNumber() != andInsnSecondOpnd.GetRegisterNumber() ||
        andInsnThirdOpnd.GetValue() != 1) {
        return;
    }
    Insn *cmpInsn = insn.GetNextMachineInsn();
    if (cmpInsn == nullptr || cmpInsn->GetMachineOpcode() != MOP_wcmpri) {
        return;
    }
    RegOperand &cmpInsnSecondOpnd = static_cast<RegOperand &>(cmpInsn->GetOperand(kInsnSecondOpnd));
    ImmOperand &cmpInsnThirdOpnd = static_cast<ImmOperand &>(cmpInsn->GetOperand(kInsnThirdOpnd));
    if (cmpInsnSecondOpnd.GetRegisterNumber() != andInsnFirstOpnd.GetRegisterNumber() ||
        cmpInsnThirdOpnd.GetValue() != 0) {
        return;
    }
    Insn *csetInsn = cmpInsn->GetNextMachineInsn();
    if (csetInsn == nullptr || csetInsn->GetMachineOpcode() != MOP_wcsetrc) {
        return;
    }
    RegOperand &csetInsnFirstOpnd = static_cast<RegOperand &>(csetInsn->GetOperand(kInsnFirstOpnd));
    CondOperand &csetSecondOpnd = static_cast<CondOperand &>(csetInsn->GetOperand(kInsnSecondOpnd));
    if (csetInsnFirstOpnd.GetRegisterNumber() != andInsnFirstOpnd.GetRegisterNumber() ||
        csetSecondOpnd.GetCode() != CC_EQ) {
        return;
    }
    Insn *eorInsn = csetInsn->GetNextMachineInsn();
    if (eorInsn == nullptr || eorInsn->GetMachineOpcode() != MOP_weorrri12) {
        return;
    }
    RegOperand &eorInsnFirstOpnd = static_cast<RegOperand &>(eorInsn->GetOperand(kInsnFirstOpnd));
    RegOperand &eorInsnSecondOpnd = static_cast<RegOperand &>(eorInsn->GetOperand(kInsnSecondOpnd));
    ImmOperand &eorInsnThirdOpnd = static_cast<ImmOperand &>(eorInsn->GetOperand(kInsnThirdOpnd));
    if (eorInsnFirstOpnd.GetRegisterNumber() != andInsnFirstOpnd.GetRegisterNumber() ||
        eorInsnFirstOpnd.GetRegisterNumber() != eorInsnSecondOpnd.GetRegisterNumber() ||
        eorInsnThirdOpnd.GetValue() != 1) {
        return;
    }
    Insn *cbzInsn = eorInsn->GetNextMachineInsn();
    if (cbzInsn == nullptr || cbzInsn->GetMachineOpcode() != MOP_wcbz) {
        return;
    }
    RegOperand &cbzInsnFirstOpnd = static_cast<RegOperand &>(cbzInsn->GetOperand(kInsnFirstOpnd));
    if (cbzInsnFirstOpnd.GetRegisterNumber() != andInsnFirstOpnd.GetRegisterNumber()) {
        return;
    }
    bb.RemoveInsn(*cmpInsn);
    bb.RemoveInsn(*csetInsn);
    bb.RemoveInsn(*eorInsn);
    bb.RemoveInsn(*cbzInsn);
    /* replace insn */
    auto &label = static_cast<LabelOperand &>(cbzInsn->GetOperand(kInsnSecondOpnd));
    ImmOperand &oneHoleOpnd = static_cast<AArch64CGFunc *>(cgFunc)->CreateImmOperand(0, k8BitSize, false);
    bb.ReplaceInsn(insn, cgFunc->GetInsnBuilder()->BuildInsn(MOP_wtbz, cbzInsnFirstOpnd, oneHoleOpnd, label));
}

void AddLdrOpt::Run(BB &bb, Insn &insn)
{
    if (insn.GetMachineOpcode() != MOP_xaddrrr) {
        return;
    }
    Insn *nextInsn = insn.GetNextMachineInsn();
    if (nextInsn == nullptr) {
        return;
    }
    auto nextMop = nextInsn->GetMachineOpcode();
    if (nextMop != MOP_xldr && nextMop != MOP_wldr) {
        return;
    }
    RegOperand &insnFirstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    RegOperand &insnSecondOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    if (insnFirstOpnd.GetRegisterNumber() != insnSecondOpnd.GetRegisterNumber()) {
        return;
    }
    RegOperand &ldrInsnFirstOpnd = static_cast<RegOperand &>(nextInsn->GetOperand(kInsnFirstOpnd));
    MemOperand &memOpnd = static_cast<MemOperand &>(nextInsn->GetOperand(kInsnSecondOpnd));
    if (memOpnd.GetAddrMode() != MemOperand::kAddrModeBOi ||
        memOpnd.GetBaseRegister()->GetRegisterNumber() != insnFirstOpnd.GetRegisterNumber() ||
        ldrInsnFirstOpnd.GetRegisterNumber() != insnFirstOpnd.GetRegisterNumber() ||
        memOpnd.GetOffsetImmediate()->GetOffsetValue() != 0) {
        return;
    }
    MemOperand &newMemOpnd = static_cast<AArch64CGFunc *>(cgFunc)->GetOrCreateMemOpnd(
        MemOperand::kAddrModeBOrX, memOpnd.GetSize(), &insnFirstOpnd,
        &static_cast<RegOperand &>(insn.GetOperand(kInsnThirdOpnd)), 0, false);
    nextInsn->SetOperand(kInsnSecondOpnd, newMemOpnd);
    bb.RemoveInsn(insn);
}

void CsetEorOpt::Run(BB &bb, Insn &insn)
{
    if (insn.GetMachineOpcode() != MOP_xcsetrc && insn.GetMachineOpcode() != MOP_wcsetrc) {
        return;
    }
    Insn *nextInsn = insn.GetNextMachineInsn();
    if (nextInsn == nullptr ||
        (nextInsn->GetMachineOpcode() != MOP_weorrri12 && nextInsn->GetMachineOpcode() != MOP_xeorrri13)) {
        return;
    }
    RegOperand &csetFirstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    RegOperand &eorFirstOpnd = static_cast<RegOperand &>(nextInsn->GetOperand(kInsnFirstOpnd));
    RegOperand &eorSecondOpnd = static_cast<RegOperand &>(nextInsn->GetOperand(kInsnSecondOpnd));
    ImmOperand &eorThirdOpnd = static_cast<ImmOperand &>(nextInsn->GetOperand(kInsnThirdOpnd));
    if (eorThirdOpnd.GetValue() != 1 || eorFirstOpnd.GetRegisterNumber() != eorSecondOpnd.GetRegisterNumber() ||
        csetFirstOpnd.GetRegisterNumber() != eorFirstOpnd.GetRegisterNumber()) {
        return;
    }
    CondOperand &csetSecondOpnd = static_cast<CondOperand &>(insn.GetOperand(kInsnSecondOpnd));
    ConditionCode inverseCondCode = GetReverseCC(csetSecondOpnd.GetCode());
    if (inverseCondCode == kCcLast) {
        return;
    }
    auto *aarFunc = static_cast<AArch64CGFunc *>(cgFunc);
    CondOperand &inverseCondOpnd = aarFunc->GetCondOperand(inverseCondCode);
    insn.SetOperand(kInsnSecondOpnd, inverseCondOpnd);
    bb.RemoveInsn(*nextInsn);
}

void MoveCmpOpt::Run(BB &bb, Insn &insn)
{
    if (insn.GetMachineOpcode() != MOP_xmovri64 && insn.GetMachineOpcode() != MOP_wmovri32) {
        return;
    }
    ImmOperand &immOpnd = static_cast<ImmOperand &>(insn.GetOperand(kInsnSecondOpnd));
    if (!immOpnd.IsInBitSize(kMaxImmVal12Bits, 0) && !immOpnd.IsInBitSize(kMaxImmVal12Bits, kMaxImmVal12Bits)) {
        return;
    }
    Insn *nextInsn = insn.GetNextMachineInsn();
    if (nextInsn == nullptr ||
        (nextInsn->GetMachineOpcode() != MOP_wcmprr && nextInsn->GetMachineOpcode() != MOP_xcmprr)) {
        return;
    }
    RegOperand &cmpSecondOpnd = static_cast<RegOperand &>(nextInsn->GetOperand(kInsnSecondOpnd));
    RegOperand &cmpThirdOpnd = static_cast<RegOperand &>(nextInsn->GetOperand(kInsnThirdOpnd));
    RegOperand &movFirstOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    if (cmpSecondOpnd.GetRegisterNumber() == cmpThirdOpnd.GetRegisterNumber()) {
        return;
    }
    if (cmpThirdOpnd.GetRegisterNumber() != movFirstOpnd.GetRegisterNumber()) {
        return;
    }
    MOperator cmpOpCode = (cmpThirdOpnd.GetSize() == k64BitSize) ? MOP_xcmpri : MOP_wcmpri;
    Insn &newCmpInsn = cgFunc->GetInsnBuilder()->BuildInsn(cmpOpCode, nextInsn->GetOperand(kInsnFirstOpnd),
                                                           nextInsn->GetOperand(kInsnSecondOpnd), immOpnd);
    bb.ReplaceInsn(*nextInsn, newCmpInsn);
    if (!IfOperandIsLiveAfterInsn(movFirstOpnd, newCmpInsn)) {
        bb.RemoveInsn(insn);
    }
}

void AddImmZeroToMov::Run(BB &bb, Insn &insn)
{
    RegOperand *insnDefReg = &static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    RegOperand *insnUseReg = &static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    int64 immVal = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd)).GetValue();
    if (immVal == static_cast<int64>(k0BitSize)) {
        if (insnDefReg->GetRegisterNumber() == insnUseReg->GetRegisterNumber()) {
            bb.RemoveInsn(insn);
            return;
        } else {
            Insn *newInsn = &cgFunc->GetInsnBuilder()->BuildInsn(MOP_xmovrr, *insnDefReg, *insnUseReg);
            bb.ReplaceInsn(insn, *newInsn);
            return;
        }
    }
    return;
}

Insn *AndCmpBranchesToCsetAArch64::FindPreviousCmp(Insn &insn) const
{
    regno_t defRegNO = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
    for (Insn *curInsn = insn.GetPrev(); curInsn != nullptr; curInsn = curInsn->GetPrev()) {
        if (!curInsn->IsMachineInstruction()) {
            continue;
        }
        if (curInsn->GetMachineOpcode() == MOP_wcmpri || curInsn->GetMachineOpcode() == MOP_xcmpri) {
            return curInsn;
        }
        /*
         * if any def/use of CC or insn defReg between insn and curInsn, stop searching and return nullptr.
         */
        if (curInsn->ScanReg(defRegNO) || curInsn->ScanReg(kRFLAG)) {
            return nullptr;
        }
    }
    return nullptr;
}

void AndCmpBranchesToCsetAArch64::Run(BB &bb, Insn &insn)
{
    /* prevInsn must be "cmp" insn */
    Insn *prevInsn = FindPreviousCmp(insn);
    if (prevInsn == nullptr) {
        return;
    }
    /* prevPrevInsn must be "and" insn */
    Insn *prevPrevInsn = prevInsn->GetPreviousMachineInsn();
    if (prevPrevInsn == nullptr ||
        (prevPrevInsn->GetMachineOpcode() != MOP_wandrri12 && prevPrevInsn->GetMachineOpcode() != MOP_xandrri13)) {
        return;
    }

    auto &csetCond = static_cast<CondOperand &>(insn.GetOperand(kInsnSecondOpnd));
    auto &cmpImm = static_cast<ImmOperand &>(prevInsn->GetOperand(kInsnThirdOpnd));
    int64 cmpImmVal = cmpImm.GetValue();
    auto &andImm = static_cast<ImmOperand &>(prevPrevInsn->GetOperand(kInsnThirdOpnd));
    int64 andImmVal = andImm.GetValue();
    if ((csetCond.GetCode() == CC_EQ && cmpImmVal == andImmVal) || (csetCond.GetCode() == CC_NE && cmpImmVal == 0)) {
        /* if flag_reg of "cmp" is live later, we can't remove cmp insn. */
        auto &flagReg = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
        if (IfOperandIsLiveAfterInsn(flagReg, insn)) {
            return;
        }

        auto &csetReg = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
        auto &prevInsnSecondReg = prevInsn->GetOperand(kInsnSecondOpnd);
        bool isRegDiff = !RegOperand::IsSameRegNO(csetReg, prevInsnSecondReg);
        if (isRegDiff && IfOperandIsLiveAfterInsn(static_cast<RegOperand &>(prevInsnSecondReg), insn)) {
            return;
        }
        if (andImmVal == 1) {
            if (!RegOperand::IsSameRegNO(prevInsnSecondReg, prevPrevInsn->GetOperand(kInsnFirstOpnd))) {
                return;
            }
            /* save the "and" insn only. */
            bb.RemoveInsn(insn);
            bb.RemoveInsn(*prevInsn);
            if (isRegDiff) {
                prevPrevInsn->Insn::SetOperand(kInsnFirstOpnd, csetReg);
            }
        } else {
            if (!RegOperand::IsSameReg(prevInsnSecondReg, prevPrevInsn->GetOperand(kInsnFirstOpnd)) ||
                !RegOperand::IsSameReg(prevInsnSecondReg, prevPrevInsn->GetOperand(kInsnSecondOpnd))) {
                return;
            }

            /* andImmVal is n power of 2 */
            int n = LogValueAtBase2(andImmVal);
            if (n < 0) {
                return;
            }

            /* create ubfx insn */
            MOperator ubfxOp = (csetReg.GetSize() <= k32BitSize) ? MOP_wubfxrri5i5 : MOP_xubfxrri6i6;
            if (ubfxOp == MOP_wubfxrri5i5 && static_cast<uint32>(n) >= k32BitSize) {
                return;
            }
            auto &dstReg = static_cast<RegOperand &>(csetReg);
            auto &srcReg = static_cast<RegOperand &>(prevInsnSecondReg);
            auto *aarch64CGFunc = static_cast<AArch64CGFunc *>(&cgFunc);
            ImmOperand &bitPos = aarch64CGFunc->CreateImmOperand(n, k8BitSize, false);
            ImmOperand &bitSize = aarch64CGFunc->CreateImmOperand(1, k8BitSize, false);
            Insn &ubfxInsn = cgFunc.GetInsnBuilder()->BuildInsn(ubfxOp, dstReg, srcReg, bitPos, bitSize);
            bb.InsertInsnBefore(*prevPrevInsn, ubfxInsn);
            bb.RemoveInsn(insn);
            bb.RemoveInsn(*prevInsn);
            bb.RemoveInsn(*prevPrevInsn);
        }
    }
}

void AndCbzBranchesToTstAArch64::Run(BB &bb, Insn &insn)
{
    /* nextInsn must be "cbz" or "cbnz" insn */
    Insn *nextInsn = insn.GetNextMachineInsn();
    if (nextInsn == nullptr || (nextInsn->GetMachineOpcode() != MOP_wcbz && nextInsn->GetMachineOpcode() != MOP_xcbz)) {
        return;
    }
    auto &andRegOp = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    regno_t andRegNO1 = andRegOp.GetRegisterNumber();
    auto &cbzRegOp2 = static_cast<RegOperand &>(nextInsn->GetOperand(kInsnFirstOpnd));
    regno_t cbzRegNO2 = cbzRegOp2.GetRegisterNumber();
    if (andRegNO1 != cbzRegNO2) {
        return;
    }
    /* If the reg will be used later, we shouldn't optimize the and insn here */
    if (IfOperandIsLiveAfterInsn(andRegOp, *nextInsn)) {
        return;
    }
    /* build tst insn */
    Operand &andOpnd3 = insn.GetOperand(kInsnThirdOpnd);
    auto &andRegOp2 = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    MOperator newTstOp = MOP_undef;
    if (andOpnd3.IsRegister()) {
        newTstOp = (andRegOp2.GetSize() <= k32BitSize && andOpnd3.GetSize() <= k32BitSize) ? MOP_wtstrr : MOP_xtstrr;
    } else {
        newTstOp =
            (andRegOp2.GetSize() <= k32BitSize && andOpnd3.GetSize() <= k32BitSize) ? MOP_wtstri32 : MOP_xtstri64;
    }
    Operand &rflag = static_cast<AArch64CGFunc *>(&cgFunc)->GetOrCreateRflag();
    Insn &newInsnTst = cgFunc.GetInsnBuilder()->BuildInsn(newTstOp, rflag, andRegOp2, andOpnd3);
    if (andOpnd3.IsImmediate()) {
        if (!static_cast<ImmOperand &>(andOpnd3).IsBitmaskImmediate(andRegOp2.GetSize())) {
            return;
        }
    }
    /* build beq insn */
    MOperator opCode = nextInsn->GetMachineOpcode();
    bool reverse = (opCode == MOP_xcbz || opCode == MOP_wcbz);
    auto &label = static_cast<LabelOperand &>(nextInsn->GetOperand(kInsnSecondOpnd));
    MOperator jmpOperator = reverse ? MOP_beq : MOP_bne;
    Insn &newInsnJmp = cgFunc.GetInsnBuilder()->BuildInsn(jmpOperator, rflag, label);
    bb.ReplaceInsn(insn, newInsnTst);
    bb.ReplaceInsn(*nextInsn, newInsnJmp);
}

void AndCmpBranchesToTstAArch64::Run(BB &bb, Insn &insn)
{
    /* nextInsn must be "cmp" insn */
    Insn *nextInsn = insn.GetNextMachineInsn();
    if (nextInsn == nullptr ||
        (nextInsn->GetMachineOpcode() != MOP_wcmpri && nextInsn->GetMachineOpcode() != MOP_xcmpri)) {
        return;
    }
    /* nextNextInsn must be "beq" or "bne" insn */
    Insn *nextNextInsn = nextInsn->GetNextMachineInsn();
    if (nextNextInsn == nullptr ||
        (nextNextInsn->GetMachineOpcode() != MOP_beq && nextNextInsn->GetMachineOpcode() != MOP_bne)) {
        return;
    }
    auto &andRegOp = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    regno_t andRegNO1 = andRegOp.GetRegisterNumber();
    auto &cmpRegOp2 = static_cast<RegOperand &>(nextInsn->GetOperand(kInsnSecondOpnd));
    regno_t cmpRegNO2 = cmpRegOp2.GetRegisterNumber();
    if (andRegNO1 != cmpRegNO2) {
        return;
    }
    /* If the reg will be used later, we shouldn't optimize the and insn here */
    if (IfOperandIsLiveAfterInsn(andRegOp, *nextInsn)) {
        return;
    }
    Operand &immOpnd = nextInsn->GetOperand(kInsnThirdOpnd);
    DEBUG_ASSERT(immOpnd.IsIntImmediate(), "expects ImmOperand");
    auto &defConst = static_cast<ImmOperand &>(immOpnd);
    int64 defConstValue = defConst.GetValue();
    if (defConstValue != 0) {
        return;
    }
    /* build tst insn */
    Operand &andOpnd3 = insn.GetOperand(kInsnThirdOpnd);
    auto &andRegOp2 = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    MOperator newOp = MOP_undef;
    if (andOpnd3.IsRegister()) {
        newOp = (andRegOp2.GetSize() <= k32BitSize) ? MOP_wtstrr : MOP_xtstrr;
    } else {
        newOp = (andRegOp2.GetSize() <= k32BitSize) ? MOP_wtstri32 : MOP_xtstri64;
    }
    Operand &rflag = static_cast<AArch64CGFunc *>(&cgFunc)->GetOrCreateRflag();
    Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(newOp, rflag, andRegOp2, andOpnd3);
    if (CGOptions::DoCGSSA() && CGOptions::GetInstance().GetOptimizeLevel() < CGOptions::kLevel0) {
        CHECK_FATAL(false, "check this case in ssa opt");
    }
    bb.InsertInsnAfter(*nextInsn, newInsn);
    bb.RemoveInsn(insn);
    bb.RemoveInsn(*nextInsn);
}

void ZeroCmpBranchesAArch64::Run(BB &bb, Insn &insn)
{
    Insn *prevInsn = insn.GetPreviousMachineInsn();
    if (!insn.IsBranch() || insn.GetOperandSize() <= kInsnSecondOpnd || prevInsn == nullptr) {
        return;
    }
    if (!insn.GetOperand(kInsnSecondOpnd).IsLabel()) {
        return;
    }
    LabelOperand *label = &static_cast<LabelOperand &>(insn.GetOperand(kInsnSecondOpnd));
    RegOperand *regOpnd = nullptr;
    RegOperand *reg0 = nullptr;
    RegOperand *reg1 = nullptr;
    MOperator newOp = MOP_undef;
    ImmOperand *imm = nullptr;
    switch (prevInsn->GetMachineOpcode()) {
        case MOP_wcmpri:
        case MOP_xcmpri: {
            regOpnd = &static_cast<RegOperand &>(prevInsn->GetOperand(kInsnSecondOpnd));
            imm = &static_cast<ImmOperand &>(prevInsn->GetOperand(kInsnThirdOpnd));
            if (imm->GetValue() != 0) {
                return;
            }
            if (insn.GetMachineOpcode() == MOP_bge) {
                newOp = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbz : MOP_xtbz;
            } else if (insn.GetMachineOpcode() == MOP_blt) {
                newOp = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbnz : MOP_xtbnz;
            } else {
                return;
            }
            break;
        }
        case MOP_wcmprr:
        case MOP_xcmprr: {
            reg0 = &static_cast<RegOperand &>(prevInsn->GetOperand(kInsnSecondOpnd));
            reg1 = &static_cast<RegOperand &>(prevInsn->GetOperand(kInsnThirdOpnd));
            if (!IsZeroRegister(*reg0) && !IsZeroRegister(*reg1)) {
                return;
            }
            switch (insn.GetMachineOpcode()) {
                case MOP_bge:
                    if (IsZeroRegister(*reg1)) {
                        regOpnd = &static_cast<RegOperand &>(prevInsn->GetOperand(kInsnSecondOpnd));
                        newOp = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbz : MOP_xtbz;
                    } else {
                        return;
                    }
                    break;
                case MOP_ble:
                    if (IsZeroRegister(*reg0)) {
                        regOpnd = &static_cast<RegOperand &>(prevInsn->GetOperand(kInsnThirdOpnd));
                        newOp = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbz : MOP_xtbz;
                    } else {
                        return;
                    }
                    break;
                case MOP_blt:
                    if (IsZeroRegister(*reg1)) {
                        regOpnd = &static_cast<RegOperand &>(prevInsn->GetOperand(kInsnSecondOpnd));
                        newOp = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbnz : MOP_xtbnz;
                    } else {
                        return;
                    }
                    break;
                case MOP_bgt:
                    if (IsZeroRegister(*reg0)) {
                        regOpnd = &static_cast<RegOperand &>(prevInsn->GetOperand(kInsnThirdOpnd));
                        newOp = (regOpnd->GetSize() <= k32BitSize) ? MOP_wtbnz : MOP_xtbnz;
                    } else {
                        return;
                    }
                    break;
                default:
                    return;
            }
            break;
        }
        default:
            return;
    }
    auto aarch64CGFunc = static_cast<AArch64CGFunc *>(&cgFunc);
    ImmOperand &bitp = aarch64CGFunc->CreateImmOperand(
        (regOpnd->GetSize() <= k32BitSize) ? (k32BitSize - 1) : (k64BitSize - 1), k8BitSize, false);
    bb.InsertInsnAfter(insn,
                       cgFunc.GetInsnBuilder()->BuildInsn(newOp, *static_cast<RegOperand *>(regOpnd), bitp, *label));
    bb.RemoveInsn(insn);
    bb.RemoveInsn(*prevInsn);
}

/*
 * if there is define point of checkInsn->GetOperand(opndIdx) between startInsn and  firstInsn
 * return define insn. else return nullptr
 */
const Insn *CmpCsetAArch64::DefInsnOfOperandInBB(const Insn &startInsn, const Insn &checkInsn, int opndIdx) const
{
    Insn *prevInsn = nullptr;
    for (const Insn *insn = &startInsn; insn != nullptr; insn = prevInsn) {
        prevInsn = insn->GetPreviousMachineInsn();
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        /* checkInsn.GetOperand(opndIdx) is thought modified conservatively */
        if (insn->IsCall()) {
            return insn;
        }
        const InsnDesc *md = insn->GetDesc();
        uint32 opndNum = insn->GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            Operand &opnd = insn->GetOperand(i);
            if (!md->opndMD[i]->IsDef()) {
                continue;
            }
            /* Operand is base reg of Memory, defined by str */
            if (opnd.IsMemoryAccessOperand()) {
                auto &memOpnd = static_cast<MemOperand &>(opnd);
                RegOperand *base = memOpnd.GetBaseRegister();
                DEBUG_ASSERT(base != nullptr, "nullptr check");
                DEBUG_ASSERT(base->IsRegister(), "expects RegOperand");
                if (RegOperand::IsSameRegNO(*base, checkInsn.GetOperand(static_cast<uint32>(opndIdx))) &&
                    memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi &&
                    (memOpnd.IsPostIndexed() || memOpnd.IsPreIndexed())) {
                    return insn;
                }
            } else {
                DEBUG_ASSERT(opnd.IsRegister(), "expects RegOperand");
                if (RegOperand::IsSameRegNO(checkInsn.GetOperand(static_cast<uint32>(opndIdx)), opnd)) {
                    return insn;
                }
            }
        }
    }
    return nullptr;
}

bool CmpCsetAArch64::OpndDefByOneValidBit(const Insn &defInsn) const
{
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

/*
 * help function for cmpcset optimize
 * if all define points of used opnd in insn has only one valid bit,return true.
 * for cmp reg,#0(#1), that is checking for reg
 */
bool CmpCsetAArch64::CheckOpndDefPoints(Insn &checkInsn, int opndIdx)
{
    if (checkInsn.GetBB()->GetPrev() == nullptr) {
        /* For 1st BB, be conservative for def of parameter registers */
        /* Since peep is light weight, do not want to insert pseudo defs */
        regno_t reg = static_cast<RegOperand &>(checkInsn.GetOperand(static_cast<uint32>(opndIdx))).GetRegisterNumber();
        if ((reg >= R0 && reg <= R7) || (reg >= D0 && reg <= D7)) {
            return false;
        }
    }
    /* check current BB */
    const Insn *defInsn = DefInsnOfOperandInBB(checkInsn, checkInsn, opndIdx);
    if (defInsn != nullptr) {
        return OpndDefByOneValidBit(*defInsn);
    }
    /* check pred */
    for (auto predBB : checkInsn.GetBB()->GetPreds()) {
        const Insn *tempInsn = nullptr;
        if (predBB->GetLastInsn() != nullptr) {
            tempInsn = DefInsnOfOperandInBB(*predBB->GetLastInsn(), checkInsn, opndIdx);
        }
        if (tempInsn == nullptr || !OpndDefByOneValidBit(*tempInsn)) {
            return false;
        }
    }
    return true;
}

/* Check there is use point of rflag start from startInsn to current bb bottom */
bool CmpCsetAArch64::FlagUsedLaterInCurBB(const BB &bb, Insn &startInsn) const
{
    if (&bb != startInsn.GetBB()) {
        return false;
    }
    Insn *nextInsn = nullptr;
    for (Insn *insn = &startInsn; insn != nullptr; insn = nextInsn) {
        nextInsn = insn->GetNextMachineInsn();
        const InsnDesc *md = insn->GetDesc();
        uint32 opndNum = insn->GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            Operand &opnd = insn->GetOperand(i);
            /*
             * For condition operand, such as NE, EQ and so on, the register number should be
             * same with RFLAG, we only need check the property of use/def.
             */
            if (!opnd.IsConditionCode()) {
                continue;
            }
            if (md->opndMD[i]->IsUse()) {
                return true;
            } else {
                DEBUG_ASSERT(md->opndMD[i]->IsDef(), "register should be redefined.");
                return false;
            }
        }
    }
    return false;
}

void CmpCsetAArch64::Run(BB &bb, Insn &insn)
{
    Insn *nextInsn = insn.GetNextMachineInsn();
    if (nextInsn == nullptr) {
        return;
    }
    MOperator firstMop = insn.GetMachineOpcode();
    MOperator secondMop = nextInsn->GetMachineOpcode();
    if ((firstMop == MOP_wcmpri || firstMop == MOP_xcmpri) && (secondMop == MOP_wcsetrc || secondMop == MOP_xcsetrc)) {
        Operand &cmpFirstOpnd = insn.GetOperand(kInsnSecondOpnd);
        /* get ImmOperand, must be 0 or 1 */
        Operand &cmpSecondOpnd = insn.GetOperand(kInsnThirdOpnd);
        auto &cmpFlagReg = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
        DEBUG_ASSERT(cmpSecondOpnd.IsIntImmediate(), "expects ImmOperand");
        auto &cmpConst = static_cast<ImmOperand &>(cmpSecondOpnd);
        int64 cmpConstVal = cmpConst.GetValue();
        Operand &csetFirstOpnd = nextInsn->GetOperand(kInsnFirstOpnd);
        if ((cmpConstVal != 0 && cmpConstVal != 1) || !CheckOpndDefPoints(insn, 1) ||
            (nextInsn->GetNextMachineInsn() != nullptr && FlagUsedLaterInCurBB(bb, *nextInsn->GetNextMachineInsn())) ||
            FindRegLiveOut(cmpFlagReg, *insn.GetBB())) {
            return;
        }

        Insn *csetInsn = nextInsn;
        nextInsn = nextInsn->GetNextMachineInsn();
        auto &cond = static_cast<CondOperand &>(csetInsn->GetOperand(kInsnSecondOpnd));
        if ((cmpConstVal == 0 && cond.GetCode() == CC_NE) || (cmpConstVal == 1 && cond.GetCode() == CC_EQ)) {
            if (RegOperand::IsSameRegNO(cmpFirstOpnd, csetFirstOpnd)) {
                bb.RemoveInsn(insn);
                bb.RemoveInsn(*csetInsn);
            } else {
                if (cmpFirstOpnd.GetSize() != csetFirstOpnd.GetSize()) {
                    return;
                }
                MOperator mopCode = (cmpFirstOpnd.GetSize() == k64BitSize) ? MOP_xmovrr : MOP_wmovrr;
                Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(mopCode, csetFirstOpnd, cmpFirstOpnd);
                bb.ReplaceInsn(insn, newInsn);
                bb.RemoveInsn(*csetInsn);
            }
        } else if ((cmpConstVal == 1 && cond.GetCode() == CC_NE) || (cmpConstVal == 0 && cond.GetCode() == CC_EQ)) {
            if (cmpFirstOpnd.GetSize() != csetFirstOpnd.GetSize()) {
                return;
            }
            MOperator mopCode = (cmpFirstOpnd.GetSize() == k64BitSize) ? MOP_xeorrri13 : MOP_weorrri12;
            ImmOperand &one = static_cast<AArch64CGFunc *>(&cgFunc)->CreateImmOperand(1, k8BitSize, false);
            Insn &newInsn = cgFunc.GetInsnBuilder()->BuildInsn(mopCode, csetFirstOpnd, cmpFirstOpnd, one);
            bb.ReplaceInsn(insn, newInsn);
            bb.RemoveInsn(*csetInsn);
        }
    }
}

/* ldr wn, [x1, wn, SXTW]
 * add x2, wn, x2
 */
bool ComplexMemOperandAddAArch64::IsExpandBaseOpnd(const Insn &insn, const Insn &prevInsn) const
{
    MOperator prevMop = prevInsn.GetMachineOpcode();
    if (prevMop >= MOP_wldrsb && prevMop <= MOP_xldr &&
        prevInsn.GetOperand(kInsnFirstOpnd).Equals(insn.GetOperand(kInsnSecondOpnd))) {
        return true;
    }
    return false;
}

void ComplexMemOperandAddAArch64::Run(BB &bb, Insn &insn)
{
    AArch64CGFunc *aarch64CGFunc = static_cast<AArch64CGFunc *>(&cgFunc);
    Insn *nextInsn = insn.GetNextMachineInsn();
    if (nextInsn == nullptr) {
        return;
    }
    Insn *prevInsn = insn.GetPreviousMachineInsn();
    MOperator thisMop = insn.GetMachineOpcode();
    if (thisMop != MOP_xaddrrr && thisMop != MOP_waddrrr) {
        return;
    }
    MOperator nextMop = nextInsn->GetMachineOpcode();
    if (nextMop && ((nextMop >= MOP_wldrsb && nextMop <= MOP_dldr) || (nextMop >= MOP_wstrb && nextMop <= MOP_dstr))) {
        if (!IsMemOperandOptPattern(insn, *nextInsn)) {
            return;
        }
        MemOperand *memOpnd = static_cast<MemOperand *>(nextInsn->GetMemOpnd());
        auto newBaseOpnd = static_cast<RegOperand *>(&insn.GetOperand(kInsnSecondOpnd));
        auto newIndexOpnd = static_cast<RegOperand *>(&insn.GetOperand(kInsnThirdOpnd));
        regno_t memBaseOpndRegNO = newBaseOpnd->GetRegisterNumber();
        if (newBaseOpnd->GetSize() <= k32BitSize && prevInsn != nullptr && IsExpandBaseOpnd(insn, *prevInsn)) {
            newBaseOpnd = &aarch64CGFunc->GetOrCreatePhysicalRegisterOperand(static_cast<AArch64reg>(memBaseOpndRegNO),
                                                                             k64BitSize, kRegTyInt);
        }
        if (newBaseOpnd->GetSize() != k64BitSize) {
            return;
        }
        DEBUG_ASSERT(memOpnd != nullptr, "memOpnd should not be nullptr");
        if (newIndexOpnd->GetSize() <= k32BitSize) {
            MemOperand &newMemOpnd = aarch64CGFunc->GetOrCreateMemOpnd(MemOperand::kAddrModeBOrX, memOpnd->GetSize(),
                                                                       newBaseOpnd, newIndexOpnd, 0, false);
            nextInsn->SetOperand(kInsnSecondOpnd, newMemOpnd);
        } else {
            MemOperand &newMemOpnd = aarch64CGFunc->GetOrCreateMemOpnd(MemOperand::kAddrModeBOrX, memOpnd->GetSize(),
                                                                       newBaseOpnd, newIndexOpnd, nullptr, nullptr);
            nextInsn->SetOperand(kInsnSecondOpnd, newMemOpnd);
        }
        bb.RemoveInsn(insn);
    }
}

void RemoveSxtBeforeStrAArch64::Run(BB &bb, Insn &insn)
{
    MOperator mop = insn.GetMachineOpcode();
    Insn *prevInsn = insn.GetPreviousMachineInsn();
    if (prevInsn == nullptr) {
        return;
    }
    MOperator prevMop = prevInsn->GetMachineOpcode();
    if (!(mop == MOP_wstrh && prevMop == MOP_xsxth32) && !(mop == MOP_wstrb && prevMop == MOP_xsxtb32)) {
        return;
    }
    auto &prevOpnd0 = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnFirstOpnd));
    if (IfOperandIsLiveAfterInsn(prevOpnd0, insn)) {
        return;
    }
    auto &prevOpnd1 = static_cast<RegOperand &>(prevInsn->GetOperand(kInsnSecondOpnd));
    regno_t prevRegNO0 = prevOpnd0.GetRegisterNumber();
    regno_t prevRegNO1 = prevOpnd1.GetRegisterNumber();
    regno_t regNO0 = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd)).GetRegisterNumber();
    if (prevRegNO0 != prevRegNO1) {
        return;
    }
    if (prevRegNO0 == regNO0) {
        bb.RemoveInsn(*prevInsn);
        return;
    }
    insn.SetOperand(0, prevOpnd1);
    bb.RemoveInsn(*prevInsn);
}
} /* namespace maplebe */
