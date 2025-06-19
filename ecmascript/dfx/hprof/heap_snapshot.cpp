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


#include "ecmascript/ecma_string-inl.h"

namespace panda::ecmascript {
CString *HeapSnapshot::GetString(const CString &as)
{
    return stringTable_->GetString(as);
}

CString *HeapSnapshot::GetArrayString(TaggedArray *array, const CString &as)
{
    CString arrayName = as;
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
    for (Node *node : nodes_) {
        node->SetLive(false);
    }
    FillNodes(isInFinish);
    for (auto iter = nodes_.begin(); iter != nodes_.end();) {
        if (!(*iter)->IsLive()) {
            entryMap_.FindAndEraseNode((*iter)->GetAddress());
            entryIdMap_->EraseId((*iter)->GetAddress());
            DecreaseNodeSize((*iter)->GetSelfSize());
            chunk_.Delete(*iter);
            iter = nodes_.erase(iter);
            nodeCount_--;
        } else {
            iter++;
        }
    }
}

bool HeapSnapshot::FinishSnapshot()
{
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
    switch (type) {
        case JSType::TAGGED_ARRAY:
        case JSType::JS_SHARED_TYPED_ARRAY:
            return GetArrayString(TaggedArray::Cast(entry), "ArkInternalArray[");
        case JSType::LEXICAL_ENV:
            return GetArrayString(TaggedArray::Cast(entry), "LexicalEnv[");    
        case JSType::SFUNCTION_ENV:
            return GetArrayString(TaggedArray::Cast(entry), "SFunctionEnv[");
        case JSType::SENDABLE_ENV:
            return GetArrayString(TaggedArray::Cast(entry), "SendableEnv[");
        case JSType::CONSTANT_POOL:
            return GetArrayString(TaggedArray::Cast(entry), "ArkInternalConstantPool[");
        case JSType::PROFILE_TYPE_INFO:
            return GetArrayString(TaggedArray::Cast(entry), "ArkInternalProfileTypeInfo[");
        case JSType::TAGGED_DICTIONARY:
            return GetArrayString(TaggedArray::Cast(entry), "ArkInternalDict[");
        case JSType::AOT_LITERAL_INFO:
            return GetArrayString(TaggedArray::Cast(entry), "ArkInternalAOTLiteralInfo[");
        case JSType::VTABLE:
            return GetArrayString(TaggedArray::Cast(entry), "ArkInternalVTable[");
        case JSType::COW_TAGGED_ARRAY:
            return GetArrayString(TaggedArray::Cast(entry), "ArkInternalCOWArray[");
        case JSType::HCLASS:
            return GetString("HiddenClass(NonMovable)");
        case JSType::LINKED_NODE:
            return GetString("LinkedNode");
        case JSType::TRACK_INFO:
            return GetString("TrackInfo");
        case JSType::LINE_STRING:
        case JSType::TREE_STRING:
        case JSType::SLICED_STRING:
            return GetString("BaseString");
        case JSType::JS_OBJECT: {
            CString objName = CString("JSObject");  // Ctor-name
            return GetString(objName);
        }
        case JSType::JS_SHARED_OBJECT: {
            return GetString("JSSharedObject");
        }
        case JSType::JS_SHARED_FUNCTION: {
            return GetString("JSSharedFunction");
        }
        case JSType::FREE_OBJECT_WITH_ONE_FIELD:
        case JSType::FREE_OBJECT_WITH_NONE_FIELD:
        case JSType::FREE_OBJECT_WITH_TWO_FIELD:
            return GetString("FreeObject");
        case JSType::JS_NATIVE_POINTER:
            return GetString("JSNativePointer");
        case JSType::JS_FUNCTION_BASE:
            return GetString("JSFunctionBase");
        case JSType::JS_FUNCTION:
            return GetString(CString("JSFunction"));
        case JSType::FUNCTION_TEMPLATE:
            return GetString(CString("ArkInternalFunctionTemplate"));
        case JSType::JS_ERROR:
            return GetString("Error");
        case JSType::JS_EVAL_ERROR:
            return GetString("Eval Error");
        case JSType::JS_RANGE_ERROR:
            return GetString("Range Error");
        case JSType::JS_TYPE_ERROR:
            return GetString("Type Error");
        case JSType::JS_AGGREGATE_ERROR:
            return GetString("Aggregate Error");
        case JSType::JS_REFERENCE_ERROR:
            return GetString("Reference Error");
        case JSType::JS_URI_ERROR:
            return GetString("Uri Error");
        case JSType::JS_SYNTAX_ERROR:
            return GetString("Syntax Error");
        case JSType::JS_OOM_ERROR:
            return GetString("OutOfMemory Error");
        case JSType::JS_TERMINATION_ERROR:
            return GetString("Termination Error");
        case JSType::JS_REG_EXP:
            return GetString("Regexp");
        case JSType::JS_SET:
            return GetString("Set");
        case JSType::JS_SHARED_SET:
            return GetString("SharedSet");
        case JSType::JS_MAP:
            return GetString("Map");
        case JSType::JS_SHARED_MAP:
            return GetString("SharedMap");
        case JSType::JS_WEAK_SET:
            return GetString("WeakSet");
        case JSType::JS_WEAK_MAP:
            return GetString("WeakMap");
        case JSType::JS_DATE:
            return GetString("Date");
        case JSType::JS_BOUND_FUNCTION:
            return GetString("Bound Function");
        case JSType::JS_ARRAY:
            return GetString("JSArray");
        case JSType::JS_TYPED_ARRAY:
            return GetString("Typed Array");
        case JSType::JS_INT8_ARRAY:
            return GetString("Int8 Array");
        case JSType::JS_UINT8_ARRAY:
            return GetString("Uint8 Array");
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            return GetString("Uint8 Clamped Array");
        case JSType::JS_INT16_ARRAY:
            return GetString("Int16 Array");
        case JSType::JS_UINT16_ARRAY:
            return GetString("Uint16 Array");
        case JSType::JS_INT32_ARRAY:
            return GetString("Int32 Array");
        case JSType::JS_UINT32_ARRAY:
            return GetString("Uint32 Array");
        case JSType::JS_FLOAT32_ARRAY:
            return GetString("Float32 Array");
        case JSType::JS_FLOAT64_ARRAY:
            return GetString("Float64 Array");
        case JSType::JS_BIGINT64_ARRAY:
            return GetString("BigInt64 Array");
        case JSType::JS_BIGUINT64_ARRAY:
            return GetString("BigUint64 Array");
        case JSType::JS_ARGUMENTS:
            return GetString("Arguments");
        case JSType::BIGINT:
            return GetString("BigInt");
        case JSType::JS_PROXY:
            return GetString("Proxy");
        case JSType::JS_PRIMITIVE_REF:
            return GetString("Primitive");
        case JSType::JS_DATA_VIEW:
            return GetString("DataView");
        case JSType::JS_ITERATOR:
            return GetString("Iterator");
        case JSType::JS_FORIN_ITERATOR:
            return GetString("ForinInterator");
        case JSType::JS_MAP_ITERATOR:
            return GetString("MapIterator");
        case JSType::JS_SHARED_MAP_ITERATOR:
            return GetString("SharedMapIterator");
        case JSType::JS_SET_ITERATOR:
            return GetString("SetIterator");
        case JSType::JS_SHARED_SET_ITERATOR:
            return GetString("SharedSetIterator");
        case JSType::JS_REG_EXP_ITERATOR:
            return GetString("RegExpIterator");
        case JSType::JS_ARRAY_ITERATOR:
            return GetString("ArrayIterator");
        case JSType::JS_STRING_ITERATOR:
            return GetString("StringIterator");
        case JSType::JS_ARRAY_BUFFER:
            return GetString("ArrayBuffer");
        case JSType::JS_SENDABLE_ARRAY_BUFFER:
            return GetString("SendableArrayBuffer");
        case JSType::JS_SHARED_ARRAY:
            return GetString("SharedArray");
        case JSType::JS_SHARED_ARRAY_BUFFER:
            return GetString("SharedArrayBuffer");
        case JSType::JS_PROXY_REVOC_FUNCTION:
            return GetString("ProxyRevocFunction");
        case JSType::PROMISE_REACTIONS:
            return GetString("PromiseReaction");
        case JSType::PROMISE_CAPABILITY:
            return GetString("PromiseCapability");
        case JSType::ASYNC_GENERATOR_REQUEST:
            return GetString("AsyncGeneratorRequest");
        case JSType::PROMISE_ITERATOR_RECORD:
            return GetString("PromiseIteratorRecord");
        case JSType::PROMISE_RECORD:
            return GetString("PromiseRecord");
        case JSType::RESOLVING_FUNCTIONS_RECORD:
            return GetString("ResolvingFunctionsRecord");
        case JSType::JS_PROMISE:
            return GetString("Promise");
        case JSType::JS_PROMISE_REACTIONS_FUNCTION:
            return GetString("PromiseReactionsFunction");
        case JSType::JS_PROMISE_EXECUTOR_FUNCTION:
            return GetString("PromiseExecutorFunction");
        case JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION:
            return GetString("AsyncModuleFulfilledFunction");
        case JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION:
            return GetString("AsyncModuleRejectedFunction");
        case JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION:
            return GetString("AsyncFromSyncIterUnwarpFunction");
        case JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION:
            return GetString("PromiseAllResolveElementFunction");
        case JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION:
            return GetString("PromiseAnyRejectElementFunction");
        case JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION:
            return GetString("PromiseAllSettledElementFunction");
        case JSType::JS_PROMISE_FINALLY_FUNCTION:
            return GetString("PromiseFinallyFunction");
        case JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION:
            return GetString("PromiseValueThunkOrThrowerFunction");
        case JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN:
            return GetString("AsyncGeneratorResumeNextReturnProcessorRstFtn");
        case JSType::JS_GENERATOR_FUNCTION:
            return GetString("JSGeneratorFunction");
        case JSType::JS_ASYNC_GENERATOR_FUNCTION:
            return GetString("JSAsyncGeneratorFunction");
        case JSType::SYMBOL:
            return GetString("Symbol");
        case JSType::JS_ASYNC_FUNCTION:
            return GetString("AsyncFunction");
        case JSType::JS_SHARED_ASYNC_FUNCTION:
            return GetString("SharedAsyncFunction");
        case JSType::JS_INTL_BOUND_FUNCTION:
            return GetString("JSIntlBoundFunction");
        case JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION:
            return GetString("AsyncAwaitStatusFunction");
        case JSType::JS_ASYNC_FUNC_OBJECT:
            return GetString("AsyncFunctionObject");
        case JSType::JS_REALM:
            return GetString("Realm");
        case JSType::JS_GLOBAL_OBJECT:
            return GetString("GlobalObject");
        case JSType::JS_INTL:
            return GetString("JSIntl");
        case JSType::JS_LOCALE:
            return GetString("JSLocale");
        case JSType::JS_DATE_TIME_FORMAT:
            return GetString("JSDateTimeFormat");
        case JSType::JS_RELATIVE_TIME_FORMAT:
            return GetString("JSRelativeTimeFormat");
        case JSType::JS_NUMBER_FORMAT:
            return GetString("JSNumberFormat");
        case JSType::JS_COLLATOR:
            return GetString("JSCollator");
        case JSType::JS_PLURAL_RULES:
            return GetString("JSPluralRules");
        case JSType::JS_DISPLAYNAMES:
            return GetString("JSDisplayNames");
        case JSType::JS_SEGMENTER:
            return GetString("JSSegmenter");
        case JSType::JS_SEGMENTS:
            return GetString("JSSegments");
        case JSType::JS_SEGMENT_ITERATOR:
            return GetString("JSSegmentIterator");
        case JSType::JS_LIST_FORMAT:
            return GetString("JSListFormat");
        case JSType::JS_GENERATOR_OBJECT:
            return GetString("JSGeneratorObject");
        case JSType::JS_ASYNC_GENERATOR_OBJECT:
            return GetString("JSAsyncGeneratorObject");
        case JSType::JS_GENERATOR_CONTEXT:
            return GetString("JSGeneratorContext");
        case JSType::ACCESSOR_DATA:
            return GetString("AccessorData");
        case JSType::INTERNAL_ACCESSOR:
            return GetString("InternalAccessor");
        case JSType::MICRO_JOB_QUEUE:
            return GetString("MicroJobQueue");
        case JSType::PENDING_JOB:
            return GetString("PendingJob");
        case JSType::COMPLETION_RECORD:
            return GetString("CompletionRecord");
        case JSType::JS_API_ARRAY_LIST:
            return GetString("ArrayList");
        case JSType::JS_API_ARRAYLIST_ITERATOR:
            return GetString("ArrayListIterator");
        case JSType::JS_API_HASH_MAP:
            return GetString("HashMap");
        case JSType::JS_API_HASH_SET:
            return GetString("HashSet");
        case JSType::JS_API_HASHMAP_ITERATOR:
            return GetString("HashMapIterator");
        case JSType::JS_API_HASHSET_ITERATOR:
            return GetString("HashSetIterator");
        case JSType::JS_API_LIGHT_WEIGHT_MAP:
            return GetString("LightWeightMap");
        case JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR:
            return GetString("LightWeightMapIterator");
        case JSType::JS_API_LIGHT_WEIGHT_SET:
            return GetString("LightWeightSet");
        case JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR:
            return GetString("LightWeightSetIterator");
        case JSType::JS_API_TREE_MAP:
            return GetString("TreeMap");
        case JSType::JS_API_TREE_SET:
            return GetString("TreeSet");
        case JSType::JS_API_TREEMAP_ITERATOR:
            return GetString("TreeMapIterator");
        case JSType::JS_API_TREESET_ITERATOR:
            return GetString("TreeSetIterator");
        case JSType::JS_API_VECTOR:
            return GetString("Vector");
        case JSType::JS_API_VECTOR_ITERATOR:
            return GetString("VectorIterator");
        case JSType::JS_API_BITVECTOR:
            return GetString("BitVector");
        case JSType::JS_API_BITVECTOR_ITERATOR:
            return GetString("BitVectorIterator");
        case JSType::JS_API_QUEUE:
            return GetString("Queue");
        case JSType::JS_API_QUEUE_ITERATOR:
            return GetString("QueueIterator");
        case JSType::JS_API_DEQUE:
            return GetString("Deque");
        case JSType::JS_API_DEQUE_ITERATOR:
            return GetString("DequeIterator");
        case JSType::JS_API_STACK:
            return GetString("Stack");
        case JSType::JS_API_STACK_ITERATOR:
            return GetString("StackIterator");
        case JSType::JS_API_LIST:
            return GetString("List");
        case JSType::JS_API_LINKED_LIST:
            return GetString("LinkedList");
        case JSType::SOURCE_TEXT_MODULE_RECORD:
            return GetString("SourceTextModule");
        case JSType::IMPORTENTRY_RECORD:
            return GetString("ImportEntry");
        case JSType::LOCAL_EXPORTENTRY_RECORD:
            return GetString("LocalExportEntry");
        case JSType::INDIRECT_EXPORTENTRY_RECORD:
            return GetString("IndirectExportEntry");
        case JSType::STAR_EXPORTENTRY_RECORD:
            return GetString("StarExportEntry");
        case JSType::RESOLVEDBINDING_RECORD:
            return GetString("ResolvedBinding");
        case JSType::RESOLVEDINDEXBINDING_RECORD:
            return GetString("ResolvedIndexBinding");
        case JSType::RESOLVEDRECORDINDEXBINDING_RECORD:
            return GetString("ResolvedRecordIndexBinding");
        case JSType::RESOLVEDRECORDBINDING_RECORD:
            return GetString("ResolvedRecordBinding");
        case JSType::JS_MODULE_NAMESPACE:
            return GetString("ModuleNamespace");
        case JSType::JS_API_PLAIN_ARRAY:
            return GetString("PlainArray");
        case JSType::JS_API_PLAIN_ARRAY_ITERATOR:
            return GetString("PlainArrayIterator");
        case JSType::JS_CJS_EXPORTS:
            return GetString("CJS Exports");
        case JSType::JS_CJS_MODULE:
            return GetString("CJS Module");
        case JSType::JS_CJS_REQUIRE:
            return GetString("CJS Require");
        case JSType::METHOD:
            return GetString("Method");
        case JSType::CELL_RECORD:
            return GetString("CellRecord");
        case JSType::JS_WEAK_REF:
            return GetString("WeakRef");
        case JSType::JS_FINALIZATION_REGISTRY:
            return GetString("JSFinalizationRegistry");
        case JSType::JS_ASYNCITERATOR:
            return GetString("AsyncIterator");
        case JSType::JS_ASYNC_FROM_SYNC_ITERATOR:
            return GetString("AsyncFromSyncIterator");
        case JSType::JS_API_LINKED_LIST_ITERATOR:
            return GetString("LinkedListIterator");
        case JSType::JS_API_LIST_ITERATOR:
            return GetString("ListIterator");
        case JSType::JS_SHARED_ARRAY_ITERATOR:
            return GetString("SharedArrayIterator");
        case JSType::JS_SHARED_INT8_ARRAY:
            return GetString("Shared Int8 Array");
        case JSType::JS_SHARED_UINT8_ARRAY:
            return GetString("Shared Uint8 Array");
        case JSType::JS_SHARED_UINT8_CLAMPED_ARRAY:
            return GetString("Shared Uint8 Clamped Array");
        case JSType::JS_SHARED_INT16_ARRAY:
            return GetString("Shared Int16 Array");
        case JSType::JS_SHARED_UINT16_ARRAY:
            return GetString("Shared Uint16 Array");
        case JSType::JS_SHARED_INT32_ARRAY:
            return GetString("Shared Int32 Array");
        case JSType::JS_SHARED_UINT32_ARRAY:
            return GetString("Shared Uint32 Array");
        case JSType::JS_SHARED_FLOAT32_ARRAY:
            return GetString("Shared Float32 Array");
        case JSType::JS_SHARED_FLOAT64_ARRAY:
            return GetString("Shared Float64 Array");
        case JSType::JS_SHARED_BIGINT64_ARRAY:
            return GetString("Shared BigInt64 Array");
        case JSType::JS_SHARED_BIGUINT64_ARRAY:
            return GetString("Shared BigUint64 Array");
        case JSType::NATIVE_MODULE_FAILURE_INFO:
            return GetString("NativeModuleFailureInfo");
        case JSType::MUTANT_TAGGED_ARRAY:
            return GetString("MutantTaggedArray");
        case JSType::BYTE_ARRAY:
            return GetString("ByteArray");
        case JSType::COW_MUTANT_TAGGED_ARRAY:
            return GetString("COWMutantTaggedArray");
        case JSType::RB_TREENODE:
            return GetString("RBTreeNode");
        case JSType::ENUM_CACHE:
            return GetString("EnumCache");
        case JSType::CLASS_LITERAL:
            return GetString("ClassLiteral");
        case JSType::ASYNC_ITERATOR_RECORD:
            return GetString("AsyncIteratorRecord");
        case JSType::MODULE_RECORD:
            return GetString("ModuleRecord");
        case JSType::PROFILE_TYPE_INFO_CELL_0:
        case JSType::PROFILE_TYPE_INFO_CELL_1:
        case JSType::PROFILE_TYPE_INFO_CELL_N:
            return GetString("ProfileTypeInfoCell");
        case JSType::EXTRA_PROFILE_TYPE_INFO:
            return GetString("ExtraProfileTypeInfo");
        default:
            break;
    }
    if (IsInVmMode()) {
        switch (type) {
            case JSType::PROPERTY_BOX:
                return GetString("PropertyBox");
            case JSType::GLOBAL_ENV:
                return GetString("GlobalEnv");
            case JSType::PROTOTYPE_HANDLER:
                return GetString("PrototypeHandler");
            case JSType::TRANSITION_HANDLER:
                return GetString("TransitionHandler");
            case JSType::TRANS_WITH_PROTO_HANDLER:
                return GetString("TransWithProtoHandler");
            case JSType::STORE_TS_HANDLER:
                return GetString("StoreAOTHandler");
            case JSType::PROTO_CHANGE_MARKER:
                return GetString("ProtoChangeMarker");
            case JSType::MARKER_CELL:
                return GetString("MarkerCell");
            case JSType::PROTOTYPE_INFO:
                return GetString("ProtoChangeDetails");
            case JSType::TEMPLATE_MAP:
                return GetString("TemplateMap");
            case JSType::PROGRAM:
                return GetString("Program");
            case JSType::MACHINE_CODE_OBJECT:
                return GetString("MachineCode");
            case JSType::CLASS_INFO_EXTRACTOR:
                return GetString("ClassInfoExtractor");
            default:
                break;
        }
    } else {
        return GetString("Hidden Object");
    }
    return GetString(CString("UnKnownType").append(std::to_string(static_cast<int>(type))));
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
    LOG_ECMA(INFO) << "HeapSnapshot::FillNodes";
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "HeapSnapshot::FillNodes", "");
    // Iterate Heap Object
    if (g_isEnableCMCGC) {
        common::Heap &heap = common::Heap::GetHeap();
        heap.ForEachObject([this, isInFinish, isSimplify](BaseObject *obj) {
            GenerateNode(JSTaggedValue(reinterpret_cast<JSTaggedType>(obj)), 0, isInFinish, isSimplify);
        }, isSimplify);
    } else {
        auto heap = vm_->GetHeap();
        if (heap != nullptr) {
            heap->IterateOverObjects([this, isInFinish, isSimplify](TaggedObject *obj) {
                GenerateNode(JSTaggedValue(obj), 0, isInFinish, isSimplify);
            }, isSimplify);
        }
    }
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
        MethodLiteral *methodLiteral = method->GetMethodLiteral();
        if (methodLiteral == nullptr) {
            continue;
        }
        if (stackInfo_.count(methodLiteral) == 0) {
            if (!AddMethodInfo(methodLiteral, method->GetJSPandaFile(), sequenceId)) {
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
    if (existNode != nullptr) {
        if (isInFinish || isBinMod) {
            existNode->SetName(GetString(EntryVisitor::ConvertKey(entry)));
        }
        existNode->SetLive(true);
        return existNode;
    }
    // Allocation Event will generate string node for "".
    // When we need to serialize and isFinish is true, the nodeName will be given the actual string content.
    size_t selfsize = (size != 0) ? size : entry.GetTaggedObject()->GetSize();
    const CString *nodeName = &EMPTY_STRING;
    if (isInFinish || isBinMod) {
        nodeName = GetString(EntryVisitor::ConvertKey(entry));
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
    strContent.append(EntryVisitor::ConvertKey(stringValue));
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
    auto iter = nodes_.begin();
    size_t count = 0;
    while (count++ < nodes_.size()) {
        ASSERT(*iter != nullptr);
        auto entryFrom = *iter;
        JSTaggedValue value(entryFrom->GetAddress());
        if (!value.IsHeapObject()) {
            iter++;
            continue;
        }
        std::vector<Reference> referenceResources;
        value.DumpForSnapshot(referenceResources, isVmMode_);
        for (auto const &it : referenceResources) {
            JSTaggedValue toValue = it.value_;
            if (toValue.IsNumber() && !captureNumericValue_) {
                continue;
            }
            Node *entryTo = nullptr;
            EdgeType type = toValue.IsWeak() ? EdgeType::WEAK : (EdgeType)it.type_;
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
                Edge *edge = (it.type_ == Reference::ReferenceType::ELEMENT) ?
                    Edge::NewEdge(chunk_, type, entryFrom, entryTo, it.index_) :
                    Edge::NewEdge(chunk_, type, entryFrom, entryTo, GetString(it.name_));
                RenameFunction(it.name_, entryFrom, entryTo);
                InsertEdgeUnique(edge);
                (*iter)->IncEdgeCount();  // Update Node's edgeCount_ here
            }
        }
        iter++;
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
    Method *method = Method::Cast(func->GetMethod().GetTaggedObject());
    MethodLiteral *methodLiteral = method->GetMethodLiteral();
    if (methodLiteral == nullptr) {
        if (!isRawHeap) {
            return "JSFunction";
        }
        JSHandle<JSFunctionBase> funcBase = JSHandle<JSFunctionBase>(vm_->GetJSThread(), obj);
        auto funcName = JSFunction::GetFunctionName(vm_->GetJSThread(), funcBase);
        if (funcName->IsString()) {
            auto name = EcmaStringAccessor(JSHandle<EcmaString>::Cast(funcName)).ToCString();
            return name.empty() ? "JSFunction" : name;
        }
        return "JSFunction";
    }
    const JSPandaFile *jsPandaFile = method->GetJSPandaFile();
    panda_file::File::EntityId methodId = methodLiteral->GetMethodId();
    const CString &nameStr = MethodLiteral::ParseFunctionNameToCString(jsPandaFile, methodId);
    const CString &moduleStr = method->GetRecordNameStr();
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
    return JSObject::ExtractFilePath(vm_->GetJSThread(), nameStr, moduleStr, defaultName, fileName, line);
}

const CString HeapSnapshot::ParseObjectName(TaggedObject *obj)
{
    ASSERT(JSTaggedValue(obj).IsJSObject());
    JSThread *thread = vm_->GetJSThread();
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
    CList<Edge *> rootEdges;

    if (g_isEnableCMCGC) {
        HandleCMCGCRoots(syntheticRoot, values, rootEdges);
    } else {
        HandleRoots(syntheticRoot, values, rootEdges);
    }

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
                               CUnorderedSet<JSTaggedType> &values, CList<Edge *> &rootEdges)
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

void HeapSnapshot::HandleCMCGCRoots(Node *syntheticRoot, CUnorderedSet<JSTaggedType> &values, CList<Edge *> &rootEdges)
{
    common::RefFieldVisitor visitor = [this, &syntheticRoot, &values, &rootEdges](common::RefField<> &refField) {
        NewRootEdge(syntheticRoot, JSTaggedValue(refField.GetFieldValue()), values, rootEdges);
    };
#if defined(ENABLE_LOCAL_HANDLE_LEAK_DETECT)
    auto heapProfiler = reinterpret_cast<HeapProfiler *>(HeapProfilerInterface::GetInstance(const_cast<EcmaVM *>(vm_)));
    bool needLeakDetect = !heapProfiler->IsStartLocalHandleLeakDetect() && heapProfiler->GetLeakStackTraceFd() > 0;
    if (needLeakDetect) {
        std::ostringstream buffer;
        buffer << "========================== Local Handle Leak Detection Result ==========================\n";
        heapProfiler->WriteToLeakStackTraceFd(buffer);
        common::RefFieldVisitor visitorWithDetect = [this, &syntheticRoot, &values, &rootEdges](
            common::RefField<> &refField) {
            LogLeakedLocalHandleBackTrace(refField);
            NewRootEdge(syntheticRoot, JSTaggedValue(refField.GetFieldValue()), values, rootEdges);
        };
        common::VisitRoots(visitorWithDetect, false);
        buffer << "======================== End of Local Handle Leak Detection Result =======================";
        heapProfiler->WriteToLeakStackTraceFd(buffer);
        heapProfiler->CloseLeakStackTraceFd();
    } else {
        common::VisitRoots(visitor, false);
    }
    heapProfiler->ClearHandleBackTrace();
#else
    common::VisitRoots(visitor, false);
#endif  // ENABLE_LOCAL_HANDLE_LEAK_DETECT
}

void HeapSnapshot::HandleRoots(Node *syntheticRoot, CUnorderedSet<JSTaggedType> &values, CList<Edge *> &rootEdges)
{
    class EdgeBuilderRootVisitor final : public RootVisitor {
    public:
        explicit EdgeBuilderRootVisitor(HeapSnapshot &snapshot, Node *syntheticRoot,
                                        CList<Edge *> &rootEdges, CUnorderedSet<JSTaggedType> &values)
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
        CList<Edge *> &rootEdges_;
        CUnorderedSet<JSTaggedType> &values_;
    };

#if defined(ENABLE_LOCAL_HANDLE_LEAK_DETECT)
    class EdgeBuilderWithLeakDetectRootVisitor final : public RootVisitor {
    public:
        explicit EdgeBuilderWithLeakDetectRootVisitor(HeapSnapshot &snapshot, Node *syntheticRoot,
                                                      CList<Edge *> &rootEdges, CUnorderedSet<JSTaggedType> &values)
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
        CList<Edge *> &rootEdges_;
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

CString EntryVisitor::ConvertKey(JSTaggedValue key)
{
    ASSERT(key.GetTaggedObject() != nullptr);
    EcmaString *keyString = EcmaString::Cast(key.GetTaggedObject());

    if (key.IsSymbol()) {
        JSSymbol *symbol = JSSymbol::Cast(key.GetTaggedObject());
        keyString = EcmaString::Cast(symbol->GetDescription().GetTaggedObject());
    }
    // convert, expensive but safe
    return EcmaStringAccessor(keyString).ToCString(StringConvertedUsage::PRINT);
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
