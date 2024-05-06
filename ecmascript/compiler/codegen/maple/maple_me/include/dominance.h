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
#include "mpl_number.h"
#include "mempool.h"
#include "mempool_allocator.h"
#include "types_def.h"
#include "base_graph_node.h"
namespace maple {
class Dominance {
public:
    using NodeId = uint32;

    Dominance(MemPool &memPool, MapleVector<BaseGraphNode *> &nodeVec, BaseGraphNode &commonEntryNode,
              BaseGraphNode &commonExitNode, bool isPdom)
        : domAllocator(&memPool),
          nodeVec(nodeVec),
          commonEntryNode(commonEntryNode),
          commonExitNode(commonExitNode),
          isPdom(isPdom),
          postOrderIDVec(nodeVec.size(), -1, domAllocator.Adapter()),
          reversePostOrder(domAllocator.Adapter()),
          reversePostOrderId(domAllocator.Adapter()),
          doms(domAllocator.Adapter()),
          domFrontier(nodeVec.size(), MapleSet<NodeId>(domAllocator.Adapter()), domAllocator.Adapter()),
          domChildren(nodeVec.size(), MapleVector<NodeId>(domAllocator.Adapter()), domAllocator.Adapter()),
          iterDomFrontier(nodeVec.size(), MapleSet<NodeId>(domAllocator.Adapter()), domAllocator.Adapter()),
          dtPreOrder(nodeVec.size(), NodeId(0), domAllocator.Adapter()),
          dtDfn(nodeVec.size(), -1, domAllocator.Adapter())
    {
    }

    ~Dominance() = default;

    const MapleVector<BaseGraphNode *> &GetNodeVec() const
    {
        return nodeVec;
    }

    bool IsNodeVecEmpty() const
    {
        return nodeVec.empty();
    }

    size_t GetNodeVecSize() const
    {
        return nodeVec.size();
    }

    BaseGraphNode *GetNodeAt(size_t i) const
    {
        return nodeVec.at(i);
    }

    const BaseGraphNode &GetCommonEntryNode() const
    {
        return commonEntryNode;
    }

    const BaseGraphNode &GetCommonExitNode() const
    {
        return commonExitNode;
    }

    const BaseGraphNode *GetReversePostOrder(size_t idx) const
    {
        return reversePostOrder[idx];
    }

    const MapleVector<BaseGraphNode *> &GetReversePostOrder() const
    {
        return reversePostOrder;
    }

    MapleVector<BaseGraphNode *> &GetReversePostOrder()
    {
        return reversePostOrder;
    }

    const MapleVector<uint32> &GetReversePostOrderId()
    {
        if (reversePostOrderId.empty()) {
            reversePostOrderId.resize(nodeVec.size(), 0);
            for (size_t id = 0; id < reversePostOrder.size(); ++id) {
                reversePostOrderId[reversePostOrder[id]->GetID()] = static_cast<uint32>(id);
            }
        }
        return reversePostOrderId;
    }

    BaseGraphNode *GetDom(NodeId id)
    {
        auto it = doms.find(id);
        if (it == doms.end()) {
            return nullptr;
        }
        return it->second;
    }

    MapleSet<NodeId> &GetDomFrontier(size_t idx)
    {
        return domFrontier[idx];
    }

    const MapleSet<NodeId> &GetDomFrontier(size_t idx) const
    {
        return domFrontier.at(idx);
    }

    size_t GetDomFrontierSize() const
    {
        return domFrontier.size();
    }

    const MapleSet<NodeId> &GetIterDomFrontier(const NodeId &idx) const
    {
        return iterDomFrontier[idx];
    }

    MapleSet<NodeId> &GetIterDomFrontier(const NodeId &idx)
    {
        return iterDomFrontier[idx];
    }

    size_t GetIterDomFrontierSize() const
    {
        return iterDomFrontier.size();
    }

    const MapleVector<NodeId> &GetDomChildren(const NodeId &idx) const
    {
        return domChildren[idx];
    }

    MapleVector<NodeId> &GetDomChildren(const NodeId &idx)
    {
        return domChildren[idx];
    }

    size_t GetDomChildrenSize() const
    {
        return domChildren.size();
    }

    const NodeId &GetDtPreOrderItem(size_t idx) const
    {
        return dtPreOrder[idx];
    }

    const MapleVector<NodeId> &GetDtPreOrder() const
    {
        return dtPreOrder;
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

    void DumpDoms() const
    {
        const std::string tag = isPdom ? "pdom" : "dom";
        for (auto &node : reversePostOrder) {
            auto nodeId = node->GetID();
            LogInfo::MapleLogger() << tag << "_postorder no " << postOrderIDVec[nodeId];
            LogInfo::MapleLogger() << " is node:" << nodeId;
            LogInfo::MapleLogger() << " im_" << tag << " is node:" << doms.at(nodeId)->GetID();
            LogInfo::MapleLogger() << " " << tag << "frontier: [";
            for (auto &id : domFrontier[nodeId]) {
                LogInfo::MapleLogger() << id << " ";
            }
            LogInfo::MapleLogger() << "] iter" << tag << "Frontier: [";
            for (auto &id : iterDomFrontier[nodeId]) {
                LogInfo::MapleLogger() << id << " ";
            }
            LogInfo::MapleLogger() << "] " << tag << "children: [";
            for (auto &id : domChildren[nodeId]) {
                LogInfo::MapleLogger() << id << " ";
            }
            LogInfo::MapleLogger() << "]\n";
        }
        LogInfo::MapleLogger() << "\npreorder traversal of " << tag << " tree:";
        for (auto &id : dtPreOrder) {
            LogInfo::MapleLogger() << id << " ";
        }
        LogInfo::MapleLogger() << "\n\n";
    }

    void Init()
    {
        GenPostOrderID();
        ComputeDominance();
        ComputeDomFrontiers();
        ComputeDomChildren();
        ComputeIterDomFrontiers();
        size_t num = 0;
        ComputeDtPreorder(num);
        dtPreOrder.resize(num);
        ComputeDtDfn();
    }

    // true if b1 dominates b2
    bool Dominate(const BaseGraphNode &node1, const BaseGraphNode &node2) const
    {
        if (&node1 == &node2) {
            return true;
        }
        const auto *iDom = &node2;
        while (iDom != &commonEntryNode) {
            auto it = doms.find(iDom->GetID());
            if (it == doms.end() || it->second == nullptr) {
                return false;
            }
            iDom = it->second;
            if (iDom == &node1) {
                return true;
            }
        }
        return false;
    }

    // true if b1 dominates b2
    bool Dominate(const uint32 nodeId1, const uint32 nodeId2) const
    {
        if (nodeId1 == nodeId2) {
            return true;
        }
        uint32 curId = nodeId2;
        while (curId != commonEntryNode.GetID()) {
            auto it = doms.find(curId);
            if (it == doms.end() || it->second == nullptr) {
                return false;
            }
            curId = it->second->GetID();
            if (curId == nodeId1) {
                return true;
            }
        }
        return false;
    }

private:
    void GenPostOrderID()
    {
        DEBUG_ASSERT(!nodeVec.empty(), "size to be allocated is 0");
        std::vector<bool> visitedMap(nodeVec.size(), false);
        size_t postOrderID = 0;
        PostOrderWalk(commonEntryNode, postOrderID, visitedMap);
        // initialize reversePostOrder
        reversePostOrder.resize(postOrderID);
        CHECK_FATAL(postOrderID > 0, "must not be zero");
        auto maxPostOrderID = postOrderID - 1;
        for (size_t i = 0; i < postOrderIDVec.size(); ++i) {
            auto postOrderNo = postOrderIDVec[i];
            if (postOrderNo == -1) {
                continue;
            }
            reversePostOrder[maxPostOrderID - static_cast<uint32>(postOrderNo)] = nodeVec[i];
        }
    }

    // Figure 5 in "A Simple, Fast Dominance Algorithm" by Keith Cooper et al.
    void ComputeDomFrontiers()
    {
        constexpr uint32 kNodeVectorInitialSize = 2;
        for (const auto node : nodeVec) {
            if (node == nullptr || node == &commonExitNode) {
                continue;
            }
            std::vector<BaseGraphNode *> prevNodes;
            GetPrevNodesToVisit(*node, prevNodes);

            if (prevNodes.size() < kNodeVectorInitialSize) {
                continue;
            }
            for (auto *pre : prevNodes) {
                auto runner = pre;
                while (runner != doms.at(node->GetID()) && runner != &commonEntryNode) {
                    (void)domFrontier[runner->GetID()].insert(node->GetID());
                    runner = doms.at(runner->GetID());
                }
            }
        }
    }

    void ComputeDomChildren()
    {
        for (const auto node : reversePostOrder) {
            if (node == nullptr) {
                continue;
            }
            auto *parent = doms.at(node->GetID());
            if (parent == nullptr || parent == node) {
                continue;
            }
            domChildren[parent->GetID()].push_back(node->GetID());
        }
    }

    void ComputeIterDomFrontiers()
    {
        for (auto &node : nodeVec) {
            if (node == nullptr || node == &commonExitNode) {
                continue;
            }
            std::vector<bool> visitedMap(nodeVec.size(), false);
            GetIterDomFrontier(*node, iterDomFrontier[node->GetID()], node->GetID(), visitedMap);
        }
    }

    void ComputeDtPreorder(size_t &num)
    {
        ComputeDtPreorder(commonEntryNode, num);
    }

    void ComputeDtDfn()
    {
        for (size_t i = 0; i < dtPreOrder.size(); ++i) {
            dtDfn[dtPreOrder[i]] = static_cast<uint32>(i);
        }
    }

    // Figure 3 in "A Simple, Fast Dominance Algorithm" by Keith Cooper et al.
    void ComputeDominance()
    {
        doms[commonEntryNode.GetID()] = &commonEntryNode;
        bool changed;
        do {
            changed = false;
            for (size_t i = 1; i < reversePostOrder.size(); ++i) {
                auto node = reversePostOrder[i];
                BaseGraphNode *pre = nullptr;
                std::vector<BaseGraphNode *> prevNodes;
                GetPrevNodesToVisit(*node, prevNodes);
                auto numOfPrevNodes = prevNodes.size();
                if (InitNodeIsPred(*node) || numOfPrevNodes == 0) {
                    pre = &commonEntryNode;
                } else {
                    pre = prevNodes[0];
                }
                size_t j = 1;
                while ((doms[pre->GetID()] == nullptr || pre == node) && j < numOfPrevNodes) {
                    pre = prevNodes[j];
                    ++j;
                }
                BaseGraphNode *newIDom = pre;
                for (; j < numOfPrevNodes; ++j) {
                    pre = prevNodes[j];
                    if (doms[pre->GetID()] != nullptr && pre != node) {
                        newIDom = Intersect(*pre, *newIDom);
                    }
                }
                if (doms[node->GetID()] != newIDom) {
                    doms[node->GetID()] = newIDom;
                    changed = true;
                }
            }
        } while (changed);
    }

    BaseGraphNode *Intersect(BaseGraphNode &node1, const BaseGraphNode &node2) const
    {
        auto *ptrNode1 = &node1;
        auto *ptrNode2 = &node2;
        while (ptrNode1 != ptrNode2) {
            while (postOrderIDVec[ptrNode1->GetID()] < postOrderIDVec[ptrNode2->GetID()]) {
                ptrNode1 = doms.at(ptrNode1->GetID());
            }
            while (postOrderIDVec[ptrNode2->GetID()] < postOrderIDVec[ptrNode1->GetID()]) {
                ptrNode2 = doms.at(ptrNode2->GetID());
            }
        }
        return ptrNode1;
    }

    bool InitNodeIsPred(const BaseGraphNode &node) const
    {
        std::vector<BaseGraphNode *> succNodes;
        GetNextNodesToVisit(commonEntryNode, succNodes);
        for (const auto &suc : succNodes) {
            if (suc == &node) {
                return true;
            }
        }
        return false;
    }

    void PostOrderWalk(const BaseGraphNode &root, size_t &pid, std::vector<bool> &visitedMap)
    {
        std::stack<const BaseGraphNode *> s;
        s.push(&root);
        visitedMap[root.GetID()] = true;
        while (!s.empty()) {
            auto node = s.top();
            auto nodeId = node->GetID();
            if (nodeVec[nodeId] == nullptr) {
                s.pop();
                continue;
            }
            DEBUG_ASSERT(nodeId < visitedMap.size() && nodeId < postOrderIDVec.size(), "index out of range");
            bool tail = true;
            std::vector<BaseGraphNode *> succNodes;
            GetNextNodesToVisit(*node, succNodes);
            for (auto succ : succNodes) {
                if (!visitedMap[succ->GetID()]) {
                    tail = false;
                    visitedMap[succ->GetID()] = true;
                    s.push(succ);
                    break;
                }
            }
            if (tail) {
                s.pop();
                postOrderIDVec[nodeId] = static_cast<int32>(pid++);
            }
        }
    }

    void ComputeDtPreorder(const BaseGraphNode &node, size_t &num)
    {
        CHECK_FATAL(num < dtPreOrder.size(), "index out of range");
        dtPreOrder[num++] = node.GetID();
        for (auto &k : domChildren[node.GetID()]) {
            ComputeDtPreorder(*nodeVec[k], num);
        }
    }

    void GetNextNodesToVisit(const BaseGraphNode &node, std::vector<BaseGraphNode *> &nodesToVisit) const
    {
        if (isPdom) {
            node.GetInNodes(nodesToVisit);
        } else {
            node.GetOutNodes(nodesToVisit);
        }
    }

    void GetPrevNodesToVisit(const BaseGraphNode &node, std::vector<BaseGraphNode *> &nodesToVisit) const
    {
        if (isPdom) {
            node.GetOutNodes(nodesToVisit);
        } else {
            node.GetInNodes(nodesToVisit);
        }
    }

    // nodeIdMarker indicates that the iterDomFrontier results for nodeId < nodeIdMarker have been computed
    void GetIterDomFrontier(const BaseGraphNode &node, MapleSet<NodeId> &dfSet, const NodeId &nodeIdMarker,
                            std::vector<bool> &visitedMap)
    {
        if (visitedMap[node.GetID()]) {
            return;
        }
        visitedMap[node.GetID()] = true;
        for (auto frontierNodeId : domFrontier[node.GetID()]) {
            (void)dfSet.insert(frontierNodeId);
            if (frontierNodeId < nodeIdMarker) {  // union with its computed result
                dfSet.insert(iterDomFrontier[frontierNodeId].cbegin(), iterDomFrontier[frontierNodeId].cend());
            } else {  // recursive call
                auto frontierNode = nodeVec[frontierNodeId];
                if (frontierNode == nullptr) {
                    continue;
                }
                GetIterDomFrontier(*frontierNode, dfSet, nodeIdMarker, visitedMap);
            }
        }
    }

    MapleAllocator domAllocator;  // stores the analysis results
    MapleVector<BaseGraphNode *> &nodeVec;
    BaseGraphNode &commonEntryNode;
    BaseGraphNode &commonExitNode;
    bool isPdom;
    MapleVector<int32> postOrderIDVec;                // index is node id
    MapleVector<BaseGraphNode *> reversePostOrder;    // an ordering of the node in reverse postorder
    MapleVector<uint32> reversePostOrderId;           // gives position of each node in reversePostOrder
    MapleUnorderedMap<NodeId, BaseGraphNode *> doms;  // index is node id; immediate dominator for each node
    MapleVector<MapleSet<NodeId>> domFrontier;        // index is node id
    MapleVector<MapleVector<NodeId>> domChildren;     // index is node id; for dom tree
    MapleVector<MapleSet<NodeId>> iterDomFrontier;    // index is node id
    MapleVector<NodeId> dtPreOrder;  // ordering of the nodes in a preorder traversal of the dominator tree
    MapleVector<uint32> dtDfn;       // gives position of each node in dt_preorder
};
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_DOMINANCE_H
