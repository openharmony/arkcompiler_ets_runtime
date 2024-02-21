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

#ifndef MAPLEBE_INCLUDE_CG_LOOP_H
#define MAPLEBE_INCLUDE_CG_LOOP_H

#include "cgfunc.h"
#include "cg_phase.h"
#include "cg_dominance.h"
#include "cgbb.h"
#include "insn.h"
#include "maple_phase.h"

namespace maplebe {
class LoopDesc {
public:
    struct LoopDescCmp {
        bool operator()(const LoopDesc *loop1, const LoopDesc *loop2) const
        {
            CHECK_NULL_FATAL(loop1);
            CHECK_NULL_FATAL(loop2);
            return (loop1->GetHeader().GetId() < loop2->GetHeader().GetId());
        }
    };

    LoopDesc(MapleAllocator &allocator, BB &head)
        : alloc(allocator),
          header(head),
          loopBBs(alloc.Adapter()),
          exitBBs(alloc.Adapter()),
          backEdges(alloc.Adapter()),
          childLoops(alloc.Adapter())
    {
    }

    ~LoopDesc() = default;

    void Dump() const;

    BB &GetHeader()
    {
        return header;
    }

    const BB &GetHeader() const
    {
        return header;
    }

    // get all bbIds in the loop
    const MapleSet<BBID> &GetLoopBBs() const
    {
        return loopBBs;
    }

    // check whether the BB exists in the current loop
    bool Has(const BB &bb) const
    {
        return loopBBs.find(bb.GetId()) != loopBBs.end();
    }

    void InsertLoopBBs(const BB &bb)
    {
        (void)loopBBs.insert(bb.GetId());
    }

    const MapleSet<BBID> &GetExitBBs() const
    {
        return exitBBs;
    }

    void InsertExitBBs(const BB &bb)
    {
        (void)exitBBs.insert(bb.GetId());
    }

    void InsertBackEdges(const BB &bb)
    {
        (void)backEdges.insert(bb.GetId());
    }

    // check whether from->to is the back edge of the current loop
    bool IsBackEdge(const BB &from, const BB &to) const
    {
        return (to.GetId() == header.GetId()) && (backEdges.find(from.GetId()) != backEdges.end());
    }

    // get the BBId of all back edges
    const MapleSet<BBID> &GetBackEdges() const
    {
        return backEdges;
    }

    const LoopDesc *GetParentLoop() const
    {
        return parentLoop;
    }

    LoopDesc *GetParentLoop()
    {
        return parentLoop;
    }

    void SetParentLoop(LoopDesc &loop)
    {
        parentLoop = &loop;
    }

    uint32 GetNestDepth() const
    {
        return nestDepth;
    }

    void SetNestDepth(uint32 depth)
    {
        nestDepth = depth;
    }

    const MapleSet<LoopDesc *, LoopDescCmp> &GetChildLoops() const
    {
        return childLoops;
    }

    void InsertChildLoops(LoopDesc &loop)
    {
        (void)childLoops.insert(&loop);
    }

private:
    MapleAllocator &alloc;
    BB &header;                      // BB's header
    MapleSet<BBID> loopBBs;          // BBs in loop
    MapleSet<BBID> exitBBs;          // loop exit BBs
    MapleSet<BBID> backEdges;        // loop back edges, backBB -> headBB
    LoopDesc *parentLoop = nullptr;  // points to its closest nesting loop
    uint32 nestDepth = 1;            // the nesting depth
    MapleSet<LoopDesc *, LoopDescCmp> childLoops;
};
class LoopHierarchy {
public:
    struct HeadIDCmp {
        bool operator()(const LoopHierarchy *loopHierarchy1, const LoopHierarchy *loopHierarchy2) const
        {
            CHECK_NULL_FATAL(loopHierarchy1);
            CHECK_NULL_FATAL(loopHierarchy2);
            return (loopHierarchy1->GetHeader()->GetId() < loopHierarchy2->GetHeader()->GetId());
        }
    };

    explicit LoopHierarchy(MemPool &memPool)
        : loopMemPool(&memPool),
          otherLoopEntries(loopMemPool.Adapter()),
          loopMembers(loopMemPool.Adapter()),
          backedge(loopMemPool.Adapter()),
          exits(loopMemPool.Adapter()),
          innerLoops(loopMemPool.Adapter())
    {
    }

    virtual ~LoopHierarchy() = default;

    BB *GetHeader() const
    {
        return header;
    }
    const MapleSet<BB *, BBIdCmp> &GetLoopMembers() const
    {
        return loopMembers;
    }
    const MapleSet<BB *, BBIdCmp> &GetBackedge() const
    {
        return backedge;
    }
    MapleSet<BB *, BBIdCmp> &GetBackedgeNonConst()
    {
        return backedge;
    }
    const MapleSet<BB *, BBIdCmp> &GetExits() const
    {
        return exits;
    }
    const MapleSet<LoopHierarchy *, HeadIDCmp> &GetInnerLoops() const
    {
        return innerLoops;
    }
    const LoopHierarchy *GetOuterLoop() const
    {
        return outerLoop;
    }
    LoopHierarchy *GetPrev()
    {
        return prev;
    }
    LoopHierarchy *GetNext()
    {
        return next;
    }

    MapleSet<BB *, BBIdCmp>::iterator EraseLoopMembers(MapleSet<BB *, BBIdCmp>::iterator it)
    {
        return loopMembers.erase(it);
    }
    void InsertLoopMembers(BB &bb)
    {
        (void)loopMembers.insert(&bb);
    }
    void InsertBackedge(BB &bb)
    {
        (void)backedge.insert(&bb);
    }
    void InsertExit(BB &bb)
    {
        (void)exits.insert(&bb);
    }
    void InsertInnerLoops(LoopHierarchy &loop)
    {
        (void)innerLoops.insert(&loop);
    }
    void SetHeader(BB &bb)
    {
        header = &bb;
    }
    void SetOuterLoop(LoopHierarchy &loop)
    {
        outerLoop = &loop;
    }
    void SetPrev(LoopHierarchy *loop)
    {
        prev = loop;
    }
    void SetNext(LoopHierarchy *loop)
    {
        next = loop;
    }
    void PrintLoops(const std::string &name) const;

protected:
    LoopHierarchy *prev = nullptr;
    LoopHierarchy *next = nullptr;

private:
    MapleAllocator loopMemPool;
    BB *header = nullptr;

public:
    MapleSet<BB *, BBIdCmp> otherLoopEntries;
    MapleSet<BB *, BBIdCmp> loopMembers;
    MapleSet<BB *, BBIdCmp> backedge;
    MapleSet<BB *, BBIdCmp> exits;
    MapleSet<LoopHierarchy *, HeadIDCmp> innerLoops;
    LoopHierarchy *outerLoop = nullptr;
};

class LoopFinder : public AnalysisResult {
public:
    LoopFinder(CGFunc &func, MemPool &mem)
        : AnalysisResult(&mem),
          cgFunc(&func),
          memPool(&mem),
          loopMemPool(memPool),
          visitedBBs(loopMemPool.Adapter()),
          sortedBBs(loopMemPool.Adapter()),
          dfsBBs(loopMemPool.Adapter()),
          onPathBBs(loopMemPool.Adapter()),
          recurseVisited(loopMemPool.Adapter())
    {
    }

    ~LoopFinder() override = default;

    void formLoop(BB *headBB, BB *backBB);
    void seekBackEdge(BB *bb, MapleList<BB *> succs);
    void seekCycles();
    void markExtraEntryAndEncl();
    bool HasSameHeader(const LoopHierarchy *lp1, const LoopHierarchy *lp2);
    void MergeLoops();
    void SortLoops();
    void UpdateOuterForInnerLoop(BB *bb, LoopHierarchy *outer);
    void UpdateOuterLoop(const LoopHierarchy *loop);
    void CreateInnerLoop(LoopHierarchy &inner, LoopHierarchy &outer);
    void DetectInnerLoop();
    void UpdateCGFunc();
    void FormLoopHierarchy();

private:
    CGFunc *cgFunc;
    MemPool *memPool;
    MapleAllocator loopMemPool;
    MapleVector<bool> visitedBBs;
    MapleVector<BB *> sortedBBs;
    MapleStack<BB *> dfsBBs;
    MapleVector<bool> onPathBBs;
    MapleVector<bool> recurseVisited;
    LoopHierarchy *loops = nullptr;
};

class CGFuncLoops {
public:
    explicit CGFuncLoops(MemPool &memPool)
        : loopMemPool(&memPool),
          multiEntries(loopMemPool.Adapter()),
          loopMembers(loopMemPool.Adapter()),
          backedge(loopMemPool.Adapter()),
          exits(loopMemPool.Adapter()),
          innerLoops(loopMemPool.Adapter())
    {
    }

    ~CGFuncLoops() = default;

    void CheckOverlappingInnerLoops(const MapleVector<CGFuncLoops *> &iLoops, const MapleVector<BB *> &loopMem) const;
    void CheckLoops() const;
    void PrintLoops(const CGFuncLoops &funcLoop) const;
    bool IsBBLoopMember(const BB *bb);

    const BB *GetHeader() const
    {
        return header;
    }
    const MapleVector<BB *> &GetMultiEntries() const
    {
        return multiEntries;
    }
    const MapleVector<BB *> &GetLoopMembers() const
    {
        return loopMembers;
    }
    const MapleVector<BB *> &GetBackedge() const
    {
        return backedge;
    }
    const MapleVector<BB *> &GetExits() const
    {
        return exits;
    }
    const MapleVector<CGFuncLoops *> &GetInnerLoops() const
    {
        return innerLoops;
    }
    const CGFuncLoops *GetOuterLoop() const
    {
        return outerLoop;
    }
    uint32 GetLoopLevel() const
    {
        return loopLevel;
    }

    void AddMultiEntries(BB &bb)
    {
        multiEntries.emplace_back(&bb);
    }
    void AddLoopMembers(BB &bb)
    {
        loopMembers.emplace_back(&bb);
    }
    void AddBackedge(BB &bb)
    {
        backedge.emplace_back(&bb);
    }
    void AddExit(BB &bb)
    {
        exits.emplace_back(&bb);
    }
    void AddInnerLoops(CGFuncLoops &loop)
    {
        innerLoops.emplace_back(&loop);
    }
    void SetHeader(BB &bb)
    {
        header = &bb;
    }
    void SetOuterLoop(CGFuncLoops &loop)
    {
        outerLoop = &loop;
    }
    void SetLoopLevel(uint32 val)
    {
        loopLevel = val;
    }

private:
    MapleAllocator loopMemPool;
    BB *header = nullptr;
    MapleVector<BB *> multiEntries;
    MapleVector<BB *> loopMembers;
    MapleVector<BB *> backedge;
    MapleVector<BB *> exits;
    MapleVector<CGFuncLoops *> innerLoops;
    CGFuncLoops *outerLoop = nullptr;
    uint32 loopLevel = 0;
};

struct CGFuncLoopCmp {
    bool operator()(const CGFuncLoops *lhs, const CGFuncLoops *rhs) const
    {
        CHECK_NULL_FATAL(lhs);
        CHECK_NULL_FATAL(rhs);
        return lhs->GetHeader()->GetId() < rhs->GetHeader()->GetId();
    }
};

class LoopAnalysis : public AnalysisResult {
public:
    LoopAnalysis(CGFunc &func, MemPool &memPool, DomAnalysis &domInfo)
        : AnalysisResult(&memPool),
          alloc(&memPool),
          cgFunc(func),
          dom(domInfo),
          loops(alloc.Adapter()),
          bbLoopParent(cgFunc.GetAllBBSize(), nullptr, alloc.Adapter())
    {
    }

    ~LoopAnalysis() override = default;

    const MapleVector<LoopDesc *> &GetLoops() const
    {
        return loops;
    }

    MapleVector<LoopDesc *> &GetLoops()
    {
        return loops;
    }

    // get the loop to which the BB belong, null -> not in loop.
    LoopDesc *GetBBLoopParent(BBID bbID) const
    {
        if (bbID >= bbLoopParent.size()) {
            return nullptr;
        }
        return bbLoopParent[bbID];
    }

    // check whether from->to is the back edge
    bool IsBackEdge(const BB &from, const BB &to) const
    {
        auto *loop = GetBBLoopParent(to.GetId());
        if (loop && loop->IsBackEdge(from, to)) {
            return true;
        }
        loop = GetBBLoopParent(from.GetId());
        return loop && loop->IsBackEdge(from, to);
    }

    void Analysis();

    void Dump() const
    {
        LogInfo::MapleLogger() << "Dump LoopAnalysis Result For Func " << cgFunc.GetName() << ":\n";
        for (const auto *loop : loops) {
            loop->Dump();
        }
        for (BBID bbId = 0; bbId < bbLoopParent.size(); ++bbId) {
            if (bbLoopParent[bbId] == nullptr) {
                continue;
            }
            LogInfo::MapleLogger() << "BB " << bbId << " in loop " << bbLoopParent[bbId]->GetHeader().GetId() << "\n";
        }
    }

    bool IsLoopHeaderBB(const BB &bb) const
    {
        if (GetBBLoopParent(bb.GetId()) == nullptr) {
            return false;
        } else if (GetBBLoopParent(bb.GetId())->GetHeader().GetId() == bb.GetId()) {
            return true;
        }
        return false;
    }

private:
    MapleAllocator alloc;
    CGFunc &cgFunc;
    DomAnalysis &dom;
    MapleVector<LoopDesc *> loops;         // all loops in func
    MapleVector<LoopDesc *> bbLoopParent;  // gives closest nesting loop for each bb

    LoopDesc *GetOrCreateLoopDesc(BB &headBB);
    void SetLoopParent4BB(const BB &bb, LoopDesc &loopDesc);
    void SetExitBBs(LoopDesc &loop) const;
    void ProcessBB(BB &bb);
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgLoopAnalysis, maplebe::CGFunc);
LoopAnalysis *GetResult()
{
    return loop;
}
LoopAnalysis *loop = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_LOOP_H */
