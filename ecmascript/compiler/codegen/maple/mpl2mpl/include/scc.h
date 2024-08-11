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

#ifndef MPL2MPL_INCLUDE_SCC_H
#define MPL2MPL_INCLUDE_SCC_H
#include "base_graph_node.h"
namespace maple {
class BaseGraphNode;

constexpr uint32 kShiftSccUniqueIDNum = 16;

// Note that T is the type of the graph node.
template <typename T>
class SCCNode {
public:
    SCCNode(uint32 index, MapleAllocator &alloc)
        : id(index), nodes(alloc.Adapter()), inScc(alloc.Adapter()), outScc(alloc.Adapter())
    {
    }

    ~SCCNode() = default;

    void AddNode(T *node)
    {
        nodes.push_back(node);
    }

    void Dump() const
    {
        LogInfo::MapleLogger() << "SCC " << id << " contains " << nodes.size() << " node(s)\n";
        for (auto const kIt : nodes) {
            T *node = kIt;
            LogInfo::MapleLogger() << "  " << node->GetIdentity() << "\n";
        }
    }

    void DumpCycle() const
    {
        T *currNode = nodes[0];
        std::vector<T *> searched;
        searched.push_back(currNode);
        std::vector<T *> invalidNodes;
        std::vector<BaseGraphNode *> outNodes;
        while (true) {
            bool findNewOut = false;
            currNode->GetOutNodes(outNodes);
            for (auto outIt : outNodes) {
                auto outNode = static_cast<T *>(outIt);
                if (outNode->GetSCCNode() == this) {
                    size_t j = 0;
                    for (; j < invalidNodes.size(); ++j) {
                        if (invalidNodes[j] == outNode) {
                            break;
                        }
                    }
                    // Find a invalid node
                    if (j < invalidNodes.size()) {
                        continue;
                    }
                    for (j = 0; j < searched.size(); ++j) {
                        if (searched[j] == outNode) {
                            break;
                        }
                    }
                    if (j == searched.size()) {
                        currNode = outNode;
                        searched.push_back(currNode);
                        findNewOut = true;
                        break;
                    }
                }
            }
            outNodes.clear();
            if (searched.size() == nodes.size()) {
                break;
            }
            if (!findNewOut) {
                invalidNodes.push_back(searched[searched.size() - 1]);
                searched.pop_back();
                currNode = searched[searched.size() - 1];
            }
        }
        for (auto it = searched.begin(); it != searched.end(); ++it) {
            LogInfo::MapleLogger() << (*it)->GetIdentity() << '\n';
        }
    }

    void Verify() const
    {
        CHECK_FATAL(!nodes.empty(), "the size of nodes less than zero");
        for (T *const &node : nodes) {
            if (node->GetSCCNode() != this) {
                CHECK_FATAL(false, "must equal this");
            }
        }
    }

    void Setup()
    {
        std::vector<BaseGraphNode *> outNodes;
        std::vector<BaseGraphNode *> inNodes;
        for (T *const &node : nodes) {
            node->GetOutNodes(outNodes);
            for (auto outIt : outNodes) {
                auto outNode = static_cast<T *>(outIt);
                if (outNode == nullptr) {
                    continue;
                }
                auto outNodeScc = outNode->GetSCCNode();
                if (outNodeScc == this) {
                    continue;
                }
                outScc.insert(outNodeScc);
                outNodeScc->inScc.insert(this);
            }
            outNodes.clear();
        }
    }

    const MapleVector<T *> &GetNodes() const
    {
        return nodes;
    }

    MapleVector<T *> &GetNodes()
    {
        return nodes;
    }

    const MapleSet<SCCNode<T> *, Comparator<SCCNode<T>>> &GetOutScc() const
    {
        return outScc;
    }

    const MapleSet<SCCNode<T> *, Comparator<SCCNode<T>>> &GetInScc() const
    {
        return inScc;
    }

    void RemoveInScc(SCCNode<T> *const sccNode)
    {
        inScc.erase(sccNode);
    }

    bool HasRecursion() const
    {
        if (nodes.empty()) {
            return false;
        }
        if (nodes.size() > 1) {
            return true;
        }
        T *node = nodes[0];
        std::vector<BaseGraphNode *> outNodes;
        node->GetOutNodes(outNodes);
        for (auto outIt : outNodes) {
            auto outNode = static_cast<T *>(outIt);
            if (outNode == nullptr) {
                continue;
            }
            if (node == outNode) {
                return true;
            }
        }
        return false;
    }

    bool HasSelfRecursion() const
    {
        if (nodes.size() != 1) {
            return false;
        }
        T *node = nodes[0];
        std::vector<BaseGraphNode *> outNodes;
        node->GetOutNodes(outNodes);
        for (auto outIt : outNodes) {
            auto outNode = static_cast<T *>(outIt);
            if (outNode == nullptr) {
                continue;
            }
            if (node == outNode) {
                return true;
            }
        }
        return false;
    }

    bool HasInScc() const
    {
        return (!inScc.empty());
    }

    uint32 GetID() const
    {
        return id;
    }

    uint32 GetUniqueID() const
    {
        return GetID() << maple::kShiftSccUniqueIDNum;
    }

private:
    uint32 id;
    MapleVector<T *> nodes;
    MapleSet<SCCNode<T> *, Comparator<SCCNode<T>>> inScc;
    MapleSet<SCCNode<T> *, Comparator<SCCNode<T>>> outScc;
};

template <typename T>
void BuildSCCDFS(T &rootNode, uint32 &visitIndex, MapleVector<SCCNode<T> *> &sccNodes, std::vector<T *> &nodes,
                 std::vector<uint32> &visitedOrder, std::vector<uint32> &lowestOrder, std::vector<bool> &inStack,
                 std::vector<uint32> &visitStack, uint32 &numOfSccs, MapleAllocator &cgAlloc)
{
    uint32 id = rootNode.GetID();
    nodes.at(id) = &rootNode;
    visitedOrder.at(id) = visitIndex;
    lowestOrder.at(id) = visitIndex;
    ++visitIndex;
    inStack.at(id) = true;

    std::vector<BaseGraphNode *> outNodes;
    rootNode.GetOutNodes(outNodes);
    for (auto outIt : outNodes) {
        auto outNode = static_cast<T *>(outIt);
        if (outNode == nullptr) {
            continue;
        }
        uint32 outNodeId = outNode->GetID();
        if (visitedOrder.at(outNodeId) == 0) {
            // callee has not been processed yet
            BuildSCCDFS(*outNode, visitIndex, sccNodes, nodes, visitedOrder, lowestOrder, inStack, visitStack,
                        numOfSccs, cgAlloc);
            if (lowestOrder.at(outNodeId) < lowestOrder.at(id)) {
                lowestOrder.at(id) = lowestOrder.at(outNodeId);
            }
        } else if (inStack.at(outNodeId) && (visitedOrder.at(outNodeId) < lowestOrder.at(id))) {
            // back edge
            lowestOrder.at(id) = visitedOrder.at(outNodeId);
        }
    }

    if (visitedOrder.at(id) == lowestOrder.at(id)) {
        SCCNode<T> *sccNode = cgAlloc.GetMemPool()->New<SCCNode<T>>(numOfSccs++, cgAlloc);
        inStack.at(id) = false;
        T *rootNode = nodes.at(id);
        rootNode->SetSCCNode(sccNode);
        sccNode->AddNode(rootNode);
        while (!visitStack.empty()) {
            auto stackTopId = visitStack.back();
            if (visitedOrder.at(stackTopId) < visitedOrder.at(id)) {
                break;
            }
            visitStack.pop_back();
            inStack.at(stackTopId) = false;
            T *topNode = nodes.at(stackTopId);
            topNode->SetSCCNode(sccNode);
            sccNode->AddNode(topNode);
        }
        sccNodes.push_back(sccNode);
    } else {
        visitStack.push_back(id);
    }
}

template <typename T>
void VerifySCC(std::vector<T *> nodes)
{
    for (auto node : nodes) {
        if (node->GetSCCNode() == nullptr) {
            CHECK_FATAL(false, "nullptr check in VerifySCC()");
        }
    }
}

template <typename T>
uint32 BuildSCC(MapleAllocator &cgAlloc, uint32 numOfNodes, std::vector<T *> &allNodes, bool debugScc,
                MapleVector<SCCNode<T> *> &topologicalVec, bool clearOld = false)
{
    // This is the mapping between cg_id to node.
    std::vector<T *> id2NodeMap(numOfNodes, nullptr);
    std::vector<uint32> visitedOrder(numOfNodes, 0);
    std::vector<uint32> lowestOrder(numOfNodes, 0);
    std::vector<bool> inStack(numOfNodes, false);
    std::vector<uint32> visitStack;
    uint32 visitIndex = 1;
    uint32 numOfSccs = 0;
    if (clearOld) {
        // clear old scc before computing
        for (auto node : allNodes) {
            node->SetSCCNode(nullptr);
        }
    }
    // However, not all SCC can be reached from roots.
    // E.g. foo()->foo(), foo is not considered as a root.
    for (auto node : allNodes) {
        if (node->GetSCCNode() == nullptr) {
            BuildSCCDFS(*node, visitIndex, topologicalVec, id2NodeMap, visitedOrder, lowestOrder, inStack, visitStack,
                        numOfSccs, cgAlloc);
        }
    }
    for (auto scc : topologicalVec) {
        scc->Verify();
        scc->Setup();  // fix caller and callee info.
        if (debugScc && scc->HasRecursion()) {
            scc->Dump();
        }
    }
    std::reverse(topologicalVec.begin(), topologicalVec.end());
    return numOfSccs;
}
}  // namespace maple
#endif  // MPL2MPL_INCLUDE_SCC_H