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
#include "cg_dominance.h"
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

    const MapleSet<LoopDesc*, LoopDescCmp> &GetChildLoops() const
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
    MapleSet<LoopDesc*, LoopDescCmp> childLoops;
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

    const MapleVector<LoopDesc*> &GetLoops() const
    {
        return loops;
    }

    MapleVector<LoopDesc*> &GetLoops()
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
    MapleVector<LoopDesc*> loops;         // all loops in func
    MapleVector<LoopDesc*> bbLoopParent;  // gives closest nesting loop for each bb

    LoopDesc *GetOrCreateLoopDesc(BB &headBB);
    void SetLoopParent4BB(const BB &bb, LoopDesc &loopDesc);
    void SetExitBBs(LoopDesc &loop) const;
    void GenerateLoop(BB *bb);
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
