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
#include "cg.h"
#include "optimize_common.h"

namespace maplebe {
#define LOOP_ANALYSIS_DUMP_NEWPM CG_DEBUG_FUNC(f)

static void PrintLoopInfo(const LoopHierarchy &loop)
{
    LogInfo::MapleLogger() << "header " << loop.GetHeader()->GetId();
    if (loop.otherLoopEntries.size()) {
        LogInfo::MapleLogger() << " multi-header ";
        for (auto en : loop.otherLoopEntries) {
            LogInfo::MapleLogger() << en->GetId() << " ";
        }
    }
    if (loop.GetOuterLoop() != nullptr) {
        LogInfo::MapleLogger() << " parent " << loop.GetOuterLoop()->GetHeader()->GetId();
    }
    LogInfo::MapleLogger() << " backedge ";
    for (auto *bb : loop.GetBackedge()) {
        LogInfo::MapleLogger() << bb->GetId() << " ";
    }
    LogInfo::MapleLogger() << "\n members ";
    for (auto *bb : loop.GetLoopMembers()) {
        LogInfo::MapleLogger() << bb->GetId() << " ";
    }
    if (!loop.GetInnerLoops().empty()) {
        LogInfo::MapleLogger() << "\n inner_loop_headers ";
        for (auto *inner : loop.GetInnerLoops()) {
            LogInfo::MapleLogger() << inner->GetHeader()->GetId() << " ";
        }
    }
    LogInfo::MapleLogger() << "\n";
}

static void PrintInner(const LoopHierarchy &loop, uint32 level)
{
    for (auto *inner : loop.GetInnerLoops()) {
        LogInfo::MapleLogger() << "loop-level-" << level << "\n";
        PrintLoopInfo(*inner);
        PrintInner(*inner, level + 1);
    }
}

void LoopHierarchy::PrintLoops(const std::string &name) const
{
    LogInfo::MapleLogger() << name << "\n";
    for (const LoopHierarchy *loop = this; loop != nullptr; loop = loop->next) {
        PrintLoopInfo(*loop);
    }
    for (const LoopHierarchy *loop = this; loop != nullptr; loop = loop->next) {
        PrintInner(*loop, 1);
    }
}

bool CGFuncLoops::IsBBLoopMember(const BB *bb)
{
    return (*(std::find(loopMembers.begin(), loopMembers.end(), bb)) == bb);
}

void CGFuncLoops::CheckOverlappingInnerLoops(const MapleVector<CGFuncLoops *> &iLoops,
                                             const MapleVector<BB *> &loopMem) const
{
    for (auto iloop : iLoops) {
        CHECK_FATAL(iloop->loopMembers.size() > 0, "Empty loop");
        for (auto bb : iloop->loopMembers) {
            if (find(loopMem.begin(), loopMem.end(), bb) != loopMem.end()) {
                LogInfo::MapleLogger() << "Error: inconsistent loop member";
                CHECK_FATAL(0, "loop member overlap with inner loop");
            }
        }
        CheckOverlappingInnerLoops(iloop->innerLoops, loopMem);
    }
}

void CGFuncLoops::CheckLoops() const
{
    // Make sure backedge -> header relationship holds
    for (auto bEdge : backedge) {
        if (find(bEdge->GetSuccs().begin(), bEdge->GetSuccs().end(), header) == bEdge->GetSuccs().end()) {
            bool inOtherEntry = false;
            for (auto entry : multiEntries) {
                if (find(bEdge->GetSuccs().begin(), bEdge->GetSuccs().end(), entry) != bEdge->GetSuccs().end()) {
                    inOtherEntry = true;
                    break;
                }
            }
            if (inOtherEntry == false) {
                if (find(bEdge->GetEhSuccs().begin(), bEdge->GetEhSuccs().end(), header) == bEdge->GetEhSuccs().end()) {
                    LogInfo::MapleLogger() << "Error: inconsistent loop backedge";
                    CHECK_FATAL(0, "loop backedge does not go to loop header");
                }
            }
        }
        if (find(header->GetPreds().begin(), header->GetPreds().end(), bEdge) == header->GetPreds().end()) {
            bool inOtherEntry = false;
            for (auto entry : multiEntries) {
                if (find(entry->GetPreds().begin(), entry->GetPreds().end(), bEdge) != entry->GetPreds().end()) {
                    inOtherEntry = true;
                    break;
                }
            }
            if (inOtherEntry == false) {
                if (find(header->GetEhPreds().begin(), header->GetEhPreds().end(), bEdge) ==
                    header->GetEhPreds().end()) {
                    LogInfo::MapleLogger() << "Error: inconsistent loop header";
                    CHECK_FATAL(0, "loop header does not have a backedge");
                }
            }
        }
    }

    // Make sure containing loop members do not overlap
    CheckOverlappingInnerLoops(innerLoops, loopMembers);

    if (innerLoops.empty() == false) {
        for (auto lp : innerLoops) {
            lp->CheckLoops();
        }
    }
}

void CGFuncLoops::PrintLoops(const CGFuncLoops &funcLoop) const
{
    LogInfo::MapleLogger() << "loop_level(" << funcLoop.loopLevel << ") ";
    LogInfo::MapleLogger() << "header " << funcLoop.GetHeader()->GetId() << " ";
    if (funcLoop.multiEntries.size()) {
        LogInfo::MapleLogger() << "other-header ";
        for (auto bb : funcLoop.multiEntries) {
            LogInfo::MapleLogger() << bb->GetId() << " ";
        }
    }
    if (funcLoop.GetOuterLoop() != nullptr) {
        LogInfo::MapleLogger() << "parent " << funcLoop.GetOuterLoop()->GetHeader()->GetId() << " ";
    }
    LogInfo::MapleLogger() << "backedge ";
    for (auto *bb : funcLoop.GetBackedge()) {
        LogInfo::MapleLogger() << bb->GetId() << " ";
    }
    LogInfo::MapleLogger() << "\n members ";
    for (auto *bb : funcLoop.GetLoopMembers()) {
        LogInfo::MapleLogger() << bb->GetId() << " ";
    }
    LogInfo::MapleLogger() << "\n exits ";
    for (auto *bb : funcLoop.GetExits()) {
        LogInfo::MapleLogger() << bb->GetId() << " ";
    }
    LogInfo::MapleLogger() << "\n";
    if (!funcLoop.GetInnerLoops().empty()) {
        LogInfo::MapleLogger() << " inner_loop_headers ";
        for (auto *inner : funcLoop.GetInnerLoops()) {
            LogInfo::MapleLogger() << inner->GetHeader()->GetId() << " ";
        }
        LogInfo::MapleLogger() << "\n";
        for (auto *inner : funcLoop.GetInnerLoops()) {
            PrintLoops(*inner);
        }
    }
}

// partial loop body found with formLoop is NOT really needed in down stream
//       It should be simplied later
void LoopFinder::formLoop(BB *headBB, BB *backBB)
{
    DEBUG_ASSERT(headBB != nullptr && backBB != nullptr, "headBB or backBB is nullptr");
    LoopHierarchy *simple_loop = memPool->New<LoopHierarchy>(*memPool);

    if (headBB != backBB) {
        DEBUG_ASSERT(!dfsBBs.empty(), "dfsBBs is empty");
        DEBUG_ASSERT(onPathBBs[headBB->GetId()], "headBB is not on execution path");
        std::stack<BB *> tempStk;

        tempStk.push(dfsBBs.top());
        dfsBBs.pop();

        while (tempStk.top() != headBB && !dfsBBs.empty()) {
            tempStk.push(dfsBBs.top());
            dfsBBs.pop();
        }

        while (!tempStk.empty()) {
            BB *topBB = tempStk.top();
            tempStk.pop();

            if (onPathBBs[topBB->GetId()]) {
                simple_loop->InsertLoopMembers(*topBB);
            }
            dfsBBs.push(topBB);
        }
    }
    // Note: backBB is NOT on dfsBBs
    simple_loop->InsertLoopMembers(*backBB);
    simple_loop->SetHeader(*headBB);
    simple_loop->InsertBackedge(*backBB);

    if (loops) {
        loops->SetPrev(simple_loop);
    }
    simple_loop->SetNext(loops);
    loops = simple_loop;
}

void LoopFinder::seekBackEdge(BB *bb, MapleList<BB *> succs)
{
    for (const auto succBB : succs) {
        if (!visitedBBs[succBB->GetId()]) {
            dfsBBs.push(succBB);
        } else {
            if (onPathBBs[succBB->GetId()]) {
                formLoop(succBB, bb);
                bb->PushBackLoopSuccs(*succBB);
                succBB->PushBackLoopPreds(*bb);
            }
        }
    }
}

void LoopFinder::seekCycles()
{
    while (!dfsBBs.empty()) {
        BB *bb = dfsBBs.top();
        if (visitedBBs[bb->GetId()]) {
            onPathBBs[bb->GetId()] = false;
            dfsBBs.pop();
            continue;
        }

        visitedBBs[bb->GetId()] = true;
        onPathBBs[bb->GetId()] = true;
        seekBackEdge(bb, bb->GetSuccs());
        seekBackEdge(bb, bb->GetEhSuccs());
    }
}

void LoopFinder::markExtraEntryAndEncl()
{
    DEBUG_ASSERT(dfsBBs.empty(), "dfsBBs is NOT empty");
    std::vector<BB *> loopEnclosure;
    loopEnclosure.resize(cgFunc->NumBBs());
    std::vector<bool> startProcess;
    startProcess.resize(cgFunc->NumBBs());
    std::vector<BB *> origEntries;
    origEntries.resize(cgFunc->NumBBs());
    std::vector<BB *> newEntries;
    newEntries.resize(cgFunc->NumBBs());

    for (LoopHierarchy *loop = loops; loop != nullptr; loop = loop->GetNext()) {
        fill(visitedBBs.begin(), visitedBBs.end(), false);
        fill(loopEnclosure.begin(), loopEnclosure.end(), nullptr);
        fill(startProcess.begin(), startProcess.end(), false);
        fill(origEntries.begin(), origEntries.end(), nullptr);
        fill(newEntries.begin(), newEntries.end(), nullptr);

        for (auto *bb : loop->GetLoopMembers()) {
            loopEnclosure[bb->GetId()] = bb;
        }
        origEntries[loop->GetHeader()->GetId()] = loop->GetHeader();

        // Form loop closure from the primary entry. At end collect all other entries
        bool changed = false;
        dfsBBs.push(loop->GetHeader());
        while (true) {
            while (!dfsBBs.empty()) {
                BB *bb = dfsBBs.top();
                visitedBBs[bb->GetId()] = true;
                if (startProcess[bb->GetId()]) {
                    dfsBBs.pop();
                    for (const auto succBB : bb->GetSuccs()) {
                        if (loopEnclosure[bb->GetId()] == nullptr && loopEnclosure[succBB->GetId()] != nullptr &&
                            succBB != loop->GetHeader()) {
                            changed = true;
                            loopEnclosure[bb->GetId()] = bb;
                            break;
                        }
                    }
                    continue;
                } else {
                    startProcess[bb->GetId()] = true;
                    for (const auto succBB : bb->GetSuccs()) {
                        if (!visitedBBs[succBB->GetId()]) {
                            dfsBBs.push(succBB);
                        }
                    }
                }
            }

            // Repeat till no new item is added in
            if (changed) {
                dfsBBs.push(loop->GetHeader());
                changed = false;
                fill(visitedBBs.begin(), visitedBBs.end(), false);
                fill(startProcess.begin(), startProcess.end(), false);
                continue;
            }

            // Collect all entries
            bool foundNewEntry = false;
            fill(visitedBBs.begin(), visitedBBs.end(), false);
            FOR_ALL_BB(bb, cgFunc)
            {
                if (!visitedBBs[bb->GetId()]) {
                    dfsBBs.push(bb);
                    visitedBBs[bb->GetId()] = true;
                    while (!dfsBBs.empty()) {
                        BB *currBB = dfsBBs.top();
                        visitedBBs[currBB->GetId()] = true;
                        dfsBBs.pop();
                        for (const auto succBB : currBB->GetSuccs()) {
                            // check if entering a loop.
                            if ((loopEnclosure[succBB->GetId()] != nullptr) &&
                                (loopEnclosure[currBB->GetId()] == nullptr)) {
                                newEntries[succBB->GetId()] = succBB;
                                if (origEntries[succBB->GetId()] == nullptr) {
                                    foundNewEntry = true;
                                }
                            }
                            if (!visitedBBs[succBB->GetId()]) {
                                dfsBBs.push(succBB);
                            }
                        }
                    }
                }
            }
            if (foundNewEntry) {
                origEntries = newEntries;
                for (const auto bb : newEntries) {
                    if (bb != nullptr) {
                        dfsBBs.push(bb);
                    }
                }
                fill(visitedBBs.begin(), visitedBBs.end(), false);
                fill(startProcess.begin(), startProcess.end(), false);
                fill(newEntries.begin(), newEntries.end(), nullptr);
            } else {
                break;
            }
        }

        // Setup loop body
        for (size_t id = 0; id < loopEnclosure.size(); id++) {
            if (loopEnclosure[id] != nullptr) {
                loop->InsertLoopMembers(*loopEnclosure[id]);
            }
        }

        // Setup head and extra entries
        for (const auto bb : newEntries) {
            if (bb != nullptr) {
                loop->otherLoopEntries.insert(bb);
            }
        }
        loop->otherLoopEntries.erase(loop->GetHeader());
    }
}

bool LoopFinder::HasSameHeader(const LoopHierarchy *lp1, const LoopHierarchy *lp2)
{
    if (lp1->GetHeader() == lp2->GetHeader()) {
        return true;
    }
    for (auto other1 : lp1->otherLoopEntries) {
        if (lp2->GetHeader() == other1) {
            return true;
        }
        for (auto other2 : lp2->otherLoopEntries) {
            if (other2 == other1) {
                return true;
            }
        }
    }
    return false;
}

void LoopFinder::MergeLoops()
{
    for (LoopHierarchy *loopHierarchy1 = loops; loopHierarchy1 != nullptr; loopHierarchy1 = loopHierarchy1->GetNext()) {
        for (LoopHierarchy *loopHierarchy2 = loopHierarchy1->GetNext(); loopHierarchy2 != nullptr;
             loopHierarchy2 = loopHierarchy2->GetNext()) {
            // Different loop bodies imply different loops
            bool sameLoop = true;
            if (loopHierarchy1->GetLoopMembers().size() == loopHierarchy2->GetLoopMembers().size()) {
                for (auto *bb : loopHierarchy2->GetLoopMembers()) {
                    if (find(loopHierarchy1->GetLoopMembers().begin(), loopHierarchy1->GetLoopMembers().end(), bb) ==
                        loopHierarchy1->GetLoopMembers().end()) {
                        sameLoop = false;
                        break;
                    }
                }
                if (sameLoop) {
                    for (auto *bb : loopHierarchy1->GetLoopMembers()) {
                        if (find(loopHierarchy2->GetLoopMembers().begin(), loopHierarchy2->GetLoopMembers().end(),
                                 bb) == loopHierarchy2->GetLoopMembers().end()) {
                            sameLoop = false;
                            break;
                        }
                    }
                }
                if (sameLoop) {
                    loopHierarchy2->GetPrev()->SetNext(loopHierarchy2->GetNext());
                    if (loopHierarchy2->GetNext() != nullptr) {
                        loopHierarchy2->GetNext()->SetPrev(loopHierarchy2->GetPrev());
                    }
                    continue;
                }
            }
            if (HasSameHeader(loopHierarchy1, loopHierarchy2) == false) {
                continue;
            }
            for (auto *bb : loopHierarchy2->GetLoopMembers()) {
                loopHierarchy1->InsertLoopMembers(*bb);
            }
            if (loopHierarchy1->GetHeader() != loopHierarchy2->GetHeader()) {
                loopHierarchy1->otherLoopEntries.insert(loopHierarchy2->GetHeader());
            }
            for (auto bb : loopHierarchy2->otherLoopEntries) {
                if (loopHierarchy1->GetHeader() != bb) {
                    loopHierarchy1->otherLoopEntries.insert(bb);
                }
            }
            for (auto *bb : loopHierarchy2->GetBackedge()) {
                loopHierarchy1->InsertBackedge(*bb);
            }
            loopHierarchy2->GetPrev()->SetNext(loopHierarchy2->GetNext());
            if (loopHierarchy2->GetNext() != nullptr) {
                loopHierarchy2->GetNext()->SetPrev(loopHierarchy2->GetPrev());
            }
        }
    }
}

void LoopFinder::SortLoops()
{
    LoopHierarchy *head = nullptr;
    LoopHierarchy *next1 = nullptr;
    LoopHierarchy *next2 = nullptr;
    bool swapped;
    do {
        swapped = false;
        for (LoopHierarchy *loopHierarchy1 = loops; loopHierarchy1 != nullptr;) {
            /* remember loopHierarchy1's prev in case if loopHierarchy1 moved */
            head = loopHierarchy1;
            next1 = loopHierarchy1->GetNext();
            for (LoopHierarchy *loopHierarchy2 = loopHierarchy1->GetNext(); loopHierarchy2 != nullptr;) {
                next2 = loopHierarchy2->GetNext();

                if (loopHierarchy1->GetLoopMembers().size() > loopHierarchy2->GetLoopMembers().size()) {
                    if (head->GetPrev() == nullptr) {
                        /* remove loopHierarchy2 from list */
                        loopHierarchy2->GetPrev()->SetNext(loopHierarchy2->GetNext());
                        if (loopHierarchy2->GetNext() != nullptr) {
                            loopHierarchy2->GetNext()->SetPrev(loopHierarchy2->GetPrev());
                        }
                        /* link loopHierarchy2 as head */
                        loops = loopHierarchy2;
                        loopHierarchy2->SetPrev(nullptr);
                        loopHierarchy2->SetNext(head);
                        head->SetPrev(loopHierarchy2);
                    } else {
                        loopHierarchy2->GetPrev()->SetNext(loopHierarchy2->GetNext());
                        if (loopHierarchy2->GetNext() != nullptr) {
                            loopHierarchy2->GetNext()->SetPrev(loopHierarchy2->GetPrev());
                        }
                        head->GetPrev()->SetNext(loopHierarchy2);
                        loopHierarchy2->SetPrev(head->GetPrev());
                        loopHierarchy2->SetNext(head);
                        head->SetPrev(loopHierarchy2);
                    }
                    head = loopHierarchy2;
                    swapped = true;
                }
                loopHierarchy2 = next2;
            }
            loopHierarchy1 = next1;
        }
    } while (swapped);
}

void LoopFinder::UpdateOuterForInnerLoop(BB *bb, LoopHierarchy *outer)
{
    if (outer == nullptr) {
        return;
    }
    for (auto ito = outer->GetLoopMembers().begin(); ito != outer->GetLoopMembers().end();) {
        if (*ito == bb) {
            ito = outer->EraseLoopMembers(ito);
        } else {
            ++ito;
        }
    }
    if (outer->GetOuterLoop() != nullptr) {
        UpdateOuterForInnerLoop(bb, const_cast<LoopHierarchy *>(outer->GetOuterLoop()));
    }
}

void LoopFinder::UpdateOuterLoop(const LoopHierarchy *loop)
{
    for (auto inner : loop->GetInnerLoops()) {
        UpdateOuterLoop(inner);
    }
    for (auto *bb : loop->GetLoopMembers()) {
        UpdateOuterForInnerLoop(bb, const_cast<LoopHierarchy *>(loop->GetOuterLoop()));
    }
}

void LoopFinder::CreateInnerLoop(LoopHierarchy &inner, LoopHierarchy &outer)
{
    outer.InsertInnerLoops(inner);
    inner.SetOuterLoop(outer);
    if (loops == &inner) {
        loops = inner.GetNext();
    } else {
        LoopHierarchy *prev = loops;
        for (LoopHierarchy *loopHierarchy1 = loops->GetNext(); loopHierarchy1 != nullptr;
             loopHierarchy1 = loopHierarchy1->GetNext()) {
            if (loopHierarchy1 == &inner) {
                prev->SetNext(prev->GetNext()->GetNext());
            }
            prev = loopHierarchy1;
        }
    }
}

static void FindLoopExits(LoopHierarchy *loop)
{
    for (auto *bb : loop->GetLoopMembers()) {
        for (auto succ : bb->GetSuccs()) {
            if (find(loop->GetLoopMembers().begin(), loop->GetLoopMembers().end(), succ) ==
                loop->GetLoopMembers().end()) {
                loop->InsertExit(*bb);
            }
        }
    }
    for (auto *inner : loop->GetInnerLoops()) {
        FindLoopExits(inner);
    }
}

void LoopFinder::DetectInnerLoop()
{
    for (LoopHierarchy *loop = loops; loop != nullptr; loop = loop->GetNext()) {
        FindLoopExits(loop);
    }
    bool innerCreated;
    do {
        innerCreated = false;
        for (LoopHierarchy *loopHierarchy1 = loops; loopHierarchy1 != nullptr;
             loopHierarchy1 = loopHierarchy1->GetNext()) {
            for (LoopHierarchy *loopHierarchy2 = loopHierarchy1->GetNext(); loopHierarchy2 != nullptr;
                 loopHierarchy2 = loopHierarchy2->GetNext()) {
                if (loopHierarchy1->GetHeader() != loopHierarchy2->GetHeader()) {
                    auto &loopHierarchy2Members = loopHierarchy2->GetLoopMembers();
                    if (find(loopHierarchy2Members.begin(), loopHierarchy2Members.end(), loopHierarchy1->GetHeader()) !=
                        loopHierarchy2Members.end()) {
                        bool allin = true;
                        // Make sure body is included
                        for (auto *bb1 : loopHierarchy1->GetLoopMembers()) {
                            if (find(loopHierarchy2Members.begin(), loopHierarchy2Members.end(), bb1) ==
                                loopHierarchy2Members.end()) {
                                allin = false;
                                break;
                            }
                        }
                        if (allin) {
                            CreateInnerLoop(*loopHierarchy1, *loopHierarchy2);
                            innerCreated = true;
                        }
                    }
                    if (innerCreated) {
                        break;
                    }
                }
            }
            if (innerCreated) {
                break;
            }
        }
    } while (innerCreated);

    for (LoopHierarchy *outer = loops; outer != nullptr; outer = outer->GetNext()) {
        UpdateOuterLoop(outer);
    }
}

static void CopyLoopInfo(const LoopHierarchy *from, CGFuncLoops *to, CGFuncLoops *parent, MemPool *memPool)
{
    to->SetHeader(*const_cast<BB *>(from->GetHeader()));
    for (auto bb : from->otherLoopEntries) {
        to->AddMultiEntries(*bb);
    }
    for (auto *bb : from->GetLoopMembers()) {
        to->AddLoopMembers(*bb);
        bb->SetLoop(*to);
    }
    for (auto *bb : from->GetBackedge()) {
        to->AddBackedge(*bb);
    }
    for (auto *bb : from->GetExits()) {
        to->AddExit(*bb);
    }
    if (!from->GetInnerLoops().empty()) {
        for (auto *inner : from->GetInnerLoops()) {
            CGFuncLoops *floop = memPool->New<CGFuncLoops>(*memPool);
            to->AddInnerLoops(*floop);
            floop->SetLoopLevel(to->GetLoopLevel() + 1);
            CopyLoopInfo(inner, floop, to, memPool);
        }
    }
    if (parent != nullptr) {
        to->SetOuterLoop(*parent);
    }
}

void LoopFinder::UpdateCGFunc()
{
    for (LoopHierarchy *loop = loops; loop != nullptr; loop = loop->GetNext()) {
        CGFuncLoops *floop = cgFunc->GetMemoryPool()->New<CGFuncLoops>(*cgFunc->GetMemoryPool());
        cgFunc->PushBackLoops(*floop);
        floop->SetLoopLevel(1); /* top level */
        CopyLoopInfo(loop, floop, nullptr, cgFunc->GetMemoryPool());
    }
}

void LoopFinder::FormLoopHierarchy()
{
    visitedBBs.clear();
    visitedBBs.resize(cgFunc->NumBBs(), false);
    sortedBBs.clear();
    sortedBBs.resize(cgFunc->NumBBs(), nullptr);
    onPathBBs.clear();
    onPathBBs.resize(cgFunc->NumBBs(), false);

    FOR_ALL_BB(bb, cgFunc)
    {
        bb->SetLevel(0);
    }
    bool changed;
    do {
        changed = false;
        FOR_ALL_BB(bb, cgFunc)
        {
            if (!visitedBBs[bb->GetId()]) {
                dfsBBs.push(bb);
                seekCycles();
                changed = true;
            }
        }
    } while (changed);

    markExtraEntryAndEncl();
    /*
     * FIX : Should merge the partial loops at the time of initial
     * construction.  And make the linked list as a sorted set,
     * then the merge and sort phases below can go away.
     *
     * Start merging the loops with the same header
     */
    MergeLoops();
    /* order loops from least number of members */
    SortLoops();
    DetectInnerLoop();
    UpdateCGFunc();
}

bool CgLoopAnalysis::PhaseRun(maplebe::CGFunc &f)
{
    f.ClearLoopInfo();
    MemPool *loopMemPool = GetPhaseMemPool();
    LoopFinder *loopFinder = loopMemPool->New<LoopFinder>(f, *loopMemPool);
    loopFinder->FormLoopHierarchy();

    if (LOOP_ANALYSIS_DUMP_NEWPM) {
        /* do dot gen after detection so the loop backedge can be properly colored using the loop info */
        DotGenerator::GenerateDot("buildloop", f, f.GetMirModule(), true, f.GetName());
    }
#if DEBUG
    for (const auto *lp : f.GetLoops()) {
        lp->CheckLoops();
    }
#endif
    return false;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgLoopAnalysis, loopanalysis)
} /* namespace maplebe */
