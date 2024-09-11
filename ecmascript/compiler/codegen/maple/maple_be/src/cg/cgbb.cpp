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

#include "cgbb.h"
#include "cgfunc.h"

namespace maplebe {
constexpr uint32 kCondBrNum = 2;
constexpr uint32 kSwitchCaseNum = 5;

const std::string BB::bbNames[BB::kBBLast] = {"BB_ft", "BB_if", "BB_goto", "BB_ret", "BB_noret", "BB_rangegoto"};

Insn *BB::InsertInsnBefore(Insn &existing, Insn &newInsn)
{
    Insn *pre = existing.GetPrev();
    newInsn.SetPrev(pre);
    newInsn.SetNext(&existing);
    existing.SetPrev(&newInsn);
    if (pre != nullptr) {
        pre->SetNext(&newInsn);
    }
    if (&existing == firstInsn) {
        firstInsn = &newInsn;
    }
    newInsn.SetBB(this);
    return &newInsn;
}

Insn *BB::InsertInsnAfter(Insn &existing, Insn &newInsn)
{
    newInsn.SetPrev(&existing);
    newInsn.SetNext(existing.GetNext());
    existing.SetNext(&newInsn);
    if (&existing == lastInsn) {
        lastInsn = &newInsn;
    } else if (newInsn.GetNext()) {
        newInsn.GetNext()->SetPrev(&newInsn);
    }
    newInsn.SetBB(this);
    internalFlag1++;
    return &newInsn;
}

void BB::ReplaceInsn(Insn &insn, Insn &newInsn)
{
    if (insn.IsAccessRefField()) {
        newInsn.MarkAsAccessRefField(true);
    }
    if (insn.GetDoNotRemove()) {
        newInsn.SetDoNotRemove(true);
    }
    newInsn.SetPrev(insn.GetPrev());
    newInsn.SetNext(insn.GetNext());
    if (&insn == lastInsn) {
        lastInsn = &newInsn;
    } else if (newInsn.GetNext() != nullptr) {
        newInsn.GetNext()->SetPrev(&newInsn);
    }
    if (firstInsn == &insn) {
        firstInsn = &newInsn;
    } else if (newInsn.GetPrev() != nullptr) {
        newInsn.GetPrev()->SetNext(&newInsn);
    }
    newInsn.SetComment(insn.GetComment());
    newInsn.SetBB(this);
}

void BB::RemoveInsn(Insn &insn)
{
    if ((firstInsn == &insn) && (lastInsn == &insn)) {
        firstInsn = lastInsn = nullptr;
    } else if (firstInsn == &insn) {
        firstInsn = insn.GetNext();
    } else if (lastInsn == &insn) {
        lastInsn = insn.GetPrev();
    }
    /* remove insn from lir list */
    Insn *prevInsn = insn.GetPrev();
    Insn *nextInsn = insn.GetNext();
    if (prevInsn != nullptr) {
        prevInsn->SetNext(nextInsn);
    }
    if (nextInsn != nullptr) {
        nextInsn->SetPrev(prevInsn);
    }
}

/* Remove insns in this bb from insn1 to insn2. */
void BB::RemoveInsnSequence(Insn &insn, const Insn &nextInsn)
{
    DEBUG_ASSERT(insn.GetBB() == this, "remove insn sequence in one bb");
    DEBUG_ASSERT(nextInsn.GetBB() == this, "remove insn sequence in one bb");
    if ((firstInsn == &insn) && (lastInsn == &nextInsn)) {
        firstInsn = lastInsn = nullptr;
    } else if (firstInsn == &insn) {
        firstInsn = nextInsn.GetNext();
    } else if (lastInsn == &nextInsn) {
        lastInsn = insn.GetPrev();
    }

    if (insn.GetPrev() != nullptr) {
        insn.GetPrev()->SetNext(nextInsn.GetNext());
    }
    if (nextInsn.GetNext() != nullptr) {
        nextInsn.GetNext()->SetPrev(insn.GetPrev());
    }
}

/* append all insns from bb into this bb */
void BB::AppendBBInsns(BB &bb)
{
    if (firstInsn == nullptr) {
        firstInsn = bb.firstInsn;
        lastInsn = bb.lastInsn;
        if (firstInsn != nullptr) {
            FOR_BB_INSNS(i, &bb)
            {
                i->SetBB(this);
            }
        }
        return;
    }
    if ((bb.firstInsn == nullptr) || (bb.lastInsn == nullptr)) {
        return;
    }
    FOR_BB_INSNS_SAFE(insn, &bb, nextInsn)
    {
        AppendInsn(*insn);
    }
}

/* prepend all insns from bb into this bb */
void BB::InsertAtBeginning(BB &bb)
{
    if (bb.firstInsn == nullptr) { /* nothing to add */
        return;
    }

    FOR_BB_INSNS(insn, &bb)
    {
        insn->SetBB(this);
    }

    if (firstInsn == nullptr) {
        firstInsn = bb.firstInsn;
        lastInsn = bb.lastInsn;
    } else {
        bb.lastInsn->SetNext(firstInsn);
        firstInsn->SetPrev(bb.lastInsn);
        firstInsn = bb.firstInsn;
    }
    bb.firstInsn = bb.lastInsn = nullptr;
}

/* append all insns from bb into this bb */
void BB::InsertAtEnd(BB &bb)
{
    if (bb.firstInsn == nullptr) { /* nothing to add */
        return;
    }

    FOR_BB_INSNS(insn, &bb)
    {
        insn->SetBB(this);
    }

    if (firstInsn == nullptr) {
        firstInsn = bb.firstInsn;
        lastInsn = bb.lastInsn;
    } else {
        bb.firstInsn->SetPrev(lastInsn);
        lastInsn->SetNext(bb.firstInsn);
        lastInsn = bb.lastInsn;
    }
    bb.firstInsn = bb.lastInsn = nullptr;
}

/* Number of instructions excluding DbgInsn and comments */
int32 BB::NumInsn() const
{
    int32 bbSize = 0;
    FOR_BB_INSNS_CONST(i, this)
    {
        if (i->IsImmaterialInsn() || i->IsDbgInsn()) {
            continue;
        }
        ++bbSize;
    }
    return bbSize;
}

void BB::Dump() const
{
    LogInfo::MapleLogger() << "=== BB " << this << " <" << GetKindName();
    if (labIdx) {
        LogInfo::MapleLogger() << "[labeled with " << labIdx << "]";
        if (labelTaken) {
            LogInfo::MapleLogger() << " taken";
        }
    }
    LogInfo::MapleLogger() << "> <" << id << "> ";
    if (isCleanup) {
        LogInfo::MapleLogger() << "[is_cleanup] ";
    }
    if (unreachable) {
        LogInfo::MapleLogger() << "[unreachable] ";
    }
    LogInfo::MapleLogger() << "frequency:" << frequency << "===\n";

    Insn *insn = firstInsn;
    while (insn != nullptr) {
        insn->Dump();
        insn = insn->GetNext();
    }
}

bool BB::IsCommentBB() const
{
    if (GetKind() != kBBFallthru) {
        return false;
    }
    FOR_BB_INSNS_CONST(insn, this)
    {
        if (insn->IsMachineInstruction()) {
            return false;
        }
    }
    return true;
}

/* return true if bb has no real insns. */
bool BB::IsEmptyOrCommentOnly() const
{
    return (IsEmpty() || IsCommentBB());
}

bool BB::IsSoloGoto() const
{
    if (GetKind() != kBBGoto) {
        return false;
    }
    if (GetHasCfi()) {
        return false;
    }
    FOR_BB_INSNS_CONST(insn, this)
    {
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        return (insn->IsUnCondBranch());
    }
    return false;
}

void Bfs::SeekCycles()
{
    MapleVector<bool> visited(cgfunc->NumBBs(), false, alloc.Adapter());
    MapleVector<bool> onPath(cgfunc->NumBBs(), false, alloc.Adapter());
    MapleStack<BB*> workStack(alloc.Adapter());

    // searhing workStack BBs cycle
    auto seekCycles = [this, &visited, &onPath, &workStack]() {
        while (!workStack.empty()) {
            auto *bb = workStack.top();
            if (visited[bb->GetId()]) {
                onPath[bb->GetId()] = false;
                workStack.pop();
                continue;
            }

            visited[bb->GetId()] = true;
            onPath[bb->GetId()] = true;
            for (auto *succBB : bb->GetSuccs()) {
                if (!visited[succBB->GetId()]) {
                    workStack.push(succBB);
                } else if (onPath[succBB->GetId()]) {
                    (void)cycleSuccs[bb->GetId()].insert(succBB->GetId());
                }
            }
        }
    };

    bool changed = false;
    do {
        changed = false;
        FOR_ALL_BB(bb, cgfunc)
        {
            if (!visited[bb->GetId()]) {
                workStack.push(bb);
                seekCycles();
                changed = true;
            }
        }
    } while (changed);
}

bool Bfs::AllPredBBVisited(const BB &bb, long &level) const
{
    // check pred bb is in cycle
    auto predBBInCycle = [this](const BB &bb, const BB &predBB) {
        for (auto bbId : cycleSuccs[predBB.GetId()]) {
            if (bb.GetId() == bbId) {
                return true;
            }
        }
        return false;
    };

    bool isAllPredsVisited = true;
    for (const auto *predBB : bb.GetPreds()) {
        if (!predBBInCycle(bb, *predBB) && !visitedBBs[predBB->GetId()]) {
            isAllPredsVisited = false;
            break;
        }
        level = std::max(level, predBB->GetInternalFlag2());
    }
    return isAllPredsVisited;
}

/*
 * During live interval construction, bb has only one predecessor and/or one
 * successor are stright line bb.  It can be considered to be a single large bb
 * for the purpose of finding live interval.  This is to prevent extending live
 * interval of registers unnecessarily when interleaving bb from other paths.
 */
BB *Bfs::MarkStraightLineBBInBFS(BB *bb)
{
    while (true) {
        if (bb->GetSuccs().size() != 1) {
            break;
        }
        BB *sbb = bb->GetSuccs().front();
        if (visitedBBs[sbb->GetId()]) {
            break;
        }
        if (sbb->GetPreds().size() != 1) {
            break;
        }
        sortedBBs.push_back(sbb);
        visitedBBs[sbb->GetId()] = true;
        sbb->SetInternalFlag2(bb->GetInternalFlag2() + 1);
        bb = sbb;
    }
    return bb;
}

BB *Bfs::SearchForStraightLineBBs(BB &bb)
{
    if ((bb.GetSuccs().size() != kCondBrNum)) {
        return &bb;
    }
    BB *sbb1 = bb.GetSuccs().front();
    BB *sbb2 = bb.GetSuccs().back();
    size_t predSz1 = sbb1->GetPreds().size();
    size_t predSz2 = sbb2->GetPreds().size();
    BB *candidateBB = nullptr;
    if ((predSz1 == 1) && (predSz2 > kSwitchCaseNum)) {
        candidateBB = sbb1;
    } else if ((predSz2 == 1) && (predSz1 > kSwitchCaseNum)) {
        candidateBB = sbb2;
    } else {
        return &bb;
    }
    DEBUG_ASSERT(candidateBB->GetId() < visitedBBs.size(), "index out of range in RA::SearchForStraightLineBBs");
    if (visitedBBs[candidateBB->GetId()]) {
        return &bb;
    }
    if (candidateBB->GetSuccs().size() != 1) {
        return &bb;
    }

    sortedBBs.push_back(candidateBB);
    visitedBBs[candidateBB->GetId()] = true;
    return MarkStraightLineBBInBFS(candidateBB);
}

void Bfs::BFS(BB &curBB)
{
    std::queue<BB *> workList;
    workList.push(&curBB);
    DEBUG_ASSERT(curBB.GetId() < cgfunc->NumBBs(), "RA::BFS visitedBBs overflow");
    DEBUG_ASSERT(curBB.GetId() < visitedBBs.size(), "index out of range in RA::BFS");
    visitedBBs[curBB.GetId()] = true;
    do {
        BB *bb = workList.front();
        sortedBBs.push_back(bb);
        DEBUG_ASSERT(bb->GetId() < cgfunc->NumBBs(), "RA::BFS visitedBBs overflow");
        visitedBBs[bb->GetId()] = true;
        workList.pop();
        /* Look for straight line bb */
        bb = MarkStraightLineBBInBFS(bb);
        /* Look for an 'if' followed by some straight-line bb */
        bb = SearchForStraightLineBBs(*bb);
        for (auto *ibb : bb->GetSuccs()) {
            /* See if there are unvisited predecessor */
            if (visitedBBs[ibb->GetId()]) {
                continue;
            }
            long prevLevel = 0;
            if (AllPredBBVisited(*ibb, prevLevel)) {
                ibb->SetInternalFlag2(prevLevel + 1);
                workList.push(ibb);
                DEBUG_ASSERT(ibb->GetId() < cgfunc->NumBBs(), "GCRA::BFS visitedBBs overflow");
                visitedBBs[ibb->GetId()] = true;
            }
        }
    } while (!workList.empty());
}

void Bfs::ComputeBlockOrder()
{
    cycleSuccs.assign(cgfunc->NumBBs(), MapleSet<BBID>(alloc.Adapter()));
    SeekCycles();

    sortedBBs.clear();
    visitedBBs.assign(cgfunc->NumBBs(), false);
    BB *cleanupBB = nullptr;
    FOR_ALL_BB(bb, cgfunc)
    {
        bb->SetInternalFlag1(0);
        bb->SetInternalFlag2(1);
        if (bb->IsCleanup()) {
            DEBUG_ASSERT(cleanupBB == nullptr, "one cleanupBB in the function only");
            cleanupBB = bb;
        }
    }
    if (cleanupBB != nullptr) {
        cleanupBB->SetInternalFlag1(1);
    }

    bool changed;
    size_t sortedCnt = 0;
    bool done = false;
    do {
        changed = false;
        FOR_ALL_BB(bb, cgfunc)
        {
            if (bb->GetInternalFlag1() == 1 || bb->IsUnreachable()) {
                continue;
            }
            if (visitedBBs[bb->GetId()]) {
                continue;
            }
            changed = true;
            long prevLevel = 0;
            if (AllPredBBVisited(*bb, prevLevel)) {
                bb->SetInternalFlag2(prevLevel + 1);
                BFS(*bb);
            }
        }
        /* Make sure there is no infinite loop. */
        if (sortedCnt == sortedBBs.size()) {
            if (!done) {
                done = true;
            } else {
                LogInfo::MapleLogger() << "Error: RA BFS loop " << sortedCnt
                                       << " in func " << cgfunc->GetName() << "\n";
                CHECK_FATAL(false, "");
            }
        }
        sortedCnt = sortedBBs.size();
    } while (changed);

    if (cleanupBB != nullptr) {
        sortedBBs.push_back(cleanupBB);
    }
}

void CgBBSort::GetAnalysisDependence(AnalysisDep &aDep) const
{
#if TARGX86_64
    if (Triple::GetTriple().GetArch() == Triple::ArchType::x64) {
        aDep.AddRequired<CgHandleCFG>();
    }
#endif
    aDep.SetPreservedAll();
}

bool CgBBSort::PhaseRun(CGFunc &f)
{
    MemPool *memPool = GetPhaseMemPool();
    bfs = memPool->New<Bfs>(f, *memPool);
    CHECK_FATAL(bfs != nullptr, "NIY, ptr null check.");
    bfs->ComputeBlockOrder();
    return false;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgBBSort, bbsort)
} /* namespace maplebe */
