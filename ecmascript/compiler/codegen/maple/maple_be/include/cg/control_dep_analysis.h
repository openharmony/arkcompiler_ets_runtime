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
#ifndef MAPLEBE_INCLUDE_CG_PDG_ANALYSIS_H
#define MAPLEBE_INCLUDE_CG_PDG_ANALYSIS_H

#include <utility>
#include "me_dominance.h"
#include "cfg_mst.h"
#include "instrument.h"
#include "cg_cdg.h"
#include "cg_dominance.h"
#include "data_dep_base.h"
#include "loop.h"

namespace maplebe {
#define CONTROL_DEP_ANALYSIS_DUMP CG_DEBUG_FUNC(cgFunc)
// Analyze Control Dependence
class ControlDepAnalysis {
public:
    ControlDepAnalysis(CGFunc &func, MemPool &memPool, MemPool &tmpPool, DomAnalysis &d, PostDomAnalysis &pd,
                       LoopAnalysis &lp, CFGMST<BBEdge<maplebe::BB>, maplebe::BB> *cfgmst, std::string pName = "")
        : cgFunc(func),
          dom(&d),
          pdom(&pd),
          loop(&lp),
          cfgMST(cfgmst),
          cdgMemPool(memPool),
          cdgAlloc(&memPool),
          tmpAlloc(&tmpPool),
          nonPdomEdges(tmpAlloc.Adapter()),
          curCondNumOfBB(tmpAlloc.Adapter()),
          phaseName(std::move(pName))
    {
    }
    ControlDepAnalysis(CGFunc &func, MemPool &memPool, std::string pName = "", bool isSingle = true)
        : cgFunc(func),
          cdgMemPool(memPool),
          cdgAlloc(&memPool),
          tmpAlloc(&memPool),
          nonPdomEdges(cdgAlloc.Adapter()),
          curCondNumOfBB(cdgAlloc.Adapter()),
          phaseName(std::move(pName)),
          isSingleBB(isSingle)
    {
    }
    virtual ~ControlDepAnalysis()
    {
        dom = nullptr;
        fcdg = nullptr;
        cfgMST = nullptr;
        pdom = nullptr;
        loop = nullptr;
    }

    std::string PhaseName() const
    {
        if (phaseName.empty()) {
            return "controldepanalysis";
        } else {
            return phaseName;
        }
    }
    void SetIsSingleBB(bool isSingle)
    {
        isSingleBB = isSingle;
    }

    // The entry of analysis
    void Run();

    // Provide scheduling-related interfaces
    void ComputeSingleBBRegions();  // For local-scheduling in a single BB
    void GetEquivalentNodesInRegion(CDGRegion &region, CDGNode &cdgNode, std::vector<CDGNode *> &equivalentNodes);

    // Interface for obtaining PDGAnalysis infos
    FCDG *GetFCDG()
    {
        return fcdg;
    }
    CFGMST<BBEdge<maplebe::BB>, maplebe::BB> *GetCFGMst()
    {
        return cfgMST;
    }

    // Print forward-control-dependence-graph in dot syntax
    void GenerateFCDGDot() const;
    // Print control-flow-graph with condition at edges in dot syntax
    void GenerateCFGDot() const;
    // Print control-flow-graph with only bbId
    void GenerateSimplifiedCFGDot() const;
    // Print control-flow-graph of the region in dot syntax
    void GenerateCFGInRegionDot(CDGRegion &region) const;

protected:
    void BuildCFGInfo();
    void ConstructFCDG();
    void ComputeRegions(bool doCDRegion);
    void ComputeGeneralNonLinearRegions();
    void FindInnermostLoops(std::vector<LoopDesc *> &innermostLoops, std::unordered_map<LoopDesc *, bool> &visited,
                            LoopDesc *lp);
    void FindFallthroughPath(std::vector<CDGNode *> &regionMembers, BB *curBB, bool isRoot);
    void CreateRegionForSingleBB();
    bool AddRegionNodesInTopologicalOrder(CDGRegion &region, CDGNode &root, const MapleSet<BBID> &members);
    void ComputeSameCDRegions(bool considerNonDep);
    void ComputeRegionForCurNode(uint32 curBBId, std::vector<bool> &visited);
    void CreateAndDivideRegion(uint32 pBBId);
    void ComputeRegionForNonDepNodes();
    CDGRegion *FindExistRegion(CDGNode &node) const;
    bool IsISEqualToCDs(CDGNode &parent, CDGNode &child) const;
    void MergeRegions(CDGNode &mergeNode, CDGNode &candiNode);

    CDGEdge *BuildControlDependence(const BB &fromBB, const BB &toBB, int32 condition);
    CDGRegion *CreateFCDGRegion(CDGNode &curNode);
    void CreateAllCDGNodes();
    Dominance *ComputePdomInRegion(CDGRegion &region, std::vector<BB *> &nonUniformRegionCFG, uint32 &maxBBId);
    bool IsInDifferentSCCNode(CDGRegion &region, std::vector<BB *> &regionCFG, uint32 maxBBId, const BB &curBB,
                              const BB &memberBB);

    void AddNonPdomEdges(BBEdge<maplebe::BB> *bbEdge)
    {
        nonPdomEdges.emplace_back(bbEdge);
    }

    uint32 GetAndAccSuccedCondNum(uint32 bbId)
    {
        auto pair = curCondNumOfBB.try_emplace(bbId, 0);
        if (pair.second) {
            return 0;
        } else {
            uint32 curNum = pair.first->second;
            pair.first->second = curNum + 1;
            return curNum;
        }
    }

    static bool IsSameControlDependence(const CDGEdge &edge1, const CDGEdge &edge2)
    {
        CDGNode &fromNode1 = edge1.GetFromNode();
        CDGNode &fromNode2 = edge2.GetFromNode();
        if (fromNode1.GetNodeId() != fromNode2.GetNodeId()) {
            return false;
        }
        if (edge1.GetCondition() != edge2.GetCondition()) {
            return false;
        }
        return true;
    }

    CGFunc &cgFunc;
    DomAnalysis *dom = nullptr;
    PostDomAnalysis *pdom = nullptr;
    LoopAnalysis *loop = nullptr;
    CFGMST<BBEdge<maplebe::BB>, maplebe::BB> *cfgMST = nullptr;
    MemPool &cdgMemPool;
    MapleAllocator cdgAlloc;
    MapleAllocator tmpAlloc;
    MapleVector<BBEdge<maplebe::BB> *> nonPdomEdges;
    MapleUnorderedMap<uint32, uint32> curCondNumOfBB;  // <BBId, assigned condNum>
    FCDG *fcdg = nullptr;
    uint32 lastRegionId = 0;
    std::string phaseName;
    bool isSingleBB = false;
};

MAPLE_FUNC_PHASE_DECLARE_BEGIN(CgControlDepAnalysis, maplebe::CGFunc);
ControlDepAnalysis *GetResult()
{
    return cda;
}
ControlDepAnalysis *cda = nullptr;
OVERRIDE_DEPENDENCE
MAPLE_FUNC_PHASE_DECLARE_END
}  // namespace maplebe

#endif  // MAPLEBE_INCLUDE_CG_PDG_ANALYSIS_H
