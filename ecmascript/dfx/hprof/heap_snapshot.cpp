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

#include <functional>

#include "ecmascript/dfx/hprof/heap_root_visitor.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/global_dictionary.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_symbol.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/property_attributes.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/jspandafile/js_pandafile_manager.h"

namespace panda::ecmascript {
CString *HeapSnapshot::GetString(const CString &as)
{
    return stringTable_.GetString(as);
}

CString *HeapSnapshot::GetArrayString(TaggedArray *array, const CString &as)
{
    CString arrayName = as;
    arrayName.append(ToCString(array->GetLength()));
    arrayName.append("]");
    return GetString(arrayName);  // String type was handled singly, see#GenerateStringNode
}

Node *Node::NewNode(Chunk *chunk, size_t id, size_t index, const CString *name, NodeType type, size_t size,
                    TaggedObject *entry, bool isLive)
{
    auto node = chunk->New<Node>(id, index, name, type, size, 0, NewAddress<TaggedObject>(entry), isLive);
    if (UNLIKELY(node == nullptr)) {
        LOG_FULL(FATAL) << "internal allocator failed";
        UNREACHABLE();
    }
    return node;
}

Edge *Edge::NewEdge(Chunk *chunk, uint32_t id, EdgeType type, Node *from, Node *to, CString *name)
{
    auto edge = chunk->New<Edge>(id, type, from, to, name);
    if (UNLIKELY(edge == nullptr)) {
        LOG_FULL(FATAL) << "internal allocator failed";
        UNREACHABLE();
    }
    return edge;
}

Edge *Edge::NewEdge(Chunk *chunk, uint32_t id, EdgeType type, Node *from, Node *to, uint32_t index)
{
    auto edge = chunk->New<Edge>(id, type, from, to, index);
    if (UNLIKELY(edge == nullptr)) {
        LOG_FULL(FATAL) << "internal allocator failed";
        UNREACHABLE();
    }
    return edge;
}

HeapSnapshot::~HeapSnapshot()
{
    for (Node *node : nodes_) {
        chunk_->Delete(node);
    }
    for (Edge *edge : edges_) {
        chunk_->Delete(edge);
    }
    nodes_.clear();
    edges_.clear();
    traceInfoStack_.clear();
    stackInfo_.clear();
    scriptIdMap_.clear();
    methodToTraceNodeId_.clear();
    traceNodeIndex_.clear();
    entryIdMap_ = nullptr;
    chunk_ = nullptr;
}

bool HeapSnapshot::BuildUp()
{
    FillNodes(true);
    FillEdges();
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
            Node *node = entryMap_.FindAndEraseNode((*iter)->GetAddress());
            ASSERT(*iter == node);
            entryIdMap_->EraseId((*iter)->GetAddress());
            if (node != nullptr) {
                DecreaseNodeSize(node->GetSelfSize());
                chunk_->Delete(node);
            }
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
    stream->UpdateLastSeenObjectId(sequenceId, timeStampUs);
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

    Node *node = entryMap_.FindAndEraseNode(address);
    if (node != nullptr) {
        ASSERT(node->GetId() <= UINT_MAX);

        Node *oldNode = entryMap_.FindAndEraseNode(Node::NewAddress(forwardAddress));
        if (oldNode != nullptr) {
            EraseNodeUnique(oldNode);
        }

        // Size and name may change during its life for some types(such as string, array and etc).
        if (forwardAddress->GetClass() != nullptr) {
            node->SetName(GenerateNodeName(forwardAddress));
        }
        if (JSTaggedValue(forwardAddress).IsString()) {
            node->SetSelfSize(EcmaStringAccessor(forwardAddress).GetFlatStringSize());
        } else {
            node->SetSelfSize(size);
        }
        node->SetAddress(Node::NewAddress(forwardAddress));
        entryMap_.InsertEntry(node);
    } else {
        LOG_DEBUGGER(WARN) << "Untracked object moves from " << address << " to " << forwardAddress;
        int32_t sequenceId = -1;
        GenerateNode(JSTaggedValue(forwardAddress), size, sequenceId);
    }
}

// NOLINTNEXTLINE(readability-function-size)
CString *HeapSnapshot::GenerateNodeName(TaggedObject *entry)
{
    auto *hCls = entry->GetClass();
    JSType type = hCls->GetObjectType();
    switch (type) {
        case JSType::TAGGED_ARRAY:
            return GetArrayString(TaggedArray::Cast(entry), "TaggedArray[");
        case JSType::LEXICAL_ENV:
            return GetArrayString(TaggedArray::Cast(entry), "LexicalEnv[");
        case JSType::CONSTANT_POOL:
            return GetArrayString(TaggedArray::Cast(entry), "ConstantPool[");
        case JSType::TAGGED_DICTIONARY:
            return GetArrayString(TaggedArray::Cast(entry), "TaggedDict[");
        case JSType::AOT_LITERAL_INFO:
            return GetArrayString(TaggedArray::Cast(entry), "AOTLiteralInfo[");
        case JSType::VTABLE:
            return GetArrayString(TaggedArray::Cast(entry), "VTable[");
        case JSType::COW_TAGGED_ARRAY:
            return GetArrayString(TaggedArray::Cast(entry), "COWArray[");
        case JSType::HCLASS:
            return GetString("HiddenClass");
        case JSType::LINE_STRING:
        case JSType::CONSTANT_STRING:
        case JSType::TREE_STRING:
            return GetString("BaseString");
        case JSType::JS_OBJECT: {
            CString objName = CString("JSOBJECT(Ctor=");  // Ctor-name
            return GetString(objName);
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
            return GetString("JSFunction");
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
        case JSType::JS_REG_EXP:
            return GetString("Regexp");
        case JSType::JS_SET:
            return GetString("Set");
        case JSType::JS_MAP:
            return GetString("Map");
        case JSType::JS_WEAK_SET:
            return GetString("WeakSet");
        case JSType::JS_WEAK_MAP:
            return GetString("WeakMap");
        case JSType::JS_DATE:
            return GetString("Date");
        case JSType::JS_BOUND_FUNCTION:
            return GetString("Bound Function");
        case JSType::JS_ARRAY: {
            JSArray *jsArray = JSArray::Cast(entry);
            CString jsArrayName("JSArray[");
            jsArrayName.append(ToCString(jsArray->GetLength()));
            jsArrayName.append("]");
            return GetString(jsArrayName);
        }
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
        case JSType::JS_SET_ITERATOR:
            return GetString("SetIterator");
        case JSType::JS_REG_EXP_ITERATOR:
            return GetString("RegExpIterator");
        case JSType::JS_ARRAY_ITERATOR:
            return GetString("ArrayIterator");
        case JSType::JS_STRING_ITERATOR:
            return GetString("StringIterator");
        case JSType::JS_ARRAY_BUFFER:
            return GetString("ArrayBuffer");
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
                return GetString("ProtoTypeHandler");
            case JSType::TRANSITION_HANDLER:
                return GetString("TransitionHandler");
            case JSType::TRANS_WITH_PROTO_HANDLER:
                return GetString("TransWithProtoHandler");
            case JSType::STORE_TS_HANDLER:
                return GetString("StoreTSHandler");
            case JSType::PROTO_CHANGE_MARKER:
                return GetString("ProtoChangeMarker");
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
            case JSType::TS_OBJECT_TYPE:
                return GetString("TSObjectType");
            case JSType::TS_INTERFACE_TYPE:
                return GetString("TSInterfaceType");
            case JSType::TS_CLASS_TYPE:
                return GetString("TSClassType");
            case JSType::TS_UNION_TYPE:
                return GetString("TSUnionType");
            case JSType::TS_CLASS_INSTANCE_TYPE:
                return GetString("TSClassInstanceType");
            case JSType::TS_FUNCTION_TYPE:
                return GetString("TSFunctionType");
            case JSType::TS_ARRAY_TYPE:
                return GetString("TSArrayType");
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
    NodeType nodeType = NodeType::INVALID;
    auto *hCls = entry->GetClass();
    if (hCls->IsTaggedArray()) {
        nodeType = NodeType::JS_ARRAY;
    } else if (hCls->IsHClass()) {
        nodeType = NodeType::HCLASS;
    } else {
        nodeType = NodeType(hCls->GetObjectType());
    }
    return nodeType;
}

void HeapSnapshot::FillNodes(bool isInFinish)
{
    // Iterate Heap Object
    auto heap = vm_->GetHeap();
    if (heap != nullptr) {
        heap->IterateOverObjects([this, isInFinish](TaggedObject *obj) {
            GenerateNode(JSTaggedValue(obj), 0, isInFinish);
        });
    }
}

Node *HeapSnapshot::GenerateNode(JSTaggedValue entry, size_t size, bool isInFinish)
{
    Node *node = nullptr;
    if (entry.IsHeapObject()) {
        if (entry.IsWeak()) {
            entry.RemoveWeakTag();
        }
        if (entry.IsString()) {
            if (isPrivate_) {
                node = GeneratePrivateStringNode(size);
            } else {
                node = GenerateStringNode(entry, size, isInFinish);
            }
            if (node == nullptr) {
                LOG_ECMA(DEBUG) << "string node nullptr";
            }
            return node;
        }
        TaggedObject *obj = entry.GetTaggedObject();
        auto *baseClass = obj->GetClass();
        if (baseClass != nullptr) {
            Address addr = reinterpret_cast<Address>(obj);
            Node *existNode = entryMap_.FindEntry(addr);  // Fast Index
            auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
            if (existNode == nullptr) {
                size_t selfSize = (size != 0) ? size : obj->GetClass()->SizeFromJSHClass(obj);
                node = Node::NewNode(chunk_, sequenceId, nodeCount_, GenerateNodeName(obj), GenerateNodeType(obj),
                    selfSize, obj);
                entryMap_.InsertEntry(node);
                if (!idExist) {
                    entryIdMap_->InsertId(addr, sequenceId);
                }
                InsertNodeUnique(node);
                ASSERT(entryMap_.FindEntry(node->GetAddress())->GetAddress() == node->GetAddress());
            } else {
                existNode->SetLive(true);
                return existNode;
            }
        }
    } else {
        CString primitiveName;
        if (entry.IsInt()) {
            if (!captureNumericValue_) {
                return nullptr;
            }
            primitiveName.append("Int:");
            if (!isPrivate_) {
                primitiveName.append(ToCString(entry.GetInt()));
            }
        } else if (entry.IsDouble()) {
            if (!captureNumericValue_) {
                return nullptr;
            }
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

        // A primitive value with tag will be regarded as a pointer
        auto *obj = reinterpret_cast<TaggedObject *>(entry.GetRawData());
        Node *existNode = entryMap_.FindEntry(reinterpret_cast<Address>(obj));
        if (existNode != nullptr) {
            existNode->SetLive(true);
            return existNode;
        }
        Address addr = reinterpret_cast<Address>(obj);
        auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
        node = Node::NewNode(chunk_, sequenceId, nodeCount_, GetString(primitiveName), NodeType::JS_PRIMITIVE_REF, 0,
                             obj);
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

Node *HeapSnapshot::GenerateStringNode(JSTaggedValue entry, size_t size, bool isInFinish)
{
    static const CString EMPTY_STRING;

    Node *existNode = entryMap_.FindEntry(reinterpret_cast<Address>(entry.GetTaggedObject()));  // Fast Index
    if (existNode != nullptr) {
        if (isInFinish) {
            existNode->SetName(GetString(EntryVisitor::ConvertKey(entry)));
        }
        existNode->SetLive(true);
        return existNode;
    }
    // Allocation Event will generate string node for "".
    // When we need to serialize and isFinish is true, the nodeName will be given the actual string content.
    auto originStr = static_cast<EcmaString *>(entry.GetTaggedObject());
    size_t selfsize = (size != 0) ? size : EcmaStringAccessor(originStr).GetFlatStringSize();
    const CString *nodeName = &EMPTY_STRING;
    if (isInFinish) {
        nodeName = GetString(EntryVisitor::ConvertKey(entry));
    }
    Address addr = reinterpret_cast<Address>(entry.GetTaggedObject());
    auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
    Node *node = Node::NewNode(chunk_, sequenceId, nodeCount_, nodeName, NodeType::PRIM_STRING, selfsize,
                               entry.GetTaggedObject());
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
    Node *node = nullptr;
    JSTaggedValue stringValue = vm_->GetJSThread()->GlobalConstants()->GetStringString();
    auto originStr = static_cast<EcmaString *>(stringValue.GetTaggedObject());
    size_t selfsize = (size != 0) ? size : EcmaStringAccessor(originStr).GetFlatStringSize();
    CString strContent;
    strContent.append(EntryVisitor::ConvertKey(stringValue));
    Address addr = reinterpret_cast<Address>(stringValue.GetTaggedObject());
    auto [idExist, sequenceId] = entryIdMap_->FindId(addr);
    node = Node::NewNode(chunk_, sequenceId, nodeCount_, GetString(strContent), NodeType::PRIM_STRING, selfsize,
                         stringValue.GetTaggedObject());
    Node *existNode = entryMap_.FindOrInsertNode(node);  // Fast Index
    if (existNode == node) {
        if (!idExist) {
            entryIdMap_->InsertId(addr, sequenceId);
        }
        entryMap_.InsertEntry(node);
        InsertNodeUnique(node);
    } else {
        existNode->SetLive(true);
    }
    ASSERT(entryMap_.FindEntry(node->GetAddress())->GetAddress() == node->GetAddress());
    if (existNode != node) {
        if ((node->GetAddress() == existNode->GetAddress()) && (existNode->GetName()->empty())) {
            existNode->SetName(GetString(strContent));
        }
        const_cast<NativeAreaAllocator *>(vm_->GetNativeAreaAllocator())->Delete(node);
        return nullptr;
    }
    privateStringNode_ = node;
    return node;
}

void HeapSnapshot::FillEdges()
{
    size_t length = nodes_.size();
    auto iter = nodes_.begin();
    size_t count = 0;
    while (++count < length) {
        ASSERT(*iter != nullptr);
        auto entryFrom = *iter;
        auto *objFrom = reinterpret_cast<TaggedObject *>(entryFrom->GetAddress());
        std::vector<Reference> referenceResources;
        JSTaggedValue objValue(objFrom);
        objValue.DumpForSnapshot(referenceResources, isVmMode_);
        for (auto const &it : referenceResources) {
            JSTaggedValue toValue = it.value_;
            if (toValue.IsNumber() && !captureNumericValue_) {
                continue;
            }
            Node *entryTo = nullptr;
            if (toValue.IsWeak()) {
                toValue.RemoveWeakTag();
            }
            if (toValue.IsHeapObject()) {
                auto *to = reinterpret_cast<TaggedObject *>(toValue.GetTaggedObject());
                entryTo = entryMap_.FindEntry(Node::NewAddress(to));
            }
            if (entryTo == nullptr) {
                entryTo = GenerateNode(toValue, 0, true);
            }
            if (entryTo != nullptr) {
                Edge *edge = (it.type_ == Reference::ReferenceType::ELEMENT) ?
                    Edge::NewEdge(chunk_, edgeCount_, (EdgeType)it.type_, entryFrom, entryTo, it.index_) :
                    Edge::NewEdge(chunk_, edgeCount_, (EdgeType)it.type_, entryFrom, entryTo, GetString(it.name_));
                RenameFunction(it.name_, entryFrom, entryTo);
                InsertEdgeUnique(edge);
                (*iter)->IncEdgeCount();  // Update Node's edgeCount_ here
            }
        }
        iter++;
    }
    FillPrimitiveEdge(count, iter);
}

void HeapSnapshot::FillPrimitiveEdge(size_t count, CList<Node *>::iterator iter)
{
    size_t lengthExtend = nodes_.size();
    while (++count < lengthExtend) {
        ASSERT(*iter != nullptr);
        if ((*iter)->GetType() == NodeType::JS_PRIMITIVE_REF) {
            JSTaggedValue jsFrom(reinterpret_cast<TaggedObject *>((*iter)->GetAddress()));
            CString valueName;
            if (jsFrom.IsInt()) {
                valueName.append(ToCString(jsFrom.GetInt()));
            } else if (jsFrom.IsDouble()) {
                valueName.append(FloatToCString(jsFrom.GetDouble()));
            } else {
                valueName.append("NaN");
            }
            Edge *edge = Edge::NewEdge(chunk_, edgeCount_, EdgeType::DEFAULT, (*iter), (*iter), GetString(valueName));
            InsertEdgeUnique(edge);
            (*iter)->IncEdgeCount();  // Update Node's edgeCount_ here
        }
        iter++;
    }
}

void HeapSnapshot::RenameFunction(const CString &edgeName, Node *entryFrom, Node *entryTo)
{
    if (edgeName != "name") {
        return;
    }
    auto fromName = *entryFrom->GetName();
    if (*entryTo->GetName() != "" && (fromName == "JSFunctionBase" ||
        fromName == "JSFunction" || fromName == "PromiseValueThunkOrThrowerFunction" ||
        fromName == "JSGeneratorFunction" || fromName == "PromiseAllSettledElementFunction" ||
        fromName == "Bound Function" || fromName == "PromiseAnyRejectElementFunction" ||
        fromName == "PromiseReactionsFunction" || fromName == "PromiseExecutorFunction" ||
        fromName == "PromiseAllResolveElementFunction" || fromName == "AsyncFunction" ||
        fromName == "ProxyRevocFunction" || fromName == "AsyncFromSyncIterUnwarpFunction" ||
        fromName == "PromiseFinallyFunction" || fromName == "JSIntlBoundFunction" ||
        fromName == "JSAsyncGeneratorFunction" || fromName == "AsyncAwaitStatusFunction")) {
        entryFrom->SetName(GetString(*entryTo->GetName()));
    }
}

void HeapSnapshot::BridgeAllReferences()
{
    // This Function is Unused
    for (Edge *edge : edges_) {
        auto *from = reinterpret_cast<TaggedObject *>(edge->GetFrom()->GetAddress());
        auto *to = reinterpret_cast<TaggedObject *>(edge->GetTo()->GetAddress());
        if (!JSTaggedValue(from).IsECMAObject()) {
            continue;  // named it by other way
        }
        edge->SetName(GenerateEdgeName(from, to));
    }
}

CString *HeapSnapshot::GenerateEdgeName([[maybe_unused]] TaggedObject *from, [[maybe_unused]] TaggedObject *to)
{
    // This Function is Unused
    ASSERT(from != nullptr && from != to);
    return GetString("[]");  // unAnalysed
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
        chunk_->Delete(node);
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
    Node *syntheticRoot = Node::NewNode(chunk_, 1, nodeCount_, GetString("SyntheticRoot"),
                                        NodeType::SYNTHETIC, 0, nullptr);
    InsertNodeAt(0, syntheticRoot);

    int edgeOffset = 0;
// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ROOT_EDGE_BUILDER_CORE(type, slot)                                                            \
    do {                                                                                              \
        JSTaggedValue value((slot).GetTaggedType());                                                  \
        if (value.IsHeapObject()) {                                                                   \
            TaggedObject *root = value.GetTaggedObject();                                             \
            Node *rootNode = entryMap_.FindEntry(Node::NewAddress(root));                             \
            if (rootNode != nullptr) {                                                                \
                Edge *edge = Edge::NewEdge(chunk_,                                                    \
                    edgeCount_, EdgeType::SHORTCUT, syntheticRoot, rootNode, GetString("-subroot-")); \
                InsertEdgeAt(edgeOffset, edge);                                                       \
                edgeOffset++;                                                                         \
                syntheticRoot->IncEdgeCount();                                                        \
            }                                                                                         \
        }                                                                                             \
    } while (false)

    RootVisitor rootEdgeBuilder = [this, syntheticRoot, &edgeOffset]([[maybe_unused]] Root type, ObjectSlot slot) {
        ROOT_EDGE_BUILDER_CORE(type, slot);
    };
    RootBaseAndDerivedVisitor rootBaseEdgeBuilder = []
        ([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base, [[maybe_unused]] ObjectSlot derived,
         [[maybe_unused]] uintptr_t baseOldObject) {
    };

    RootRangeVisitor rootRangeEdgeBuilder = [this, syntheticRoot, &edgeOffset]([[maybe_unused]] Root type,
                                                                               ObjectSlot start, ObjectSlot end) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            ROOT_EDGE_BUILDER_CORE(type, slot);
        }
    };
#undef ROOT_EDGE_BUILDER_CORE
    rootVisitor_.VisitHeapRoots(vm_->GetJSThread(), rootEdgeBuilder, rootRangeEdgeBuilder, rootBaseEdgeBuilder);

    int reindex = 0;
    for (Node *node : nodes_) {
        node->SetIndex(reindex);
        reindex++;
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

Node *HeapEntryMap::FindAndEraseNode(Address addr)
{
    auto it = nodesMap_.find(addr);
    if (it != nodesMap_.end()) {
        Node *node = it->second;
        nodesMap_.erase(it);
        nodeEntryCount_--;
        return node;
    }
    return nullptr;
}

Node *HeapEntryMap::FindEntry(Address addr)
{
    auto it = nodesMap_.find(addr);
    return it != nodesMap_.end() ? it->second : nullptr;
}

void HeapEntryMap::InsertEntry(Node *node)
{
    nodeEntryCount_++;
    nodesMap_.emplace(node->GetAddress(), node);
}

FrontType NodeTypeConverter::Convert(NodeType type)
{
    FrontType  fType = FrontType::DEFAULT;
    if (type == NodeType::PROPERTY_BOX) {
        fType = FrontType::HIDDEN;
    } else if (type == NodeType::JS_ARRAY || type == NodeType::JS_TYPED_ARRAY) {
        fType = FrontType::ARRAY;
    } else if (type == NodeType::PRIM_STRING) {  // STRING
        fType = FrontType::STRING;
    } else if (type == NodeType::JS_OBJECT) {
        fType = FrontType::OBJECT;
    } else if (type >= NodeType::JS_FUNCTION_FIRST && type <= NodeType::JS_FUNCTION_LAST) {
        fType = FrontType::DEFAULT;
    } else if (type == NodeType::JS_BOUND_FUNCTION) {
        fType = FrontType::DEFAULT;
    } else if (type == NodeType::JS_FUNCTION_BASE) {
        fType = FrontType::DEFAULT;
    } else if (type == NodeType::JS_REG_EXP) {
        fType = FrontType::REGEXP;
    } else if (type == NodeType::SYMBOL) {
        fType = FrontType::SYMBOL;
    } else if (type == NodeType::JS_PRIMITIVE_REF) {
        fType = FrontType::HEAPNUMBER;
    } else if (type == NodeType::SYNTHETIC) {
        fType = FrontType::SYNTHETIC;
    } else {
        fType = FrontType::DEFAULT;
        // NATIVE,           /* kNative */
        // CONSSTRING,       /* kConsString */
        // SLICEDSTRING,     /* kSlicedString */
        // SYMBOL,           /* kSymbol */
        // BIGINT,           /* kBigInt */
    }
    return fType;
}
}  // namespace panda::ecmascript
