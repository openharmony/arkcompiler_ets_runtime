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

#ifndef ECMASCRIPT_DFX_HPROF_HYBRID_HYBRID_HEAP_SNAPSHOT_H
#define ECMASCRIPT_DFX_HPROF_HYBRID_HYBRID_HEAP_SNAPSHOT_H

#include <unordered_map>

#include "ecmascript/dfx/hprof/heap_snapshot.h"
#include "hybrid/hybrid_heap_snapshot_info.h"
#include "hybrid/sts_vm_interface.h"

namespace panda::ecmascript {

class HybridHeapSnapshot : public HeapSnapshot {
public:
    HybridHeapSnapshot(EcmaVM *vm, arkplatform::STSVMInterface *stsInterface,
                       EntryIdMap *entryIdMap, StringHashMap *stringTable, bool isSimplify,
                       bool dumpDynamicHeap, bool dumpStaticHeap, NativeAreaAllocator *allocator);
    ~HybridHeapSnapshot() override = default;

    NO_COPY_SEMANTIC(HybridHeapSnapshot);
    NO_MOVE_SEMANTIC(HybridHeapSnapshot);

    bool BuildHybridSnapshot();
    CString *GetString(const char *str);

    static NodeType MapStaticNodeType(arkplatform::StaticNodeType type);
    static EdgeType MapStaticEdgeType(arkplatform::StaticEdgeType type);

private:
    static constexpr size_t DYNAMIC_SPECIFIC_ROOTS_COUNT = 4;
    static constexpr size_t SYNTHETIC_ROOT_INDEX = 0;
    static constexpr size_t LOCAL_HANDLE_ROOT_INDEX = 1;
    static constexpr size_t GLOBAL_HANDLE_ROOT_INDEX = 2;
    static constexpr size_t VM_ROOT_INDEX = 3;
    static constexpr size_t FRAME_ROOT_INDEX = 4;

    size_t GetActualRootStartIndex() const
    {
        return 1 + (dumpDynamicHeap_ ? DYNAMIC_SPECIFIC_ROOTS_COUNT : 0);
    }

    void CreateSyntheticRootNode();
    void CreateSpecificSyntheticRootNodes();
    void CreateSyntheticRootEdges();
    void DumpReferences();
    void CreateStaticRootNodes();
    void CreateDynamicRootNodes();
    void DumpRefForDynamicNode(HprofNode *node);
    void DumpRefForStaticNode(HprofNode *node);
    void DumpXRefDynamicToStatic(HprofNode *entryFrom);
    void DumpXRefStaticToDynamic(HprofNode *entryFrom);
    void CreateEdgeToSpecificSyntheticRoot(HprofNode *syntheticRoot, HprofNode *specificSyntheticRoot);
    void CreateSpecificSyntheticRootEdges();
    void InsertNativeAddressNodes();
    void CollectXRefMaps();
    void CreateEdgesToSpecificRoot(HprofNode *specificSyntheticRoot,
                                   const CUnorderedSet<HprofNode *> &rootSet);

    HprofNode *CreateStaticNode(const arkplatform::NodeInfo &nodeInfo);
    HprofNode *GetOrCreateStaticNode(uint64_t addr);

    arkplatform::STSVMInterface *stsInterface_ {nullptr};
    bool dumpDynamicHeap_ {false};
    bool dumpStaticHeap_ {false};
    bool isSimplify_ {false};

    // xref maps (populated in BuildUp, used in FillEdges)
    std::unordered_map<uint64_t, uint64_t> jsToEts_;
    std::unordered_map<uint64_t, uint64_t> etsToJs_;
};

}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_DFX_HPROF_HYBRID_HYBRID_HEAP_SNAPSHOT_H
