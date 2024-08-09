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

#ifndef MAPLEBE_INCLUDE_CG_DOM_H
#define MAPLEBE_INCLUDE_CG_DOM_H

#include "cg_phase.h"
#include "insn.h"
#include "cgbb.h"
#include "datainfo.h"
#include "maple_phase.h"

namespace maplebe {
class DominanceBase : public AnalysisResult {
public:
    DominanceBase(CGFunc &func, MemPool &memPool, MemPool &tmpPool, MapleVector<BB *> &bbVec, BB &commonEntryBB,
                  BB &commonExitBB)
        : AnalysisResult(&memPool),
          domAllocator(&memPool),
          tmpAllocator(&tmpPool),
          bbVec(bbVec),
          cgFunc(func),
          commonEntryBB(commonEntryBB),
          commonExitBB(commonExitBB)
    {
    }

    ~DominanceBase() override = default;

    BB &GetCommonEntryBB() const
    {
        return commonEntryBB;
    }

    BB &GetCommonExitBB() const
    {
        return commonExitBB;
    }

protected:
    bool CommonEntryBBIsPred(const BB &bb) const;
    MapleAllocator domAllocator;  // stores the analysis results
    MapleAllocator tmpAllocator;  // can be freed after dominator computation
    MapleVector<BB *> &bbVec;
    CGFunc &cgFunc;
    BB &commonEntryBB;
    BB &commonExitBB;
};

class DomAnalysis : public DominanceBase {
public:
    DomAnalysis(CGFunc &func, MemPool &memPool, MemPool &tmpPool, MapleVector<BB *> &bbVec, BB &commonEntryBB,
                BB &commonExitBB)
        : DominanceBase(func, memPool, tmpPool, bbVec, commonEntryBB, commonExitBB),
          postOrderIDVec(bbVec.size() + 1, -1, tmpAllocator.Adapter()),
          reversePostOrder(tmpAllocator.Adapter()),
          doms(bbVec.size() + 1, nullptr, domAllocator.Adapter()),
          domFrontier(bbVec.size() + 1, MapleVector<uint32>(domAllocator.Adapter()), domAllocator.Adapter()),
          domChildren(bbVec.size() + 1, MapleVector<uint32>(domAllocator.Adapter()), domAllocator.Adapter()),
          iterDomFrontier(bbVec.size() + 1, MapleUnorderedSet<uint32>(domAllocator.Adapter()), domAllocator.Adapter()),
          dtPreOrder(bbVec.size() + 1, 0, domAllocator.Adapter()),
          dtDfn(bbVec.size() + 1, -1, domAllocator.Adapter()),
          dtDfnOut(bbVec.size() + 1, -1, domAllocator.Adapter())
    {
    }
    ~DomAnalysis() override = default;

    void Compute();
    void Dump();

    void GenPostOrderID();
    void ComputeDominance();
    void ComputeDomFrontiers();
    void ComputeDomChildren();
    void GetIterDomFrontier(const BB *bb, MapleUnorderedSet<uint32> *dfset, uint32 bbidMarker,
                            std::vector<bool> &visitedMap);
    void ComputeIterDomFrontiers();
    uint32 ComputeDtPreorder(const BB &bb, uint32 &num);
    bool Dominate(const BB &bb1, const BB &bb2);  // true if bb1 dominates bb2

    MapleVector<BB *> &GetReversePostOrder()
    {
        return reversePostOrder;
    }

    MapleVector<uint32> &GetDtPreOrder()
    {
        return dtPreOrder;
    }

    uint32 GetDtPreOrderItem(size_t idx) const
    {
        return dtPreOrder[idx];
    }

    size_t GetDtPreOrderSize() const
    {
        return dtPreOrder.size();
    }

    uint32 GetDtDfnItem(size_t idx) const
    {
        return dtDfn[idx];
    }

    size_t GetDtDfnSize() const
    {
        return dtDfn.size();
    }

    BB *GetDom(uint32 id)
    {
        DEBUG_ASSERT(id < doms.size(), "bbid out of range");
        return doms[id];
    }
    void SetDom(uint32 id, BB *bb)
    {
        DEBUG_ASSERT(id < doms.size(), "bbid out of range");
        doms[id] = bb;
    }
    size_t GetDomsSize() const
    {
        return doms.size();
    }

    auto &GetDomFrontier(size_t idx)
    {
        return domFrontier[idx];
    }
    bool HasDomFrontier(uint32 id, uint32 frontier) const
    {
        return std::find(domFrontier[id].begin(), domFrontier[id].end(), frontier) != domFrontier[id].end();
    }

    size_t GetDomFrontierSize() const
    {
        return domFrontier.size();
    }

    auto &GetDomChildren()
    {
        return domChildren;
    }

    auto &GetDomChildren(size_t idx)
    {
        return domChildren[idx];
    }

    auto &GetIdomFrontier(uint32 idx)
    {
        return iterDomFrontier[idx];
    }

    size_t GetDomChildrenSize() const
    {
        return domChildren.size();
    }

private:
    void PostOrderWalk(const BB &bb, int32 &pid, MapleVector<bool> &visitedMap);
    BB *Intersect(BB &bb1, const BB &bb2);

    MapleVector<int32> postOrderIDVec;             // index is bb id
    MapleVector<BB *> reversePostOrder;            // an ordering of the BB in reverse postorder
    MapleVector<BB *> doms;                        // index is bb id; immediate dominator for each BB
    MapleVector<MapleVector<uint32>> domFrontier;  // index is bb id
    MapleVector<MapleVector<uint32>> domChildren;  // index is bb id; for dom tree
    MapleVector<MapleUnorderedSet<uint32>> iterDomFrontier;
    MapleVector<uint32> dtPreOrder;  // ordering of the BBs in a preorder traversal of the dominator tree
    MapleVector<uint32> dtDfn;       // gives position of each BB in dt_preorder
    MapleVector<uint32> dtDfnOut;    // max position of all nodes in the sub tree of each BB in dt_preorder
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgDomAnalysis, maplebe::CGFunc);
DomAnalysis *GetResult()
{
    return domAnalysis;
}
DomAnalysis *domAnalysis = nullptr;
MAPLE_FUNC_PHASE_DECLARE_END
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_DOM_H */
