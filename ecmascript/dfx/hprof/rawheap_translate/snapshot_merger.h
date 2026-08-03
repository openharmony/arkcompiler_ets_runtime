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
 * WITHOUT WARRANTIES OR CONDITIONS of ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#ifndef RAWHEAP_TRANSLATE_SNAPSHOT_MERGER_H
#define RAWHEAP_TRANSLATE_SNAPSHOT_MERGER_H

#include "rawheap_translate.h"
#include "static_rawheap_translate.h"
#include <unordered_map>
#include <vector>

namespace rawheap_translate {

/**
 * @brief Merge a static-side StaticRawheapTranslate graph into a dynamic-side
 * RawHeap graph, producing one combined .heapsnapshot.
 *
 * The dynamic side is parsed+translated by the existing V1/V2 machinery
 * (dynamic parsing unchanged), which builds a complete graph including its own
 * synthetic root. The static side is parsed (object nodes + edges only, no
 * synthetic root) by StaticRawheapTranslate; the writer/reader pair and shared
 * wire format live in
 * runtime_core/static_core/runtime/tooling/hprof/static_dump.h,
 * rawheap_translate/common.h, and
 * runtime_core/static_core/plugins/ets/runtime/tooling/hprof/session/dump_format.h.
 *
 * String pools are merged by CONTENT, not by shared id: the two
 * virtual machines do not actually share a StringIdPool, so equal strings are
 * deduplicated and static string ids are remapped into the dynamic (target)
 * string table.
 *
 * XRef (cross-virtual-machine reference) records use explicit field semantics
 * instead of generic from/to addresses: each record carries
 * [dynNodeId(u32)][staNodeId(u32)][direction(u8)]. Both endpoints are 4-byte
 * nodeIds - the dump side resolves jsAddr->dynNodeId via the dynamic
 * participant's GetNodeId (mirroring etsAddr->staNodeId via ObjectIdMap) - so
 * the merger resolves both sides by nodeId->Node lookup. This eliminates the
 * need for a unified address index across both virtual machines.
 *
 * The .heapsnapshot format assigns edges to nodes sequentially (an object's
 * edges are the next `edgeCount` edges after the previous object's), so the
 * merged edge vector must be grouped by source node in node order. We rebuild
 * the edge vector: slice each side's edges by source node (using edgeCount),
 * bucket new edges (StaticRoot wiring + XRef) onto their source node, then
 * flatten in node order.
 *
 * Merge is a 5-phase process:
 *   Phase 1 - Bucket existing dynamic edges by source node (using edgeCount).
 *   Phase 2 - Append static nodes/edges with strId remapping, transfer
 * ownership. Phase 3 - Add a StaticRoot group node under the dynamic synthetic
 * root. Phase 4 - Splice XRef edges (cross-virtual-machine references from the
 * static file). Phase 5 - Flatten edges in node order; renumber node indices.
 */
class SnapshotMerger {
public:
    /**
     * @brief Merge `staticParser` into `dynamic` (in place).
     * @return false when the dynamic graph is empty or a non-empty XRef set
     *         cannot resolve any record; true otherwise.
     */
    bool Merge(RawHeap &dynamic, StaticRawheapTranslate &staticParser);

private:
    using EdgeBucket = std::unordered_map<Node *, std::vector<Edge *>>;
    using NodeIndex = std::unordered_map<uint64_t, Node *>;

    // Phase 1: bucket existing dynamic edges by source node (slice by edgeCount).
    EdgeBucket BucketExistingEdges(RawHeap &dynamic);

    // Phase 2: append static nodes + edges, remap strIds into target pool.
    // After this call, staticParser's node/edge vectors are empty (ownership
    // transferred to the dynamic graph).
    void AppendStaticNodesAndEdges(RawHeap &dynamic, StaticRawheapTranslate &sp, EdgeBucket &bucket);

    // Phase 3: add a StaticRoot group node under the dynamic synthetic root.
    void WireStaticRoot(RawHeap &dynamic, StaticRawheapTranslate &sp, EdgeBucket &bucket);

    // Phase 4: splice XRef edges. Both endpoints are nodeIds resolved by
    // nodeId->Node lookup (see class doc for the full rationale).
    bool SpliceXRefEdges(RawHeap &dynamic, StaticRawheapTranslate &sp, const NodeIndex &dynamicNodeIndex,
                         EdgeBucket &bucket);

    // Phase 4 helpers (kept next to SpliceXRefEdges so decl/impl order match).
    // The three XRef edge-name string ids, inserted into the target pool once.
    struct XRefEdgeStrings {
        StringId dynToSta;
        StringId staToDyn;
        StringId bidir;
    };
    // Build a dynNodeId->Node index over the dynamic side only (nodeId != 0).
    NodeIndex BuildDynNodeIndex(RawHeap &dynamic) const;
    // Emit one resolved XRef edge (or two, for bidirectional) into the bucket.
    void EmitXRefEdge(uint8_t direction, Node *dynNode, Node *staNode, const XRefEdgeStrings &strs, EdgeBucket &bucket);
    // Validate and log the resolve-rate summary. Empty XRef input and partial
    // resolution are valid; a non-empty set with zero resolved records is not.
    bool ValidateXRefResolution(const std::vector<StaticRawheapTranslate::XRefRecord> &xrefs, uint32_t resolved) const;

    // Phase 5: flatten edges in node order; renumber node indices.
    void FlattenAndRenumber(RawHeap &dynamic, EdgeBucket &bucket);
    // Phase 5 helper: warn on duplicate nodeIds (breaks XRef nodeId lookup).
    void DetectDuplicateNodeIds(std::vector<Node *> &nodes);

    // Remap a static-side StringId into the target (dynamic) string table,
    // inserting the string content if new. Cached in strIdMap_.
    StringId RemapStrId(RawHeap &target, StaticRawheapTranslate &sp, StringId staticStrId);

    // ELEMENT edges carry a numeric index in nameOrIndex. All other supported
    // edge types, including INTERNAL edges such as "superClass", carry a string
    // id.
    static bool EdgeUsesStringId(EdgeType type);

    std::unordered_map<uint32_t, uint32_t> strIdMap_;

    // Diagnostic counters (detection + warning only, no remap).
    uint32_t duplicateNodeIds_ {0};  // nodeIds seen more than once in the merged graph
    uint32_t xrefUnresolved_ {0};    // XRef records that failed to resolve either side
};

}  // namespace rawheap_translate
#endif  // RAWHEAP_TRANSLATE_SNAPSHOT_MERGER_H
