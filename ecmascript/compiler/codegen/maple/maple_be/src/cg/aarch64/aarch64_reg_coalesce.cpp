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

#include "aarch64_reg_coalesce.h"
#include "cg.h"
#include "cg_option.h"
#include "aarch64_isa.h"
#include "aarch64_insn.h"
#include "aarch64_cgfunc.h"
#include "aarch64_cg.h"

/*
 * This phase implements if-conversion optimization,
 * which tries to convert conditional branches into cset/csel instructions
 */
namespace maplebe {

#define REGCOAL_DUMP CG_DEBUG_FUNC(*cgFunc)

bool AArch64LiveIntervalAnalysis::IsUnconcernedReg(const RegOperand &regOpnd) const
{
    RegType regType = regOpnd.GetRegisterType();
    if (regType == kRegTyCc || regType == kRegTyVary) {
        return true;
    }
    if (regOpnd.GetRegisterNumber() == RZR) {
        return true;
    }
    if (!regOpnd.IsVirtualRegister()) {
        return true;
    }
    return false;
}

LiveInterval *AArch64LiveIntervalAnalysis::GetOrCreateLiveInterval(regno_t regNO)
{
    LiveInterval *lr = GetLiveInterval(regNO);
    if (lr == nullptr) {
        lr = memPool->New<LiveInterval>(alloc);
        vregIntervals[regNO] = lr;
        lr->SetRegNO(regNO);
    }
    return lr;
}

void AArch64LiveIntervalAnalysis::UpdateCallInfo()
{
    for (auto vregNO : vregLive) {
        LiveInterval *lr = GetLiveInterval(vregNO);
        if (lr == nullptr) {
            return;
        }
        lr->IncNumCall();
    }
}

void AArch64LiveIntervalAnalysis::SetupLiveIntervalByOp(Operand &op, Insn &insn, bool isDef)
{
    if (!op.IsRegister()) {
        return;
    }
    auto &regOpnd = static_cast<RegOperand &>(op);
    uint32 regNO = regOpnd.GetRegisterNumber();
    if (IsUnconcernedReg(regOpnd)) {
        return;
    }
    LiveInterval *lr = GetOrCreateLiveInterval(regNO);
    CHECK_FATAL(insn.GetId() > 1 && insn.GetId() < UINT32_MAX, "value overflow");
    uint32 point = isDef ? insn.GetId() : (insn.GetId() - 1);
    lr->AddRange(insn.GetBB()->GetId(), point, vregLive.find(regNO) != vregLive.end());
    if (lr->GetRegType() == kRegTyUndef) {
        lr->SetRegType(regOpnd.GetRegisterType());
    }
    if (candidates.find(regNO) != candidates.end()) {
        lr->AddRefPoint(&insn, isDef);
    }
    if (isDef) {
        vregLive.erase(regNO);
    } else {
        vregLive.insert(regNO);
    }
}

void AArch64LiveIntervalAnalysis::ComputeLiveIntervalsForEachDefOperand(Insn &insn)
{
    const InsnDesc *md = insn.GetDesc();
    uint32 opndNum = insn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        if (insn.GetMachineOpcode() == MOP_asm && (i == kAsmOutputListOpnd || i == kAsmClobberListOpnd)) {
            for (auto opnd : static_cast<ListOperand &>(insn.GetOperand(i)).GetOperands()) {
                SetupLiveIntervalByOp(*static_cast<RegOperand *>(opnd), insn, true);
            }
            continue;
        }
        Operand &opnd = insn.GetOperand(i);
        if (opnd.IsMemoryAccessOperand()) {
            auto &memOpnd = static_cast<MemOperand &>(opnd);
            if (!memOpnd.IsIntactIndexed()) {
                SetupLiveIntervalByOp(opnd, insn, true);
            }
        }
        if (!md->GetOpndDes(i)->IsRegDef()) {
            continue;
        }
        SetupLiveIntervalByOp(opnd, insn, true);
        auto *drivedRef = static_cast<RegOperand&>(opnd).GetBaseRefOpnd();
        if (drivedRef != nullptr) {
            SetupLiveIntervalByOp(*drivedRef, insn, false);
        }
    }
}

void AArch64LiveIntervalAnalysis::ComputeLiveIntervalsForEachUseOperand(Insn &insn)
{
    const InsnDesc *md = insn.GetDesc();
    uint32 opndNum = insn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        if (insn.GetMachineOpcode() == MOP_asm && i == kAsmInputListOpnd) {
            for (auto opnd : static_cast<ListOperand &>(insn.GetOperand(i)).GetOperands()) {
                SetupLiveIntervalByOp(*static_cast<RegOperand *>(opnd), insn, false);
            }
            continue;
        }
        if (md->GetOpndDes(i)->IsRegDef() && !md->GetOpndDes(i)->IsRegUse()) {
            continue;
        }
        Operand &opnd = insn.GetOperand(i);
        if (opnd.IsList()) {
            auto &listOpnd = static_cast<ListOperand &>(opnd);
            for (auto op : listOpnd.GetOperands()) {
                SetupLiveIntervalByOp(*op, insn, false);
            }
        } else if (opnd.IsMemoryAccessOperand()) {
            auto &memOpnd = static_cast<MemOperand &>(opnd);
            Operand *base = memOpnd.GetBaseRegister();
            Operand *offset = memOpnd.GetIndexRegister();
            if (base != nullptr) {
                SetupLiveIntervalByOp(*base, insn, false);
            }
            if (offset != nullptr) {
                SetupLiveIntervalByOp(*offset, insn, false);
            }
        } else if (opnd.IsPhi()) {
            auto &phiOpnd = static_cast<PhiOperand &>(opnd);
            for (auto opIt : phiOpnd.GetOperands()) {
                SetupLiveIntervalByOp(*opIt.second, insn, false);
            }
        } else if (opnd.IsRegister()) {
            SetupLiveIntervalByOp(opnd, insn, false);
            auto *drivedRef = static_cast<RegOperand&>(opnd).GetBaseRefOpnd();
            if (drivedRef != nullptr) {
                SetupLiveIntervalByOp(*drivedRef, insn, false);
            }
        }
    }

    if (insn.GetStackMap() != nullptr) {
        for (auto [_, opnd] : insn.GetStackMap()->GetDeoptInfo().GetDeoptBundleInfo()) {
            SetupLiveIntervalByOp(*opnd, insn, false);
        }
    }
}

/* handle live range for bb->live_out */
void AArch64LiveIntervalAnalysis::SetupLiveIntervalInLiveOut(regno_t liveOut, const BB &bb, uint32 currPoint)
{
    --currPoint;

    if (liveOut >= kAllRegNum) {
        (void)vregLive.insert(liveOut);
        LiveInterval *lr = GetOrCreateLiveInterval(liveOut);
        if (lr == nullptr) {
            return;
        }
        lr->AddRange(bb.GetId(), currPoint, false);
        return;
    }
}

void AArch64LiveIntervalAnalysis::CollectCandidate()
{
    for (size_t bbIdx = bfs->sortedBBs.size(); bbIdx > 0; --bbIdx) {
        BB *bb = bfs->sortedBBs[bbIdx - 1];

        FOR_BB_INSNS_SAFE(insn, bb, ninsn) {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            if (IsRegistersCopy(*insn)) {
                RegOperand &regDest = static_cast<RegOperand &>(insn->GetOperand(kInsnFirstOpnd));
                RegOperand &regSrc = static_cast<RegOperand &>(insn->GetOperand(kInsnSecondOpnd));
                if (regDest.GetRegisterNumber() == regSrc.GetRegisterNumber()) {
                    continue;
                }
                if (regDest.IsVirtualRegister()) {
                    candidates.insert(regDest.GetRegisterNumber());
                }
                if (regSrc.IsVirtualRegister()) {
                    candidates.insert(regSrc.GetRegisterNumber());
                }
            }
        }
    }
}

bool AArch64LiveIntervalAnalysis::IsRegistersCopy(Insn &insn)
{
    MOperator mOp = insn.GetMachineOpcode();
    if (mOp == MOP_xmovrr || mOp == MOP_wmovrr || mOp == MOP_xvmovs || mOp == MOP_xvmovd) {
        return true;
    }
    return false;
}

void AArch64LiveIntervalAnalysis::ComputeLiveIntervals()
{
    /* colloct refpoints and build interfere only for cands. */
    CollectCandidate();

    uint32 currPoint =
        static_cast<uint32>(cgFunc->GetTotalNumberOfInstructions()) + static_cast<uint32>(bfs->sortedBBs.size());
    /* distinguish use/def */
    CHECK_FATAL(currPoint < (INT_MAX >> k4BitShift), "integer overflow check");
    currPoint = currPoint << k4BitShift;
    for (size_t bbIdx = bfs->sortedBBs.size(); bbIdx > 0; --bbIdx) {
        BB *bb = bfs->sortedBBs[bbIdx - 1];

        vregLive.clear();
        for (auto liveOut : bb->GetLiveOutRegNO()) {
            SetupLiveIntervalInLiveOut(liveOut, *bb, currPoint);
        }
        --currPoint;

        if (bb->GetLastInsn() != nullptr && bb->GetLastInsn()->IsMachineInstruction() && bb->GetLastInsn()->IsCall()) {
            UpdateCallInfo();
        }

        FOR_BB_INSNS_REV_SAFE(insn, bb, ninsn) {
            if (!runAnalysis) {
                insn->SetId(currPoint);
            }
            if (!insn->IsMachineInstruction() && !insn->IsPhi()) {
                --currPoint;
                if (ninsn != nullptr && ninsn->IsMachineInstruction() && ninsn->IsCall()) {
                    UpdateCallInfo();
                }
                continue;
            }

            ComputeLiveIntervalsForEachDefOperand(*insn);
            ComputeLiveIntervalsForEachUseOperand(*insn);

            if (ninsn != nullptr && ninsn->IsMachineInstruction() && ninsn->IsCall()) {
                UpdateCallInfo();
            }

            currPoint -= 2; /* 2 for distinguish use/def */
        }
        for (auto lin : bb->GetLiveInRegNO()) {
            if (lin >= kAllRegNum) {
                LiveInterval *li = GetLiveInterval(lin);
                if (li != nullptr) {
                    li->AddRange(bb->GetId(), currPoint, currPoint);
                }
            }
        }
        /* move one more step for each BB */
        --currPoint;
    }

    if (REGCOAL_DUMP) {
        LogInfo::MapleLogger() << "\nAfter ComputeLiveIntervals\n";
        Dump();
    }
}

void AArch64LiveIntervalAnalysis::CheckInterference(LiveInterval &li1, LiveInterval &li2) const
{
    auto ranges1 = li1.GetRanges();
    auto ranges2 = li2.GetRanges();
    bool conflict = false;
    for (auto range : ranges1) {
        auto bbid = range.first;
        auto posVec1 = range.second;
        auto it = ranges2.find(bbid);
        if (it == ranges2.end()) {
            continue;
        } else {
            /* check overlap */
            auto posVec2 = it->second;
            for (auto pos1 : posVec1) {
                for (auto pos2 : posVec2) {
                    if (!((pos1.first < pos2.first && pos1.second < pos2.first) ||
                          (pos2.first < pos1.second && pos2.second < pos1.first))) {
                        conflict = true;
                        break;
                    }
                }
            }
        }
    }
    if (conflict) {
        li1.AddConflict(li2.GetRegNO());
        li2.AddConflict(li1.GetRegNO());
    }
    return;
}

/* replace regDest with regSrc. */
void AArch64LiveIntervalAnalysis::CoalesceRegPair(RegOperand &regDest, RegOperand &regSrc)
{
    LiveInterval *lrDest = GetLiveInterval(regDest.GetRegisterNumber());
    if (lrDest == nullptr) {
        return;
    }
    LiveInterval *lrSrc = GetLiveInterval(regSrc.GetRegisterNumber());
    regno_t destNO = regDest.GetRegisterNumber();
    /* replace all refPoints */
    for (auto insn : lrDest->GetDefPoint()) {
        cgFunc->ReplaceOpndInInsn(regDest, regSrc, *insn, destNO);
    }
    for (auto insn : lrDest->GetUsePoint()) {
        cgFunc->ReplaceOpndInInsn(regDest, regSrc, *insn, destNO);
    }

    DEBUG_ASSERT(lrDest && lrSrc, "get live interval failed");
    CoalesceLiveIntervals(*lrDest, *lrSrc);
}

void AArch64LiveIntervalAnalysis::CollectMoveForEachBB(BB &bb, std::vector<Insn *> &movInsns) const
{
    FOR_BB_INSNS_SAFE(insn, &bb, ninsn) {
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        if (IsRegistersCopy(*insn)) {
            auto &regDest = static_cast<RegOperand &>(insn->GetOperand(kInsnFirstOpnd));
            auto &regSrc = static_cast<RegOperand &>(insn->GetOperand(kInsnSecondOpnd));
            if (!regSrc.IsVirtualRegister() || !regDest.IsVirtualRegister()) {
                continue;
            }
            if (regSrc.GetRegisterNumber() == regDest.GetRegisterNumber()) {
                continue;
            }
            movInsns.emplace_back(insn);
        }
    }
}

void AArch64LiveIntervalAnalysis::CoalesceMoves(std::vector<Insn *> &movInsns, bool phiOnly)
{
    AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc *>(cgFunc);
    bool changed = false;
    do {
        changed = false;
        for (auto insn : movInsns) {
            RegOperand &regDest = static_cast<RegOperand &>(insn->GetOperand(kInsnFirstOpnd));
            RegOperand &regSrc = static_cast<RegOperand &>(insn->GetOperand(kInsnSecondOpnd));
            if (regSrc.GetRegisterNumber() == regDest.GetRegisterNumber()) {
                continue;
            }
            if (!insn->IsPhiMovInsn() && phiOnly) {
                continue;
            }
            if (a64CGFunc->IsRegRematCand(regDest) != a64CGFunc->IsRegRematCand(regSrc)) {
                if (insn->IsPhiMovInsn()) {
                    a64CGFunc->ClearRegRematInfo(regDest);
                    a64CGFunc->ClearRegRematInfo(regSrc);
                } else {
                    continue;
                }
            }
            if (a64CGFunc->IsRegRematCand(regDest) && a64CGFunc->IsRegRematCand(regSrc) &&
                !a64CGFunc->IsRegSameRematInfo(regDest, regSrc)) {
                if (insn->IsPhiMovInsn()) {
                    a64CGFunc->ClearRegRematInfo(regDest);
                    a64CGFunc->ClearRegRematInfo(regSrc);
                } else {
                    continue;
                }
            }
            LiveInterval *li1 = GetLiveInterval(regDest.GetRegisterNumber());
            LiveInterval *li2 = GetLiveInterval(regSrc.GetRegisterNumber());
            if (li1 == nullptr || li2 == nullptr) {
                return;
            }
            CheckInterference(*li1, *li2);
            if (!li1->IsConflictWith(regSrc.GetRegisterNumber()) ||
                (li1->GetDefPoint().size() == 1 && li2->GetDefPoint().size() == 1)) {
                if (REGCOAL_DUMP) {
                    LogInfo::MapleLogger() << "try to coalesce: " << regDest.GetRegisterNumber() << " <- "
                                           << regSrc.GetRegisterNumber() << std::endl;
                }
                CoalesceRegPair(regDest, regSrc);
                changed = true;
            } else {
                if (insn->IsPhiMovInsn() && phiOnly && REGCOAL_DUMP) {
                    LogInfo::MapleLogger() << "fail to coalesce: " << regDest.GetRegisterNumber() << " <- "
                                           << regSrc.GetRegisterNumber() << std::endl;
                }
            }
        }
    } while (changed);
}

void AArch64LiveIntervalAnalysis::CoalesceRegisters()
{
    std::vector<Insn *> movInsns;
    AArch64CGFunc *a64CGFunc = static_cast<AArch64CGFunc *>(cgFunc);
    if (REGCOAL_DUMP) {
        cgFunc->DumpCFGToDot("regcoal-");
        LogInfo::MapleLogger() << "handle function: " << a64CGFunc->GetFunction().GetName() << std::endl;
    }
    for (size_t bbIdx = bfs->sortedBBs.size(); bbIdx > 0; --bbIdx) {
        BB *bb = bfs->sortedBBs[bbIdx - 1];

        if (!bb->GetCritical()) {
            continue;
        }
        CollectMoveForEachBB(*bb, movInsns);
    }
    for (size_t bbIdx = bfs->sortedBBs.size(); bbIdx > 0; --bbIdx) {
        BB *bb = bfs->sortedBBs[bbIdx - 1];

        if (bb->GetCritical()) {
            continue;
        }
        CollectMoveForEachBB(*bb, movInsns);
    }

    bool coalescePhiOnly = (CGOptions::GetInstance().GetOptimizeLevel() != CGOptions::kLevelLiteCG);
    CoalesceMoves(movInsns, coalescePhiOnly);

    /* clean up dead mov */
    a64CGFunc->CleanupDeadMov(REGCOAL_DUMP);
}

} /* namespace maplebe */
