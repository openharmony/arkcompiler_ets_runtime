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

/*
 * The file defines the data structures of Control Dependence Graph(CDG).
 */
#ifndef MAPLEBE_INCLUDE_CG_CG_CDG_H
#define MAPLEBE_INCLUDE_CG_CG_CDG_H

#include <string>
#include "cgfunc.h"
#include "mpl_number.h"
#include "deps.h"
#include "mempool_allocator.h"

namespace maplebe {
class CDGNode;
class CDGEdge;
class CDGRegion;
using CDGNodeId = utils::Index<CDGNode>;
using CDGRegionId = utils::Index<CDGRegion>;

// Encapsulation of BB
class CDGNode {
public:
    CDGNode(CDGNodeId nId, BB &bb, MapleAllocator &alloc)
        : id(nId),
          bb(&bb),
          outEdges(alloc.Adapter()),
          inEdges(alloc.Adapter()),
          lastComments(alloc.Adapter()),
          dataNodes(alloc.Adapter()),
          cfiInsns(alloc.Adapter())
    {
    }
    virtual ~CDGNode()
    {
        topoPredInRegion = nullptr;
        lastFrameDef = nullptr;
        bb = nullptr;
        regUses = nullptr;
        mayThrows = nullptr;
        ehInRegs = nullptr;
        heapDefs = nullptr;
        ambiInsns = nullptr;
        membarInsn = nullptr;
        pseudoSepNodes = nullptr;
        lastCallInsn = nullptr;
        lastInlineAsmInsn = nullptr;
        regDefs = nullptr;
        stackDefs = nullptr;
        region = nullptr;
        heapUses = nullptr;
        stackUses = nullptr;
    }

    BB *GetBB()
    {
        return bb;
    }

    const BB *GetBB() const
    {
        return bb;
    }

    CDGNodeId GetNodeId()
    {
        return id;
    }

    CDGNodeId GetNodeId() const
    {
        return id;
    }

    void SetNodeId(CDGNodeId nId)
    {
        id = nId;
    }

    CDGRegion *GetRegion()
    {
        return region;
    }

    const CDGRegion *GetRegion() const
    {
        return region;
    }

    void SetRegion(CDGRegion *cdgRegion)
    {
        region = cdgRegion;
    }

    void ClearRegion()
    {
        region = nullptr;
    }

    bool IsEntryNode() const
    {
        return isEntryNode;
    }

    void SetEntryNode()
    {
        isEntryNode = true;
    }

    bool IsExitNode() const
    {
        return isExitNode;
    }

    void SetExitNode()
    {
        isExitNode = true;
    }

    void AddOutEdges(CDGEdge *edge)
    {
        outEdges.emplace_back(edge);
    }

    MapleVector<CDGEdge *> &GetAllOutEdges()
    {
        return outEdges;
    }

    void AddInEdges(CDGEdge *edge)
    {
        inEdges.emplace_back(edge);
    }

    MapleVector<CDGEdge *> &GetAllInEdges()
    {
        return inEdges;
    }

    std::size_t GetOutEdgesNum() const
    {
        return outEdges.size();
    }

    std::size_t GetInEdgesNum() const
    {
        return inEdges.size();
    }

    void SetVisitedInTopoSort(bool isVisited)
    {
        isVisitedInTopoSort = isVisited;
    }

    bool IsVisitedInTopoSort() const
    {
        return isVisitedInTopoSort;
    }

    void SetVisitedInExtendedFind()
    {
        isVisitedInExtendedFind = true;
    }

    bool IsVisitedInExtendedFind() const
    {
        return isVisitedInExtendedFind;
    }

    uint32 GetInsnNum() const
    {
        return insnNum;
    }

    void SetInsnNum(uint32 num)
    {
        insnNum = num;
    }

    bool HasAmbiRegs() const
    {
        return hasAmbiRegs;
    }

    void SetHasAmbiRegs(bool flag)
    {
        hasAmbiRegs = flag;
    }

    Insn *GetMembarInsn()
    {
        return membarInsn;
    }

    void SetMembarInsn(Insn *insn)
    {
        membarInsn = insn;
    }

    Insn *GetLastCallInsn()
    {
        return lastCallInsn;
    }

    void SetLastCallInsn(Insn *callInsn)
    {
        lastCallInsn = callInsn;
    }

    Insn *GetLastFrameDefInsn()
    {
        return lastFrameDef;
    }

    void SetLastFrameDefInsn(Insn *frameInsn)
    {
        lastFrameDef = frameInsn;
    }

    Insn *GetLastInlineAsmInsn()
    {
        return lastInlineAsmInsn;
    }

    void SetLastInlineAsmInsn(Insn *asmInsn)
    {
        lastInlineAsmInsn = asmInsn;
    }

    void InitTopoInRegionInfo(MemPool &tmpMp, MapleAllocator &tmpAlloc)
    {
        topoPredInRegion = tmpMp.New<MapleSet<CDGNodeId>>(tmpAlloc.Adapter());
    }

    void ClearTopoInRegionInfo()
    {
        topoPredInRegion = nullptr;
    }

    void InitDataDepInfo(MemPool &tmpMp, MapleAllocator &tmpAlloc, uint32 maxRegNum)
    {
        regDefs = tmpMp.New<MapleVector<Insn *>>(tmpAlloc.Adapter());
        regUses = tmpMp.New<MapleVector<RegList *>>(tmpAlloc.Adapter());
        stackUses = tmpMp.New<MapleVector<Insn *>>(tmpAlloc.Adapter());
        stackDefs = tmpMp.New<MapleVector<Insn *>>(tmpAlloc.Adapter());
        heapUses = tmpMp.New<MapleVector<Insn *>>(tmpAlloc.Adapter());
        heapDefs = tmpMp.New<MapleVector<Insn *>>(tmpAlloc.Adapter());
        mayThrows = tmpMp.New<MapleVector<Insn *>>(tmpAlloc.Adapter());
        ambiInsns = tmpMp.New<MapleVector<Insn *>>(tmpAlloc.Adapter());
        pseudoSepNodes = tmpMp.New<MapleVector<DepNode *>>(tmpAlloc.Adapter());
        ehInRegs = tmpMp.New<MapleSet<regno_t>>(tmpAlloc.Adapter());

        regDefs->resize(maxRegNum);
        regUses->resize(maxRegNum);
    }

    void ClearDataDepInfo()
    {
        membarInsn = nullptr;
        lastCallInsn = nullptr;
        lastFrameDef = nullptr;
        lastInlineAsmInsn = nullptr;
        lastComments.clear();

        regDefs = nullptr;
        regUses = nullptr;
        stackUses = nullptr;
        stackDefs = nullptr;
        heapUses = nullptr;
        heapDefs = nullptr;
        mayThrows = nullptr;
        ambiInsns = nullptr;
        pseudoSepNodes = nullptr;
        ehInRegs = nullptr;
    }

    void ClearDepDataVec()
    {
        membarInsn = nullptr;
        lastCallInsn = nullptr;
        lastFrameDef = nullptr;
        lastInlineAsmInsn = nullptr;

        for (auto &regDef : *regDefs) {
            regDef = nullptr;
        }
        for (auto &regUse : *regUses) {
            regUse = nullptr;
        }

        stackUses->clear();
        stackDefs->clear();
        heapUses->clear();
        heapDefs->clear();
        mayThrows->clear();
        ambiInsns->clear();
    }

    Insn *GetLatestDefInsn(regno_t regNO)
    {
        return (*regDefs)[regNO];
    }

    void SetLatestDefInsn(regno_t regNO, Insn *defInsn)
    {
        (*regDefs)[regNO] = defInsn;
    }

    RegList *GetUseInsnChain(regno_t regNO)
    {
        return (*regUses)[regNO];
    }

    void AppendUseInsnChain(regno_t regNO, Insn *useInsn, MemPool &mp)
    {
        CHECK_FATAL(useInsn != nullptr, "invalid useInsn");
        auto *newUse = mp.New<RegList>();
        newUse->insn = useInsn;
        newUse->next = nullptr;

        RegList *headUse = (*regUses)[regNO];
        if (headUse == nullptr) {
            (*regUses)[regNO] = newUse;
        } else {
            while (headUse->next != nullptr) {
                headUse = headUse->next;
            }
            headUse->next = newUse;
        }
    }

    void ClearUseInsnChain(regno_t regNO)
    {
        (*regUses)[regNO] = nullptr;
    }

    MapleVector<Insn *> &GetStackUseInsns()
    {
        return *stackUses;
    }

    void AddStackUseInsn(Insn *stackInsn)
    {
        stackUses->emplace_back(stackInsn);
    }

    MapleVector<Insn *> &GetStackDefInsns()
    {
        return *stackDefs;
    }

    void AddStackDefInsn(Insn *stackInsn)
    {
        stackDefs->emplace_back(stackInsn);
    }

    MapleVector<Insn *> &GetHeapUseInsns()
    {
        return *heapUses;
    }

    void AddHeapUseInsn(Insn *heapInsn) const
    {
        heapUses->emplace_back(heapInsn);
    }

    MapleVector<Insn *> &GetHeapDefInsns()
    {
        return *heapDefs;
    }

    void AddHeapDefInsn(Insn *heapInsn) const
    {
        heapDefs->emplace_back(heapInsn);
    }

    MapleVector<Insn *> &GetMayThrowInsns()
    {
        return *mayThrows;
    }

    void AddMayThrowInsn(Insn *throwInsn)
    {
        mayThrows->emplace_back(throwInsn);
    }

    MapleVector<Insn *> &GetAmbiguousInsns()
    {
        return *ambiInsns;
    }

    void AddAmbiguousInsn(Insn *ambiInsn) const
    {
        ambiInsns->emplace_back(ambiInsn);
    }

    MapleSet<CDGNodeId> &GetTopoPredInRegion()
    {
        return *topoPredInRegion;
    }

    void InsertVisitedTopoPredInRegion(CDGNodeId nodeId) const
    {
        topoPredInRegion->insert(nodeId);
    }

    MapleVector<Insn *> &GetLastComments()
    {
        return lastComments;
    }

    void AddCommentInsn(Insn *commentInsn)
    {
        lastComments.emplace_back(commentInsn);
    }

    void CopyAndClearComments(MapleVector<Insn *> &comments)
    {
        lastComments = comments;
        comments.clear();
    }

    void ClearLastComments()
    {
        lastComments.clear();
    }

    void AddPseudoSepNodes(DepNode *node) const
    {
        pseudoSepNodes->emplace_back(node);
    }

    MapleSet<regno_t> &GetEhInRegs()
    {
        return *ehInRegs;
    }

    MapleVector<DepNode *> &GetAllDataNodes()
    {
        return dataNodes;
    }

    void AddDataNode(DepNode *depNode)
    {
        (void)dataNodes.emplace_back(depNode);
    }

    void ClearDataNodes()
    {
        dataNodes.clear();
    }

    MapleVector<Insn *> &GetCfiInsns()
    {
        return cfiInsns;
    }

    void AddCfiInsn(Insn *cfiInsn)
    {
        (void)cfiInsns.emplace_back(cfiInsn);
    }

    void RemoveDepNodeFromDataNodes(const DepNode &depNode)
    {
        for (auto iter = dataNodes.begin(); iter != dataNodes.end(); ++iter) {
            if (*iter == &depNode) {
                void(dataNodes.erase(iter));
                break;
            }
        }
    }

    void InitPredNodeSumInRegion(int32 predSum)
    {
        CHECK_FATAL(predSum >= 0, "invalid predSum");
        predNodesInRegion = predSum;
    }

    void DecPredNodeSumInRegion()
    {
        predNodesInRegion--;
    }

    bool IsAllPredInRegionProcessed() const
    {
        return (predNodesInRegion == 0);
    }

    uint32 &GetNodeSum()
    {
        return nodeSum;
    }

    void AccNodeSum()
    {
        nodeSum++;
    }

    void SetNodeSum(uint32 sum)
    {
        nodeSum = sum;
    }

    bool operator!=(const CDGNode &node)
    {
        if (this != &node) {
            return true;
        }
        if (this->id != node.GetNodeId() || this->bb != node.GetBB() || this->region != node.GetRegion()) {
            return true;
        }
        if (this->GetInEdgesNum() != node.GetInEdgesNum() || this->GetOutEdgesNum() != node.GetOutEdgesNum()) {
            return true;
        }
        return false;
    }

private:
    CDGNodeId id;  // same to bbId
    BB *bb = nullptr;
    CDGRegion *region = nullptr;
    bool isEntryNode = false;
    bool isExitNode = false;
    MapleVector<CDGEdge *> outEdges;
    MapleVector<CDGEdge *> inEdges;
    bool isVisitedInTopoSort = false;      // for sorting nodes in region by topological order
    bool isVisitedInExtendedFind = false;  // for finding a fallthrough path as a region
    uint32 insnNum = 0;                    // record insn total num of BB
    // The following structures are used to record data flow infos in building data dependence among insns
    bool hasAmbiRegs = false;
    Insn *membarInsn = nullptr;
    Insn *lastCallInsn = nullptr;
    Insn *lastFrameDef = nullptr;
    Insn *lastInlineAsmInsn = nullptr;
    MapleVector<Insn *> *regDefs = nullptr;     // the index is regNO, record the latest defInsn in the curBB
    MapleVector<RegList *> *regUses = nullptr;  // the index is regNO
    MapleVector<Insn *> *stackUses = nullptr;
    MapleVector<Insn *> *stackDefs = nullptr;
    MapleVector<Insn *> *heapUses = nullptr;
    MapleVector<Insn *> *heapDefs = nullptr;
    MapleVector<Insn *> *mayThrows = nullptr;
    MapleVector<Insn *> *ambiInsns = nullptr;
    MapleVector<DepNode *> *pseudoSepNodes = nullptr;
    MapleSet<regno_t> *ehInRegs = nullptr;
    MapleSet<CDGNodeId> *topoPredInRegion = nullptr;
    MapleVector<Insn *> lastComments;
    MapleVector<DepNode *> dataNodes;
    MapleVector<Insn *> cfiInsns;
    // For computing topological order of cdgNodes in a region,
    // which is initialized to the number of pred nodes in CFG at the beginning of processing the region,
    // and change dynamically
    int32 predNodesInRegion = -1;
    // For intra-block dda: it accumulates from the first insn (nodeSum = 1) of bb
    // For inter-block dda: it accumulates from the maximum of nodeSum in all the predecessor of cur cdgNode
    uint32 nodeSum = 0;
};

class CDGEdge {
public:
    CDGEdge(CDGNode &from, CDGNode &to, int32 cond) : fromNode(from), toNode(to), condition(cond) {}
    virtual ~CDGEdge() = default;

    CDGNode &GetFromNode()
    {
        return fromNode;
    }

    CDGNode &GetFromNode() const
    {
        return fromNode;
    }

    void SetFromNode(const CDGNode &from)
    {
        fromNode = from;
    }

    CDGNode &GetToNode()
    {
        return toNode;
    }

    CDGNode &GetToNode() const
    {
        return toNode;
    }

    void SetToNode(const CDGNode &to)
    {
        toNode = to;
    }

    int32 GetCondition() const
    {
        return condition;
    }

    void SetCondition(int32 cond)
    {
        condition = cond;
    }

    // for checking same control dependence
    bool operator==(const CDGEdge &edge)
    {
        if (this == &edge) {
            return true;
        }
        if (this->fromNode != edge.GetFromNode()) {
            return false;
        }
        if (this->toNode != edge.GetToNode()) {
            return false;
        }
        if (this->condition != edge.GetCondition()) {
            return false;
        }
        return true;
    }

private:
    CDGNode &fromNode;
    CDGNode &toNode;
    // allocate different COND number to different succ edges of the same fromBB
    // default value is -1 indicated no cond.
    int32 condition;
};

// A region consists of nodes with the same control dependence sets
class CDGRegion {
public:
    CDGRegion(CDGRegionId rId, MapleAllocator &alloc) : id(rId), memberNodes(alloc.Adapter()), cdEdges(alloc.Adapter())
    {
    }
    virtual ~CDGRegion()
    {
        root = nullptr;
    }

    CDGRegionId GetRegionId()
    {
        return id;
    }

    MapleVector<CDGNode *> &GetRegionNodes()
    {
        return memberNodes;
    }

    std::size_t GetRegionNodeSize() const
    {
        return memberNodes.size();
    }

    // Ensure the node is unique
    void AddCDGNode(CDGNode *node)
    {
        if (std::find(memberNodes.cbegin(), memberNodes.cend(), node) == memberNodes.cend()) {
            memberNodes.emplace_back(node);
        }
    }

    void RemoveCDGNode(CDGNode *node)
    {
        auto it = std::find(memberNodes.begin(), memberNodes.end(), node);
        if (it == memberNodes.end()) {
            return;
        }
        (void)memberNodes.erase(it);
    }

    CDGNode *GetCDGNodeById(CDGNodeId nodeId)
    {
        for (auto cdgNode : memberNodes) {
            if (cdgNode->GetNodeId() == nodeId) {
                return cdgNode;
            }
        }
        return nullptr;
    }

    MapleVector<CDGEdge *> &GetCDEdges()
    {
        return cdEdges;
    }

    void AddCDEdge(CDGEdge *edge)
    {
        cdEdges.emplace_back(edge);
    }

    void AddCDEdgeSet(MapleVector<CDGEdge *> &cds)
    {
        for (auto &cd : cds) {
            cdEdges.emplace_back(cd);
        }
    }

    uint32 GetMaxBBIdInRegion()
    {
        uint32 maxId = 0;
        for (auto node : memberNodes) {
            maxId = (node->GetNodeId() > maxId ? static_cast<uint32>(node->GetNodeId()) : maxId);
        }
        return maxId;
    }

    void SetRegionRoot(CDGNode &node)
    {
        root = &node;
    }

    CDGNode *GetRegionRoot()
    {
        return root;
    }

    void SetBackBBId(BBID bbId)
    {
        backEdgeFromBBId = bbId;
    }

    BBID GetBackBBId() const
    {
        return backEdgeFromBBId;
    }

private:
    CDGRegionId id;
    MapleVector<CDGNode *> memberNodes;  // The nodes in CDGRegion by topological order
    // The control dependence sets of the parent node.
    // If it is a general non-linear region, the cdEdges is empty.
    MapleVector<CDGEdge *> cdEdges;
    CDGNode *root = nullptr;
    BBID backEdgeFromBBId = UINT32_MAX;  // For loop region
};

// Forward Control Dependence Graph
// which does not compute the control dependence on the back edges
class FCDG {
public:
    FCDG(const CGFunc &f, MapleAllocator &alloc)
        : nodes(f.GetAllBBSize(), alloc.Adapter()),
          fcds(alloc.Adapter()),
          regions(f.GetAllBBSize() + 1, alloc.Adapter())
    {
    }
    virtual ~FCDG() = default;

    MapleVector<CDGNode *> &GetAllFCDGNodes()
    {
        return nodes;
    }

    std::size_t GetFCDGNodeSize() const
    {
        return nodes.size();
    }

    CDGNode *GetCDGNodeFromId(CDGNodeId id)
    {
        return nodes[id];
    }

    MapleVector<CDGEdge *> &GetAllFCDGEdges()
    {
        return fcds;
    }

    MapleVector<CDGRegion *> &GetAllRegions()
    {
        return regions;
    }

    CDGRegion *GetRegionFromId(CDGRegionId id)
    {
        return regions[id];
    }

    // Ensure the node is unique
    void AddFCDGNode(CDGNode &node)
    {
        if (nodes[node.GetNodeId()] == nullptr) {
            nodes[node.GetNodeId()] = &node;
        }
    }

    void AddFCDGEdge(CDGEdge *edge)
    {
        fcds.emplace_back(edge);
    }

    // Ensure the region is unique
    void AddRegion(CDGRegion &region)
    {
        if (regions[region.GetRegionId()] == nullptr) {
            regions[region.GetRegionId()] = &region;
        }
    }

    void RemoveRegionById(CDGRegionId id)
    {
        regions[id] = nullptr;
    }

    // Provide interfaces for global scheduling

private:
    MapleVector<CDGNode *> nodes;      // all CDGNodes in FCDG that use nodeId as the index
    MapleVector<CDGEdge *> fcds;       // all forward-control-dependence in FCDG
    MapleVector<CDGRegion *> regions;  // all regions in FCDG that use CDGRegionId as the index
};

struct CDGOutEdgeComparator {
    bool operator()(const CDGEdge &outEdge1, const CDGEdge &outEdge2) const
    {
        const CDGNode &toNode1 = outEdge1.GetToNode();
        const CDGNode &toNode2 = outEdge2.GetToNode();
        return toNode1.GetNodeId() < toNode2.GetNodeId();
    }
};

struct CDGInEdgeComparator {
    bool operator()(const CDGEdge &inEdge1, const CDGEdge &inEdge2) const
    {
        const CDGNode &fromNode1 = inEdge1.GetFromNode();
        const CDGNode &fromNode2 = inEdge2.GetToNode();
        return fromNode1.GetNodeId() < fromNode2.GetNodeId();
    }
};
}  // namespace maplebe

namespace std {
template <>
struct hash<maplebe::CDGNodeId> {
    size_t operator()(const maplebe::CDGNodeId &nId) const
    {
        return nId;
    }
};

template <>
struct hash<maplebe::CDGRegionId> {
    size_t operator()(const maplebe::CDGRegionId &rId) const
    {
        return rId;
    }
};
}  // namespace std
#endif  // MAPLEBE_INCLUDE_CG_CG_CDG_H
