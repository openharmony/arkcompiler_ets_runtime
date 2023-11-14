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

#ifndef MAPLE_ME_INCLUDE_DOMINANCE_H
#define MAPLE_ME_INCLUDE_DOMINANCE_H
#include "bb.h"

namespace maple {
class Dominance : public AnalysisResult {
public:
    Dominance(MemPool &memPool, MemPool &tmpPool, MapleVector<BB *> &bbVec, BB &commonEntryBB, BB &commonExitBB)
        : AnalysisResult(&memPool),
          domAllocator(&memPool),
          tmpAllocator(&tmpPool),
          bbVec(bbVec),
          commonEntryBB(commonEntryBB),
          commonExitBB(commonExitBB),
          postOrderIDVec(bbVec.size(), -1, domAllocator.Adapter()),
          reversePostOrder(domAllocator.Adapter()),
          reversePostOrderId(domAllocator.Adapter()),
          doms(domAllocator.Adapter()),
          pdomPostOrderIDVec(bbVec.size(), -1, domAllocator.Adapter()),
          pdomReversePostOrder(domAllocator.Adapter()),
          pdoms(domAllocator.Adapter()),
          domFrontier(bbVec.size(), MapleSet<BBId>(domAllocator.Adapter()), domAllocator.Adapter()),
          domChildren(bbVec.size(), MapleVector<BBId>(domAllocator.Adapter()), domAllocator.Adapter()),
          iterDomFrontier(bbVec.size(), MapleSet<BBId>(domAllocator.Adapter()), domAllocator.Adapter()),
          dtPreOrder(bbVec.size(), BBId(0), domAllocator.Adapter()),
          dtDfn(bbVec.size(), -1, domAllocator.Adapter()),
          pdomFrontier(bbVec.size(), MapleSet<BBId>(domAllocator.Adapter()), domAllocator.Adapter()),
          pdomChildren(bbVec.size(), MapleSet<BBId>(domAllocator.Adapter()), domAllocator.Adapter()),
          iterPdomFrontier(bbVec.size(), MapleSet<BBId>(domAllocator.Adapter()), domAllocator.Adapter()),
          pdtPreOrder(bbVec.size(), BBId(0), domAllocator.Adapter()),
          pdtDfn(bbVec.size(), -1, domAllocator.Adapter())
    {
    }

    ~Dominance() override = default;

    void GenPostOrderID();
    void ComputeDominance();
    void ComputeDomFrontiers();
    void ComputeDomChildren();
    void GetIterDomFrontier(const BB *bb, MapleSet<BBId> *dfset, BBId bbidMarker, std::vector<bool> &visitedMap);
    void ComputeIterDomFrontiers();
    void ComputeDtPreorder(const BB &bb, size_t &num);
    void ComputeDtDfn();
    bool Dominate(const BB &bb1, const BB &bb2);  // true if bb1 dominates bb2
    void DumpDoms();
    void PdomGenPostOrderID();
    void ComputePostDominance();
    void ComputePdomFrontiers();
    void ComputePdomChildren();
    void GetIterPdomFrontier(const BB *bb, MapleSet<BBId> *dfset, BBId bbidMarker, std::vector<bool> &visitedMap);
    void ComputeIterPdomFrontiers();
    void ComputePdtPreorder(const BB &bb, size_t &num);
    void ComputePdtDfn();
    bool PostDominate(const BB &bb1, const BB &bb2);  // true if bb1 postdominates bb2
    void DumpPdoms();

    const MapleVector<BB *> &GetBBVec() const
    {
        return bbVec;
    }

    bool IsBBVecEmpty() const
    {
        return bbVec.empty();
    }

    size_t GetBBVecSize() const
    {
        return bbVec.size();
    }

    BB *GetBBAt(size_t i) const
    {
        return bbVec[i];
    }

    BB &GetCommonEntryBB() const
    {
        return commonEntryBB;
    }

    BB &GetCommonExitBB() const
    {
        return commonExitBB;
    }

    const MapleVector<int32> &GetPostOrderIDVec() const
    {
        return postOrderIDVec;
    }

    MapleVector<BB *> &GetReversePostOrder()
    {
        return reversePostOrder;
    }

    MapleVector<uint32> &GetReversePostOrderId()
    {
        if (reversePostOrderId.empty()) {
            reversePostOrderId.resize(bbVec.size(), 0);
            for (size_t id = 0; id < reversePostOrder.size(); ++id) {
                reversePostOrderId[reversePostOrder[id]->GetBBId()] = static_cast<uint32>(id);
            }
        }
        return reversePostOrderId;
    }

    const MapleUnorderedMap<BBId, BB *> &GetDoms() const
    {
        return doms;
    }

    MapleVector<BBId> &GetDtPreOrder()
    {
        return dtPreOrder;
    }

    BBId GetDtPreOrderItem(size_t idx) const
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

    MapleSet<BBId> &GetPdomFrontierItem(size_t idx)
    {
        return pdomFrontier[idx];
    }

    size_t GetPdomFrontierSize() const
    {
        return pdomFrontier.size();
    }

    MapleSet<BBId> &GetPdomChildrenItem(size_t idx)
    {
        return pdomChildren[idx];
    }

    void ResizePdtPreOrder(size_t n)
    {
        pdtPreOrder.resize(n);
    }

    BBId GetPdtPreOrderItem(size_t idx) const
    {
        return pdtPreOrder[idx];
    }

    uint32 GetPdtDfnItem(size_t idx) const
    {
        return pdtDfn[idx];
    }

    BB *GetDom(BBId id)
    {
        return doms[id];
    }

    size_t GetDomsSize() const
    {
        return doms.size();
    }

    int32 GetPdomPostOrderIDVec(size_t idx) const
    {
        return pdomPostOrderIDVec[idx];
    }

    BB *GetPdomReversePostOrder(size_t idx)
    {
        return pdomReversePostOrder[idx];
    }

    MapleVector<BB *> &GetPdomReversePostOrder()
    {
        return pdomReversePostOrder;
    }

    size_t GetPdomReversePostOrderSize() const
    {
        return pdomReversePostOrder.size();
    }

    MapleSet<BBId> &GetDomFrontier(size_t idx)
    {
        return domFrontier[idx];
    }

    size_t GetDomFrontierSize() const
    {
        return domFrontier.size();
    }

    MapleVector<MapleVector<BBId>> &GetDomChildren()
    {
        return domChildren;
    }

    MapleVector<BBId> &GetDomChildren(size_t idx)
    {
        return domChildren[idx];
    }

    size_t GetDomChildrenSize() const
    {
        return domChildren.size();
    }

    BB *GetPdom(BBId id)
    {
        return pdoms[id];
    }

protected:
    void PostOrderWalk(const BB &bb, int32 &pid, std::vector<bool> &visitedMap);
    BB *Intersect(BB &bb1, const BB &bb2) const;
    bool CommonEntryBBIsPred(const BB &bb) const;
    void PdomPostOrderWalk(const BB &bb, int32 &pid, std::vector<bool> &visitedMap);
    BB *PdomIntersect(BB &bb1, const BB &bb2);

    MapleAllocator domAllocator;  // stores the analysis results

private:
    MapleAllocator tmpAllocator;  // can be freed after dominator computation
    MapleVector<BB *> &bbVec;
    BB &commonEntryBB;
    BB &commonExitBB;
    MapleVector<int32> postOrderIDVec;       // index is bb id
    MapleVector<BB *> reversePostOrder;      // an ordering of the BB in reverse postorder
    MapleVector<uint32> reversePostOrderId;  // gives position of each BB in reversePostOrder
    MapleUnorderedMap<BBId, BB *> doms;      // index is bb id; immediate dominator for each BB
    // following is for post-dominance
    MapleVector<int32> pdomPostOrderIDVec;    // index is bb id
    MapleVector<BB *> pdomReversePostOrder;   // an ordering of the BB in reverse postorder
    MapleUnorderedMap<BBId, BB *> pdoms;      // index is bb id; immediate dominator for each BB
    MapleVector<MapleSet<BBId>> domFrontier;  // index is bb id
public:
    MapleVector<MapleVector<BBId>> domChildren;   // index is bb id; for dom tree
    MapleVector<MapleSet<BBId>> iterDomFrontier;  // index is bb id
private:
    MapleVector<BBId> dtPreOrder;              // ordering of the BBs in a preorder traversal of the dominator tree
    MapleVector<uint32> dtDfn;                 // gives position of each BB in dt_preorder
    MapleVector<MapleSet<BBId>> pdomFrontier;  // index is bb id
public:
    MapleVector<MapleSet<BBId>> pdomChildren;      // index is bb id; for pdom tree
    MapleVector<MapleSet<BBId>> iterPdomFrontier;  // index is bb id
private:
    MapleVector<BBId> pdtPreOrder;  // ordering of the BBs in a preorder traversal of the post-dominator tree
    MapleVector<uint32> pdtDfn;     // gives position of each BB in pdt_preorder
};
}  // namespace maple
#endif  // MAPLE_ME_INCLUDE_DOMINANCE_H
