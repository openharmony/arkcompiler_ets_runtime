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
void AArch64FPLROffsetAdjustment::Run()
{
    AdjustmentOffsetForFPLR();
}

void AArch64FPLROffsetAdjustment::AdjustmentOffsetForOpnd(Insn &insn, AArch64CGFunc &aarchCGFunc)
{
    bool isLmbc = (aarchCGFunc.GetMirModule().GetFlavor() == MIRFlavor::kFlavorLmbc);
    uint32 opndNum = insn.GetOperandSize();
    MemLayout *memLayout = aarchCGFunc.GetMemlayout();
    bool stackBaseOpnd = false;
    AArch64reg stackBaseReg = isLmbc ? R29 : (aarchCGFunc.UseFP() ? R29 : RSP);
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = insn.GetOperand(i);
        if (opnd.IsRegister()) {
            auto &regOpnd = static_cast<RegOperand &>(opnd);
            if (regOpnd.IsOfVary()) {
                insn.SetOperand(i, aarchCGFunc.GetOrCreateStackBaseRegOperand());
            }
            if (regOpnd.GetRegisterNumber() == RFP) {
                insn.SetOperand(i, aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt));
                stackBaseOpnd = true;
            }
        } else if (opnd.IsMemoryAccessOperand()) {
            auto &memOpnd = static_cast<MemOperand &>(opnd);
            if (((memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi) ||
                 (memOpnd.GetAddrMode() == MemOperand::kAddrModeBOrX)) &&
                memOpnd.GetBaseRegister() != nullptr) {
                if (memOpnd.GetBaseRegister()->IsOfVary()) {
                    memOpnd.SetBaseRegister(static_cast<RegOperand &>(aarchCGFunc.GetOrCreateStackBaseRegOperand()));
                }
                RegOperand *memBaseReg = memOpnd.GetBaseRegister();
                if (memBaseReg->GetRegisterNumber() == RFP) {
                    RegOperand &newBaseOpnd =
                        aarchCGFunc.GetOrCreatePhysicalRegisterOperand(stackBaseReg, k64BitSize, kRegTyInt);
                    MemOperand &newMemOpnd = *aarchCGFunc.CreateMemOperand(memOpnd.GetAddrMode(), memOpnd.GetSize(),
                        newBaseOpnd, memOpnd.GetIndexRegister(), memOpnd.GetOffsetOperand(), memOpnd.GetSymbol());
                    insn.SetOperand(i, newMemOpnd);
                    stackBaseOpnd = true;
                }
            }
            if ((memOpnd.GetAddrMode() != MemOperand::kAddrModeBOi) || !memOpnd.IsIntactIndexed()) {
                continue;
            }
            OfstOperand *ofstOpnd = memOpnd.GetOffsetImmediate();
            if (ofstOpnd == nullptr) {
                continue;
            }
            if (ofstOpnd->GetVary() == kUnAdjustVary) {
                ofstOpnd->AdjustOffset(static_cast<int32>(
                    static_cast<AArch64MemLayout *>(memLayout)->RealStackFrameSize() -
                    memLayout->SizeOfArgsToStackPass() - cgFunc->GetFunction().GetFrameReseverdSlot()));
                ofstOpnd->SetVary(kAdjustVary);
            }
            if (!stackBaseOpnd && (ofstOpnd->GetVary() == kAdjustVary || ofstOpnd->GetVary() == kNotVary)) {
                bool condition = aarchCGFunc.IsOperandImmValid(insn.GetMachineOpcode(), &memOpnd, i);
                if (!condition) {
                    MemOperand &newMemOpnd = aarchCGFunc.SplitOffsetWithAddInstruction(
                        memOpnd, memOpnd.GetSize(), static_cast<AArch64reg>(R16), false, &insn);
                    insn.SetOperand(i, newMemOpnd);
                }
            }
        } else if (opnd.IsIntImmediate()) {
            AdjustmentOffsetForImmOpnd(insn, i, aarchCGFunc);
        }
    }
    if (stackBaseOpnd && !aarchCGFunc.UseFP()) {
        AdjustmentStackPointer(insn, aarchCGFunc);
    }
}

void AArch64FPLROffsetAdjustment::AdjustmentOffsetForImmOpnd(Insn &insn, uint32 index, AArch64CGFunc &aarchCGFunc) const
{
    auto &immOpnd = static_cast<ImmOperand &>(insn.GetOperand(index));
    MemLayout *memLayout = aarchCGFunc.GetMemlayout();
    if (immOpnd.GetVary() == kUnAdjustVary) {
        int64 ofst = static_cast<AArch64MemLayout *>(memLayout)->RealStackFrameSize() -
                     memLayout->SizeOfArgsToStackPass() - cgFunc->GetFunction().GetFrameReseverdSlot();
        if (insn.GetMachineOpcode() == MOP_xsubrri12 || insn.GetMachineOpcode() == MOP_wsubrri12) {
            immOpnd.SetValue(immOpnd.GetValue() - ofst);
            if (immOpnd.GetValue() < 0) {
                immOpnd.Negate();
            }
            insn.SetMOP(AArch64CG::kMd[A64ConstProp::GetReversalMOP(insn.GetMachineOpcode())]);
        } else {
            immOpnd.Add(ofst);
        }
    }
    if (!aarchCGFunc.IsOperandImmValid(insn.GetMachineOpcode(), &immOpnd, index)) {
        if (insn.GetMachineOpcode() >= MOP_xaddrri24 && insn.GetMachineOpcode() <= MOP_waddrri12) {
            PrimType destTy =
                static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd)).GetSize() == k64BitSize ? PTY_i64 : PTY_i32;
            RegOperand *resOpnd = &static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
            ImmOperand &copyImmOpnd =
                aarchCGFunc.CreateImmOperand(immOpnd.GetValue(), immOpnd.GetSize(), immOpnd.IsSignedValue());
            aarchCGFunc.SelectAddAfterInsn(*resOpnd, insn.GetOperand(kInsnSecondOpnd), copyImmOpnd, destTy, false,
                                           insn);
            insn.GetBB()->RemoveInsn(insn);
        } else if (insn.GetMachineOpcode() == MOP_xsubrri12 || insn.GetMachineOpcode() == MOP_wsubrri12) {
            if (immOpnd.IsSingleInstructionMovable()) {
                RegOperand &tempReg = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
                bool is64bit = insn.GetOperand(kInsnFirstOpnd).GetSize() == k64BitSize;
                MOperator tempMovOp = is64bit ? MOP_xmovri64 : MOP_wmovri32;
                Insn &tempMov = cgFunc->GetInsnBuilder()->BuildInsn(tempMovOp, tempReg, immOpnd);
                insn.SetOperand(index, tempReg);
                insn.SetMOP(is64bit ? AArch64CG::kMd[MOP_xsubrrr] : AArch64CG::kMd[MOP_wsubrrr]);
                (void)insn.GetBB()->InsertInsnBefore(insn, tempMov);
            }
        } else {
            CHECK_FATAL(false, "NIY");
        }
    }
    immOpnd.SetVary(kAdjustVary);
}

void AArch64FPLROffsetAdjustment::AdjustmentStackPointer(Insn &insn, AArch64CGFunc &aarchCGFunc)
{
    AArch64MemLayout *aarch64memlayout = static_cast<AArch64MemLayout *>(aarchCGFunc.GetMemlayout());
    int32 offset =
        static_cast<int32>(aarch64memlayout->SizeOfArgsToStackPass() + cgFunc->GetFunction().GetFrameReseverdSlot());
    if (offset == 0) {
        return;
    }
    if (insn.IsLoad() || insn.IsStore()) {
        uint32 opndNum = insn.GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            Operand &opnd = insn.GetOperand(i);
            if (opnd.IsMemoryAccessOperand()) {
                auto &memOpnd = static_cast<MemOperand &>(opnd);
                DEBUG_ASSERT(memOpnd.GetBaseRegister() != nullptr, "Unexpect, need check");
                CHECK_FATAL(memOpnd.IsIntactIndexed(), "unsupport yet");
                if (memOpnd.GetAddrMode() == MemOperand::kAddrModeBOi) {
                    ImmOperand *ofstOpnd = memOpnd.GetOffsetOperand();
                    ImmOperand *newOfstOpnd = &aarchCGFunc.GetOrCreateOfstOpnd(
                        static_cast<uint64>(ofstOpnd->GetValue() + offset), ofstOpnd->GetSize());
                    MemOperand &newOfstMemOpnd = aarchCGFunc.GetOrCreateMemOpnd(
                        MemOperand::kAddrModeBOi, memOpnd.GetSize(), memOpnd.GetBaseRegister(),
                        memOpnd.GetIndexRegister(), newOfstOpnd, memOpnd.GetSymbol());
                    insn.SetOperand(i, newOfstMemOpnd);
                    if (!aarchCGFunc.IsOperandImmValid(insn.GetMachineOpcode(), &newOfstMemOpnd, i)) {
                        bool isPair = (i == kInsnThirdOpnd);
                        MemOperand &newMemOpnd = aarchCGFunc.SplitOffsetWithAddInstruction(
                            newOfstMemOpnd, newOfstMemOpnd.GetSize(), static_cast<AArch64reg>(R16), false, &insn,
                            isPair);
                        insn.SetOperand(i, newMemOpnd);
                    }
                    continue;
                } else if (memOpnd.GetAddrMode() == MemOperand::kAddrModeBOrX) {
                    CHECK_FATAL(false, "Unexpect adjust insn");
                } else {
                    insn.Dump();
                    CHECK_FATAL(false, "Unexpect adjust insn");
                }
            }
        }
    } else {
        switch (insn.GetMachineOpcode()) {
            case MOP_xaddrri12: {
                DEBUG_ASSERT(static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
                             "regNumber should be changed in AdjustmentOffsetForOpnd");
                ImmOperand &addend = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
                addend.SetValue(addend.GetValue() + offset);
                AdjustmentOffsetForImmOpnd(insn, kInsnThirdOpnd, aarchCGFunc); /* legalize imm opnd */
                break;
            }
            case MOP_xaddrri24: {
                DEBUG_ASSERT(static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
                             "regNumber should be changed in AdjustmentOffsetForOpnd");
                RegOperand &tempReg = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
                ImmOperand &offsetReg = aarchCGFunc.CreateImmOperand(offset, k64BitSize, false);
                aarchCGFunc.SelectAddAfterInsn(tempReg, insn.GetOperand(kInsnSecondOpnd), offsetReg, PTY_i64, false,
                                               insn);
                insn.SetOperand(kInsnSecondOpnd, tempReg);
                break;
            }
            case MOP_xsubrri12: {
                DEBUG_ASSERT(static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
                             "regNumber should be changed in AdjustmentOffsetForOpnd");
                ImmOperand &subend = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
                subend.SetValue(subend.GetValue() - offset);
                break;
            }
            case MOP_xsubrri24: {
                DEBUG_ASSERT(static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
                             "regNumber should be changed in AdjustmentOffsetForOpnd");
                RegOperand &tempReg = aarchCGFunc.GetOrCreatePhysicalRegisterOperand(R16, k64BitSize, kRegTyInt);
                ImmOperand &offsetReg = aarchCGFunc.CreateImmOperand(offset, k64BitSize, false);
                aarchCGFunc.SelectAddAfterInsn(tempReg, insn.GetOperand(kInsnSecondOpnd), offsetReg, PTY_i64, false,
                                               insn);
                insn.SetOperand(kInsnSecondOpnd, tempReg);
                break;
            }
            case MOP_waddrri12: {
                if (!CGOptions::IsArm64ilp32()) {
                    insn.Dump();
                    CHECK_FATAL(false, "Unexpect offset adjustment insn");
                } else {
                    DEBUG_ASSERT(static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd)).GetRegisterNumber() == RSP,
                                 "regNumber should be changed in AdjustmentOffsetForOpnd");
                    ImmOperand &addend = static_cast<ImmOperand &>(insn.GetOperand(kInsnThirdOpnd));
                    addend.SetValue(addend.GetValue() + offset);
                    AdjustmentOffsetForImmOpnd(insn, kInsnThirdOpnd, aarchCGFunc); /* legalize imm opnd */
                }
                break;
            }
            default:
                insn.Dump();
                CHECK_FATAL(false, "Unexpect offset adjustment insn");
        }
    }
}

void AArch64FPLROffsetAdjustment::AdjustmentOffsetForFPLR()
{
    AArch64CGFunc *aarchCGFunc = static_cast<AArch64CGFunc *>(cgFunc);
    FOR_ALL_BB(bb, aarchCGFunc) {
        FOR_BB_INSNS(insn, bb) {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            AdjustmentOffsetForOpnd(*insn, *aarchCGFunc);
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
} /* namespace maplebe */
