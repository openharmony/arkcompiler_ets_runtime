/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "snapshot_merger.h"

#include <unordered_set>

namespace rawheap_translate {

// ---- Merge (main entry point) ----

bool SnapshotMerger::Merge(RawHeap &dynamic, StaticRawheapTranslate &sp)
{
    strIdMap_.clear();
    duplicateNodeIds_ = 0;
    xrefUnresolved_ = 0;
    auto *targetNodes = dynamic.GetNodes();
    if (targetNodes->empty()) {
        LOG_ERROR_ << "Merge: dynamic graph is empty";
        return false;
    }

    auto bucket = BucketExistingEdges(dynamic);
    auto dynamicNodeIndex = BuildDynNodeIndex(dynamic);
    AppendStaticNodesAndEdges(dynamic, sp, bucket);
    WireStaticRoot(dynamic, sp, bucket);
    bool xrefValid = SpliceXRefEdges(dynamic, sp, dynamicNodeIndex, bucket);
    FlattenAndRenumber(dynamic, bucket);

    if (!xrefValid) {
        return false;
    }

    LOG_INFO_ << "Merge completed: nodes=" << dynamic.GetNodeCount() << " edges=" << dynamic.GetEdgeCount();
    if (duplicateNodeIds_ > 0 || xrefUnresolved_ > 0) {
        LOG_INFO_ << "Merge warnings: duplicateNodeIds=" << duplicateNodeIds_ << " xrefUnresolved=" << xrefUnresolved_;
    }
    return true;
}

// ---- Phase 1 ----

SnapshotMerger::EdgeBucket SnapshotMerger::BucketExistingEdges(RawHeap &dynamic)
{
    auto *targetNodes = dynamic.GetNodes();
    auto *targetEdges = dynamic.GetEdges();
    EdgeBucket bucket;
    size_t pos = 0;
    for (Node *node : *targetNodes) {
        auto &list = bucket[node];
        uint32_t cnt = node->edgeCount;
        for (uint32_t i = 0; i < cnt && pos < targetEdges->size(); ++i, ++pos) {
            list.push_back((*targetEdges)[pos]);
        }
    }
    return bucket;
}

// ---- Phase 2 ----

void SnapshotMerger::AppendStaticNodesAndEdges(RawHeap &dynamic, StaticRawheapTranslate &sp, EdgeBucket &bucket)
{
    auto *targetNodes = dynamic.GetNodes();
    auto *staNodes = sp.GetNodes();
    auto *staEdges = sp.GetEdges();

    size_t pos = 0;
    for (Node *node : *staNodes) {
        // Remap the node's own type-name string id into the target pool.
        node->strId = RemapStrId(dynamic, sp, node->strId);
        targetNodes->push_back(node);

        auto &list = bucket[node];
        uint32_t cnt = node->edgeCount;
        for (uint32_t i = 0; i < cnt && pos < staEdges->size(); ++i, ++pos) {
            Edge *edge = (*staEdges)[pos];
            if (EdgeUsesStringId(edge->type)) {
                edge->nameOrIndex = RemapStrId(dynamic, sp, edge->nameOrIndex);
            }
            list.push_back(edge);
        }
    }
    // Static edges/nodes are now owned by the target; detach from the static
    // parser so its destructor does not double-free them.
    staNodes->clear();
    staEdges->clear();
}

// ---- Phase 3 ----

void SnapshotMerger::WireStaticRoot(RawHeap &dynamic, StaticRawheapTranslate &sp, EdgeBucket &bucket)
{
    auto *targetNodes = dynamic.GetNodes();
    // The dynamic synthetic root is nodes_[0] (built by V1/V2 Translate).
    Node *syntheticRoot = (*targetNodes)[0];

    Node *staticRoot = dynamic.CreateNode();
    dynamic.CreateRootNode(staticRoot, "StaticRoot", sp.GetRoots().size());
    // Note: CreateNode already pushed staticRoot into targetNodes
    // (dynamic.nodes_), so no second push_back is needed.

    StringId subrootStrId = dynamic.InsertAndGetStringId("-subroot-");
    bucket[syntheticRoot].push_back(new Edge(staticRoot, subrootStrId, EdgeType::SHORTCUT));
    syntheticRoot->edgeCount++;

    uint32_t index = 0;
    uint32_t skipped = 0;
    for (uint32_t nodeId : sp.GetRoots()) {
        Node *root = sp.FindNodeByNodeId(nodeId);
        if (root == nullptr) {
            skipped++;
            continue;
        }
        bucket[staticRoot].push_back(new Edge(root, index++, EdgeType::ELEMENT));
        staticRoot->edgeCount++;
    }
    if (skipped > 0) {
        LOG_INFO_ << "WireStaticRoot: " << skipped << " root nodeIds had no matching node";
    }
}

// ---- Phase 4 ----

bool SnapshotMerger::SpliceXRefEdges(RawHeap &dynamic, StaticRawheapTranslate &sp, const NodeIndex &dynamicNodeIndex,
                                     EdgeBucket &bucket)
{
    XRefEdgeStrings strs = {dynamic.InsertAndGetStringId("xref_dyn_sta"), dynamic.InsertAndGetStringId("xref_sta_dyn"),
                            dynamic.InsertAndGetStringId("xref_bidir")};
    uint32_t resolved = 0;
    const auto &xrefs = sp.GetXRefs();
    for (const auto &x : xrefs) {
        // dynNodeId matches a dynamic node's nodeId (see class doc for rationale).
        auto it = dynamicNodeIndex.find(x.dynNodeId);
        Node *dynNode = (it != dynamicNodeIndex.end()) ? it->second : nullptr;
        Node *staNode = sp.FindNodeByNodeId(x.staNodeId);
        if (dynNode == nullptr || staNode == nullptr) {
            ++xrefUnresolved_;
            LOG_INFO_ << "SpliceXRefEdges: skipped dynNodeId=" << x.dynNodeId << " staNodeId=" << x.staNodeId
                      << " dir=" << static_cast<int>(x.direction);
            continue;
        }
        EmitXRefEdge(x.direction, dynNode, staNode, strs, bucket);
        ++resolved;
    }
    return ValidateXRefResolution(xrefs, resolved);
}

SnapshotMerger::NodeIndex SnapshotMerger::BuildDynNodeIndex(RawHeap &dynamic) const
{
    // Index the dynamic side by nodeId so dynNodeId resolves directly (see class
    // doc).
    std::unordered_map<uint64_t, Node *> dynNodeIndex;
    auto *dynNodes = dynamic.GetNodes();
    for (Node *node : *dynNodes) {
        if (node->nodeId != 0) {
            dynNodeIndex[node->nodeId] = node;
        }
    }
    return dynNodeIndex;
}

void SnapshotMerger::EmitXRefEdge(uint8_t direction, Node *dynNode, Node *staNode, const XRefEdgeStrings &strs,
                                  EdgeBucket &bucket)
{
    switch (direction) {
        case XREF_DYN_TO_STA:
            bucket[dynNode].push_back(new Edge(staNode, strs.dynToSta, EdgeType::XREF));
            dynNode->edgeCount++;
            break;
        case XREF_STA_TO_DYN:
            bucket[staNode].push_back(new Edge(dynNode, strs.staToDyn, EdgeType::XREF));
            staNode->edgeCount++;
            break;
        case XREF_BIDIR:
            bucket[dynNode].push_back(new Edge(staNode, strs.bidir, EdgeType::XREF));
            dynNode->edgeCount++;
            bucket[staNode].push_back(new Edge(dynNode, strs.bidir, EdgeType::XREF));
            staNode->edgeCount++;
            break;
        default:
            break;
    }
}

bool SnapshotMerger::ValidateXRefResolution(const std::vector<StaticRawheapTranslate::XRefRecord> &xrefs,
                                            uint32_t resolved) const
{
    if (xrefs.empty()) {
        return true;
    }
    if (resolved == 0) {
        LOG_ERROR_ << "SpliceXRefEdges: 0/" << xrefs.size()
                   << " XRef records resolved - dynNodeId<->nodeId convention may not hold"
                   << " (unresolved=" << xrefUnresolved_ << ")";
        return false;
    }
    if (xrefUnresolved_ > 0) {
        LOG_INFO_ << "SpliceXRefEdges: " << xrefUnresolved_ << "/" << xrefs.size() << " XRef records unresolved";
    }
    return true;
}

// ---- Phase 5 ----

void SnapshotMerger::FlattenAndRenumber(RawHeap &dynamic, EdgeBucket &bucket)
{
    auto *targetNodes = dynamic.GetNodes();
    auto *targetEdges = dynamic.GetEdges();

    targetEdges->clear();
    for (Node *node : *targetNodes) {
        auto it = bucket.find(node);
        uint32_t cnt = (it == bucket.end()) ? 0 : static_cast<uint32_t>(it->second.size());
        node->edgeCount = cnt;
        if (it != bucket.end()) {
            for (Edge *edge : it->second) {
                targetEdges->push_back(edge);
            }
        }
    }
    for (uint32_t i = 0; i < targetNodes->size(); ++i) {
        (*targetNodes)[i]->index = i;
    }
    DetectDuplicateNodeIds(*targetNodes);
}

void SnapshotMerger::DetectDuplicateNodeIds(std::vector<Node *> &nodes)
{
    // Duplicate-nodeId detection. The .heapsnapshot format addresses edges by
    // node index, not nodeId, so duplicates do not corrupt serialization - but
    // they break the XRef staNodeId<->nodeId convention and any tool that looks
    // nodes up by nodeId. Warn only; do not remap (preserves address semantics).
    std::unordered_set<uint64_t> seen;
    for (Node *node : nodes) {
        uint64_t id = node->nodeId;
        if (id == 0) {
            continue;  // 0 is the synthetic root / placeholder, not a real id
        }
        if (!seen.insert(id).second) {
            duplicateNodeIds_++;
            LOG_ERROR_ << "FlattenAndRenumber: duplicate nodeId 0x" << std::hex << id << std::dec
                       << " - XRef resolution by nodeId may be ambiguous";
        }
    }
    if (duplicateNodeIds_ > 0) {
        LOG_ERROR_ << "FlattenAndRenumber: " << duplicateNodeIds_ << " duplicate nodeIds detected";
    }
}

// ---- String-id remapping helpers ----

StringId SnapshotMerger::RemapStrId(RawHeap &target, StaticRawheapTranslate &sp, StringId staticStrId)
{
    // StringIds in the StringHashMap start from CUSTOM_STRID_START (3).
    // A value < 3 means the node/edge was never assigned a real string id
    // (e.g., a Node whose classAddr was not found in classMap_ during BuildGraph,
    //  leaving strId at its default value). Treat it as empty string.
    if (staticStrId < StringHashMap::CUSTOM_STRID_START) {
        return target.InsertAndGetStringId("");
    }
    auto cached = strIdMap_.find(staticStrId);
    if (cached != strIdMap_.end()) {
        return cached->second;
    }
    auto *table = sp.GetStringTable();
    StringKey key = table->GetKeyByStringId(staticStrId);
    // GetKeyByStringId returns 0 for out-of-range ids (bounds-checked
    // internally).
    if (key == 0) {
        LOG_INFO_ << "RemapStrId: staticStrId=" << staticStrId << " out of range (capacity=" << table->GetCapacity()
                  << "), falling back to empty string";
        return target.InsertAndGetStringId("");
    }
    std::string content = table->GetStringByKey(key);
    // Re-insert by content into the target (dynamic) pool - content-dedup.
    StringId newId = target.InsertAndGetStringId(content);
    strIdMap_[staticStrId] = newId;
    return newId;
}

bool SnapshotMerger::EdgeUsesStringId(EdgeType type)
{
    return type == EdgeType::CONTEXT || type == EdgeType::PROPERTY || type == EdgeType::INTERNAL ||
           type == EdgeType::SHORTCUT || type == EdgeType::WEAK || type == EdgeType::HIDDEN || type == EdgeType::XREF;
}

}  // namespace rawheap_translate
