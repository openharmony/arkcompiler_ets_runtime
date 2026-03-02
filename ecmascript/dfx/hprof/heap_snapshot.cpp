/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#include "ecmascript/dfx/hprof/heap_snapshot.h"

#include <unordered_map>
#include <string>

#include "ecmascript/ecma_string-inl.h"

namespace panda::ecmascript {
CString *HeapSnapshot::GetString(const CString &as)
{
    return stringTable_->GetString(as);
}

CString *HeapSnapshot::GetArrayString(TaggedArray *array, const CString &as)
{
    CString arrayName = as;
    arrayName.append("[");
    arrayName.append(ToCString(array->GetLength()));
    arrayName.append("]");
    return GetString(arrayName);  // String type was handled singly, see#GenerateStringNode
}

Node *Node::NewNode(Chunk &chunk, NodeId id, size_t index, const CString *name, NodeType type, size_t size,
                    size_t nativeSize, JSTaggedType entry, bool isLive)
{
    auto node = chunk.New<Node>(id, index, name, type, size, nativeSize, 0, entry, isLive);
    if (UNLIKELY(node == nullptr)) {
        LOG_FULL(FATAL) << "internal allocator failed";
        UNREACHABLE();
    }
    return node;
}

Edge *Edge::NewEdge(Chunk &chunk, EdgeType type, Node *from, Node *to, CString *name)
{
    auto edge = chunk.New<Edge>(type, from, to, name);
    if (UNLIKELY(edge == nullptr)) {
        LOG_FULL(FATAL) << "internal allocator failed";
        UNREACHABLE();
    }
    return edge;
}

Edge *Edge::NewEdge(Chunk &chunk, EdgeType type, Node *from, Node *to, uint32_t index)
{
    auto edge = chunk.New<Edge>(type, from, to, index);
    if (UNLIKELY(edge == nullptr)) {
        LOG_FULL(FATAL) << "internal allocator failed";
        UNREACHABLE();
    }
    return edge;
}

HeapSnapshot::~HeapSnapshot()
{
    for (Node *node : nodes_) {
        chunk_.Delete(node);
    }
    for (Edge *edge : edges_) {
        chunk_.Delete(edge);
    }
    nodes_.clear();
    edges_.clear();
    traceInfoStack_.clear();
    stackInfo_.clear();
    scriptIdMap_.clear();
    methodToTraceNodeId_.clear();
    traceNodeIndex_.clear();
    entryIdMap_ = nullptr;
    stringTable_ = nullptr;
}

bool HeapSnapshot::BuildUp(bool isSimplify)
{
    FillNodes(true, isSimplify);
    FillEdges(isSimplify);
    AddSyntheticRoot();
    return Verify();
}

bool HeapSnapshot::Verify()
{
    GetString(CString("HeapVerify:").append(ToCString(totalNodesSize_)));
    return (edgeCount_ > nodeCount_) && (totalNodesSize_ > 0);
}

void HeapSnapshot::PrepareSnapshot()
{
    FillNodes();
    if (trackAllocations()) {
        PrepareTraceInfo();
    }
}

void HeapSnapshot::UpdateNodes(bool isInFinish)
{
    ASSERT(!vm_->GetAssociatedJSThread()->IsConcurrentCopying());
    for (Node *node : nodes_) {
        node->SetLive(false);
    }

    // Collect alive objects
    auto heap = vm_->GetHeap();
    if (heap != nullptr) {
        heap->IterateOverObjects([this, isInFinish](TaggedObject *obj) {
            GenerateNode(JSTaggedValue(obj), 0, isInFinish);
        });
    }

    // Remove all dead objects
    auto newEnd = std::remove_if(nodes_.begin(), nodes_.end(), [this](Node *node) {
        if (!node->IsLive()) {
            entryMap_.FindAndEraseNode(node->GetAddress());
            entryIdMap_->EraseId(node->GetAddress());
            DecreaseNodeSize(node->GetSelfSize());
            chunk_.Delete(node);
            nodeCount_--;
            return true;
        }
        return false;
    });
    nodes_.erase(newEnd, nodes_.end());
}

bool HeapSnapshot::FinishSnapshot()
{
    ASSERT(!vm_->GetAssociatedJSThread()->IsConcurrentCopying());
    UpdateNodes(true);
    FillEdges();
    AddSyntheticRoot();
    return Verify();
}

void HeapSnapshot::RecordSampleTime()
{
    timeStamps_.emplace_back(entryIdMap_->GetLastId());
}

void HeapSnapshot::PushHeapStat(Stream* stream)
{
    CVector<HeapStat> statsBuffer;
    if (stream == nullptr) {
        LOG_DEBUGGER(ERROR) << "HeapSnapshot::PushHeapStat::stream is nullptr";
        return;
    }
    int32_t preChunkSize = stream->GetSize();
    int32_t sequenceId = 0;
    int64_t timeStampUs = 0;
    auto iter = nodes_.begin();
    for (size_t timeIndex = 0; timeIndex < timeStamps_.size(); ++timeIndex) {
        TimeStamp& timeStamp = timeStamps_[timeIndex];
        sequenceId = timeStamp.GetLastSequenceId();
        timeStampUs = timeStamp.GetTimeStamp();
        uint32_t nodesSize = 0;
        uint32_t nodesCount = 0;
        while (iter != nodes_.end() && (*iter)->GetId() <= static_cast<uint32_t>(sequenceId)) {
            nodesCount++;
            nodesSize += (*iter)->GetSelfSize();
            iter++;
        }
        if ((timeStamp.GetCount() != nodesCount) || (timeStamp.GetSize() != nodesSize)) {
            timeStamp.SetCount(nodesCount);
            timeStamp.SetSize(nodesSize);
            statsBuffer.emplace_back(static_cast<int32_t>(timeIndex), nodesCount, nodesSize);
            if (static_cast<int32_t>(statsBuffer.size()) >= preChunkSize) {
                stream->UpdateHeapStats(&statsBuffer.front(), static_cast<int32_t>(statsBuffer.size()));
                statsBuffer.clear();
            }
        }
    }
    if (!statsBuffer.empty()) {
        stream->UpdateHeapStats(&statsBuffer.front(), static_cast<int32_t>(statsBuffer.size()));
        statsBuffer.clear();
    }
    stream->UpdateLastSeenObjectId(entryIdMap_->GetLastId(), timeStampUs);
}

Node *HeapSnapshot::AddNode(TaggedObject *address, size_t size)
{
    return GenerateNode(JSTaggedValue(address), size);
}

void HeapSnapshot::MoveNode(uintptr_t address, TaggedObject *forwardAddress, size_t size)
{
    if (address == reinterpret_cast<uintptr_t>(forwardAddress)) {
        return;
    }

    Node *node = entryMap_.FindAndEraseNode(static_cast<JSTaggedType>(address));
    if (node != nullptr) {
        ASSERT(node->GetId() <= UINT_MAX);

        Node *oldNode = entryMap_.FindAndEraseNode(Node::NewAddress(forwardAddress));
        if (oldNode != nullptr) {
            oldNode->SetAddress(Node::NewAddress(static_cast<TaggedObject *>(nullptr)));
        }

        // Size and name may change during its life for some types(such as string, array and etc).
        if (forwardAddress->GetClass() != nullptr) {
            node->SetName(GenerateNodeName(forwardAddress));
        }
        if (JSTaggedValue(forwardAddress).IsString()) {
            node->SetSelfSize(forwardAddress->GetSize());
        } else {
            node->SetSelfSize(size);
        }
        node->SetAddress(Node::NewAddress(forwardAddress));
        entryMap_.InsertEntry(node);
    } else {
        LOG_DEBUGGER(WARN) << "Untracked object moves from " << address << " to " << forwardAddress;
        GenerateNode(JSTaggedValue(forwardAddress), size, false);
    }
}

// NOLINTNEXTLINE(readability-function-size)
CString *HeapSnapshot::GenerateNodeName(TaggedObject *entry)
{
    auto *hCls = entry->GetClass();
    JSType type = hCls->GetObjectType();
    CString *nodename = GetString(GetNodeName(type, IsInVmMode()));
    switch (type) {
        case JSType::TAGGED_ARRAY:
        case JSType::JS_SHARED_TYPED_ARRAY:
        case JSType::FUNC_SLOT:
        case JSType::LEXICAL_ENV:
        case JSType::WEAK_LINKED_HASH_MAP:
        case JSType::SFUNCTION_ENV:
        case JSType::SENDABLE_ENV:
        case JSType::CONSTANT_POOL:
        case JSType::PROFILE_TYPE_INFO:
        case JSType::TAGGED_DICTIONARY:
        case JSType::AOT_LITERAL_INFO:
        case JSType::VTABLE:
        case JSType::COW_TAGGED_ARRAY:
            return GetArrayString(TaggedArray::Cast(entry), *nodename);
        default:
            break;
    }
    return nodename;
}

CString HeapSnapshot::GetNodeName(JSType type, bool isVmMode)
{
    static const std::unordered_map<JSType, std::string> baseNodeNameMap_ = {
        {JSType::TAGGED_ARRAY, "ArkInternalArray"},
        {JSType::JS_SHARED_TYPED_ARRAY, "ArkInternalArray"},
        {JSType::FUNC_SLOT, "ArkInternalFuncSlot"},
        {JSType::LEXICAL_ENV, "LexicalEnv"},
        {JSType::WEAK_LINKED_HASH_MAP, "WeakLinkedHashMap"},
        {JSType::SFUNCTION_ENV, "SFunctionEnv"},
        {JSType::SENDABLE_ENV, "SendableEnv"},
        {JSType::CONSTANT_POOL, "ArkInternalConstantPool"},
        {JSType::PROFILE_TYPE_INFO, "ArkInternalProfileTypeInfo"},
        {JSType::TAGGED_DICTIONARY, "ArkInternalDict"},
        {JSType::AOT_LITERAL_INFO, "ArkInternalAOTLiteralInfo"},
        {JSType::VTABLE, "ArkInternalVTable"},
        {JSType::COW_TAGGED_ARRAY, "ArkInternalCOWArray"},
        {JSType::HCLASS, "HiddenClass(NonMovable)"},
        {JSType::LINKED_NODE, "LinkedNode"},
        {JSType::TRACK_INFO, "TrackInfo"},
        {JSType::LINE_STRING, "BaseString"},
        {JSType::TREE_STRING, "BaseString"},
        {JSType::SLICED_STRING, "BaseString"},
        {JSType::CACHED_EXTERNAL_STRING, "BaseString"},
        {JSType::JS_OBJECT, "JSObject"},
        {JSType::JS_SHARED_OBJECT, "JSSharedObject"},
        {JSType::JS_WRAPPED_NAPI_OBJECT, "JSWrappedNapiObject"},
        {JSType::JS_XREF_OBJECT, "JSXRefObject"},
        {JSType::JS_SHARED_FUNCTION, "JSSharedFunction"},
        {JSType::FREE_OBJECT_WITH_ONE_FIELD, "FreeObject"},
        {JSType::FREE_OBJECT_WITH_NONE_FIELD, "FreeObject"},
        {JSType::FREE_OBJECT_WITH_TWO_FIELD, "FreeObject"},
        {JSType::JS_NATIVE_POINTER, "JSNativePointer"},
        {JSType::JS_FUNCTION_BASE, "JSFunctionBase"},
        {JSType::JS_FUNCTION, "JSFunction"},
        {JSType::JS_API_FUNCTION, "JSApiFunction"},
        {JSType::FUNCTION_TEMPLATE, "ArkInternalFunctionTemplate"},
        {JSType::JS_ERROR, "Error"},
        {JSType::JS_EVAL_ERROR, "Eval Error"},
        {JSType::JS_RANGE_ERROR, "Range Error"},
        {JSType::JS_TYPE_ERROR, "Type Error"},
        {JSType::JS_AGGREGATE_ERROR, "Aggregate Error"},
        {JSType::JS_REFERENCE_ERROR, "Reference Error"},
        {JSType::JS_URI_ERROR, "Uri Error"},
        {JSType::JS_SYNTAX_ERROR, "Syntax Error"},
        {JSType::JS_OOM_ERROR, "OutOfMemory Error"},
        {JSType::JS_TERMINATION_ERROR, "Termination Error"},
        {JSType::JS_REG_EXP, "Regexp"},
        {JSType::JS_SET, "Set"},
        {JSType::JS_SHARED_SET, "SharedSet"},
        {JSType::JS_MAP, "Map"},
        {JSType::JS_SHARED_MAP, "SharedMap"},
        {JSType::JS_WEAK_SET, "WeakSet"},
        {JSType::JS_WEAK_MAP, "WeakMap"},
        {JSType::JS_DATE, "Date"},
        {JSType::JS_BOUND_FUNCTION, "Bound Function"},
        {JSType::JS_ARRAY, "JSArray"},
        {JSType::JS_TYPED_ARRAY, "Typed Array"},
        {JSType::JS_INT8_ARRAY, "Int8 Array"},
        {JSType::JS_UINT8_ARRAY, "Uint8 Array"},
        {JSType::JS_UINT8_CLAMPED_ARRAY, "Uint8 Clamped Array"},
        {JSType::JS_INT16_ARRAY, "Int16 Array"},
        {JSType::JS_UINT16_ARRAY, "Uint16 Array"},
        {JSType::JS_INT32_ARRAY, "Int32 Array"},
        {JSType::JS_UINT32_ARRAY, "Uint32 Array"},
        {JSType::JS_FLOAT32_ARRAY, "Float32 Array"},
        {JSType::JS_FLOAT64_ARRAY, "Float64 Array"},
        {JSType::JS_BIGINT64_ARRAY, "BigInt64 Array"},
        {JSType::JS_BIGUINT64_ARRAY, "BigUint64 Array"},
        {JSType::JS_ARGUMENTS, "Arguments"},
        {JSType::BIGINT, "BigInt"},
        {JSType::JS_PROXY, "Proxy"},
        {JSType::JS_PRIMITIVE_REF, "Primitive"},
        {JSType::JS_DATA_VIEW, "DataView"},
        {JSType::JS_ITERATOR, "Iterator"},
        {JSType::JS_FORIN_ITERATOR, "ForinInterator"},
        {JSType::JS_MAP_ITERATOR, "MapIterator"},
        {JSType::JS_SHARED_MAP_ITERATOR, "SharedMapIterator"},
        {JSType::JS_SET_ITERATOR, "SetIterator"},
        {JSType::JS_SHARED_SET_ITERATOR, "SharedSetIterator"},
        {JSType::JS_REG_EXP_ITERATOR, "RegExpIterator"},
        {JSType::JS_ARRAY_ITERATOR, "ArrayIterator"},
        {JSType::JS_STRING_ITERATOR, "StringIterator"},
        {JSType::JS_ARRAY_BUFFER, "ArrayBuffer"},
        {JSType::JS_SENDABLE_ARRAY_BUFFER, "SendableArrayBuffer"},
        {JSType::JS_SHARED_ARRAY, "SharedArray"},
        {JSType::JS_SHARED_ARRAY_BUFFER, "SharedArrayBuffer"},
        {JSType::JS_PROXY_REVOC_FUNCTION, "ProxyRevocFunction"},
        {JSType::PROMISE_REACTIONS, "PromiseReaction"},
        {JSType::PROMISE_CAPABILITY, "PromiseCapability"},
        {JSType::ASYNC_GENERATOR_REQUEST, "AsyncGeneratorRequest"},
        {JSType::PROMISE_ITERATOR_RECORD, "PromiseIteratorRecord"},
        {JSType::PROMISE_RECORD, "PromiseRecord"},
        {JSType::RESOLVING_FUNCTIONS_RECORD, "ResolvingFunctionsRecord"},
        {JSType::JS_PROMISE, "Promise"},
        {JSType::JS_PROMISE_REACTIONS_FUNCTION, "PromiseReactionsFunction"},
        {JSType::JS_PROMISE_EXECUTOR_FUNCTION, "PromiseExecutorFunction"},
        {JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION, "AsyncModuleFulfilledFunction"},
        {JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION, "AsyncModuleRejectedFunction"},
        {JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION, "AsyncFromSyncIterUnwarpFunction"},
        {JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION, "PromiseAllResolveElementFunction"},
        {JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION, "PromiseAnyRejectElementFunction"},
        {JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION, "PromiseAllSettledElementFunction"},
        {JSType::JS_PROMISE_FINALLY_FUNCTION, "PromiseFinallyFunction"},
        {JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION, "PromiseValueThunkOrThrowerFunction"},
        {JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN, "AsyncGeneratorResumeNextReturnProcessorRstFtn"},
        {JSType::JS_GENERATOR_FUNCTION, "JSGeneratorFunction"},
        {JSType::JS_ASYNC_GENERATOR_FUNCTION, "JSAsyncGeneratorFunction"},
        {JSType::SYMBOL, "Symbol"},
        {JSType::JS_ASYNC_FUNCTION, "AsyncFunction"},
        {JSType::JS_SHARED_ASYNC_FUNCTION, "SharedAsyncFunction"},
        {JSType::JS_INTL_BOUND_FUNCTION, "JSIntlBoundFunction"},
        {JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION, "AsyncAwaitStatusFunction"},
        {JSType::JS_ASYNC_FUNC_OBJECT, "AsyncFunctionObject"},
        {JSType::JS_REALM, "Realm"},
        {JSType::JS_GLOBAL_OBJECT, "GlobalObject"},
        {JSType::JS_INTL, "JSIntl"},
        {JSType::JS_LOCALE, "JSLocale"},
        {JSType::JS_DATE_TIME_FORMAT, "JSDateTimeFormat"},
        {JSType::JS_RELATIVE_TIME_FORMAT, "JSRelativeTimeFormat"},
        {JSType::JS_NUMBER_FORMAT, "JSNumberFormat"},
        {JSType::JS_COLLATOR, "JSCollator"},
        {JSType::JS_PLURAL_RULES, "JSPluralRules"},
        {JSType::JS_DISPLAYNAMES, "JSDisplayNames"},
        {JSType::JS_SEGMENTER, "JSSegmenter"},
        {JSType::JS_SEGMENTS, "JSSegments"},
        {JSType::JS_SEGMENT_ITERATOR, "JSSegmentIterator"},
        {JSType::JS_LIST_FORMAT, "JSListFormat"},
        {JSType::JS_GENERATOR_OBJECT, "JSGeneratorObject"},
        {JSType::JS_ASYNC_GENERATOR_OBJECT, "JSAsyncGeneratorObject"},
        {JSType::JS_GENERATOR_CONTEXT, "JSGeneratorContext"},
        {JSType::ACCESSOR_DATA, "AccessorData"},
        {JSType::INTERNAL_ACCESSOR, "InternalAccessor"},
        {JSType::MICRO_JOB_QUEUE, "MicroJobQueue"},
        {JSType::PENDING_JOB, "PendingJob"},
        {JSType::COMPLETION_RECORD, "CompletionRecord"},
        {JSType::JS_API_ARRAY_LIST, "ArrayList"},
        {JSType::JS_API_ARRAYLIST_ITERATOR, "ArrayListIterator"},
        {JSType::JS_API_HASH_MAP, "HashMap"},
        {JSType::JS_API_HASH_SET, "HashSet"},
        {JSType::JS_API_HASHMAP_ITERATOR, "HashMapIterator"},
        {JSType::JS_API_HASHSET_ITERATOR, "HashSetIterator"},
        {JSType::JS_API_LIGHT_WEIGHT_MAP, "LightWeightMap"},
        {JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR, "LightWeightMapIterator"},
        {JSType::JS_API_LIGHT_WEIGHT_SET, "LightWeightSet"},
        {JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR, "LightWeightSetIterator"},
        {JSType::JS_API_TREE_MAP, "TreeMap"},
        {JSType::JS_API_TREE_SET, "TreeSet"},
        {JSType::JS_API_TREEMAP_ITERATOR, "TreeMapIterator"},
        {JSType::JS_API_TREESET_ITERATOR, "TreeSetIterator"},
        {JSType::JS_API_VECTOR, "Vector"},
        {JSType::JS_API_VECTOR_ITERATOR, "VectorIterator"},
        {JSType::JS_API_BITVECTOR, "BitVector"},
        {JSType::JS_API_BITVECTOR_ITERATOR, "BitVectorIterator"},
        {JSType::JS_API_QUEUE, "Queue"},
        {JSType::JS_API_QUEUE_ITERATOR, "QueueIterator"},
        {JSType::JS_API_DEQUE, "Deque"},
        {JSType::JS_API_DEQUE_ITERATOR, "DequeIterator"},
        {JSType::JS_API_STACK, "Stack"},
        {JSType::JS_API_STACK_ITERATOR, "StackIterator"},
        {JSType::JS_API_LIST, "List"},
        {JSType::JS_API_LINKED_LIST, "LinkedList"},
        {JSType::SOURCE_TEXT_MODULE_RECORD, "SourceTextModule"},
        {JSType::IMPORTENTRY_RECORD, "ImportEntry"},
        {JSType::LOCAL_EXPORTENTRY_RECORD, "LocalExportEntry"},
        {JSType::INDIRECT_EXPORTENTRY_RECORD, "IndirectExportEntry"},
        {JSType::STAR_EXPORTENTRY_RECORD, "StarExportEntry"},
        {JSType::RESOLVEDBINDING_RECORD, "ResolvedBinding"},
        {JSType::RESOLVEDINDEXBINDING_RECORD, "ResolvedIndexBinding"},
        {JSType::RESOLVEDRECORDINDEXBINDING_RECORD, "ResolvedRecordIndexBinding"},
        {JSType::RESOLVEDRECORDBINDING_RECORD, "ResolvedRecordBinding"},
        {JSType::JS_MODULE_NAMESPACE, "ModuleNamespace"},
        {JSType::JS_API_PLAIN_ARRAY, "PlainArray"},
        {JSType::JS_API_PLAIN_ARRAY_ITERATOR, "PlainArrayIterator"},
        {JSType::JS_CJS_EXPORTS, "CJS Exports"},
        {JSType::JS_CJS_MODULE, "CJS Module"},
        {JSType::JS_CJS_REQUIRE, "CJS Require"},
        {JSType::METHOD, "Method"},
        {JSType::CELL_RECORD, "CellRecord"},
        {JSType::JS_WEAK_REF, "WeakRef"},
        {JSType::JS_FINALIZATION_REGISTRY, "JSFinalizationRegistry"},
        {JSType::JS_ASYNCITERATOR, "AsyncIterator"},
        {JSType::JS_ASYNC_FROM_SYNC_ITERATOR, "AsyncFromSyncIterator"},
        {JSType::JS_API_LINKED_LIST_ITERATOR, "LinkedListIterator"},
        {JSType::JS_API_LIST_ITERATOR, "ListIterator"},
        {JSType::JS_SHARED_ARRAY_ITERATOR, "SharedArrayIterator"},
        {JSType::JS_SHARED_INT8_ARRAY, "Shared Int8 Array"},
        {JSType::JS_SHARED_UINT8_ARRAY, "Shared Uint8 Array"},
        {JSType::JS_SHARED_UINT8_CLAMPED_ARRAY, "Shared Uint8 Clamped Array"},
        {JSType::JS_SHARED_INT16_ARRAY, "Shared Int16 Array"},
        {JSType::JS_SHARED_UINT16_ARRAY, "Shared Uint16 Array"},
        {JSType::JS_SHARED_INT32_ARRAY, "Shared Int32 Array"},
        {JSType::JS_SHARED_UINT32_ARRAY, "Shared Uint32 Array"},
        {JSType::JS_SHARED_FLOAT32_ARRAY, "Shared Float32 Array"},
        {JSType::JS_SHARED_FLOAT64_ARRAY, "Shared Float64 Array"},
        {JSType::JS_SHARED_BIGINT64_ARRAY, "Shared BigInt64 Array"},
        {JSType::JS_SHARED_BIGUINT64_ARRAY, "Shared BigUint64 Array"},
        {JSType::NATIVE_MODULE_FAILURE_INFO, "NativeModuleFailureInfo"},
        {JSType::MUTANT_TAGGED_ARRAY, "MutantTaggedArray"},
        {JSType::BYTE_ARRAY, "ByteArray"},
        {JSType::COW_MUTANT_TAGGED_ARRAY, "COWMutantTaggedArray"},
        {JSType::RB_TREENODE, "RBTreeNode"},
        {JSType::ENUM_CACHE, "EnumCache"},
        {JSType::CLASS_LITERAL, "ClassLiteral"},
        {JSType::ASYNC_ITERATOR_RECORD, "AsyncIteratorRecord"},
        {JSType::MODULE_RECORD, "ModuleRecord"},
        {JSType::PROFILE_TYPE_INFO_CELL_0, "ProfileTypeInfoCell"},
        {JSType::PROFILE_TYPE_INFO_CELL_1, "ProfileTypeInfoCell"},
        {JSType::PROFILE_TYPE_INFO_CELL_N, "ProfileTypeInfoCell"},
        {JSType::EXTRA_PROFILE_TYPE_INFO, "ExtraProfileTypeInfo"},
    };
    static const std::unordered_map<JSType, std::string> vmModeNodeNameMap_ = {
        {JSType::PROPERTY_BOX, "PropertyBox"},
        {JSType::GLOBAL_ENV, "GlobalEnv"},
        {JSType::PROTOTYPE_HANDLER, "PrototypeHandler"},
        {JSType::TRANSITION_HANDLER, "TransitionHandler"},
        {JSType::TRANS_WITH_PROTO_HANDLER, "TransWithProtoHandler"},
        {JSType::STORE_TS_HANDLER, "StoreAOTHandler"},
        {JSType::PROTO_CHANGE_MARKER, "ProtoChangeMarker"},
        {JSType::MARKER_CELL, "MarkerCell"},
        {JSType::PROTOTYPE_INFO, "ProtoChangeDetails"},
        {JSType::TEMPLATE_MAP, "TemplateMap"},
        {JSType::PROGRAM, "Program"},
        {JSType::MACHINE_CODE_OBJECT, "MachineCode"},
        {JSType::CLASS_INFO_EXTRACTOR, "ClassInfoExtractor"},
    };
    auto it = baseNodeNameMap_.find(type);
    if (it != baseNodeNameMap_.end()) {
        return CString(it->second);
    }
    if (isVmMode) {
        auto vmIt = vmModeNodeNameMap_.find(type);
        if (vmIt != vmModeNodeNameMap_.end()) {
            return CString(vmIt->second);
        }
    } else {
        return CString("Hidden Object");
    }
    return CString("UnKnownType").append(std::to_string(static_cast<int>(type)));
}

NodeType HeapSnapshot::GenerateNodeType(TaggedObject *entry)
{
    NodeType nodeType = NodeType::DEFAULT;
    auto *hCls = entry->GetClass();
    JSType type = hCls->GetObjectType();

    if (hCls->IsTaggedArray()) {
        nodeType = NodeType::ARRAY;
    } else if (hCls->IsHClass()) {
        nodeType = NodeType::DEFAULT;
    } else if (type == JSType::PROPERTY_BOX) {
        nodeType = NodeType::HIDDEN;
    } else if (type == JSType::JS_ARRAY || type == JSType::JS_TYPED_ARRAY) {
        nodeType = NodeType::OBJECT;
    } else if (type == JSType::JS_OBJECT || type == JSType::JS_SHARED_OBJECT) {
        nodeType = NodeType::OBJECT;
    } else if (type >= JSType::JS_FUNCTION_FIRST && type <= JSType::JS_FUNCTION_LAST) {
        nodeType = NodeType::CLOSURE;
    } else if (type == JSType::JS_BOUND_FUNCTION) {
        nodeType = NodeType::DEFAULT;
    } else if (type == JSType::JS_FUNCTION_BASE) {
        nodeType = NodeType::DEFAULT;
    } else if (type == JSType::JS_REG_EXP) {
        nodeType = NodeType::REGEXP;
    } else if (type == JSType::SYMBOL) {
        nodeType = NodeType::SYMBOL;
    } else if (type == JSType::JS_PRIMITIVE_REF) {
        nodeType = NodeType::HEAPNUMBER;
    } else if (type == JSType::BIGINT) {
        nodeType = NodeType::BIGINT;
    } else {
        nodeType = NodeType::DEFAULT;
    }

    return nodeType;
}

void HeapSnapshot::FillNodes(bool isInFinish, bool isSimplify)
{
    class GenerateNodeRootVisitor final : public RootVisitor {
    public:
        explicit GenerateNodeRootVisitor(HeapSnapshot &snapshot, bool isInFinish, bool isSimplify)
            : snapshot_(snapshot), isInFinish_(isInFinish), isSimplify_(isSimplify) {}
        ~GenerateNodeRootVisitor() = default;

        void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override
        {
            snapshot_.GenerateNode(JSTaggedValue(slot.GetTaggedType()), 0, isInFinish_, isSimplify_);
        }

        void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override
        {
            for (ObjectSlot slot = start; slot < end; slot++) {
                snapshot_.GenerateNode(JSTaggedValue(slot.GetTaggedType()), 0, isInFinish_, isSimplify_);
            }
        }

        void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
            [[maybe_unused]] ObjectSlot derived, [[maybe_unused]] uintptr_t baseOldObject) override {}
    private:
        HeapSnapshot &snapshot_;
        bool isInFinish_;
        bool isSimplify_;
    };
    LOG_ECMA(INFO) << "HeapSnapshot::FillNodes";
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "HeapSnapshot::FillNodes", "");
    // Iterate Heap Object
    GenerateNodeRootVisitor visitor(*this, isInFinish, isSimplify);
    rootVisitor_.VisitHeapRoots(vm_->GetJSThread(), visitor);
}

Node *HeapSnapshot::HandleStringNode(JSTaggedValue &entry, size_t &size, bool &isInFinish, bool isBinMod)
{
    Node* node = nullptr;
    if (isPrivate_) {
        node = GeneratePrivateStringNode(size);
    } else {
        node = GenerateStringNode(entry, size, isInFinish, isBinMod);
    }
    if (node == nullptr) {
        LOG_ECMA(ERROR) << "string node nullptr";
    }
    return node;
}

Node *HeapSnapshot::HandleFunctionNode(JSTaggedValue &entry, size_t &size, bool &isInFinish)
{
    Node* node = GenerateFunctionNode(entry, size, isInFinish);
    if (node == nullptr) {
        LOG_ECMA(ERROR) << "function node nullptr";
    }
    return node;
}

Node *HeapSnapshot::HandleObjectNode(JSTaggedValue &entry, size_t &size, bool &isInFinish)
{
    Node* node = GenerateObjectNode(entry, size, isInFinish);
    if (node == nullptr) {
        LOG_ECMA(ERROR) << "object node nullptr";
    }
    return node;
}

Node *HeapSnapshot::HandleBaseClassNode(size_t size, bool idExist, NodeId &sequenceId,
                                        TaggedObject* obj, JSTaggedType &addr)
{
    size_t selfSize;
    if (g_isEnableCMCGC) {
        selfSize = (size != 0) ? size : reinterpret_cast<BaseObject *>(obj)->GetSize();
    } else {
        selfSize = (size != 0) ? size : obj->GetSize();
    }
    size_t nativeSize = 0;
    if (obj->GetClass()->IsJSNativePointer()) {
        nativeSize = JSNativePointer::Cast(obj)->GetBindingSize();
    }
    Node* node = Node::NewNode(chunk_, sequenceId, nodeCount_, GenerateNodeName(obj), GenerateNodeType(obj),
        selfSize, nativeSize, addr);
    entryMap_.InsertEntry(node);
    if (!idExist) {
        entryIdMap_->InsertId(addr, sequenceId);
    }
    InsertNodeUnique(node);
    ASSERT(entryMap_.FindEntry(node->GetAddress())->GetAddress() == node->GetAddress());
    return node;
}

CString HeapSnapshot::GeneratePrimitiveNameString(JSTaggedValue &entry)
{
    CString primitiveName;
    if (entry.IsInt()) {
        primitiveName.append("Int:");
        if (!isPrivate_) {
            primitiveName.append(ToCString(entry.GetInt()));
        }
    } else if (entry.IsDouble()) {
        primitiveName.append("Double:");
        if (!isPrivate_) {
            primitiveName.append(FloatToCString(entry.GetDouble()));
        }
    } else if (entry.IsHole()) {
        primitiveName.append("Hole");
    } else if (entry.IsNull()) {
        primitiveName.append("Null");
    } else if (entry.IsTrue()) {
        primitiveName.append("Boolean:true");
    } else if (entry.IsFalse()) {
        primitiveName.append("Boolean:false");
    } else if (entry.IsException()) {
        primitiveName.append("Exception");
    } else if (entry.IsUndefined()) {
        primitiveName.append("Undefined");
    } else {
        primitiveName.append("Illegal_Primitive");
    }
    return primitiveName;
}

Node *HeapSnapshot::GenerateNode(JSTaggedValue entry, size_t size, bool isInFinish, bool isSimplify, bool isBinMod)
{
    Node *node = nullptr;
    if (entry.IsHeapObject()) {
        if (entry.IsWeak()) {
            entry.RemoveWeakTag();
        }
        if (entry.IsString()) {
            return HandleStringNode(entry, size, isInFinish, isBinMod);
        }
        if (entry.IsJSFunction()) {
            return HandleFunctionNode(entry, size, isInFinish);
        }
        if (entry.IsOnlyJSObject()) {
            return HandleObjectNode(entry, size, isInFinish);
        }
        TaggedObject *obj = entry.GetTaggedObject();
        auto *baseClass = obj->GetClass();
        if (baseClass != nullptr) {
            JSTaggedType addr = entry.GetRawData();
            Node *existNode = entryMap_.FindEntry(addr);  // Fast Index
            auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
            if (existNode == nullptr) {
                return HandleBaseClassNode(size, idExist, sequenceId, obj, addr);
            } else {
                existNode->SetLive(true);
                return existNode;
            }
        }
    } else if (!isSimplify) {
        if ((entry.IsInt() || entry.IsDouble()) && !captureNumericValue_) {
                return nullptr;
        }
        CString primitiveName = GeneratePrimitiveNameString(entry);
        // A primitive value with tag will be regarded as a pointer
        JSTaggedType addr = entry.GetRawData();
        Node *existNode = entryMap_.FindEntry(addr);
        if (existNode != nullptr) {
            existNode->SetLive(true);
            return existNode;
        }
        auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
        node = Node::NewNode(chunk_, sequenceId, nodeCount_, GetString(primitiveName), NodeType::HEAPNUMBER, 0,
                             0, addr);
        entryMap_.InsertEntry(node);  // Fast Index
        if (!idExist) {
            entryIdMap_->InsertId(addr, sequenceId);
        }
        InsertNodeUnique(node);
    }
    return node;
}

TraceNode::TraceNode(TraceTree* tree, uint32_t nodeIndex)
    : tree_(tree),
      nodeIndex_(nodeIndex),
      totalSize_(0),
      totalCount_(0),
      id_(tree->GetNextNodeId())
{
}

TraceNode::~TraceNode()
{
    for (TraceNode* node : children_) {
        delete node;
    }
    children_.clear();
}

TraceNode* TraceTree::AddNodeToTree(CVector<uint32_t> traceNodeIndex)
{
    uint32_t len = traceNodeIndex.size();
    if (len == 0) {
        return nullptr;
    }

    TraceNode* node = GetRoot();
    for (int i = static_cast<int>(len) - 1; i >= 0; i--) {
        node = node->FindOrAddChild(traceNodeIndex[i]);
    }
    return node;
}

TraceNode* TraceNode::FindOrAddChild(uint32_t nodeIndex)
{
    TraceNode* child = FindChild(nodeIndex);
    if (child == nullptr) {
        child = new TraceNode(tree_, nodeIndex);
        children_.push_back(child);
    }
    return child;
}

TraceNode* TraceNode::FindChild(uint32_t nodeIndex)
{
    for (TraceNode* node : children_) {
        if (node->GetNodeIndex() == nodeIndex) {
            return node;
        }
    }
    return nullptr;
}

void HeapSnapshot::AddTraceNodeId(MethodLiteral *methodLiteral)
{
    uint32_t traceNodeId = 0;
    auto result = methodToTraceNodeId_.find(methodLiteral);
    if (result == methodToTraceNodeId_.end()) {
        ASSERT(traceInfoStack_.size() > 0);
        traceNodeId = traceInfoStack_.size() - 1;
        methodToTraceNodeId_.emplace(methodLiteral, traceNodeId);
    } else {
        traceNodeId = result->second;
    }
    traceNodeIndex_.emplace_back(traceNodeId);
}

int HeapSnapshot::AddTraceNode(int sequenceId, int size)
{
    traceNodeIndex_.clear();
    auto thread = vm_->GetJSThread();
    JSTaggedType *current = const_cast<JSTaggedType *>(thread->GetCurrentFrame());
    FrameIterator it(current, thread);
    for (; !it.Done(); it.Advance<GCVisitedFlag::VISITED>()) {
        if (!it.IsJSFrame()) {
            continue;
        }
        auto method = it.CheckAndGetMethod();
        if (method == nullptr || method->IsNativeWithCallField()) {
            continue;
        }
        MethodLiteral *methodLiteral = method->GetMethodLiteral(thread);
        if (methodLiteral == nullptr) {
            continue;
        }
        if (stackInfo_.count(methodLiteral) == 0) {
            if (!AddMethodInfo(methodLiteral, method->GetJSPandaFile(thread), sequenceId)) {
                continue;
            }
        }
        AddTraceNodeId(methodLiteral);
    }

    TraceNode* topNode = traceTree_.AddNodeToTree(traceNodeIndex_);
    if (topNode == nullptr) {
        return -1;
    }
    ASSERT(topNode->GetTotalSize() <= static_cast<uint32_t>(INT_MAX));
    int totalSize = static_cast<int>(topNode->GetTotalSize());
    totalSize += size;
    topNode->SetTotalSize(totalSize);
    uint32_t totalCount = topNode->GetTotalCount();
    topNode->SetTotalCount(++totalCount);
    return topNode->GetId();
}

bool HeapSnapshot::AddMethodInfo(MethodLiteral *methodLiteral,
                                 const JSPandaFile *jsPandaFile,
                                 int sequenceId)
{
    struct FunctionInfo codeEntry;
    codeEntry.functionId = sequenceId;
    panda_file::File::EntityId methodId = methodLiteral->GetMethodId();
    const std::string &functionName = MethodLiteral::ParseFunctionName(jsPandaFile, methodId);
    if (functionName.empty()) {
        codeEntry.functionName = "anonymous";
    } else {
        codeEntry.functionName = functionName;
    }
    GetString(codeEntry.functionName.c_str());

    // source file
    DebugInfoExtractor *debugExtractor =
        JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
    const std::string &sourceFile = debugExtractor->GetSourceFile(methodId);
    if (sourceFile.empty()) {
        codeEntry.scriptName = "";
    } else {
        codeEntry.scriptName = sourceFile;
        auto iter = scriptIdMap_.find(codeEntry.scriptName);
        if (iter == scriptIdMap_.end()) {
            scriptIdMap_.emplace(codeEntry.scriptName, scriptIdMap_.size() + 1);
            codeEntry.scriptId = static_cast<int>(scriptIdMap_.size());
        } else {
            codeEntry.scriptId = iter->second;
        }
    }
    GetString(codeEntry.scriptName.c_str());

    // line number
    codeEntry.lineNumber = debugExtractor->GetFristLine(methodId);
    // lineNumber is 0 means that lineTable error or empty function body, so jump this frame.
    if (UNLIKELY(codeEntry.lineNumber == 0)) {
        return false;
    }
    codeEntry.columnNumber = debugExtractor->GetFristColumn(methodId);

    traceInfoStack_.emplace_back(codeEntry);
    stackInfo_.emplace(methodLiteral, codeEntry);
    return true;
}

Node *HeapSnapshot::GenerateStringNode(JSTaggedValue entry, size_t size, bool isInFinish, bool isBinMod)
{
    static const CString EMPTY_STRING;
    JSTaggedType addr = entry.GetRawData();
    Node *existNode = entryMap_.FindEntry(addr);  // Fast Index
    JSThread *thread = vm_->GetJSThread();
    if (existNode != nullptr) {
        if (isInFinish || isBinMod) {
            existNode->SetName(GetString(EntryVisitor::ConvertKey(thread, entry)));
        }
        existNode->SetLive(true);
        return existNode;
    }
    // Allocation Event will generate string node for "".
    // When we need to serialize and isFinish is true, the nodeName will be given the actual string content.
    size_t selfsize = (size != 0) ? size : entry.GetTaggedObject()->GetSize();
    const CString *nodeName = &EMPTY_STRING;
    if (isInFinish || isBinMod) {
        nodeName = GetString(EntryVisitor::ConvertKey(thread, entry));
    }
    auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
    Node *node = Node::NewNode(chunk_, sequenceId, nodeCount_, nodeName, NodeType::STRING, selfsize,
                               0, addr);
    if (!idExist) {
        entryIdMap_->InsertId(addr, sequenceId);
    }
    entryMap_.InsertEntry(node);
    InsertNodeUnique(node);
    return node;
}

Node *HeapSnapshot::GeneratePrivateStringNode(size_t size)
{
    if (privateStringNode_ != nullptr) {
        return privateStringNode_;
    }
    JSTaggedValue stringValue = vm_->GetJSThread()->GlobalConstants()->GetStringString();
    size_t selfsize = (size != 0) ? size : stringValue.GetTaggedObject()->GetSize();
    CString strContent;
    strContent.append(EntryVisitor::ConvertKey(vm_->GetJSThread(), stringValue));
    JSTaggedType addr = stringValue.GetRawData();
    auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
    Node *node = Node::NewNode(chunk_, sequenceId, nodeCount_, GetString(strContent), NodeType::STRING, selfsize,
                               0, addr);
    if (!idExist) {
        entryIdMap_->InsertId(addr, sequenceId);
    }
    entryMap_.InsertEntry(node);
    InsertNodeUnique(node);
    ASSERT(entryMap_.FindEntry(node->GetAddress())->GetAddress() == node->GetAddress());
    privateStringNode_ = node;
    return node;
}

Node *HeapSnapshot::GenerateFunctionNode(JSTaggedValue entry, size_t size, bool isInFinish)
{
    TaggedObject *obj = entry.GetTaggedObject();
    JSTaggedType addr = entry.GetRawData();
    Node *existNode = entryMap_.FindEntry(addr);
    auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
    if (existNode != nullptr) {
        if (isInFinish) {
            CString *functionName = GetString(ParseFunctionName(obj));
            existNode->SetName(functionName);
            if (functionName->find("_GLOBAL") != std::string::npos) {
                existNode->SetType(NodeType::FRAMEWORK);
            }
        }
        existNode->SetLive(true);
        return existNode;
    }
    size_t selfsize = (size != 0) ? size : obj->GetSize();
    Node *node = Node::NewNode(chunk_, sequenceId, nodeCount_, GetString("JSFunction"), NodeType::CLOSURE, selfsize,
                               0, addr);
    if (isInFinish) {
        CString *functionName = GetString(ParseFunctionName(obj));
        node->SetName(functionName);
        if (functionName->find("_GLOBAL") != std::string::npos) {
            node->SetType(NodeType::FRAMEWORK);
        }
    }
    if (!idExist) {
        entryIdMap_->InsertId(addr, sequenceId);
    }
    entryMap_.InsertEntry(node);
    InsertNodeUnique(node);
    return node;
}

Node *HeapSnapshot::GenerateObjectNode(JSTaggedValue entry, size_t size, bool isInFinish)
{
    TaggedObject *obj = entry.GetTaggedObject();
    JSTaggedType addr = entry.GetRawData();
    Node *existNode = entryMap_.FindEntry(addr);
    auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
    if (existNode != nullptr) {
        if (isInFinish) {
            CString *objectName = GetString(ParseObjectName(obj));
            existNode->SetName(objectName);
            if (objectName->find("_GLOBAL") != std::string::npos) {
                existNode->SetType(NodeType::FRAMEWORK);
            }
        }
        existNode->SetLive(true);
        return existNode;
    }
    size_t selfsize = (size != 0) ? size : obj->GetSize();
    Node *node = Node::NewNode(chunk_, sequenceId, nodeCount_, GetString("Object"), NodeType::OBJECT, selfsize,
                               0, addr);
    if (isInFinish) {
        CString *objectName = GetString(ParseObjectName(obj));
        node->SetName(objectName);
        if (objectName->find("_GLOBAL") != std::string::npos) {
            node->SetType(NodeType::FRAMEWORK);
        }
    }
    if (!idExist) {
        entryIdMap_->InsertId(addr, sequenceId);
    }
    entryMap_.InsertEntry(node);
    InsertNodeUnique(node);
    return node;
}

void HeapSnapshot::FillEdges(bool isSimplify)
{
    LOG_ECMA(INFO) << "HeapSnapshot::FillEdges begin, nodeCount: " << nodeCount_;
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "HeapSnapshot::FillEdges", "");
    for (size_t i = 0; i < nodeCount_; ++i) {
        Node *entryFrom = nodes_[i];
        ASSERT(entryFrom != nullptr);
        JSTaggedValue value(entryFrom->GetAddress());
        if (!value.IsHeapObject()) {
            continue;
        }
        std::vector<Reference> referenceResources;
        value.DumpForSnapshot(vm_->GetJSThread(), referenceResources, isVmMode_);
        for (auto const &it : referenceResources) {
            JSTaggedValue toValue = it.value_;
            if (toValue.IsNumber() && !captureNumericValue_) {
                continue;
            }
            Node *entryTo = nullptr;
            EdgeType type = toValue.IsWeak() ? EdgeType::WEAK : it.type_;
            if (toValue.IsWeak()) {
                toValue.RemoveWeakTag();
            }
            if (toValue.IsHeapObject()) {
                auto *to = toValue.GetTaggedObject();
                entryTo = entryMap_.FindEntry(Node::NewAddress(to));
            }
            if (entryTo == nullptr) {
                entryTo = GenerateNode(toValue, 0, true, isSimplify);
            }
            if (entryTo != nullptr) {
                Edge *edge = (it.type_ == EdgeType::ELEMENT) ?
                    Edge::NewEdge(chunk_, type, entryFrom, entryTo, it.index_) :
                    Edge::NewEdge(chunk_, type, entryFrom, entryTo, GetString(it.name_));
                RenameFunction(it.name_, entryFrom, entryTo);
                InsertEdgeUnique(edge);
                entryFrom->IncEdgeCount();  // Update Node's edgeCount_ here
            }
        }
    }
    LOG_ECMA(INFO) << "HeapSnapshot::FillEdges exit, nodeCount: " << nodeCount_ << ", edgeCount: " << edgeCount_;
}

void HeapSnapshot::RenameFunction(const CString &edgeName, Node *entryFrom, Node *entryTo)
{
    if (edgeName != "name") {
        return;
    }
    if (entryFrom->GetType() != NodeType::CLOSURE) {
        return;
    }
    if (*entryFrom->GetName() == "JSFunction" && *entryTo->GetName() != "" &&
        *entryTo->GetName() != "InternalAccessor") {
        entryFrom->SetName(GetString(*entryTo->GetName()));
    }
}

CString HeapSnapshot::ParseFunctionName(TaggedObject *obj, bool isRawHeap)
{
    CString result;
    JSFunctionBase *func = JSFunctionBase::Cast(obj);
    JSThread *thread = vm_->GetAssociatedJSThread();
    Method *method = Method::Cast(func->GetMethod(thread).GetTaggedObject());
    MethodLiteral *methodLiteral = method->GetMethodLiteral(thread);
    if (methodLiteral == nullptr) {
        if (!isRawHeap) {
            return "JSFunction";
        }
        JSHandle<JSFunctionBase> funcBase = JSHandle<JSFunctionBase>(thread, obj);
        auto funcName = JSFunction::GetFunctionName(thread, funcBase);
        if (funcName->IsString()) {
            auto name = EcmaStringAccessor(JSHandle<EcmaString>::Cast(funcName)).ToCString(thread);
            return name.empty() ? "JSFunction" : name;
        }
        return "JSFunction";
    }
    const JSPandaFile *jsPandaFile = method->GetJSPandaFile(thread);
    panda_file::File::EntityId methodId = methodLiteral->GetMethodId();
    const CString &nameStr = MethodLiteral::ParseFunctionNameToCString(jsPandaFile, methodId);
    const CString &moduleStr = method->GetRecordNameStr(thread);
    CString defaultName = "anonymous";
    DebugInfoExtractor *debugExtractor =
        JSPandaFileManager::GetInstance()->GetJSPtExtractor(jsPandaFile);
    if (debugExtractor == nullptr) {
        if (nameStr.empty()) {
            return defaultName;
        } else {
            return nameStr;
        }
    }
    // fileName: module|referencedModule|version/filePath
    CString fileName = CString(debugExtractor->GetSourceFile(methodId));
    int32_t line = debugExtractor->GetFristLine(methodId);
    return JSObject::ExtractFilePath(vm_->GetAssociatedJSThread(), nameStr, moduleStr, defaultName, fileName, line);
}

const CString HeapSnapshot::ParseObjectName(TaggedObject *obj)
{
    ASSERT(JSTaggedValue(obj).IsJSObject());
    JSThread *thread = vm_->GetAssociatedJSThread();
    bool isCallGetter = false;
    return JSObject::ExtractConstructorAndRecordName(thread, obj, true, &isCallGetter);
}

Node *HeapSnapshot::InsertNodeUnique(Node *node)
{
    AccumulateNodeSize(node->GetSelfSize());
    nodes_.emplace_back(node);
    nodeCount_++;
    return node;
}

void HeapSnapshot::EraseNodeUnique(Node *node)
{
    auto iter = std::find(nodes_.begin(), nodes_.end(), node);
    if (iter != nodes_.end()) {
        DecreaseNodeSize(node->GetSelfSize());
        chunk_.Delete(node);
        nodes_.erase(iter);
        nodeCount_--;
    }
}

Edge *HeapSnapshot::InsertEdgeUnique(Edge *edge)
{
    edges_.emplace_back(edge);
    edgeCount_++;
    return edge;
}

void HeapSnapshot::AddSyntheticRoot()
{
    LOG_ECMA(INFO) << "HeapSnapshot::AddSyntheticRoot";
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "HeapSnapshot::AddSyntheticRoot", "");
    Node *syntheticRoot = Node::NewNode(chunk_, 1, nodeCount_, GetString("SyntheticRoot"),
                                        NodeType::SYNTHETIC, 0, 0, 0);
    InsertNodeAt(0, syntheticRoot);
    CUnorderedSet<JSTaggedType> values {};
    CVector<Edge *> rootEdges;

    HandleRoots(syntheticRoot, values, rootEdges);

    // add root edges to edges begin
    edges_.insert(edges_.begin(), rootEdges.begin(), rootEdges.end());
    edgeCount_ += rootEdges.size();
    int reindex = 0;
    for (Node *node : nodes_) {
        node->SetIndex(reindex);
        reindex++;
    }
}

void HeapSnapshot::NewRootEdge(Node *syntheticRoot, JSTaggedValue value,
                               CUnorderedSet<JSTaggedType> &values, CVector<Edge *> &rootEdges)
{
    if (!value.IsHeapObject()) {
        return;
    }
    Node *rootNode = entryMap_.FindEntry(value.GetRawData());
    if (rootNode != nullptr) {
        JSTaggedType valueTo = value.GetRawData();
        if (values.insert(valueTo).second) {
            Edge *edge = Edge::NewEdge(chunk_, EdgeType::SHORTCUT, syntheticRoot, rootNode, GetString("-subroot-"));
            rootEdges.emplace_back(edge);
            syntheticRoot->IncEdgeCount();
        }
    }
}

void HeapSnapshot::HandleRoots(Node *syntheticRoot, CUnorderedSet<JSTaggedType> &values, CVector<Edge *> &rootEdges)
{
    class EdgeBuilderRootVisitor final : public RootVisitor {
    public:
        explicit EdgeBuilderRootVisitor(HeapSnapshot &snapshot, Node *syntheticRoot,
                                        CVector<Edge *> &rootEdges, CUnorderedSet<JSTaggedType> &values)
            : snapshot_(snapshot), syntheticRoot_(syntheticRoot), rootEdges_(rootEdges), values_(values) {}
        ~EdgeBuilderRootVisitor() = default;

        void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override
        {
            snapshot_.NewRootEdge(syntheticRoot_, JSTaggedValue(slot.GetTaggedType()), values_, rootEdges_);
        }

        void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override
        {
            for (ObjectSlot slot = start; slot < end; ++slot) {
                snapshot_.NewRootEdge(syntheticRoot_, JSTaggedValue(slot.GetTaggedType()), values_, rootEdges_);
            }
        }

        void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
            [[maybe_unused]] ObjectSlot derived, [[maybe_unused]] uintptr_t baseOldObject) override {}

    private:
        HeapSnapshot &snapshot_;
        Node *syntheticRoot_;
        CVector<Edge *> &rootEdges_;
        CUnorderedSet<JSTaggedType> &values_;
    };

#if defined(ENABLE_LOCAL_HANDLE_LEAK_DETECT)
    class EdgeBuilderWithLeakDetectRootVisitor final : public RootVisitor {
    public:
        explicit EdgeBuilderWithLeakDetectRootVisitor(HeapSnapshot &snapshot, Node *syntheticRoot,
                                                      CVector<Edge *> &rootEdges, CUnorderedSet<JSTaggedType> &values)
            : snapshot_(snapshot), syntheticRoot_(syntheticRoot), rootEdges_(rootEdges), values_(values) {}
        ~EdgeBuilderWithLeakDetectRootVisitor() = default;

        void VisitRoot([[maybe_unused]] Root type, ObjectSlot slot) override
        {
            snapshot_.LogLeakedLocalHandleBackTrace(slot);
            snapshot_.NewRootEdge(syntheticRoot_, JSTaggedValue(slot.GetTaggedType()), values_, rootEdges_);
        }

        void VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) override
        {
            for (ObjectSlot slot = start; slot < end; slot++) {
                snapshot_.LogLeakedLocalHandleBackTrace(slot);
                snapshot_.NewRootEdge(syntheticRoot_, JSTaggedValue(slot.GetTaggedType()), values_, rootEdges_);
            }
        }

        void VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
            [[maybe_unused]] ObjectSlot derived, [[maybe_unused]] uintptr_t baseOldObject) override {}
    private:
        HeapSnapshot &snapshot_;
        Node *syntheticRoot_;
        CVector<Edge *> &rootEdges_;
        CUnorderedSet<JSTaggedType> &values_;
    };

    auto heapProfiler = reinterpret_cast<HeapProfiler *>(HeapProfilerInterface::GetInstance(const_cast<EcmaVM *>(vm_)));
    bool needLeakDetect = !heapProfiler->IsStartLocalHandleLeakDetect() && heapProfiler->GetLeakStackTraceFd() > 0;
    if (needLeakDetect) {
        std::ostringstream buffer;
        buffer << "========================== Local Handle Leak Detection Result ==========================\n";
        heapProfiler->WriteToLeakStackTraceFd(buffer);
        EdgeBuilderWithLeakDetectRootVisitor visitor(*this, syntheticRoot, rootEdges, values);
        rootVisitor_.VisitHeapRoots(vm_->GetJSThread(), visitor);
        buffer << "======================== End of Local Handle Leak Detection Result =======================";
        heapProfiler->WriteToLeakStackTraceFd(buffer);
        heapProfiler->CloseLeakStackTraceFd();
    } else {
        EdgeBuilderRootVisitor visitor(*this, syntheticRoot, rootEdges, values);
        rootVisitor_.VisitHeapRoots(vm_->GetJSThread(), visitor);
    }
    heapProfiler->ClearHandleBackTrace();
#else
    EdgeBuilderRootVisitor visitor(*this, syntheticRoot, rootEdges, values);
    rootVisitor_.VisitHeapRoots(vm_->GetJSThread(), visitor);
#endif  // ENABLE_LOCAL_HANDLE_LEAK_DETECT
}

void HeapSnapshot::LogLeakedLocalHandleBackTrace(ObjectSlot slot)
{
    auto heapProfiler = reinterpret_cast<HeapProfiler *>(HeapProfilerInterface::GetInstance(const_cast<EcmaVM *>(vm_)));
    if (!heapProfiler->GetBackTraceOfHandle(slot.SlotAddress()).empty()) {
        Node *rootNode = entryMap_.FindEntry(slot.GetTaggedType());
        if (rootNode != nullptr && heapProfiler->GetLeakStackTraceFd() > 0) {
            std::ostringstream buffer;
            buffer << "NodeId: " << rootNode->GetId() << "\n"
                   << heapProfiler->GetBackTraceOfHandle(slot.SlotAddress()) << "\n";
            heapProfiler->WriteToLeakStackTraceFd(buffer);
        }
    }
}

void HeapSnapshot::LogLeakedLocalHandleBackTrace(common::RefField<> &refField)
{
    auto heapProfiler = reinterpret_cast<HeapProfiler *>(HeapProfilerInterface::GetInstance(const_cast<EcmaVM *>(vm_)));
    if (!heapProfiler->GetBackTraceOfHandle(reinterpret_cast<uintptr_t>(&refField)).empty()) {
        Node *rootNode = entryMap_.FindEntry(reinterpret_cast<JSTaggedType>(refField.GetTargetObject()));
        if (rootNode != nullptr && heapProfiler->GetLeakStackTraceFd() > 0) {
            std::ostringstream buffer;
            buffer << "NodeId: " << rootNode->GetId() << "\n"
                   << heapProfiler->GetBackTraceOfHandle(reinterpret_cast<uintptr_t>(&refField)) << "\n";
            heapProfiler->WriteToLeakStackTraceFd(buffer);
        }
    }
}

Node *HeapSnapshot::InsertNodeAt(size_t pos, Node *node)
{
    ASSERT(node != nullptr);
    auto iter = nodes_.begin();
    std::advance(iter, static_cast<int>(pos));
    nodes_.insert(iter, node);
    nodeCount_++;
    return node;
}

Edge *HeapSnapshot::InsertEdgeAt(size_t pos, Edge *edge)
{
    ASSERT(edge != nullptr);
    auto iter = edges_.begin();
    std::advance(iter, static_cast<int>(pos));
    edges_.insert(iter, edge);
    edgeCount_++;
    return edge;
}

CString EntryVisitor::ConvertKey(JSThread *thread, JSTaggedValue key)
{
    ASSERT(key.GetTaggedObject() != nullptr);
    EcmaString *keyString = EcmaString::Cast(key.GetTaggedObject());

    if (key.IsSymbol()) {
        JSSymbol *symbol = JSSymbol::Cast(key.GetTaggedObject());
        keyString = EcmaString::Cast(symbol->GetDescription(thread).GetTaggedObject());
    }
    // convert, expensive but safe
    return EcmaStringAccessor(keyString).ToCString(thread, StringConvertedUsage::PRINT);
}

Node *HeapEntryMap::FindOrInsertNode(Node *node)
{
    ASSERT(node != nullptr);
    auto it = nodesMap_.find(node->GetAddress());
    if (it != nodesMap_.end()) {
        return it->second;
    }
    InsertEntry(node);
    return node;
}

Node *HeapEntryMap::FindAndEraseNode(JSTaggedType addr)
{
    auto it = nodesMap_.find(addr);
    if (it != nodesMap_.end()) {
        Node *node = it->second;
        nodesMap_.erase(it);
        return node;
    }
    return nullptr;
}

Node *HeapEntryMap::FindEntry(JSTaggedType addr)
{
    auto it = nodesMap_.find(addr);
    return it != nodesMap_.end() ? it->second : nullptr;
}

void HeapEntryMap::InsertEntry(Node *node)
{
    nodesMap_.emplace(node->GetAddress(), node);
}
}  // namespace panda::ecmascript
