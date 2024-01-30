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

#include "control_dep_analysis.h"
#include "mpl_logging.h"

namespace maplebe {
static BB *GetExistBB(std::vector<BB *> &bbVec, BBID bbId)
{
    for (auto *bb : bbVec) {
        if (bb->GetId() == bbId) {
            return bb;
        }
    }
    return nullptr;
}

void ControlDepAnalysis::Run()
{
    // Local-scheduler(after RA) does not need pdom-analysis
    if (CONTROL_DEP_ANALYSIS_DUMP && phaseName != "localschedule") {
        pdom->GeneratePdomTreeDot();
    }
    if (cgFunc.IsAfterRegAlloc() || isSingleBB) {
        // For local scheduling
        ComputeSingleBBRegions();
    } else {
        // For global scheduling based on regions
        BuildCFGInfo();
        ConstructFCDG();
        ComputeRegions(false);
    }
}

// Augment CFG info
void ControlDepAnalysis::BuildCFGInfo()
{
    CHECK_FATAL(cgFunc.GetCommonExitBB() != nullptr, "there must be a virtual ExitBB in cfg");
    cfgMST->BuildEdges(cgFunc.GetFirstBB(), cgFunc.GetCommonExitBB());
    // denote back-edge on CFGEdge
    for (auto cfgEdge : cfgMST->GetAllEdges()) {
        BB *srcBB = cfgEdge->GetSrcBB();
        BB *destBB = cfgEdge->GetDestBB();
        (void)*srcBB;
        (void)*destBB;
        if (loop->IsBackEdge(*srcBB, *destBB)) {
            cfgEdge->SetIsBackEdge();
        }
    }
    // denote the condition on CFGEdge except for back-edge
    for (auto &cfgEdge : cfgMST->GetAllEdges()) {
        if (cfgEdge->IsBackEdge()) {
            continue;
        }
        BB *srcBB = cfgEdge->GetSrcBB();
        BB *destBB = cfgEdge->GetDestBB();
        CHECK_FATAL(srcBB != nullptr, "get srcBB of cfgEdge failed");
        if (srcBB == cgFunc.GetFirstBB()) {
            cfgEdge->SetCondition(0);
            continue;
        } else if (srcBB == cgFunc.GetCommonExitBB()) {
            continue;
        }
        BB::BBKind srcKind = srcBB->GetKind();
        switch (srcKind) {
            case BB::kBBFallthru:
            case BB::kBBGoto:
            case BB::kBBIgoto:
            case BB::kBBReturn:
                cfgEdge->SetCondition(0);
                break;
            case BB::kBBIntrinsic:
                ASSERT_NOT_NULL(srcBB->GetLastMachineInsn());
                if (!srcBB->GetLastMachineInsn()->IsBranch()) {
                    // set default cond number
                    cfgEdge->SetCondition(0);
                }
                // else fall through
                [[clang::fallthrough]];
            case BB::kBBIf: {
                Insn *branchInsn = srcBB->GetLastMachineInsn();
                CHECK_FATAL(branchInsn != nullptr, "ifBB must have a machine insn at the end");
                DEBUG_ASSERT(branchInsn->IsCondBranch(), "ifBB must have a conditional branch insn at the end");
                int lastOpndIdx = static_cast<int>(branchInsn->GetOperandSize()) - 1;
                DEBUG_ASSERT(lastOpndIdx > -1, "lastOpndIdx must greater than -1");
                Operand &lastOpnd = branchInsn->GetOperand(static_cast<uint32>(lastOpndIdx));
                DEBUG_ASSERT(lastOpnd.IsLabelOpnd(), "lastOpnd must be labelOpnd in branchInsn");
                BB *jumpBB = cgFunc.GetBBFromLab2BBMap(static_cast<LabelOperand &>(lastOpnd).GetLabelIndex());
                if (jumpBB == destBB) {
                    cfgEdge->SetCondition(0);
                } else {
                    cfgEdge->SetCondition(1);
                }
                break;
            }
            case BB::kBBRangeGoto: {
                // Each successor cfgEdge is assigned a different cond number
                cfgEdge->SetCondition(static_cast<int32>(GetAndAccSuccedCondNum(srcBB->GetId())));
                break;
            }
            default:
                // these kindBBs set default cond number [kBBNoReturn kBBThrow kBBLast]
                cfgEdge->SetCondition(0);
                break;
        }
    }
}

// Construct forward control dependence graph
void ControlDepAnalysis::ConstructFCDG()
{
    CreateAllCDGNodes();
    // 1. Collect all edges(A, B) in CFG that B does not post-dom A
    for (auto cfgEdge : cfgMST->GetAllEdges()) {
        if (cfgEdge->IsBackEdge()) {
            continue;
        }
        BB *srcBB = cfgEdge->GetSrcBB();
        BB *destBB = cfgEdge->GetDestBB();
        CHECK_FATAL(srcBB != nullptr && destBB != nullptr, "get edge-connected nodes in cfg failed");
        if (srcBB == cgFunc.GetCommonExitBB()) {
            continue;
        }
        if (!pdom->PostDominate(*destBB, *srcBB)) {
            AddNonPdomEdges(cfgEdge);
        }
    }

    // 2. Determine control dependence by traversal backward in the post-dom tree for every bbEdge in nonPdomEdges
    for (auto candiEdge : nonPdomEdges) {
        BB *srcBB = candiEdge->GetSrcBB();
        BB *destBB = candiEdge->GetDestBB();
        CHECK_FATAL(srcBB != nullptr && destBB != nullptr, "get edge-connected nodes in nonPdomEdges failed");
        // Find the nearest common ancestor (L) of srcBB and destBB in the pdom-tree :
        //   (1) L == parent of srcBB in the pdom-tree (immediate dominator of srcBB)
        //   (2) L == srcBB
        BB *ancestor = (pdom->GetPdom(destBB->GetId()) == srcBB) ? srcBB : pdom->GetPdom(srcBB->GetId());
        BB *curBB = destBB;
        while (curBB != nullptr && curBB != ancestor && curBB != cgFunc.GetCommonExitBB()) {
            (void)BuildControlDependence(*srcBB, *curBB, candiEdge->GetCondition());
            curBB = pdom->GetPdom(curBB->GetId());
        }
    }
}

/** Divide regions for the CDGNodes :
 *   Traverse the post-dominator tree by means of a post-order to
 *   assure that all children in the post-dominator tree are visited before their parent.
 */
void ControlDepAnalysis::ComputeRegions(bool doCDRegion)
{
    if (doCDRegion) {
        ComputeSameCDRegions(false);
    } else {
        ComputeGeneralNonLinearRegions();
    }
}

// This algorithm computes the general non-linear region, including:
//   1). A reducible loops which have a single-in edge
//   2). A fallthrough path not in any regions
//   3). A single-bb not in other regions as a region
void ControlDepAnalysis::ComputeGeneralNonLinearRegions()
{
    // If ebo phase was removed, must recalculate loop info
    // 1. Find all innermost loops
    std::vector<LoopDesc *> innermostLoops;
    std::unordered_map<LoopDesc *, bool> visited;

    for (auto lp : loop->GetLoops()) {
        FindInnermostLoops(innermostLoops, visited, lp);
    }

    // 2. Find reducible loops as a region
    for (auto innerLoop : innermostLoops) {
        // Avoid the case as following:
        //             |
        //             |
        //      ----- BB4 ----------
        //      |    /   \         |
        //      |   /     \        |
        //       BB3      BB5      |
        //               /  \      |
        //              /    \     |
        //            BB6     BB10 -
        //             |
        //             |
        //            EXIT
        // By the current loop analysis, {BB4, BB3, BB5, BB10} are in the same loop, which the headerBB is BB4
        // By the dom and pdom analysis, {BB4 dom BB5} and {BB5 pdom BB4}, BB4 and BB5 are EQUIVALENT,
        //   but they cannot schedule in parallel.
        //
        // The above case may cause loop-carried dependency instructions to be scheduled, and currently this
        // dependency is not considered.
        if (innerLoop->GetBackEdges().size() > 1) {
            continue;
        }

        bool isReducible = true;
        auto &header = innerLoop->GetHeader();
        for (auto memberId : innerLoop->GetLoopBBs()) {
            if (!dom->Dominate(header, *cgFunc.GetBBFromID(memberId))) {
                isReducible = false;
            }
        }
        if (isReducible) {
            auto *region = cdgMemPool.New<CDGRegion>(CDGRegionId(lastRegionId++), cdgAlloc);
            CDGNode *headerNode = header.GetCDGNode();
            CHECK_FATAL(headerNode != nullptr, "get cdgNode from bb failed");
            region->SetRegionRoot(*headerNode);
            region->SetBackBBId(*innerLoop->GetBackEdges().begin());
            for (auto memberId : innerLoop->GetLoopBBs()) {
                CDGNode *memberNode = cgFunc.GetBBFromID(memberId)->GetCDGNode();
                CHECK_FATAL(memberNode != nullptr, "get cdgNode from bb failed");
                memberNode->SetRegion(region);
            }
            if (AddRegionNodesInTopologicalOrder(*region, *headerNode, innerLoop->GetLoopBBs())) {
                fcdg->AddRegion(*region);
            }
        }
    }

    // 3. Find fallthrough path not in any regions as a region
    FOR_ALL_BB(bb, &cgFunc)
    {
        if (bb->IsUnreachable()) {
            continue;
        }
        std::vector<CDGNode *> regionMembers;
        CDGNode *cdgNode = bb->GetCDGNode();
        CHECK_FATAL(cdgNode != nullptr, "get cdgNode from bb failed");
        if (!cdgNode->IsVisitedInExtendedFind() && bb->GetSuccsSize() == 1 && cdgNode->GetRegion() == nullptr) {
            // Nodes in the region are in the order of topology in this way
            FindFallthroughPath(regionMembers, bb, true);
            auto *region = cdgMemPool.New<CDGRegion>(CDGRegionId(lastRegionId++), cdgAlloc);
            region->SetRegionRoot(*cdgNode);
            for (auto memberNode : regionMembers) {
                region->AddCDGNode(memberNode);
                memberNode->SetRegion(region);
            }
            fcdg->AddRegion(*region);
            regionMembers.clear();
        }
    }

    // 4. Create region for the remaining BBs that are not in any region
    CreateRegionForSingleBB();
}

void ControlDepAnalysis::FindFallthroughPath(std::vector<CDGNode *> &regionMembers, BB *curBB, bool isRoot)
{
    CHECK_FATAL(curBB != nullptr, "invalid bb");
    CDGNode *curNode = curBB->GetCDGNode();
    CHECK_FATAL(curNode != nullptr, "get cdgNode from bb failed");
    if (curNode->IsVisitedInExtendedFind()) {
        return;
    }
    curNode->SetVisitedInExtendedFind();
    if (isRoot) {
        if (curBB->GetSuccsSize() == 1 && curNode->GetRegion() == nullptr) {
            regionMembers.emplace_back(curNode);
        } else {
            return;
        }
    } else {
        if (curBB->GetPreds().size() == 1 && curBB->GetSuccsSize() == 1 && curNode->GetRegion() == nullptr) {
            regionMembers.emplace_back(curNode);
        } else {
            return;
        }
    }
    FindFallthroughPath(regionMembers, *curBB->GetSuccsBegin(), false);
}

void ControlDepAnalysis::CreateRegionForSingleBB()
{
    FOR_ALL_BB(bb, &cgFunc)
    {
        if (bb->IsUnreachable()) {
            continue;
        }
        CDGNode *cdgNode = bb->GetCDGNode();
        CHECK_FATAL(cdgNode != nullptr, "get cdgNode from bb failed");
        if (cdgNode->GetRegion() == nullptr) {
            auto *region = cdgMemPool.New<CDGRegion>(CDGRegionId(lastRegionId++), cdgAlloc);
            region->AddCDGNode(cdgNode);
            cdgNode->SetRegion(region);
            region->SetRegionRoot(*cdgNode);
            fcdg->AddRegion(*region);
        }
    }
}

// Recursive search for innermost loop
void ControlDepAnalysis::FindInnermostLoops(std::vector<LoopDesc *> &innermostLoops,
                                            std::unordered_map<LoopDesc *, bool> &visited, LoopDesc *lp)
{
    if (lp == nullptr) {
        return;
    }
    auto it = visited.find(lp);
    if (it != visited.end() && it->second) {
        return;
    }
    visited.emplace(lp, true);
    const auto &innerLoops = lp->GetChildLoops();
    if (innerLoops.empty()) {
        innermostLoops.emplace_back(lp);
    } else {
        for (auto innerLoop : innerLoops) {
            FindInnermostLoops(innermostLoops, visited, innerLoop);
        }
    }
}

bool ControlDepAnalysis::AddRegionNodesInTopologicalOrder(CDGRegion &region, CDGNode &root,
                                                          const MapleSet<BBID> &members)
{
    // Init predSum for memberNode except for root in region
    for (auto bbId : members) {
        auto *bb = cgFunc.GetBBFromID(bbId);
        CDGNode *cdgNode = bb->GetCDGNode();
        CHECK_FATAL(cdgNode != nullptr, "get cdgNode from bb failed");
        if (cdgNode == &root) {
            continue;
        }
        int32 predSumInRegion = 0;
        for (auto predIt = bb->GetPredsBegin(); predIt != bb->GetPredsEnd(); ++predIt) {
            CDGNode *predNode = (*predIt)->GetCDGNode();
            CHECK_FATAL(predNode != nullptr, "get CDGNode from bb failed");
            if (predNode->GetRegion() == &region) {
                predSumInRegion++;
            }
        }
        cdgNode->InitPredNodeSumInRegion(predSumInRegion);
        cdgNode->SetVisitedInTopoSort(false);
    }

    // Topological sort
    std::queue<CDGNode *> topoQueue;
    topoQueue.push(&root);
    while (!topoQueue.empty()) {
        CDGNode *curNode = topoQueue.front();
        topoQueue.pop();
        region.AddCDGNode(curNode);

        for (auto bbId : members) {
            auto *bb = cgFunc.GetBBFromID(bbId);
            CDGNode *memberNode = bb->GetCDGNode();
            CHECK_FATAL(memberNode != nullptr, "get cdgNode from bb failed");
            if (memberNode == &root || memberNode->IsVisitedInTopoSort()) {
                continue;
            }
            for (auto predIt = bb->GetPredsBegin(); predIt != bb->GetPredsEnd(); ++predIt) {
                CDGNode *predNode = (*predIt)->GetCDGNode();
                CHECK_FATAL(predNode != nullptr, "get cdgNode from bb failed");
                if (predNode == curNode) {
                    memberNode->DecPredNodeSumInRegion();
                }
            }
            if (memberNode->IsAllPredInRegionProcessed()) {
                topoQueue.push(memberNode);
                memberNode->SetVisitedInTopoSort(true);
            }
        }
    }
    // To avoid irreducible loops in reducible loops, need to modify the loop analysis algorithm in the future.
    if (region.GetRegionNodeSize() != members.size()) {
        return false;
    }
    return true;
}

/** This region computing algorithm is based on this paper:
 *      The Program Dependence Graph and Its Use in Optimization
 * It traverses the post-dominator tree by means of a post-order to assure that
 * all children in the post-dominator tree are visited before their parent.
 * The region is non-linear too.
 * If cdgNodes that do not have any control dependency are divided into a region, the region is multi-root,
 * which is not supported in inter-block data dependence analysis
 */
void ControlDepAnalysis::ComputeSameCDRegions(bool considerNonDep)
{
    // The default bbId starts from 1
    std::vector<bool> visited(fcdg->GetFCDGNodeSize(), false);
    for (uint32 bbId = 1; bbId < fcdg->GetFCDGNodeSize(); ++bbId) {
        if (!visited[bbId]) {
            ComputeRegionForCurNode(bbId, visited);
        }
    }
    if (considerNonDep) {
        ComputeRegionForNonDepNodes();
    }
}

// Nodes that don't have any control dependency are divided into a region
void ControlDepAnalysis::ComputeRegionForNonDepNodes()
{
    CDGRegion *curRegion = nullptr;
    CDGNode *mergeNode = nullptr;
    for (auto node : fcdg->GetAllFCDGNodes()) {
        if (node == nullptr) {
            continue;
        }
        if (node->GetInEdgesNum() != 0) {
            continue;
        }
        if (curRegion == nullptr) {
            curRegion = node->GetRegion();
            CHECK_FATAL(curRegion != nullptr, "each CDGNode must be in a region");
            mergeNode = node;
        } else if (node->GetRegion() != curRegion) {
            // Merge Region
            CHECK_FATAL(mergeNode != nullptr, "invalid non-dep cdgNode");
            MergeRegions(*mergeNode, *node);
        }
    }
}

// Recursively computes the region of each node
void ControlDepAnalysis::ComputeRegionForCurNode(uint32 curBBId, std::vector<bool> &visited)
{
    if (visited[curBBId]) {
        return;
    }
    visited[curBBId] = true;
    MapleVector<uint32> children = pdom->GetPdomChildrenItem(curBBId);
    if (!children.empty()) {
        // Check that each child of the node has been computed
        for (auto childId : children) {
            if (!visited[childId]) {
                ComputeRegionForCurNode(childId, visited);
            }
        }
    }
    // Leaf nodes and the nodes whose children have been computed in the pdom-tree that can be merged region
    CreateAndDivideRegion(curBBId);
}

void ControlDepAnalysis::CreateAndDivideRegion(uint32 pBBId)
{
    // 1. Visit every CDGNode:N, Get and Create the region of the control dependence set
    CDGNode *parentNode = fcdg->GetCDGNodeFromId(CDGNodeId(pBBId));
    CHECK_FATAL(parentNode != nullptr, "get CDGNode failed");
    CDGRegion *region = FindExistRegion(*parentNode);
    if (region == nullptr) {
        region = CreateFCDGRegion(*parentNode);
    } else {
        region->AddCDGNode(parentNode);
        parentNode->SetRegion(region);
    }
    MapleVector<CDGNode *> &regionNodes = region->GetRegionNodes();
    // 2. Visit each immediate child of N in the post-dom tree, compute the intersection of CDs
    BB *curBB = parentNode->GetBB();
    CHECK_FATAL(curBB != nullptr, "get bb of CDGNode failed");
    for (auto childBBId : pdom->GetPdomChildrenItem(curBB->GetId())) {
        CDGNode *childNode = fcdg->GetCDGNodeFromId(CDGNodeId(childBBId));
        if (std::find(regionNodes.begin(), regionNodes.end(), childNode) != regionNodes.end()) {
            continue;
        }
        if (IsISEqualToCDs(*parentNode, *childNode)) {
            MergeRegions(*parentNode, *childNode);
        }
    }
}

// Check whether the region corresponding to the control dependence set exists
CDGRegion *ControlDepAnalysis::FindExistRegion(CDGNode &node) const
{
    MapleVector<CDGRegion *> &allRegions = fcdg->GetAllRegions();
    MapleVector<CDGEdge *> &curCDs = node.GetAllInEdges();
    // Nodes that don't have control dependencies are processed in a unified method at last
    if (curCDs.empty()) {
        return nullptr;
    }
    for (auto region : allRegions) {
        if (region == nullptr) {
            continue;
        }
        MapleVector<CDGEdge *> &regionCDs = region->GetCDEdges();
        if (regionCDs.size() != curCDs.size()) {
            continue;
        }
        bool isAllCDExist = true;
        for (auto curCD : curCDs) {
            CHECK_FATAL(curCD != nullptr, "invalid control dependence edge");
            bool isOneCDExist = false;
            for (auto regionCD : regionCDs) {
                CHECK_FATAL(regionCD != nullptr, "invalid control dependence edge");
                if (IsSameControlDependence(*curCD, *regionCD)) {
                    isOneCDExist = true;
                    break;
                }
            }
            if (!isOneCDExist) {
                isAllCDExist = false;
                break;
            }
        }
        if (isAllCDExist) {
            return region;
        }
    }
    return nullptr;
}

// Check whether the intersection(IS) of the control dependency set of the parent node (CDs)
// and the child node is equal to the control dependency set of the parent node
bool ControlDepAnalysis::IsISEqualToCDs(CDGNode &parent, CDGNode &child) const
{
    MapleVector<CDGEdge *> &parentCDs = parent.GetAllInEdges();
    MapleVector<CDGEdge *> &childCDs = child.GetAllInEdges();
    // Nodes that don't have control dependencies are processed in a unified method at last
    if (parentCDs.empty() || childCDs.empty()) {
        return false;
    }
    bool equal = true;
    for (auto parentCD : parentCDs) {
        CHECK_FATAL(parentCD != nullptr, "invalid CDGEdge in parentCDs");
        for (auto childCD : childCDs) {
            if (!IsSameControlDependence(*parentCD, *childCD)) {
                equal = false;
                continue;
            }
        }
        if (!equal) {
            return false;
        }
    }
    return true;
}

// Merge regions of parentNode and childNode
void ControlDepAnalysis::MergeRegions(CDGNode &mergeNode, CDGNode &candiNode)
{
    CDGRegion *oldRegion = candiNode.GetRegion();
    CHECK_FATAL(oldRegion != nullptr, "get child's CDGRegion failed");

    // Set newRegion of all memberNodes in oldRegion of child
    CDGRegion *mergeRegion = mergeNode.GetRegion();
    CHECK_FATAL(mergeRegion != nullptr, "get parent's CDGRegion failed");
    for (auto node : oldRegion->GetRegionNodes()) {
        node->SetRegion(mergeRegion);
        mergeRegion->AddCDGNode(node);
        oldRegion->RemoveCDGNode(node);
    }

    if (oldRegion->GetRegionNodeSize() == 0) {
        fcdg->RemoveRegionById(oldRegion->GetRegionId());
    }
}

CDGEdge *ControlDepAnalysis::BuildControlDependence(const BB &fromBB, const BB &toBB, int32 condition)
{
    auto *fromNode = fcdg->GetCDGNodeFromId(CDGNodeId(fromBB.GetId()));
    auto *toNode = fcdg->GetCDGNodeFromId(CDGNodeId(toBB.GetId()));
    CHECK_FATAL(fromNode != nullptr && toNode != nullptr, "get CDGNode failed");
    auto *cdgEdge = cdgMemPool.New<CDGEdge>(*fromNode, *toNode, condition);

    fromNode->AddOutEdges(cdgEdge);
    toNode->AddInEdges(cdgEdge);
    fcdg->AddFCDGEdge(cdgEdge);
    return cdgEdge;
}

CDGRegion *ControlDepAnalysis::CreateFCDGRegion(CDGNode &curNode)
{
    MapleVector<CDGEdge *> cdEdges = curNode.GetAllInEdges();
    auto *region = cdgMemPool.New<CDGRegion>(CDGRegionId(lastRegionId++), cdgAlloc);
    region->AddCDEdgeSet(cdEdges);
    region->AddCDGNode(&curNode);
    fcdg->AddRegion(*region);
    curNode.SetRegion(region);
    return region;
}

void ControlDepAnalysis::ComputeSingleBBRegions()
{
    CreateAllCDGNodes();
    CreateRegionForSingleBB();
}

// Create CDGNode for every BB
void ControlDepAnalysis::CreateAllCDGNodes()
{
    fcdg = cdgMemPool.New<FCDG>(cgFunc, cdgAlloc);
    FOR_ALL_BB(bb, &cgFunc)
    {
        if (bb->IsUnreachable()) {
            continue;
        }
        auto *node = cdgMemPool.New<CDGNode>(CDGNodeId(bb->GetId()), *bb, cdgAlloc);
        if (bb == cgFunc.GetFirstBB()) {
            node->SetEntryNode();
        }
        bb->SetCDGNode(node);
        fcdg->AddFCDGNode(*node);
    }
    // Create CDGNode for exitBB
    BB *exitBB = cgFunc.GetCommonExitBB();
    auto *exitNode = cdgMemPool.New<CDGNode>(CDGNodeId(exitBB->GetId()), *exitBB, cdgAlloc);
    exitNode->SetExitNode();
    exitBB->SetCDGNode(exitNode);
    fcdg->AddFCDGNode(*exitNode);
}

// Compute the pdom info only in the CDGRegion:
// we copy the cfg of the CDGRegion and perform pdom analysis on the tmp cfg.
// Currently, only for the innermost reducible loops.
// e.g.
//         tmpCommonEntry
//             |
//           ....
//             | (regionInEdge)
//            \|/
//    |--|--->BB9
//    |  |       \
//    |  |      BB10
//    |  |       /   \
//    |  |     BB11  BB16
//    |  |       \   /
//    |  |        BB12
//    |  |        /    \   (regionOutEdge)
//    |  |      BB13   _\/
//    |  |      /       ....   ---------
//    |--|--->BB14                     |
//              \                      |
//              _\/ (regionOutEdge)    |
//              ....                   |
//                \                    |
//                  tmpCommonExit ------
// based on the global cfg, the BB12 pdom BB9;
// but based on the region cfg, the BB12 not pdom BB9.
Dominance *ControlDepAnalysis::ComputePdomInRegion(CDGRegion &region, std::vector<BB *> &nonUniformRegionCFG,
                                                   uint32 &maxBBId)
{
    if (region.GetRegionNodeSize() <= 1) {
        return nullptr;
    }
    // Copy the cfg of the CDGRegion
    std::set<BB *> regionInBBs;
    std::set<BB *> regionOutBBs;
    maxBBId = 0;
    BB *startBB = nullptr;
    BB *endBB = nullptr;
    for (auto *memberNode : region.GetRegionNodes()) {
        BB *memberBB = memberNode->GetBB();
        ASSERT_NOT_NULL(memberBB);
        BB *tmpBB = GetExistBB(nonUniformRegionCFG, memberBB->GetId());
        if (tmpBB == nullptr) {
            tmpBB = cdgMemPool.New<BB>(memberBB->GetId(), cdgAlloc);
            nonUniformRegionCFG.emplace_back(tmpBB);
        }
        if (memberNode == region.GetRegionRoot()) {
            startBB = tmpBB;
        } else if (memberNode == region.GetRegionNodes().back()) {
            endBB = tmpBB;
        }
        maxBBId = (tmpBB->GetId() > maxBBId ? tmpBB->GetId() : maxBBId);
        bool hasOutSidePred = false;
        for (auto *predBB : memberBB->GetPreds()) {
            if (predBB == nullptr) {
                continue;
            }
            ASSERT_NOT_NULL(predBB->GetCDGNode());
            BB *existBB = GetExistBB(nonUniformRegionCFG, predBB->GetId());
            if (existBB != nullptr) {
                tmpBB->PushBackPreds(*existBB);
                existBB->PushBackSuccs(*tmpBB);
            } else {
                auto *tmpPredBB = cdgMemPool.New<BB>(predBB->GetId(), cdgAlloc);
                if (predBB->GetCDGNode()->GetRegion() == &region) {
                    tmpBB->PushBackPreds(*tmpPredBB);
                    tmpPredBB->PushBackSuccs(*tmpBB);
                } else if (!hasOutSidePred) {
                    tmpBB->PushBackPreds(*tmpPredBB);
                    tmpPredBB->PushBackSuccs(*tmpBB);
                    regionInBBs.insert(tmpPredBB);
                    hasOutSidePred = true;
                } else {
                    continue;
                }
                nonUniformRegionCFG.emplace_back(tmpPredBB);
                maxBBId = (tmpPredBB->GetId() > maxBBId ? tmpPredBB->GetId() : maxBBId);
            }
        }
        bool hasOutSideSucc = false;
        for (auto *succBB : memberBB->GetSuccs()) {
            if (succBB == nullptr) {
                continue;
            }
            ASSERT_NOT_NULL(succBB->GetCDGNode());
            BB *existBB = GetExistBB(nonUniformRegionCFG, succBB->GetId());
            if (existBB != nullptr) {
                tmpBB->PushBackSuccs(*existBB);
                existBB->PushBackPreds(*tmpBB);
            } else {
                auto *tmpSuccBB = cdgMemPool.New<BB>(succBB->GetId(), cdgAlloc);
                if (succBB->GetCDGNode()->GetRegion() == &region) {
                    tmpBB->PushBackSuccs(*tmpSuccBB);
                    tmpSuccBB->PushBackPreds(*tmpBB);
                } else if (!hasOutSideSucc) {
                    tmpBB->PushBackSuccs(*tmpSuccBB);
                    tmpSuccBB->PushBackPreds(*tmpBB);
                    regionOutBBs.insert(tmpSuccBB);
                    hasOutSideSucc = true;
                } else {
                    continue;
                }
                nonUniformRegionCFG.emplace_back(tmpSuccBB);
                maxBBId = (tmpSuccBB->GetId() > maxBBId ? tmpSuccBB->GetId() : maxBBId);
            }
        }
    }
    // Create temp commonEntry
    maxBBId++;
    BB *tmpEntry = cdgMemPool.New<BB>(maxBBId, cdgAlloc);
    if (!regionInBBs.empty()) {
        for (auto *regionIn : regionInBBs) {
            tmpEntry->PushBackSuccs(*regionIn);
        }
    } else {
        CHECK_NULL_FATAL(startBB);
        tmpEntry->PushBackSuccs(*startBB);
    }
    nonUniformRegionCFG.emplace_back(tmpEntry);
    // Create temp CommonExit
    maxBBId++;
    BB *tmpExit = cdgMemPool.New<BB>(maxBBId, cdgAlloc);
    if (!regionOutBBs.empty()) {
        for (auto *regionOut : regionOutBBs) {
            tmpExit->PushBackPreds(*regionOut);
        }
    } else {
        CHECK_NULL_FATAL(endBB);
        tmpExit->PushBackPreds(*endBB);
    }
    nonUniformRegionCFG.emplace_back(tmpExit);
    // Uniform region bb vector: <BBId, BB*>
    MapleVector<BaseGraphNode *> regionCFG(maxBBId + 1, cdgAlloc.Adapter());
    for (auto *bb : nonUniformRegionCFG) {
        regionCFG[bb->GetId()] = bb;
    }
    // For pdom analysis, the commonEntry and commonExit need to be reversed
    auto *regionPdom = cdgMemPool.New<Dominance>(cdgMemPool, regionCFG, *tmpExit, *tmpEntry, true);
    regionPdom->Init();
    return regionPdom;
}

bool ControlDepAnalysis::IsInDifferentSCCNode(CDGRegion &region, std::vector<BB *> &regionCFG, uint32 maxBBId,
                                              const BB &curBB, const BB &memberBB)
{
    // For fallthrough region, we do not need to check this
    if (region.GetBackBBId() == UINT32_MAX) {
        return false;
    }
    // Before pdom analysis, uniform region bb vector: <BBId, BB*>
    MapleVector<BaseGraphNode *> uniformedRegionCFG(maxBBId + 1, cdgAlloc.Adapter());
    for (auto *bb : regionCFG) {
        uniformedRegionCFG[bb->GetId()] = bb;
    }
    // 1. Check SCC
    // Before buildSCC, record regionCFG, but not record commonEntry and commonExit
    regionCFG.erase(regionCFG.end() - kNumOne);
    regionCFG.erase(regionCFG.end() - kNumTwo);
    MapleVector<SCCNode<BB> *> sccs(cdgAlloc.Adapter());
    (void)BuildSCC(cdgAlloc, maxBBId, regionCFG, false, sccs, true);
    for (auto *scc : sccs) {
        if (!scc->HasRecursion()) {
            continue;
        }
        uint32 count = 0;
        for (BB *bb : scc->GetNodes()) {
            if (bb->GetId() == curBB.GetId() || bb->GetId() == memberBB.GetId()) {
                count++;
            }
        }
        // The two equivalent candidate BBs are not in the same SCC,
        // we can not thin they are equivalent.
        if (count == 1) {
            return true;
        }
    }
    // 2. Check pdom analysis for cfg of region that removes the back edge.
    //    Region construction ensures that there is only one back edge in the region.
    bool allSuccOfBackBBInRegion = true;
    for (auto *succ : cgFunc.GetBBFromID(region.GetBackBBId())->GetSuccs()) {
        CDGNode *cdgNode = succ->GetCDGNode();
        ASSERT_NOT_NULL(cdgNode);
        if (cdgNode->GetRegion() != &region) {
            allSuccOfBackBBInRegion = false;
            break;
        }
    }
    if (allSuccOfBackBBInRegion) {
        BB *commonEntryBB = static_cast<BB *>(uniformedRegionCFG[uniformedRegionCFG.size() - 2]);
        BB *commonExitBB = static_cast<BB *>(uniformedRegionCFG[uniformedRegionCFG.size() - 1]);
        ASSERT_NOT_NULL(region.GetRegionRoot());
        ASSERT_NOT_NULL(region.GetRegionRoot()->GetBB());
        BB *headerBB = static_cast<BB *>(uniformedRegionCFG[region.GetRegionRoot()->GetBB()->GetId()]);
        BB *backBB = static_cast<BB *>(uniformedRegionCFG[region.GetBackBBId()]);
        backBB->RemoveSuccs(*headerBB);
        headerBB->RemovePreds(*backBB);
        commonExitBB->PushBackPreds(*backBB);
        auto *pruneRegionPdom =
            cdgMemPool.New<Dominance>(cdgMemPool, uniformedRegionCFG, *commonExitBB, *commonEntryBB, true);
        pruneRegionPdom->Init();
        if (!pruneRegionPdom->Dominate(memberBB.GetId(), curBB.GetId())) {
            return true;
        }
    }
    return false;
}

/** Find equivalent candidate nodes of current cdgNode:
 *   A and B are equivalent if and only if A dominates B and B post-dominates A
 * And it must be behind the current cdgNode in the topology order
 */
void ControlDepAnalysis::GetEquivalentNodesInRegion(CDGRegion &region, CDGNode &cdgNode,
                                                    std::vector<CDGNode *> &equivalentNodes)
{
    BB *curBB = cdgNode.GetBB();
    CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
    MapleVector<CDGNode *> &memberNodes = region.GetRegionNodes();
    bool isBehind = false;
    for (auto member : memberNodes) {
        if (member == &cdgNode) {
            isBehind = true;
            continue;
        }
        BB *memberBB = member->GetBB();
        CHECK_FATAL(memberBB != nullptr, "get bb from cdgNode failed");
        std::vector<BB *> nonUniformRegionCFG;
        uint32 maxBBId = 0;
        Dominance *regionPdom = ComputePdomInRegion(region, nonUniformRegionCFG, maxBBId);
        if (dom->Dominate(*curBB, *memberBB) &&
            (regionPdom != nullptr && regionPdom->Dominate(memberBB->GetId(), curBB->GetId())) && isBehind) {
            // To avoid the loop-carried instructions are scheduled
            bool isInPartialCycle = false;
            for (auto predBB : memberBB->GetPreds()) {
                if (predBB->GetCDGNode()->GetRegion() == &region && predBB->GetSuccsSize() > 1) {
                    isInPartialCycle = true;
                    break;
                }
            }
            if (!isInPartialCycle && !IsInDifferentSCCNode(region, nonUniformRegionCFG, maxBBId, *curBB, *memberBB)) {
                equivalentNodes.emplace_back(member);
            }
        }
    }
}

void ControlDepAnalysis::GenerateFCDGDot() const
{
    CHECK_FATAL(fcdg != nullptr, "construct FCDG failed");
    MapleVector<CDGNode *> &allNodes = fcdg->GetAllFCDGNodes();
    MapleVector<CDGEdge *> &allEdges = fcdg->GetAllFCDGEdges();
    MapleVector<CDGRegion *> &allRegions = fcdg->GetAllRegions();

    std::streambuf *coutBuf = std::cout.rdbuf();
    std::ofstream fcdgFile;
    std::streambuf *fileBuf = fcdgFile.rdbuf();
    (void)std::cout.rdbuf(fileBuf);

    // Define the output file name
    std::string fileName;
    (void)fileName.append("fcdg_");
    (void)fileName.append(cgFunc.GetName());
    (void)fileName.append(".dot");

    fcdgFile.open(fileName, std::ios::trunc);
    if (!fcdgFile.is_open()) {
        LogInfo::MapleLogger(kLlWarn) << "fileName:" << fileName << " open failed.\n";
        return;
    }
    fcdgFile << "digraph FCDG_" << cgFunc.GetName() << " {\n\n";
    fcdgFile << "  node [shape=box,style=filled,color=lightgrey];\n\n";

    // Dump nodes style
    for (auto node : allNodes) {
        if (node == nullptr) {
            continue;
        }
        BB *bb = node->GetBB();
        CHECK_FATAL(bb != nullptr, "get bb of CDGNode failed");
        fcdgFile << "  BB_" << bb->GetId();
        fcdgFile << "[label= \"";
        if (node->IsEntryNode()) {
            fcdgFile << "ENTRY\n";
        } else if (node->IsExitNode()) {
            fcdgFile << "EXIT\n";
        }
        fcdgFile << "BB_" << bb->GetId() << " Label_" << bb->GetLabIdx() << ":\n";
        fcdgFile << "  { " << bb->GetKindName() << " }\"];\n";
    }
    fcdgFile << "\n";

    // Dump edges style
    for (auto edge : allEdges) {
        CDGNode &fromNode = edge->GetFromNode();
        CDGNode &toNode = edge->GetToNode();
        fcdgFile << "  BB_" << fromNode.GetBB()->GetId() << " -> "
                 << "BB_" << toNode.GetBB()->GetId();
        fcdgFile << " [label = \"";
        fcdgFile << edge->GetCondition() << "\"];\n";
    }
    fcdgFile << "\n";

    // Dump region style using cluster in dot language
    for (auto region : allRegions) {
        if (region == nullptr) {
            continue;
        }
        CHECK_FATAL(region->GetRegionNodeSize() != 0, "invalid region");
        fcdgFile << "  subgraph cluster_" << region->GetRegionId() << " {\n";
        fcdgFile << "    color=red;\n";
        fcdgFile << "    label = \"region #" << region->GetRegionId() << "\";\n";
        MapleVector<CDGNode *> &memberNodes = region->GetRegionNodes();
        for (auto node : memberNodes) {
            fcdgFile << "    BB_" << node->GetBB()->GetId() << ";\n";
        }
        fcdgFile << "}\n\n";
    }

    fcdgFile << "}\n";
    (void)fcdgFile.flush();
    fcdgFile.close();
    (void)std::cout.rdbuf(coutBuf);
}

void ControlDepAnalysis::GenerateCFGDot() const
{
    std::streambuf *coutBuf = std::cout.rdbuf();
    std::ofstream cfgFile;
    std::streambuf *fileBuf = cfgFile.rdbuf();
    (void)std::cout.rdbuf(fileBuf);

    // Define the output file name
    std::string fileName;
    (void)fileName.append("cfg_after_cdg_");
    (void)fileName.append(cgFunc.GetName());
    (void)fileName.append(".dot");

    cfgFile.open(fileName, std::ios::trunc);
    if (!cfgFile.is_open()) {
        LogInfo::MapleLogger(kLlWarn) << "fileName:" << fileName << " open failed.\n";
        return;
    }

    cfgFile << "digraph CFG_" << cgFunc.GetName() << " {\n\n";
    cfgFile << "  node [shape=box];\n\n";

    // Dump nodes style
    FOR_ALL_BB_CONST(bb, &cgFunc)
    {
        if (bb->IsUnreachable()) {
            continue;
        }
        cfgFile << "  BB_" << bb->GetId();
        cfgFile << "[label= \"";
        if (bb == cgFunc.GetFirstBB()) {
            cfgFile << "ENTRY\n";
        }
        cfgFile << "BB_" << bb->GetId() << " Label_" << bb->GetLabIdx() << ":\n";
        cfgFile << "  { " << bb->GetKindName() << " }\"];\n";
    }
    BB *exitBB = cgFunc.GetCommonExitBB();
    cfgFile << "  BB_" << exitBB->GetId();
    cfgFile << "[label= \"EXIT\n";
    cfgFile << "BB_" << exitBB->GetId() << "\"];\n";
    cfgFile << "\n";

    // Dump edges style
    for (auto cfgEdge : cfgMST->GetAllEdges()) {
        BB *srcBB = cfgEdge->GetSrcBB();
        BB *destBB = cfgEdge->GetDestBB();
        CHECK_FATAL(srcBB != nullptr && destBB != nullptr, "get wrong cfg-edge");
        if (srcBB == cgFunc.GetCommonExitBB()) {
            continue;
        }
        cfgFile << "  BB_" << srcBB->GetId() << " -> "
                << "BB_" << destBB->GetId();
        cfgFile << " [label = \"";
        cfgFile << cfgEdge->GetCondition() << "\"";
        if (cfgEdge->IsBackEdge()) {
            cfgFile << ",color=darkorchid1";
        }
        cfgFile << "];\n";
    }
    cfgFile << "\n";

    // Dump region style using cluster in dot language
    for (auto region : fcdg->GetAllRegions()) {
        if (region == nullptr) {
            continue;
        }
        CHECK_FATAL(region->GetRegionNodeSize() != 0, "invalid region");
        cfgFile << "  subgraph cluster_" << region->GetRegionId() << " {\n";
        cfgFile << "    color=red;\n";
        cfgFile << "    label = \"region #" << region->GetRegionId() << "\";\n";
        for (auto cdgNode : region->GetRegionNodes()) {
            BB *bb = cdgNode->GetBB();
            CHECK_FATAL(bb != nullptr, "get bb from cdgNode failed");
            cfgFile << "    BB_" << bb->GetId() << ";\n";
        }
        cfgFile << "}\n\n";
    }

    cfgFile << "}\n";
    (void)cfgFile.flush();
    cfgFile.close();
    (void)std::cout.rdbuf(coutBuf);
}

void ControlDepAnalysis::GenerateSimplifiedCFGDot() const
{
    std::streambuf *coutBuf = std::cout.rdbuf();
    std::ofstream cfgFile;
    std::streambuf *fileBuf = cfgFile.rdbuf();
    (void)std::cout.rdbuf(fileBuf);

    // Define the output file name
    std::string fileName;
    (void)fileName.append("cfg_simplify_");
    (void)fileName.append(cgFunc.GetName());
    (void)fileName.append(".dot");

    cfgFile.open(fileName.c_str(), std::ios::trunc);
    if (!cfgFile.is_open()) {
        LogInfo::MapleLogger(kLlWarn) << "fileName:" << fileName << " open failed.\n";
        return;
    }

    cfgFile << "digraph CFG_SIMPLE" << cgFunc.GetName() << " {\n\n";
    cfgFile << "  node [shape=box];\n\n";

    // Dump nodes style
    FOR_ALL_BB_CONST(bb, &cgFunc)
    {
        if (bb->IsUnreachable()) {
            continue;
        }
        cfgFile << "  BB_" << bb->GetId();
        cfgFile << "[label= \"";
        if (bb == cgFunc.GetFirstBB()) {
            cfgFile << "ENTRY\n";
        }
        cfgFile << bb->GetId() << "\"];\n";
    }
    BB *exitBB = cgFunc.GetCommonExitBB();
    cfgFile << "  BB_" << exitBB->GetId();
    cfgFile << "[label= \"EXIT\n";
    cfgFile << exitBB->GetId() << "\"];\n";
    cfgFile << "\n";

    // Dump edges style
    for (auto cfgEdge : cfgMST->GetAllEdges()) {
        BB *srcBB = cfgEdge->GetSrcBB();
        BB *destBB = cfgEdge->GetDestBB();
        CHECK_FATAL(srcBB != nullptr && destBB != nullptr, "get wrong cfg-edge");
        if (srcBB == cgFunc.GetCommonExitBB()) {
            continue;
        }
        cfgFile << "  BB_" << srcBB->GetId() << " -> "
                << "BB_" << destBB->GetId();
        cfgFile << " [label = \"";
        cfgFile << cfgEdge->GetCondition() << "\"";
        if (cfgEdge->IsBackEdge()) {
            cfgFile << ",color=darkorchid1";
        }
        cfgFile << "];\n";
    }

    cfgFile << "}\n";
    (void)cfgFile.flush();
    cfgFile.close();
    (void)std::cout.rdbuf(coutBuf);
}

void ControlDepAnalysis::GenerateCFGInRegionDot(CDGRegion &region) const
{
    std::streambuf *coutBuf = std::cout.rdbuf();
    std::ofstream cfgOfRFile;
    std::streambuf *fileBuf = cfgOfRFile.rdbuf();
    (void)std::cout.rdbuf(fileBuf);

    // Define the output file name
    std::string fileName;
    (void)fileName.append("cfg_region");
    (void)fileName.append(std::to_string(region.GetRegionId()));
    (void)fileName.append("_");
    (void)fileName.append(cgFunc.GetName());
    (void)fileName.append(".dot");

    cfgOfRFile.open(fileName.c_str(), std::ios::trunc);
    if (!cfgOfRFile.is_open()) {
        LogInfo::MapleLogger(kLlWarn) << "fileName:" << fileName << " open failed.\n";
        return;
    }

    cfgOfRFile << "digraph CFG_REGION" << region.GetRegionId() << " {\n\n";
    cfgOfRFile << "  node [shape=box];\n\n";

    for (auto cdgNode : region.GetRegionNodes()) {
        BB *bb = cdgNode->GetBB();
        CHECK_FATAL(bb != nullptr, "get bb from cdgNode failed");

        for (auto succ = bb->GetSuccsBegin(); succ != bb->GetSuccsEnd(); ++succ) {
            CDGNode *node = (*succ)->GetCDGNode();
            CHECK_FATAL(node != nullptr, "get cdgNode from bb failed");
            if (node->GetRegion() == &region) {
                cfgOfRFile << "\tbb_" << bb->GetId() << " -> "
                           << "bb_" << (*succ)->GetId() << "\n";
            }
        }
    }
    cfgOfRFile << "}\n";
    (void)cfgOfRFile.flush();
    cfgOfRFile.close();
    (void)std::cout.rdbuf(coutBuf);
}

void CgControlDepAnalysis::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgDomAnalysis>();
    aDep.AddRequired<CgPostDomAnalysis>();
    aDep.AddRequired<CgLoopAnalysis>();
    aDep.SetPreservedAll();
}

bool CgControlDepAnalysis::PhaseRun(maplebe::CGFunc &f)
{
    MemPool *cdgMemPool = GetPhaseMemPool();
    MemPool *tmpMemPool = ApplyTempMemPool();
    CHECK_FATAL(cdgMemPool != nullptr && tmpMemPool != nullptr, "get memPool failed");
    DomAnalysis *domInfo = GET_ANALYSIS(CgDomAnalysis, f);
    CHECK_FATAL(domInfo != nullptr, "get result of DomAnalysis failed");
    PostDomAnalysis *pdomInfo = GET_ANALYSIS(CgPostDomAnalysis, f);
    CHECK_FATAL(pdomInfo != nullptr, "get result of PostDomAnalysis failed");
    LoopAnalysis *loopInfo = GET_ANALYSIS(CgLoopAnalysis, f);
    CHECK_FATAL(loopInfo != nullptr, "get result of LoopAnalysis failed");
    auto *cfgMST = cdgMemPool->New<CFGMST<BBEdge<maplebe::BB>, maplebe::BB>>(*cdgMemPool);
    cda = f.IsAfterRegAlloc() ? cdgMemPool->New<ControlDepAnalysis>(f, *cdgMemPool, "localschedule", true)
                              : cdgMemPool->New<ControlDepAnalysis>(f, *cdgMemPool, *tmpMemPool, *domInfo, *pdomInfo,
                                                                    *loopInfo, cfgMST, "globalschedule");
    cda->Run();
    return true;
}
MAPLE_ANALYSIS_PHASE_REGISTER(CgControlDepAnalysis, cgcontroldepanalysis)
}  // namespace maplebe
