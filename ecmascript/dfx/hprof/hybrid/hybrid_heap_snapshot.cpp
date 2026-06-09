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

#include "ecmascript/dfx/hprof/hybrid/hybrid_heap_snapshot.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/object_xray.h"

namespace panda::ecmascript {
CString *HybridHeapSnapshot::GetString(const char *str)
{
    return stringTable_->GetString(str);
}

NodeType HybridHeapSnapshot::MapStaticNodeType(arkplatform::StaticNodeType type)
{
    switch (type) {
        case arkplatform::StaticNodeType::ARRAY:
            return NodeType::ARRAY;
        case arkplatform::StaticNodeType::STRING:
            return NodeType::STRING;
        case arkplatform::StaticNodeType::OBJECT:
            return NodeType::OBJECT;
        case arkplatform::StaticNodeType::CLASS:
            return NodeType::CLASS;
        default:
            return NodeType::DEFAULT;
    }
}

EdgeType HybridHeapSnapshot::MapStaticEdgeType(arkplatform::StaticEdgeType type)
{
    switch (type) {
        case arkplatform::StaticEdgeType::ELEMENT:
            return EdgeType::ELEMENT;
        case arkplatform::StaticEdgeType::PROPERTY:
            return EdgeType::PROPERTY;
        case arkplatform::StaticEdgeType::WEAK:
            return EdgeType::WEAK;
        default:
            return EdgeType::DEFAULT;
    }
}

HybridHeapSnapshot::HybridHeapSnapshot(EcmaVM *vm, arkplatform::STSVMInterface *stsInterface,
                                       EntryIdMap *entryIdMap, StringHashMap *stringTable, bool isSimplify,
                                       bool dumpDynamicHeap, bool dumpStaticHeap, NativeAreaAllocator *allocator)
    : HeapSnapshot(vm, stringTable, DumpSnapShotOption{DumpFormat::JSON, true}, false, entryIdMap, allocator),
      stsInterface_(stsInterface),
      dumpDynamicHeap_(dumpDynamicHeap),
      dumpStaticHeap_(dumpStaticHeap),
      isSimplify_(isSimplify)
{
}

HprofNode *HybridHeapSnapshot::CreateStaticNode(const arkplatform::NodeInfo &nodeInfo)
{
    auto [idExist, nodeId] = entryIdMap_->FindId(nodeInfo.addr);
    NodeId id = idExist ? nodeId : entryIdMap_->GetNextId();
    if (!idExist) {
        entryIdMap_->InsertId(nodeInfo.addr, id);
    }
    CString *name = GetString(nodeInfo.name.c_str());
    HprofNode *node = HprofNode::NewNode(chunk_, id, nodeCount_, name, MapStaticNodeType(nodeInfo.nodeType),
                                         nodeInfo.size, nodeInfo.nativeSize, nodeInfo.addr);
    node->SetDynamic(false);
    return node;
}

HprofNode *HybridHeapSnapshot::GetOrCreateStaticNode(uint64_t addr)
{
    HprofNode *existing = entryMap_.FindEntry(addr);
    if (existing != nullptr) {
        return existing;
    }
    arkplatform::NodeInfo info = stsInterface_->GetEtsNodeInfo(addr);
    HprofNode *node = CreateStaticNode(info);
    entryMap_.InsertEntry(node);
    InsertNodeUnique(node);
    return node;
}

void HybridHeapSnapshot::CreateSyntheticRootNode()
{
    LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapSnapshot::CreateSyntheticRootNode";
 
    CString *name = GetString("Synthetic");
    HprofNode *syntheticRoot = HprofNode::NewNode(chunk_, 1, 0, name, NodeType::SYNTHETIC, 0, 0, 0);
    InsertNodeUnique(syntheticRoot);
}

void HybridHeapSnapshot::CreateSpecificSyntheticRootNodes()
{
    if (!dumpDynamicHeap_) {
        return;
    }

    LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapSnapshot::CreateSpecificSyntheticRootNodes";
    CreateSpecificSyntheticRoot(localHandleRoots_, "LocalHandleRoot", LOCAL_HANDLE_ROOT_INDEX);
    CreateSpecificSyntheticRoot(globalHandleRoots_, "GlobalHandleRoot", GLOBAL_HANDLE_ROOT_INDEX);
    CreateSpecificSyntheticRoot(vmRoots_, "VMRoot", VM_ROOT_INDEX);
    CreateSpecificSyntheticRoot(frameRoots_, "FrameRoot", FRAME_ROOT_INDEX);
}

void HybridHeapSnapshot::CreateDynamicRootNodes()
{
    if (!dumpDynamicHeap_) {
        return;
    }
    LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapSnapshot::CreateDynamicRootNodes";
    FillNodes(true, isSimplify_);
}

void HybridHeapSnapshot::CreateStaticRootNodes()
{
    if (!dumpStaticHeap_) {
        return;
    }
    std::vector<arkplatform::NodeInfo> staticRoots = stsInterface_->GetEtsVMRoots();
    LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapSnapshot::CreateStaticRootNodes";
    for (auto &nodeInfo : staticRoots) {
        HprofNode *existing = entryMap_.FindEntry(nodeInfo.addr);
        if (existing != nullptr) {
            continue;
        }
        HprofNode *node = CreateStaticNode(nodeInfo);
        entryMap_.InsertEntry(node);
        InsertNodeUnique(node);
    }
}

void HybridHeapSnapshot::CreateSyntheticRootEdges()
{
    LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapSnapshot::CreateSyntheticRootEdges";
    HprofNode *syntheticRoot = nodes_[SYNTHETIC_ROOT_INDEX];
    if (dumpDynamicHeap_) {
        CreateEdgeToSpecificSyntheticRoot(syntheticRoot, nodes_[LOCAL_HANDLE_ROOT_INDEX]);
        CreateEdgeToSpecificSyntheticRoot(syntheticRoot, nodes_[GLOBAL_HANDLE_ROOT_INDEX]);
        CreateEdgeToSpecificSyntheticRoot(syntheticRoot, nodes_[VM_ROOT_INDEX]);
        CreateEdgeToSpecificSyntheticRoot(syntheticRoot, nodes_[FRAME_ROOT_INDEX]);
    }

    for (size_t i = GetActualRootStartIndex(); i < nodes_.size(); ++i) {
        HprofNode *rootNode = nodes_[i];
        CString *name = GetString("-subroot-");
        InsertEdgeUnique(Edge::NewEdge(chunk_, EdgeType::SHORTCUT, syntheticRoot, rootNode, name));
        syntheticRoot->IncEdgeCount();
    }
}

void HybridHeapSnapshot::CreateEdgeToSpecificSyntheticRoot(HprofNode *syntheticRoot,
                                                           HprofNode *specificSyntheticRoot)
{
    Edge *edge = Edge::NewEdge(chunk_, EdgeType::SHORTCUT, syntheticRoot,
                               specificSyntheticRoot, GetString("-subroot-"));
    InsertEdgeUnique(edge);
    syntheticRoot->IncEdgeCount();
}

void HybridHeapSnapshot::CreateSpecificSyntheticRootEdges()
{
    if (!dumpDynamicHeap_) {
        return;
    }
    LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapSnapshot::CreateSpecificSyntheticRootEdges";
    CreateEdgesToSpecificRoot(nodes_[LOCAL_HANDLE_ROOT_INDEX], localHandleRoots_);
    CreateEdgesToSpecificRoot(nodes_[GLOBAL_HANDLE_ROOT_INDEX], globalHandleRoots_);
    CreateEdgesToSpecificRoot(nodes_[VM_ROOT_INDEX], vmRoots_);
    CreateEdgesToSpecificRoot(nodes_[FRAME_ROOT_INDEX], frameRoots_);
}

void HybridHeapSnapshot::CreateEdgesToSpecificRoot(HprofNode *specificSyntheticRoot,
                                                   const CUnorderedSet<HprofNode *> &rootSet)
{
    for (const auto &root : rootSet) {
        Edge *edge = Edge::NewEdge(chunk_, EdgeType::ELEMENT, specificSyntheticRoot,
                                   root, specificSyntheticRoot->GetEdgeCount());
        InsertEdgeUnique(edge);
        specificSyntheticRoot->IncEdgeCount();
    }
}

void HybridHeapSnapshot::CollectXRefMaps()
{
    if (dumpDynamicHeap_ && dumpStaticHeap_) {
        stsInterface_->GetXRefMaps(reinterpret_cast<uintptr_t>(vm_), jsToEts_, etsToJs_);
        LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapSnapshot::CollectXRefMaps";
    }
}

void HybridHeapSnapshot::DumpRefForDynamicNode(HprofNode *entryFrom)
{
    JSTaggedValue value(entryFrom->GetAddress());
    if (!value.IsHeapObject()) {
        return;
    }

    std::vector<Reference> refs;
    value.DumpForSnapshot(vm_->GetJSThread(), refs, isVmMode_);
    for (const auto &ref : refs) {
        if (ref.type_ == EdgeType::NATIVE) {
            ProcessNativeEdge(ref, entryFrom);
        } else if (ref.type_ == EdgeType::NATIVE_STRING) {
            ProcessNativeStringEdge(ref, entryFrom);
        } else {
            ProcessRegularEdge(ref, entryFrom, isSimplify_);
        }
    }

    DumpXRefDynamicToStatic(entryFrom);
}

void HybridHeapSnapshot::DumpRefForStaticNode(HprofNode *entryFrom)
{
    std::vector<arkplatform::EdgeInfo> staticEdges;
    stsInterface_->GetEtsNodeEdges(entryFrom->GetAddress(), staticEdges);

    for (auto &edgeInfo : staticEdges) {
        HprofNode *entryTo = GetOrCreateStaticNode(edgeInfo.toAddr);

        if (edgeInfo.edgeType == arkplatform::StaticEdgeType::ELEMENT) {
            InsertEdgeUnique(Edge::NewEdge(chunk_, EdgeType::ELEMENT, entryFrom, entryTo, edgeInfo.index));
        } else {
            CString *edgeName = GetString(edgeInfo.name.c_str());
            InsertEdgeUnique(Edge::NewEdge(chunk_, MapStaticEdgeType(edgeInfo.edgeType), entryFrom, entryTo, edgeName));
        }
        entryFrom->IncEdgeCount();
    }

    if (dumpDynamicHeap_) {
        DumpXRefStaticToDynamic(entryFrom);
    }
}

void HybridHeapSnapshot::DumpXRefDynamicToStatic(HprofNode *entryFrom)
{
    if (!dumpStaticHeap_) {
        return;
    }
    auto it = jsToEts_.find(entryFrom->GetAddress());
    if (it == jsToEts_.end()) {
        return;
    }
    uint64_t etsAddr = it->second;
    HprofNode *staticNode = GetOrCreateStaticNode(etsAddr);
    if (staticNode != nullptr) {
        CString *xrefName = GetString("xref_dynamic_to_static");
        InsertEdgeUnique(Edge::NewEdge(chunk_, EdgeType::XREF, entryFrom, staticNode, xrefName));
        entryFrom->IncEdgeCount();
    }
}

void HybridHeapSnapshot::DumpXRefStaticToDynamic(HprofNode *entryFrom)
{
    if (!dumpDynamicHeap_) {
        return;
    }
    auto it = etsToJs_.find(entryFrom->GetAddress());
    if (it == etsToJs_.end()) {
        return;
    }
    uint64_t jsAddr = it->second;
    HprofNode *dynNode = GenerateNode(JSTaggedValue(jsAddr), 0, true, isSimplify_);
    if (dynNode != nullptr) {
        CString *xrefName = GetString("xref_static_to_dynamic");
        InsertEdgeUnique(Edge::NewEdge(chunk_, EdgeType::XREF, entryFrom, dynNode, xrefName));
        entryFrom->IncEdgeCount();
    }
}

void HybridHeapSnapshot::DumpReferences()
{
    CollectXRefMaps();

    LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapSnapshot::DumpReferences";
    // Skip the synthetic root and specific synthetic roots
    for (uint32_t i = GetActualRootStartIndex(); i < nodeCount_; i++) {
        HprofNode *node = nodes_[i];
        ASSERT(node != nullptr);
        if (node->IsDynamic()) {
            DumpRefForDynamicNode(node);
        } else {
            DumpRefForStaticNode(node);
        }
    }
    if (dumpDynamicHeap_ && vm_ != nullptr) {
        const_cast<EcmaVM *>(vm_)->ClearWrappedNativePointerAddrsMap();
    }
}

void HybridHeapSnapshot::InsertNativeAddressNodes()
{
    for (auto node : nativeAddressNodes_) {
        InsertNodeUnique(node);
    }
}

bool HybridHeapSnapshot::BuildHybridSnapshot()
{
    LOG_ECMA(INFO) << "[HybridHeapDump] HybridHeapSnapshot::BuildHybridSnapshot";
    CreateSyntheticRootNode();
    CreateDynamicRootNodes();
    CreateSpecificSyntheticRootNodes();
    CreateStaticRootNodes();
    CreateSyntheticRootEdges();
    CreateSpecificSyntheticRootEdges();
    DumpReferences();
    InsertNativeAddressNodes();
    ReindexAllNodes();
    return Verify();
}
}  // namespace panda::ecmascript
