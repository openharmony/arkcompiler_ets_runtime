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
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/vtable.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/tagged_tree.h"
#ifdef ARK_SUPPORT_INTL
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_relative_time_format.h"
#include "ecmascript/js_segmenter.h"
#include "ecmascript/js_segments.h"
#include "ecmascript/js_segment_iterator.h"
#endif

namespace panda::ecmascript {
using MicroJobQueue = panda::ecmascript::job::MicroJobQueue;
using PendingJob = panda::ecmascript::job::PendingJob;
using ExtraProfileTypeInfo = panda::ecmascript::pgo::ExtraProfileTypeInfo;

static constexpr uint32_t DUMP_TYPE_OFFSET = 12;
static constexpr uint32_t DUMP_PROPERTY_OFFSET = 20;
static constexpr uint32_t DUMP_ELEMENT_OFFSET = 2;

CString JSHClass::DumpJSType(JSType type)
{
    switch (type) {
        case JSType::HCLASS:
            return "JSHClass";
        case JSType::TAGGED_ARRAY:
            return "TaggedArray";
        case JSType::LEXICAL_ENV:
            return "LexicalEnv";
        case JSType::SFUNCTION_ENV:
            return "SFunctionEnv";
        case JSType::SENDABLE_ENV:
            return "SendableEnv";
        case JSType::TAGGED_DICTIONARY:
            return "TaggedDictionary";
        case JSType::CONSTANT_POOL:
            return "ConstantPool";
        case JSType::PROFILE_TYPE_INFO:
            return "ProfileTypeInfo";
        case JSType::COW_TAGGED_ARRAY:
            return "COWArray";
        case JSType::MUTANT_TAGGED_ARRAY:
            return "MutantTaggedArray";
        case JSType::COW_MUTANT_TAGGED_ARRAY:
            return "COWMutantTaggedArray";
        case JSType::LINE_STRING:
        case JSType::TREE_STRING:
        case JSType::SLICED_STRING:
            return "BaseString";
        case JSType::JS_NATIVE_POINTER:
            return "NativePointer";
        case JSType::JS_OBJECT:
            return "Object";
        case JSType::JS_XREF_OBJECT:
            return "XRefObject";
        case JSType::JS_SHARED_OBJECT:
            return  "SharedObject";
        case JSType::JS_FUNCTION_BASE:
            return "Function Base";
        case JSType::JS_FUNCTION:
            return "Function";
        case JSType::JS_SHARED_FUNCTION:
            return "Shared Function";
        case JSType::JS_ERROR:
            return "Error";
        case JSType::NATIVE_MODULE_FAILURE_INFO:
            return "NativeModuleFailureInfo";
        case JSType::JS_EVAL_ERROR:
            return "Eval Error";
        case JSType::JS_RANGE_ERROR:
            return "Range Error";
        case JSType::JS_TYPE_ERROR:
            return "Type Error";
        case JSType::JS_AGGREGATE_ERROR:
            return "Aggregate Error";
        case JSType::JS_REFERENCE_ERROR:
            return "Reference Error";
        case JSType::JS_URI_ERROR:
            return "Uri Error";
        case JSType::JS_SYNTAX_ERROR:
            return "Syntax Error";
        case JSType::JS_OOM_ERROR:
            return "OutOfMemory Error";
        case JSType::JS_TERMINATION_ERROR:
            return "Termination Error";
        case JSType::JS_REG_EXP:
            return "Regexp";
        case JSType::JS_SET:
            return "Set";
        case JSType::JS_SHARED_SET:
            return "SharedSet";
        case JSType::JS_MAP:
            return "Map";
        case JSType::JS_SHARED_MAP:
            return "SharedMap";
        case JSType::JS_WEAK_SET:
            return "WeakSet";
        case JSType::JS_WEAK_MAP:
            return "WeakMap";
        case JSType::JS_WEAK_REF:
            return "WeakRef";
        case JSType::JS_FINALIZATION_REGISTRY:
            return "JSFinalizationRegistry";
        case JSType::CELL_RECORD:
            return "CellRecord";
        case JSType::JS_DATE:
            return "Date";
        case JSType::JS_BOUND_FUNCTION:
            return "Bound Function";
        case JSType::JS_ARRAY:
            return "Array";
        case JSType::JS_SHARED_ARRAY:
            return "SharedArray";
        case JSType::JS_SHARED_ARRAY_ITERATOR:
            return "SharedArrayIterator";
        case JSType::JS_TYPED_ARRAY:
            return "Typed Array";
        case JSType::JS_INT8_ARRAY:
            return "Int8 Array";
        case JSType::JS_UINT8_ARRAY:
            return "Uint8 Array";
        case JSType::JS_UINT8_CLAMPED_ARRAY:
            return "Uint8 Clamped Array";
        case JSType::JS_INT16_ARRAY:
            return "Int16 Array";
        case JSType::JS_UINT16_ARRAY:
            return "Uint16 Array";
        case JSType::JS_INT32_ARRAY:
            return "Int32 Array";
        case JSType::JS_UINT32_ARRAY:
            return "Uint32 Array";
        case JSType::BIGINT:
            return "BigInt";
        case JSType::JS_FLOAT32_ARRAY:
            return "Float32 Array";
        case JSType::JS_FLOAT64_ARRAY:
            return "Float64 Array";
        case JSType::JS_BIGINT64_ARRAY:
            return "BigInt64 Array";
        case JSType::JS_BIGUINT64_ARRAY:
            return "BigUint64 Array";
        case JSType::JS_SHARED_TYPED_ARRAY:
            return "Shared Typed Array";
        case JSType::JS_SHARED_INT8_ARRAY:
            return "Shared Int8 Array";
        case JSType::JS_SHARED_UINT8_ARRAY:
            return "Shared Uint8 Array";
        case JSType::JS_SHARED_UINT8_CLAMPED_ARRAY:
            return "Shared Uint8 Clamped Array";
        case JSType::JS_SHARED_INT16_ARRAY:
            return "Shared Int16 Array";
        case JSType::JS_SHARED_UINT16_ARRAY:
            return "Shared Uint16 Array";
        case JSType::JS_SHARED_INT32_ARRAY:
            return "Shared Int32 Array";
        case JSType::JS_SHARED_UINT32_ARRAY:
            return "Shared Uint32 Array";
        case JSType::JS_SHARED_FLOAT32_ARRAY:
            return "Shared Float32 Array";
        case JSType::JS_SHARED_FLOAT64_ARRAY:
            return "Shared Float64 Array";
        case JSType::JS_SHARED_BIGINT64_ARRAY:
            return "Shared BigInt64 Array";
        case JSType::JS_SHARED_BIGUINT64_ARRAY:
            return "Shared BigUint64 Array";
        case JSType::BYTE_ARRAY:
            return "ByteArray";
        case JSType::JS_ARGUMENTS:
            return "Arguments";
        case JSType::JS_PROXY:
            return "Proxy";
        case JSType::JS_PRIMITIVE_REF:
            return "Primitive";
        case JSType::JS_DATA_VIEW:
            return "DataView";
        case JSType::JS_ITERATOR:
            return "Iterator";
        case JSType::JS_ASYNCITERATOR:
            return "AsyncIterator";
        case JSType::JS_FORIN_ITERATOR:
            return "ForinInterator";
        case JSType::JS_MAP_ITERATOR:
            return "MapIterator";
        case JSType::JS_SHARED_MAP_ITERATOR:
            return "SharedMapIterator";
        case JSType::JS_SET_ITERATOR:
            return "SetIterator";
        case JSType::JS_SHARED_SET_ITERATOR:
            return "SharedSetIterator";
        case JSType::JS_ARRAY_ITERATOR:
            return "ArrayIterator";
        case JSType::JS_STRING_ITERATOR:
            return "StringIterator";
        case JSType::JS_REG_EXP_ITERATOR:
            return "RegExpIterator";
        case JSType::JS_ARRAY_BUFFER:
            return "ArrayBuffer";
        case JSType::JS_SENDABLE_ARRAY_BUFFER:
            return "SendableArrayBuffer";
        case JSType::JS_SHARED_ARRAY_BUFFER:
            return "SharedArrayBuffer";
        case JSType::JS_PROXY_REVOC_FUNCTION:
            return "ProxyRevocFunction";
        case JSType::PROMISE_REACTIONS:
            return "PromiseReaction";
        case JSType::PROMISE_CAPABILITY:
            return "PromiseCapability";
        case JSType::PROMISE_ITERATOR_RECORD:
            return "PromiseIteratorRecord";
        case JSType::PROMISE_RECORD:
            return "PromiseRecord";
        case JSType::RESOLVING_FUNCTIONS_RECORD:
            return "ResolvingFunctionsRecord";
        case JSType::ASYNC_GENERATOR_REQUEST:
            return "AsyncGeneratorRequest";
        case JSType::ASYNC_ITERATOR_RECORD:
            return "AsyncIteratorRecord";
        case JSType::JS_ASYNC_FROM_SYNC_ITERATOR:
            return "AsyncFromSyncIterator";
        case JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION:
            return "AsyncFromSyncIterUnwarpFunction";
        case JSType::JS_PROMISE:
            return "Promise";
        case JSType::JS_PROMISE_REACTIONS_FUNCTION:
            return "PromiseReactionsFunction";
        case JSType::JS_PROMISE_EXECUTOR_FUNCTION:
            return "PromiseExecutorFunction";
        case JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION:
            return "AsyncModuleFulfilledFunction";
        case JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION:
            return "AsyncModuleRejectedFunction";
        case JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION:
            return "PromiseAllResolveElementFunction";
        case JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION:
            return "PromiseAnyRejectElementFunction";
        case JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION:
            return "PromiseAllSettledElementFunction";
        case JSType::JS_PROMISE_FINALLY_FUNCTION:
            return "PromiseFinallyFunction";
        case JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION:
            return "PromiseValueThunkOrThrowerFunction";
        case JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN:
            return "AsyncGeneratorResumeNextReturnProcessorRstFtn";
        case JSType::MICRO_JOB_QUEUE:
            return "MicroJobQueue";
        case JSType::PENDING_JOB:
            return "PendingJob";
        case JSType::COMPLETION_RECORD:
            return "CompletionRecord";
        case JSType::GLOBAL_ENV:
            return "GlobalEnv";
        case JSType::ACCESSOR_DATA:
            return "AccessorData";
        case JSType::INTERNAL_ACCESSOR:
            return "InternalAccessor";
        case JSType::SYMBOL:
            return "Symbol";
        case JSType::PROPERTY_BOX:
            return "PropertyBox";
        case JSType::JS_ASYNC_FUNCTION:
            return "AsyncFunction";
        case JSType::JS_SHARED_ASYNC_FUNCTION:
            return "SharedAsyncFunction";
        case JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION:
            return "AsyncAwaitStatusFunction";
        case JSType::JS_ASYNC_FUNC_OBJECT:
            return "AsyncFunctionObject";
        case JSType::JS_REALM:
            return "Realm";
        case JSType::JS_GLOBAL_OBJECT:
            return "GlobalObject";
        case JSType::JS_INTL:
            return "JSIntl";
        case JSType::JS_LOCALE:
            return "JSLocale";
        case JSType::JS_DATE_TIME_FORMAT:
            return "JSDateTimeFormat";
        case JSType::JS_RELATIVE_TIME_FORMAT:
            return "JSRelativeTimeFormat";
        case JSType::JS_NUMBER_FORMAT:
            return "JSNumberFormat";
        case JSType::JS_COLLATOR:
            return "JSCollator";
        case JSType::JS_PLURAL_RULES:
            return "JSPluralRules";
        case JSType::JS_DISPLAYNAMES:
            return "JSDisplayNames";
        case JSType::JS_SEGMENTER:
            return "JSSegmenter";
        case JSType::JS_SEGMENTS:
            return "JSSegments";
        case JSType::JS_SEGMENT_ITERATOR:
            return "JSSegmentIterator";
        case JSType::JS_LIST_FORMAT:
            return "JSListFormat";
        case JSType::JS_GENERATOR_OBJECT:
            return "JSGeneratorObject";
        case JSType::JS_ASYNC_GENERATOR_OBJECT:
            return "JSAsyncGeneratorObject";
        case JSType::JS_GENERATOR_CONTEXT:
            return "JSGeneratorContext";
        case JSType::ENUM_CACHE:
            return "EnumCache";
        case JSType::PROTO_CHANGE_MARKER:
            return "ProtoChangeMarker";
        case JSType::MARKER_CELL:
            return "MarkerCell";
        case JSType::PROTOTYPE_INFO:
            return "PrototypeInfo";
        case JSType::PROGRAM:
            return "program";
        case JSType::MACHINE_CODE_OBJECT:
            return "MachineCode";
        case JSType::CLASS_INFO_EXTRACTOR:
            return "ClassInfoExtractor";
        case JSType::JS_API_ARRAY_LIST:
            return "ArrayList";
        case JSType::JS_API_ARRAYLIST_ITERATOR:
            return "JSArraylistIterator";
        case JSType::LINKED_NODE:
            return "LinkedNode";
        case JSType::RB_TREENODE:
             return "RBTreeNode";
        case JSType::JS_API_HASH_MAP:
            return "HashMap";
        case JSType::JS_API_HASH_SET:
             return "HashSet";
        case JSType::JS_API_HASHMAP_ITERATOR:
             return "HashMapIterator";
        case JSType::JS_API_HASHSET_ITERATOR:
             return "HashSetIterator";
        case JSType::JS_API_LIGHT_WEIGHT_MAP:
            return "LightWeightMap";
        case JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR:
            return "LightWeightMapIterator";
        case JSType::JS_API_LIGHT_WEIGHT_SET:
            return "LightWeightSet";
        case JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR:
            return "LightWeightSetIterator";
        case JSType::JS_API_TREE_MAP:
            return "TreeMap";
        case JSType::JS_API_TREE_SET:
            return "TreeSet";
        case JSType::JS_API_TREEMAP_ITERATOR:
            return "TreeMapIterator";
        case JSType::JS_API_TREESET_ITERATOR:
            return "TreeSetIterator";
        case JSType::JS_API_VECTOR:
            return "Vector";
        case JSType::JS_API_VECTOR_ITERATOR:
            return "VectorIterator";
        case JSType::JS_API_BITVECTOR:
            return "BitVector";
        case JSType::JS_API_BITVECTOR_ITERATOR:
            return "BitVectorIterator";
        case JSType::JS_API_FAST_BUFFER:
            return "Buffer";
        case JSType::JS_API_QUEUE:
            return "Queue";
        case JSType::JS_API_QUEUE_ITERATOR:
            return "QueueIterator";
        case JSType::JS_API_PLAIN_ARRAY:
            return "PlainArray";
        case JSType::JS_API_PLAIN_ARRAY_ITERATOR:
            return "PlainArrayIterator";
        case JSType::JS_API_DEQUE:
            return "Deque";
        case JSType::JS_API_DEQUE_ITERATOR:
            return "DequeIterator";
        case JSType::JS_API_STACK:
            return "Stack";
        case JSType::JS_API_STACK_ITERATOR:
            return "StackIterator";
        case JSType::JS_API_LIST:
            return "List";
        case JSType::JS_API_LIST_ITERATOR:
            return "ListIterator";
        case JSType::JS_API_LINKED_LIST:
            return "LinkedList";
        case JSType::JS_API_LINKED_LIST_ITERATOR:
            return "LinkedListIterator";
        case JSType::JS_CJS_EXPORTS:
            return "CommonJSExports";
        case JSType::JS_CJS_MODULE:
            return "CommonJSModule";
        case JSType::JS_CJS_REQUIRE:
            return "CommonJSRequire";
        case JSType::METHOD:
            return "Method";
        case JSType::AOT_LITERAL_INFO:
            return "AOTLiteralInfo";
        case JSType::CLASS_LITERAL:
            return "ClassLiteral";
        case JSType::PROFILE_TYPE_INFO_CELL_0:
        case JSType::PROFILE_TYPE_INFO_CELL_1:
        case JSType::PROFILE_TYPE_INFO_CELL_N:
            return "ProfileTypeInfoCell";
        case JSType::FUNCTION_TEMPLATE:
            return "FunctionTemplate";
        case JSType::VTABLE:
            return "VTable";
        case JSType::EXTRA_PROFILE_TYPE_INFO:
            return "ExtraProfileTypeInfo";
        case JSType::SOURCE_TEXT_MODULE_RECORD:
            return "SourceTextModuleRecord";
        case JSType::RESOLVEDBINDING_RECORD:
            return "ResolvedBindingRecord";
        case JSType::RESOLVEDINDEXBINDING_RECORD:
            return "ResolvedIndexBindingRecord";
        case JSType::RESOLVEDRECORDINDEXBINDING_RECORD:
            return "ResolvedRecordIndexBindingRecord";
        case JSType::RESOLVEDRECORDBINDING_RECORD:
            return "ResolvedRecordBindingRecord";
        case JSType::IMPORTENTRY_RECORD:
            return "ImportEntry";
        case JSType::LOCAL_EXPORTENTRY_RECORD:
            return "LocalExportEntry";
        case JSType::STAR_EXPORTENTRY_RECORD:
            return "StarExportEntry";
        default: {
            CString ret = "unknown type ";
            return ret.append(std::to_string(static_cast<char>(type)));
        }
    }
}

static void DumpArrayClass(const JSThread *thread, const TaggedArray *arr, std::ostream &os)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetLength();
    os << " <TaggedArray[" << std::dec << len << "]>\n";
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->Get(thread, i));
        if (!val.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET) << i << ": ";
            val.DumpTaggedValue(thread, os);
            os << "\n";
        }
    }
}

static void DumpMutantTaggedArray(const JSThread *thread, const MutantTaggedArray *arr, std::ostream &os)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetLength();
    os << " <MutantTaggedArray[" << std::dec << len << "]>\n";
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->Get(thread, i));
        os << std::right << std::setw(DUMP_PROPERTY_OFFSET) << i << ": ";
        os << std::left << std::setw(DUMP_TYPE_OFFSET) << "[JSTaggedType] : " << val.GetRawData();
        os << "\n";
    }
}

static void DumpCOWMutantTaggedArray(const JSThread *thread, const COWMutantTaggedArray *arr, std::ostream &os)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetLength();
    os << " <COWMutantTaggedArray[" << std::dec << len << "]>\n";
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->Get(thread, i));
        os << std::right << std::setw(DUMP_PROPERTY_OFFSET) << i << ": ";
        os << std::left << std::setw(DUMP_TYPE_OFFSET) << "[JSTaggedType] : " << val.GetRawData();
        os << "\n";
    }
}

static void DumpConstantPoolClass(const JSThread *thread, const ConstantPool *pool, std::ostream &os)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = pool->GetCacheLength();
    os << " <ConstantPool[" << std::dec << len << "]>\n";
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(pool->GetObjectFromCache(thread, i));
        if (!val.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET) << i << ": ";
            val.DumpTaggedValue(thread, os);
            os << "\n";
        }
    }
}

static void DumpStringClass(const JSThread *thread, const EcmaString *str, std::ostream &os)
{
    DISALLOW_GARBAGE_COLLECTION;
    CString string = ConvertToString(thread, str);
    os << string;
}

static void DumpPropertyKey(const JSThread *thread, JSTaggedValue key, std::ostream &os)
{
    if (key.IsString()) {
        DumpStringClass(thread, EcmaString::Cast(key.GetTaggedObject()), os);
    } else if (key.IsSymbol()) {
        JSSymbol *sym = JSSymbol::Cast(key.GetTaggedObject());
        if (sym->GetDescription(thread).IsString()) {
            os << "Symbol(\"";
            DumpStringClass(thread, EcmaString::Cast(sym->GetDescription(thread).GetTaggedObject()), os);
            os << "\")";
        } else {
            os << "Symbol(" << sym << ")";
        }
    } else {
        LOG_ECMA(FATAL) << "this branch is unreachable";
        UNREACHABLE();
    }
}

static void DumpAttr(const PropertyAttributes &attr, bool fastMode, std::ostream &os)
{
    if (attr.IsAccessor()) {
        os << "(Accessor) ";
    }

    os << "Attr(";
    os << "[Raw: " << std::hex << attr.GetValue() << "]  ";
    if (attr.IsNoneAttributes()) {
        os << "NONE";
    }
    if (attr.IsWritable()) {
        os << "W";
    }
    if (attr.IsEnumerable()) {
        os << "E";
    }
    if (attr.IsConfigurable()) {
        os << "C";
    }

    os << ")";

    os << " Rep: " << static_cast<int>(attr.GetRepresentation());

    os << " InlinedProps: " << attr.IsInlinedProps();

    if (fastMode) {
        os << " Order: " << std::dec << attr.GetOffset();
        os << " SortedIndex: " << std::dec << attr.GetSortedIndex();
    } else {
        os << " Order: " << std::dec << attr.GetDictionaryOrder();
    }
}

static void DumpHClass(const JSThread *thread, const JSHClass *jshclass, std::ostream &os, bool withDetail)
{
    DISALLOW_GARBAGE_COLLECTION;
    os << "JSHClass :" << std::setw(DUMP_TYPE_OFFSET);
    os << "Type :" << JSHClass::DumpJSType(jshclass->GetObjectType()) << "\n";

    os << " - Prototype :" << std::setw(DUMP_TYPE_OFFSET);
    jshclass->GetPrototype(thread).DumpTaggedValue(thread, os);
    os << "\n";
    os << " - PropertyDescriptors :" << std::setw(DUMP_TYPE_OFFSET);
    JSTaggedValue attrs = jshclass->GetLayout(thread);
    attrs.DumpTaggedValue(thread, os);
    os << "\n";
    if (withDetail && !attrs.IsNull()) {
        LayoutInfo *layout = LayoutInfo::Cast(jshclass->GetLayout(thread).GetTaggedObject());
        int element = static_cast<int>(jshclass->NumberOfProps());
        for (int i = 0; i < element; i++) {
            JSTaggedValue key = layout->GetKey(thread, i);
            PropertyAttributes attr = layout->GetAttr(thread, i);
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            os << "[" << i << "]: ";
            DumpPropertyKey(thread, key, os);
            os << " : ";
            DumpAttr(attr, true, os);
            os << "\n";
        }
    }

    os << " - Transitions :" << std::setw(DUMP_TYPE_OFFSET);
    JSTaggedValue transtions = jshclass->GetTransitions(thread);
    transtions.DumpTaggedValue(thread, os);
    os << "\n";
    if (withDetail && !transtions.IsWeakForHeapObject() && transtions.IsDictionary()) {
        transtions.Dump(thread, os);
    }

    os << " - ProtoChangeMarker :" << std::setw(DUMP_TYPE_OFFSET);
    JSTaggedValue marker = jshclass->GetProtoChangeMarker(thread);
    marker.DumpTaggedValue(thread, os);
    if (marker.IsHeapObject()) {
        ProtoChangeMarker::Cast(marker.GetTaggedObject())->Dump(thread, os);
    } else {
        os << "\n";
    }

    os << " - ProtoChangeDetails :" << std::setw(DUMP_TYPE_OFFSET);
    JSTaggedValue details = jshclass->GetProtoChangeDetails(thread);
    details.DumpTaggedValue(thread, os);
    if (details.IsHeapObject()) {
        ProtoChangeDetails::Cast(details.GetTaggedObject())->Dump(thread, os);
    } else {
        os << "\n";
    }

    os << " - ProfileType : " << std::hex << jshclass->GetProfileType() << "\n";

    os << " - Flags : " << std::setw(DUMP_TYPE_OFFSET);
    os << "IsCtor :" << std::boolalpha << jshclass->IsConstructor();
    os << "| IsCallable :" << std::boolalpha << jshclass->IsCallable();
    os << "| IsExtensible :" << std::boolalpha << jshclass->IsExtensible();
    os << "| ElementsKind :" << Elements::GetString(jshclass->GetElementsKind());
    os << "| NumberOfProps :" << std::dec << jshclass->NumberOfProps();
    os << "| InlinedProperties :" << std::dec << jshclass->GetInlinedProperties();
    os << "| IsAOT :" << std::boolalpha << jshclass->IsAOT();
    os << "| HasCtor :" << std::boolalpha << jshclass->HasConstructor();
    os << "| IsStable :" << std::boolalpha << jshclass->IsStableElements();
    os << "\n";
}

static void DumpClass(const JSThread *thread, TaggedObject *obj, std::ostream &os)
{
    ASSERT(obj->GetClass()->GetObjectType() == JSType::HCLASS);
    DumpHClass(thread, JSHClass::Cast(obj), os, true);
}

static void DumpObject(const JSThread *thread, TaggedObject *obj, std::ostream &os, bool isPrivacy)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto jsHclass = obj->GetClass();
    JSType type = jsHclass->GetObjectType();

    bool needDumpHClass = false;
    switch (type) {
        case JSType::HCLASS:
            return DumpClass(thread, obj, os);
        case JSType::TAGGED_ARRAY:
        case JSType::TAGGED_DICTIONARY:
        case JSType::TEMPLATE_MAP:
        case JSType::LEXICAL_ENV:
        case JSType::SFUNCTION_ENV:
        case JSType::SENDABLE_ENV:
        case JSType::COW_TAGGED_ARRAY:
        case JSType::AOT_LITERAL_INFO:
            DumpArrayClass(thread, TaggedArray::Cast(obj), os);
            break;
        case JSType::MUTANT_TAGGED_ARRAY:
            DumpMutantTaggedArray(thread, MutantTaggedArray::Cast(obj), os);
            break;
        case JSType::COW_MUTANT_TAGGED_ARRAY:
            DumpCOWMutantTaggedArray(thread, COWMutantTaggedArray::Cast(obj), os);
            break;
        case JSType::CONSTANT_POOL:
            DumpConstantPoolClass(thread, ConstantPool::Cast(obj), os);
            break;
        case JSType::PROFILE_TYPE_INFO:
            ProfileTypeInfo::Cast(obj)->Dump(thread, os);
            break;
        case JSType::PROFILE_TYPE_INFO_CELL_0:
        case JSType::PROFILE_TYPE_INFO_CELL_1:
        case JSType::PROFILE_TYPE_INFO_CELL_N:
            ProfileTypeInfoCell::Cast(obj)->Dump(thread, os);
            break;
        case JSType::FUNCTION_TEMPLATE:
            FunctionTemplate::Cast(obj)->Dump(thread, os);
            break;
        case JSType::VTABLE:
            VTable::Cast(obj)->Dump(thread, os);
            break;
        case JSType::EXTRA_PROFILE_TYPE_INFO:
            ExtraProfileTypeInfo::Cast(obj)->Dump(thread, os);
            break;
        case JSType::LINE_STRING:
        case JSType::TREE_STRING:
        case JSType::SLICED_STRING:
            DumpStringClass(thread, EcmaString::Cast(obj), os);
            os << "\n";
            break;
        case JSType::JS_NATIVE_POINTER:
            break;
        case JSType::JS_OBJECT:
        case JSType::JS_XREF_OBJECT:
        case JSType::JS_SHARED_OBJECT:
        case JSType::JS_GLOBAL_OBJECT:
        case JSType::JS_ERROR:
        case JSType::JS_EVAL_ERROR:
        case JSType::JS_RANGE_ERROR:
        case JSType::JS_TYPE_ERROR:
        case JSType::JS_AGGREGATE_ERROR:
        case JSType::JS_REFERENCE_ERROR:
        case JSType::JS_URI_ERROR:
        case JSType::JS_SYNTAX_ERROR:
        case JSType::JS_OOM_ERROR:
        case JSType::JS_TERMINATION_ERROR:
        case JSType::JS_ARGUMENTS:
            needDumpHClass = true;
            JSObject::Cast(obj)->Dump(thread, os, isPrivacy);
            break;
        case JSType::JS_FUNCTION_BASE:
            needDumpHClass = true;
            JSFunctionBase::Cast(obj)->Dump(thread, os);
            break;
        case JSType::GLOBAL_ENV:
            GlobalEnv::Cast(obj)->Dump(thread, os);
            break;
        case JSType::ACCESSOR_DATA:
            break;
        case JSType::JS_SHARED_FUNCTION:
        case JSType::JS_FUNCTION:
            needDumpHClass = true;
            JSFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_BOUND_FUNCTION:
            needDumpHClass = true;
            JSBoundFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_SET:
            needDumpHClass = true;
            JSSet::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_SHARED_SET:
            needDumpHClass = true;
            JSSharedSet::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_MAP:
            needDumpHClass = true;
            JSMap::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_SHARED_MAP:
            needDumpHClass = true;
            JSSharedMap::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_WEAK_SET:
            needDumpHClass = true;
            JSWeakSet::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_WEAK_MAP:
            needDumpHClass = true;
            JSWeakMap::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_WEAK_REF:
            needDumpHClass = true;
            JSWeakRef::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_FINALIZATION_REGISTRY:
            JSFinalizationRegistry::Cast(obj)->Dump(thread, os);
            break;
        case JSType::CELL_RECORD:
            CellRecord::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_REG_EXP:
            needDumpHClass = true;
            JSRegExp::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_DATE:
            needDumpHClass = true;
            JSDate::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_ARRAY:
            needDumpHClass = true;
            JSArray::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_SHARED_ARRAY:
            needDumpHClass = true;
            JSSharedArray::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_TYPED_ARRAY:
        case JSType::JS_INT8_ARRAY:
        case JSType::JS_UINT8_ARRAY:
        case JSType::JS_UINT8_CLAMPED_ARRAY:
        case JSType::JS_INT16_ARRAY:
        case JSType::JS_UINT16_ARRAY:
        case JSType::JS_INT32_ARRAY:
        case JSType::JS_UINT32_ARRAY:
        case JSType::JS_FLOAT32_ARRAY:
        case JSType::JS_FLOAT64_ARRAY:
        case JSType::JS_BIGINT64_ARRAY:
        case JSType::JS_BIGUINT64_ARRAY:
            needDumpHClass = true;
            JSTypedArray::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_SHARED_TYPED_ARRAY:
        case JSType::JS_SHARED_INT8_ARRAY:
        case JSType::JS_SHARED_UINT8_ARRAY:
        case JSType::JS_SHARED_UINT8_CLAMPED_ARRAY:
        case JSType::JS_SHARED_INT16_ARRAY:
        case JSType::JS_SHARED_UINT16_ARRAY:
        case JSType::JS_SHARED_INT32_ARRAY:
        case JSType::JS_SHARED_UINT32_ARRAY:
        case JSType::JS_SHARED_FLOAT32_ARRAY:
        case JSType::JS_SHARED_FLOAT64_ARRAY:
        case JSType::JS_SHARED_BIGINT64_ARRAY:
        case JSType::JS_SHARED_BIGUINT64_ARRAY:
            needDumpHClass = true;
            JSSharedTypedArray::Cast(obj)->Dump(thread, os);
            break;
        case JSType::BIGINT:
            BigInt::Cast(obj)->Dump(thread, os);
            break;
        case JSType::BYTE_ARRAY:
            ByteArray::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_PROXY:
            needDumpHClass = true;
            JSProxy::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_PRIMITIVE_REF:
            JSPrimitiveRef::Cast(obj)->Dump(thread, os);
            break;
        case JSType::SYMBOL:
            JSSymbol::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_DATA_VIEW:
            JSDataView::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_ARRAY_BUFFER:
            JSArrayBuffer::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_SENDABLE_ARRAY_BUFFER:
            JSSendableArrayBuffer::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_SHARED_ARRAY_BUFFER:
            JSArrayBuffer::Cast(obj)->Dump(thread, os);
            break;
        case JSType::PROMISE_REACTIONS:
            PromiseReaction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::PROMISE_CAPABILITY:
            PromiseCapability::Cast(obj)->Dump(thread, os);
            break;
        case JSType::PROMISE_ITERATOR_RECORD:
            PromiseIteratorRecord::Cast(obj)->Dump(thread, os);
            break;
        case JSType::PROMISE_RECORD:
            PromiseRecord::Cast(obj)->Dump(thread, os);
            break;
        case JSType::RESOLVING_FUNCTIONS_RECORD:
            ResolvingFunctionsRecord::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_PROMISE:
            needDumpHClass = true;
            JSPromise::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_PROMISE_REACTIONS_FUNCTION:
            JSPromiseReactionsFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_PROMISE_EXECUTOR_FUNCTION:
            JSPromiseExecutorFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION:
            JSAsyncModuleFulfilledFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION:
            JSAsyncModuleRejectedFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::ASYNC_GENERATOR_REQUEST:
            AsyncGeneratorRequest::Cast(obj)->Dump(thread, os);
            break;
        case JSType::ASYNC_ITERATOR_RECORD:
            AsyncIteratorRecord::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_ASYNC_FROM_SYNC_ITERATOR:
            JSAsyncFromSyncIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION:
            JSAsyncFromSyncIterUnwarpFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION:
            JSPromiseAllResolveElementFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION:
            JSPromiseAnyRejectElementFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION:
            JSPromiseAllSettledElementFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_PROMISE_FINALLY_FUNCTION:
            JSPromiseFinallyFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION:
            JSPromiseValueThunkOrThrowerFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::MICRO_JOB_QUEUE:
            MicroJobQueue::Cast(obj)->Dump(thread, os);
            break;
        case JSType::PENDING_JOB:
            PendingJob::Cast(obj)->Dump(thread, os);
            break;
        case JSType::COMPLETION_RECORD:
            CompletionRecord::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_PROXY_REVOC_FUNCTION:
            JSProxyRevocFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_ASYNC_FUNCTION:
        case JSType::JS_SHARED_ASYNC_FUNCTION:
            needDumpHClass = true;
            JSAsyncFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION:
            JSAsyncAwaitStatusFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_GENERATOR_FUNCTION:
            JSGeneratorFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_ASYNC_GENERATOR_FUNCTION:
            needDumpHClass = true;
            JSAsyncGeneratorFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN:
            JSAsyncGeneratorResNextRetProRstFtn::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_INTL_BOUND_FUNCTION:
            JSIntlBoundFunction::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_ITERATOR:
            break;
        case JSType::JS_ASYNCITERATOR:
            break;
        case JSType::JS_FORIN_ITERATOR:
            JSForInIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_MAP_ITERATOR:
            JSMapIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_SHARED_MAP_ITERATOR:
            JSSharedMapIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_SET_ITERATOR:
            JSSetIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_SHARED_SET_ITERATOR:
            JSSharedSetIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_REG_EXP_ITERATOR:
            JSRegExpIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_ARRAY_ITERATOR:
            JSArrayIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_SHARED_ARRAY_ITERATOR:
            JSSharedArrayIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_STRING_ITERATOR:
            JSStringIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::PROTOTYPE_HANDLER:
            PrototypeHandler::Cast(obj)->Dump(thread, os);
            break;
        case JSType::TRANSITION_HANDLER:
            TransitionHandler::Cast(obj)->Dump(thread, os);
            break;
        case JSType::TRANS_WITH_PROTO_HANDLER:
            TransWithProtoHandler::Cast(obj)->Dump(thread, os);
            break;
        case JSType::STORE_TS_HANDLER:
            StoreAOTHandler::Cast(obj)->Dump(thread, os);
            break;
        case JSType::PROPERTY_BOX:
            PropertyBox::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_REALM:
            needDumpHClass = true;
            JSRealm::Cast(obj)->Dump(thread, os);
            break;
#ifdef ARK_SUPPORT_INTL
        case JSType::JS_INTL:
            needDumpHClass = true;
            JSIntl::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_LOCALE:
            needDumpHClass = true;
            JSLocale::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_DATE_TIME_FORMAT:
            JSDateTimeFormat::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_RELATIVE_TIME_FORMAT:
            JSRelativeTimeFormat::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_NUMBER_FORMAT:
            JSNumberFormat::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_COLLATOR:
            needDumpHClass = true;
            JSCollator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_PLURAL_RULES:
            JSPluralRules::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_DISPLAYNAMES:
            JSDisplayNames::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_SEGMENTER:
            JSSegmenter::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_SEGMENTS:
            JSSegments::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_SEGMENT_ITERATOR:
            JSSegmentIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_LIST_FORMAT:
            JSListFormat::Cast(obj)->Dump(thread, os);
            break;
#else
        case JSType::JS_INTL:
        case JSType::JS_LOCALE:
        case JSType::JS_DATE_TIME_FORMAT:
        case JSType::JS_RELATIVE_TIME_FORMAT:
        case JSType::JS_NUMBER_FORMAT:
        case JSType::JS_COLLATOR:
        case JSType::JS_PLURAL_RULES:
        case JSType::JS_DISPLAYNAMES:
        case JSType::JS_SEGMENTER:
        case JSType::JS_SEGMENTS:
        case JSType::JS_SEGMENT_ITERATOR:
        case JSType::JS_LIST_FORMAT:
            break;
#endif
        case JSType::JS_GENERATOR_OBJECT:
            needDumpHClass = true;
            JSGeneratorObject::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_ASYNC_GENERATOR_OBJECT:
            needDumpHClass = true;
            JSAsyncGeneratorObject::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_ASYNC_FUNC_OBJECT:
            needDumpHClass = true;
            JSAsyncFuncObject::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_GENERATOR_CONTEXT:
            GeneratorContext::Cast(obj)->Dump(thread, os);
            break;
        case JSType::PROTOTYPE_INFO:
            ProtoChangeDetails::Cast(obj)->Dump(thread, os);
            break;
        case JSType::TRACK_INFO:
            TrackInfo::Cast(obj)->Dump(thread, os);
            break;
        case JSType::ENUM_CACHE:
            EnumCache::Cast(obj)->Dump(thread, os);
            break;
        case JSType::PROTO_CHANGE_MARKER:
            ProtoChangeMarker::Cast(obj)->Dump(thread, os);
            break;
        case JSType::MARKER_CELL:
            MarkerCell::Cast(obj)->Dump(thread, os);
            break;
        case JSType::PROGRAM:
            Program::Cast(obj)->Dump(thread, os);
            break;
        case JSType::MACHINE_CODE_OBJECT:
            MachineCode::Cast(obj)->Dump(thread, os);
            break;
        case JSType::CLASS_INFO_EXTRACTOR:
            ClassInfoExtractor::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_ARRAY_LIST:
            JSAPIArrayList::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_ARRAYLIST_ITERATOR:
            JSAPIArrayListIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_BITVECTOR:
            JSAPIBitVector::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_BITVECTOR_ITERATOR:
            JSAPIBitVectorIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_FAST_BUFFER:
            JSAPIFastBuffer::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_MAP:
            JSAPILightWeightMap::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR:
            JSAPILightWeightMapIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_SET:
            JSAPILightWeightSet::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR:
            JSAPILightWeightSetIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::LINKED_NODE:
            LinkedNode::Cast(obj)->Dump(thread, os);
            break;
        case JSType::RB_TREENODE:
            break;
        case JSType::JS_API_HASH_MAP:
            JSAPIHashMap::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_HASH_SET:
            JSAPIHashSet::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_HASHMAP_ITERATOR:
            JSAPIHashMapIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_HASHSET_ITERATOR:
            JSAPIHashSetIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_TREE_MAP:
            JSAPITreeMap::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_TREE_SET:
            JSAPITreeSet::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_TREEMAP_ITERATOR:
            JSAPITreeMapIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_TREESET_ITERATOR:
            JSAPITreeSetIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_VECTOR:
            JSAPIVector::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_VECTOR_ITERATOR:
            JSAPIVectorIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_QUEUE:
            JSAPIQueue::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_QUEUE_ITERATOR:
            JSAPIQueueIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_DEQUE:
            JSAPIDeque::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_DEQUE_ITERATOR:
            JSAPIDequeIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_STACK:
            JSAPIStack::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_STACK_ITERATOR:
            JSAPIStackIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_LIST:
            JSAPIList::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_LIST_ITERATOR:
            JSAPIListIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_LINKED_LIST:
            JSAPILinkedList::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_LINKED_LIST_ITERATOR:
            JSAPILinkedListIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::SOURCE_TEXT_MODULE_RECORD:
            SourceTextModule::Cast(obj)->Dump(thread, os);
            break;
        case JSType::IMPORTENTRY_RECORD:
            ImportEntry::Cast(obj)->Dump(thread, os);
            break;
        case JSType::LOCAL_EXPORTENTRY_RECORD:
            LocalExportEntry::Cast(obj)->Dump(thread, os);
            break;
        case JSType::INDIRECT_EXPORTENTRY_RECORD:
            IndirectExportEntry::Cast(obj)->Dump(thread, os);
            break;
        case JSType::STAR_EXPORTENTRY_RECORD:
            StarExportEntry::Cast(obj)->Dump(thread, os);
            break;
        case JSType::RESOLVEDBINDING_RECORD:
            ResolvedBinding::Cast(obj)->Dump(thread, os);
            break;
        case JSType::RESOLVEDINDEXBINDING_RECORD:
            ResolvedIndexBinding::Cast(obj)->Dump(thread, os);
            break;
        case JSType::RESOLVEDRECORDINDEXBINDING_RECORD:
            ResolvedRecordIndexBinding::Cast(obj)->Dump(thread, os);
            break;
        case JSType::RESOLVEDRECORDBINDING_RECORD:
            ResolvedRecordBinding::Cast(obj)->Dump(thread, os);
            break;
        case JSType::NATIVE_MODULE_FAILURE_INFO:
            NativeModuleFailureInfo::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_MODULE_NAMESPACE:
            ModuleNamespace::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_PLAIN_ARRAY:
            JSAPIPlainArray::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_API_PLAIN_ARRAY_ITERATOR:
            JSAPIPlainArrayIterator::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_CJS_MODULE:
            CjsModule::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_CJS_REQUIRE:
            CjsRequire::Cast(obj)->Dump(thread, os);
            break;
        case JSType::JS_CJS_EXPORTS:
            CjsExports::Cast(obj)->Dump(thread, os);
            break;
        case JSType::METHOD:
            Method::Cast(obj)->Dump(thread, os);
            break;
        case JSType::CLASS_LITERAL:
            ClassLiteral::Cast(obj)->Dump(thread, os);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }

    if (needDumpHClass) {
        DumpHClass(thread, jsHclass, os, false);
    }
}

void JSTaggedValue::DumpSpecialValue(std::ostream &os) const
{
    ASSERT(IsSpecial());
    os << "[Special Value] : ";
    switch (GetRawData()) {
        case VALUE_HOLE:
            os << "Hole";
            break;
        case VALUE_NULL:
            os << "Null";
            break;
        case VALUE_FALSE:
            os << "False";
            break;
        case VALUE_TRUE:
            os << "True";
            break;
        case VALUE_UNDEFINED:
            os << "Undefined";
            break;
        case VALUE_EXCEPTION:
            os << "Exception";
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
}

void JSTaggedValue::DumpHeapObjectType(const JSThread *thread, std::ostream &os) const
{
    ASSERT(IsWeak() || IsHeapObject());
    bool isWeak = IsWeak();
    TaggedObject *obj = isWeak ? GetTaggedWeakRef() : GetTaggedObject();
    if (isWeak) {
        os << " [Weak Ref] ";
    }

    JSType type = obj->GetClass()->GetObjectType();
    if (type >= JSType::STRING_FIRST && type <= JSType::STRING_LAST) {
        CString string = ConvertToString(thread, EcmaString::Cast(obj));
        os << std::left << std::setw(DUMP_TYPE_OFFSET) << "[" + string + "]";
    } else if (type == JSType::METHOD) {
        std::ostringstream address;
        address << obj;
        CString addrStr = CString(address.str());
        Method *method = Method::Cast(obj);

        os << std::left << std::setw(DUMP_TYPE_OFFSET)
            << "[" + JSHClass::DumpJSType(type) + "(" + addrStr + "-" + method->GetMethodName(thread) + ")]";
    } else {
        std::ostringstream address;
        address << obj;
        CString addrStr = CString(address.str());

        os << std::left << std::setw(DUMP_TYPE_OFFSET) << "[" + JSHClass::DumpJSType(type) + "(" + addrStr + ")]";
    }
}

void JSTaggedValue::DumpTaggedValue(const JSThread *thread, std::ostream &os) const
{
    if (IsInt()) {
        os << std::left << std::setw(DUMP_TYPE_OFFSET) << "[Int] : " << std::hex << "0x" << GetInt() << std::dec << " ("
           << GetInt() << ")";
    } else if (IsDouble()) {
        os << std::left << std::setw(DUMP_TYPE_OFFSET) << "[Double] : " << GetDouble();
    } else if (IsSpecial()) {
        DumpSpecialValue(os);
    } else {
        DumpHeapObjectType(thread, os);
    }
}

void JSTaggedValue::DumpTaggedValueType(std::ostream &os) const
{
    if (IsInt()) {
        os << "[Int]";
    } else if (IsDouble()) {
        os << "[Double]";
    } else if (IsSpecial()) {
        DumpSpecialValue(os);
    } else {
        ASSERT(IsWeak() || IsHeapObject());
        TaggedObject *obj = IsWeak() ? GetTaggedWeakRef() : GetTaggedObject();
        os << "[" + JSHClass::DumpJSType(obj->GetClass()->GetObjectType()) + "]";
    }
}

void JSTaggedValue::Dump(const JSThread *thread, std::ostream &os, bool isPrivacy) const
{
    DumpTaggedValue(thread, os);
    os << "\n";

    if (IsHeapObject()) {
        if (UNLIKELY(thread == nullptr)) {
            thread = JSThread::GetCurrent();
        }
        TaggedObject *obj = IsWeak() ? GetTaggedWeakRef() : GetTaggedObject();
        DumpObject(thread, obj, os, isPrivacy);
    }
}

void JSTaggedValue::DumpHeapObjAddress([[maybe_unused]]const JSThread *thread, std::ostream &os) const
{
    if (IsHeapObject()) {
        TaggedObject *obj = IsWeak() ? GetTaggedWeakRef() : GetTaggedObject();
        os << obj << "\n";
    } else {
        os << "not a heapobject\n";
    }
}

void JSTaggedValue::D(const JSThread *thread) const
{
    Dump(thread, std::cout);
}

void JSTaggedValue::DV(const JSThread *thread, JSTaggedType val)
{
    JSTaggedValue(val).D(thread);
}

void JSThread::DumpStack()
{
    FrameHandler handler(this);
    handler.DumpStack(std::cout);
}

void NumberDictionary::Dump(const JSThread *thread, std::ostream &os, bool isPrivacy) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(thread, hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET)
               << static_cast<uint32_t>(JSTaggedNumber(key).GetNumber()) << ": ";
            if (!isPrivacy) {
                JSTaggedValue val(GetValue(thread, hashIndex));
                val.DumpTaggedValue(thread, os);
            }
            os << " ";
            DumpAttr(GetAttributes(thread, hashIndex), false, os);
            os << "\n";
        }
    }
}

void NameDictionary::Dump(const JSThread *thread, std::ostream &os, bool isPrivacy) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(thread, hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            DumpPropertyKey(thread, key, os);
            os << ": ";
            if (!isPrivacy) {
                JSTaggedValue val(GetValue(thread, hashIndex));
                val.DumpTaggedValue(thread, os);
            }
            os << " ";
            DumpAttr(GetAttributes(thread, hashIndex), false, os);
            os << "\n";
        }
    }
}

void GlobalDictionary::Dump(const JSThread *thread, std::ostream &os, bool isPrivacy) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(thread, hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            DumpPropertyKey(thread, key, os);
            os << " : ";
            if (!isPrivacy) {
                JSTaggedValue val(GetValue(thread, hashIndex));
                val.DumpTaggedValue(thread, os);
            }
            os << " ";
            DumpAttr(GetAttributes(thread, hashIndex), false, os);
            os << "\n";
        }
    }
}

void LayoutInfo::Dump(const JSThread *thread, std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int num = NumberOfElements();
    for (int i = 0; i < num; i++) {
        JSTaggedValue key = GetKey(thread, i);
        PropertyAttributes attr = GetAttr(thread, i);
        os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
        os << "[" << i << "]: ";
        DumpPropertyKey(thread, key, os);
        os << " : ";
        DumpAttr(attr, true, os);
        os << "\n";
    }
}

void TransitionsDictionary::Dump(const JSThread *thread, std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(thread, hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            DumpPropertyKey(thread, key, os);
            os << " : ";
            GetValue(thread, hashIndex).DumpTaggedValue(thread, os);
            os << " : ";
            GetAttributes(thread, hashIndex).DumpTaggedValue(thread, os);
            os << "\n";
        }
    }
}

void LinkedHashSet::Dump(const JSThread *thread, std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfElements() + NumberOfDeletedElements();
    for (int hashIndex = 0; hashIndex < capacity; hashIndex++) {
        JSTaggedValue key(GetKey(thread, hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            key.DumpTaggedValue(thread, os);
            os << "\n";
        }
    }
}

void LinkedHashMap::Dump(const JSThread *thread, std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfElements() + NumberOfDeletedElements();
    for (int hashIndex = 0; hashIndex < capacity; hashIndex++) {
        JSTaggedValue key(GetKey(thread, hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            JSTaggedValue val(GetValue(thread, hashIndex));
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            key.DumpTaggedValue(thread, os);
            os << ": ";
            val.DumpTaggedValue(thread, os);
            os << "\n";
        }
    }
}

void TaggedDoubleList::Dump(const JSThread *thread, std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfNodes();
    os << " - node num: " << std::dec << capacity << "\n";
    os << " - delete node num: " << std::dec << NumberOfDeletedNodes() << "\n";
    os << "head-next: ";
    // 5 : 5 first element next ptr
    GetElement(thread, 5).Dump(thread, os);
    os << "head-pre: ";
    // 6 : 6 first element per ptr
    GetElement(thread, 6).Dump(thread, os);
    os << "\n";
    int i = 0;
    int next = GetElement(thread, 5).GetInt();
    while (capacity > i) {
        os << " value: ";
        GetElement(thread, next).DumpTaggedValue(thread, os);
        os << " next: ";
        // 1 : 1 current element next ptr offset
        GetElement(thread, next + 1).Dump(thread, os);
        os << " pre: ";
        // 2 : 2 current element pre ptr offset
        GetElement(thread, next + 2).Dump(thread, os);
        os << "\n";
        next = GetElement(thread, next + 1).GetInt();
        i++;
    }
}

void TaggedSingleList::Dump(const JSThread *thread, std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfNodes();
    os << "head-next: ";
    // 5 : 5 first element next ptr
    GetElement(thread, 5).Dump(thread, os);
    os << "\n";
    int i = 0;
    int next = GetElement(thread, 5).GetInt();
    while (capacity > i) {
        os << " value: ";
        GetElement(thread, next).DumpTaggedValue(thread, os);
        os << " next: ";
        // 1 : 1 current element next ptr offset
        GetElement(thread, next + 1).Dump(thread, os);
        os << "\n";
        next = GetElement(thread, next + 1).GetInt();
        i++;
    }
}

void JSObject::Dump(const JSThread *thread, std::ostream &os, bool isPrivacy) const
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHClass *jshclass = GetJSHClass();
    os << " - hclass: " << std::hex << jshclass << "\n";
    os << " - prototype: ";
    jshclass->GetPrototype(thread).DumpTaggedValue(thread, os);
    os << "\n";

    JSTaggedType hashField = Barriers::GetTaggedValue(thread, this, HASH_OFFSET);
    JSTaggedValue value(hashField);
    os << " - hash: " << std::hex << hashField;
    value.Dump(thread, os);

    TaggedArray *elements = TaggedArray::Cast(GetElements(thread).GetTaggedObject());
    os << " - elements: " << std::hex << elements;
    if (elements->GetLength() == 0) {
        os << " NONE\n";
    } else if (!elements->IsDictionaryMode()) {
        DumpArrayClass(thread, elements, os);
    } else {
        NumberDictionary *dict = NumberDictionary::Cast(elements);
        os << " <NumberDictionary[" << std::dec << dict->EntriesCount() << "]>\n";
        dict->Dump(thread, os, isPrivacy);
    }

    TaggedArray *properties = TaggedArray::Cast(GetProperties(thread).GetTaggedObject());
    os << " - properties: " << std::hex << properties;
    if (IsJSGlobalObject()) {
        GlobalDictionary *dict = GlobalDictionary::Cast(properties);
        os << " <GlobalDictionary[" << std::dec << dict->EntriesCount() << "]>\n";
        dict->Dump(thread, os, isPrivacy);
        return;
    }

    if (!properties->IsDictionaryMode()) {
        JSTaggedValue attrs = jshclass->GetLayout(thread);
        if (attrs.IsNull()) {
            return;
        }

        LayoutInfo *layoutInfo = LayoutInfo::Cast(attrs.GetTaggedObject());
        int propNumber = static_cast<int>(jshclass->NumberOfProps());
        os << " <LayoutInfo[" << std::dec << propNumber << "]>\n";
        for (int i = 0; i < propNumber; i++) {
            JSTaggedValue key = layoutInfo->GetKey(thread, i);
            PropertyAttributes attr = layoutInfo->GetAttr(thread, i);
            ASSERT(i == static_cast<int>(attr.GetOffset()));
            os << "     " << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            DumpPropertyKey(thread, key, os);
            os << ": (";
            JSTaggedValue val;
            if (attr.IsInlinedProps()) {
                val = GetPropertyInlinedPropsWithRep(thread, i, attr);
            } else {
                val = properties->Get(thread, i - static_cast<int>(jshclass->GetInlinedProperties()));
            }
            if (!isPrivacy) {
                val.DumpTaggedValue(thread, os);
            }
            os << ") ";
            DumpAttr(attr, true, os);
            os << "\n";
        }
    } else {
        NameDictionary *dict = NameDictionary::Cast(properties);
        os << " <NameDictionary[" << std::dec << dict->EntriesCount() << "]>\n";
        dict->Dump(thread, os, isPrivacy);
    }
}

void TaggedArray::Dump(const JSThread *thread, std::ostream &os) const
{
    DumpArrayClass(thread, this, os);
}

void AccessorData::Dump(const JSThread *thread, std::ostream &os) const
{
    auto *hclass = GetClass();
    if (hclass->GetObjectType() == JSType::INTERNAL_ACCESSOR) {
        os << " - Getter: " << InternalAccessor::ConstCast(this)->GetGetter() << "\n";
        os << " - Setter: " << InternalAccessor::ConstCast(this)->GetSetter() << "\n";
        return;
    }

    os << " - Getter: ";
    GetGetter(thread).DumpTaggedValue(thread, os);
    os << "\n";

    os << " - Setter: ";
    GetSetter(thread).DumpTaggedValue(thread, os);
    os << "\n";
}

void Program::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - MainFunction: ";
    GetMainFunction(thread).Dump(thread, os);
    os << "\n";
}

void LinkedNode::Dump([[maybe_unused]] const JSThread *thread, std::ostream &os) const
{
    os << " - Next: ";

    os << "\n";
}

void ConstantPool::Dump(const JSThread *thread, std::ostream &os) const
{
    DumpArrayClass(thread, this, os);
}

void ProfileTypeInfo::Dump(const JSThread *thread, std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = GetIcSlotLength();
    os << " <ProfileTypeInfo[" << std::dec << len << "]>\n";
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(Get(thread, i));
        if (!val.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET) << i << ": ";
            val.DumpTaggedValue(thread, os);
            os << "\n";
        }
    }
}

void ExtraProfileTypeInfo::Dump(const JSThread *thread, std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    os << " - Receiver: ";
    GetReceiverObject(thread).Dump(thread, os);
    os << " - Holder: ";
    GetHolderObject(thread).Dump(thread, os);
    os << "\n";
}

void ProfileTypeInfoCell::Dump(const JSThread *thread, std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    os << " - Value: ";
    GetValue(thread).Dump(thread, os);
    os << " - Handle: ";
    GetHandle(thread).Dump(thread, os);
    os << "\n";
}

void FunctionTemplate::Dump(const JSThread *thread, std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    os << " - Method: ";
    GetMethod(thread).Dump(thread, os);
    os << " - Module: ";
    GetModule(thread).Dump(thread, os);
    os << " - RawProfileTypeInfo: ";
    GetRawProfileTypeInfo(thread).Dump(thread, os);
    os << " - length : " << GetLength() << "\n";
}

void VTable::Dump(const JSThread *thread, std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t num = GetNumberOfTuples();

    for (uint32_t i = 0; i < num; i++) {
        os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
        os << "[" << i << "]: [ name :";
        JSTaggedValue name = GetTupleItem(thread, i, VTable::TupleItem::NAME);
        DumpPropertyKey(thread, name, os);
        os << ", type :" << (IsAccessor(thread, i) ? "Accessor" : "Function");
        JSTaggedValue owner = GetTupleItem(thread, i, VTable::TupleItem::OWNER);
        os << ", owner :";
        owner.DumpTaggedValue(thread, os);
        JSTaggedValue offset = GetTupleItem(thread, i, VTable::TupleItem::OFFSET);
        os << ", offset :" << offset.GetInt();
        os << " ] ";
        os << "\n";
    }
}

void JSFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - ProtoOrHClass: ";
    GetProtoOrHClass(thread).Dump(thread, os);
    os << "\n";
    os << " - LexicalEnv: ";
    if (GetLexicalEnv(thread).IsLexicalEnv()) {
        GetLexicalEnv(thread).Dump(thread, os);
    } else {
        GetLexicalEnv(thread).DumpTaggedValue(thread, os); // reduce circular calls
    }
    os << "\n";
    os << " - RawProfileTypeInfo: ";
    GetRawProfileTypeInfo(thread).Dump(thread, os);
    os << "\n";
    os << " - HomeObject: ";
    GetHomeObject(thread).Dump(thread, os);
    os << "\n";
    os << " - FunctionExtraInfo: ";
    GetFunctionExtraInfo(thread).Dump(thread, os);
    os << "\n";
    os << " - Method: ";
    GetMethod(thread).Dump(thread, os);
    os << "\n";
    os << " - Module: ";
    GetModule(thread).Dump(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSHClass::Dump(const JSThread *thread, std::ostream &os) const
{
    DumpHClass(thread, this, os, true);
}

void JSBoundFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - BoundTarget: ";
    GetBoundTarget(thread).DumpTaggedValue(thread, os);
    os << "\n";

    os << " - BoundThis: ";
    GetBoundThis(thread).DumpTaggedValue(thread, os);
    os << "\n";

    os << " - BoundArguments: ";
    GetBoundArguments(thread).DumpTaggedValue(thread, os);
    os << "\n";

    JSObject::Dump(thread, os);
}

void JSPrimitiveRef::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - SubValue : ";
    GetValue(thread).DumpTaggedValue(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}

void BigInt::Dump([[maybe_unused]] const JSThread *thread, std::ostream &os) const
{
    os << " - length : " << GetLength() << "\n";
    os << " - Sign : " << GetSign() << "\n";
    os << " - value : " << ToStdString(DECIMAL) << "\n";
}

void JSDate::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - time: " << GetTime(const_cast<JSThread*>(thread)).GetDouble() << "\n";
    os << " - localOffset: " << GetLocalOffset(thread).GetDouble() << "\n";
    JSObject::Dump(thread, os);
}

void JSMap::Dump(const JSThread *thread, std::ostream &os) const
{
    LinkedHashMap *map = LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject());
    os << " - elements: " << std::dec << map->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << map->Capacity() << "\n";
    JSObject::Dump(thread, os);

    os << " <NameDictionary[" << map->NumberOfElements() << "]>\n";
    map->Dump(thread, os);
}

void JSSharedMap::Dump(const JSThread *thread, std::ostream &os) const
{
    LinkedHashMap *map = LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject());
    os << " - modRecord: " << std::dec << GetModRecord() << "\n";
    os << " - elements: " << std::dec << map->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << map->Capacity() << "\n";
    JSObject::Dump(thread, os);

    os << " <NameDictionary[" << map->NumberOfElements() << "]>\n";
    map->Dump(thread, os);
}

void JSAPITreeMap::Dump(const JSThread *thread, std::ostream &os) const
{
    TaggedTreeMap *map = TaggedTreeMap::Cast(GetTreeMap(thread).GetTaggedObject());
    os << " - elements: " << std::dec << map->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << map->Capacity() << "\n";
    JSObject::Dump(thread, os);

    os << " <TaggedTree[" << map->NumberOfElements() << "]>\n";
    map->Dump(thread, os);
}

void JSAPITreeMap::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetTreeMap(thread).IsInvalidValue())) {
        TaggedTreeMap *map = TaggedTreeMap::Cast(GetTreeMap(thread).GetTaggedObject());
        vec.emplace_back("treemap", GetTreeMap(thread));
        map->DumpForSnapshot(thread, vec);
    }

    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPITreeMapIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    TaggedTreeMap *map =
        TaggedTreeMap::Cast(JSAPITreeMap::Cast(GetIteratedMap(thread).GetTaggedObject())->
            GetTreeMap(thread).GetTaggedObject());
    os << " - elements: " << std::dec << map->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << map->Capacity() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(thread, os);

    os << " <TaggedTree[" << map->NumberOfElements() << "]>\n";
    map->Dump(thread, os);
}

void JSAPITreeMapIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedMap(thread).IsInvalidValue())) {
        TaggedTreeMap *map =
            TaggedTreeMap::Cast(JSAPITreeMap::Cast(GetIteratedMap(thread).GetTaggedObject())->
                GetTreeMap(thread).GetTaggedObject());
        vec.emplace_back("iteratedmap", GetIteratedMap(thread));
        map->DumpForSnapshot(thread, vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(thread, vec);
}

template <typename T>
void DumpTaggedTreeEntry(const JSThread *thread, T tree, std::ostream &os, int index, bool isMap = false)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue parent(tree->GetParent(index));
    JSTaggedValue val(tree->GetValue(thread, index));
    JSTaggedValue color(static_cast<int>(tree->GetColor(index)));
    JSTaggedValue left = tree->GetLeftChild(index);
    JSTaggedValue right = tree->GetRightChild(index);
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[entry] " << index << ": ";
    os << "\n";
    if (isMap) {
        os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "   [key]:    {";
        JSTaggedValue key(tree->GetKey(thread, index));
        key.DumpTaggedValue(thread, os);
        os << std::right << "};";
        os << "\n";
    }
    os << std::left << std::setw(DUMP_TYPE_OFFSET) << "   [value]:  {";
    val.DumpTaggedValue(thread, os);
    os << std::right << "};";
    os << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "   [parent]: {";
    parent.DumpTaggedValue(thread, os);
    os << std::right << "};";
    os << "\n";
    os << std::left << std::setw(DUMP_TYPE_OFFSET) << "   [color]:  {";
    color.DumpTaggedValue(thread, os);
    os << std::right << "};";
    os << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "   [left]:   {";
    left.DumpTaggedValue(thread, os);
    os << std::right << "}; ";
    os << std::left << std::setw(DUMP_TYPE_OFFSET) << "  [right]: {";
    right.DumpTaggedValue(thread, os);
    os << std::right << "};";
    os << "\n";
}
void TaggedTreeMap::Dump(const JSThread *thread, std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[Elements]: {";
    JSTaggedValue node = TaggedArray::Get(thread, 0);
    node.DumpTaggedValue(thread, os);
    os << std::right << "}" << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[Delete]:   {";
    node = TaggedArray::Get(thread, 1);
    node.DumpTaggedValue(thread, os);
    os << std::right << "}" << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[Capacity]: {";
    node = TaggedArray::Get(thread, 2); // 2 means the three element
    node.DumpTaggedValue(thread, os);
    os << std::right << "}" << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[RootNode]: {";
    node = TaggedArray::Get(thread, 3); // 3 means the three element
    node.DumpTaggedValue(thread, os);
    os << std::right << "}" << "\n";

    uint32_t capacity = NumberOfElements() + NumberOfDeletedElements();
    for (uint32_t index = 0; index < capacity; index++) {
        if (GetKey(thread, index).IsHole()) {
            os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[entry] " << index << ": ";
            GetKey(thread, index).DumpTaggedValue(thread, os);
            os << "\n";
        } else {
            DumpTaggedTreeEntry(thread, const_cast<TaggedTreeMap *>(this), os, index, true);
        }
    }
}

void JSAPITreeSet::Dump(const JSThread *thread, std::ostream &os) const
{
    TaggedTreeSet *set = TaggedTreeSet::Cast(GetTreeSet(thread).GetTaggedObject());
    os << " - elements: " << std::dec << set->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << set->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << set->Capacity() << "\n";
    JSObject::Dump(thread, os);

    os << " <TaggedTree[" << set->NumberOfElements() << "]>\n";
    set->Dump(thread, os);
}

void JSAPITreeSet::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetTreeSet(thread).IsInvalidValue())) {
        TaggedTreeSet *set = TaggedTreeSet::Cast(GetTreeSet(thread).GetTaggedObject());
        vec.emplace_back("treeset", GetTreeSet(thread));
        set->DumpForSnapshot(thread, vec);
    }

    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPITreeSetIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    TaggedTreeSet *set =
        TaggedTreeSet::Cast(JSAPITreeSet::Cast(GetIteratedSet(thread).GetTaggedObject())->
            GetTreeSet(thread).GetTaggedObject());
    os << " - elements: " << std::dec << set->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << set->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << set->Capacity() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(thread, os);

    os << " <TaggedTree[" << set->NumberOfElements() << "]>\n";
    set->Dump(thread, os);
}

void JSAPITreeSetIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedSet(thread).IsInvalidValue())) {
        TaggedTreeSet *set =
            TaggedTreeSet::Cast(JSAPITreeSet::Cast(GetIteratedSet(thread).GetTaggedObject())->
                GetTreeSet(thread).GetTaggedObject());
        vec.emplace_back("iteratedset", GetIteratedSet(thread));
        set->DumpForSnapshot(thread, vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(thread, vec);
}

void TaggedTreeSet::Dump(const JSThread *thread, std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[Elements]: {";
    JSTaggedValue node = TaggedArray::Get(thread, 0);
    node.DumpTaggedValue(thread, os);
    os << std::right << "}" << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[Delete]:   {";
    node = TaggedArray::Get(thread, 1);
    node.DumpTaggedValue(thread, os);
    os << std::right << "}" << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[Capacity]: {";
    node = TaggedArray::Get(thread, 2); // 2 means the three element
    node.DumpTaggedValue(thread, os);
    os << std::right << "}" << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[RootNode]: {";
    node = TaggedArray::Get(thread, 3); // 3 means the three element
    node.DumpTaggedValue(thread, os);
    os << std::right << "}" << "\n";

    uint32_t capacity = NumberOfElements() + NumberOfDeletedElements();
    for (uint32_t index = 0; index < capacity; index++) {
        if (GetKey(thread, index).IsHole()) {
            os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[entry] " << index << ": ";
            GetKey(thread, index).DumpTaggedValue(thread, os);
            os << "\n";
        } else {
            DumpTaggedTreeEntry(thread, const_cast<TaggedTreeSet *>(this), os, index);
        }
    }
}

void JSAPIPlainArray::Dump(const JSThread *thread, std::ostream &os) const
{
    TaggedArray *keys = TaggedArray::Cast(GetKeys(thread).GetTaggedObject());
    TaggedArray *values = TaggedArray::Cast(GetValues(thread).GetTaggedObject());
    uint32_t len = static_cast<uint32_t>(GetLength());
    for (uint32_t i = 0; i < len; i++) {
        os << " - keys: ";
        keys->Get(thread, i).DumpTaggedValue(thread, os);
        os << "\n";
        os << " - values: ";
        values->Get(thread, i).DumpTaggedValue(thread, os);
        os << "\n";
    }
    os << " - length: " << std::dec << len << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIPlainArrayIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(GetIteratedPlainArray(thread).GetTaggedObject());
    os << " - length: " << std::dec << array->GetSize() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIPlainArrayIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedPlainArray(thread).IsInvalidValue())) {
        JSAPIPlainArray *array = JSAPIPlainArray::Cast(GetIteratedPlainArray(thread).GetTaggedObject());
        vec.emplace_back("iteratedplainarray", GetIteratedPlainArray(thread));
        array->DumpForSnapshot(thread, vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSForInIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Object : ";
    GetObject(thread).DumpTaggedValue(thread, os);
    os << " - CachedHclass : ";
    GetCachedHClass(thread).DumpTaggedValue(thread, os);
    os << "\n";
    os << " - Keys : ";
    GetKeys(thread).DumpTaggedValue(thread, os);
    os << "\n";
    os << " - Index : " << GetIndex();
    os << "\n";
    os << " - Length : " << GetLength();
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSMapIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    LinkedHashMap *map = LinkedHashMap::Cast(GetIteratedMap(thread).GetTaggedObject());
    os << " - elements: " << std::dec << map->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << map->Capacity() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(thread, os);

    os << " <NameDictionary[" << map->NumberOfElements() << "]>\n";
    map->Dump(thread, os);
}

void JSSharedMapIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    JSSharedMap *iteratedMap = JSSharedMap::Cast(GetIteratedMap(thread).GetTaggedObject());
    LinkedHashMap *map = LinkedHashMap::Cast(iteratedMap->GetLinkedMap(thread).GetTaggedObject());
    os << " - elements: " << std::dec << map->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << map->Capacity() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(thread, os);

    os << " <NameDictionary[" << map->NumberOfElements() << "]>\n";
    map->Dump(thread, os);
}

void JSSet::Dump(const JSThread *thread, std::ostream &os) const
{
    LinkedHashSet *set = LinkedHashSet::Cast(GetLinkedSet(thread).GetTaggedObject());
    os << " - elements: " << std::dec << set->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << set->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << set->Capacity() << "\n";
    JSObject::Dump(thread, os);

    os << " <NameDictionary[" << set->NumberOfElements() << "]>\n";
    set->Dump(thread, os);
}

void JSSharedSet::Dump(const JSThread *thread, std::ostream &os) const
{
    LinkedHashSet *set = LinkedHashSet::Cast(GetLinkedSet(thread).GetTaggedObject());
    os << " - modRecord: " << std::dec << GetModRecord() << "\n";
    os << " - elements: " << std::dec << set->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << set->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << set->Capacity() << "\n";
    JSObject::Dump(thread, os);

    os << " <NameDictionary[" << set->NumberOfElements() << "]>\n";
    set->Dump(thread, os);
}

void JSWeakMap::Dump(const JSThread *thread, std::ostream &os) const
{
    LinkedHashMap *map = LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject());
    os << " - length: " << std::dec << GetSize(thread) << "\n";
    os << " - elements: " << std::dec << map->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << map->Capacity() << "\n";
    JSObject::Dump(thread, os);

    os << " <NameDictionary[" << map->NumberOfElements() << "]>\n";
    map->Dump(thread, os);
}

void JSWeakSet::Dump(const JSThread *thread, std::ostream &os) const
{
    LinkedHashSet *set = LinkedHashSet::Cast(GetLinkedSet(thread).GetTaggedObject());
    os << " - size: " << std::dec << GetSize(thread) << "\n";
    os << " - elements: " << std::dec << set->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << set->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << set->Capacity() << "\n";
    JSObject::Dump(thread, os);

    os << " <NameDictionary[" << set->NumberOfElements() << "]>\n";
    set->Dump(thread, os);
}

void JSWeakRef::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - WeakObject : ";
    GetWeakObject(thread).DumpTaggedValue(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSFinalizationRegistry::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - CleanupCallback : ";
    GetCleanupCallback(thread).DumpTaggedValue(thread, os);
    os << "\n";
    os << " - NoUnregister : ";
    GetNoUnregister(thread).Dump(thread, os);
    os << "\n";
    os << " - MaybeUnregister : ";
    LinkedHashMap *map = LinkedHashMap::Cast(GetMaybeUnregister(thread).GetTaggedObject());
    os << "   -   elements: " << std::dec << map->NumberOfElements() << "\n";
    os << "   -   deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << "   -   capacity: " << std::dec << map->Capacity() << "\n";
    JSObject::Dump(thread, os);
}

void CellRecord::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - WeakRefTarget : ";
    GetFromWeakRefTarget(thread).DumpTaggedValue(thread, os);
    os << "\n";
    os << " - HeldValue : ";
    GetHeldValue(thread).DumpTaggedValue(thread, os);
    os << "\n";
}

void JSSetIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    LinkedHashSet *set = LinkedHashSet::Cast(GetIteratedSet(thread).GetTaggedObject());
    os << " - elements: " << std::dec << set->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << set->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << set->Capacity() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(thread, os);

    os << " <NameDictionary[" << set->NumberOfElements() << "]>\n";
    set->Dump(thread, os);
}

void JSSharedSetIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    JSSharedSet *iteratedSet = JSSharedSet::Cast(GetIteratedSet(thread).GetTaggedObject());
    LinkedHashSet *set = LinkedHashSet::Cast(iteratedSet->GetLinkedSet(thread).GetTaggedObject());
    os << " - elements: " << std::dec << set->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << set->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << set->Capacity() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(thread, os);

    os << " <NameDictionary[" << set->NumberOfElements() << "]>\n";
    set->Dump(thread, os);
}

void JSRegExpIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - IteratingRegExp: ";
    GetIteratingRegExp(thread).Dump(thread, os);
    os << "\n";
    os << " - IteratedString: ";
    GetIteratedString(thread).Dump(thread, os);
    os << "\n";
    os << " - Global: " << std::dec << GetGlobal() << "\n";
    os << " - Unicode: " << std::dec << GetUnicode() << "\n";
    os << " - Done: " << std::dec << GetDone() << "\n";
    JSObject::Dump(thread, os);
}

void JSArray::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - length: " << std::dec << GetArrayLength() << "\n";
    JSObject::Dump(thread, os);
}

void JSSharedArray::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - length: " << std::dec << GetArrayLength() << "\n";
    JSObject::Dump(thread, os);
}

void JSArrayIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    JSArray *array = JSArray::Cast(GetIteratedArray(thread).GetTaggedObject());
    os << " - length: " << std::dec << array->GetArrayLength() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(thread, os);
}

void JSSharedArrayIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    JSSharedArray *array = JSSharedArray::Cast(GetIteratedArray(thread).GetTaggedObject());
    os << " - length: " << std::dec << array->GetArrayLength() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIArrayList::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - length: " << std::dec << GetSize(const_cast<JSThread *>(thread)) << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIArrayListIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    JSAPIArrayList *arrayList = JSAPIArrayList::Cast(GetIteratedArrayList(thread).GetTaggedObject());
    os << " - length: " << std::dec << arrayList->GetSize(const_cast<JSThread *>(thread)) << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIDeque::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - first: " << std::dec << GetFirst() << "\n";
    os << " - last: " << std::dec << GetLast() << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIDequeIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    JSAPIDeque *deque = JSAPIDeque::Cast(GetIteratedDeque(thread).GetTaggedObject());
    os << " - length: " << std::dec << deque->GetSize(thread) << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(thread, os);
}

void JSAPILightWeightMap::Dump(const JSThread *thread, std::ostream &os) const
{
    uint32_t capacity = GetSize();
    os << " - length: " << std::dec << capacity << "\n";
    uint32_t i = 0;
    TaggedArray *hashArray = TaggedArray::Cast(GetHashes(thread).GetTaggedObject());
    TaggedArray *keyArray = TaggedArray::Cast(GetKeys(thread).GetTaggedObject());
    TaggedArray *valueArray = TaggedArray::Cast(GetValues(thread).GetTaggedObject());
    while (capacity > i) {
        os << " hash: ";
        hashArray->Get(thread, i).DumpTaggedValue(thread, os);
        os << " key: ";
        keyArray->Get(thread, i).DumpTaggedValue(thread, os);
        os << " value: ";
        valueArray->Get(thread, i).DumpTaggedValue(thread, os);
        os << "\n";
        i++;
    }
    JSObject::Dump(thread, os);
}

void JSAPILightWeightMapIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIHashMap::Dump(const JSThread *thread, std::ostream &os) const
{
    TaggedHashArray *hashArray = TaggedHashArray::Cast(GetTable(thread).GetTaggedObject());
    os << " - elements: " << std::dec << GetSize() << "\n";
    os << " - table capacity: " << std::dec << static_cast<int>(hashArray->GetLength()) << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIHashMap::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetTable(thread).IsInvalidValue())) {
        TaggedHashArray *map = TaggedHashArray::Cast(GetTable(thread).GetTaggedObject());
        vec.emplace_back("hashmap", GetTable(thread));
        map->DumpForSnapshot(thread, vec);
    }

    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIHashSet::Dump(const JSThread *thread, std::ostream &os) const
{
    TaggedHashArray *hashArray = TaggedHashArray::Cast(GetTable(thread).GetTaggedObject());
    os << " - elements: " << std::dec << GetSize() << "\n";
    os << " - table capacity: " << std::dec << static_cast<int>(hashArray->GetLength()) << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIHashSet::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetTable(thread).IsInvalidValue())) {
        TaggedHashArray *set = TaggedHashArray::Cast(GetTable(thread).GetTaggedObject());
        vec.emplace_back("hashset", GetTable(thread));
        set->DumpForSnapshot(thread, vec);
    }

    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIHashMapIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIHashMapIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("IteratedHashMap"), JSTaggedValue(GetIteratedHashMap(thread)));
    vec.emplace_back(CString("TaggedQueue"), JSTaggedValue(GetTaggedQueue(thread)));
    vec.emplace_back(CString("CurrentNodeResult"), JSTaggedValue(GetCurrentNodeResult(thread)));
    vec.emplace_back(CString("BitField"), JSTaggedValue(GetBitField()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIHashSetIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIHashSetIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPILightWeightSet::Dump(const JSThread *thread, std::ostream &os) const
{
    TaggedArray *keys = TaggedArray::Cast(GetHashes(thread).GetTaggedObject());
    TaggedArray *values = TaggedArray::Cast(GetValues(thread).GetTaggedObject());
    uint32_t len = GetLength();
    for (uint32_t i = 0; i < len; i++) {
        os << " - keys: ";
        keys->Get(thread, i).DumpTaggedValue(thread, os);
        os << "\n";
        os << " - values: ";
        values->Get(thread, i).DumpTaggedValue(thread, os);
        os << "\n";
    }
    os << " - length: " << std::dec << len << "\n";
    JSObject::Dump(thread, os);
}

void JSAPILightWeightSetIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIList::Dump(const JSThread *thread, std::ostream &os) const
{
    TaggedSingleList *list = TaggedSingleList::Cast(GetSingleList(thread).GetTaggedObject());
    os << " - length: " << std::dec << list->GetCapacityFromTaggedArray() << "\n";
    os << " - node num: " << std::dec << list->NumberOfNodes() << "\n";
    os << " - delete node num: " << std::dec << list->NumberOfDeletedNodes() << "\n";
    os << " - is odered list: " << std::dec << this->IsOrderedList() << "\n";
    JSObject::Dump(thread, os);
    list->Dump(thread, os);
}

void JSAPIList::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetSingleList(thread).IsInvalidValue())) {
        TaggedSingleList *list = TaggedSingleList::Cast(GetSingleList(thread).GetTaggedObject());
        vec.emplace_back("singleList", GetSingleList(thread));
        list->DumpForSnapshot(thread, vec);
    }
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIListIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    TaggedSingleList *list = TaggedSingleList::Cast(GetIteratedList(thread).GetTaggedObject());
    os << " - length: " << std::dec << list->GetCapacityFromTaggedArray() << "\n";
    os << " - node num: " << std::dec << list->NumberOfNodes() << "\n";
    os << " - delete node num: " << std::dec << list->NumberOfDeletedNodes() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(thread, os);
    list->Dump(thread, os);
}

void JSAPIListIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedList(thread).IsInvalidValue())) {
        TaggedSingleList *list = TaggedSingleList::Cast(GetIteratedList(thread).GetTaggedObject());
        vec.emplace_back("iteratedlist", GetIteratedList(thread));
        list->DumpForSnapshot(thread, vec);
    }
    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPILinkedList::Dump(const JSThread *thread, std::ostream &os) const
{
    TaggedDoubleList *linkedList = TaggedDoubleList::Cast(GetDoubleList(thread).GetTaggedObject());
    os << " - length: " << std::dec << linkedList->GetCapacityFromTaggedArray() << "\n";
    os << " - node num: " << std::dec << linkedList->NumberOfNodes() << "\n";
    os << " - delete node num: " << std::dec << linkedList->NumberOfDeletedNodes() << "\n";
    JSObject::Dump(thread, os);
    linkedList->Dump(thread, os);
}

void JSAPILinkedList::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetDoubleList(thread).IsInvalidValue())) {
        TaggedDoubleList *list = TaggedDoubleList::Cast(GetDoubleList(thread).GetTaggedObject());
        vec.emplace_back("doubleList", GetDoubleList(thread));
        list->DumpForSnapshot(thread, vec);
    }

    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPILinkedListIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    TaggedDoubleList *linkedList = TaggedDoubleList::Cast(GetIteratedLinkedList(thread).GetTaggedObject());
    os << " - length: " << std::dec << linkedList->GetCapacityFromTaggedArray() << "\n";
    os << " - node num: " << std::dec << linkedList->NumberOfNodes() << "\n";
    os << " - delete node num: " << std::dec << linkedList->NumberOfDeletedNodes() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(thread, os);
    linkedList->Dump(thread, os);
}

void JSAPILinkedListIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedLinkedList(thread).IsInvalidValue())) {
        TaggedDoubleList *linkedList = TaggedDoubleList::Cast(GetIteratedLinkedList(thread).GetTaggedObject());
        vec.emplace_back("iteratedlist", GetIteratedLinkedList(thread));
        linkedList->DumpForSnapshot(thread, vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIQueue::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - length: " << std::dec << GetSize(thread) << "\n";
    os << " - front: " << std::dec << GetFront() << "\n";
    os << " - tail: " << std::dec << GetTail() << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIQueueIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    JSAPIQueue *queue = JSAPIQueue::Cast(GetIteratedQueue(thread).GetTaggedObject());
    os << " - length: " << std::dec << queue->GetSize(thread) << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIStack::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - top: " << std::dec << GetTop() << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIStackIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    JSAPIStack *stack = JSAPIStack::Cast(GetIteratedStack(thread).GetTaggedObject());
    os << " - length: " << std::dec << stack->GetSize() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIVector::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - length: " << std::dec << GetSize() << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIVectorIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    JSAPIVector *vector = JSAPIVector::Cast(GetIteratedVector(thread).GetTaggedObject());
    os << " - length: " << std::dec << vector->GetSize() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIBitVector::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - length: " << std::dec << GetSize() << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIBitVectorIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    JSAPIBitVector *bitVector = JSAPIBitVector::Cast(GetIteratedBitVector(thread).GetTaggedObject());
    os << " - length: " << std::dec << bitVector->GetSize() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(thread, os);
}

void JSAPIFastBuffer::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - ByteLength: " << std::dec << GetSize() << "\n";
    os << " - Length: " << std::dec << GetLength() << "\n";
    auto array = JSTypedArray::Cast(GetFastBufferData(thread).GetTaggedObject());
    array->Dump(thread, os);
    JSObject::Dump(thread, os);
}

void JSStringIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    EcmaString *str = EcmaString::Cast(GetIteratedString(thread).GetTaggedObject());
    os << " - IteratedString: " << EcmaStringAccessor(str).ToCString(thread) << "\n";
    os << " - StringIteratorNextIndex: " << std::dec << GetStringIteratorNextIndex() << "\n";
    JSObject::Dump(thread, os);
}
void JSTypedArray::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - viewed-array-buffer: ";
    GetViewedArrayBufferOrByteArray(thread).Dump(thread, os);
    os << " - typed-array-name: ";
    GetTypedArrayName(thread).Dump(thread, os);
    os << " - byte-length: " << GetByteLength();
    os << " - byte-offset: " << GetByteOffset();
    os << " - array-length: " << GetArrayLength();
    JSObject::Dump(thread, os);
}

void JSSharedTypedArray::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - viewed-array-buffer: ";
    GetViewedArrayBufferOrByteArray(thread).Dump(thread, os);
    os << " - typed-array-name: ";
    GetTypedArrayName(thread).Dump(thread, os);
    os << " - byte-length: " << GetByteLength();
    os << " - byte-offset: " << GetByteOffset();
    os << " - array-length: " << GetArrayLength();
    JSObject::Dump(thread, os);
}

void ByteArray::Dump([[maybe_unused]] const JSThread *thread, std::ostream &os) const
{
    os << " - array-length: " << GetArrayLength();
    os << " - byte-length: " << GetByteLength();
}

void JSRegExp::Dump(const JSThread *thread, std::ostream &os) const
{
    os << "\n";
    os << " - ByteCodeBuffer: ";
    GetByteCodeBuffer(thread).Dump(thread, os);
    os << "\n";
    os << " - OriginalSource: ";
    GetOriginalSource(thread).Dump(thread, os);
    os << "\n";
    os << " - OriginalFlags: ";
    GetOriginalFlags(thread).Dump(thread, os);
    os << "\n";
    os << " - GroupName: ";
    GetGroupName(thread).Dump(thread, os);
    os << "\n";
    os << " - Length: " << GetLength();
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSProxy::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Target: ";
    os << "\n";
    JSObject::Cast(GetTarget(thread).GetTaggedObject())->Dump(thread, os);
    os << " - Handler: ";
    os << "\n";
    JSObject::Cast(GetHandler(thread).GetTaggedObject())->Dump(thread, os);
    os << "\n";
}

void JSSymbol::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - hash-field: " << GetHashField();
    os << "\n - flags: " << GetFlags();
    os << "\n - description: ";
    JSTaggedValue description = GetDescription(thread);
    description.Dump(thread, os);
}

void LexicalEnv::Dump(const JSThread *thread, std::ostream &os) const
{
    DumpArrayClass(thread, this, os);
}

void SFunctionEnv::Dump(const JSThread *thread, std::ostream &os) const
{
    DumpArrayClass(thread, this, os);
}

void SendableEnv::Dump(const JSThread *thread, std::ostream &os) const
{
    DumpArrayClass(thread, this, os);
}

void COWTaggedArray::Dump(const JSThread *thread, std::ostream &os) const
{
    DumpArrayClass(thread, this, os);
}

void MutantTaggedArray::Dump(const JSThread *thread, std::ostream &os) const
{
    DumpMutantTaggedArray(thread, this, os);
}

void COWMutantTaggedArray::Dump(const JSThread *thread, std::ostream &os) const
{
    DumpCOWMutantTaggedArray(thread, this, os);
}

// NOLINTNEXTLINE(readability-function-size)
void GlobalEnv::Dump(const JSThread *thread, std::ostream &os) const
{
    auto globalConst = GetJSThread()->GlobalConstants();
    os << " - ObjectFunction: ";
    GetObjectFunction().GetTaggedValue().Dump(thread, os);
    os << " - FunctionFunction: ";
    GetFunctionFunction().GetTaggedValue().Dump(thread, os);
    os << " - NumberFunction: ";
    GetNumberFunction().GetTaggedValue().Dump(thread, os);
    os << " - BigIntFunction: ";
    GetBigIntFunction().GetTaggedValue().Dump(thread, os);
    os << " - DateFunction: ";
    GetDateFunction().GetTaggedValue().Dump(thread, os);
    os << " - BooleanFunction: ";
    GetBooleanFunction().GetTaggedValue().Dump(thread, os);
    os << " - ErrorFunction: ";
    GetErrorFunction().GetTaggedValue().Dump(thread, os);
    os << " - ArrayFunction: ";
    GetArrayFunction().GetTaggedValue().Dump(thread, os);
    os << " - TypedArrayFunction: ";
    GetTypedArrayFunction().GetTaggedValue().Dump(thread, os);
    os << " - Int8ArrayFunction: ";
    GetInt8ArrayFunction().GetTaggedValue().Dump(thread, os);
    os << " - Uint8ArrayFunction: ";
    GetUint8ArrayFunction().GetTaggedValue().Dump(thread, os);
    os << " - Uint8ClampedArrayFunction: ";
    GetUint8ClampedArrayFunction().GetTaggedValue().Dump(thread, os);
    os << " - Int16ArrayFunction: ";
    GetInt16ArrayFunction().GetTaggedValue().Dump(thread, os);
    os << " - ArrayBufferFunction: ";
    GetArrayBufferFunction().GetTaggedValue().Dump(thread, os);
    os << " - SharedArrayBufferFunction: ";
    GetSharedArrayBufferFunction().GetTaggedValue().Dump(thread, os);
    os << " - SymbolFunction: ";
    GetSymbolFunction().GetTaggedValue().Dump(thread, os);
    os << " - RangeErrorFunction: ";
    GetRangeErrorFunction().GetTaggedValue().Dump(thread, os);
    os << " - ReferenceErrorFunction: ";
    GetReferenceErrorFunction().GetTaggedValue().Dump(thread, os);
    os << " - TypeErrorFunction: ";
    GetTypeErrorFunction().GetTaggedValue().Dump(thread, os);
    os << " - AggregateErrorFunction: ";
    GetAggregateErrorFunction().GetTaggedValue().Dump(thread, os);
    os << " - URIErrorFunction: ";
    GetURIErrorFunction().GetTaggedValue().Dump(thread, os);
    os << " - SyntaxErrorFunction: ";
    GetSyntaxErrorFunction().GetTaggedValue().Dump(thread, os);
    os << " - EvalErrorFunction: ";
    GetEvalErrorFunction().GetTaggedValue().Dump(thread, os);
    os << " - OOMErrorFunction: ";
    GetOOMErrorFunction().GetTaggedValue().Dump(thread, os);
    os << " - TerminationErrorFunction: ";
    GetTerminationErrorFunction().GetTaggedValue().Dump(thread, os);
    os << " - RegExpFunction: ";
    GetRegExpFunction().GetTaggedValue().Dump(thread, os);
    os << " - BuiltinsSetFunction: ";
    GetBuiltinsSetFunction().GetTaggedValue().Dump(thread, os);
    os << " - BuiltinsMapFunction: ";
    GetBuiltinsMapFunction().GetTaggedValue().Dump(thread, os);
    os << " - BuiltinsWeakSetFunction: ";
    GetBuiltinsWeakSetFunction().GetTaggedValue().Dump(thread, os);
    os << " - BuiltinsWeakMapFunction: ";
    GetBuiltinsWeakMapFunction().GetTaggedValue().Dump(thread, os);
    os << " - BuiltinsWeakRefFunction: ";
    GetBuiltinsWeakRefFunction().GetTaggedValue().Dump(thread, os);
    os << " - BuiltinsFinalizationRegistryFunction: ";
    GetBuiltinsFinalizationRegistryFunction().GetTaggedValue().Dump(thread, os);
    os << " - MathFunction: ";
    GetMathFunction().GetTaggedValue().Dump(thread, os);
    os << " - AtomicsFunction: ";
    GetAtomicsFunction().GetTaggedValue().Dump(thread, os);
    os << " - JsonFunction: ";
    GetJsonFunction().GetTaggedValue().Dump(thread, os);
    os << " - StringFunction: ";
    GetStringFunction().GetTaggedValue().Dump(thread, os);
    os << " - ProxyFunction: ";
    GetProxyFunction().GetTaggedValue().Dump(thread, os);
    os << " - ReflectFunction: ";
    GetReflectFunction().GetTaggedValue().Dump(thread, os);
    os << " - AsyncFunction: ";
    GetAsyncFunction().GetTaggedValue().Dump(thread, os);
    os << " - AsyncFunctionPrototype: ";
    GetAsyncFunctionPrototype().GetTaggedValue().Dump(thread, os);
    os << " - JSGlobalObject: ";
    GetJSGlobalObject().GetTaggedValue().Dump(thread, os);
    os << " - GlobalPatch: ";
    GetGlobalPatch().GetTaggedValue().Dump(thread, os);
    os << " - EmptyArray: ";
    globalConst->GetEmptyArray().Dump(thread, os);
    os << " - EmptyString ";
    globalConst->GetEmptyString().Dump(thread, os);
    os << " - EmptyTaggedQueue: ";
    globalConst->GetEmptyTaggedQueue().Dump(thread, os);
    os << " - EmptyProfileTypeInfoCell: ";
    globalConst->GetEmptyProfileTypeInfoCell().Dump(thread, os);
    os << " - PrototypeString: ";
    globalConst->GetPrototypeString().Dump(thread, os);
    os << " - HasInstanceSymbol: ";
    GetHasInstanceSymbol().GetTaggedValue().Dump(thread, os);
    os << " - IsConcatSpreadableSymbol: ";
    GetIsConcatSpreadableSymbol().GetTaggedValue().Dump(thread, os);
    os << " - ToStringTagSymbol: ";
    GetToStringTagSymbol().GetTaggedValue().Dump(thread, os);
    os << " - IteratorSymbol: ";
    GetIteratorSymbol().GetTaggedValue().Dump(thread, os);
    os << " - AsyncIteratorSymbol: ";
    GetAsyncIteratorSymbol().GetTaggedValue().Dump(thread, os);
    os << " - MatchSymbol: ";
    GetMatchSymbol().GetTaggedValue().Dump(thread, os);
    os << " - MatchAllSymbol: ";
    GetMatchAllSymbol().GetTaggedValue().Dump(thread, os);
    os << " - ReplaceSymbol: ";
    GetReplaceSymbol().GetTaggedValue().Dump(thread, os);
    os << " - SearchSymbol: ";
    GetSearchSymbol().GetTaggedValue().Dump(thread, os);
    os << " - SpeciesSymbol: ";
    GetSpeciesSymbol().GetTaggedValue().Dump(thread, os);
    os << " - SplitSymbol: ";
    GetSplitSymbol().GetTaggedValue().Dump(thread, os);
    os << " - ToPrimitiveSymbol: ";
    GetToPrimitiveSymbol().GetTaggedValue().Dump(thread, os);
    os << " - UnscopablesSymbol: ";
    GetUnscopablesSymbol().GetTaggedValue().Dump(thread, os);
    os << " - HoleySymbol: ";
    GetHoleySymbol().GetTaggedValue().Dump(thread, os);
    os << " - NativeBindingSymbol: ";
    GetNativeBindingSymbol().GetTaggedValue().Dump(thread, os);
    os << " - ConstructorString: ";
    globalConst->GetConstructorString().Dump(thread, os);
    os << " - IteratorPrototype: ";
    GetIteratorPrototype().GetTaggedValue().Dump(thread, os);
    os << " - ForinIteratorPrototype: ";
    GetForinIteratorPrototype().GetTaggedValue().Dump(thread, os);
    os << " - StringIterator: ";
    GetStringIterator().GetTaggedValue().Dump(thread, os);
    os << " - MapIteratorPrototype: ";
    GetMapIteratorPrototype().GetTaggedValue().Dump(thread, os);
    os << " - SetIteratorPrototype: ";
    GetSetIteratorPrototype().GetTaggedValue().Dump(thread, os);
    os << " - RegExpIteratorPrototype: ";
    GetRegExpIteratorPrototype().GetTaggedValue().Dump(thread, os);
    os << " - ArrayIteratorPrototype: ";
    GetArrayIteratorPrototype().GetTaggedValue().Dump(thread, os);
    os << " - StringIteratorPrototype: ";
    GetStringIteratorPrototype().GetTaggedValue().Dump(thread, os);
    os << " - LengthString: ";
    globalConst->GetLengthString().Dump(thread, os);
    os << " - ValueString: ";
    globalConst->GetValueString().Dump(thread, os);
    os << " - WritableString: ";
    globalConst->GetWritableString().Dump(thread, os);
    os << " - GetString: ";
    globalConst->GetGetString().Dump(thread, os);
    os << " - SetString: ";
    globalConst->GetSetString().Dump(thread, os);
    os << " - EnumerableString: ";
    globalConst->GetEnumerableString().Dump(thread, os);
    os << " - ConfigurableString: ";
    globalConst->GetConfigurableString().Dump(thread, os);
    os << " - NameString: ";
    globalConst->GetNameString().Dump(thread, os);
    os << " - ValueOfString: ";
    globalConst->GetValueOfString().Dump(thread, os);
    os << " - ToStringString: ";
    globalConst->GetToStringString().Dump(thread, os);
    os << " - ToLocaleStringString: ";
    globalConst->GetToLocaleStringString().Dump(thread, os);
    os << " - UndefinedString: ";
    globalConst->GetUndefinedString().Dump(thread, os);
    os << " - NullString: ";
    globalConst->GetNullString().Dump(thread, os);
    os << " - TrueString: ";
    globalConst->GetTrueString().Dump(thread, os);
    os << " - FalseString: ";
    globalConst->GetFalseString().Dump(thread, os);
    os << " - ThrowTypeError: ";
    GetThrowTypeError().GetTaggedValue().Dump(thread, os);
    os << " - GetPrototypeOfString: ";
    globalConst->GetGetPrototypeOfString().Dump(thread, os);
    os << " - SetPrototypeOfString: ";
    globalConst->GetSetPrototypeOfString().Dump(thread, os);
    os << " - IsExtensibleString: ";
    globalConst->GetIsExtensibleString().Dump(thread, os);
    os << " - PreventExtensionsString: ";
    globalConst->GetPreventExtensionsString().Dump(thread, os);
    os << " - GetOwnPropertyDescriptorString: ";
    globalConst->GetGetOwnPropertyDescriptorString().Dump(thread, os);
    os << " - DefinePropertyString: ";
    globalConst->GetDefinePropertyString().Dump(thread, os);
    os << " - HasString: ";
    globalConst->GetHasString().Dump(thread, os);
    os << " - DeletePropertyString: ";
    globalConst->GetDeletePropertyString().Dump(thread, os);
    os << " - EnumerateString: ";
    globalConst->GetEnumerateString().Dump(thread, os);
    os << " - OwnKeysString: ";
    globalConst->GetOwnKeysString().Dump(thread, os);
    os << " - ApplyString: ";
    globalConst->GetApplyString().Dump(thread, os);
    os << " - ProxyString: ";
    globalConst->GetProxyString().Dump(thread, os);
    os << " - RevokeString: ";
    globalConst->GetRevokeString().Dump(thread, os);
    os << " - ProxyConstructString: ";
    globalConst->GetProxyConstructString().Dump(thread, os);
    os << " - ProxyCallString: ";
    globalConst->GetProxyCallString().Dump(thread, os);
    os << " - DoneString: ";
    globalConst->GetDoneString().Dump(thread, os);
    os << " - NegativeZeroString: ";
    globalConst->GetNegativeZeroString().Dump(thread, os);
    os << " - NextString: ";
    globalConst->GetNextString().Dump(thread, os);
    os << " - PromiseThenString: ";
    globalConst->GetPromiseThenString().Dump(thread, os);
    os << " - PromiseFunction: ";
    GetPromiseFunction().GetTaggedValue().Dump(thread, os);
    os << " - PromiseReactionJob: ";
    GetPromiseReactionJob().GetTaggedValue().Dump(thread, os);
    os << " - PromiseResolveThenableJob: ";
    GetPromiseResolveThenableJob().GetTaggedValue().Dump(thread, os);
    os << " - DynamicImportJob: ";
    GetDynamicImportJob().GetTaggedValue().Dump(thread, os);
    os << " - ScriptJobString: ";
    globalConst->GetScriptJobString().Dump(thread, os);
    os << " - PromiseString: ";
    globalConst->GetPromiseString().Dump(thread, os);
    os << " - IdentityString: ";
    globalConst->GetIdentityString().Dump(thread, os);
    os << " - AsyncFunctionString: ";
    globalConst->GetAsyncFunctionString().Dump(thread, os);
    os << " - ThrowerString: ";
    globalConst->GetThrowerString().Dump(thread, os);
    os << " - Undefined: ";
    globalConst->GetUndefined().Dump(thread, os);
}

void JSDataView::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - data-view: ";
    GetDataView(thread).Dump(thread, os);
    os << " - buffer: ";
    GetViewedArrayBuffer(thread).Dump(thread, os);
    os << "- byte-length: " << GetByteLength();
    os << "\n - byte-offset: " << GetByteOffset();
}

void JSArrayBuffer::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - byte-length: " << GetArrayBufferByteLength();
    os << " - buffer-data: ";
    GetArrayBufferData(thread).Dump(thread, os);
    os << " - Shared: " << GetShared();
}

void JSSendableArrayBuffer::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - byte-length: " << GetArrayBufferByteLength();
    os << " - buffer-data: ";
    GetArrayBufferData(thread).Dump(thread, os);
    os << " - Shared: " << GetShared();
}

void PromiseReaction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - promise-capability: ";
    GetPromiseCapability(thread).Dump(thread, os);
    os << " - type: " << static_cast<int>(GetType());
    os << " - handler: ";
    GetHandler(thread).Dump(thread, os);
}

void PromiseCapability::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - promise: ";
    GetPromise(thread).Dump(thread, os);
    os << " - resolve: ";
    GetResolve(thread).Dump(thread, os);
    os << " - reject: ";
    GetReject(thread).Dump(thread, os);
}

void PromiseIteratorRecord::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - iterator: ";
    GetIterator(thread).Dump(thread, os);
    os << " - done: " << GetDone();
}

void PromiseRecord::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - value: ";
    GetValue(thread).Dump(thread, os);
}

void ResolvingFunctionsRecord::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - resolve-function: ";
    GetResolveFunction(thread).Dump(thread, os);
    os << " - reject-function: ";
    GetRejectFunction(thread).Dump(thread, os);
}

void AsyncGeneratorRequest::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - completion: ";
    GetCompletion(thread).Dump(thread, os);
    os << " - capability: ";
    GetCapability(thread).Dump(thread, os);
}

void AsyncIteratorRecord::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - completion: ";
    GetIterator(thread).Dump(thread, os);
    os << " - nextmethod: ";
    GetNextMethod(thread).Dump(thread, os);
    os << " - done: " << GetDone();
}

void JSAsyncFromSyncIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - sync-iterator-record: ";
    GetSyncIteratorRecord(thread).Dump(thread, os);
    JSObject::Dump(thread, os);
}

void JSAsyncFromSyncIterUnwarpFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - done: " ;
    GetDone(thread).Dump(thread, os);
    JSObject::Dump(thread, os);
}

void JSPromise::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - promise-state: " << static_cast<int>(GetPromiseState());
    os << "\n - promise-result: ";
    GetPromiseResult(thread).Dump(thread, os);
    os << " - promise-fulfill-reactions: ";
    GetPromiseFulfillReactions(thread).Dump(thread, os);
    os << " - promise-reject-reactions: ";
    GetPromiseRejectReactions(thread).Dump(thread, os);
    os << " - promise-is-handled: " << GetPromiseIsHandled();
    JSObject::Dump(thread, os);
}

void JSPromiseReactionsFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - promise: ";
    GetPromise(thread).Dump(thread, os);
    os << " - already-resolved: ";
    GetAlreadyResolved(thread).Dump(thread, os);
    JSObject::Dump(thread, os);
}

void JSPromiseExecutorFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - capability: ";
    GetCapability(thread).Dump(thread, os);
    JSObject::Dump(thread, os);
}

void JSAsyncModuleFulfilledFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - module: ";
    GetModule(thread).Dump(thread, os);
    JSObject::Dump(thread, os);
}

void JSAsyncModuleRejectedFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - module: ";
    GetModule(thread).Dump(thread, os);
    JSObject::Dump(thread, os);
}

void JSPromiseAllResolveElementFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - index: ";
    GetIndex(thread).Dump(thread, os);
    os << " - values: ";
    GetValues(thread).Dump(thread, os);
    os << " - capability: ";
    GetCapabilities(thread).Dump(thread, os);
    os << " - remaining-elements: ";
    GetRemainingElements(thread).Dump(thread, os);
    os << " - already-called: ";
    GetAlreadyCalled(thread).Dump(thread, os);
    JSObject::Dump(thread, os);
}

void JSPromiseAnyRejectElementFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - index: ";
    JSTaggedValue(GetIndex()).Dump(thread, os);
    os << " - errors: ";
    GetErrors(thread).Dump(thread, os);
    os << " - capability: ";
    GetCapability(thread).Dump(thread, os);
    os << " - remaining-elements: ";
    GetRemainingElements(thread).Dump(thread, os);
    os << " - already-called: ";
    GetAlreadyCalled(thread).Dump(thread, os);
    JSObject::Dump(thread, os);
}

void JSPromiseAllSettledElementFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - already-called: ";
    GetAlreadyCalled(thread).Dump(thread, os);
    os << " - index: ";
    JSTaggedValue(GetIndex()).Dump(thread, os);
    os << " - values: ";
    GetValues(thread).Dump(thread, os);
    os << " - capability: ";
    GetCapability(thread).Dump(thread, os);
    os << " - remaining-elements: ";
    GetRemainingElements(thread).Dump(thread, os);
    JSObject::Dump(thread, os);
}

void JSPromiseFinallyFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - constructor: ";
    GetConstructor(thread).Dump(thread, os);
    os << " - onFinally: ";
    GetOnFinally(thread).Dump(thread, os);
    JSObject::Dump(thread, os);
}

void JSPromiseValueThunkOrThrowerFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - result: ";
    GetResult(thread).Dump(thread, os);
    JSObject::Dump(thread, os);
}

void JSAsyncGeneratorResNextRetProRstFtn::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - AsyncGeneratorObject";
    GetAsyncGeneratorObject(thread).Dump(thread, os);
    JSObject::Dump(thread, os);
}

void MicroJobQueue::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - promise-job-queue: ";
    GetPromiseJobQueue(thread).Dump(thread, os);
    os << " - script-job-queue: ";
    GetScriptJobQueue(thread).Dump(thread, os);
}

void PendingJob::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - job: ";
    GetJob(thread).Dump(thread, os);
    os << "\n";
    os << " - arguments: ";
    GetArguments(thread).Dump(thread, os);
#if defined(ENABLE_HITRACE)
    os << "\n";
    os << " - chainId: " << GetChainId();
    os << "\n";
    os << " - spanId: " << GetSpanId();
    os << "\n";
    os << " - parentSpanId: " << GetParentSpanId();
    os << "\n";
    os << " - flags: " << GetFlags();
    os << "\n";
#endif
}

void CompletionRecord::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - type: " << static_cast<int>(GetType());
    os << " - value: ";
    GetValue(thread).Dump(thread, os);
}

void JSProxyRevocFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - RevocableProxy: ";
    os << "\n";
    GetRevocableProxy(thread).Dump(thread, os);
    os << "\n";
}

void JSAsyncFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    JSFunction::Dump(thread, os);
}

void JSAsyncAwaitStatusFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - AsyncContext: ";
    os << "\n";
    GetAsyncContext(thread).Dump(thread, os);
    os << "\n";
}

void JSGeneratorFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    JSFunction::Dump(thread, os);
}

void JSAsyncGeneratorFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    JSFunction::Dump(thread, os);
}

void JSIntlBoundFunction::Dump(const JSThread *thread, std::ostream &os) const
{
    JSObject::Dump(thread, os);
}

void PropertyBox::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Value: ";
    GetValue(thread).Dump(thread, os);
    os << "\n";
}

void PrototypeHandler::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - HandlerInfo: ";
    GetHandlerInfo(thread).Dump(thread, os);
    os << "\n";
    os << " - ProtoCell: ";
    GetProtoCell(thread).Dump(thread, os);
    os << "\n";
    os << " - Holder: ";
    GetHolder(thread).Dump(thread, os);
    os << "\n";
}

void TransitionHandler::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - HandlerInfo: ";
    GetHandlerInfo(thread).Dump(thread, os);
    os << "\n";
    os << " - TransitionHClass: ";
    GetTransitionHClass(thread).Dump(thread, os);
    os << "\n";
}

void TransWithProtoHandler::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - HandlerInfo: ";
    GetHandlerInfo(thread).Dump(thread, os);
    os << "\n";
    os << " - TransitionHClass: ";
    GetTransitionHClass(thread).Dump(thread, os);
    os << "\n";
    os << " - ProtoCell: ";
    GetProtoCell(thread).Dump(thread, os);
    os << "\n";
}

void StoreAOTHandler::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - HandlerInfo: ";
    GetHandlerInfo(thread).Dump(thread, os);
    os << "\n";
    os << " - ProtoCell: ";
    GetProtoCell(thread).Dump(thread, os);
    os << "\n";
    os << " - Holder: ";
    GetHolder(thread).Dump(thread, os);
    os << "\n";
}

void JSRealm::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Value: ";
    GetValue(thread).Dump(thread, os);
    os << "\n";
    os << " - GlobalEnv: ";
    GetGlobalEnv(thread).Dump(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}
#ifdef ARK_SUPPORT_INTL
void JSIntl::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - FallbackSymbol: ";
    GetFallbackSymbol(thread).Dump(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSLocale::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - IcuField: ";
    GetIcuField(thread).Dump(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSDateTimeFormat::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Locale: ";
    GetLocale(thread).Dump(thread, os);
    os << "\n";
    os << " - Calendar: ";
    GetCalendar(thread).Dump(thread, os);
    os << "\n";
    os << " - NumberingSystem: ";
    GetNumberingSystem(thread).Dump(thread, os);
    os << "\n";
    os << " - TimeZone: ";
    GetTimeZone(thread).Dump(thread, os);
    os << "\n";
    os << " - HourCycle: " << static_cast<int>(GetHourCycle());
    os << "\n";
    os << " - LocaleIcu: ";
    GetLocaleIcu(thread).Dump(thread, os);
    os << "\n";
    os << " - SimpleDateTimeFormatIcu: ";
    GetSimpleDateTimeFormatIcu(thread).Dump(thread, os);
    os << "\n";
    os << " - Iso8601: ";
    GetIso8601(thread).Dump(thread, os);
    os << "\n";
    os << " - DateStyle: " << static_cast<int>(GetDateStyle());
    os << "\n";
    os << " - TimeStyle: " << static_cast<int>(GetTimeStyle());
    os << "\n";
    os << " - BoundFormat: ";
    GetBoundFormat(thread).Dump(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSRelativeTimeFormat::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Locale: ";
    GetLocale(thread).Dump(thread, os);
    os << "\n";
    os << " - NumberingSystem: ";
    GetNumberingSystem(thread).Dump(thread, os);
    os << "\n";
    os << " - Style: " << static_cast<int>(GetStyle());
    os << "\n";
    os << " - Numeric: " << static_cast<int>(GetNumeric());
    os << "\n";
    os << " - IcuField: ";
    GetIcuField(thread).Dump(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSNumberFormat::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Locale: ";
    GetLocale(thread).Dump(thread, os);
    os << "\n" << " - NumberingSystem: ";
    GetNumberingSystem(thread).Dump(thread, os);
    os << "\n" << " - Style: " << static_cast<int>(GetStyle());
    os << "\n" << " - Currency: ";
    GetCurrency(thread).Dump(thread, os);
    os << "\n" << " - CurrencyDisplay: " << static_cast<int>(GetCurrencyDisplay());
    os << "\n" << " - CurrencySign: " << static_cast<int>(GetCurrencySign());
    os << "\n" << " - Unit: ";
    GetUnit(thread).Dump(thread, os);
    os << "\n" << " - UnitDisplay: " << static_cast<int>(GetUnitDisplay());
    os << "\n" << " - MinimumIntegerDigits: ";
    GetMinimumIntegerDigits(thread).Dump(thread, os);
    os << "\n" << " - MinimumFractionDigits: ";
    GetMinimumFractionDigits(thread).Dump(thread, os);
    os << "\n" << " - MaximumFractionDigits: ";
    GetMaximumFractionDigits(thread).Dump(thread, os);
    os << "\n" << " - MinimumSignificantDigits: ";
    GetMinimumSignificantDigits(thread).Dump(thread, os);
    os << "\n" << " - MaximumSignificantDigits: ";
    GetMaximumSignificantDigits(thread).Dump(thread, os);
    os << "\n" << " - UseGrouping: ";
    GetUseGrouping(thread).Dump(thread, os);
    os << "\n" << " - RoundingType: " << static_cast<int>(GetRoundingType());
    os << "\n" << " - Notation: " << static_cast<int>(GetNotation());
    os << "\n" << " - CompactDisplay: " << static_cast<int>(GetCompactDisplay());
    os << "\n" << " - SignDisplay: " << static_cast<int>(GetSignDisplay());
    os << "\n" << " - BoundFormat: ";
    GetBoundFormat(thread).Dump(thread, os);
    os << "\n" << " - IcuField: ";
    GetIcuField(thread).Dump(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSCollator::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - IcuField: ";
    GetIcuField(thread).Dump(thread, os);
    os << "\n - Locale: ";
    GetLocale(thread).Dump(thread, os);
    os << "\n - Usage: " << static_cast<int>(GetUsage());
    os << "\n - Sensitivity: " << static_cast<int>(GetSensitivity());
    os << "\n - IgnorePunctuation: " << GetIgnorePunctuation();
    os << "\n - Collation: ";
    GetCollation(thread).Dump(thread, os);
    os << "\n - Numeric: " << GetNumeric();
    os << "\n - CaseFirst: " << static_cast<int>(GetCaseFirst());
    os << "\n - BoundCompare: ";
    GetBoundCompare(thread).Dump(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSPluralRules::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Locale: ";
    GetLocale(thread).Dump(thread, os);
    os << "\n";
    os << " - Type: " << static_cast<int>(GetType());
    os << "\n";
    os << " - MinimumIntegerDigits: ";
    GetMinimumIntegerDigits(thread).Dump(thread, os);
    os << "\n";
    os << " - MinimumFractionDigits: ";
    GetMinimumFractionDigits(thread).Dump(thread, os);
    os << "\n";
    os << " - MaximumFractionDigits: ";
    GetMaximumFractionDigits(thread).Dump(thread, os);
    os << "\n";
    os << " - MinimumSignificantDigits: ";
    GetMinimumSignificantDigits(thread).Dump(thread, os);
    os << "\n";
    os << " - MaximumSignificantDigits: ";
    GetMaximumSignificantDigits(thread).Dump(thread, os);
    os << "\n";
    os << " - RoundingType: " << static_cast<int>(GetRoundingType());
    os << "\n";
    os << " - IcuPR: ";
    GetIcuPR(thread).Dump(thread, os);
    os << "\n";
    os << " - IcuNF: ";
    GetIcuNF(thread).Dump(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSDisplayNames::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Locale: ";
    GetLocale(thread).Dump(thread, os);
    os << "\n";
    os << " - Type: "<< static_cast<int>(GetType());
    os << "\n";
    os << " - Style: "<< static_cast<int>(GetStyle());
    os << "\n";
    os << " - Fallback: "<< static_cast<int>(GetFallback());
    os << "\n";
    os << " - IcuLDN: ";
    GetIcuLDN(thread).Dump(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSSegmenter::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Locale: ";
    GetLocale(thread).Dump(thread, os);
    os << "\n";
    os << " - Granularity: "<< static_cast<int>(GetGranularity());
    os << "\n";
    os << " - IcuField: ";
    GetIcuField(thread).Dump(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSSegments::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - SegmentsString: ";
    GetSegmentsString(thread).Dump(thread, os);
    os << "\n";
    os << " - UnicodeString: ";
    GetUnicodeString(thread).Dump(thread, os);
    os << "\n";
    os << " - Granularity: "<< static_cast<int>(GetGranularity());
    os << "\n";
    os << " - IcuField: ";
    GetIcuField(thread).Dump(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSSegmentIterator::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - IteratedString: ";
    GetIteratedString(thread).Dump(thread, os);
    os << "\n";
    os << " - UnicodeString: ";
    GetUnicodeString(thread).Dump(thread, os);
    os << "\n";
    os << " - Granularity: "<< static_cast<int>(GetGranularity());
    os << "\n";
    os << " - IcuField: ";
    GetIcuField(thread).Dump(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSListFormat::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Locale: ";
    GetLocale(thread).Dump(thread, os);
    os << "\n";
    os << " - Type: "<< static_cast<int>(GetType());
    os << "\n";
    os << " - Style: "<< static_cast<int>(GetStyle());
    os << "\n";
    os << " - IcuLF: ";
    GetIcuLF(thread).Dump(thread, os);
    os << "\n";
    JSObject::Dump(thread, os);
}
#endif
void JSGeneratorObject::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - GeneratorContext: ";
    GetGeneratorContext(thread).Dump(thread, os);
    os << "\n";
    os << " - ResumeResult: ";
    GetResumeResult(thread).Dump(thread, os);
    os << "\n";
    os << " - GeneratorState: " << static_cast<uint8_t>(GetGeneratorState());
    os << "\n";
    os << " - ResumeMode: " << static_cast<uint8_t>(GetResumeMode());
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSAsyncGeneratorObject::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - GeneratorContext: ";
    GetGeneratorContext(thread).Dump(thread, os);
    os << "\n";
    os << " - AsyncGeneratorQueue: ";
    GetAsyncGeneratorQueue(thread).Dump(thread, os);
    os << "\n";
    os << " - GeneratorBrand: ";
    GetGeneratorBrand(thread).Dump(thread, os);
    os << "\n";
    os << " - ResumeResult: ";
    GetResumeResult(thread).Dump(thread, os);
    os << "\n";
    os << " - AsyncGeneratorState: " << static_cast<uint8_t>(GetAsyncGeneratorState());
    os << "\n";
    os << " - ResumeMode: " << static_cast<uint8_t>(GetResumeMode());
    os << "\n";
    JSObject::Dump(thread, os);
}

void JSAsyncFuncObject::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Promise: ";
    GetPromise(thread).Dump(thread, os);
    os << "\n";
}

void GeneratorContext::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - RegsArray: ";
    GetRegsArray(thread).Dump(thread, os);
    os << "\n";
    os << " - Method: ";
    GetMethod(thread).Dump(thread, os);
    os << "\n";
    os << " - This: ";
    GetThis(thread).Dump(thread, os);
    os << "\n";
    os << " - Acc: ";
    GetAcc(thread).Dump(thread, os);
    os << "\n";
    os << " - GeneratorObject: ";
    GetGeneratorObject(thread).Dump(thread, os);
    os << "\n";
    os << " - LexicalEnv: ";
    GetLexicalEnv(thread).Dump(thread, os);
    os << "\n";
    os << " - NRegs: " << GetNRegs();
    os << "\n";
    os << " - BCOffset: " << GetBCOffset();
    os << "\n";
}

void EnumCache::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - EnumCacheOwn: " << GetEnumCacheOwn(thread);
    os << "\n";
    os << " - EnumCacheAll: " << GetEnumCacheAll(thread);
    os << "\n";
    os << " - ProtoChainInfoEnumCache: " << GetProtoChainInfoEnumCache(thread);
    os << "\n";
    os << " - EnumCacheKind: " << GetEnumCacheKind();
    os << "\n";
}

void ProtoChangeMarker::Dump([[maybe_unused]]const JSThread *thread, std::ostream &os) const
{
    os << " - HasChanged: " << GetHasChanged() << "\n";
    os << " - HasAccessorChanged: " << GetAccessorHasChanged() << "\n";
    os << " - HasNotFoundChanged: " << GetNotFoundHasChanged() << "\n";
}

void MarkerCell::Dump([[maybe_unused]] const JSThread *thread, std::ostream &os) const
{
    os << " - IsDetectorInvalid: " << GetIsDetectorInvalid() << "\n";
}

void ProtoChangeDetails::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - ChangeListener: ";
    GetChangeListener(thread).Dump(thread, os);
    os << " \t- RegisterIndex: " << GetRegisterIndex();
    os << "\n";
}

void TrackInfo::Dump([[maybe_unused]] const JSThread *thread, std::ostream &os) const
{
    os << " - ElementsKind: " << static_cast<uint32_t>(GetElementsKind()) << "\n";
    os << " - ArrayLength: " << static_cast<uint32_t>(GetArrayLength()) << "\n";
    os << " - SpaceFlag: " << static_cast<uint32_t>(GetSpaceFlag()) << "\n";
}

void MachineCode::Dump([[maybe_unused]] const JSThread *thread, std::ostream &os) const
{
    os << " - InstructionSizeInBytes: " << GetInstructionSizeInBytes();
    os << "\n";
}

void ClassInfoExtractor::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - NonStaticKeys: ";
    GetNonStaticKeys(thread).Dump(thread, os);
    os << "\n";
    os << " - NonStaticProperties: ";
    GetNonStaticProperties(thread).Dump(thread, os);
    os << "\n";
    os << " - NonStaticElements: ";
    GetNonStaticElements(thread).Dump(thread, os);
    os << "\n";
    os << " - StaticKeys: ";
    GetStaticKeys(thread).Dump(thread, os);
    os << "\n";
    os << " - StaticProperties: ";
    GetStaticProperties(thread).Dump(thread, os);
    os << "\n";
    os << " - StaticElements: ";
    GetStaticElements(thread).Dump(thread, os);
    os << "\n";
}

void SourceTextModule::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Environment: ";
    GetEnvironment(thread).Dump(thread, os);
    os << "\n";
    os << " - Namespace: ";
    GetNamespace(thread).Dump(thread, os);
    os << "\n";
    os << " - EcmaModuleFilename: ";
    os << GetEcmaModuleFilenameString();
    os << "\n";
    os << " - EcmaModuleRecordName: ";
    os << GetEcmaModuleRecordNameString();
    os << "\n";
    os << " - ModuleRequests: ";
    GetModuleRequests(thread).Dump(thread, os);
    os << "\n";
    os << " - RequestedModules: ";
    GetRequestedModules(thread).Dump(thread, os);
    os << "\n";
    os << " - ImportEntries: ";
    GetImportEntries(thread).Dump(thread, os);
    os << "\n";
    os << " - LocalExportEntries: ";
    GetLocalExportEntries(thread).Dump(thread, os);
    os << "\n";
    os << " - IndirectExportEntries: ";
    GetIndirectExportEntries(thread).Dump(thread, os);
    os << "\n";
    os << " - StarExportEntries: ";
    GetStarExportEntries(thread).Dump(thread, os);
    os << "\n";
    os << " - Status: ";
    os << static_cast<int32_t>(GetStatus());
    os << "\n";
    os << " - Exception: ";
    GetException(thread).Dump(thread, os);
    os << "\n";
    os << " - DFSIndex: ";
    os << GetDFSIndex();
    os << "\n";
    os << " - DFSAncestorIndex: ";
    os << GetDFSAncestorIndex();
    os << "\n";
    os << " - NameDictionary: ";
    GetNameDictionary(thread).Dump(thread, os);
    os << "\n";
    os << " - CycleRoot: ";
    // avoid infinite dump
    if (GetCycleRoot(thread) != JSTaggedValue(this)) {
        GetCycleRoot(thread).Dump(thread, os);
    } else {
        os << "this";
    }
    os << "\n";
    os << " - TopLevelCapability: ";
    GetTopLevelCapability(thread).Dump(thread, os);
    os << "\n";
    os << " - AsyncParentModules: ";
    GetAsyncParentModules(thread).Dump(thread, os);
    os << "\n";
    os << " - SendableEnv: ";
    GetSendableEnv(thread).Dump(thread, os);
    os << "\n";
    os << " - HasTLA: ";
    os << GetHasTLA();
    os << "\n";
    os << " - AsyncEvaluatingOrdinal: ";
    os << GetAsyncEvaluatingOrdinal();
    os << "\n";
    os << " - PendingAsyncDependencies: ";
    os << GetPendingAsyncDependencies();
    os << "\n";
}

void ImportEntry::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - ModuleRequestIndex: ";
    os << GetModuleRequestIndex();
    os << "\n";
    os << " - ImportName: ";
    GetImportName(thread).Dump(thread, os);
    os << "\n";
    os << " - LocalName: ";
    GetLocalName(thread).Dump(thread, os);
    os << "\n";
}

void LocalExportEntry::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - ExportName: ";
    GetExportName(thread).Dump(thread, os);
    os << "\n";
    os << " - LocalName: ";
    GetLocalName(thread).Dump(thread, os);
    os << "\n";
    os << " - LocalIndex: " << GetLocalIndex();
    os << "\n";
}

void IndirectExportEntry::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - ExportName: ";
    GetExportName(thread).Dump(thread, os);
    os << "\n";
    os << " - ModuleRequestIndex: ";
    os << GetModuleRequestIndex();
    os << "\n";
    os << " - ImportName: ";
    GetImportName(thread).Dump(thread, os);
    os << "\n";
}

void StarExportEntry::Dump([[maybe_unused]] const JSThread *thread, std::ostream &os) const
{
    os << " - ModuleRequestIndex: ";
    os << GetModuleRequestIndex();
    os << "\n";
}

void ResolvedBinding::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Module: ";
    GetModule(thread).Dump(thread, os);
    os << "\n";
    os << " - BindingName: ";
    GetBindingName(thread).Dump(thread, os);
    os << "\n";
}

void ResolvedIndexBinding::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Module: ";
    GetModule(thread).Dump(thread, os);
    os << "\n";
    os << " - Index: " << GetIndex();
    os << "\n";
    os << " - IsUpdatedFromResolvedBinding: " << GetIsUpdatedFromResolvedBinding();
    os << "\n";
}

void ResolvedRecordIndexBinding::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Module: ";
    GetModuleRecord(thread).Dump(thread, os);
    os << "\n";
    os << " - AbcFileName: ";
    GetAbcFileName(thread).Dump(thread, os);
    os << "\n";
    os << " - Index: " << GetIndex();
    os << "\n";
}

void ResolvedRecordBinding::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Module: ";
    GetModuleRecord(thread).Dump(thread, os);
    os << "\n";
    os << " - BindingName: ";
    GetBindingName(thread).Dump(thread, os);
    os << "\n";
}

void ModuleNamespace::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Exports: ";
    GetExports(thread).Dump(thread, os);
    os << "\n";
}

void NativeModuleFailureInfo::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - ArkNativeModuleFailureInfo: ";
    GetArkNativeModuleFailureInfo(thread).Dump(thread, os);
    os << "\n";
}

void CjsModule::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - current module path: ";
    GetPath(thread).Dump(thread, os);
    os << "\n";
    os << " - current module filename: ";
    GetFilename(thread).Dump(thread, os);
    os << "\n";
}

void CjsRequire::Dump([[maybe_unused]] const JSThread *thread, std::ostream &os) const
{
    os << " --- CjsRequire is JSFunction: ";
    os << "\n";
}

void CjsExports::Dump(const JSThread *thread, std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHClass *jshclass = GetJSHClass();
    os << " - hclass: " << std::hex << jshclass << "\n";
    os << " - prototype: ";
    jshclass->GetPrototype(thread).DumpTaggedValue(thread, os);
    os << "\n";

    TaggedArray *properties = TaggedArray::Cast(GetProperties(thread).GetTaggedObject());
    os << " - properties: " << std::hex << properties;

    if (!properties->IsDictionaryMode()) {
        JSTaggedValue attrs = jshclass->GetLayout(thread);
        if (attrs.IsNull()) {
            return;
        }

        LayoutInfo *layoutInfo = LayoutInfo::Cast(attrs.GetTaggedObject());
        int propNumber = static_cast<int>(jshclass->NumberOfProps());
        os << " <LayoutInfo[" << std::dec << propNumber << "]>\n";
        for (int i = 0; i < propNumber; i++) {
            JSTaggedValue key = layoutInfo->GetKey(thread, i);
            PropertyAttributes attr = layoutInfo->GetAttr(thread, i);
            ASSERT(i == static_cast<int>(attr.GetOffset()));
            os << "     " << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            DumpPropertyKey(thread, key, os);
            os << ": (";
            JSTaggedValue val;
            if (attr.IsInlinedProps()) {
                val = GetPropertyInlinedPropsWithRep(thread, i, attr);
            } else {
                val = properties->Get(thread, i - static_cast<int>(jshclass->GetInlinedProperties()));
            }
            val.DumpTaggedValue(thread, os);
            os << ") ";
            DumpAttr(attr, true, os);
            os << "\n";
        }
    } else {
        NameDictionary *dict = NameDictionary::Cast(properties);
        os << " <NameDictionary[" << std::dec << dict->EntriesCount() << "]>\n";
        dict->Dump(thread, os);
    }
}

void JSFunctionBase::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - Method: ";
    GetMethod(thread).Dump(thread, os);
    os << "\n";
}

void Method::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - MethodName: ";
    os << GetMethodName(thread);
    os << "\n";
    os << " - ConstantPool: ";
    GetConstantPool(thread).Dump(thread, os);
    os << "\n";
    os << " - FunctionKind: " << static_cast<int>(GetFunctionKind());
    os << "\n";
    os << " - CodeEntryOrLiteral: " << std::hex << GetCodeEntryOrLiteral() << "\n";
    os << "\n";
}

void ClassLiteral::Dump(const JSThread *thread, std::ostream &os) const
{
    os << " - ClassLiteral: ";
    os << "\n";
    os << " - IsAOTUsed: " << std::boolalpha << GetIsAOTUsed();
    os << "\n";
    os << " - Array: ";
    GetArray(thread).Dump(thread, os);
    os << "\n";
}

// ########################################################################################
// Dump for Snapshot
// ########################################################################################
static void DumpArrayClass(const JSThread *thread, const TaggedArray *arr, std::vector<Reference> &vec)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetLength();
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->Get(thread, i));
        CString str = ToCString(i);
        vec.emplace_back(str, val);
    }
}

static void DumpMutantTaggedArrayClass(const JSThread *thread, const MutantTaggedArray *arr,
                                       std::vector<Reference> &vec)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetLength();
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->Get(thread, i));
        CString str = ToCString(i);
        vec.emplace_back(str, val);
    }
}

static void DumpCOWMutantTaggedArrayClass(const JSThread *thread, const COWMutantTaggedArray *arr,
                                          std::vector<Reference> &vec)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetLength();
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->Get(thread, i));
        CString str = ToCString(i);
        vec.emplace_back(str, val);
    }
}

static void DumpElementClass(const JSThread *thread, const TaggedArray *arr, std::vector<Reference> &vec)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetLength();
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->Get(thread, i));
        vec.emplace_back(i, val, Reference::ReferenceType::ELEMENT);
    }
}

static void DumpConstantPoolClass(const JSThread *thread, const ConstantPool *arr, std::vector<Reference> &vec)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetCacheLength();
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->GetObjectFromCache(thread, i));
        CString str = ToCString(i);
        vec.emplace_back(str, val);
    }
}

static void DumpStringClass([[maybe_unused]] const EcmaString *str, [[maybe_unused]] std::vector<Reference> &vec)
{
    // Before EcmaString dump self node, it need not show, so delete.
    // If some properties need be shown, add here.
}

static void DumpObject(const JSThread *thread, TaggedObject *obj, std::vector<Reference> &vec, bool isVmMode)
{
    JSTaggedValue objValue(obj);
    DISALLOW_GARBAGE_COLLECTION;
    auto jsHclass = obj->GetClass();
    JSType type = jsHclass->GetObjectType();
    vec.emplace_back("hclass", JSTaggedValue(jsHclass));
    switch (type) {
        case JSType::HCLASS:
            JSHClass::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::TAGGED_ARRAY:
        case JSType::TAGGED_DICTIONARY:
        case JSType::LEXICAL_ENV:
        case JSType::SFUNCTION_ENV:
        case JSType::SENDABLE_ENV:
        case JSType::COW_TAGGED_ARRAY:
        case JSType::AOT_LITERAL_INFO:
            DumpArrayClass(thread, TaggedArray::Cast(obj), vec);
            break;
        case JSType::MUTANT_TAGGED_ARRAY:
            DumpMutantTaggedArrayClass(thread, MutantTaggedArray::Cast(obj), vec);
            break;
        case JSType::COW_MUTANT_TAGGED_ARRAY:
            DumpCOWMutantTaggedArrayClass(thread, COWMutantTaggedArray::Cast(obj), vec);
            break;
        case JSType::CONSTANT_POOL:
            DumpConstantPoolClass(thread, ConstantPool::Cast(obj), vec);
            break;
        case JSType::PROFILE_TYPE_INFO_CELL_0:
        case JSType::PROFILE_TYPE_INFO_CELL_1:
        case JSType::PROFILE_TYPE_INFO_CELL_N:
            ProfileTypeInfoCell::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::FUNCTION_TEMPLATE:
            FunctionTemplate::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::VTABLE:
            VTable::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::PROFILE_TYPE_INFO:
            ProfileTypeInfo::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::LINE_STRING:
        case JSType::TREE_STRING:
        case JSType::SLICED_STRING:
            DumpStringClass(EcmaString::Cast(obj), vec);
            break;
        case JSType::JS_NATIVE_POINTER:
            break;
        case JSType::JS_OBJECT:
        case JSType::JS_XREF_OBJECT:
        case JSType::JS_ERROR:
        case JSType::JS_EVAL_ERROR:
        case JSType::JS_RANGE_ERROR:
        case JSType::JS_TYPE_ERROR:
        case JSType::JS_AGGREGATE_ERROR:
        case JSType::JS_REFERENCE_ERROR:
        case JSType::JS_URI_ERROR:
        case JSType::JS_SYNTAX_ERROR:
        case JSType::JS_OOM_ERROR:
        case JSType::JS_TERMINATION_ERROR:
        case JSType::JS_ARGUMENTS:
        case JSType::JS_GLOBAL_OBJECT:
        case JSType::JS_SHARED_OBJECT:
            JSObject::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_FUNCTION_BASE:
        case JSType::JS_FUNCTION:
        case JSType::JS_SHARED_FUNCTION:
            JSFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_BOUND_FUNCTION:
            JSBoundFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_SET:
            JSSet::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_SHARED_SET:
            JSSharedSet::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_MAP:
            JSMap::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_SHARED_MAP:
            JSSharedMap::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_WEAK_SET:
            JSWeakSet::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_WEAK_MAP:
            JSWeakMap::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_WEAK_REF:
            JSWeakRef::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_FINALIZATION_REGISTRY:
            JSFinalizationRegistry::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::CELL_RECORD:
            CellRecord::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_REG_EXP:
            JSRegExp::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_DATE:
            JSDate::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_ARRAY:
            JSArray::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_SHARED_ARRAY:
            JSSharedArray::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_TYPED_ARRAY:
        case JSType::JS_INT8_ARRAY:
        case JSType::JS_UINT8_ARRAY:
        case JSType::JS_UINT8_CLAMPED_ARRAY:
        case JSType::JS_INT16_ARRAY:
        case JSType::JS_UINT16_ARRAY:
        case JSType::JS_INT32_ARRAY:
        case JSType::JS_UINT32_ARRAY:
        case JSType::JS_FLOAT32_ARRAY:
        case JSType::JS_FLOAT64_ARRAY:
        case JSType::JS_BIGINT64_ARRAY:
        case JSType::JS_BIGUINT64_ARRAY:
            JSTypedArray::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_SHARED_TYPED_ARRAY:
        case JSType::JS_SHARED_INT8_ARRAY:
        case JSType::JS_SHARED_UINT8_ARRAY:
        case JSType::JS_SHARED_UINT8_CLAMPED_ARRAY:
        case JSType::JS_SHARED_INT16_ARRAY:
        case JSType::JS_SHARED_UINT16_ARRAY:
        case JSType::JS_SHARED_INT32_ARRAY:
        case JSType::JS_SHARED_UINT32_ARRAY:
        case JSType::JS_SHARED_FLOAT32_ARRAY:
        case JSType::JS_SHARED_FLOAT64_ARRAY:
        case JSType::JS_SHARED_BIGINT64_ARRAY:
        case JSType::JS_SHARED_BIGUINT64_ARRAY:
            JSSharedTypedArray::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::BIGINT:
            BigInt::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::BYTE_ARRAY:
            ByteArray::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_PROXY:
            JSProxy::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_PRIMITIVE_REF:
            JSPrimitiveRef::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::SYMBOL:
            JSSymbol::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::ACCESSOR_DATA:
        case JSType::INTERNAL_ACCESSOR:
            AccessorData::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_DATA_VIEW:
            JSDataView::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::PROMISE_REACTIONS:
            PromiseReaction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::PROMISE_CAPABILITY:
            PromiseCapability::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::PROMISE_ITERATOR_RECORD:
            PromiseIteratorRecord::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::PROMISE_RECORD:
            PromiseRecord::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::RESOLVING_FUNCTIONS_RECORD:
            ResolvingFunctionsRecord::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_PROMISE:
            JSPromise::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_PROMISE_REACTIONS_FUNCTION:
            JSPromiseReactionsFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_PROMISE_EXECUTOR_FUNCTION:
            JSPromiseExecutorFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION:
            JSAsyncModuleFulfilledFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION:
            JSAsyncModuleRejectedFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::ASYNC_GENERATOR_REQUEST:
            AsyncGeneratorRequest::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::ASYNC_ITERATOR_RECORD:
            AsyncIteratorRecord::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_ASYNC_FROM_SYNC_ITERATOR:
            JSAsyncFromSyncIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION:
            JSAsyncFromSyncIterUnwarpFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION:
            JSPromiseAllResolveElementFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION:
            JSPromiseAnyRejectElementFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION:
            JSPromiseAllSettledElementFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_PROMISE_FINALLY_FUNCTION:
            JSPromiseFinallyFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION:
            JSPromiseValueThunkOrThrowerFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN:
            JSAsyncGeneratorResNextRetProRstFtn::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::MICRO_JOB_QUEUE:
            MicroJobQueue::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::PENDING_JOB:
            PendingJob::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::COMPLETION_RECORD:
            CompletionRecord::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_ITERATOR:
        case JSType::JS_ASYNCITERATOR:
        case JSType::JS_FORIN_ITERATOR:
        case JSType::JS_MAP_ITERATOR:
        case JSType::JS_SHARED_MAP_ITERATOR:
        case JSType::JS_SET_ITERATOR:
        case JSType::JS_SHARED_SET_ITERATOR:
        case JSType::JS_ARRAY_ITERATOR:
        case JSType::JS_SHARED_ARRAY_ITERATOR:
        case JSType::JS_STRING_ITERATOR:
        case JSType::JS_REG_EXP_ITERATOR:
        case JSType::JS_ARRAY_BUFFER:
            JSArrayBuffer::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_SHARED_ARRAY_BUFFER:
            JSArrayBuffer::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_SENDABLE_ARRAY_BUFFER:
            JSSendableArrayBuffer::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_PROXY_REVOC_FUNCTION:
            JSProxyRevocFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_ASYNC_FUNCTION:
        case JSType::JS_SHARED_ASYNC_FUNCTION:
            JSAsyncFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION:
            JSAsyncAwaitStatusFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_GENERATOR_FUNCTION:
            JSGeneratorFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_ASYNC_GENERATOR_FUNCTION:
            JSAsyncGeneratorFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_INTL_BOUND_FUNCTION:
            JSIntlBoundFunction::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_REALM:
            JSRealm::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
#ifdef ARK_SUPPORT_INTL
        case JSType::JS_INTL:
            JSIntl::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_LOCALE:
            JSLocale::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_DATE_TIME_FORMAT:
            JSDateTimeFormat::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_RELATIVE_TIME_FORMAT:
            JSRelativeTimeFormat::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_NUMBER_FORMAT:
            JSNumberFormat::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_COLLATOR:
            JSCollator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_PLURAL_RULES:
            JSPluralRules::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_DISPLAYNAMES:
            JSDisplayNames::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_SEGMENTER:
            JSSegmenter::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_SEGMENTS:
            JSSegments::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_SEGMENT_ITERATOR:
            JSSegmentIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_LIST_FORMAT:
            JSListFormat::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
#else
        case JSType::JS_INTL:
        case JSType::JS_LOCALE:
        case JSType::JS_DATE_TIME_FORMAT:
        case JSType::JS_RELATIVE_TIME_FORMAT:
        case JSType::JS_NUMBER_FORMAT:
        case JSType::JS_COLLATOR:
        case JSType::JS_PLURAL_RULES:
        case JSType::JS_DISPLAYNAMES:
        case JSType::JS_SEGMENTER:
        case JSType::JS_SEGMENTS:
        case JSType::JS_SEGMENT_ITERATOR:
        case JSType::JS_LIST_FORMAT:
            break;
#endif
        case JSType::JS_CJS_MODULE:
            CjsModule::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_CJS_EXPORTS:
            CjsExports::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_CJS_REQUIRE:
            CjsRequire::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_GENERATOR_OBJECT:
            JSGeneratorObject::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_ASYNC_GENERATOR_OBJECT:
            JSAsyncGeneratorObject::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_ASYNC_FUNC_OBJECT:
            JSAsyncFuncObject::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_GENERATOR_CONTEXT:
            GeneratorContext::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_ARRAY_LIST:
            JSAPIArrayList::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_ARRAYLIST_ITERATOR:
            JSAPIArrayListIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::LINKED_NODE:
            LinkedNode::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::RB_TREENODE:
            break;
        case JSType::JS_API_HASH_MAP:
            JSAPIHashMap::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_HASH_SET:
            JSAPIHashSet::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_HASHMAP_ITERATOR:
            JSAPIHashMapIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_HASHSET_ITERATOR:
            JSAPIHashSetIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_MAP:
            JSAPILightWeightMap::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR:
            JSAPILightWeightMapIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_SET:
            JSAPILightWeightSet::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR:
            JSAPILightWeightSetIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_TREE_MAP:
            JSAPITreeMap::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_TREE_SET:
            JSAPITreeSet::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_TREEMAP_ITERATOR:
            JSAPITreeMapIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_TREESET_ITERATOR:
            JSAPITreeSetIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_VECTOR:
            JSAPIVector::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_VECTOR_ITERATOR:
            JSAPIVectorIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_BITVECTOR:
            JSAPIBitVector::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_BITVECTOR_ITERATOR:
            JSAPIBitVectorIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_FAST_BUFFER:
            JSAPIFastBuffer::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_QUEUE:
            JSAPIQueue::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_QUEUE_ITERATOR:
            JSAPIQueueIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_DEQUE:
            JSAPIDeque::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_DEQUE_ITERATOR:
            JSAPIDequeIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_STACK:
            JSAPIStack::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_STACK_ITERATOR:
            JSAPIStackIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_LIST:
            JSAPIList::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_LINKED_LIST:
            JSAPILinkedList::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_LIST_ITERATOR:
            JSAPIListIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_LINKED_LIST_ITERATOR:
            JSAPILinkedListIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::SOURCE_TEXT_MODULE_RECORD:
            SourceTextModule::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::IMPORTENTRY_RECORD:
            ImportEntry::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::LOCAL_EXPORTENTRY_RECORD:
            LocalExportEntry::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::INDIRECT_EXPORTENTRY_RECORD:
            IndirectExportEntry::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::STAR_EXPORTENTRY_RECORD:
            StarExportEntry::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::RESOLVEDBINDING_RECORD:
            ResolvedBinding::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::RESOLVEDINDEXBINDING_RECORD:
            ResolvedIndexBinding::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::RESOLVEDRECORDINDEXBINDING_RECORD:
            ResolvedRecordIndexBinding::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::RESOLVEDRECORDBINDING_RECORD:
            ResolvedRecordBinding::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_MODULE_NAMESPACE:
            ModuleNamespace::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::NATIVE_MODULE_FAILURE_INFO:
            NativeModuleFailureInfo::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_PLAIN_ARRAY:
            JSAPIPlainArray::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        case JSType::JS_API_PLAIN_ARRAY_ITERATOR:
            JSAPIPlainArrayIterator::Cast(obj)->DumpForSnapshot(thread, vec);
            break;
        default:
            break;
    }
    if (isVmMode) {
        switch (type) {
            case JSType::PROPERTY_BOX:
                PropertyBox::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::TEMPLATE_MAP:
                DumpArrayClass(thread, TaggedArray::Cast(obj), vec);
                break;
            case JSType::GLOBAL_ENV:
                GlobalEnv::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::ENUM_CACHE:
                EnumCache::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::PROTO_CHANGE_MARKER:
                ProtoChangeMarker::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::MARKER_CELL:
                MarkerCell::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::PROTOTYPE_INFO:
                ProtoChangeDetails::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::TRACK_INFO:
                TrackInfo::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::PROGRAM:
                Program::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::MACHINE_CODE_OBJECT:
                MachineCode::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::TRANSITION_HANDLER:
                TransitionHandler::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::TRANS_WITH_PROTO_HANDLER:
                TransWithProtoHandler::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::STORE_TS_HANDLER:
                StoreAOTHandler::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::PROTOTYPE_HANDLER:
                PrototypeHandler::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::CLASS_INFO_EXTRACTOR:
                ClassInfoExtractor::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::METHOD:
                Method::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            case JSType::CLASS_LITERAL:
                ClassLiteral::Cast(obj)->DumpForSnapshot(thread, vec);
                break;
            default:
                break;
        }
    } else {
        vec.pop_back();
    }
}

static inline void EcmaStringToStd(const JSThread *thread, CString &res, EcmaString *str)
{
    if (EcmaStringAccessor(str).GetLength() == 0) {
        CString emptyStr = "EmptyString";
        res.append(emptyStr);
    }

    CString string = ConvertToString(thread, str);
    res.append(string);
}

static void KeyToStd(const JSThread *thread, CString &res, JSTaggedValue key)
{
    if (key.IsInt()) {
        res = std::to_string(key.GetInt());
    } else if (key.IsDouble()) {
        res = std::to_string(key.GetDouble());
    } else if (key.IsBoolean()) {
        res = key.IsTrue() ? "true" : "false";
    } else if (key.IsHeapObject()) {
        if (key.IsWeak()) {
            key.RemoveWeakTag();
        }
        if (key.IsString()) {
            EcmaStringToStd(thread, res, EcmaString::Cast(key.GetTaggedObject()));
        } else if (key.IsSymbol()) {
            JSSymbol *sym = JSSymbol::Cast(key.GetTaggedObject());
            JSTaggedValue desc = sym->GetDescription(thread);
            if (desc.IsString()) {
                EcmaStringToStd(thread, res, EcmaString::Cast(desc.GetTaggedObject()));
            }
        }
    }
}

void JSAPIPlainArray::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    TaggedArray *keys = TaggedArray::Cast(GetKeys(thread).GetTaggedObject());
    TaggedArray *values = TaggedArray::Cast(GetValues(thread).GetTaggedObject());
    uint32_t len = static_cast<uint32_t>(GetLength());
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        CString str;
        KeyToStd(thread, str, keys->Get(thread, i));
        vec.emplace_back(str, values->Get(thread, i));
    }
    JSObject::DumpForSnapshot(thread, vec);
}

void JSTaggedValue::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec, bool isVmMode) const
{
    if (IsHeapObject()) {
        return DumpObject(thread, GetTaggedObject(), vec, isVmMode);
    }
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

void NumberDictionary::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    vec.reserve(vec.size() + size);
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(thread, hashIndex));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            JSTaggedValue val(GetValue(thread, hashIndex));
            vec.emplace_back(
                static_cast<uint32_t>(JSTaggedNumber(key).GetNumber()), val, Reference::ReferenceType::ELEMENT);
        }
    }
}

void NameDictionary::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    vec.reserve(vec.size() + size);
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(thread, hashIndex));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            JSTaggedValue val(GetValue(thread, hashIndex));
            CString str;
            KeyToStd(thread, str, key);
            vec.emplace_back(str, val);
        }
    }
}

void GlobalDictionary::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    vec.reserve(vec.size() + size);
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(thread, hashIndex));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            CString str;
            KeyToStd(thread, str, key);
            JSTaggedValue val = GetValue(thread, hashIndex);
            vec.emplace_back(str, val);
        }
    }
}

void LinkedHashSet::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfElements() + NumberOfDeletedElements();
    vec.reserve(vec.size() + capacity);
    for (int hashIndex = 0; hashIndex < capacity; hashIndex++) {
        JSTaggedValue key(GetKey(thread, hashIndex));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            CString str;
            KeyToStd(thread, str, key);
            vec.emplace_back(str, JSTaggedValue::Hole());
        }
    }
}

void LinkedHashMap::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfElements() + NumberOfDeletedElements();
    vec.reserve(vec.size() + capacity);
    for (int hashIndex = 0; hashIndex < capacity; hashIndex++) {
        JSTaggedValue key(GetKey(thread, hashIndex));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            JSTaggedValue val = GetValue(thread, hashIndex);
            CString str;
            KeyToStd(thread, str, key);
            vec.emplace_back(str, val);
        }
    }
}

void TaggedHashArray::Dump(const JSThread *thread, std::ostream &os) const
{
    DumpArrayClass(thread, this, os);
}

void TaggedHashArray::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = static_cast<int>(GetLength());
    vec.reserve(vec.size() + capacity);
    for (int hashIndex = 0; hashIndex < capacity; hashIndex++) {
        JSTaggedValue value = Get(thread, hashIndex);
        if (!value.IsUndefined() && !value.IsHole() && !value.IsNull()) {
            LinkedNode *node = LinkedNode::Cast(value.GetTaggedObject());
            node->DumpForSnapshot(thread, vec);
        }
    }
}

template <typename T>
void DumpForSnapshotTaggedTreeEntry(const JSThread *thread, T tree, int index, std::vector<Reference> &vec,
                                    bool isMap = false)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (isMap) {
        vec.emplace_back("key", JSTaggedValue(tree->GetKey(thread, index)));
    }
    vec.emplace_back("value", JSTaggedValue(tree->GetValue(thread, index)));
    vec.emplace_back("parent", JSTaggedValue(tree->GetParent(index)));
    vec.emplace_back("color", JSTaggedValue(static_cast<int>(tree->GetColor(index))));
    vec.emplace_back("left", tree->GetLeftChild(index));
    vec.emplace_back("right", tree->GetRightChild(index));
}

void TaggedTreeMap::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    vec.emplace_back("Elements", TaggedArray::Get(thread, NUMBER_OF_ELEMENTS_INDEX));
    vec.emplace_back("Delete", TaggedArray::Get(thread, NUMBER_OF_HOLE_ENTRIES_INDEX));
    vec.emplace_back("Capacity", TaggedArray::Get(thread, CAPACITY_INDEX));
    vec.emplace_back("RootNode", TaggedArray::Get(thread, ROOT_INDEX));
    vec.emplace_back("CompareFunction", TaggedArray::Get(thread, COMPARE_FUNCTION_INDEX));
    uint32_t capacity = NumberOfElements() + NumberOfDeletedElements();
    vec.reserve(vec.size() + capacity);
    for (uint32_t index = 0; index < capacity; index++) {
        JSTaggedValue key(GetKey(thread, index));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            JSTaggedValue val = GetValue(thread, index);
            CString str;
            KeyToStd(thread, str, key);
            vec.emplace_back(str, val);
        } else {
            DumpForSnapshotTaggedTreeEntry(thread, const_cast<TaggedTreeMap *>(this), index, vec, true);
        }
    }
}

void TaggedTreeSet::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    vec.emplace_back("Elements", TaggedArray::Get(thread, NUMBER_OF_ELEMENTS_INDEX));
    vec.emplace_back("Delete", TaggedArray::Get(thread, NUMBER_OF_HOLE_ENTRIES_INDEX));
    vec.emplace_back("Capacity", TaggedArray::Get(thread, CAPACITY_INDEX));
    vec.emplace_back("RootNode", TaggedArray::Get(thread, ROOT_INDEX));
    vec.emplace_back("CompareFunction", TaggedArray::Get(thread, COMPARE_FUNCTION_INDEX));
    uint32_t capacity = NumberOfElements() + NumberOfDeletedElements();
    vec.reserve(vec.size() + capacity);
    for (uint32_t index = 0; index < capacity; index++) {
        JSTaggedValue key(GetKey(thread, index));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            CString str;
            KeyToStd(thread, str, key);
            vec.emplace_back(str, JSTaggedValue::Hole());
        } else {
            DumpForSnapshotTaggedTreeEntry(thread, const_cast<TaggedTreeSet *>(this), index, vec, true);
        }
    }
}

void TaggedDoubleList::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfNodes();
    vec.reserve(vec.size() + capacity);
    for (int index = 0; index < capacity; index++) {
        JSTaggedValue val = GetElement(thread, index);
        CString str;
        KeyToStd(thread, str, JSTaggedValue(index));
        vec.emplace_back(str, val);
    }
}

void TaggedSingleList::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfNodes();
    vec.reserve(vec.size() + capacity);
    for (int index = 0; index < capacity; index++) {
        JSTaggedValue val = GetElement(thread, index);
        CString str;
        KeyToStd(thread, str, JSTaggedValue(index));
        vec.emplace_back(str, val);
    }
}

void JSObject::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHClass *jshclass = GetJSHClass();
    if (jshclass != nullptr) {
        vec.emplace_back(CString("__proto__"), jshclass->GetPrototype(thread));
    }
    vec.emplace_back(CString("ArkInternalHash"), JSTaggedValue(GetHash(thread)));
    JSTaggedType hashField = Barriers::GetTaggedValue(thread, this, HASH_OFFSET);
    if (JSTaggedValue(hashField).IsHeapObject()) {
        vec.emplace_back(CString("HashField"), JSTaggedValue(hashField));
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements(thread).GetTaggedObject());
    vec.emplace_back("(object elements)", JSTaggedValue(elements));
    if (elements->GetLength() == 0) {
    } else if (!elements->IsDictionaryMode()) {
        DumpElementClass(thread, elements, vec);
    } else {
        NumberDictionary *dict = NumberDictionary::Cast(elements);
        dict->DumpForSnapshot(thread, vec);
    }

    TaggedArray *properties = TaggedArray::Cast(GetProperties(thread).GetTaggedObject());
    if (IsJSGlobalObject()) {
        GlobalDictionary *dict = GlobalDictionary::Cast(properties);
        dict->DumpForSnapshot(thread, vec);
        return;
    }
    vec.emplace_back("(object properties)", JSTaggedValue(properties));
    if ((!properties->IsDictionaryMode()) && (jshclass != nullptr)) {
        JSTaggedValue attrs = jshclass->GetLayout(thread);
        if (attrs.IsNull()) {
            return;
        }

        LayoutInfo *layoutInfo = LayoutInfo::Cast(attrs.GetTaggedObject());
        int propNumber = static_cast<int>(jshclass->NumberOfProps());
        vec.reserve(vec.size() + propNumber);
        for (int i = 0; i < propNumber; i++) {
            JSTaggedValue key = layoutInfo->GetKey(thread, i);
            PropertyAttributes attr = layoutInfo->GetAttr(thread, i);
            ASSERT(i == static_cast<int>(attr.GetOffset()));
            JSTaggedValue val;
            if (attr.IsInlinedProps()) {
                val = GetPropertyInlinedPropsWithRep(thread, i, attr);
            } else {
                val = properties->Get(thread, i - static_cast<int>(jshclass->GetInlinedProperties()));
            }

            CString str;
            KeyToStd(thread, str, key);
            vec.emplace_back(str, val);
        }
    } else {
        NameDictionary *dict = NameDictionary::Cast(properties);
        dict->DumpForSnapshot(thread, vec);
    }
}

void JSHClass::DumpForSnapshot(const JSThread *thread, [[maybe_unused]] std::vector<Reference> &vec) const
{
    if (!IsCompositeHClass()) {
        vec.emplace_back(CString("__proto__"), GetPrototype(thread));
        vec.emplace_back(CString("Layout"), GetLayout(thread));
        vec.emplace_back(CString("Transitions"), GetTransitions(thread));
        vec.emplace_back(CString("Parent"), GetParent(thread));
        vec.emplace_back(CString("ProtoChangeMarker"), GetProtoChangeMarker(thread));
        vec.emplace_back(CString("ProtoChangeDetails"), GetProtoChangeDetails(thread));
        vec.emplace_back(CString("EnumCache"), GetEnumCache(thread));
    }
    vec.emplace_back(CString("BitField"), JSTaggedValue(GetBitField()));
    vec.emplace_back(CString("BitField1"), JSTaggedValue(GetBitField1()));
}

void JSFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ProtoOrHClass"), GetProtoOrHClass(thread));
    vec.emplace_back(CString("LexicalEnv"), GetLexicalEnv(thread));
    vec.emplace_back(CString("RawProfileTypeInfo"), GetRawProfileTypeInfo(thread));
    vec.emplace_back(CString("HomeObject"), GetHomeObject(thread));
    vec.emplace_back(CString("Module"), GetModule(thread));
    vec.emplace_back(CString("Method"), GetMethod(thread));
    if ((!GetMethod(thread).IsNull()) && (!GetMethod(thread).IsUndefined())) {
        vec.emplace_back(CString("FunctionKind"), JSTaggedValue(static_cast<int>(GetFunctionKind(thread))));
    }
    vec.emplace_back(CString("FunctionExtraInfo"), GetFunctionExtraInfo(thread));
    vec.emplace_back(CString("Method"), GetMethod(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void Method::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("MethodName"), JSTaggedValue(GetMethodName(thread)));
    vec.emplace_back(CString("ConstantPool"), GetConstantPool(thread));
}

void Program::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("MainFunction"), GetMainFunction(thread));
}

void LinkedNode::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSTaggedValue next = GetNext(thread);
    if (!next.IsUndefined() && !next.IsHole() && !next.IsNull()) {
        LinkedNode *nextNode = LinkedNode::Cast(next.GetTaggedObject());
        vec.emplace_back(CString("Next"), next);
        nextNode->DumpForSnapshot(thread, vec);
    }
    vec.emplace_back(CString("Key"), GetKey(thread));
    JSTaggedValue key = GetKey(thread);
    CString str;
    KeyToStd(thread, str, key);
    vec.emplace_back(str, GetValue(thread));
}

void ConstantPool::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DumpArrayClass(thread, this, vec);
}

void ProfileTypeInfoCell::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Value"), GetValue(thread));
    vec.emplace_back(CString("Handle"), GetHandle(thread));
}

void FunctionTemplate::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Method"), GetMethod(thread));
    vec.emplace_back(CString("Module"), GetModule(thread));
    vec.emplace_back(CString("RawProfileTypeInfo"), GetRawProfileTypeInfo(thread));
    vec.emplace_back(CString("Length"), JSTaggedValue(GetLength()));
}

void ExtraProfileTypeInfo::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Receiver"), GetReceiverObject(thread));
    vec.emplace_back(CString("Holder"), GetHolderObject(thread));
}

void VTable::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DumpArrayClass(thread, this, vec);
}

void ProfileTypeInfo::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = GetIcSlotLength();
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(Get(thread, i));
        CString str = ToCString(i);
        vec.emplace_back(str, val);
    }
}

void COWTaggedArray::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DumpArrayClass(thread, this, vec);
}

void MutantTaggedArray::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DumpMutantTaggedArrayClass(thread, this, vec);
}

void COWMutantTaggedArray::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DumpCOWMutantTaggedArrayClass(thread, this, vec);
}

void JSBoundFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSObject::DumpForSnapshot(thread, vec);
    vec.emplace_back(CString("Method"), GetMethod(thread));
    vec.emplace_back(CString("BoundTarget"), GetBoundTarget(thread));
    vec.emplace_back(CString("BoundThis"), GetBoundThis(thread));
    vec.emplace_back(CString("BoundArguments"), GetBoundArguments(thread));
}

void JSPrimitiveRef::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("subValue"), GetValue(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void BigInt::DumpForSnapshot([[maybe_unused]] const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Length"), JSTaggedValue(GetLength()));
    vec.emplace_back(CString("Sign"), JSTaggedValue(GetSign()));
}

void JSDate::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("time"), GetTime(thread));
    vec.emplace_back(CString("localOffset"), GetLocalOffset(thread));

    JSObject::DumpForSnapshot(thread, vec);
}

void JSMap::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetLinkedMap(thread).IsInvalidValue())) {
        LinkedHashMap *map = LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject());
        vec.emplace_back("linkedmap", GetLinkedMap(thread));
        map->DumpForSnapshot(thread, vec);
    }

    JSObject::DumpForSnapshot(thread, vec);
}

void JSSharedMap::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    LinkedHashMap *map = LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject());
    vec.emplace_back("linkedmap", GetLinkedMap(thread));
    vec.emplace_back("ModRecord", JSTaggedValue(GetModRecord()));
    map->DumpForSnapshot(thread, vec);

    JSObject::DumpForSnapshot(thread, vec);
}

void JSForInIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Object"), GetObject(thread));
    vec.emplace_back(CString("CachedHclass"), GetCachedHClass(thread));
    vec.emplace_back(CString("Keys"), GetKeys(thread));
    vec.emplace_back(CString("Index"), JSTaggedValue(GetIndex()));
    vec.emplace_back(CString("Length"), JSTaggedValue(GetLength()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSMapIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedMap(thread).IsInvalidValue())) {
        LinkedHashMap *map = LinkedHashMap::Cast(GetIteratedMap(thread).GetTaggedObject());
        vec.emplace_back("iteratedmap", GetIteratedMap(thread));
        map->DumpForSnapshot(thread, vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSSharedMapIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSSharedMap *iteratedMap = JSSharedMap::Cast(GetIteratedMap(thread).GetTaggedObject());
    LinkedHashMap *map = LinkedHashMap::Cast(iteratedMap->GetLinkedMap(thread).GetTaggedObject());
    vec.emplace_back("iteratedmap", GetIteratedMap(thread));
    map->DumpForSnapshot(thread, vec);
    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSSet::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetLinkedSet(thread).IsInvalidValue())) {
        LinkedHashSet *set = LinkedHashSet::Cast(GetLinkedSet(thread).GetTaggedObject());
        vec.emplace_back("linkedset", GetLinkedSet(thread));
        set->DumpForSnapshot(thread, vec);
    }

    JSObject::DumpForSnapshot(thread, vec);
}

void JSSharedSet::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    LinkedHashSet *set = LinkedHashSet::Cast(GetLinkedSet(thread).GetTaggedObject());
    vec.emplace_back("linkedset", GetLinkedSet(thread));
    vec.emplace_back("ModRecord", JSTaggedValue(GetModRecord()));
    set->DumpForSnapshot(thread, vec);

    JSObject::DumpForSnapshot(thread, vec);
}

void JSWeakMap::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetLinkedMap(thread).IsInvalidValue())) {
        LinkedHashMap *map = LinkedHashMap::Cast(GetLinkedMap(thread).GetTaggedObject());
        vec.emplace_back("linkedmap", GetLinkedMap(thread));
        map->DumpForSnapshot(thread, vec);
    }

    JSObject::DumpForSnapshot(thread, vec);
}

void JSWeakSet::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetLinkedSet(thread).IsInvalidValue())) {
        LinkedHashSet *set = LinkedHashSet::Cast(GetLinkedSet(thread).GetTaggedObject());
        vec.emplace_back("linkeset", GetLinkedSet(thread));
        set->DumpForSnapshot(thread, vec);
    }

    JSObject::DumpForSnapshot(thread, vec);
}

void JSWeakRef::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("WeakObject"), GetWeakObject(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSFinalizationRegistry::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("CleanupCallback"), GetCleanupCallback(thread));
    if (!(GetMaybeUnregister(thread).IsInvalidValue())) {
        LinkedHashMap *map = LinkedHashMap::Cast(GetMaybeUnregister(thread).GetTaggedObject());
        vec.emplace_back(CString("MaybeUnregister"), GetMaybeUnregister(thread));
        map->DumpForSnapshot(thread, vec);
    }

    vec.emplace_back(CString("Next"), GetNext(thread));
    vec.emplace_back(CString("Prev"), GetPrev(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void CellRecord::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("WeakRefTarget"), GetWeakRefTarget(thread));
    vec.emplace_back(CString("HeldValue"), GetHeldValue(thread));
}

void JSSetIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedSet(thread).IsInvalidValue())) {
        LinkedHashSet *set = LinkedHashSet::Cast(GetIteratedSet(thread).GetTaggedObject());
        vec.emplace_back("iteratedset", GetIteratedSet(thread));
        set->DumpForSnapshot(thread, vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSSharedSetIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSSharedSet *iteratedSet = JSSharedSet::Cast(GetIteratedSet(thread).GetTaggedObject());
    LinkedHashSet *set = LinkedHashSet::Cast(iteratedSet->GetLinkedSet(thread).GetTaggedObject());
    vec.emplace_back("iteratedset", GetIteratedSet(thread));
    set->DumpForSnapshot(thread, vec);
    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSArray::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Length"), JSTaggedValue(GetLength()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSSharedArray::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back("ModRecord: ", JSTaggedValue(GetModRecord()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIArrayList::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Length"), JSTaggedValue(GetLength(thread)));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIArrayListIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedArrayList(thread).IsInvalidValue())) {
        JSAPIArrayList *arraylist = JSAPIArrayList::Cast(GetIteratedArrayList(thread).GetTaggedObject());
        vec.emplace_back("iteratedlist", GetIteratedArrayList(thread));
        arraylist->DumpForSnapshot(thread, vec);
    }
    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPILightWeightMap::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    vec.emplace_back("Hashes", GetHashes(thread));
    vec.emplace_back("Keys", GetKeys(thread));
    TaggedArray *keys = TaggedArray::Cast(GetKeys(thread).GetTaggedObject());
    TaggedArray *values = TaggedArray::Cast(GetValues(thread).GetTaggedObject());
    uint32_t len = static_cast<uint32_t>(GetLength());
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        CString str;
        KeyToStd(thread, str, keys->Get(thread, i));
        vec.emplace_back(str, values->Get(thread, i));
    }
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPILightWeightMapIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedLightWeightMap(thread).IsInvalidValue())) {
        JSAPILightWeightMap *map =
            JSAPILightWeightMap::Cast(GetIteratedLightWeightMap(thread).GetTaggedObject());
        vec.emplace_back("iteratedmap", GetIteratedLightWeightMap(thread));
        map->DumpForSnapshot(thread, vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    vec.emplace_back(CString("BitField"), JSTaggedValue(GetBitField()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIQueue::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Length"), JSTaggedValue(GetLength(thread)));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIQueueIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedQueue(thread).IsInvalidValue())) {
        JSAPIQueue *queue = JSAPIQueue::Cast(GetIteratedQueue(thread).GetTaggedObject());
        vec.emplace_back("iteratedqueue", GetIteratedQueue(thread));
        queue->DumpForSnapshot(thread, vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIDeque::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIDequeIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedDeque(thread).IsInvalidValue())) {
        JSAPIDeque *deque = JSAPIDeque::Cast(GetIteratedDeque(thread).GetTaggedObject());
        vec.emplace_back("iterateddeque", GetIteratedDeque(thread));
        deque->DumpForSnapshot(thread, vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPILightWeightSet::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    TaggedArray *hashes = TaggedArray::Cast(GetHashes(thread).GetTaggedObject());
    TaggedArray *values = TaggedArray::Cast(GetValues(thread).GetTaggedObject());
    uint32_t len = GetLength();
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        CString str;
        KeyToStd(thread, str, hashes->Get(thread, i));
        vec.emplace_back(str, values->Get(thread, i));
    }
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPILightWeightSetIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedLightWeightSet(thread).IsInvalidValue())) {
        JSAPILightWeightSet *set =
            JSAPILightWeightSet::Cast(GetIteratedLightWeightSet(thread).GetTaggedObject());
        vec.emplace_back("iteratedset", GetIteratedLightWeightSet(thread));
        set->DumpForSnapshot(thread, vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIStack::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIStackIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedStack(thread).IsInvalidValue())) {
        JSAPIStack *stack = JSAPIStack::Cast(GetIteratedStack(thread).GetTaggedObject());
        vec.emplace_back("iteratedstack", GetIteratedStack(thread));
        stack->DumpForSnapshot(thread, vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSArrayIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedArray(thread).IsInvalidValue())) {
        JSArray *array = JSArray::Cast(GetIteratedArray(thread).GetTaggedObject());
        vec.emplace_back("iteratedarray", GetIteratedArray(thread));
        array->DumpForSnapshot(thread, vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    vec.emplace_back(CString("BitField"), JSTaggedValue(GetBitField()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSSharedArrayIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSSharedArray *array = JSSharedArray::Cast(GetIteratedArray(thread).GetTaggedObject());
    vec.emplace_back("iteratedarray", GetIteratedArray(thread));
    array->DumpForSnapshot(thread, vec);
    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIVector::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIVectorIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (!(GetIteratedVector(thread).IsInvalidValue())) {
        JSAPIVector *vector = JSAPIVector::Cast(GetIteratedVector(thread).GetTaggedObject());
        vec.emplace_back("iteratedvector", GetIteratedVector(thread));
        vector->DumpForSnapshot(thread, vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIBitVector::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIBitVectorIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSAPIVector *vector = JSAPIVector::Cast(GetIteratedBitVector(thread).GetTaggedObject());
    vec.emplace_back("iteratedbitvector", GetIteratedBitVector(thread));
    vector->DumpForSnapshot(thread, vec);
    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAPIFastBuffer::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSObject::DumpForSnapshot(thread, vec);
}

void JSStringIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("IteratedString"), GetIteratedString(thread));
    vec.emplace_back(CString("StringIteratorNextIndex"), JSTaggedValue(GetStringIteratorNextIndex()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSTypedArray::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 5;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("viewed-array-buffer"), GetViewedArrayBufferOrByteArray(thread));
    vec.emplace_back(CString("typed-array-name"), GetTypedArrayName(thread));
    vec.emplace_back(CString("byte-length"), JSTaggedValue(GetByteLength()));
    vec.emplace_back(CString("byte-offset"), JSTaggedValue(GetByteOffset()));
    vec.emplace_back(CString("array-length"), JSTaggedValue(GetArrayLength()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSSharedTypedArray::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 5;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("viewed-array-buffer"), GetViewedArrayBufferOrByteArray(thread));
    vec.emplace_back(CString("typed-array-name"), GetTypedArrayName(thread));
    vec.emplace_back(CString("byte-length"), JSTaggedValue(GetByteLength()));
    vec.emplace_back(CString("byte-offset"), JSTaggedValue(GetByteOffset()));
    vec.emplace_back(CString("array-length"), JSTaggedValue(GetArrayLength()));
    JSObject::DumpForSnapshot(thread, vec);
}

void ByteArray::DumpForSnapshot([[maybe_unused]] const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("array-length"), JSTaggedValue(GetArrayLength()));
    vec.emplace_back(CString("byte-length"), JSTaggedValue(GetByteLength()));
}

void JSRegExp::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ByteCodeBuffer"), GetByteCodeBuffer(thread));
    vec.emplace_back(CString("originalSource"), GetOriginalSource(thread));
    vec.emplace_back(CString("originalFlags"), GetOriginalFlags(thread));
    vec.emplace_back(CString("groupName"), GetGroupName(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSRegExpIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("IteratingRegExp"), GetIteratingRegExp(thread));
    vec.emplace_back(CString("IteratedString"), GetIteratedString(thread));
    vec.emplace_back(CString("Global"), JSTaggedValue(GetGlobal()));
    vec.emplace_back(CString("Unicode"), JSTaggedValue(GetUnicode()));
    vec.emplace_back(CString("Done"), JSTaggedValue(GetDone()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSProxy::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("target"), GetTarget(thread));
    vec.emplace_back(CString("handler"), GetHandler(thread));
    vec.emplace_back(CString("Method"), GetMethod(thread));
    vec.emplace_back(CString("PrivateField"), GetPrivateField(thread));
    vec.emplace_back(CString("BitField"), JSTaggedValue(GetBitField()));
}

void JSSymbol::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("hash-field"), JSTaggedValue(GetHashField()));
    vec.emplace_back(CString("flags"), JSTaggedValue(GetFlags()));
    vec.emplace_back(CString("description"), GetDescription(thread));
}

void AccessorData::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    if (GetClass()->GetObjectType() == JSType::INTERNAL_ACCESSOR) {
        vec.emplace_back(CString("getter"), JSTaggedValue(InternalAccessor::ConstCast(this)->GetGetter()));
        vec.emplace_back(CString("setter"), JSTaggedValue(InternalAccessor::ConstCast(this)->GetSetter()));
        return;
    }

    vec.emplace_back(CString("getter"), GetGetter(thread));
    vec.emplace_back(CString("setter"), GetSetter(thread));
}

void LexicalEnv::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DumpArrayClass(thread, this, vec);
}

void SFunctionEnv::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DumpArrayClass(thread, this, vec);
}

void SendableEnv::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    DumpArrayClass(thread, this, vec);
}

void GlobalEnv::DumpForSnapshot([[maybe_unused]] const JSThread *thread, std::vector<Reference> &vec) const
{
    auto globalConst = GetJSThread()->GlobalConstants();
#define DUMP_ENV_FIELD(type, name, _) vec.emplace_back(#name, GetRaw##name().GetTaggedValue());
#define DUMP_CONST_FIELD(type, name, ...) vec.emplace_back(#name, globalConst->Get##name());
#define DUMP_CONST_STRING(name, ...) vec.emplace_back(#name, globalConst->Get##name());

    GLOBAL_ENV_FIELDS(DUMP_ENV_FIELD)

    GLOBAL_ENV_CONSTANT_CLASS(DUMP_CONST_FIELD)

    GLOBAL_ENV_CONSTANT_SPECIAL(DUMP_CONST_FIELD)

    GLOBAL_ENV_CACHES(DUMP_CONST_FIELD)

#undef DUMP_FIELD
#undef DUMP_CONST_FIELD
#undef DUMP_CONST_STRING
}

void JSDataView::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("data-view"), GetDataView(thread));
    vec.emplace_back(CString("buffer"), GetViewedArrayBuffer(thread));
    vec.emplace_back(CString("byte-length"), JSTaggedValue(GetByteLength()));
    vec.emplace_back(CString("byte-offset"), JSTaggedValue(GetByteOffset()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSArrayBuffer::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("buffer-data"), GetArrayBufferData(thread));
    vec.emplace_back(CString("byte-length"), JSTaggedValue(GetArrayBufferByteLength()));
    vec.emplace_back(CString("shared"), JSTaggedValue(GetShared()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSSendableArrayBuffer::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("buffer-data"), GetArrayBufferData(thread));
    vec.emplace_back(CString("byte-length"), JSTaggedValue(GetArrayBufferByteLength()));
    vec.emplace_back(CString("shared"), JSTaggedValue(GetShared()));
    JSObject::DumpForSnapshot(thread, vec);
}

void PromiseReaction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("promise-capability"), GetPromiseCapability(thread));
    vec.emplace_back(CString("handler"), GetHandler(thread));
    vec.emplace_back(CString("type"), JSTaggedValue(static_cast<int>(GetType())));
}

void PromiseCapability::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("promise"), GetPromise(thread));
    vec.emplace_back(CString("resolve"), GetResolve(thread));
    vec.emplace_back(CString("reject"), GetReject(thread));
}

void PromiseIteratorRecord::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("iterator"), GetIterator(thread));
    vec.emplace_back(CString("done"), JSTaggedValue(GetDone()));
}

void PromiseRecord::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("value"), GetValue(thread));
}

void ResolvingFunctionsRecord::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("resolve-function"), GetResolveFunction(thread));
    vec.emplace_back(CString("reject-function"), GetRejectFunction(thread));
}

void AsyncGeneratorRequest::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("completion"), GetCompletion(thread));
    vec.emplace_back(CString("capability"), GetCapability(thread));
}

void AsyncIteratorRecord::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("iterator"), GetIterator(thread));
    vec.emplace_back(CString("nextmethod"), GetNextMethod(thread));
    vec.emplace_back(CString("done"), JSTaggedValue(GetDone()));
}

void JSAsyncFromSyncIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("synciteratorrecord"), GetSyncIteratorRecord(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAsyncFromSyncIterUnwarpFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("done"), JSTaggedValue(GetDone(thread)));
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSPromise::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("promise-state"), JSTaggedValue(static_cast<int>(GetPromiseState())));
    vec.emplace_back(CString("promise-result"), GetPromiseResult(thread));
    vec.emplace_back(CString("promise-fulfill-reactions"), GetPromiseFulfillReactions(thread));
    vec.emplace_back(CString("promise-reject-reactions"), GetPromiseRejectReactions(thread));
    vec.emplace_back(CString("promise-is-handled"), JSTaggedValue(GetPromiseIsHandled()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSPromiseReactionsFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("promise"), GetPromise(thread));
    vec.emplace_back(CString("already-resolved"), GetAlreadyResolved(thread));
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSAsyncGeneratorResNextRetProRstFtn::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("async-generator-object"), GetAsyncGeneratorObject(thread));
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSPromiseExecutorFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("capability"), GetCapability(thread));
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSAsyncModuleFulfilledFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("module"), GetModule(thread));
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSAsyncModuleRejectedFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("module"), GetModule(thread));
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSPromiseAllResolveElementFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("index"), GetIndex(thread));
    vec.emplace_back(CString("values"), GetValues(thread));
    vec.emplace_back(CString("capabilities"), GetCapabilities(thread));
    vec.emplace_back(CString("remaining-elements"), GetRemainingElements(thread));
    vec.emplace_back(CString("already-called"), GetAlreadyCalled(thread));
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSPromiseAnyRejectElementFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("index"), JSTaggedValue(GetIndex()));
    vec.emplace_back(CString("errors"), GetErrors(thread));
    vec.emplace_back(CString("capability"), GetCapability(thread));
    vec.emplace_back(CString("remaining-elements"), GetRemainingElements(thread));
    vec.emplace_back(CString("already-called"), GetAlreadyCalled(thread));
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSPromiseAllSettledElementFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("already-called"), GetAlreadyCalled(thread));
    vec.emplace_back(CString("index"), JSTaggedValue(GetIndex()));
    vec.emplace_back(CString("values"), GetValues(thread));
    vec.emplace_back(CString("capability"), GetCapability(thread));
    vec.emplace_back(CString("remaining-elements"), GetRemainingElements(thread));
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSPromiseFinallyFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("constructor"), GetConstructor(thread));
    vec.emplace_back(CString("onFinally"), GetOnFinally(thread));
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSPromiseValueThunkOrThrowerFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("result"), GetResult(thread));
    JSFunction::DumpForSnapshot(thread, vec);
}

void MicroJobQueue::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("promise-job-queue"), GetPromiseJobQueue(thread));
    vec.emplace_back(CString("script-job-queue"), GetScriptJobQueue(thread));
}

void PendingJob::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("job"), GetJob(thread));
    vec.emplace_back(CString("arguments"), GetArguments(thread));
}

void CompletionRecord::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("value"), GetValue(thread));
    vec.emplace_back(CString("type"), JSTaggedValue(static_cast<int>(GetType())));
}

void JSProxyRevocFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("RevocableProxy"), GetRevocableProxy(thread));
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSAsyncFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSAsyncAwaitStatusFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("AsyncContext"), GetAsyncContext(thread));
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSGeneratorFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSAsyncGeneratorFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    JSFunction::DumpForSnapshot(thread, vec);
}

void JSIntlBoundFunction::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("NumberFormat"), GetNumberFormat(thread));
    vec.emplace_back(CString("DateTimeFormat"), GetDateTimeFormat(thread));
    vec.emplace_back(CString("Collator"), GetCollator(thread));
    JSFunction::DumpForSnapshot(thread, vec);
}

void PropertyBox::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Value"), GetValue(thread));
}

void PrototypeHandler::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("HandlerInfo"), GetHandlerInfo(thread));
    vec.emplace_back(CString("ProtoCell"), GetProtoCell(thread));
    vec.emplace_back(CString("Holder"), GetHolder(thread));
}

void TransitionHandler::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("HandlerInfo"), GetHandlerInfo(thread));
    vec.emplace_back(CString("TransitionHClass"), GetTransitionHClass(thread));
}

void TransWithProtoHandler::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("HandlerInfo"), GetHandlerInfo(thread));
    vec.emplace_back(CString("TransitionHClass"), GetTransitionHClass(thread));
    vec.emplace_back(CString("ProtoCell"), GetProtoCell(thread));
}

void StoreAOTHandler::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("HandlerInfo"), GetHandlerInfo(thread));
    vec.emplace_back(CString("ProtoCell"), GetProtoCell(thread));
    vec.emplace_back(CString("Holder"), GetHolder(thread));
}

void JSRealm::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Value"), GetValue(thread));
    vec.emplace_back(CString("GLobalEnv"), GetGlobalEnv(thread));
    JSObject::DumpForSnapshot(thread, vec);
}
#ifdef ARK_SUPPORT_INTL
void JSIntl::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("FallbackSymbol"), GetFallbackSymbol(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSLocale::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("IcuField"), GetIcuField(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSDateTimeFormat::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 11;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("Locale"), GetLocale(thread));
    vec.emplace_back(CString("Calendar"), GetCalendar(thread));
    vec.emplace_back(CString("NumberingSystem"), GetNumberingSystem(thread));
    vec.emplace_back(CString("TimeZone"), GetTimeZone(thread));
    vec.emplace_back(CString("HourCycle"), JSTaggedValue(static_cast<int>(GetHourCycle())));
    vec.emplace_back(CString("LocaleIcu"), GetLocaleIcu(thread));
    vec.emplace_back(CString("SimpleDateTimeFormatIcu"), GetSimpleDateTimeFormatIcu(thread));
    vec.emplace_back(CString("Iso8601"), GetIso8601(thread));
    vec.emplace_back(CString("DateStyle"), JSTaggedValue(static_cast<int>(GetDateStyle())));
    vec.emplace_back(CString("TimeStyle"), JSTaggedValue(static_cast<int>(GetTimeStyle())));
    vec.emplace_back(CString("BoundFormat"), GetBoundFormat(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSRelativeTimeFormat::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Locale"), GetLocale(thread));
    vec.emplace_back(CString("NumberingSystem"), GetNumberingSystem(thread));
    vec.emplace_back(CString("Style"), JSTaggedValue(static_cast<int>(GetStyle())));
    vec.emplace_back(CString("Numeric"), JSTaggedValue(static_cast<int>(GetNumeric())));
    vec.emplace_back(CString("IcuField"), GetIcuField(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSNumberFormat::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 20;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("Locale"), GetLocale(thread));
    vec.emplace_back(CString("NumberingSystem"), GetNumberingSystem(thread));
    vec.emplace_back(CString("Style"), JSTaggedValue(static_cast<int>(GetStyle())));
    vec.emplace_back(CString("Currency"), GetCurrency(thread));
    vec.emplace_back(CString("CurrencyDisplay"), JSTaggedValue(static_cast<int>(GetCurrencyDisplay())));
    vec.emplace_back(CString("CurrencySign"), JSTaggedValue(static_cast<int>(GetCurrencySign())));
    vec.emplace_back(CString("Unit"), GetUnit(thread));
    vec.emplace_back(CString("UnitDisplay"), JSTaggedValue(static_cast<int>(GetUnitDisplay())));
    vec.emplace_back(CString("MinimumIntegerDigits"), GetMinimumIntegerDigits(thread));
    vec.emplace_back(CString("MinimumFractionDigits"), GetMinimumFractionDigits(thread));
    vec.emplace_back(CString("MaximumFractionDigits"), GetMaximumFractionDigits(thread));
    vec.emplace_back(CString("MinimumSignificantDigits"), GetMinimumSignificantDigits(thread));
    vec.emplace_back(CString("MaximumSignificantDigits"), GetMaximumSignificantDigits(thread));
    vec.emplace_back(CString("UseGrouping"), GetUseGrouping(thread));
    vec.emplace_back(CString("RoundingType"), JSTaggedValue(static_cast<int>(GetRoundingType())));
    vec.emplace_back(CString("Notation"), JSTaggedValue(static_cast<int>(GetNotation())));
    vec.emplace_back(CString("CompactDisplay"), JSTaggedValue(static_cast<int>(GetCompactDisplay())));
    vec.emplace_back(CString("SignDisplay"), JSTaggedValue(static_cast<int>(GetSignDisplay())));
    vec.emplace_back(CString("BoundFormat"), GetBoundFormat(thread));
    vec.emplace_back(CString("IcuField"), GetIcuField(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSCollator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 9;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("IcuField"), GetIcuField(thread));
    vec.emplace_back(CString("Locale"), GetLocale(thread));
    vec.emplace_back(CString("Collation"), GetCollation(thread));
    vec.emplace_back(CString("BoundCompare"), GetBoundCompare(thread));
    vec.emplace_back(CString("CaseFirst"), JSTaggedValue(static_cast<int>(GetCaseFirst())));
    vec.emplace_back(CString("Usage"), JSTaggedValue(static_cast<int>(GetUsage())));
    vec.emplace_back(CString("Sensitivity"), JSTaggedValue(static_cast<int>(GetSensitivity())));
    vec.emplace_back(CString("IgnorePunctuation"), JSTaggedValue(GetIgnorePunctuation()));
    vec.emplace_back(CString("Numeric"), JSTaggedValue(GetNumeric()));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSPluralRules::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 10;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("Locale"), GetLocale(thread));
    vec.emplace_back(CString("MinimumIntegerDigits"), GetMinimumIntegerDigits(thread));
    vec.emplace_back(CString("MinimumFractionDigits"), GetMinimumFractionDigits(thread));
    vec.emplace_back(CString("MaximumFractionDigits"), GetMaximumFractionDigits(thread));
    vec.emplace_back(CString("MinimumSignificantDigits"), GetMinimumSignificantDigits(thread));
    vec.emplace_back(CString("MaximumSignificantDigits"), GetMaximumSignificantDigits(thread));
    vec.emplace_back(CString("RoundingType"), JSTaggedValue(static_cast<int>(GetRoundingType())));
    vec.emplace_back(CString("IcuPR"), GetIcuPR(thread));
    vec.emplace_back(CString("IcuNF"), GetIcuNF(thread));
    vec.emplace_back(CString("Type"), JSTaggedValue(static_cast<int>(GetType())));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSDisplayNames::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Locale"), GetLocale(thread));
    vec.emplace_back(CString("Type"), JSTaggedValue(static_cast<int>(GetType())));
    vec.emplace_back(CString("Style"), JSTaggedValue(static_cast<int>(GetStyle())));
    vec.emplace_back(CString("Fallback"), JSTaggedValue(static_cast<int>(GetFallback())));
    vec.emplace_back(CString("IcuLDN"), GetIcuLDN(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSSegmenter::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Locale"), GetLocale(thread));
    vec.emplace_back(CString("Granularity"), JSTaggedValue(static_cast<int>(GetGranularity())));
    vec.emplace_back(CString("IcuField"), GetIcuField(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSSegments::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("SegmentsString"), GetSegmentsString(thread));
    vec.emplace_back(CString("UnicodeString"), GetUnicodeString(thread));
    vec.emplace_back(CString("Granularity"), JSTaggedValue(static_cast<int>(GetGranularity())));
    vec.emplace_back(CString("IcuField"), GetIcuField(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSSegmentIterator::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("IteratedString"), GetIteratedString(thread));
    vec.emplace_back(CString("UnicodeString"), GetUnicodeString(thread));
    vec.emplace_back(CString("Granularity"), JSTaggedValue(static_cast<int>(GetGranularity())));
    vec.emplace_back(CString("IcuField"), GetIcuField(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSListFormat::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Locale"), GetLocale(thread));
    vec.emplace_back(CString("Type"), JSTaggedValue(static_cast<int>(GetType())));
    vec.emplace_back(CString("Style"), JSTaggedValue(static_cast<int>(GetStyle())));
    vec.emplace_back(CString("IcuLF"), GetIcuLF(thread));
    JSObject::DumpForSnapshot(thread, vec);
}
#endif
void JSGeneratorObject::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("GeneratorContext"), GetGeneratorContext(thread));
    vec.emplace_back(CString("ResumeResult"), GetResumeResult(thread));
    vec.emplace_back(CString("GeneratorState"), JSTaggedValue(static_cast<int>(GetGeneratorState())));
    vec.emplace_back(CString("ResumeMode"), JSTaggedValue(static_cast<int>(GetResumeMode())));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAsyncGeneratorObject::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("GeneratorContext"), GetGeneratorContext(thread));
    vec.emplace_back(CString("AsyncGeneratorQueue"), GetAsyncGeneratorQueue(thread));
    vec.emplace_back(CString("GeneratorBrand"), GetGeneratorBrand(thread));
    vec.emplace_back(CString("ResumeResult"), GetResumeResult(thread));
    vec.emplace_back(CString("AsyncGeneratorState"), JSTaggedValue(static_cast<int>(GetAsyncGeneratorState())));
    vec.emplace_back(CString("ResumeMode"), JSTaggedValue(static_cast<int>(GetResumeMode())));
    JSObject::DumpForSnapshot(thread, vec);
}

void JSAsyncFuncObject::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Promise"), GetPromise(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void GeneratorContext::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 8;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("RegsArray"), GetRegsArray(thread));
    vec.emplace_back(CString("Method"), GetMethod(thread));
    vec.emplace_back(CString("This"), GetThis(thread));
    vec.emplace_back(CString("Acc"), GetAcc(thread));
    vec.emplace_back(CString("GeneratorObject"), GetGeneratorObject(thread));
    vec.emplace_back(CString("LexicalEnv"), GetLexicalEnv(thread));
    vec.emplace_back(CString("NRegs"),  JSTaggedValue(GetNRegs()));
    vec.emplace_back(CString("BCOffset"),  JSTaggedValue(GetBCOffset()));
}

void EnumCache::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    constexpr int16_t NUM_OF_ITEMS = 4;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("EnumCacheOwn"), GetEnumCacheOwn(thread));
    vec.emplace_back(CString("EnumCacheAll"), GetEnumCacheAll(thread));
    vec.emplace_back(CString("ProtoChainInfoEnumCache"), GetProtoChainInfoEnumCache(thread));
    vec.emplace_back(CString("EnumCacheKind"), JSTaggedValue(GetEnumCacheKind()));
}

void ProtoChangeMarker::DumpForSnapshot([[maybe_unused]] const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("HasChanged"), JSTaggedValue(GetHasChanged()));
}

void MarkerCell::DumpForSnapshot([[maybe_unused]] const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("IsDetectorInvalid"), JSTaggedValue(GetIsDetectorInvalid()));
}

void ProtoChangeDetails::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ChangeListener"), GetChangeListener(thread));
    vec.emplace_back(CString("RegisterIndex"), JSTaggedValue(GetRegisterIndex()));
}

void MachineCode::DumpForSnapshot([[maybe_unused]] const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("InstructionSizeInBytes"), JSTaggedValue(GetInstructionSizeInBytes()));
}

void TrackInfo::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back("ElementsKind", JSTaggedValue(static_cast<uint32_t>(GetElementsKind())));

    vec.emplace_back(CString("CachedHClass"), GetCachedHClass(thread));
    vec.emplace_back(CString("CachedFunc"), GetCachedFunc(thread));
    vec.emplace_back(CString("ArrayLength"), JSTaggedValue(GetArrayLength()));
}

void ClassInfoExtractor::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 6;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("NonStaticKeys"), GetNonStaticKeys(thread));
    vec.emplace_back(CString("NonStaticProperties"), GetNonStaticProperties(thread));
    vec.emplace_back(CString("NonStaticElements"), GetNonStaticElements(thread));
    vec.emplace_back(CString("StaticKeys"), GetStaticKeys(thread));
    vec.emplace_back(CString("StaticProperties"), GetStaticProperties(thread));
    vec.emplace_back(CString("StaticElements"), GetStaticElements(thread));
    vec.emplace_back(CString("ConstructorMethod"), GetConstructorMethod(thread));
    vec.emplace_back(CString("BitField"), JSTaggedValue(GetBitField()));
}

void SourceTextModule::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 14;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("Environment"), GetEnvironment(thread));
    vec.emplace_back(CString("Namespace"), GetNamespace(thread));
    vec.emplace_back(CString("ModuleRequests"), GetModuleRequests(thread));
    vec.emplace_back(CString("RequestedModules"), GetRequestedModules(thread));
    vec.emplace_back(CString("ImportEntries"), GetImportEntries(thread));
    vec.emplace_back(CString("LocalExportEntries"), GetLocalExportEntries(thread));
    vec.emplace_back(CString("IndirectExportEntries"), GetIndirectExportEntries(thread));
    vec.emplace_back(CString("StarExportEntries"), GetStarExportEntries(thread));
    vec.emplace_back(CString("Status"), JSTaggedValue(static_cast<int32_t>(GetStatus())));
    vec.emplace_back(CString("Exception"), GetException(thread));
    vec.emplace_back(CString("DFSIndex"), JSTaggedValue(GetDFSIndex()));
    vec.emplace_back(CString("DFSAncestorIndex"), JSTaggedValue(GetDFSAncestorIndex()));
    vec.emplace_back(CString("NameDictionary"), GetNameDictionary(thread));
    vec.emplace_back(CString("CycleRoot"), GetCycleRoot(thread));
    vec.emplace_back(CString("TopLevelCapability"), GetTopLevelCapability(thread));
    vec.emplace_back(CString("AsyncParentModules"), GetAsyncParentModules(thread));
    vec.emplace_back(CString("SendableEnv"), GetSendableEnv(thread));
    vec.emplace_back(CString("HasTLA"), JSTaggedValue(GetHasTLA()));
    vec.emplace_back(CString("AsyncEvaluatingOrdinal"), JSTaggedValue(GetAsyncEvaluatingOrdinal()));
    vec.emplace_back(CString("PendingAsyncDependencies"), JSTaggedValue(GetPendingAsyncDependencies()));
}

void ImportEntry::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ModuleRequestIndex"), JSTaggedValue(GetModuleRequestIndex()));
    vec.emplace_back(CString("ImportName"), GetImportName(thread));
    vec.emplace_back(CString("LocalName"), GetLocalName(thread));
}

void LocalExportEntry::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ExportName"), GetExportName(thread));
    vec.emplace_back(CString("LocalName"), GetLocalName(thread));
}

void IndirectExportEntry::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ExportName"), GetExportName(thread));
    vec.emplace_back(CString("ModuleRequest"), JSTaggedValue(GetModuleRequestIndex()));
    vec.emplace_back(CString("ImportName"), GetImportName(thread));
}

void StarExportEntry::DumpForSnapshot([[maybe_unused]] const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ModuleRequest"), JSTaggedValue(GetModuleRequestIndex()));
}

void ResolvedBinding::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Module"), GetModule(thread));
    vec.emplace_back(CString("BindingName"), GetBindingName(thread));
}

void ResolvedIndexBinding::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Module"), GetModule(thread));
    vec.emplace_back(CString("Index"), JSTaggedValue(GetIndex()));
    vec.emplace_back(CString("IsUpdatedFromResolvedBinding"), JSTaggedValue(GetIsUpdatedFromResolvedBinding()));
}

void ResolvedRecordIndexBinding::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ModuleRecord"), GetModuleRecord(thread));
    vec.emplace_back(CString("AbcFileName"), GetAbcFileName(thread));
    vec.emplace_back(CString("Index"), JSTaggedValue(GetIndex()));
}

void ResolvedRecordBinding::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ModuleRecord"), GetModuleRecord(thread));
    vec.emplace_back(CString("BindingName"), GetBindingName(thread));
}

void ModuleNamespace::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Module"), GetModule(thread));
    vec.emplace_back(CString("Exports"), GetExports(thread));
    vec.emplace_back(CString("DeregisterProcession"), GetDeregisterProcession(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void NativeModuleFailureInfo::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ArkNativeModuleFailureInfo"), GetArkNativeModuleFailureInfo(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void CjsModule::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Id"), GetId(thread));
    vec.emplace_back(CString("Path"), GetPath(thread));
    vec.emplace_back(CString("Exports"), GetExports(thread));
    vec.emplace_back(CString("Filename"), GetFilename(thread));
    vec.emplace_back(CString("BitField"), JSTaggedValue(GetBitField()));
    JSObject::DumpForSnapshot(thread, vec);
}

void CjsExports::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Exports"), GetExports(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void CjsRequire::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Cache"), GetCache(thread));
    vec.emplace_back(CString("Parent"), GetParent(thread));
    JSObject::DumpForSnapshot(thread, vec);
}

void ClassLiteral::DumpForSnapshot(const JSThread *thread, std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Array"), GetArray(thread));
    vec.emplace_back(CString("IsAOTUsed"), JSTaggedValue(GetIsAOTUsed()));
    if (!GetArray(thread).IsUndefined()) {
        DumpArrayClass(thread, TaggedArray::Cast(GetArray(thread).GetTaggedObject()), vec);
    }
}
}  // namespace panda::ecmascript
