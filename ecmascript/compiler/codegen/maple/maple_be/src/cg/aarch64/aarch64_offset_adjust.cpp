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

#include "aarch64_offset_adjust.h"
#include "aarch64_cgfunc.h"
#include "aarch64_cg.h"

namespace maplebe {

static MOperator GetReversalMOP(MOperator arithMop)
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

void AArch64FPLROffsetAdjustment::Run()
{
    AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc *>(cgFunc);
    FOR_ALL_BB(bb, aarchCGFunc)
    {
        FOR_BB_INSNS_SAFE(insn, bb, ninsn)
        {  // AdjustmentOffsetForOpnd may replace curInsn
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            AdjustmentOffsetForOpnd(*insn);
        }
    }
#ifdef STKLAY_DEBUG
    AArch64MemLayout *aarch64memlayout = static_cast<AArch64MemLayout *>(cgFunc->GetMemlayout());
    LogInfo::MapleLogger() << "--------layout of " << cgFunc->GetName() << "-------------"
                           << "\n";
    LogInfo::MapleLogger() << "stkpassed: " << aarch64memlayout->GetSegArgsStkPassed().GetSize() << "\n";
    LogInfo::MapleLogger() << "real framesize: " << aarch64memlayout->RealStackFrameSize() << "\n";
    LogInfo::MapleLogger() << "gr save: " << aarch64memlayout->GetSizeOfGRSaveArea() << "\n";
    LogInfo::MapleLogger() << "vr save: " << aarch64memlayout->GetSizeOfVRSaveArea() << "\n";
    LogInfo::MapleLogger() << "calleesave (includes fp lr): "
                           << static_cast<AArch64CGFunc *>(cgFunc)->SizeOfCalleeSaved() << "\n";
    LogInfo::MapleLogger() << "regspill: " << aarch64memlayout->GetSizeOfSpillReg() << "\n";
    LogInfo::MapleLogger() << "ref local: " << aarch64memlayout->GetSizeOfRefLocals() << "\n";
    LogInfo::MapleLogger() << "local: " << aarch64memlayout->GetSizeOfLocals() << "\n";
    LogInfo::MapleLogger() << "regpass: " << aarch64memlayout->GetSegArgsRegPassed().GetSize() << "\n";
    LogInfo::MapleLogger() << "stkpass: " << aarch64memlayout->GetSegArgsToStkPass().GetSize() << "\n";
    LogInfo::MapleLogger() << "-------------------------------------------------"
                           << "\n";
#endif
}

void AArch64FPLROffsetAdjustment::AdjustmentOffsetForOpnd(Insn &insn) const
{
    bool isLmbc = (aarchCGFunc->GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc);
    AArch64reg stackBaseReg = isLmbc ? R29 : (aarchCGFunc->UseFP() ? R29 : RSP);
    uint32 opndNum = insn.GetOperandSize();
    bool replaceFP = false;
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = insn.GetOperand(i);
        if (opnd.IsRegister()) {
            auto &regOpnd = static_cast<RegOperand &>(opnd);
            if (regOpnd.IsOfVary()) {
                insn.SetOperand(i, aarchCGFunc->GetOrCreateStackBaseRegOperand());
                regOpnd = aarchCGFunc->GetOrCreateStackBaseRegOperand();
            }
            if (regOpnd.GetRegisterNumber() == RFP) {
                insn.SetOperand(i,
                                aarchCGFunc->GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt));
                replaceFP = true;
            }
        } else if (opnd.IsMemoryAccessOperand()) {
            AdjustMemBaseReg(insn, i, replaceFP);
            AdjustMemOfstVary(insn, i);
        } else if (opnd.IsIntImmediate()) {
            AdjustmentOffsetForImmOpnd(insn, i);
        }
    }
    if (replaceFP && !aarchCGFunc->UseFP()) {
        AdjustmentStackPointer(insn);
    }
    if (!VERIFY_INSN(&insn)) {  // split insn
        SPLIT_INSN(&insn, aarchCGFunc);
    }
}

void AArch64FPLROffsetAdjustment::AdjustMemBaseReg(Insn &insn, uint32 i, bool &replaceFP) const
{
    Operand &opnd = insn.GetOperand(i);
    auto &currMemOpnd = static_cast<MemOperand &>(opnd);
    MemOperand *newMemOpnd = currMemOpnd.Clone(*aarchCGFunc->GetMemoryPool());
    CHECK_NULL_FATAL(newMemOpnd);
    if (newMemOpnd->GetBaseRegister() != nullptr) {
        if (newMemOpnd->GetBaseRegister()->IsOfVary()) {
            newMemOpnd->SetBaseRegister(static_cast<RegOperand &>(aarchCGFunc->GetOrCreateStackBaseRegOperand()));
        }
        RegOperand *memBaseReg = newMemOpnd->GetBaseRegister();
        if (memBaseReg->GetRegisterNumber() == RFP) {
            RegOperand &newBaseOpnd =
                aarchCGFunc->GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);
            newMemOpnd->SetBaseRegister(newBaseOpnd);
            replaceFP = true;
        }
    }
    if (newMemOpnd->GetBaseRegister() != nullptr && (newMemOpnd->GetBaseRegister()->GetRegisterNumber() == RFP ||
                                                     newMemOpnd->GetBaseRegister()->GetRegisterNumber() == RSP)) {
        newMemOpnd->SetStackMem(true);
    }
    insn.SetOperand(i, *newMemOpnd);
}

void AArch64FPLROffsetAdjustment::AdjustMemOfstVary(Insn &insn, uint32 i) const
{
    // Note: SplitInsn invalidates the current insn. However, the current insn will still be manipulated
    //       in subsequent steps, which will cause some unknown errors. So, we're going to do a unified
    //       split after all the steps are complete.
    Operand &opnd = insn.GetOperand(i);
    auto &currMemOpnd = static_cast<MemOperand &>(opnd);
    if (currMemOpnd.GetAddrMode() != MemOperand::kAddrModeBOi) {
        return;
    }
    OfstOperand *ofstOpnd = currMemOpnd.GetOffsetImmediate();
    CHECK_NULL_FATAL(ofstOpnd);
    if (ofstOpnd->GetVary() == kUnAdjustVary) {
        MemLayout *memLayout = aarchCGFunc->GetMemlayout();
        ofstOpnd->AdjustOffset(static_cast<int32>(static_cast<AArch64MemLayout *>(memLayout)->RealStackFrameSize() -
                                                  memLayout->SizeOfArgsToStackPass() -
                                                  static_cast<AArch64MemLayout *>(memLayout)->GetSizeOfColdToStk() -
                                                  cgFunc->GetFunction().GetFrameReseverdSlot()));
        ofstOpnd->SetVary(kAdjustVary);
    }
}

void AArch64FPLROffsetAdjustment::AdjustmentOffsetForImmOpnd(Insn &insn, uint32 index) const
{
    // Note: SplitInsn invalidates the current insn. However, the current insn will still be manipulated
    //       in subsequent steps, which will cause some unknown errors. So, we're going to do a unified
    //       split after all the steps are complete.
    auto &immOpnd = static_cast<ImmOperand &>(insn.GetOperand(index));
    auto *memLayout = static_cast<AArch64MemLayout *>(aarchCGFunc->GetMemlayout());
    if (immOpnd.GetVary() == kUnAdjustVary) {
        int64 ofst = static_cast<int64>(
            memLayout->RealStackFrameSize() -
            static_cast<uint32>(memLayout->SizeOfArgsToStackPass() + memLayout->GetSizeOfColdToStk()) -
            cgFunc->GetFunction().GetFrameReseverdSlot());
        if (insn.GetMachineOpcode() == MOP_xsubrri12 || insn.GetMachineOpcode() == MOP_wsubrri12) {
            immOpnd.SetValue(immOpnd.GetValue() - ofst);
            if (immOpnd.GetValue() < 0) {
                immOpnd.Negate();
            }
            insn.SetMOP(AArch64CG::kMd[GetReversalMOP(insn.GetMachineOpcode())]);
        } else {
            immOpnd.Add(ofst);
        }
        immOpnd.SetVary(kAdjustVary);
    }
}

void AArch64FPLROffsetAdjustment::AdjustmentStackPointer(Insn &insn) const
{
    // Note: SplitInsn invalidates the current insn. However, the current insn will still be manipulated
    //       in subsequent steps, which will cause some unknown errors. So, we're going to do a unified
    //       split after all the steps are complete.
    auto *aarch64memlayout = static_cast<AArch64MemLayout *>(aarchCGFunc->GetMemlayout());
    uint32 offset = aarch64memlayout->SizeOfArgsToStackPass() + cgFunc->GetFunction().GetFrameReseverdSlot();
    if (offset == 0) {
        return;
    }
    if (insn.IsLoad() || insn.IsStore()) {
        auto *memOpnd = static_cast<MemOperand *>(insn.GetMemOpnd());
        CHECK_NULL_FATAL(memOpnd);
        DEBUG_ASSERT(memOpnd->GetBaseRegister() != nullptr, "Unexpect, need check");
        CHECK_FATAL(memOpnd->IsIntactIndexed(), "unsupport yet");
        ImmOperand *ofstOpnd = memOpnd->GetOffsetOperand();
        CHECK_NULL_FATAL(ofstOpnd);
        ImmOperand *newOfstOpnd =
            &aarchCGFunc->GetOrCreateOfstOpnd(static_cast<uint64>(ofstOpnd->GetValue() + offset), ofstOpnd->GetSize());
        memOpnd->SetOffsetOperand(*newOfstOpnd);
    } else {
        switch (insn.GetMachineOpcode()) {
            case MOP_waddrri12:
            case MOP_xaddrri12: {
                DEBUG_ASSERT(static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
                             "regNumber should be changed in AdjustmentOffsetForOpnd");
                auto *newAddImmOpnd = static_cast<ImmOperand *>(
                    static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd)).Clone(*cgFunc->GetMemoryPool()));
                newAddImmOpnd->SetValue(newAddImmOpnd->GetValue() + offset);
                insn.SetOperand(kInsnThirdOpnd, *newAddImmOpnd);
                break;
            }
            case MOP_waddrri24:
            case MOP_xaddrri24: {
                DEBUG_ASSERT(static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
                             "regNumber should be changed in AdjustmentOffsetForOpnd");
                RegOperand &tempReg = aarchCGFunc->GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
                ImmOperand &offsetReg = aarchCGFunc->CreateImmOperand(offset, k64BitSize, false);
                aarchCGFunc->SelectAddAfterInsn(tempReg, insn.GetOperand(kInsnSecondOpnd), offsetReg, PTY_i64, false,
                                                insn);
                insn.SetOperand(kInsnSecondOpnd, tempReg);
                break;
            }
            case MOP_wsubrri12:
            case MOP_xsubrri12: {
                auto *newAddImmOpnd = static_cast<ImmOperand *>(
                    static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd)).Clone(*cgFunc->GetMemoryPool()));
                newAddImmOpnd->SetValue(newAddImmOpnd->GetValue() - offset);
                if (newAddImmOpnd->GetValue() < 0) {
                    newAddImmOpnd->Negate();
                }
                insn.SetMOP(AArch64CG::kMd[GetReversalMOP(insn.GetMachineOpcode())]);
                insn.SetOperand(kInsnThirdOpnd, *newAddImmOpnd);
                break;
            }
            case MOP_waddsrri12:
            case MOP_xaddsrri12: {
                auto *newAddImmOpnd = static_cast<ImmOperand *>(
                    static_cast<ImmOperand &>(insn.GetOperand(kInsnFourthOpnd)).Clone(*cgFunc->GetMemoryPool()));
                newAddImmOpnd->SetValue(newAddImmOpnd->GetValue() + offset);
                insn.SetOperand(kInsnFourthOpnd, *newAddImmOpnd);
                break;
            }
            default: {
                // Only some special insn will replace FP，
                insn.Dump();
                CHECK_FATAL(false, "Unexpect offset adjustment insn");
            }
        }
    }
}
} /* namespace maplebe */
