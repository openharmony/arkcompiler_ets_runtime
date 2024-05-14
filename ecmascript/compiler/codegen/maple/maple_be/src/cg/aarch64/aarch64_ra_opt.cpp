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

#include "loop.h"
#include "aarch64_ra_opt.h"

namespace maplebe {
using namespace std;
bool RaX0Opt::PropagateX0CanReplace(Operand *opnd, regno_t replaceReg) const
{
    if (opnd != nullptr) {
        RegOperand *regopnd = static_cast<RegOperand *>(opnd);
        regno_t regCandidate = regopnd->GetRegisterNumber();
        if (regCandidate == replaceReg) {
            return true;
        }
    }
    return false;
}

/*
 * Replace replace_reg with rename_reg.
 * return true if there is a redefinition that needs to terminate the propagation.
 */
bool RaX0Opt::PropagateRenameReg(Insn *nInsn, const X0OptInfo &optVal) const
{
    uint32 renameReg = static_cast<RegOperand *>(optVal.GetRenameOpnd())->GetRegisterNumber();
    const InsnDesc *md = nInsn->GetDesc();
    CHECK_FATAL(nInsn->GetOperandSize() >= 1, "value overflow");
    int32 lastOpndId = static_cast<int32>(nInsn->GetOperandSize() - 1);
    for (int32_t i = lastOpndId; i >= 0; i--) {
        Operand &opnd = nInsn->GetOperand(static_cast<uint32>(i));

        if (opnd.IsList()) {
            /* call parameters */
        } else if (opnd.IsMemoryAccessOperand()) {
            MemOperand &memopnd = static_cast<MemOperand &>(opnd);
            if (PropagateX0CanReplace(memopnd.GetBaseRegister(), optVal.GetReplaceReg())) {
                RegOperand *renameOpnd = static_cast<RegOperand *>(optVal.GetRenameOpnd());
                memopnd.SetBaseRegister(*renameOpnd);
            }
            if (PropagateX0CanReplace(memopnd.GetIndexRegister(), optVal.GetReplaceReg())) {
                RegOperand *renameOpnd = static_cast<RegOperand *>(optVal.GetRenameOpnd());
                memopnd.SetIndexRegister(*renameOpnd);
            }
        } else if (opnd.IsRegister()) {
            bool isdef = (md->GetOpndDes(i))->IsRegDef();
            RegOperand &regopnd = static_cast<RegOperand &>(opnd);
            regno_t regCandidate = regopnd.GetRegisterNumber();
            if (isdef) {
                /* Continue if both replace_reg & rename_reg are not redefined. */
                if (regCandidate == optVal.GetReplaceReg() || regCandidate == renameReg) {
                    return true;
                }
            } else {
                if (regCandidate == optVal.GetReplaceReg()) {
                    nInsn->SetOperand(static_cast<uint32>(i), *optVal.GetRenameOpnd());
                }
            }
        }
    }
    return false; /* false == no redefinition */
}

/* Propagate x0 from a call return value to a def of x0.
 * This eliminates some local reloads under high register pressure, since
 * the use has been replaced by x0.
 */
bool RaX0Opt::PropagateX0DetectX0(const Insn *insn, X0OptInfo &optVal) const
{
    if (insn->GetMachineOpcode() != MOP_xmovrr && insn->GetMachineOpcode() != MOP_wmovrr) {
        return false;
    }
    RegOperand &movSrc = static_cast<RegOperand &>(insn->GetOperand(1));
    if (movSrc.GetRegisterNumber() != R0) {
        return false;
    }

    optVal.SetMovSrc(&movSrc);
    return true;
}

bool RaX0Opt::PropagateX0DetectRedefine(const InsnDesc *md, const Insn *ninsn, const X0OptInfo &optVal,
                                        uint32 index) const
{
    bool isdef = (md->GetOpndDes(static_cast<int>(index)))->IsRegDef();
    if (isdef) {
        RegOperand &opnd = static_cast<RegOperand &>(ninsn->GetOperand(index));
        if (opnd.GetRegisterNumber() == optVal.GetReplaceReg()) {
            return true;
        }
    }
    return false;
}

bool RaX0Opt::PropagateX0Optimize(const BB *bb, const Insn *insn, X0OptInfo &optVal)
{
    bool redefined = false;
    for (Insn *ninsn = insn->GetNext(); (ninsn != nullptr) && ninsn != bb->GetLastInsn()->GetNext();
         ninsn = ninsn->GetNext()) {
        if (!ninsn->IsMachineInstruction()) {
            continue;
        }

        if (ninsn->IsCall()) {
            break;
        }

        /* Will continue as long as the reg being replaced is not redefined.
         * Does not need to check for x0 redefinition.  The mov instruction src
         * being replaced already defines x0 and will terminate this loop.
         */
        const InsnDesc *md = ninsn->GetDesc();
        for (uint32 i = 0; i < ninsn->GetDefRegs().size(); i++) {
            redefined = PropagateX0DetectRedefine(md, ninsn, optVal, i);
            if (redefined) {
                break;
            }
        }
        if (redefined) {
            break;
        }

        /* Look for move where src is the register equivalent to x0. */
        if (ninsn->GetMachineOpcode() != MOP_xmovrr && ninsn->GetMachineOpcode() != MOP_wmovrr) {
            continue;
        }

        Operand *src = &ninsn->GetOperand(1);
        RegOperand *srcreg = static_cast<RegOperand *>(src);
        if (srcreg->GetRegisterNumber() != optVal.GetReplaceReg()) {
            continue;
        }

        /* Setup for the next optmization pattern. */
        Operand *dst = &ninsn->GetOperand(0);
        RegOperand *dstreg = static_cast<RegOperand *>(dst);
        if (dstreg->GetRegisterNumber() != R0) {
            /* This is to set up for further propagation later. */
            if (srcreg->GetRegisterNumber() == optVal.GetReplaceReg()) {
                if (optVal.GetRenameInsn() != nullptr) {
                    redefined = true;
                    break;
                } else {
                    optVal.SetRenameInsn(ninsn);
                    optVal.SetRenameOpnd(dst);
                    optVal.SetRenameReg(dstreg->GetRegisterNumber());
                }
            }
            continue;
        }

        if (redefined) {
            break;
        }

        /* x0 = x0 */
        ninsn->SetOperand(1, *optVal.GetMovSrc());
        break;
    }

    return redefined;
}

bool RaX0Opt::PropagateX0ForCurrBb(BB *bb, const X0OptInfo &optVal)
{
    bool redefined = false;
    for (Insn *ninsn = optVal.GetRenameInsn()->GetNext(); (ninsn != nullptr) && ninsn != bb->GetLastInsn()->GetNext();
         ninsn = ninsn->GetNext()) {
        if (!ninsn->IsMachineInstruction()) {
            continue;
        }
        redefined = PropagateRenameReg(ninsn, optVal);
        if (redefined) {
            break;
        }
    }
    if (!redefined) {
        auto it = bb->GetLiveOutRegNO().find(optVal.GetReplaceReg());
        if (it != bb->GetLiveOutRegNO().end()) {
            bb->EraseLiveOutRegNO(it);
        }
        uint32 renameReg = static_cast<RegOperand *>(optVal.GetRenameOpnd())->GetRegisterNumber();
        bb->InsertLiveOutRegNO(renameReg);
    }
    return redefined;
}

void RaX0Opt::PropagateX0ForNextBb(BB *nextBb, const X0OptInfo &optVal)
{
    bool redefined = false;
    for (Insn *ninsn = nextBb->GetFirstInsn(); ninsn != nextBb->GetLastInsn()->GetNext(); ninsn = ninsn->GetNext()) {
        if (!ninsn->IsMachineInstruction()) {
            continue;
        }
        redefined = PropagateRenameReg(ninsn, optVal);
        if (redefined) {
            break;
        }
    }
    if (!redefined) {
        auto it = nextBb->GetLiveOutRegNO().find(optVal.GetReplaceReg());
        if (it != nextBb->GetLiveOutRegNO().end()) {
            nextBb->EraseLiveOutRegNO(it);
        }
        uint32 renameReg = static_cast<RegOperand *>(optVal.GetRenameOpnd())->GetRegisterNumber();
        nextBb->InsertLiveOutRegNO(renameReg);
    }
}

/*
 * Perform optimization.
 * First propagate x0 in a bb.
 * Second propagation see comment in function.
 */
void RaX0Opt::PropagateX0()
{
    FOR_ALL_BB(bb, cgFunc) {
        X0OptInfo optVal;

        Insn *insn = bb->GetFirstInsn();
        while ((insn != nullptr) && !insn->IsMachineInstruction()) {
            insn = insn->GetNext();
            continue;
        }
        if (insn == nullptr) {
            continue;
        }
        if (!PropagateX0DetectX0(insn, optVal)) {
            continue;
        }

        /* At this point the 1st insn is a mov from x0. */
        RegOperand &movDst = static_cast<RegOperand &>(insn->GetOperand(0));
        optVal.SetReplaceReg(movDst.GetRegisterNumber());
        optVal.ResetRenameInsn();
        bool redefined = PropagateX0Optimize(bb, insn, optVal);
        if (redefined || (optVal.GetRenameInsn() == nullptr)) {
            continue;
        }

        /* Next pattern to help LSRA.  Short cross bb live interval.
         *  Straight line code.  Convert reg2 into bb local.
         *  bb1
         *     mov  reg2 <- x0          =>   mov  reg2 <- x0
         *     mov  reg1 <- reg2             mov  reg1 <- reg2
         *     call                          call
         *  bb2  : livein< reg1 reg2 >
         *     use          reg2             use          reg1
         *     ....
         *     reg2 not liveout
         *
         * Can allocate caller register for reg2.
         *
         * Further propagation of very short live interval cross bb reg
         */
        if (optVal.GetRenameReg() < kMaxRegNum) { /* dont propagate physical reg */
            continue;
        }
        BB *nextBb = bb->GetNext();
        if (nextBb == nullptr) {
            break;
        }
        if (bb->GetSuccs().size() != 1 || nextBb->GetPreds().size() != 1) {
            continue;
        }
        if (bb->GetSuccs().front() != nextBb || nextBb->GetPreds().front() != bb) {
            continue;
        }
        if (bb->GetLiveOutRegNO().find(optVal.GetReplaceReg()) == bb->GetLiveOutRegNO().end() ||
            bb->GetLiveOutRegNO().find(optVal.GetRenameReg()) == bb->GetLiveOutRegNO().end() ||
            nextBb->GetLiveOutRegNO().find(optVal.GetReplaceReg()) != nextBb->GetLiveOutRegNO().end()) {
            continue;
        }
        /* Replace replace_reg by rename_reg. */
        redefined = PropagateX0ForCurrBb(bb, optVal);
        if (redefined) {
            continue;
        }
        PropagateX0ForNextBb(nextBb, optVal);
    }
}

void VregRename::PrintRenameInfo(regno_t regno) const
{
    VregRenameInfo *info = (regno <= maxRegnoSeen) ? renameInfo[regno] : nullptr;
    if (info == nullptr || (info->numDefs == 0 && info->numUses == 0)) {
        return;
    }
    LogInfo::MapleLogger() << "reg: " << regno;
    if (info->firstBBLevelSeen) {
        LogInfo::MapleLogger() << " fromLevel " << info->firstBBLevelSeen->GetInternalFlag2();
    }
    if (info->lastBBLevelSeen) {
        LogInfo::MapleLogger() << " toLevel " << info->lastBBLevelSeen->GetInternalFlag2();
    }
    if (info->numDefs) {
        LogInfo::MapleLogger() << " defs " << info->numDefs;
    }
    if (info->numUses) {
        LogInfo::MapleLogger() << " uses " << info->numUses;
    }
    if (info->numDefs) {
        LogInfo::MapleLogger() << " innerDefs " << info->numInnerDefs;
    }
    if (info->numUses) {
        LogInfo::MapleLogger() << " innerUses " << info->numInnerUses;
    }
    LogInfo::MapleLogger() << "\n";
}

void VregRename::PrintAllRenameInfo() const
{
    for (uint32 regno = 0; regno < cgFunc->GetMaxRegNum(); ++regno) {
        PrintRenameInfo(regno);
    }
}

bool VregRename::IsProfitableToRename(const VregRenameInfo *info) const
{
    if ((info->numInnerDefs == 0) && (info->numUses != info->numInnerUses)) {
        return true;
    }
    return false;
}

void VregRename::RenameProfitableVreg(RegOperand &ropnd, const LoopDesc &loop)
{
    regno_t vreg = ropnd.GetRegisterNumber();
    VregRenameInfo *info = (vreg <= maxRegnoSeen) ? renameInfo[vreg] : nullptr;
    if ((info == nullptr) || (!IsProfitableToRename(info))) {
        return;
    }

    uint32 size = (ropnd.GetSize() == k64BitSize) ? k8ByteSize : k4ByteSize;
    regno_t newRegno = cgFunc->NewVReg(ropnd.GetRegisterType(), size);
    RegOperand *renameVreg = &cgFunc->CreateVirtualRegisterOperand(newRegno);

    const BB &header = loop.GetHeader();
    for (auto pred : header.GetPreds()) {
        if (loop.IsBackEdge(*pred, loop.GetHeader())) {
            continue;
        }
        MOperator mOp = (ropnd.GetRegisterType() == kRegTyInt) ? ((size == k8BitSize) ? MOP_xmovrr : MOP_wmovrr)
                                                               : ((size == k8BitSize) ? MOP_xvmovd : MOP_xvmovs);
        Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, *renameVreg, ropnd);
        Insn *last = pred->GetLastInsn();
        if (last) {
            if (last->IsBranch()) {
                last->GetBB()->InsertInsnBefore(*last, newInsn);
            } else {
                last->GetBB()->InsertInsnAfter(*last, newInsn);
            }
        } else {
            pred->AppendInsn(newInsn);
        }
    }

    for (auto bbId : loop.GetLoopBBs()) {
        auto *bb = cgFunc->GetBBFromID(bbId);
        FOR_BB_INSNS(insn, bb) {
            if (insn->IsImmaterialInsn() || !insn->IsMachineInstruction()) {
                continue;
            }
            for (uint32 i = 0; i < insn->GetOperandSize(); ++i) {
                Operand *opnd = &insn->GetOperand(i);
                if (opnd->IsList()) {
                    /* call parameters */
                } else if (opnd->IsMemoryAccessOperand()) {
                    MemOperand *memopnd = static_cast<MemOperand *>(opnd);
                    RegOperand *base = static_cast<RegOperand *>(memopnd->GetBaseRegister());
                    MemOperand *newMemOpnd = nullptr;
                    if (base != nullptr && base->IsVirtualRegister() && base->GetRegisterNumber() == vreg) {
                        newMemOpnd = static_cast<MemOperand *>(memopnd->Clone(*cgFunc->GetMemoryPool()));
                        newMemOpnd->SetBaseRegister(*renameVreg);
                        insn->SetOperand(i, *newMemOpnd);
                    }
                    RegOperand *offset = static_cast<RegOperand *>(memopnd->GetIndexRegister());
                    if (offset != nullptr && offset->IsVirtualRegister() && offset->GetRegisterNumber() == vreg) {
                        if (newMemOpnd == nullptr) {
                            newMemOpnd = static_cast<MemOperand *>(memopnd->Clone(*cgFunc->GetMemoryPool()));
                        }
                        newMemOpnd->SetIndexRegister(*renameVreg);
                        insn->SetOperand(i, *newMemOpnd);
                    }
                } else if (opnd->IsRegister() && static_cast<RegOperand *>(opnd)->IsVirtualRegister() &&
                           static_cast<RegOperand *>(opnd)->GetRegisterNumber() == vreg) {
                    insn->SetOperand(i, *renameVreg);
                }
            }
        }
    }
}

void VregRename::RenameFindLoopVregs(const LoopDesc &loop)
{
    for (auto bbId : loop.GetLoopBBs()) {
        auto *bb = cgFunc->GetBBFromID(bbId);
        FOR_BB_INSNS(insn, bb) {
            if (insn->IsImmaterialInsn() || !insn->IsMachineInstruction()) {
                continue;
            }
            for (uint32 i = 0; i < insn->GetOperandSize(); ++i) {
                Operand *opnd = &insn->GetOperand(i);
                if (opnd->IsList()) {
                    /* call parameters */
                } else if (opnd->IsMemoryAccessOperand()) {
                    MemOperand *memopnd = static_cast<MemOperand *>(opnd);
                    RegOperand *base = static_cast<RegOperand *>(memopnd->GetBaseRegister());
                    if (base != nullptr && base->IsVirtualRegister()) {
                        RenameProfitableVreg(*base, loop);
                    }
                    RegOperand *offset = static_cast<RegOperand *>(memopnd->GetIndexRegister());
                    if (offset != nullptr && offset->IsVirtualRegister()) {
                        RenameProfitableVreg(*offset, loop);
                    }
                } else if (opnd->IsRegister() && static_cast<RegOperand *>(opnd)->IsVirtualRegister() &&
                           static_cast<RegOperand *>(opnd)->GetRegisterNumber() != ccRegno) {
                    RenameProfitableVreg(static_cast<RegOperand &>(*opnd), loop);
                }
            }
        }
    }
}

/* Only the bb level is important, not the bb itself.
 * So if multiple bbs have the same level, only one bb represents the level
 */
void VregRename::UpdateVregInfo(regno_t vreg, BB *bb, bool isInner, bool isDef)
{
    VregRenameInfo *info = renameInfo[vreg];
    if (info == nullptr) {
        info = memPool->New<VregRenameInfo>();
        renameInfo[vreg] = info;
        if (vreg > maxRegnoSeen) {
            maxRegnoSeen = vreg;
        }
    }
    if (isDef) {
        info->numDefs++;
        if (isInner) {
            info->numInnerDefs++;
        }
    } else {
        info->numUses++;
        if (isInner) {
            info->numInnerUses++;
        }
    }
    if (info->firstBBLevelSeen) {
        if (info->firstBBLevelSeen->GetInternalFlag2() > bb->GetInternalFlag2()) {
            info->firstBBLevelSeen = bb;
        }
    } else {
        info->firstBBLevelSeen = bb;
    }
    if (info->lastBBLevelSeen) {
        if (info->lastBBLevelSeen->GetInternalFlag2() < bb->GetInternalFlag2()) {
            info->lastBBLevelSeen = bb;
        }
    } else {
        info->lastBBLevelSeen = bb;
    }
}

void VregRename::RenameGetFuncVregInfo()
{
    FOR_ALL_BB(bb, cgFunc) {
        auto *loop = loopInfo.GetBBLoopParent(bb->GetId());
        bool isInner = loop ? loop->GetChildLoops().empty() : false;
        FOR_BB_INSNS(insn, bb) {
            if (insn->IsImmaterialInsn() || !insn->IsMachineInstruction()) {
                continue;
            }
            const InsnDesc *md = insn->GetDesc();
            for (uint32 i = 0; i < insn->GetOperandSize(); ++i) {
                Operand *opnd = &insn->GetOperand(i);
                if (opnd->IsList()) {
                    /* call parameters */
                } else if (opnd->IsMemoryAccessOperand()) {
                    MemOperand *memopnd = static_cast<MemOperand *>(opnd);
                    RegOperand *base = static_cast<RegOperand *>(memopnd->GetBaseRegister());
                    if (base != nullptr && base->IsVirtualRegister()) {
                        regno_t vreg = base->GetRegisterNumber();
                        UpdateVregInfo(vreg, bb, isInner, false);
                    }
                    RegOperand *offset = static_cast<RegOperand *>(memopnd->GetIndexRegister());
                    if (offset != nullptr && offset->IsVirtualRegister()) {
                        regno_t vreg = offset->GetRegisterNumber();
                        UpdateVregInfo(vreg, bb, isInner, false);
                    }
                } else if (opnd->IsRegister() && static_cast<RegOperand *>(opnd)->IsVirtualRegister() &&
                           static_cast<RegOperand *>(opnd)->GetRegisterNumber() != ccRegno) {
                    bool isdef = (md->opndMD[i])->IsRegDef();
                    regno_t vreg = static_cast<RegOperand *>(opnd)->GetRegisterNumber();
                    UpdateVregInfo(vreg, bb, isInner, isdef);
                }
            }
        }
    }
}

void VregRename::RenameFindVregsToRename(const LoopDesc &loop)
{
    if (loop.GetChildLoops().empty()) {
        RenameFindLoopVregs(loop);
        return;
    }
    for (const auto inner : loop.GetChildLoops()) {
        RenameFindVregsToRename(*inner);
    }
}

void VregRename::VregLongLiveRename()
{
    if (loopInfo.GetLoops().empty()) {
        return;
    }
    RenameGetFuncVregInfo();
    for (const auto *loop : loopInfo.GetLoops()) {
        RenameFindVregsToRename(*loop);
    }
}

void AArch64RaOpt::Run()
{
    RaX0Opt x0Opt(cgFunc);
    x0Opt.PropagateX0();

    if (cgFunc->GetMirModule().GetSrcLang() == kSrcLangC && CGOptions::DoVregRename()) {
        /* loop detection considers EH bb.  That is not handled.  So C only for now. */
        auto *loop = memPool->New<LoopAnalysis>(*cgFunc, *memPool, domInfo);
        loop->Analysis();
        VregRename rename(cgFunc, memPool, *loop);
        Bfs localBfs(*cgFunc, *memPool);
        rename.bfs = &localBfs;
        rename.bfs->ComputeBlockOrder();
        rename.VregLongLiveRename();
    }
}
} /* namespace maplebe */
