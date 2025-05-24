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

static void DumpArrayClass(const TaggedArray *arr, std::ostream &os)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetLength();
    os << " <TaggedArray[" << std::dec << len << "]>\n";
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->Get(i));
        if (!val.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET) << i << ": ";
            val.DumpTaggedValue(os);
            os << "\n";
        }
    }
}

static void DumpMutantTaggedArray(const MutantTaggedArray *arr, std::ostream &os)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetLength();
    os << " <MutantTaggedArray[" << std::dec << len << "]>\n";
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->Get(i));
        os << std::right << std::setw(DUMP_PROPERTY_OFFSET) << i << ": ";
        os << std::left << std::setw(DUMP_TYPE_OFFSET) << "[JSTaggedType] : " << val.GetRawData();
        os << "\n";
    }
}

static void DumpCOWMutantTaggedArray(const COWMutantTaggedArray *arr, std::ostream &os)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetLength();
    os << " <COWMutantTaggedArray[" << std::dec << len << "]>\n";
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->Get(i));
        os << std::right << std::setw(DUMP_PROPERTY_OFFSET) << i << ": ";
        os << std::left << std::setw(DUMP_TYPE_OFFSET) << "[JSTaggedType] : " << val.GetRawData();
        os << "\n";
    }
}

static void DumpConstantPoolClass(const ConstantPool *pool, std::ostream &os)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = pool->GetCacheLength();
    os << " <ConstantPool[" << std::dec << len << "]>\n";
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(pool->GetObjectFromCache(i));
        if (!val.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET) << i << ": ";
            val.DumpTaggedValue(os);
            os << "\n";
        }
    }
}

static void DumpStringClass(const EcmaString *str, std::ostream &os)
{
    DISALLOW_GARBAGE_COLLECTION;
    CString string = ConvertToString(str);
    os << string;
}

static void DumpPropertyKey(JSTaggedValue key, std::ostream &os)
{
    if (key.IsString()) {
        DumpStringClass(EcmaString::Cast(key.GetTaggedObject()), os);
    } else if (key.IsSymbol()) {
        JSSymbol *sym = JSSymbol::Cast(key.GetTaggedObject());
        if (sym->GetDescription().IsString()) {
            os << "Symbol(\"";
            DumpStringClass(EcmaString::Cast(sym->GetDescription().GetTaggedObject()), os);
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

static void DumpHClass(const JSHClass *jshclass, std::ostream &os, bool withDetail)
{
    DISALLOW_GARBAGE_COLLECTION;
    os << "JSHClass :" << std::setw(DUMP_TYPE_OFFSET);
    os << "Type :" << JSHClass::DumpJSType(jshclass->GetObjectType()) << "\n";

    os << " - Prototype :" << std::setw(DUMP_TYPE_OFFSET);
    jshclass->GetPrototype().DumpTaggedValue(os);
    os << "\n";
    os << " - PropertyDescriptors :" << std::setw(DUMP_TYPE_OFFSET);
    JSTaggedValue attrs = jshclass->GetLayout();
    attrs.DumpTaggedValue(os);
    os << "\n";
    if (withDetail && !attrs.IsNull()) {
        LayoutInfo *layout = LayoutInfo::Cast(jshclass->GetLayout().GetTaggedObject());
        int element = static_cast<int>(jshclass->NumberOfProps());
        for (int i = 0; i < element; i++) {
            JSTaggedValue key = layout->GetKey(i);
            PropertyAttributes attr = layout->GetAttr(i);
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            os << "[" << i << "]: ";
            DumpPropertyKey(key, os);
            os << " : ";
            DumpAttr(attr, true, os);
            os << "\n";
        }
    }

    os << " - Transitions :" << std::setw(DUMP_TYPE_OFFSET);
    JSTaggedValue transtions = jshclass->GetTransitions();
    transtions.DumpTaggedValue(os);
    os << "\n";
    if (withDetail && !transtions.IsWeakForHeapObject() && transtions.IsDictionary()) {
        transtions.Dump(os);
    }

    os << " - ProtoChangeMarker :" << std::setw(DUMP_TYPE_OFFSET);
    JSTaggedValue marker = jshclass->GetProtoChangeMarker();
    marker.DumpTaggedValue(os);
    if (marker.IsHeapObject()) {
        ProtoChangeMarker::Cast(marker.GetTaggedObject())->Dump(os);
    } else {
        os << "\n";
    }

    os << " - ProtoChangeDetails :" << std::setw(DUMP_TYPE_OFFSET);
    JSTaggedValue details = jshclass->GetProtoChangeDetails();
    details.DumpTaggedValue(os);
    if (details.IsHeapObject()) {
        ProtoChangeDetails::Cast(details.GetTaggedObject())->Dump(os);
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

static void DumpClass(TaggedObject *obj, std::ostream &os)
{
    ASSERT(obj->GetClass()->GetObjectType() == JSType::HCLASS);
    DumpHClass(JSHClass::Cast(obj), os, true);
}

static void DumpObject(TaggedObject *obj, std::ostream &os, bool isPrivacy)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto jsHclass = obj->GetClass();
    JSType type = jsHclass->GetObjectType();

    bool needDumpHClass = false;
    switch (type) {
        case JSType::HCLASS:
            return DumpClass(obj, os);
        case JSType::TAGGED_ARRAY:
        case JSType::TAGGED_DICTIONARY:
        case JSType::TEMPLATE_MAP:
        case JSType::LEXICAL_ENV:
        case JSType::SFUNCTION_ENV:
        case JSType::SENDABLE_ENV:
        case JSType::COW_TAGGED_ARRAY:
        case JSType::AOT_LITERAL_INFO:
            DumpArrayClass(TaggedArray::Cast(obj), os);
            break;
        case JSType::MUTANT_TAGGED_ARRAY:
            DumpMutantTaggedArray(MutantTaggedArray::Cast(obj), os);
            break;
        case JSType::COW_MUTANT_TAGGED_ARRAY:
            DumpCOWMutantTaggedArray(COWMutantTaggedArray::Cast(obj), os);
            break;
        case JSType::CONSTANT_POOL:
            DumpConstantPoolClass(ConstantPool::Cast(obj), os);
            break;
        case JSType::PROFILE_TYPE_INFO:
            ProfileTypeInfo::Cast(obj)->Dump(os);
            break;
        case JSType::PROFILE_TYPE_INFO_CELL_0:
        case JSType::PROFILE_TYPE_INFO_CELL_1:
        case JSType::PROFILE_TYPE_INFO_CELL_N:
            ProfileTypeInfoCell::Cast(obj)->Dump(os);
            break;
        case JSType::FUNCTION_TEMPLATE:
            FunctionTemplate::Cast(obj)->Dump(os);
            break;
        case JSType::VTABLE:
            VTable::Cast(obj)->Dump(os);
            break;
        case JSType::EXTRA_PROFILE_TYPE_INFO:
            ExtraProfileTypeInfo::Cast(obj)->Dump(os);
            break;
        case JSType::LINE_STRING:
        case JSType::TREE_STRING:
        case JSType::SLICED_STRING:
            DumpStringClass(EcmaString::Cast(obj), os);
            os << "\n";
            break;
        case JSType::JS_NATIVE_POINTER:
            break;
        case JSType::JS_OBJECT:
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
            JSObject::Cast(obj)->Dump(os, isPrivacy);
            break;
        case JSType::JS_FUNCTION_BASE:
            needDumpHClass = true;
            JSFunctionBase::Cast(obj)->Dump(os);
            break;
        case JSType::GLOBAL_ENV:
            GlobalEnv::Cast(obj)->Dump(os);
            break;
        case JSType::ACCESSOR_DATA:
            break;
        case JSType::JS_SHARED_FUNCTION:
        case JSType::JS_FUNCTION:
            needDumpHClass = true;
            JSFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_BOUND_FUNCTION:
            needDumpHClass = true;
            JSBoundFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_SET:
            needDumpHClass = true;
            JSSet::Cast(obj)->Dump(os);
            break;
        case JSType::JS_SHARED_SET:
            needDumpHClass = true;
            JSSharedSet::Cast(obj)->Dump(os);
            break;
        case JSType::JS_MAP:
            needDumpHClass = true;
            JSMap::Cast(obj)->Dump(os);
            break;
        case JSType::JS_SHARED_MAP:
            needDumpHClass = true;
            JSSharedMap::Cast(obj)->Dump(os);
            break;
        case JSType::JS_WEAK_SET:
            needDumpHClass = true;
            JSWeakSet::Cast(obj)->Dump(os);
            break;
        case JSType::JS_WEAK_MAP:
            needDumpHClass = true;
            JSWeakMap::Cast(obj)->Dump(os);
            break;
        case JSType::JS_WEAK_REF:
            needDumpHClass = true;
            JSWeakRef::Cast(obj)->Dump(os);
            break;
        case JSType::JS_FINALIZATION_REGISTRY:
            JSFinalizationRegistry::Cast(obj)->Dump(os);
            break;
        case JSType::CELL_RECORD:
            CellRecord::Cast(obj)->Dump(os);
            break;
        case JSType::JS_REG_EXP:
            needDumpHClass = true;
            JSRegExp::Cast(obj)->Dump(os);
            break;
        case JSType::JS_DATE:
            needDumpHClass = true;
            JSDate::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ARRAY:
            needDumpHClass = true;
            JSArray::Cast(obj)->Dump(os);
            break;
        case JSType::JS_SHARED_ARRAY:
            needDumpHClass = true;
            JSSharedArray::Cast(obj)->Dump(os);
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
            JSTypedArray::Cast(obj)->Dump(os);
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
            JSSharedTypedArray::Cast(obj)->Dump(os);
            break;
        case JSType::BIGINT:
            BigInt::Cast(obj)->Dump(os);
            break;
        case JSType::BYTE_ARRAY:
            ByteArray::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PROXY:
            needDumpHClass = true;
            JSProxy::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PRIMITIVE_REF:
            JSPrimitiveRef::Cast(obj)->Dump(os);
            break;
        case JSType::SYMBOL:
            JSSymbol::Cast(obj)->Dump(os);
            break;
        case JSType::JS_DATA_VIEW:
            JSDataView::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ARRAY_BUFFER:
            JSArrayBuffer::Cast(obj)->Dump(os);
            break;
        case JSType::JS_SENDABLE_ARRAY_BUFFER:
            JSSendableArrayBuffer::Cast(obj)->Dump(os);
            break;
        case JSType::JS_SHARED_ARRAY_BUFFER:
            JSArrayBuffer::Cast(obj)->Dump(os);
            break;
        case JSType::PROMISE_REACTIONS:
            PromiseReaction::Cast(obj)->Dump(os);
            break;
        case JSType::PROMISE_CAPABILITY:
            PromiseCapability::Cast(obj)->Dump(os);
            break;
        case JSType::PROMISE_ITERATOR_RECORD:
            PromiseIteratorRecord::Cast(obj)->Dump(os);
            break;
        case JSType::PROMISE_RECORD:
            PromiseRecord::Cast(obj)->Dump(os);
            break;
        case JSType::RESOLVING_FUNCTIONS_RECORD:
            ResolvingFunctionsRecord::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PROMISE:
            needDumpHClass = true;
            JSPromise::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PROMISE_REACTIONS_FUNCTION:
            JSPromiseReactionsFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PROMISE_EXECUTOR_FUNCTION:
            JSPromiseExecutorFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION:
            JSAsyncModuleFulfilledFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION:
            JSAsyncModuleRejectedFunction::Cast(obj)->Dump(os);
            break;
        case JSType::ASYNC_GENERATOR_REQUEST:
            AsyncGeneratorRequest::Cast(obj)->Dump(os);
            break;
        case JSType::ASYNC_ITERATOR_RECORD:
            AsyncIteratorRecord::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ASYNC_FROM_SYNC_ITERATOR:
            JSAsyncFromSyncIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION:
            JSAsyncFromSyncIterUnwarpFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION:
            JSPromiseAllResolveElementFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION:
            JSPromiseAnyRejectElementFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION:
            JSPromiseAllSettledElementFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PROMISE_FINALLY_FUNCTION:
            JSPromiseFinallyFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION:
            JSPromiseValueThunkOrThrowerFunction::Cast(obj)->Dump(os);
            break;
        case JSType::MICRO_JOB_QUEUE:
            MicroJobQueue::Cast(obj)->Dump(os);
            break;
        case JSType::PENDING_JOB:
            PendingJob::Cast(obj)->Dump(os);
            break;
        case JSType::COMPLETION_RECORD:
            CompletionRecord::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PROXY_REVOC_FUNCTION:
            JSProxyRevocFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ASYNC_FUNCTION:
        case JSType::JS_SHARED_ASYNC_FUNCTION:
            needDumpHClass = true;
            JSAsyncFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION:
            JSAsyncAwaitStatusFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_GENERATOR_FUNCTION:
            JSGeneratorFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ASYNC_GENERATOR_FUNCTION:
            needDumpHClass = true;
            JSAsyncGeneratorFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN:
            JSAsyncGeneratorResNextRetProRstFtn::Cast(obj)->Dump(os);
            break;
        case JSType::JS_INTL_BOUND_FUNCTION:
            JSIntlBoundFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ITERATOR:
            break;
        case JSType::JS_ASYNCITERATOR:
            break;
        case JSType::JS_FORIN_ITERATOR:
            JSForInIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_MAP_ITERATOR:
            JSMapIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_SHARED_MAP_ITERATOR:
            JSSharedMapIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_SET_ITERATOR:
            JSSetIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_SHARED_SET_ITERATOR:
            JSSharedSetIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_REG_EXP_ITERATOR:
            JSRegExpIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ARRAY_ITERATOR:
            JSArrayIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_SHARED_ARRAY_ITERATOR:
            JSSharedArrayIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_STRING_ITERATOR:
            JSStringIterator::Cast(obj)->Dump(os);
            break;
        case JSType::PROTOTYPE_HANDLER:
            PrototypeHandler::Cast(obj)->Dump(os);
            break;
        case JSType::TRANSITION_HANDLER:
            TransitionHandler::Cast(obj)->Dump(os);
            break;
        case JSType::TRANS_WITH_PROTO_HANDLER:
            TransWithProtoHandler::Cast(obj)->Dump(os);
            break;
        case JSType::STORE_TS_HANDLER:
            StoreAOTHandler::Cast(obj)->Dump(os);
            break;
        case JSType::PROPERTY_BOX:
            PropertyBox::Cast(obj)->Dump(os);
            break;
        case JSType::JS_REALM:
            needDumpHClass = true;
            JSRealm::Cast(obj)->Dump(os);
            break;
#ifdef ARK_SUPPORT_INTL
        case JSType::JS_INTL:
            needDumpHClass = true;
            JSIntl::Cast(obj)->Dump(os);
            break;
        case JSType::JS_LOCALE:
            needDumpHClass = true;
            JSLocale::Cast(obj)->Dump(os);
            break;
        case JSType::JS_DATE_TIME_FORMAT:
            JSDateTimeFormat::Cast(obj)->Dump(os);
            break;
        case JSType::JS_RELATIVE_TIME_FORMAT:
            JSRelativeTimeFormat::Cast(obj)->Dump(os);
            break;
        case JSType::JS_NUMBER_FORMAT:
            JSNumberFormat::Cast(obj)->Dump(os);
            break;
        case JSType::JS_COLLATOR:
            needDumpHClass = true;
            JSCollator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PLURAL_RULES:
            JSPluralRules::Cast(obj)->Dump(os);
            break;
        case JSType::JS_DISPLAYNAMES:
            JSDisplayNames::Cast(obj)->Dump(os);
            break;
        case JSType::JS_SEGMENTER:
            JSSegmenter::Cast(obj)->Dump(os);
            break;
        case JSType::JS_SEGMENTS:
            JSSegments::Cast(obj)->Dump(os);
            break;
        case JSType::JS_SEGMENT_ITERATOR:
            JSSegmentIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_LIST_FORMAT:
            JSListFormat::Cast(obj)->Dump(os);
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
            JSGeneratorObject::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ASYNC_GENERATOR_OBJECT:
            needDumpHClass = true;
            JSAsyncGeneratorObject::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ASYNC_FUNC_OBJECT:
            needDumpHClass = true;
            JSAsyncFuncObject::Cast(obj)->Dump(os);
            break;
        case JSType::JS_GENERATOR_CONTEXT:
            GeneratorContext::Cast(obj)->Dump(os);
            break;
        case JSType::PROTOTYPE_INFO:
            ProtoChangeDetails::Cast(obj)->Dump(os);
            break;
        case JSType::TRACK_INFO:
            TrackInfo::Cast(obj)->Dump(os);
            break;
        case JSType::ENUM_CACHE:
            EnumCache::Cast(obj)->Dump(os);
            break;
        case JSType::PROTO_CHANGE_MARKER:
            ProtoChangeMarker::Cast(obj)->Dump(os);
            break;
        case JSType::MARKER_CELL:
            MarkerCell::Cast(obj)->Dump(os);
            break;
        case JSType::PROGRAM:
            Program::Cast(obj)->Dump(os);
            break;
        case JSType::MACHINE_CODE_OBJECT:
            MachineCode::Cast(obj)->Dump(os);
            break;
        case JSType::CLASS_INFO_EXTRACTOR:
            ClassInfoExtractor::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_ARRAY_LIST:
            JSAPIArrayList::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_ARRAYLIST_ITERATOR:
            JSAPIArrayListIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_BITVECTOR:
            JSAPIBitVector::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_BITVECTOR_ITERATOR:
            JSAPIBitVectorIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_FAST_BUFFER:
            JSAPIFastBuffer::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_MAP:
            JSAPILightWeightMap::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR:
            JSAPILightWeightMapIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_SET:
            JSAPILightWeightSet::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR:
            JSAPILightWeightSetIterator::Cast(obj)->Dump(os);
            break;
        case JSType::LINKED_NODE:
            LinkedNode::Cast(obj)->Dump(os);
            break;
        case JSType::RB_TREENODE:
            break;
        case JSType::JS_API_HASH_MAP:
            JSAPIHashMap::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_HASH_SET:
            JSAPIHashSet::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_HASHMAP_ITERATOR:
            JSAPIHashMapIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_HASHSET_ITERATOR:
            JSAPIHashSetIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_TREE_MAP:
            JSAPITreeMap::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_TREE_SET:
            JSAPITreeSet::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_TREEMAP_ITERATOR:
            JSAPITreeMapIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_TREESET_ITERATOR:
            JSAPITreeSetIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_VECTOR:
            JSAPIVector::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_VECTOR_ITERATOR:
            JSAPIVectorIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_QUEUE:
            JSAPIQueue::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_QUEUE_ITERATOR:
            JSAPIQueueIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_DEQUE:
            JSAPIDeque::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_DEQUE_ITERATOR:
            JSAPIDequeIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_STACK:
            JSAPIStack::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_STACK_ITERATOR:
            JSAPIStackIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_LIST:
            JSAPIList::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_LIST_ITERATOR:
            JSAPIListIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_LINKED_LIST:
            JSAPILinkedList::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_LINKED_LIST_ITERATOR:
            JSAPILinkedListIterator::Cast(obj)->Dump(os);
            break;
        case JSType::SOURCE_TEXT_MODULE_RECORD:
            SourceTextModule::Cast(obj)->Dump(os);
            break;
        case JSType::IMPORTENTRY_RECORD:
            ImportEntry::Cast(obj)->Dump(os);
            break;
        case JSType::LOCAL_EXPORTENTRY_RECORD:
            LocalExportEntry::Cast(obj)->Dump(os);
            break;
        case JSType::INDIRECT_EXPORTENTRY_RECORD:
            IndirectExportEntry::Cast(obj)->Dump(os);
            break;
        case JSType::STAR_EXPORTENTRY_RECORD:
            StarExportEntry::Cast(obj)->Dump(os);
            break;
        case JSType::RESOLVEDBINDING_RECORD:
            ResolvedBinding::Cast(obj)->Dump(os);
            break;
        case JSType::RESOLVEDINDEXBINDING_RECORD:
            ResolvedIndexBinding::Cast(obj)->Dump(os);
            break;
        case JSType::RESOLVEDRECORDINDEXBINDING_RECORD:
            ResolvedRecordIndexBinding::Cast(obj)->Dump(os);
            break;
        case JSType::RESOLVEDRECORDBINDING_RECORD:
            ResolvedRecordBinding::Cast(obj)->Dump(os);
            break;
        case JSType::NATIVE_MODULE_FAILURE_INFO:
            NativeModuleFailureInfo::Cast(obj)->Dump(os);
            break;
        case JSType::JS_MODULE_NAMESPACE:
            ModuleNamespace::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_PLAIN_ARRAY:
            JSAPIPlainArray::Cast(obj)->Dump(os);
            break;
        case JSType::JS_API_PLAIN_ARRAY_ITERATOR:
            JSAPIPlainArrayIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_CJS_MODULE:
            CjsModule::Cast(obj)->Dump(os);
            break;
        case JSType::JS_CJS_REQUIRE:
            CjsRequire::Cast(obj)->Dump(os);
            break;
        case JSType::JS_CJS_EXPORTS:
            CjsExports::Cast(obj)->Dump(os);
            break;
        case JSType::METHOD:
            Method::Cast(obj)->Dump(os);
            break;
        case JSType::CLASS_LITERAL:
            ClassLiteral::Cast(obj)->Dump(os);
            break;
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }

    if (needDumpHClass) {
        DumpHClass(jsHclass, os, false);
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

void JSTaggedValue::DumpHeapObjectType(std::ostream &os) const
{
    ASSERT(IsWeak() || IsHeapObject());
    bool isWeak = IsWeak();
    TaggedObject *obj = isWeak ? GetTaggedWeakRef() : GetTaggedObject();
    if (isWeak) {
        os << " [Weak Ref] ";
    }

    JSType type = obj->GetClass()->GetObjectType();
    if (type >= JSType::STRING_FIRST && type <= JSType::STRING_LAST) {
        CString string = ConvertToString(EcmaString::Cast(obj));
        os << std::left << std::setw(DUMP_TYPE_OFFSET) << "[" + string + "]";
    } else if (type == JSType::METHOD) {
        std::ostringstream address;
        address << obj;
        CString addrStr = CString(address.str());
        Method *method = Method::Cast(obj);

        os << std::left << std::setw(DUMP_TYPE_OFFSET)
                        << "[" + JSHClass::DumpJSType(type) + "(" + addrStr + "-" + method->GetMethodName() + ")]";
    } else {
        std::ostringstream address;
        address << obj;
        CString addrStr = CString(address.str());

        os << std::left << std::setw(DUMP_TYPE_OFFSET) << "[" + JSHClass::DumpJSType(type) + "(" + addrStr + ")]";
    }
}

void JSTaggedValue::DumpTaggedValue(std::ostream &os) const
{
    if (IsInt()) {
        os << std::left << std::setw(DUMP_TYPE_OFFSET) << "[Int] : " << std::hex << "0x" << GetInt() << std::dec << " ("
           << GetInt() << ")";
    } else if (IsDouble()) {
        os << std::left << std::setw(DUMP_TYPE_OFFSET) << "[Double] : " << GetDouble();
    } else if (IsSpecial()) {
        DumpSpecialValue(os);
    } else {
        DumpHeapObjectType(os);
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

void JSTaggedValue::Dump(std::ostream &os, bool isPrivacy) const
{
    DumpTaggedValue(os);
    os << "\n";

    if (IsHeapObject()) {
        TaggedObject *obj = IsWeak() ? GetTaggedWeakRef() : GetTaggedObject();
        DumpObject(obj, os, isPrivacy);
    }
}

void JSTaggedValue::DumpHeapObjAddress(std::ostream &os) const
{
    if (IsHeapObject()) {
        TaggedObject *obj = IsWeak() ? GetTaggedWeakRef() : GetTaggedObject();
        os << obj << "\n";
    } else {
        os << "not a heapobject\n";
    }
}

void JSTaggedValue::D() const
{
    Dump(std::cout);
}

void JSTaggedValue::DV(JSTaggedType val)
{
    JSTaggedValue(val).D();
}

void JSThread::DumpStack()
{
    FrameHandler handler(this);
    handler.DumpStack(std::cout);
}

void NumberDictionary::Dump(std::ostream &os, bool isPrivacy) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET)
               << static_cast<uint32_t>(JSTaggedNumber(key).GetNumber()) << ": ";
            if (!isPrivacy) {
                JSTaggedValue val(GetValue(hashIndex));
                val.DumpTaggedValue(os);
            }
            os << " ";
            DumpAttr(GetAttributes(hashIndex), false, os);
            os << "\n";
        }
    }
}

void NameDictionary::Dump(std::ostream &os, bool isPrivacy) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            DumpPropertyKey(key, os);
            os << ": ";
            if (!isPrivacy) {
                JSTaggedValue val(GetValue(hashIndex));
                val.DumpTaggedValue(os);
            }
            os << " ";
            DumpAttr(GetAttributes(hashIndex), false, os);
            os << "\n";
        }
    }
}

void GlobalDictionary::Dump(std::ostream &os, bool isPrivacy) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            DumpPropertyKey(key, os);
            os << " : ";
            if (!isPrivacy) {
                JSTaggedValue val(GetValue(hashIndex));
                val.DumpTaggedValue(os);
            }
            os << " ";
            DumpAttr(GetAttributes(hashIndex), false, os);
            os << "\n";
        }
    }
}

void LayoutInfo::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int num = NumberOfElements();
    for (int i = 0; i < num; i++) {
        JSTaggedValue key = GetKey(i);
        PropertyAttributes attr = GetAttr(i);
        os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
        os << "[" << i << "]: ";
        DumpPropertyKey(key, os);
        os << " : ";
        DumpAttr(attr, true, os);
        os << "\n";
    }
}

void TransitionsDictionary::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            DumpPropertyKey(key, os);
            os << " : ";
            GetValue(hashIndex).DumpTaggedValue(os);
            os << " : ";
            GetAttributes(hashIndex).DumpTaggedValue(os);
            os << "\n";
        }
    }
}

void LinkedHashSet::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfElements() + NumberOfDeletedElements();
    for (int hashIndex = 0; hashIndex < capacity; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            key.DumpTaggedValue(os);
            os << "\n";
        }
    }
}

void LinkedHashMap::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfElements() + NumberOfDeletedElements();
    for (int hashIndex = 0; hashIndex < capacity; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            JSTaggedValue val(GetValue(hashIndex));
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            key.DumpTaggedValue(os);
            os << ": ";
            val.DumpTaggedValue(os);
            os << "\n";
        }
    }
}

void TaggedDoubleList::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfNodes();
    os << " - node num: " << std::dec << capacity << "\n";
    os << " - delete node num: " << std::dec << NumberOfDeletedNodes() << "\n";
    os << "head-next: ";
    // 5 : 5 first element next ptr
    GetElement(5).Dump(os);
    os << "head-pre: ";
    // 6 : 6 first element per ptr
    GetElement(6).Dump(os);
    os << "\n";
    int i = 0;
    int next = GetElement(5).GetInt();
    while (capacity > i) {
        os << " value: ";
        GetElement(next).DumpTaggedValue(os);
        os << " next: ";
        // 1 : 1 current element next ptr offset
        GetElement(next + 1).Dump(os);
        os << " pre: ";
        // 2 : 2 current element pre ptr offset
        GetElement(next + 2).Dump(os);
        os << "\n";
        next = GetElement(next + 1).GetInt();
        i++;
    }
}

void TaggedSingleList::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfNodes();
    os << "head-next: ";
    // 5 : 5 first element next ptr
    GetElement(5).Dump(os);
    os << "\n";
    int i = 0;
    int next = GetElement(5).GetInt();
    while (capacity > i) {
        os << " value: ";
        GetElement(next).DumpTaggedValue(os);
        os << " next: ";
        // 1 : 1 current element next ptr offset
        GetElement(next + 1).Dump(os);
        os << "\n";
        next = GetElement(next + 1).GetInt();
        i++;
    }
}

void JSObject::Dump(std::ostream &os, bool isPrivacy) const
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHClass *jshclass = GetJSHClass();
    os << " - hclass: " << std::hex << jshclass << "\n";
    os << " - prototype: ";
    jshclass->GetPrototype().DumpTaggedValue(os);
    os << "\n";

    JSTaggedType hashField = Barriers::GetTaggedValue(this, HASH_OFFSET);
    JSTaggedValue value(hashField);
    os << " - hash: " << std::hex << hashField;
    value.Dump(os);

    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    os << " - elements: " << std::hex << elements;
    if (elements->GetLength() == 0) {
        os << " NONE\n";
    } else if (!elements->IsDictionaryMode()) {
        DumpArrayClass(elements, os);
    } else {
        NumberDictionary *dict = NumberDictionary::Cast(elements);
        os << " <NumberDictionary[" << std::dec << dict->EntriesCount() << "]>\n";
        dict->Dump(os, isPrivacy);
    }

    TaggedArray *properties = TaggedArray::Cast(GetProperties().GetTaggedObject());
    os << " - properties: " << std::hex << properties;
    if (IsJSGlobalObject()) {
        GlobalDictionary *dict = GlobalDictionary::Cast(properties);
        os << " <GlobalDictionary[" << std::dec << dict->EntriesCount() << "]>\n";
        dict->Dump(os, isPrivacy);
        return;
    }

    if (!properties->IsDictionaryMode()) {
        JSTaggedValue attrs = jshclass->GetLayout();
        if (attrs.IsNull()) {
            return;
        }

        LayoutInfo *layoutInfo = LayoutInfo::Cast(attrs.GetTaggedObject());
        int propNumber = static_cast<int>(jshclass->NumberOfProps());
        os << " <LayoutInfo[" << std::dec << propNumber << "]>\n";
        for (int i = 0; i < propNumber; i++) {
            JSTaggedValue key = layoutInfo->GetKey(i);
            PropertyAttributes attr = layoutInfo->GetAttr(i);
            ASSERT(i == static_cast<int>(attr.GetOffset()));
            os << "     " << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            DumpPropertyKey(key, os);
            os << ": (";
            JSTaggedValue val;
            if (attr.IsInlinedProps()) {
                val = GetPropertyInlinedPropsWithRep(i, attr);
            } else {
                val = properties->Get(i - static_cast<int>(jshclass->GetInlinedProperties()));
            }
            if (!isPrivacy) {
                val.DumpTaggedValue(os);
            }
            os << ") ";
            DumpAttr(attr, true, os);
            os << "\n";
        }
    } else {
        NameDictionary *dict = NameDictionary::Cast(properties);
        os << " <NameDictionary[" << std::dec << dict->EntriesCount() << "]>\n";
        dict->Dump(os, isPrivacy);
    }
}

void TaggedArray::Dump(std::ostream &os) const
{
    DumpArrayClass(this, os);
}

void AccessorData::Dump(std::ostream &os) const
{
    auto *hclass = GetClass();
    if (hclass->GetObjectType() == JSType::INTERNAL_ACCESSOR) {
        os << " - Getter: " << InternalAccessor::ConstCast(this)->GetGetter() << "\n";
        os << " - Setter: " << InternalAccessor::ConstCast(this)->GetSetter() << "\n";
        return;
    }

    os << " - Getter: ";
    GetGetter().DumpTaggedValue(os);
    os << "\n";

    os << " - Setter: ";
    GetSetter().DumpTaggedValue(os);
    os << "\n";
}

void Program::Dump(std::ostream &os) const
{
    os << " - MainFunction: ";
    GetMainFunction().Dump(os);
    os << "\n";
}

void LinkedNode::Dump(std::ostream &os) const
{
    os << " - Next: ";

    os << "\n";
}

void ConstantPool::Dump(std::ostream &os) const
{
    DumpArrayClass(this, os);
}

void ProfileTypeInfo::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = GetIcSlotLength();
    os << " <ProfileTypeInfo[" << std::dec << len << "]>\n";
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(Get(i));
        if (!val.IsHole()) {
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET) << i << ": ";
            val.DumpTaggedValue(os);
            os << "\n";
        }
    }
}

void ExtraProfileTypeInfo::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    os << " - Receiver: ";
    GetReceiverObject().Dump(os);
    os << " - Holder: ";
    GetHolderObject().Dump(os);
    os << "\n";
}

void ProfileTypeInfoCell::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    os << " - Value: ";
    GetValue().Dump(os);
    os << " - Handle: ";
    GetHandle().Dump(os);
    os << "\n";
}

void FunctionTemplate::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    os << " - Method: ";
    GetMethod().Dump(os);
    os << " - Module: ";
    GetModule().Dump(os);
    os << " - RawProfileTypeInfo: ";
    GetRawProfileTypeInfo().Dump(os);
    os << " - length : " << GetLength() << "\n";
}

void VTable::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t num = GetNumberOfTuples();

    for (uint32_t i = 0; i < num; i++) {
        os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
        os << "[" << i << "]: [ name :";
        JSTaggedValue name = GetTupleItem(i, VTable::TupleItem::NAME);
        DumpPropertyKey(name, os);
        os << ", type :" << (IsAccessor(i) ? "Accessor" : "Function");
        JSTaggedValue owner = GetTupleItem(i, VTable::TupleItem::OWNER);
        os << ", owner :";
        owner.DumpTaggedValue(os);
        JSTaggedValue offset = GetTupleItem(i, VTable::TupleItem::OFFSET);
        os << ", offset :" << offset.GetInt();
        os << " ] ";
        os << "\n";
    }
}

void JSFunction::Dump(std::ostream &os) const
{
    os << " - ProtoOrHClass: ";
    GetProtoOrHClass().Dump(os);
    os << "\n";
    os << " - LexicalEnv: ";
    if (GetLexicalEnv().IsLexicalEnv()) {
        GetLexicalEnv().Dump(os);
    } else {
        GetLexicalEnv().DumpTaggedValue(os); // reduce circular calls
    }
    os << "\n";
    os << " - RawProfileTypeInfo: ";
    GetRawProfileTypeInfo().Dump(os);
    os << "\n";
    os << " - HomeObject: ";
    GetHomeObject().Dump(os);
    os << "\n";
    os << " - FunctionExtraInfo: ";
    GetFunctionExtraInfo().Dump(os);
    os << "\n";
    os << " - Method: ";
    GetMethod().Dump(os);
    os << "\n";
    os << " - Module: ";
    GetModule().Dump(os);
    os << "\n";
    JSObject::Dump(os);
}

void JSHClass::Dump(std::ostream &os) const
{
    DumpHClass(this, os, true);
}

void JSBoundFunction::Dump(std::ostream &os) const
{
    os << " - BoundTarget: ";
    GetBoundTarget().DumpTaggedValue(os);
    os << "\n";

    os << " - BoundThis: ";
    GetBoundThis().DumpTaggedValue(os);
    os << "\n";

    os << " - BoundArguments: ";
    GetBoundArguments().DumpTaggedValue(os);
    os << "\n";

    JSObject::Dump(os);
}

void JSPrimitiveRef::Dump(std::ostream &os) const
{
    os << " - SubValue : ";
    GetValue().DumpTaggedValue(os);
    os << "\n";
    JSObject::Dump(os);
}

void BigInt::Dump(std::ostream &os) const
{
    os << " - length : " << GetLength() << "\n";
    os << " - Sign : " << GetSign() << "\n";
    os << " - value : " << ToStdString(DECIMAL) << "\n";
}

void JSDate::Dump(std::ostream &os) const
{
    os << " - time: " << GetTime().GetDouble() << "\n";
    os << " - localOffset: " << GetLocalOffset().GetDouble() << "\n";
    JSObject::Dump(os);
}

void JSMap::Dump(std::ostream &os) const
{
    LinkedHashMap *map = LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject());
    os << " - elements: " << std::dec << map->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << map->Capacity() << "\n";
    JSObject::Dump(os);

    os << " <NameDictionary[" << map->NumberOfElements() << "]>\n";
    map->Dump(os);
}

void JSSharedMap::Dump(std::ostream &os) const
{
    LinkedHashMap *map = LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject());
    os << " - modRecord: " << std::dec << GetModRecord() << "\n";
    os << " - elements: " << std::dec << map->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << map->Capacity() << "\n";
    JSObject::Dump(os);

    os << " <NameDictionary[" << map->NumberOfElements() << "]>\n";
    map->Dump(os);
}

void JSAPITreeMap::Dump(std::ostream &os) const
{
    TaggedTreeMap *map = TaggedTreeMap::Cast(GetTreeMap().GetTaggedObject());
    os << " - elements: " << std::dec << map->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << map->Capacity() << "\n";
    JSObject::Dump(os);

    os << " <TaggedTree[" << map->NumberOfElements() << "]>\n";
    map->Dump(os);
}

void JSAPITreeMap::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetTreeMap().IsInvalidValue())) {
        TaggedTreeMap *map = TaggedTreeMap::Cast(GetTreeMap().GetTaggedObject());
        vec.emplace_back("treemap", GetTreeMap());
        map->DumpForSnapshot(vec);
    }

    JSObject::DumpForSnapshot(vec);
}

void JSAPITreeMapIterator::Dump(std::ostream &os) const
{
    TaggedTreeMap *map =
        TaggedTreeMap::Cast(JSAPITreeMap::Cast(GetIteratedMap().GetTaggedObject())->GetTreeMap().GetTaggedObject());
    os << " - elements: " << std::dec << map->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << map->Capacity() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(os);

    os << " <TaggedTree[" << map->NumberOfElements() << "]>\n";
    map->Dump(os);
}

void JSAPITreeMapIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedMap().IsInvalidValue())) {
        TaggedTreeMap *map =
            TaggedTreeMap::Cast(JSAPITreeMap::Cast(GetIteratedMap().GetTaggedObject())->GetTreeMap().GetTaggedObject());
        vec.emplace_back("iteratedmap", GetIteratedMap());
        map->DumpForSnapshot(vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(vec);
}

template <typename T>
void DumpTaggedTreeEntry(T tree, std::ostream &os, int index, bool isMap = false)
{
    DISALLOW_GARBAGE_COLLECTION;
    JSTaggedValue parent(tree->GetParent(index));
    JSTaggedValue val(tree->GetValue(index));
    JSTaggedValue color(static_cast<int>(tree->GetColor(index)));
    JSTaggedValue left = tree->GetLeftChild(index);
    JSTaggedValue right = tree->GetRightChild(index);
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[entry] " << index << ": ";
    os << "\n";
    if (isMap) {
        os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "   [key]:    {";
        JSTaggedValue key(tree->GetKey(index));
        key.DumpTaggedValue(os);
        os << std::right << "};";
        os << "\n";
    }
    os << std::left << std::setw(DUMP_TYPE_OFFSET) << "   [value]:  {";
    val.DumpTaggedValue(os);
    os << std::right << "};";
    os << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "   [parent]: {";
    parent.DumpTaggedValue(os);
    os << std::right << "};";
    os << "\n";
    os << std::left << std::setw(DUMP_TYPE_OFFSET) << "   [color]:  {";
    color.DumpTaggedValue(os);
    os << std::right << "};";
    os << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "   [left]:   {";
    left.DumpTaggedValue(os);
    os << std::right << "}; ";
    os << std::left << std::setw(DUMP_TYPE_OFFSET) << "  [right]: {";
    right.DumpTaggedValue(os);
    os << std::right << "};";
    os << "\n";
}
void TaggedTreeMap::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[Elements]: {";
    JSTaggedValue node = TaggedArray::Get(0);
    node.DumpTaggedValue(os);
    os << std::right << "}" << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[Delete]:   {";
    node = TaggedArray::Get(1);
    node.DumpTaggedValue(os);
    os << std::right << "}" << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[Capacity]: {";
    node = TaggedArray::Get(2); // 2 means the three element
    node.DumpTaggedValue(os);
    os << std::right << "}" << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[RootNode]: {";
    node = TaggedArray::Get(3); // 3 means the three element
    node.DumpTaggedValue(os);
    os << std::right << "}" << "\n";

    uint32_t capacity = NumberOfElements() + NumberOfDeletedElements();
    for (uint32_t index = 0; index < capacity; index++) {
        if (GetKey(index).IsHole()) {
            os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[entry] " << index << ": ";
            GetKey(index).DumpTaggedValue(os);
            os << "\n";
        } else {
            DumpTaggedTreeEntry(const_cast<TaggedTreeMap *>(this), os, index, true);
        }
    }
}

void JSAPITreeSet::Dump(std::ostream &os) const
{
    TaggedTreeSet *set = TaggedTreeSet::Cast(GetTreeSet().GetTaggedObject());
    os << " - elements: " << std::dec << set->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << set->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << set->Capacity() << "\n";
    JSObject::Dump(os);

    os << " <TaggedTree[" << set->NumberOfElements() << "]>\n";
    set->Dump(os);
}

void JSAPITreeSet::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetTreeSet().IsInvalidValue())) {
        TaggedTreeSet *set = TaggedTreeSet::Cast(GetTreeSet().GetTaggedObject());
        vec.emplace_back("treeset", GetTreeSet());
        set->DumpForSnapshot(vec);
    }

    JSObject::DumpForSnapshot(vec);
}

void JSAPITreeSetIterator::Dump(std::ostream &os) const
{
    TaggedTreeSet *set =
        TaggedTreeSet::Cast(JSAPITreeSet::Cast(GetIteratedSet().GetTaggedObject())->GetTreeSet().GetTaggedObject());
    os << " - elements: " << std::dec << set->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << set->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << set->Capacity() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(os);

    os << " <TaggedTree[" << set->NumberOfElements() << "]>\n";
    set->Dump(os);
}

void JSAPITreeSetIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedSet().IsInvalidValue())) {
        TaggedTreeSet *set =
            TaggedTreeSet::Cast(JSAPITreeSet::Cast(GetIteratedSet().GetTaggedObject())->GetTreeSet().GetTaggedObject());
        vec.emplace_back("iteratedset", GetIteratedSet());
        set->DumpForSnapshot(vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(vec);
}

void TaggedTreeSet::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[Elements]: {";
    JSTaggedValue node = TaggedArray::Get(0);
    node.DumpTaggedValue(os);
    os << std::right << "}" << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[Delete]:   {";
    node = TaggedArray::Get(1);
    node.DumpTaggedValue(os);
    os << std::right << "}" << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[Capacity]: {";
    node = TaggedArray::Get(2); // 2 means the three element
    node.DumpTaggedValue(os);
    os << std::right << "}" << "\n";
    os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[RootNode]: {";
    node = TaggedArray::Get(3); // 3 means the three element
    node.DumpTaggedValue(os);
    os << std::right << "}" << "\n";

    uint32_t capacity = NumberOfElements() + NumberOfDeletedElements();
    for (uint32_t index = 0; index < capacity; index++) {
        if (GetKey(index).IsHole()) {
            os << std::left << std::setw(DUMP_ELEMENT_OFFSET) << "[entry] " << index << ": ";
            GetKey(index).DumpTaggedValue(os);
            os << "\n";
        } else {
            DumpTaggedTreeEntry(const_cast<TaggedTreeSet *>(this), os, index);
        }
    }
}

void JSAPIPlainArray::Dump(std::ostream &os) const
{
    TaggedArray *keys = TaggedArray::Cast(GetKeys().GetTaggedObject());
    TaggedArray *values = TaggedArray::Cast(GetValues().GetTaggedObject());
    uint32_t len = static_cast<uint32_t>(GetLength());
    for (uint32_t i = 0; i < len; i++) {
        os << " - keys: ";
        keys->Get(i).DumpTaggedValue(os);
        os << "\n";
        os << " - values: ";
        values->Get(i).DumpTaggedValue(os);
        os << "\n";
    }
    os << " - length: " << std::dec << len << "\n";
    JSObject::Dump(os);
}

void JSAPIPlainArrayIterator::Dump(std::ostream &os) const
{
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(GetIteratedPlainArray().GetTaggedObject());
    os << " - length: " << std::dec << array->GetSize() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(os);
}

void JSAPIPlainArrayIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedPlainArray().IsInvalidValue())) {
        JSAPIPlainArray *array = JSAPIPlainArray::Cast(GetIteratedPlainArray().GetTaggedObject());
        vec.emplace_back("iteratedplainarray", GetIteratedPlainArray());
        array->DumpForSnapshot(vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSForInIterator::Dump(std::ostream &os) const
{
    os << " - Object : ";
    GetObject().DumpTaggedValue(os);
    os << " - CachedHclass : ";
    GetCachedHClass().DumpTaggedValue(os);
    os << "\n";
    os << " - Keys : ";
    GetKeys().DumpTaggedValue(os);
    os << "\n";
    os << " - Index : " << GetIndex();
    os << "\n";
    os << " - Length : " << GetLength();
    os << "\n";
    JSObject::Dump(os);
}

void JSMapIterator::Dump(std::ostream &os) const
{
    LinkedHashMap *map = LinkedHashMap::Cast(GetIteratedMap().GetTaggedObject());
    os << " - elements: " << std::dec << map->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << map->Capacity() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(os);

    os << " <NameDictionary[" << map->NumberOfElements() << "]>\n";
    map->Dump(os);
}

void JSSharedMapIterator::Dump(std::ostream &os) const
{
    JSSharedMap *iteratedMap = JSSharedMap::Cast(GetIteratedMap().GetTaggedObject());
    LinkedHashMap *map = LinkedHashMap::Cast(iteratedMap->GetLinkedMap().GetTaggedObject());
    os << " - elements: " << std::dec << map->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << map->Capacity() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(os);

    os << " <NameDictionary[" << map->NumberOfElements() << "]>\n";
    map->Dump(os);
}

void JSSet::Dump(std::ostream &os) const
{
    LinkedHashSet *set = LinkedHashSet::Cast(GetLinkedSet().GetTaggedObject());
    os << " - elements: " << std::dec << set->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << set->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << set->Capacity() << "\n";
    JSObject::Dump(os);

    os << " <NameDictionary[" << set->NumberOfElements() << "]>\n";
    set->Dump(os);
}

void JSSharedSet::Dump(std::ostream &os) const
{
    LinkedHashSet *set = LinkedHashSet::Cast(GetLinkedSet().GetTaggedObject());
    os << " - modRecord: " << std::dec << GetModRecord() << "\n";
    os << " - elements: " << std::dec << set->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << set->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << set->Capacity() << "\n";
    JSObject::Dump(os);

    os << " <NameDictionary[" << set->NumberOfElements() << "]>\n";
    set->Dump(os);
}

void JSWeakMap::Dump(std::ostream &os) const
{
    LinkedHashMap *map = LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject());
    os << " - length: " << std::dec << GetSize() << "\n";
    os << " - elements: " << std::dec << map->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << map->Capacity() << "\n";
    JSObject::Dump(os);

    os << " <NameDictionary[" << map->NumberOfElements() << "]>\n";
    map->Dump(os);
}

void JSWeakSet::Dump(std::ostream &os) const
{
    LinkedHashSet *set = LinkedHashSet::Cast(GetLinkedSet().GetTaggedObject());
    os << " - size: " << std::dec << GetSize() << "\n";
    os << " - elements: " << std::dec << set->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << set->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << set->Capacity() << "\n";
    JSObject::Dump(os);

    os << " <NameDictionary[" << set->NumberOfElements() << "]>\n";
    set->Dump(os);
}

void JSWeakRef::Dump(std::ostream &os) const
{
    os << " - WeakObject : ";
    GetWeakObject().DumpTaggedValue(os);
    os << "\n";
    JSObject::Dump(os);
}

void JSFinalizationRegistry::Dump(std::ostream &os) const
{
    os << " - CleanupCallback : ";
    GetCleanupCallback().DumpTaggedValue(os);
    os << "\n";
    os << " - NoUnregister : ";
    GetNoUnregister().Dump(os);
    os << "\n";
    os << " - MaybeUnregister : ";
    LinkedHashMap *map = LinkedHashMap::Cast(GetMaybeUnregister().GetTaggedObject());
    os << "   -   elements: " << std::dec << map->NumberOfElements() << "\n";
    os << "   -   deleted-elements: " << std::dec << map->NumberOfDeletedElements() << "\n";
    os << "   -   capacity: " << std::dec << map->Capacity() << "\n";
    JSObject::Dump(os);
}

void CellRecord::Dump(std::ostream &os) const
{
    os << " - WeakRefTarget : ";
    GetFromWeakRefTarget().DumpTaggedValue(os);
    os << "\n";
    os << " - HeldValue : ";
    GetHeldValue().DumpTaggedValue(os);
    os << "\n";
}

void JSSetIterator::Dump(std::ostream &os) const
{
    LinkedHashSet *set = LinkedHashSet::Cast(GetIteratedSet().GetTaggedObject());
    os << " - elements: " << std::dec << set->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << set->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << set->Capacity() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(os);

    os << " <NameDictionary[" << set->NumberOfElements() << "]>\n";
    set->Dump(os);
}

void JSSharedSetIterator::Dump(std::ostream &os) const
{
    JSSharedSet *iteratedSet = JSSharedSet::Cast(GetIteratedSet().GetTaggedObject());
    LinkedHashSet *set = LinkedHashSet::Cast(iteratedSet->GetLinkedSet().GetTaggedObject());
    os << " - elements: " << std::dec << set->NumberOfElements() << "\n";
    os << " - deleted-elements: " << std::dec << set->NumberOfDeletedElements() << "\n";
    os << " - capacity: " << std::dec << set->Capacity() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(os);

    os << " <NameDictionary[" << set->NumberOfElements() << "]>\n";
    set->Dump(os);
}

void JSRegExpIterator::Dump(std::ostream &os) const
{
    os << " - IteratingRegExp: ";
    GetIteratingRegExp().Dump(os);
    os << "\n";
    os << " - IteratedString: ";
    GetIteratedString().Dump(os);
    os << "\n";
    os << " - Global: " << std::dec << GetGlobal() << "\n";
    os << " - Unicode: " << std::dec << GetUnicode() << "\n";
    os << " - Done: " << std::dec << GetDone() << "\n";
    JSObject::Dump(os);
}

void JSArray::Dump(std::ostream &os) const
{
    os << " - length: " << std::dec << GetArrayLength() << "\n";
    JSObject::Dump(os);
}

void JSSharedArray::Dump(std::ostream &os) const
{
    os << " - length: " << std::dec << GetArrayLength() << "\n";
    JSObject::Dump(os);
}

void JSArrayIterator::Dump(std::ostream &os) const
{
    JSArray *array = JSArray::Cast(GetIteratedArray().GetTaggedObject());
    os << " - length: " << std::dec << array->GetArrayLength() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(os);
}

void JSSharedArrayIterator::Dump(std::ostream &os) const
{
    JSSharedArray *array = JSSharedArray::Cast(GetIteratedArray().GetTaggedObject());
    os << " - length: " << std::dec << array->GetArrayLength() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(os);
}

void JSAPIArrayList::Dump(std::ostream &os) const
{
    os << " - length: " << std::dec << GetSize() << "\n";
    JSObject::Dump(os);
}

void JSAPIArrayListIterator::Dump(std::ostream &os) const
{
    JSAPIArrayList *arrayList = JSAPIArrayList::Cast(GetIteratedArrayList().GetTaggedObject());
    os << " - length: " << std::dec << arrayList->GetSize() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(os);
}

void JSAPIDeque::Dump(std::ostream &os) const
{
    os << " - first: " << std::dec << GetFirst() << "\n";
    os << " - last: " << std::dec << GetLast() << "\n";
    JSObject::Dump(os);
}

void JSAPIDequeIterator::Dump(std::ostream &os) const
{
    JSAPIDeque *deque = JSAPIDeque::Cast(GetIteratedDeque().GetTaggedObject());
    os << " - length: " << std::dec << deque->GetSize() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(os);
}

void JSAPILightWeightMap::Dump(std::ostream &os) const
{
    uint32_t capacity = GetSize();
    os << " - length: " << std::dec << capacity << "\n";
    uint32_t i = 0;
    TaggedArray *hashArray = TaggedArray::Cast(GetHashes().GetTaggedObject());
    TaggedArray *keyArray = TaggedArray::Cast(GetKeys().GetTaggedObject());
    TaggedArray *valueArray = TaggedArray::Cast(GetValues().GetTaggedObject());
    while (capacity > i) {
        os << " hash: ";
        hashArray->Get(i).DumpTaggedValue(os);
        os << " key: ";
        keyArray->Get(i).DumpTaggedValue(os);
        os << " value: ";
        valueArray->Get(i).DumpTaggedValue(os);
        os << "\n";
        i++;
    }
    JSObject::Dump(os);
}

void JSAPILightWeightMapIterator::Dump(std::ostream &os) const
{
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(os);
}

void JSAPIHashMap::Dump(std::ostream &os) const
{
    TaggedHashArray *hashArray = TaggedHashArray::Cast(GetTable().GetTaggedObject());
    os << " - elements: " << std::dec << GetSize() << "\n";
    os << " - table capacity: " << std::dec << static_cast<int>(hashArray->GetLength()) << "\n";
    JSObject::Dump(os);
}

void JSAPIHashMap::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetTable().IsInvalidValue())) {
        TaggedHashArray *map = TaggedHashArray::Cast(GetTable().GetTaggedObject());
        vec.emplace_back("hashmap", GetTable());
        map->DumpForSnapshot(vec);
    }

    JSObject::DumpForSnapshot(vec);
}

void JSAPIHashSet::Dump(std::ostream &os) const
{
    TaggedHashArray *hashArray = TaggedHashArray::Cast(GetTable().GetTaggedObject());
    os << " - elements: " << std::dec << GetSize() << "\n";
    os << " - table capacity: " << std::dec << static_cast<int>(hashArray->GetLength()) << "\n";
    JSObject::Dump(os);
}

void JSAPIHashSet::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetTable().IsInvalidValue())) {
        TaggedHashArray *set = TaggedHashArray::Cast(GetTable().GetTaggedObject());
        vec.emplace_back("hashset", GetTable());
        set->DumpForSnapshot(vec);
    }

    JSObject::DumpForSnapshot(vec);
}

void JSAPIHashMapIterator::Dump(std::ostream &os) const
{
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(os);
}

void JSAPIHashMapIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("IteratedHashMap"), JSTaggedValue(GetIteratedHashMap()));
    vec.emplace_back(CString("TaggedQueue"), JSTaggedValue(GetTaggedQueue()));
    vec.emplace_back(CString("CurrentNodeResult"), JSTaggedValue(GetCurrentNodeResult()));
    vec.emplace_back(CString("BitField"), JSTaggedValue(GetBitField()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIHashSetIterator::Dump(std::ostream &os) const
{
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(os);
}

void JSAPIHashSetIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPILightWeightSet::Dump(std::ostream &os) const
{
    TaggedArray *keys = TaggedArray::Cast(GetHashes().GetTaggedObject());
    TaggedArray *values = TaggedArray::Cast(GetValues().GetTaggedObject());
    uint32_t len = GetLength();
    for (uint32_t i = 0; i < len; i++) {
        os << " - keys: ";
        keys->Get(i).DumpTaggedValue(os);
        os << "\n";
        os << " - values: ";
        values->Get(i).DumpTaggedValue(os);
        os << "\n";
    }
    os << " - length: " << std::dec << len << "\n";
    JSObject::Dump(os);
}

void JSAPILightWeightSetIterator::Dump(std::ostream &os) const
{
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(os);
}

void JSAPIList::Dump(std::ostream &os) const
{
    TaggedSingleList *list = TaggedSingleList::Cast(GetSingleList().GetTaggedObject());
    os << " - length: " << std::dec << list->GetCapacityFromTaggedArray() << "\n";
    os << " - node num: " << std::dec << list->NumberOfNodes() << "\n";
    os << " - delete node num: " << std::dec << list->NumberOfDeletedNodes() << "\n";
    os << " - is odered list: " << std::dec << this->IsOrderedList() << "\n";
    JSObject::Dump(os);
    list->Dump(os);
}

void JSAPIList::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetSingleList().IsInvalidValue())) {
        TaggedSingleList *list = TaggedSingleList::Cast(GetSingleList().GetTaggedObject());
        vec.emplace_back("singleList", GetSingleList());
        list->DumpForSnapshot(vec);
    }
    JSObject::DumpForSnapshot(vec);
}

void JSAPIListIterator::Dump(std::ostream &os) const
{
    TaggedSingleList *list = TaggedSingleList::Cast(GetIteratedList().GetTaggedObject());
    os << " - length: " << std::dec << list->GetCapacityFromTaggedArray() << "\n";
    os << " - node num: " << std::dec << list->NumberOfNodes() << "\n";
    os << " - delete node num: " << std::dec << list->NumberOfDeletedNodes() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(os);
    list->Dump(os);
}

void JSAPIListIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedList().IsInvalidValue())) {
        TaggedSingleList *list = TaggedSingleList::Cast(GetIteratedList().GetTaggedObject());
        vec.emplace_back("iteratedlist", GetIteratedList());
        list->DumpForSnapshot(vec);
    }
    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPILinkedList::Dump(std::ostream &os) const
{
    TaggedDoubleList *linkedList = TaggedDoubleList::Cast(GetDoubleList().GetTaggedObject());
    os << " - length: " << std::dec << linkedList->GetCapacityFromTaggedArray() << "\n";
    os << " - node num: " << std::dec << linkedList->NumberOfNodes() << "\n";
    os << " - delete node num: " << std::dec << linkedList->NumberOfDeletedNodes() << "\n";
    JSObject::Dump(os);
    linkedList->Dump(os);
}

void JSAPILinkedList::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetDoubleList().IsInvalidValue())) {
        TaggedDoubleList *list = TaggedDoubleList::Cast(GetDoubleList().GetTaggedObject());
        vec.emplace_back("doubleList", GetDoubleList());
        list->DumpForSnapshot(vec);
    }

    JSObject::DumpForSnapshot(vec);
}

void JSAPILinkedListIterator::Dump(std::ostream &os) const
{
    TaggedDoubleList *linkedList = TaggedDoubleList::Cast(GetIteratedLinkedList().GetTaggedObject());
    os << " - length: " << std::dec << linkedList->GetCapacityFromTaggedArray() << "\n";
    os << " - node num: " << std::dec << linkedList->NumberOfNodes() << "\n";
    os << " - delete node num: " << std::dec << linkedList->NumberOfDeletedNodes() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(os);
    linkedList->Dump(os);
}

void JSAPILinkedListIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedLinkedList().IsInvalidValue())) {
        TaggedDoubleList *linkedList = TaggedDoubleList::Cast(GetIteratedLinkedList().GetTaggedObject());
        vec.emplace_back("iteratedlist", GetIteratedLinkedList());
        linkedList->DumpForSnapshot(vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIQueue::Dump(std::ostream &os) const
{
    os << " - length: " << std::dec << GetSize() << "\n";
    os << " - front: " << std::dec << GetFront() << "\n";
    os << " - tail: " << std::dec << GetTail() << "\n";
    JSObject::Dump(os);
}

void JSAPIQueueIterator::Dump(std::ostream &os) const
{
    JSAPIQueue *queue = JSAPIQueue::Cast(GetIteratedQueue().GetTaggedObject());
    os << " - length: " << std::dec << queue->GetSize() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(os);
}

void JSAPIStack::Dump(std::ostream &os) const
{
    os << " - top: " << std::dec << GetTop() << "\n";
    JSObject::Dump(os);
}

void JSAPIStackIterator::Dump(std::ostream &os) const
{
    JSAPIStack *stack = JSAPIStack::Cast(GetIteratedStack().GetTaggedObject());
    os << " - length: " << std::dec << stack->GetSize() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(os);
}

void JSAPIVector::Dump(std::ostream &os) const
{
    os << " - length: " << std::dec << GetSize() << "\n";
    JSObject::Dump(os);
}

void JSAPIVectorIterator::Dump(std::ostream &os) const
{
    JSAPIVector *vector = JSAPIVector::Cast(GetIteratedVector().GetTaggedObject());
    os << " - length: " << std::dec << vector->GetSize() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(os);
}

void JSAPIBitVector::Dump(std::ostream &os) const
{
    os << " - length: " << std::dec << GetSize() << "\n";
    JSObject::Dump(os);
}

void JSAPIBitVectorIterator::Dump(std::ostream &os) const
{
    JSAPIBitVector *bitVector = JSAPIBitVector::Cast(GetIteratedBitVector().GetTaggedObject());
    os << " - length: " << std::dec << bitVector->GetSize() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(os);
}

void JSAPIFastBuffer::Dump(std::ostream &os) const
{
    os << " - ByteLength: " << std::dec << GetSize() << "\n";
    os << " - Length: " << std::dec << GetLength() << "\n";
    auto array = JSTypedArray::Cast(GetFastBufferData().GetTaggedObject());
    array->Dump(os);
    JSObject::Dump(os);
}

void JSStringIterator::Dump(std::ostream &os) const
{
    EcmaString *str = EcmaString::Cast(GetIteratedString().GetTaggedObject());
    os << " - IteratedString: " << EcmaStringAccessor(str).ToCString() << "\n";
    os << " - StringIteratorNextIndex: " << std::dec << GetStringIteratorNextIndex() << "\n";
    JSObject::Dump(os);
}
void JSTypedArray::Dump(std::ostream &os) const
{
    os << " - viewed-array-buffer: ";
    GetViewedArrayBufferOrByteArray().Dump(os);
    os << " - typed-array-name: ";
    GetTypedArrayName().Dump(os);
    os << " - byte-length: " << GetByteLength();
    os << " - byte-offset: " << GetByteOffset();
    os << " - array-length: " << GetArrayLength();
    JSObject::Dump(os);
}

void JSSharedTypedArray::Dump(std::ostream &os) const
{
    os << " - viewed-array-buffer: ";
    GetViewedArrayBufferOrByteArray().Dump(os);
    os << " - typed-array-name: ";
    GetTypedArrayName().Dump(os);
    os << " - byte-length: " << GetByteLength();
    os << " - byte-offset: " << GetByteOffset();
    os << " - array-length: " << GetArrayLength();
    JSObject::Dump(os);
}

void ByteArray::Dump(std::ostream &os) const
{
    os << " - array-length: " << GetArrayLength();
    os << " - byte-length: " << GetByteLength();
}

void JSRegExp::Dump(std::ostream &os) const
{
    os << "\n";
    os << " - ByteCodeBuffer: ";
    GetByteCodeBuffer().Dump(os);
    os << "\n";
    os << " - OriginalSource: ";
    GetOriginalSource().Dump(os);
    os << "\n";
    os << " - OriginalFlags: ";
    GetOriginalFlags().Dump(os);
    os << "\n";
    os << " - GroupName: ";
    GetGroupName().Dump(os);
    os << "\n";
    os << " - Length: " << GetLength();
    os << "\n";
    JSObject::Dump(os);
}

void JSProxy::Dump(std::ostream &os) const
{
    os << " - Target: ";
    os << "\n";
    JSObject::Cast(GetTarget().GetTaggedObject())->Dump(os);
    os << " - Handler: ";
    os << "\n";
    JSObject::Cast(GetHandler().GetTaggedObject())->Dump(os);
    os << "\n";
}

void JSSymbol::Dump(std::ostream &os) const
{
    os << " - hash-field: " << GetHashField();
    os << "\n - flags: " << GetFlags();
    os << "\n - description: ";
    JSTaggedValue description = GetDescription();
    description.Dump(os);
}

void LexicalEnv::Dump(std::ostream &os) const
{
    DumpArrayClass(this, os);
}

void SFunctionEnv::Dump(std::ostream &os) const
{
    DumpArrayClass(this, os);
}

void SendableEnv::Dump(std::ostream &os) const
{
    DumpArrayClass(this, os);
}

void COWTaggedArray::Dump(std::ostream &os) const
{
    DumpArrayClass(this, os);
}

void MutantTaggedArray::Dump(std::ostream &os) const
{
    DumpMutantTaggedArray(this, os);
}

void COWMutantTaggedArray::Dump(std::ostream &os) const
{
    DumpCOWMutantTaggedArray(this, os);
}

// NOLINTNEXTLINE(readability-function-size)
void GlobalEnv::Dump(std::ostream &os) const
{
    auto globalConst = GetJSThread()->GlobalConstants();
    os << " - ObjectFunction: ";
    GetObjectFunction().GetTaggedValue().Dump(os);
    os << " - FunctionFunction: ";
    GetFunctionFunction().GetTaggedValue().Dump(os);
    os << " - NumberFunction: ";
    GetNumberFunction().GetTaggedValue().Dump(os);
    os << " - BigIntFunction: ";
    GetBigIntFunction().GetTaggedValue().Dump(os);
    os << " - DateFunction: ";
    GetDateFunction().GetTaggedValue().Dump(os);
    os << " - BooleanFunction: ";
    GetBooleanFunction().GetTaggedValue().Dump(os);
    os << " - ErrorFunction: ";
    GetErrorFunction().GetTaggedValue().Dump(os);
    os << " - ArrayFunction: ";
    GetArrayFunction().GetTaggedValue().Dump(os);
    os << " - TypedArrayFunction: ";
    GetTypedArrayFunction().GetTaggedValue().Dump(os);
    os << " - Int8ArrayFunction: ";
    GetInt8ArrayFunction().GetTaggedValue().Dump(os);
    os << " - Uint8ArrayFunction: ";
    GetUint8ArrayFunction().GetTaggedValue().Dump(os);
    os << " - Uint8ClampedArrayFunction: ";
    GetUint8ClampedArrayFunction().GetTaggedValue().Dump(os);
    os << " - Int16ArrayFunction: ";
    GetInt16ArrayFunction().GetTaggedValue().Dump(os);
    os << " - ArrayBufferFunction: ";
    GetArrayBufferFunction().GetTaggedValue().Dump(os);
    os << " - SharedArrayBufferFunction: ";
    GetSharedArrayBufferFunction().GetTaggedValue().Dump(os);
    os << " - SymbolFunction: ";
    GetSymbolFunction().GetTaggedValue().Dump(os);
    os << " - RangeErrorFunction: ";
    GetRangeErrorFunction().GetTaggedValue().Dump(os);
    os << " - ReferenceErrorFunction: ";
    GetReferenceErrorFunction().GetTaggedValue().Dump(os);
    os << " - TypeErrorFunction: ";
    GetTypeErrorFunction().GetTaggedValue().Dump(os);
    os << " - AggregateErrorFunction: ";
    GetAggregateErrorFunction().GetTaggedValue().Dump(os);
    os << " - URIErrorFunction: ";
    GetURIErrorFunction().GetTaggedValue().Dump(os);
    os << " - SyntaxErrorFunction: ";
    GetSyntaxErrorFunction().GetTaggedValue().Dump(os);
    os << " - EvalErrorFunction: ";
    GetEvalErrorFunction().GetTaggedValue().Dump(os);
    os << " - OOMErrorFunction: ";
    GetOOMErrorFunction().GetTaggedValue().Dump(os);
    os << " - TerminationErrorFunction: ";
    GetTerminationErrorFunction().GetTaggedValue().Dump(os);
    os << " - RegExpFunction: ";
    GetRegExpFunction().GetTaggedValue().Dump(os);
    os << " - BuiltinsSetFunction: ";
    GetBuiltinsSetFunction().GetTaggedValue().Dump(os);
    os << " - BuiltinsMapFunction: ";
    GetBuiltinsMapFunction().GetTaggedValue().Dump(os);
    os << " - BuiltinsWeakSetFunction: ";
    GetBuiltinsWeakSetFunction().GetTaggedValue().Dump(os);
    os << " - BuiltinsWeakMapFunction: ";
    GetBuiltinsWeakMapFunction().GetTaggedValue().Dump(os);
    os << " - BuiltinsWeakRefFunction: ";
    GetBuiltinsWeakRefFunction().GetTaggedValue().Dump(os);
    os << " - BuiltinsFinalizationRegistryFunction: ";
    GetBuiltinsFinalizationRegistryFunction().GetTaggedValue().Dump(os);
    os << " - MathFunction: ";
    GetMathFunction().GetTaggedValue().Dump(os);
    os << " - AtomicsFunction: ";
    GetAtomicsFunction().GetTaggedValue().Dump(os);
    os << " - JsonFunction: ";
    GetJsonFunction().GetTaggedValue().Dump(os);
    os << " - StringFunction: ";
    GetStringFunction().GetTaggedValue().Dump(os);
    os << " - ProxyFunction: ";
    GetProxyFunction().GetTaggedValue().Dump(os);
    os << " - ReflectFunction: ";
    GetReflectFunction().GetTaggedValue().Dump(os);
    os << " - AsyncFunction: ";
    GetAsyncFunction().GetTaggedValue().Dump(os);
    os << " - AsyncFunctionPrototype: ";
    GetAsyncFunctionPrototype().GetTaggedValue().Dump(os);
    os << " - JSGlobalObject: ";
    GetJSGlobalObject().GetTaggedValue().Dump(os);
    os << " - GlobalPatch: ";
    GetGlobalPatch().GetTaggedValue().Dump(os);
    os << " - EmptyArray: ";
    globalConst->GetEmptyArray().Dump(os);
    os << " - EmptyString ";
    globalConst->GetEmptyString().Dump(os);
    os << " - EmptyTaggedQueue: ";
    globalConst->GetEmptyTaggedQueue().Dump(os);
    os << " - EmptyProfileTypeInfoCell: ";
    globalConst->GetEmptyProfileTypeInfoCell().Dump(os);
    os << " - PrototypeString: ";
    globalConst->GetPrototypeString().Dump(os);
    os << " - HasInstanceSymbol: ";
    GetHasInstanceSymbol().GetTaggedValue().Dump(os);
    os << " - IsConcatSpreadableSymbol: ";
    GetIsConcatSpreadableSymbol().GetTaggedValue().Dump(os);
    os << " - ToStringTagSymbol: ";
    GetToStringTagSymbol().GetTaggedValue().Dump(os);
    os << " - IteratorSymbol: ";
    GetIteratorSymbol().GetTaggedValue().Dump(os);
    os << " - AsyncIteratorSymbol: ";
    GetAsyncIteratorSymbol().GetTaggedValue().Dump(os);
    os << " - MatchSymbol: ";
    GetMatchSymbol().GetTaggedValue().Dump(os);
    os << " - MatchAllSymbol: ";
    GetMatchAllSymbol().GetTaggedValue().Dump(os);
    os << " - ReplaceSymbol: ";
    GetReplaceSymbol().GetTaggedValue().Dump(os);
    os << " - SearchSymbol: ";
    GetSearchSymbol().GetTaggedValue().Dump(os);
    os << " - SpeciesSymbol: ";
    GetSpeciesSymbol().GetTaggedValue().Dump(os);
    os << " - SplitSymbol: ";
    GetSplitSymbol().GetTaggedValue().Dump(os);
    os << " - ToPrimitiveSymbol: ";
    GetToPrimitiveSymbol().GetTaggedValue().Dump(os);
    os << " - UnscopablesSymbol: ";
    GetUnscopablesSymbol().GetTaggedValue().Dump(os);
    os << " - HoleySymbol: ";
    GetHoleySymbol().GetTaggedValue().Dump(os);
    os << " - NativeBindingSymbol: ";
    GetNativeBindingSymbol().GetTaggedValue().Dump(os);
    os << " - ConstructorString: ";
    globalConst->GetConstructorString().Dump(os);
    os << " - IteratorPrototype: ";
    GetIteratorPrototype().GetTaggedValue().Dump(os);
    os << " - ForinIteratorPrototype: ";
    GetForinIteratorPrototype().GetTaggedValue().Dump(os);
    os << " - StringIterator: ";
    GetStringIterator().GetTaggedValue().Dump(os);
    os << " - MapIteratorPrototype: ";
    GetMapIteratorPrototype().GetTaggedValue().Dump(os);
    os << " - SetIteratorPrototype: ";
    GetSetIteratorPrototype().GetTaggedValue().Dump(os);
    os << " - RegExpIteratorPrototype: ";
    GetRegExpIteratorPrototype().GetTaggedValue().Dump(os);
    os << " - ArrayIteratorPrototype: ";
    GetArrayIteratorPrototype().GetTaggedValue().Dump(os);
    os << " - StringIteratorPrototype: ";
    GetStringIteratorPrototype().GetTaggedValue().Dump(os);
    os << " - LengthString: ";
    globalConst->GetLengthString().Dump(os);
    os << " - ValueString: ";
    globalConst->GetValueString().Dump(os);
    os << " - WritableString: ";
    globalConst->GetWritableString().Dump(os);
    os << " - GetString: ";
    globalConst->GetGetString().Dump(os);
    os << " - SetString: ";
    globalConst->GetSetString().Dump(os);
    os << " - EnumerableString: ";
    globalConst->GetEnumerableString().Dump(os);
    os << " - ConfigurableString: ";
    globalConst->GetConfigurableString().Dump(os);
    os << " - NameString: ";
    globalConst->GetNameString().Dump(os);
    os << " - ValueOfString: ";
    globalConst->GetValueOfString().Dump(os);
    os << " - ToStringString: ";
    globalConst->GetToStringString().Dump(os);
    os << " - ToLocaleStringString: ";
    globalConst->GetToLocaleStringString().Dump(os);
    os << " - UndefinedString: ";
    globalConst->GetUndefinedString().Dump(os);
    os << " - NullString: ";
    globalConst->GetNullString().Dump(os);
    os << " - TrueString: ";
    globalConst->GetTrueString().Dump(os);
    os << " - FalseString: ";
    globalConst->GetFalseString().Dump(os);
    os << " - RegisterSymbols: ";
    GetRegisterSymbols().GetTaggedValue().Dump(os);
    os << " - ThrowTypeError: ";
    GetThrowTypeError().GetTaggedValue().Dump(os);
    os << " - GetPrototypeOfString: ";
    globalConst->GetGetPrototypeOfString().Dump(os);
    os << " - SetPrototypeOfString: ";
    globalConst->GetSetPrototypeOfString().Dump(os);
    os << " - IsExtensibleString: ";
    globalConst->GetIsExtensibleString().Dump(os);
    os << " - PreventExtensionsString: ";
    globalConst->GetPreventExtensionsString().Dump(os);
    os << " - GetOwnPropertyDescriptorString: ";
    globalConst->GetGetOwnPropertyDescriptorString().Dump(os);
    os << " - DefinePropertyString: ";
    globalConst->GetDefinePropertyString().Dump(os);
    os << " - HasString: ";
    globalConst->GetHasString().Dump(os);
    os << " - DeletePropertyString: ";
    globalConst->GetDeletePropertyString().Dump(os);
    os << " - EnumerateString: ";
    globalConst->GetEnumerateString().Dump(os);
    os << " - OwnKeysString: ";
    globalConst->GetOwnKeysString().Dump(os);
    os << " - ApplyString: ";
    globalConst->GetApplyString().Dump(os);
    os << " - ProxyString: ";
    globalConst->GetProxyString().Dump(os);
    os << " - RevokeString: ";
    globalConst->GetRevokeString().Dump(os);
    os << " - ProxyConstructString: ";
    globalConst->GetProxyConstructString().Dump(os);
    os << " - ProxyCallString: ";
    globalConst->GetProxyCallString().Dump(os);
    os << " - DoneString: ";
    globalConst->GetDoneString().Dump(os);
    os << " - NegativeZeroString: ";
    globalConst->GetNegativeZeroString().Dump(os);
    os << " - NextString: ";
    globalConst->GetNextString().Dump(os);
    os << " - PromiseThenString: ";
    globalConst->GetPromiseThenString().Dump(os);
    os << " - PromiseFunction: ";
    GetPromiseFunction().GetTaggedValue().Dump(os);
    os << " - PromiseReactionJob: ";
    GetPromiseReactionJob().GetTaggedValue().Dump(os);
    os << " - PromiseResolveThenableJob: ";
    GetPromiseResolveThenableJob().GetTaggedValue().Dump(os);
    os << " - DynamicImportJob: ";
    GetDynamicImportJob().GetTaggedValue().Dump(os);
    os << " - ScriptJobString: ";
    globalConst->GetScriptJobString().Dump(os);
    os << " - PromiseString: ";
    globalConst->GetPromiseString().Dump(os);
    os << " - IdentityString: ";
    globalConst->GetIdentityString().Dump(os);
    os << " - AsyncFunctionString: ";
    globalConst->GetAsyncFunctionString().Dump(os);
    os << " - ThrowerString: ";
    globalConst->GetThrowerString().Dump(os);
    os << " - Undefined: ";
    globalConst->GetUndefined().Dump(os);
}

void JSDataView::Dump(std::ostream &os) const
{
    os << " - data-view: ";
    GetDataView().Dump(os);
    os << " - buffer: ";
    GetViewedArrayBuffer().Dump(os);
    os << "- byte-length: " << GetByteLength();
    os << "\n - byte-offset: " << GetByteOffset();
}

void JSArrayBuffer::Dump(std::ostream &os) const
{
    os << " - byte-length: " << GetArrayBufferByteLength();
    os << " - buffer-data: ";
    GetArrayBufferData().Dump(os);
    os << " - Shared: " << GetShared();
}

void JSSendableArrayBuffer::Dump(std::ostream &os) const
{
    os << " - byte-length: " << GetArrayBufferByteLength();
    os << " - buffer-data: ";
    GetArrayBufferData().Dump(os);
    os << " - Shared: " << GetShared();
}

void PromiseReaction::Dump(std::ostream &os) const
{
    os << " - promise-capability: ";
    GetPromiseCapability().Dump(os);
    os << " - type: " << static_cast<int>(GetType());
    os << " - handler: ";
    GetHandler().Dump(os);
}

void PromiseCapability::Dump(std::ostream &os) const
{
    os << " - promise: ";
    GetPromise().Dump(os);
    os << " - resolve: ";
    GetResolve().Dump(os);
    os << " - reject: ";
    GetReject().Dump(os);
}

void PromiseIteratorRecord::Dump(std::ostream &os) const
{
    os << " - iterator: ";
    GetIterator().Dump(os);
    os << " - done: " << GetDone();
}

void PromiseRecord::Dump(std::ostream &os) const
{
    os << " - value: ";
    GetValue().Dump(os);
}

void ResolvingFunctionsRecord::Dump(std::ostream &os) const
{
    os << " - resolve-function: ";
    GetResolveFunction().Dump(os);
    os << " - reject-function: ";
    GetRejectFunction().Dump(os);
}

void AsyncGeneratorRequest::Dump(std::ostream &os) const
{
    os << " - completion: ";
    GetCompletion().Dump(os);
    os << " - capability: ";
    GetCapability().Dump(os);
}

void AsyncIteratorRecord::Dump(std::ostream &os) const
{
    os << " - completion: ";
    GetIterator().Dump(os);
    os << " - nextmethod: ";
    GetNextMethod().Dump(os);
    os << " - done: " << GetDone();
}

void JSAsyncFromSyncIterator::Dump(std::ostream &os) const
{
    os << " - sync-iterator-record: ";
    GetSyncIteratorRecord().Dump(os);
    JSObject::Dump(os);
}

void JSAsyncFromSyncIterUnwarpFunction::Dump(std::ostream &os) const
{
    os << " - done: " ;
    GetDone().Dump(os);
    JSObject::Dump(os);
}

void JSPromise::Dump(std::ostream &os) const
{
    os << " - promise-state: " << static_cast<int>(GetPromiseState());
    os << "\n - promise-result: ";
    GetPromiseResult().Dump(os);
    os << " - promise-fulfill-reactions: ";
    GetPromiseFulfillReactions().Dump(os);
    os << " - promise-reject-reactions: ";
    GetPromiseRejectReactions().Dump(os);
    os << " - promise-is-handled: " << GetPromiseIsHandled();
    JSObject::Dump(os);
}

void JSPromiseReactionsFunction::Dump(std::ostream &os) const
{
    os << " - promise: ";
    GetPromise().Dump(os);
    os << " - already-resolved: ";
    GetAlreadyResolved().Dump(os);
    JSObject::Dump(os);
}

void JSPromiseExecutorFunction::Dump(std::ostream &os) const
{
    os << " - capability: ";
    GetCapability().Dump(os);
    JSObject::Dump(os);
}

void JSAsyncModuleFulfilledFunction::Dump(std::ostream &os) const
{
    os << " - module: ";
    GetModule().Dump(os);
    JSObject::Dump(os);
}

void JSAsyncModuleRejectedFunction::Dump(std::ostream &os) const
{
    os << " - module: ";
    GetModule().Dump(os);
    JSObject::Dump(os);
}

void JSPromiseAllResolveElementFunction::Dump(std::ostream &os) const
{
    os << " - index: ";
    GetIndex().Dump(os);
    os << " - values: ";
    GetValues().Dump(os);
    os << " - capability: ";
    GetCapabilities().Dump(os);
    os << " - remaining-elements: ";
    GetRemainingElements().Dump(os);
    os << " - already-called: ";
    GetAlreadyCalled().Dump(os);
    JSObject::Dump(os);
}

void JSPromiseAnyRejectElementFunction::Dump(std::ostream &os) const
{
    os << " - index: ";
    JSTaggedValue(GetIndex()).Dump(os);
    os << " - errors: ";
    GetErrors().Dump(os);
    os << " - capability: ";
    GetCapability().Dump(os);
    os << " - remaining-elements: ";
    GetRemainingElements().Dump(os);
    os << " - already-called: ";
    GetAlreadyCalled().Dump(os);
    JSObject::Dump(os);
}

void JSPromiseAllSettledElementFunction::Dump(std::ostream &os) const
{
    os << " - already-called: ";
    GetAlreadyCalled().Dump(os);
    os << " - index: ";
    JSTaggedValue(GetIndex()).Dump(os);
    os << " - values: ";
    GetValues().Dump(os);
    os << " - capability: ";
    GetCapability().Dump(os);
    os << " - remaining-elements: ";
    GetRemainingElements().Dump(os);
    JSObject::Dump(os);
}

void JSPromiseFinallyFunction::Dump(std::ostream &os) const
{
    os << " - constructor: ";
    GetConstructor().Dump(os);
    os << " - onFinally: ";
    GetOnFinally().Dump(os);
    JSObject::Dump(os);
}

void JSPromiseValueThunkOrThrowerFunction::Dump(std::ostream &os) const
{
    os << " - result: ";
    GetResult().Dump(os);
    JSObject::Dump(os);
}

void JSAsyncGeneratorResNextRetProRstFtn::Dump(std::ostream &os) const
{
    os << " - AsyncGeneratorObject";
    GetAsyncGeneratorObject().Dump(os);
    JSObject::Dump(os);
}

void MicroJobQueue::Dump(std::ostream &os) const
{
    os << " - promise-job-queue: ";
    GetPromiseJobQueue().Dump(os);
    os << " - script-job-queue: ";
    GetScriptJobQueue().Dump(os);
}

void PendingJob::Dump(std::ostream &os) const
{
    os << " - job: ";
    GetJob().Dump(os);
    os << "\n";
    os << " - arguments: ";
    GetArguments().Dump(os);
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

void CompletionRecord::Dump(std::ostream &os) const
{
    os << " - type: " << static_cast<int>(GetType());
    os << " - value: ";
    GetValue().Dump(os);
}

void JSProxyRevocFunction::Dump(std::ostream &os) const
{
    os << " - RevocableProxy: ";
    os << "\n";
    GetRevocableProxy().Dump(os);
    os << "\n";
}

void JSAsyncFunction::Dump(std::ostream &os) const
{
    JSFunction::Dump(os);
}

void JSAsyncAwaitStatusFunction::Dump(std::ostream &os) const
{
    os << " - AsyncContext: ";
    os << "\n";
    GetAsyncContext().Dump(os);
    os << "\n";
}

void JSGeneratorFunction::Dump(std::ostream &os) const
{
    JSFunction::Dump(os);
}

void JSAsyncGeneratorFunction::Dump(std::ostream &os) const
{
    JSFunction::Dump(os);
}

void JSIntlBoundFunction::Dump(std::ostream &os) const
{
    JSObject::Dump(os);
}

void PropertyBox::Dump(std::ostream &os) const
{
    os << " - Value: ";
    GetValue().Dump(os);
    os << "\n";
}

void PrototypeHandler::Dump(std::ostream &os) const
{
    os << " - HandlerInfo: ";
    GetHandlerInfo().Dump(os);
    os << "\n";
    os << " - ProtoCell: ";
    GetProtoCell().Dump(os);
    os << "\n";
    os << " - Holder: ";
    GetHolder().Dump(os);
    os << "\n";
}

void TransitionHandler::Dump(std::ostream &os) const
{
    os << " - HandlerInfo: ";
    GetHandlerInfo().Dump(os);
    os << "\n";
    os << " - TransitionHClass: ";
    GetTransitionHClass().Dump(os);
    os << "\n";
}

void TransWithProtoHandler::Dump(std::ostream &os) const
{
    os << " - HandlerInfo: ";
    GetHandlerInfo().Dump(os);
    os << "\n";
    os << " - TransitionHClass: ";
    GetTransitionHClass().Dump(os);
    os << "\n";
    os << " - ProtoCell: ";
    GetProtoCell().Dump(os);
    os << "\n";
}

void StoreAOTHandler::Dump(std::ostream &os) const
{
    os << " - HandlerInfo: ";
    GetHandlerInfo().Dump(os);
    os << "\n";
    os << " - ProtoCell: ";
    GetProtoCell().Dump(os);
    os << "\n";
    os << " - Holder: ";
    GetHolder().Dump(os);
    os << "\n";
}

void JSRealm::Dump(std::ostream &os) const
{
    os << " - Value: ";
    GetValue().Dump(os);
    os << "\n";
    os << " - GlobalEnv: ";
    GetGlobalEnv().Dump(os);
    os << "\n";
    JSObject::Dump(os);
}
#ifdef ARK_SUPPORT_INTL
void JSIntl::Dump(std::ostream &os) const
{
    os << " - FallbackSymbol: ";
    GetFallbackSymbol().Dump(os);
    os << "\n";
    JSObject::Dump(os);
}

void JSLocale::Dump(std::ostream &os) const
{
    os << " - IcuField: ";
    GetIcuField().Dump(os);
    os << "\n";
    JSObject::Dump(os);
}

void JSDateTimeFormat::Dump(std::ostream &os) const
{
    os << " - Locale: ";
    GetLocale().Dump(os);
    os << "\n";
    os << " - Calendar: ";
    GetCalendar().Dump(os);
    os << "\n";
    os << " - NumberingSystem: ";
    GetNumberingSystem().Dump(os);
    os << "\n";
    os << " - TimeZone: ";
    GetTimeZone().Dump(os);
    os << "\n";
    os << " - HourCycle: " << static_cast<int>(GetHourCycle());
    os << "\n";
    os << " - LocaleIcu: ";
    GetLocaleIcu().Dump(os);
    os << "\n";
    os << " - SimpleDateTimeFormatIcu: ";
    GetSimpleDateTimeFormatIcu().Dump(os);
    os << "\n";
    os << " - Iso8601: ";
    GetIso8601().Dump(os);
    os << "\n";
    os << " - DateStyle: " << static_cast<int>(GetDateStyle());
    os << "\n";
    os << " - TimeStyle: " << static_cast<int>(GetTimeStyle());
    os << "\n";
    os << " - BoundFormat: ";
    GetBoundFormat().Dump(os);
    os << "\n";
    JSObject::Dump(os);
}

void JSRelativeTimeFormat::Dump(std::ostream &os) const
{
    os << " - Locale: ";
    GetLocale().Dump(os);
    os << "\n";
    os << " - NumberingSystem: ";
    GetNumberingSystem().Dump(os);
    os << "\n";
    os << " - Style: " << static_cast<int>(GetStyle());
    os << "\n";
    os << " - Numeric: " << static_cast<int>(GetNumeric());
    os << "\n";
    os << " - IcuField: ";
    GetIcuField().Dump(os);
    os << "\n";
    JSObject::Dump(os);
}

void JSNumberFormat::Dump(std::ostream &os) const
{
    os << " - Locale: ";
    GetLocale().Dump(os);
    os << "\n" << " - NumberingSystem: ";
    GetNumberingSystem().Dump(os);
    os << "\n" << " - Style: " << static_cast<int>(GetStyle());
    os << "\n" << " - Currency: ";
    GetCurrency().Dump(os);
    os << "\n" << " - CurrencyDisplay: " << static_cast<int>(GetCurrencyDisplay());
    os << "\n" << " - CurrencySign: " << static_cast<int>(GetCurrencySign());
    os << "\n" << " - Unit: ";
    GetUnit().Dump(os);
    os << "\n" << " - UnitDisplay: " << static_cast<int>(GetUnitDisplay());
    os << "\n" << " - MinimumIntegerDigits: ";
    GetMinimumIntegerDigits().Dump(os);
    os << "\n" << " - MinimumFractionDigits: ";
    GetMinimumFractionDigits().Dump(os);
    os << "\n" << " - MaximumFractionDigits: ";
    GetMaximumFractionDigits().Dump(os);
    os << "\n" << " - MinimumSignificantDigits: ";
    GetMinimumSignificantDigits().Dump(os);
    os << "\n" << " - MaximumSignificantDigits: ";
    GetMaximumSignificantDigits().Dump(os);
    os << "\n" << " - UseGrouping: ";
    GetUseGrouping().Dump(os);
    os << "\n" << " - RoundingType: " << static_cast<int>(GetRoundingType());
    os << "\n" << " - Notation: " << static_cast<int>(GetNotation());
    os << "\n" << " - CompactDisplay: " << static_cast<int>(GetCompactDisplay());
    os << "\n" << " - SignDisplay: " << static_cast<int>(GetSignDisplay());
    os << "\n" << " - BoundFormat: ";
    GetBoundFormat().Dump(os);
    os << "\n" << " - IcuField: ";
    GetIcuField().Dump(os);
    os << "\n";
    JSObject::Dump(os);
}

void JSCollator::Dump(std::ostream &os) const
{
    os << " - IcuField: ";
    GetIcuField().Dump(os);
    os << "\n - Locale: ";
    GetLocale().Dump(os);
    os << "\n - Usage: " << static_cast<int>(GetUsage());
    os << "\n - Sensitivity: " << static_cast<int>(GetSensitivity());
    os << "\n - IgnorePunctuation: " << GetIgnorePunctuation();
    os << "\n - Collation: ";
    GetCollation().Dump(os);
    os << "\n - Numeric: " << GetNumeric();
    os << "\n - CaseFirst: " << static_cast<int>(GetCaseFirst());
    os << "\n - BoundCompare: ";
    GetBoundCompare().Dump(os);
    os << "\n";
    JSObject::Dump(os);
}

void JSPluralRules::Dump(std::ostream &os) const
{
    os << " - Locale: ";
    GetLocale().Dump(os);
    os << "\n";
    os << " - Type: " << static_cast<int>(GetType());
    os << "\n";
    os << " - MinimumIntegerDigits: ";
    GetMinimumIntegerDigits().Dump(os);
    os << "\n";
    os << " - MinimumFractionDigits: ";
    GetMinimumFractionDigits().Dump(os);
    os << "\n";
    os << " - MaximumFractionDigits: ";
    GetMaximumFractionDigits().Dump(os);
    os << "\n";
    os << " - MinimumSignificantDigits: ";
    GetMinimumSignificantDigits().Dump(os);
    os << "\n";
    os << " - MaximumSignificantDigits: ";
    GetMaximumSignificantDigits().Dump(os);
    os << "\n";
    os << " - RoundingType: " << static_cast<int>(GetRoundingType());
    os << "\n";
    os << " - IcuPR: ";
    GetIcuPR().Dump(os);
    os << "\n";
    os << " - IcuNF: ";
    GetIcuNF().Dump(os);
    os << "\n";
    JSObject::Dump(os);
}

void JSDisplayNames::Dump(std::ostream &os) const
{
    os << " - Locale: ";
    GetLocale().Dump(os);
    os << "\n";
    os << " - Type: "<< static_cast<int>(GetType());
    os << "\n";
    os << " - Style: "<< static_cast<int>(GetStyle());
    os << "\n";
    os << " - Fallback: "<< static_cast<int>(GetFallback());
    os << "\n";
    os << " - IcuLDN: ";
    GetIcuLDN().Dump(os);
    os << "\n";
    JSObject::Dump(os);
}

void JSSegmenter::Dump(std::ostream &os) const
{
    os << " - Locale: ";
    GetLocale().Dump(os);
    os << "\n";
    os << " - Granularity: "<< static_cast<int>(GetGranularity());
    os << "\n";
    os << " - IcuField: ";
    GetIcuField().Dump(os);
    os << "\n";
    JSObject::Dump(os);
}

void JSSegments::Dump(std::ostream &os) const
{
    os << " - SegmentsString: ";
    GetSegmentsString().Dump(os);
    os << "\n";
    os << " - UnicodeString: ";
    GetUnicodeString().Dump(os);
    os << "\n";
    os << " - Granularity: "<< static_cast<int>(GetGranularity());
    os << "\n";
    os << " - IcuField: ";
    GetIcuField().Dump(os);
    os << "\n";
    JSObject::Dump(os);
}

void JSSegmentIterator::Dump(std::ostream &os) const
{
    os << " - IteratedString: ";
    GetIteratedString().Dump(os);
    os << "\n";
    os << " - UnicodeString: ";
    GetUnicodeString().Dump(os);
    os << "\n";
    os << " - Granularity: "<< static_cast<int>(GetGranularity());
    os << "\n";
    os << " - IcuField: ";
    GetIcuField().Dump(os);
    os << "\n";
    JSObject::Dump(os);
}

void JSListFormat::Dump(std::ostream &os) const
{
    os << " - Locale: ";
    GetLocale().Dump(os);
    os << "\n";
    os << " - Type: "<< static_cast<int>(GetType());
    os << "\n";
    os << " - Style: "<< static_cast<int>(GetStyle());
    os << "\n";
    os << " - IcuLF: ";
    GetIcuLF().Dump(os);
    os << "\n";
    JSObject::Dump(os);
}
#endif
void JSGeneratorObject::Dump(std::ostream &os) const
{
    os << " - GeneratorContext: ";
    GetGeneratorContext().Dump(os);
    os << "\n";
    os << " - ResumeResult: ";
    GetResumeResult().Dump(os);
    os << "\n";
    os << " - GeneratorState: " << static_cast<uint8_t>(GetGeneratorState());
    os << "\n";
    os << " - ResumeMode: " << static_cast<uint8_t>(GetResumeMode());
    os << "\n";
    JSObject::Dump(os);
}

void JSAsyncGeneratorObject::Dump(std::ostream &os) const
{
    os << " - GeneratorContext: ";
    GetGeneratorContext().Dump(os);
    os << "\n";
    os << " - AsyncGeneratorQueue: ";
    GetAsyncGeneratorQueue().Dump(os);
    os << "\n";
    os << " - GeneratorBrand: ";
    GetGeneratorBrand().Dump(os);
    os << "\n";
    os << " - ResumeResult: ";
    GetResumeResult().Dump(os);
    os << "\n";
    os << " - AsyncGeneratorState: " << static_cast<uint8_t>(GetAsyncGeneratorState());
    os << "\n";
    os << " - ResumeMode: " << static_cast<uint8_t>(GetResumeMode());
    os << "\n";
    JSObject::Dump(os);
}

void JSAsyncFuncObject::Dump(std::ostream &os) const
{
    os << " - Promise: ";
    GetPromise().Dump(os);
    os << "\n";
}

void GeneratorContext::Dump(std::ostream &os) const
{
    os << " - RegsArray: ";
    GetRegsArray().Dump(os);
    os << "\n";
    os << " - Method: ";
    GetMethod().Dump(os);
    os << "\n";
    os << " - This: ";
    GetThis().Dump(os);
    os << "\n";
    os << " - Acc: ";
    GetAcc().Dump(os);
    os << "\n";
    os << " - GeneratorObject: ";
    GetGeneratorObject().Dump(os);
    os << "\n";
    os << " - LexicalEnv: ";
    GetLexicalEnv().Dump(os);
    os << "\n";
    os << " - NRegs: " << GetNRegs();
    os << "\n";
    os << " - BCOffset: " << GetBCOffset();
    os << "\n";
}

void EnumCache::Dump(std::ostream &os) const
{
    os << " - EnumCacheOwn: " << GetEnumCacheOwn();
    os << "\n";
    os << " - EnumCacheAll: " << GetEnumCacheAll();
    os << "\n";
    os << " - ProtoChainInfoEnumCache: " << GetProtoChainInfoEnumCache();
    os << "\n";
    os << " - EnumCacheKind: " << GetEnumCacheKind();
    os << "\n";
}

void ProtoChangeMarker::Dump(std::ostream &os) const
{
    os << " - HasChanged: " << GetHasChanged() << "\n";
    os << " - HasAccessorChanged: " << GetAccessorHasChanged() << "\n";
    os << " - HasNotFoundChanged: " << GetNotFoundHasChanged() << "\n";
}

void MarkerCell::Dump(std::ostream &os) const
{
    os << " - IsDetectorInvalid: " << GetIsDetectorInvalid() << "\n";
}

void ProtoChangeDetails::Dump(std::ostream &os) const
{
    os << " - ChangeListener: ";
    GetChangeListener().Dump(os);
    os << " \t- RegisterIndex: " << GetRegisterIndex();
    os << "\n";
}

void TrackInfo::Dump(std::ostream &os) const
{
    os << " - ElementsKind: " << static_cast<uint32_t>(GetElementsKind()) << "\n";
    os << " - ArrayLength: " << static_cast<uint32_t>(GetArrayLength()) << "\n";
    os << " - SpaceFlag: " << static_cast<uint32_t>(GetSpaceFlag()) << "\n";
}

void MachineCode::Dump(std::ostream &os) const
{
    os << " - InstructionSizeInBytes: " << GetInstructionSizeInBytes();
    os << "\n";
}

void ClassInfoExtractor::Dump(std::ostream &os) const
{
    os << " - NonStaticKeys: ";
    GetNonStaticKeys().Dump(os);
    os << "\n";
    os << " - NonStaticProperties: ";
    GetNonStaticProperties().Dump(os);
    os << "\n";
    os << " - NonStaticElements: ";
    GetNonStaticElements().Dump(os);
    os << "\n";
    os << " - StaticKeys: ";
    GetStaticKeys().Dump(os);
    os << "\n";
    os << " - StaticProperties: ";
    GetStaticProperties().Dump(os);
    os << "\n";
    os << " - StaticElements: ";
    GetStaticElements().Dump(os);
    os << "\n";
}

void SourceTextModule::Dump(std::ostream &os) const
{
    os << " - Environment: ";
    GetEnvironment().Dump(os);
    os << "\n";
    os << " - Namespace: ";
    GetNamespace().Dump(os);
    os << "\n";
    os << " - EcmaModuleFilename: ";
    os << GetEcmaModuleFilenameString();
    os << "\n";
    os << " - EcmaModuleRecordName: ";
    os << GetEcmaModuleRecordNameString();
    os << "\n";
    os << " - ModuleRequests: ";
    GetModuleRequests().Dump(os);
    os << "\n";
    os << " - RequestedModules: ";
    GetRequestedModules().Dump(os);
    os << "\n";
    os << " - ImportEntries: ";
    GetImportEntries().Dump(os);
    os << "\n";
    os << " - LocalExportEntries: ";
    GetLocalExportEntries().Dump(os);
    os << "\n";
    os << " - IndirectExportEntries: ";
    GetIndirectExportEntries().Dump(os);
    os << "\n";
    os << " - StarExportEntries: ";
    GetStarExportEntries().Dump(os);
    os << "\n";
    os << " - Status: ";
    os << static_cast<int32_t>(GetStatus());
    os << "\n";
    os << " - Exception: ";
    GetException().Dump(os);
    os << "\n";
    os << " - DFSIndex: ";
    os << GetDFSIndex();
    os << "\n";
    os << " - DFSAncestorIndex: ";
    os << GetDFSAncestorIndex();
    os << "\n";
    os << " - NameDictionary: ";
    GetNameDictionary().Dump(os);
    os << "\n";
    os << " - CycleRoot: ";
    // avoid infinite dump
    if (GetCycleRoot() != JSTaggedValue(this)) {
        GetCycleRoot().Dump(os);
    } else {
        os << "this";
    }
    os << "\n";
    os << " - TopLevelCapability: ";
    GetTopLevelCapability().Dump(os);
    os << "\n";
    os << " - AsyncParentModules: ";
    GetAsyncParentModules().Dump(os);
    os << "\n";
    os << " - SendableEnv: ";
    GetSendableEnv().Dump(os);
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

void ImportEntry::Dump(std::ostream &os) const
{
    os << " - ModuleRequestIndex: ";
    os << GetModuleRequestIndex();
    os << "\n";
    os << " - ImportName: ";
    GetImportName().Dump(os);
    os << "\n";
    os << " - LocalName: ";
    GetLocalName().Dump(os);
    os << "\n";
}

void LocalExportEntry::Dump(std::ostream &os) const
{
    os << " - ExportName: ";
    GetExportName().Dump(os);
    os << "\n";
    os << " - LocalName: ";
    GetLocalName().Dump(os);
    os << "\n";
    os << " - LocalIndex: " << GetLocalIndex();
    os << "\n";
}

void IndirectExportEntry::Dump(std::ostream &os) const
{
    os << " - ExportName: ";
    GetExportName().Dump(os);
    os << "\n";
    os << " - ModuleRequestIndex: ";
    os << GetModuleRequestIndex();
    os << "\n";
    os << " - ImportName: ";
    GetImportName().Dump(os);
    os << "\n";
}

void StarExportEntry::Dump(std::ostream &os) const
{
    os << " - ModuleRequestIndex: ";
    os << GetModuleRequestIndex();
    os << "\n";
}

void ResolvedBinding::Dump(std::ostream &os) const
{
    os << " - Module: ";
    GetModule().Dump(os);
    os << "\n";
    os << " - BindingName: ";
    GetBindingName().Dump(os);
    os << "\n";
}

void ResolvedIndexBinding::Dump(std::ostream &os) const
{
    os << " - Module: ";
    GetModule().Dump(os);
    os << "\n";
    os << " - Index: ";
    GetIndex();
    os << "\n";
}

void ResolvedRecordIndexBinding::Dump(std::ostream &os) const
{
    os << " - Module: ";
    GetModuleRecord().Dump(os);
    os << "\n";
    os << " - AbcFileName: ";
    GetAbcFileName().Dump(os);
    os << "\n";
    os << " - Index: ";
    GetIndex();
    os << "\n";
}

void ResolvedRecordBinding::Dump(std::ostream &os) const
{
    os << " - Module: ";
    GetModuleRecord().Dump(os);
    os << "\n";
    os << " - BindingName: ";
    GetBindingName().Dump(os);
    os << "\n";
}

void ModuleNamespace::Dump(std::ostream &os) const
{
    os << " - Exports: ";
    GetExports().Dump(os);
    os << "\n";
}

void NativeModuleFailureInfo::Dump(std::ostream &os) const
{
    os << " - ArkNativeModuleFailureInfo: ";
    GetArkNativeModuleFailureInfo().Dump(os);
    os << "\n";
}

void CjsModule::Dump(std::ostream &os) const
{
    os << " - current module path: ";
    GetPath().Dump(os);
    os << "\n";
    os << " - current module filename: ";
    GetFilename().Dump(os);
    os << "\n";
}

void CjsRequire::Dump(std::ostream &os) const
{
    os << " --- CjsRequire is JSFunction: ";
    os << "\n";
}

void CjsExports::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHClass *jshclass = GetJSHClass();
    os << " - hclass: " << std::hex << jshclass << "\n";
    os << " - prototype: ";
    jshclass->GetPrototype().DumpTaggedValue(os);
    os << "\n";

    TaggedArray *properties = TaggedArray::Cast(GetProperties().GetTaggedObject());
    os << " - properties: " << std::hex << properties;

    if (!properties->IsDictionaryMode()) {
        JSTaggedValue attrs = jshclass->GetLayout();
        if (attrs.IsNull()) {
            return;
        }

        LayoutInfo *layoutInfo = LayoutInfo::Cast(attrs.GetTaggedObject());
        int propNumber = static_cast<int>(jshclass->NumberOfProps());
        os << " <LayoutInfo[" << std::dec << propNumber << "]>\n";
        for (int i = 0; i < propNumber; i++) {
            JSTaggedValue key = layoutInfo->GetKey(i);
            PropertyAttributes attr = layoutInfo->GetAttr(i);
            ASSERT(i == static_cast<int>(attr.GetOffset()));
            os << "     " << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            DumpPropertyKey(key, os);
            os << ": (";
            JSTaggedValue val;
            if (attr.IsInlinedProps()) {
                val = GetPropertyInlinedPropsWithRep(i, attr);
            } else {
                val = properties->Get(i - static_cast<int>(jshclass->GetInlinedProperties()));
            }
            val.DumpTaggedValue(os);
            os << ") ";
            DumpAttr(attr, true, os);
            os << "\n";
        }
    } else {
        NameDictionary *dict = NameDictionary::Cast(properties);
        os << " <NameDictionary[" << std::dec << dict->EntriesCount() << "]>\n";
        dict->Dump(os);
    }
}

void JSFunctionBase::Dump(std::ostream &os) const
{
    os << " - Method: ";
    GetMethod().Dump(os);
    os << "\n";
}

void Method::Dump(std::ostream &os) const
{
    os << " - MethodName: ";
    os << GetMethodName();
    os << "\n";
    os << " - ConstantPool: ";
    GetConstantPool().Dump(os);
    os << "\n";
    os << " - FunctionKind: " << static_cast<int>(GetFunctionKind());
    os << "\n";
    os << " - CodeEntryOrLiteral: " << std::hex << GetCodeEntryOrLiteral() << "\n";
    os << "\n";
}

void ClassLiteral::Dump(std::ostream &os) const
{
    os << " - ClassLiteral: ";
    os << "\n";
    os << " - IsAOTUsed: " << std::boolalpha << GetIsAOTUsed();
    os << "\n";
    os << " - Array: ";
    GetArray().Dump(os);
    os << "\n";
}

// ########################################################################################
// Dump for Snapshot
// ########################################################################################
static void DumpArrayClass(const TaggedArray *arr, std::vector<Reference> &vec)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetLength();
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->Get(i));
        CString str = ToCString(i);
        vec.emplace_back(str, val);
    }
}

static void DumpMutantTaggedArrayClass(const MutantTaggedArray *arr, std::vector<Reference> &vec)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetLength();
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->Get(i));
        CString str = ToCString(i);
        vec.emplace_back(str, val);
    }
}

static void DumpCOWMutantTaggedArrayClass(const COWMutantTaggedArray *arr, std::vector<Reference> &vec)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetLength();
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->Get(i));
        CString str = ToCString(i);
        vec.emplace_back(str, val);
    }
}

static void DumpElementClass(const TaggedArray *arr, std::vector<Reference> &vec)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetLength();
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->Get(i));
        vec.emplace_back(i, val, Reference::ReferenceType::ELEMENT);
    }
}

static void DumpConstantPoolClass(const ConstantPool *arr, std::vector<Reference> &vec)
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = arr->GetCacheLength();
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(arr->GetObjectFromCache(i));
        CString str = ToCString(i);
        vec.emplace_back(str, val);
    }
}

static void DumpStringClass([[maybe_unused]] const EcmaString *str, [[maybe_unused]] std::vector<Reference> &vec)
{
    // Before EcmaString dump self node, it need not show, so delete.
    // If some properties need be shown, add here.
}

static void DumpObject(TaggedObject *obj, std::vector<Reference> &vec, bool isVmMode)
{
    JSTaggedValue objValue(obj);
    DISALLOW_GARBAGE_COLLECTION;
    auto jsHclass = obj->GetClass();
    JSType type = jsHclass->GetObjectType();
    vec.emplace_back("hclass", JSTaggedValue(jsHclass));
    switch (type) {
        case JSType::HCLASS:
            JSHClass::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::TAGGED_ARRAY:
        case JSType::TAGGED_DICTIONARY:
        case JSType::LEXICAL_ENV:
        case JSType::SFUNCTION_ENV:
        case JSType::SENDABLE_ENV:
        case JSType::COW_TAGGED_ARRAY:
        case JSType::AOT_LITERAL_INFO:
            DumpArrayClass(TaggedArray::Cast(obj), vec);
            break;
        case JSType::MUTANT_TAGGED_ARRAY:
            DumpMutantTaggedArrayClass(MutantTaggedArray::Cast(obj), vec);
            break;
        case JSType::COW_MUTANT_TAGGED_ARRAY:
            DumpCOWMutantTaggedArrayClass(COWMutantTaggedArray::Cast(obj), vec);
            break;
        case JSType::CONSTANT_POOL:
            DumpConstantPoolClass(ConstantPool::Cast(obj), vec);
            break;
        case JSType::PROFILE_TYPE_INFO_CELL_0:
        case JSType::PROFILE_TYPE_INFO_CELL_1:
        case JSType::PROFILE_TYPE_INFO_CELL_N:
            ProfileTypeInfoCell::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::FUNCTION_TEMPLATE:
            FunctionTemplate::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::VTABLE:
            VTable::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::PROFILE_TYPE_INFO:
            ProfileTypeInfo::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::LINE_STRING:
        case JSType::TREE_STRING:
        case JSType::SLICED_STRING:
            DumpStringClass(EcmaString::Cast(obj), vec);
            break;
        case JSType::JS_NATIVE_POINTER:
            break;
        case JSType::JS_OBJECT:
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
            JSObject::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_FUNCTION_BASE:
        case JSType::JS_FUNCTION:
        case JSType::JS_SHARED_FUNCTION:
            JSFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_BOUND_FUNCTION:
            JSBoundFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_SET:
            JSSet::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_SHARED_SET:
            JSSharedSet::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_MAP:
            JSMap::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_SHARED_MAP:
            JSSharedMap::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_WEAK_SET:
            JSWeakSet::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_WEAK_MAP:
            JSWeakMap::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_WEAK_REF:
            JSWeakRef::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_FINALIZATION_REGISTRY:
            JSFinalizationRegistry::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::CELL_RECORD:
            CellRecord::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_REG_EXP:
            JSRegExp::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_DATE:
            JSDate::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_ARRAY:
            JSArray::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_SHARED_ARRAY:
            JSSharedArray::Cast(obj)->DumpForSnapshot(vec);
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
            JSTypedArray::Cast(obj)->DumpForSnapshot(vec);
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
            JSSharedTypedArray::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::BIGINT:
            BigInt::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::BYTE_ARRAY:
            ByteArray::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_PROXY:
            JSProxy::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_PRIMITIVE_REF:
            JSPrimitiveRef::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::SYMBOL:
            JSSymbol::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::ACCESSOR_DATA:
        case JSType::INTERNAL_ACCESSOR:
            AccessorData::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_DATA_VIEW:
            JSDataView::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::PROMISE_REACTIONS:
            PromiseReaction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::PROMISE_CAPABILITY:
            PromiseCapability::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::PROMISE_ITERATOR_RECORD:
            PromiseIteratorRecord::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::PROMISE_RECORD:
            PromiseRecord::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::RESOLVING_FUNCTIONS_RECORD:
            ResolvingFunctionsRecord::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_PROMISE:
            JSPromise::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_PROMISE_REACTIONS_FUNCTION:
            JSPromiseReactionsFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_PROMISE_EXECUTOR_FUNCTION:
            JSPromiseExecutorFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION:
            JSAsyncModuleFulfilledFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION:
            JSAsyncModuleRejectedFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::ASYNC_GENERATOR_REQUEST:
            AsyncGeneratorRequest::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::ASYNC_ITERATOR_RECORD:
            AsyncIteratorRecord::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_ASYNC_FROM_SYNC_ITERATOR:
            JSAsyncFromSyncIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION:
            JSAsyncFromSyncIterUnwarpFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION:
            JSPromiseAllResolveElementFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION:
            JSPromiseAnyRejectElementFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION:
            JSPromiseAllSettledElementFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_PROMISE_FINALLY_FUNCTION:
            JSPromiseFinallyFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION:
            JSPromiseValueThunkOrThrowerFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN:
            JSAsyncGeneratorResNextRetProRstFtn::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::MICRO_JOB_QUEUE:
            MicroJobQueue::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::PENDING_JOB:
            PendingJob::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::COMPLETION_RECORD:
            CompletionRecord::Cast(obj)->DumpForSnapshot(vec);
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
            JSArrayBuffer::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_SHARED_ARRAY_BUFFER:
            JSArrayBuffer::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_SENDABLE_ARRAY_BUFFER:
            JSSendableArrayBuffer::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_PROXY_REVOC_FUNCTION:
            JSProxyRevocFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_ASYNC_FUNCTION:
        case JSType::JS_SHARED_ASYNC_FUNCTION:
            JSAsyncFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION:
            JSAsyncAwaitStatusFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_GENERATOR_FUNCTION:
            JSGeneratorFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_ASYNC_GENERATOR_FUNCTION:
            JSAsyncGeneratorFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_INTL_BOUND_FUNCTION:
            JSIntlBoundFunction::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_REALM:
            JSRealm::Cast(obj)->DumpForSnapshot(vec);
            break;
#ifdef ARK_SUPPORT_INTL
        case JSType::JS_INTL:
            JSIntl::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_LOCALE:
            JSLocale::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_DATE_TIME_FORMAT:
            JSDateTimeFormat::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_RELATIVE_TIME_FORMAT:
            JSRelativeTimeFormat::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_NUMBER_FORMAT:
            JSNumberFormat::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_COLLATOR:
            JSCollator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_PLURAL_RULES:
            JSPluralRules::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_DISPLAYNAMES:
            JSDisplayNames::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_SEGMENTER:
            JSSegmenter::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_SEGMENTS:
            JSSegments::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_SEGMENT_ITERATOR:
            JSSegmentIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_LIST_FORMAT:
            JSListFormat::Cast(obj)->DumpForSnapshot(vec);
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
            CjsModule::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_CJS_EXPORTS:
            CjsExports::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_CJS_REQUIRE:
            CjsRequire::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_GENERATOR_OBJECT:
            JSGeneratorObject::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_ASYNC_GENERATOR_OBJECT:
            JSAsyncGeneratorObject::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_ASYNC_FUNC_OBJECT:
            JSAsyncFuncObject::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_GENERATOR_CONTEXT:
            GeneratorContext::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_ARRAY_LIST:
            JSAPIArrayList::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_ARRAYLIST_ITERATOR:
            JSAPIArrayListIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::LINKED_NODE:
            LinkedNode::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::RB_TREENODE:
            break;
        case JSType::JS_API_HASH_MAP:
            JSAPIHashMap::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_HASH_SET:
            JSAPIHashSet::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_HASHMAP_ITERATOR:
            JSAPIHashMapIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_HASHSET_ITERATOR:
            JSAPIHashSetIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_MAP:
            JSAPILightWeightMap::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR:
            JSAPILightWeightMapIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_SET:
            JSAPILightWeightSet::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR:
            JSAPILightWeightSetIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_TREE_MAP:
            JSAPITreeMap::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_TREE_SET:
            JSAPITreeSet::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_TREEMAP_ITERATOR:
            JSAPITreeMapIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_TREESET_ITERATOR:
            JSAPITreeSetIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_VECTOR:
            JSAPIVector::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_VECTOR_ITERATOR:
            JSAPIVectorIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_BITVECTOR:
            JSAPIBitVector::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_BITVECTOR_ITERATOR:
            JSAPIBitVectorIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_FAST_BUFFER:
            JSAPIFastBuffer::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_QUEUE:
            JSAPIQueue::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_QUEUE_ITERATOR:
            JSAPIQueueIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_DEQUE:
            JSAPIDeque::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_DEQUE_ITERATOR:
            JSAPIDequeIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_STACK:
            JSAPIStack::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_STACK_ITERATOR:
            JSAPIStackIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_LIST:
            JSAPIList::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_LINKED_LIST:
            JSAPILinkedList::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_LIST_ITERATOR:
            JSAPIListIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_LINKED_LIST_ITERATOR:
            JSAPILinkedListIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::SOURCE_TEXT_MODULE_RECORD:
            SourceTextModule::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::IMPORTENTRY_RECORD:
            ImportEntry::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::LOCAL_EXPORTENTRY_RECORD:
            LocalExportEntry::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::INDIRECT_EXPORTENTRY_RECORD:
            IndirectExportEntry::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::STAR_EXPORTENTRY_RECORD:
            StarExportEntry::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::RESOLVEDBINDING_RECORD:
            ResolvedBinding::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::RESOLVEDINDEXBINDING_RECORD:
            ResolvedIndexBinding::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::RESOLVEDRECORDINDEXBINDING_RECORD:
            ResolvedRecordIndexBinding::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::RESOLVEDRECORDBINDING_RECORD:
            ResolvedRecordBinding::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_MODULE_NAMESPACE:
            ModuleNamespace::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::NATIVE_MODULE_FAILURE_INFO:
            NativeModuleFailureInfo::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_PLAIN_ARRAY:
            JSAPIPlainArray::Cast(obj)->DumpForSnapshot(vec);
            break;
        case JSType::JS_API_PLAIN_ARRAY_ITERATOR:
            JSAPIPlainArrayIterator::Cast(obj)->DumpForSnapshot(vec);
            break;
        default:
            break;
    }
    if (isVmMode) {
        switch (type) {
            case JSType::PROPERTY_BOX:
                PropertyBox::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::TEMPLATE_MAP:
                DumpArrayClass(TaggedArray::Cast(obj), vec);
                break;
            case JSType::GLOBAL_ENV:
                GlobalEnv::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::ENUM_CACHE:
                EnumCache::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::PROTO_CHANGE_MARKER:
                ProtoChangeMarker::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::MARKER_CELL:
                MarkerCell::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::PROTOTYPE_INFO:
                ProtoChangeDetails::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::TRACK_INFO:
                TrackInfo::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::PROGRAM:
                Program::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::MACHINE_CODE_OBJECT:
                MachineCode::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::TRANSITION_HANDLER:
                TransitionHandler::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::TRANS_WITH_PROTO_HANDLER:
                TransWithProtoHandler::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::STORE_TS_HANDLER:
                StoreAOTHandler::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::PROTOTYPE_HANDLER:
                PrototypeHandler::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::CLASS_INFO_EXTRACTOR:
                ClassInfoExtractor::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::METHOD:
                Method::Cast(obj)->DumpForSnapshot(vec);
                break;
            case JSType::CLASS_LITERAL:
                ClassLiteral::Cast(obj)->DumpForSnapshot(vec);
                break;
            default:
                break;
        }
    } else {
        vec.pop_back();
    }
}

static inline void EcmaStringToStd(CString &res, EcmaString *str)
{
    if (EcmaStringAccessor(str).GetLength() == 0) {
        CString emptyStr = "EmptyString";
        res.append(emptyStr);
    }

    CString string = ConvertToString(str);
    res.append(string);
}

static void KeyToStd(CString &res, JSTaggedValue key)
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
            EcmaStringToStd(res, EcmaString::Cast(key.GetTaggedObject()));
        } else if (key.IsSymbol()) {
            JSSymbol *sym = JSSymbol::Cast(key.GetTaggedObject());
            JSTaggedValue desc = sym->GetDescription();
            if (desc.IsString()) {
                EcmaStringToStd(res, EcmaString::Cast(desc.GetTaggedObject()));
            }
        }
    }
}

void JSAPIPlainArray::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    TaggedArray *keys = TaggedArray::Cast(GetKeys().GetTaggedObject());
    TaggedArray *values = TaggedArray::Cast(GetValues().GetTaggedObject());
    uint32_t len = static_cast<uint32_t>(GetLength());
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        CString str;
        KeyToStd(str, keys->Get(i));
        vec.emplace_back(str, values->Get(i));
    }
    JSObject::DumpForSnapshot(vec);
}

void JSTaggedValue::DumpForSnapshot(std::vector<Reference> &vec, bool isVmMode) const
{
    if (IsHeapObject()) {
        return DumpObject(GetTaggedObject(), vec, isVmMode);
    }
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

void NumberDictionary::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    vec.reserve(vec.size() + size);
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            JSTaggedValue val(GetValue(hashIndex));
            vec.emplace_back(
                static_cast<uint32_t>(JSTaggedNumber(key).GetNumber()), val, Reference::ReferenceType::ELEMENT);
        }
    }
}

void NameDictionary::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    vec.reserve(vec.size() + size);
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            JSTaggedValue val(GetValue(hashIndex));
            CString str;
            KeyToStd(str, key);
            vec.emplace_back(str, val);
        }
    }
}

void GlobalDictionary::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    vec.reserve(vec.size() + size);
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            CString str;
            KeyToStd(str, key);
            JSTaggedValue val = GetValue(hashIndex);
            vec.emplace_back(str, val);
        }
    }
}

void LinkedHashSet::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfElements() + NumberOfDeletedElements();
    vec.reserve(vec.size() + capacity);
    for (int hashIndex = 0; hashIndex < capacity; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            CString str;
            KeyToStd(str, key);
            vec.emplace_back(str, JSTaggedValue::Hole());
        }
    }
}

void LinkedHashMap::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfElements() + NumberOfDeletedElements();
    vec.reserve(vec.size() + capacity);
    for (int hashIndex = 0; hashIndex < capacity; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            JSTaggedValue val = GetValue(hashIndex);
            CString str;
            KeyToStd(str, key);
            vec.emplace_back(str, val);
        }
    }
}

void TaggedHashArray::Dump(std::ostream &os) const
{
    DumpArrayClass(this, os);
}

void TaggedHashArray::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = static_cast<int>(GetLength());
    vec.reserve(vec.size() + capacity);
    for (int hashIndex = 0; hashIndex < capacity; hashIndex++) {
        JSTaggedValue value = Get(hashIndex);
        if (!value.IsUndefined() && !value.IsHole() && !value.IsNull()) {
            LinkedNode *node = LinkedNode::Cast(value.GetTaggedObject());
            node->DumpForSnapshot(vec);
        }
    }
}

template <typename T>
void DumpForSnapshotTaggedTreeEntry(T tree, int index, std::vector<Reference> &vec, bool isMap = false)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (isMap) {
        vec.emplace_back("key", JSTaggedValue(tree->GetKey(index)));
    }
    vec.emplace_back("value", JSTaggedValue(tree->GetValue(index)));
    vec.emplace_back("parent", JSTaggedValue(tree->GetParent(index)));
    vec.emplace_back("color", JSTaggedValue(static_cast<int>(tree->GetColor(index))));
    vec.emplace_back("left", tree->GetLeftChild(index));
    vec.emplace_back("right", tree->GetRightChild(index));
}

void TaggedTreeMap::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    vec.emplace_back("Elements", TaggedArray::Get(NUMBER_OF_ELEMENTS_INDEX));
    vec.emplace_back("Delete", TaggedArray::Get(NUMBER_OF_HOLE_ENTRIES_INDEX));
    vec.emplace_back("Capacity", TaggedArray::Get(CAPACITY_INDEX));
    vec.emplace_back("RootNode", TaggedArray::Get(ROOT_INDEX));
    vec.emplace_back("CompareFunction", TaggedArray::Get(COMPARE_FUNCTION_INDEX));
    uint32_t capacity = NumberOfElements() + NumberOfDeletedElements();
    vec.reserve(vec.size() + capacity);
    for (uint32_t index = 0; index < capacity; index++) {
        JSTaggedValue key(GetKey(index));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            JSTaggedValue val = GetValue(index);
            CString str;
            KeyToStd(str, key);
            vec.emplace_back(str, val);
        } else {
            DumpForSnapshotTaggedTreeEntry(const_cast<TaggedTreeMap *>(this), index, vec, true);
        }
    }
}

void TaggedTreeSet::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    vec.emplace_back("Elements", TaggedArray::Get(NUMBER_OF_ELEMENTS_INDEX));
    vec.emplace_back("Delete", TaggedArray::Get(NUMBER_OF_HOLE_ENTRIES_INDEX));
    vec.emplace_back("Capacity", TaggedArray::Get(CAPACITY_INDEX));
    vec.emplace_back("RootNode", TaggedArray::Get(ROOT_INDEX));
    vec.emplace_back("CompareFunction", TaggedArray::Get(COMPARE_FUNCTION_INDEX));
    uint32_t capacity = NumberOfElements() + NumberOfDeletedElements();
    vec.reserve(vec.size() + capacity);
    for (uint32_t index = 0; index < capacity; index++) {
        JSTaggedValue key(GetKey(index));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            CString str;
            KeyToStd(str, key);
            vec.emplace_back(str, JSTaggedValue::Hole());
        } else {
            DumpForSnapshotTaggedTreeEntry(const_cast<TaggedTreeSet *>(this), index, vec, true);
        }
    }
}

void TaggedDoubleList::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfNodes();
    vec.reserve(vec.size() + capacity);
    for (int index = 0; index < capacity; index++) {
        JSTaggedValue val = GetElement(index);
        CString str;
        KeyToStd(str, JSTaggedValue(index));
        vec.emplace_back(str, val);
    }
}

void TaggedSingleList::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfNodes();
    vec.reserve(vec.size() + capacity);
    for (int index = 0; index < capacity; index++) {
        JSTaggedValue val = GetElement(index);
        CString str;
        KeyToStd(str, JSTaggedValue(index));
        vec.emplace_back(str, val);
    }
}

void JSObject::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHClass *jshclass = GetJSHClass();
    if (jshclass != nullptr) {
        vec.emplace_back(CString("__proto__"), jshclass->GetPrototype());
    }
    vec.emplace_back(CString("ArkInternalHash"), JSTaggedValue(GetHash()));
    JSTaggedType hashField = Barriers::GetTaggedValue(this, HASH_OFFSET);
    if (JSTaggedValue(hashField).IsHeapObject()) {
        vec.emplace_back(CString("HashField"), JSTaggedValue(hashField));
    }

    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    vec.emplace_back("(object elements)", JSTaggedValue(elements));
    if (elements->GetLength() == 0) {
    } else if (!elements->IsDictionaryMode()) {
        DumpElementClass(elements, vec);
    } else {
        NumberDictionary *dict = NumberDictionary::Cast(elements);
        dict->DumpForSnapshot(vec);
    }

    TaggedArray *properties = TaggedArray::Cast(GetProperties().GetTaggedObject());
    if (IsJSGlobalObject()) {
        GlobalDictionary *dict = GlobalDictionary::Cast(properties);
        dict->DumpForSnapshot(vec);
        return;
    }
    vec.emplace_back("(object properties)", JSTaggedValue(properties));
    if ((!properties->IsDictionaryMode()) && (jshclass != nullptr)) {
        JSTaggedValue attrs = jshclass->GetLayout();
        if (attrs.IsNull()) {
            return;
        }

        LayoutInfo *layoutInfo = LayoutInfo::Cast(attrs.GetTaggedObject());
        int propNumber = static_cast<int>(jshclass->NumberOfProps());
        vec.reserve(vec.size() + propNumber);
        for (int i = 0; i < propNumber; i++) {
            JSTaggedValue key = layoutInfo->GetKey(i);
            PropertyAttributes attr = layoutInfo->GetAttr(i);
            ASSERT(i == static_cast<int>(attr.GetOffset()));
            JSTaggedValue val;
            if (attr.IsInlinedProps()) {
                val = GetPropertyInlinedPropsWithRep(i, attr);
            } else {
                val = properties->Get(i - static_cast<int>(jshclass->GetInlinedProperties()));
            }

            CString str;
            KeyToStd(str, key);
            vec.emplace_back(str, val);
        }
    } else {
        NameDictionary *dict = NameDictionary::Cast(properties);
        dict->DumpForSnapshot(vec);
    }
}

void JSHClass::DumpForSnapshot([[maybe_unused]] std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("__proto__"), GetPrototype());
    vec.emplace_back(CString("Layout"), GetLayout());
    vec.emplace_back(CString("Transitions"), GetTransitions());
    vec.emplace_back(CString("Parent"), GetParent());
    vec.emplace_back(CString("ProtoChangeMarker"), GetProtoChangeMarker());
    vec.emplace_back(CString("ProtoChangeDetails"), GetProtoChangeDetails());
    vec.emplace_back(CString("EnumCache"), GetEnumCache());
    vec.emplace_back(CString("BitField"), JSTaggedValue(GetBitField()));
    vec.emplace_back(CString("BitField1"), JSTaggedValue(GetBitField1()));
}

void JSFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ProtoOrHClass"), GetProtoOrHClass());
    vec.emplace_back(CString("LexicalEnv"), GetLexicalEnv());
    vec.emplace_back(CString("RawProfileTypeInfo"), GetRawProfileTypeInfo());
    vec.emplace_back(CString("HomeObject"), GetHomeObject());
    vec.emplace_back(CString("Module"), GetModule());
    vec.emplace_back(CString("Method"), GetMethod());
    if ((!GetMethod().IsNull()) && (!GetMethod().IsUndefined())) {
        vec.emplace_back(CString("FunctionKind"), JSTaggedValue(static_cast<int>(GetFunctionKind())));
    }
    vec.emplace_back(CString("FunctionExtraInfo"), GetFunctionExtraInfo());
    vec.emplace_back(CString("Method"), GetMethod());
    JSObject::DumpForSnapshot(vec);
}

void Method::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("MethodName"), JSTaggedValue(GetMethodName()));
    vec.emplace_back(CString("ConstantPool"), GetConstantPool());
}

void Program::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("MainFunction"), GetMainFunction());
}

void LinkedNode::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSTaggedValue next = GetNext();
    if (!next.IsUndefined() && !next.IsHole() && !next.IsNull()) {
        LinkedNode *nextNode = LinkedNode::Cast(next.GetTaggedObject());
        vec.emplace_back(CString("Next"), next);
        nextNode->DumpForSnapshot(vec);
    }
    vec.emplace_back(CString("Key"), GetKey());
    JSTaggedValue key = GetKey();
    CString str;
    KeyToStd(str, key);
    vec.emplace_back(str, GetValue());
}

void ConstantPool::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DumpArrayClass(this, vec);
}

void ProfileTypeInfoCell::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Value"), GetValue());
    vec.emplace_back(CString("Handle"), GetHandle());
}

void FunctionTemplate::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Method"), GetMethod());
    vec.emplace_back(CString("Module"), GetModule());
    vec.emplace_back(CString("RawProfileTypeInfo"), GetRawProfileTypeInfo());
    vec.emplace_back(CString("Length"), JSTaggedValue(GetLength()));
}

void ExtraProfileTypeInfo::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Receiver"), GetReceiverObject());
    vec.emplace_back(CString("Holder"), GetHolderObject());
}

void VTable::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DumpArrayClass(this, vec);
}

void ProfileTypeInfo::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    uint32_t len = GetIcSlotLength();
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        JSTaggedValue val(Get(i));
        CString str = ToCString(i);
        vec.emplace_back(str, val);
    }
}

void COWTaggedArray::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DumpArrayClass(this, vec);
}

void MutantTaggedArray::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DumpMutantTaggedArrayClass(this, vec);
}

void COWMutantTaggedArray::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DumpCOWMutantTaggedArrayClass(this, vec);
}

void JSBoundFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSObject::DumpForSnapshot(vec);
    vec.emplace_back(CString("Method"), GetMethod());
    vec.emplace_back(CString("BoundTarget"), GetBoundTarget());
    vec.emplace_back(CString("BoundThis"), GetBoundThis());
    vec.emplace_back(CString("BoundArguments"), GetBoundArguments());
}

void JSPrimitiveRef::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("subValue"), GetValue());
    JSObject::DumpForSnapshot(vec);
}

void BigInt::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Length"), JSTaggedValue(GetLength()));
    vec.emplace_back(CString("Sign"), JSTaggedValue(GetSign()));
}

void JSDate::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("time"), GetTime());
    vec.emplace_back(CString("localOffset"), GetLocalOffset());

    JSObject::DumpForSnapshot(vec);
}

void JSMap::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetLinkedMap().IsInvalidValue())) {
        LinkedHashMap *map = LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject());
        vec.emplace_back("linkedmap", GetLinkedMap());
        map->DumpForSnapshot(vec);
    }

    JSObject::DumpForSnapshot(vec);
}

void JSSharedMap::DumpForSnapshot(std::vector<Reference> &vec) const
{
    LinkedHashMap *map = LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject());
    vec.emplace_back("linkedmap", GetLinkedMap());
    vec.emplace_back("ModRecord", JSTaggedValue(GetModRecord()));
    map->DumpForSnapshot(vec);

    JSObject::DumpForSnapshot(vec);
}

void JSForInIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Object"), GetObject());
    vec.emplace_back(CString("CachedHclass"), GetCachedHClass());
    vec.emplace_back(CString("Keys"), GetKeys());
    vec.emplace_back(CString("Index"), JSTaggedValue(GetIndex()));
    vec.emplace_back(CString("Length"), JSTaggedValue(GetLength()));
    JSObject::DumpForSnapshot(vec);
}

void JSMapIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedMap().IsInvalidValue())) {
        LinkedHashMap *map = LinkedHashMap::Cast(GetIteratedMap().GetTaggedObject());
        vec.emplace_back("iteratedmap", GetIteratedMap());
        map->DumpForSnapshot(vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(vec);
}

void JSSharedMapIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSSharedMap *iteratedMap = JSSharedMap::Cast(GetIteratedMap().GetTaggedObject());
    LinkedHashMap *map = LinkedHashMap::Cast(iteratedMap->GetLinkedMap().GetTaggedObject());
    vec.emplace_back("iteratedmap", GetIteratedMap());
    map->DumpForSnapshot(vec);
    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(vec);
}

void JSSet::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetLinkedSet().IsInvalidValue())) {
        LinkedHashSet *set = LinkedHashSet::Cast(GetLinkedSet().GetTaggedObject());
        vec.emplace_back("linkedset", GetLinkedSet());
        set->DumpForSnapshot(vec);
    }

    JSObject::DumpForSnapshot(vec);
}

void JSSharedSet::DumpForSnapshot(std::vector<Reference> &vec) const
{
    LinkedHashSet *set = LinkedHashSet::Cast(GetLinkedSet().GetTaggedObject());
    vec.emplace_back("linkedset", GetLinkedSet());
    vec.emplace_back("ModRecord", JSTaggedValue(GetModRecord()));
    set->DumpForSnapshot(vec);

    JSObject::DumpForSnapshot(vec);
}

void JSWeakMap::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetLinkedMap().IsInvalidValue())) {
        LinkedHashMap *map = LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject());
        vec.emplace_back("linkedmap", GetLinkedMap());
        map->DumpForSnapshot(vec);
    }

    JSObject::DumpForSnapshot(vec);
}

void JSWeakSet::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetLinkedSet().IsInvalidValue())) {
        LinkedHashSet *set = LinkedHashSet::Cast(GetLinkedSet().GetTaggedObject());
        vec.emplace_back("linkeset", GetLinkedSet());
        set->DumpForSnapshot(vec);
    }

    JSObject::DumpForSnapshot(vec);
}

void JSWeakRef::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("WeakObject"), GetWeakObject());
    JSObject::DumpForSnapshot(vec);
}

void JSFinalizationRegistry::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("CleanupCallback"), GetCleanupCallback());
    if (!(GetMaybeUnregister().IsInvalidValue())) {
        LinkedHashMap *map = LinkedHashMap::Cast(GetMaybeUnregister().GetTaggedObject());
        vec.emplace_back(CString("MaybeUnregister"), GetMaybeUnregister());
        map->DumpForSnapshot(vec);
    }

    vec.emplace_back(CString("Next"), GetNext());
    vec.emplace_back(CString("Prev"), GetPrev());
    JSObject::DumpForSnapshot(vec);
}

void CellRecord::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("WeakRefTarget"), GetWeakRefTarget());
    vec.emplace_back(CString("HeldValue"), GetHeldValue());
}

void JSSetIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedSet().IsInvalidValue())) {
        LinkedHashSet *set = LinkedHashSet::Cast(GetIteratedSet().GetTaggedObject());
        vec.emplace_back("iteratedset", GetIteratedSet());
        set->DumpForSnapshot(vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(vec);
}

void JSSharedSetIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSSharedSet *iteratedSet = JSSharedSet::Cast(GetIteratedSet().GetTaggedObject());
    LinkedHashSet *set = LinkedHashSet::Cast(iteratedSet->GetLinkedSet().GetTaggedObject());
    vec.emplace_back("iteratedset", GetIteratedSet());
    set->DumpForSnapshot(vec);
    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(vec);
}

void JSArray::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Length"), JSTaggedValue(GetLength()));
    JSObject::DumpForSnapshot(vec);
}

void JSSharedArray::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back("ModRecord: ", JSTaggedValue(GetModRecord()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIArrayList::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Length"), JSTaggedValue(GetLength()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIArrayListIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedArrayList().IsInvalidValue())) {
        JSAPIArrayList *arraylist = JSAPIArrayList::Cast(GetIteratedArrayList().GetTaggedObject());
        vec.emplace_back("iteratedlist", GetIteratedArrayList());
        arraylist->DumpForSnapshot(vec);
    }
    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPILightWeightMap::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    vec.emplace_back("Hashes", GetHashes());
    vec.emplace_back("Keys", GetKeys());
    TaggedArray *keys = TaggedArray::Cast(GetKeys().GetTaggedObject());
    TaggedArray *values = TaggedArray::Cast(GetValues().GetTaggedObject());
    uint32_t len = static_cast<uint32_t>(GetLength());
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        CString str;
        KeyToStd(str, keys->Get(i));
        vec.emplace_back(str, values->Get(i));
    }
    JSObject::DumpForSnapshot(vec);
}

void JSAPILightWeightMapIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedLightWeightMap().IsInvalidValue())) {
        JSAPILightWeightMap *map =
            JSAPILightWeightMap::Cast(GetIteratedLightWeightMap().GetTaggedObject());
        vec.emplace_back("iteratedmap", GetIteratedLightWeightMap());
        map->DumpForSnapshot(vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    vec.emplace_back(CString("BitField"), JSTaggedValue(GetBitField()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIQueue::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Length"), JSTaggedValue(GetLength()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIQueueIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedQueue().IsInvalidValue())) {
        JSAPIQueue *queue = JSAPIQueue::Cast(GetIteratedQueue().GetTaggedObject());
        vec.emplace_back("iteratedqueue", GetIteratedQueue());
        queue->DumpForSnapshot(vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIDeque::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPIDequeIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedDeque().IsInvalidValue())) {
        JSAPIDeque *deque = JSAPIDeque::Cast(GetIteratedDeque().GetTaggedObject());
        vec.emplace_back("iterateddeque", GetIteratedDeque());
        deque->DumpForSnapshot(vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPILightWeightSet::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    TaggedArray *hashes = TaggedArray::Cast(GetHashes().GetTaggedObject());
    TaggedArray *values = TaggedArray::Cast(GetValues().GetTaggedObject());
    uint32_t len = GetLength();
    vec.reserve(vec.size() + len);
    for (uint32_t i = 0; i < len; i++) {
        CString str;
        KeyToStd(str, hashes->Get(i));
        vec.emplace_back(str, values->Get(i));
    }
    JSObject::DumpForSnapshot(vec);
}

void JSAPILightWeightSetIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedLightWeightSet().IsInvalidValue())) {
        JSAPILightWeightSet *set =
            JSAPILightWeightSet::Cast(GetIteratedLightWeightSet().GetTaggedObject());
        vec.emplace_back("iteratedset", GetIteratedLightWeightSet());
        set->DumpForSnapshot(vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIStack::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPIStackIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedStack().IsInvalidValue())) {
        JSAPIStack *stack = JSAPIStack::Cast(GetIteratedStack().GetTaggedObject());
        vec.emplace_back("iteratedstack", GetIteratedStack());
        stack->DumpForSnapshot(vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSArrayIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedArray().IsInvalidValue())) {
        JSArray *array = JSArray::Cast(GetIteratedArray().GetTaggedObject());
        vec.emplace_back("iteratedarray", GetIteratedArray());
        array->DumpForSnapshot(vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    vec.emplace_back(CString("BitField"), JSTaggedValue(GetBitField()));
    JSObject::DumpForSnapshot(vec);
}

void JSSharedArrayIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSSharedArray *array = JSSharedArray::Cast(GetIteratedArray().GetTaggedObject());
    vec.emplace_back("iteratedarray", GetIteratedArray());
    array->DumpForSnapshot(vec);
    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    vec.emplace_back(CString("IterationKind"), JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIVector::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPIVectorIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (!(GetIteratedVector().IsInvalidValue())) {
        JSAPIVector *vector = JSAPIVector::Cast(GetIteratedVector().GetTaggedObject());
        vec.emplace_back("iteratedvector", GetIteratedVector());
        vector->DumpForSnapshot(vec);
    }

    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIBitVector::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPIBitVectorIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSAPIVector *vector = JSAPIVector::Cast(GetIteratedBitVector().GetTaggedObject());
    vec.emplace_back("iteratedbitvector", GetIteratedBitVector());
    vector->DumpForSnapshot(vec);
    vec.emplace_back(CString("NextIndex"), JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIFastBuffer::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSStringIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("IteratedString"), GetIteratedString());
    vec.emplace_back(CString("StringIteratorNextIndex"), JSTaggedValue(GetStringIteratorNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSTypedArray::DumpForSnapshot(std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 5;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("viewed-array-buffer"), GetViewedArrayBufferOrByteArray());
    vec.emplace_back(CString("typed-array-name"), GetTypedArrayName());
    vec.emplace_back(CString("byte-length"), JSTaggedValue(GetByteLength()));
    vec.emplace_back(CString("byte-offset"), JSTaggedValue(GetByteOffset()));
    vec.emplace_back(CString("array-length"), JSTaggedValue(GetArrayLength()));
    JSObject::DumpForSnapshot(vec);
}

void JSSharedTypedArray::DumpForSnapshot(std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 5;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("viewed-array-buffer"), GetViewedArrayBufferOrByteArray());
    vec.emplace_back(CString("typed-array-name"), GetTypedArrayName());
    vec.emplace_back(CString("byte-length"), JSTaggedValue(GetByteLength()));
    vec.emplace_back(CString("byte-offset"), JSTaggedValue(GetByteOffset()));
    vec.emplace_back(CString("array-length"), JSTaggedValue(GetArrayLength()));
    JSObject::DumpForSnapshot(vec);
}

void ByteArray::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("array-length"), JSTaggedValue(GetArrayLength()));
    vec.emplace_back(CString("byte-length"), JSTaggedValue(GetByteLength()));
}

void JSRegExp::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ByteCodeBuffer"), GetByteCodeBuffer());
    vec.emplace_back(CString("originalSource"), GetOriginalSource());
    vec.emplace_back(CString("originalFlags"), GetOriginalFlags());
    vec.emplace_back(CString("groupName"), GetGroupName());
    JSObject::DumpForSnapshot(vec);
}

void JSRegExpIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("IteratingRegExp"), GetIteratingRegExp());
    vec.emplace_back(CString("IteratedString"), GetIteratedString());
    vec.emplace_back(CString("Global"), JSTaggedValue(GetGlobal()));
    vec.emplace_back(CString("Unicode"), JSTaggedValue(GetUnicode()));
    vec.emplace_back(CString("Done"), JSTaggedValue(GetDone()));
    JSObject::DumpForSnapshot(vec);
}

void JSProxy::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("target"), GetTarget());
    vec.emplace_back(CString("handler"), GetHandler());
    vec.emplace_back(CString("Method"), GetMethod());
    vec.emplace_back(CString("PrivateField"), GetPrivateField());
    vec.emplace_back(CString("BitField"), JSTaggedValue(GetBitField()));
}

void JSSymbol::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("hash-field"), JSTaggedValue(GetHashField()));
    vec.emplace_back(CString("flags"), JSTaggedValue(GetFlags()));
    vec.emplace_back(CString("description"), GetDescription());
}

void AccessorData::DumpForSnapshot(std::vector<Reference> &vec) const
{
    if (GetClass()->GetObjectType() == JSType::INTERNAL_ACCESSOR) {
        vec.emplace_back(CString("getter"), JSTaggedValue(InternalAccessor::ConstCast(this)->GetGetter()));
        vec.emplace_back(CString("setter"), JSTaggedValue(InternalAccessor::ConstCast(this)->GetSetter()));
        return;
    }

    vec.emplace_back(CString("getter"), GetGetter());
    vec.emplace_back(CString("setter"), GetSetter());
}

void LexicalEnv::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DumpArrayClass(this, vec);
}

void SFunctionEnv::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DumpArrayClass(this, vec);
}

void SendableEnv::DumpForSnapshot(std::vector<Reference> &vec) const
{
    DumpArrayClass(this, vec);
}

void GlobalEnv::DumpForSnapshot(std::vector<Reference> &vec) const
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

void JSDataView::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("data-view"), GetDataView());
    vec.emplace_back(CString("buffer"), GetViewedArrayBuffer());
    vec.emplace_back(CString("byte-length"), JSTaggedValue(GetByteLength()));
    vec.emplace_back(CString("byte-offset"), JSTaggedValue(GetByteOffset()));
    JSObject::DumpForSnapshot(vec);
}

void JSArrayBuffer::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("buffer-data"), GetArrayBufferData());
    vec.emplace_back(CString("byte-length"), JSTaggedValue(GetArrayBufferByteLength()));
    vec.emplace_back(CString("shared"), JSTaggedValue(GetShared()));
    JSObject::DumpForSnapshot(vec);
}

void JSSendableArrayBuffer::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("buffer-data"), GetArrayBufferData());
    vec.emplace_back(CString("byte-length"), JSTaggedValue(GetArrayBufferByteLength()));
    vec.emplace_back(CString("shared"), JSTaggedValue(GetShared()));
    JSObject::DumpForSnapshot(vec);
}

void PromiseReaction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("promise-capability"), GetPromiseCapability());
    vec.emplace_back(CString("handler"), GetHandler());
    vec.emplace_back(CString("type"), JSTaggedValue(static_cast<int>(GetType())));
}

void PromiseCapability::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("promise"), GetPromise());
    vec.emplace_back(CString("resolve"), GetResolve());
    vec.emplace_back(CString("reject"), GetReject());
}

void PromiseIteratorRecord::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("iterator"), GetIterator());
    vec.emplace_back(CString("done"), JSTaggedValue(GetDone()));
}

void PromiseRecord::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("value"), GetValue());
}

void ResolvingFunctionsRecord::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("resolve-function"), GetResolveFunction());
    vec.emplace_back(CString("reject-function"), GetRejectFunction());
}

void AsyncGeneratorRequest::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("completion"), GetCompletion());
    vec.emplace_back(CString("capability"), GetCapability());
}

void AsyncIteratorRecord::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("iterator"), GetIterator());
    vec.emplace_back(CString("nextmethod"), GetNextMethod());
    vec.emplace_back(CString("done"), JSTaggedValue(GetDone()));
}

void JSAsyncFromSyncIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("synciteratorrecord"), GetSyncIteratorRecord());
    JSObject::DumpForSnapshot(vec);
}

void JSAsyncFromSyncIterUnwarpFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("done"), JSTaggedValue(GetDone()));
    JSFunction::DumpForSnapshot(vec);
}

void JSPromise::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("promise-state"), JSTaggedValue(static_cast<int>(GetPromiseState())));
    vec.emplace_back(CString("promise-result"), GetPromiseResult());
    vec.emplace_back(CString("promise-fulfill-reactions"), GetPromiseFulfillReactions());
    vec.emplace_back(CString("promise-reject-reactions"), GetPromiseRejectReactions());
    vec.emplace_back(CString("promise-is-handled"), JSTaggedValue(GetPromiseIsHandled()));
    JSObject::DumpForSnapshot(vec);
}

void JSPromiseReactionsFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("promise"), GetPromise());
    vec.emplace_back(CString("already-resolved"), GetAlreadyResolved());
    JSFunction::DumpForSnapshot(vec);
}

void JSAsyncGeneratorResNextRetProRstFtn::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("async-generator-object"), GetAsyncGeneratorObject());
    JSFunction::DumpForSnapshot(vec);
}

void JSPromiseExecutorFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("capability"), GetCapability());
    JSFunction::DumpForSnapshot(vec);
}

void JSAsyncModuleFulfilledFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("module"), GetModule());
    JSFunction::DumpForSnapshot(vec);
}

void JSAsyncModuleRejectedFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("module"), GetModule());
    JSFunction::DumpForSnapshot(vec);
}

void JSPromiseAllResolveElementFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("index"), GetIndex());
    vec.emplace_back(CString("values"), GetValues());
    vec.emplace_back(CString("capabilities"), GetCapabilities());
    vec.emplace_back(CString("remaining-elements"), GetRemainingElements());
    vec.emplace_back(CString("already-called"), GetAlreadyCalled());
    JSFunction::DumpForSnapshot(vec);
}

void JSPromiseAnyRejectElementFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("index"), JSTaggedValue(GetIndex()));
    vec.emplace_back(CString("errors"), GetErrors());
    vec.emplace_back(CString("capability"), GetCapability());
    vec.emplace_back(CString("remaining-elements"), GetRemainingElements());
    vec.emplace_back(CString("already-called"), GetAlreadyCalled());
    JSFunction::DumpForSnapshot(vec);
}

void JSPromiseAllSettledElementFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("already-called"), GetAlreadyCalled());
    vec.emplace_back(CString("index"), JSTaggedValue(GetIndex()));
    vec.emplace_back(CString("values"), GetValues());
    vec.emplace_back(CString("capability"), GetCapability());
    vec.emplace_back(CString("remaining-elements"), GetRemainingElements());
    JSFunction::DumpForSnapshot(vec);
}

void JSPromiseFinallyFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("constructor"), GetConstructor());
    vec.emplace_back(CString("onFinally"), GetOnFinally());
    JSFunction::DumpForSnapshot(vec);
}

void JSPromiseValueThunkOrThrowerFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("result"), GetResult());
    JSFunction::DumpForSnapshot(vec);
}

void MicroJobQueue::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("promise-job-queue"), GetPromiseJobQueue());
    vec.emplace_back(CString("script-job-queue"), GetScriptJobQueue());
}

void PendingJob::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("job"), GetJob());
    vec.emplace_back(CString("arguments"), GetArguments());
}

void CompletionRecord::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("value"), GetValue());
    vec.emplace_back(CString("type"), JSTaggedValue(static_cast<int>(GetType())));
}

void JSProxyRevocFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("RevocableProxy"), GetRevocableProxy());
    JSFunction::DumpForSnapshot(vec);
}

void JSAsyncFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSFunction::DumpForSnapshot(vec);
}

void JSAsyncAwaitStatusFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("AsyncContext"), GetAsyncContext());
    JSFunction::DumpForSnapshot(vec);
}

void JSGeneratorFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSFunction::DumpForSnapshot(vec);
}

void JSAsyncGeneratorFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    JSFunction::DumpForSnapshot(vec);
}

void JSIntlBoundFunction::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("NumberFormat"), GetNumberFormat());
    vec.emplace_back(CString("DateTimeFormat"), GetDateTimeFormat());
    vec.emplace_back(CString("Collator"), GetCollator());
    JSFunction::DumpForSnapshot(vec);
}

void PropertyBox::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Value"), GetValue());
}

void PrototypeHandler::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("HandlerInfo"), GetHandlerInfo());
    vec.emplace_back(CString("ProtoCell"), GetProtoCell());
    vec.emplace_back(CString("Holder"), GetHolder());
}

void TransitionHandler::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("HandlerInfo"), GetHandlerInfo());
    vec.emplace_back(CString("TransitionHClass"), GetTransitionHClass());
}

void TransWithProtoHandler::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("HandlerInfo"), GetHandlerInfo());
    vec.emplace_back(CString("TransitionHClass"), GetTransitionHClass());
    vec.emplace_back(CString("ProtoCell"), GetProtoCell());
}

void StoreAOTHandler::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("HandlerInfo"), GetHandlerInfo());
    vec.emplace_back(CString("ProtoCell"), GetProtoCell());
    vec.emplace_back(CString("Holder"), GetHolder());
}

void JSRealm::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Value"), GetValue());
    vec.emplace_back(CString("GLobalEnv"), GetGlobalEnv());
    JSObject::DumpForSnapshot(vec);
}
#ifdef ARK_SUPPORT_INTL
void JSIntl::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("FallbackSymbol"), GetFallbackSymbol());
    JSObject::DumpForSnapshot(vec);
}

void JSLocale::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("IcuField"), GetIcuField());
    JSObject::DumpForSnapshot(vec);
}

void JSDateTimeFormat::DumpForSnapshot(std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 11;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("Locale"), GetLocale());
    vec.emplace_back(CString("Calendar"), GetCalendar());
    vec.emplace_back(CString("NumberingSystem"), GetNumberingSystem());
    vec.emplace_back(CString("TimeZone"), GetTimeZone());
    vec.emplace_back(CString("HourCycle"), JSTaggedValue(static_cast<int>(GetHourCycle())));
    vec.emplace_back(CString("LocaleIcu"), GetLocaleIcu());
    vec.emplace_back(CString("SimpleDateTimeFormatIcu"), GetSimpleDateTimeFormatIcu());
    vec.emplace_back(CString("Iso8601"), GetIso8601());
    vec.emplace_back(CString("DateStyle"), JSTaggedValue(static_cast<int>(GetDateStyle())));
    vec.emplace_back(CString("TimeStyle"), JSTaggedValue(static_cast<int>(GetTimeStyle())));
    vec.emplace_back(CString("BoundFormat"), GetBoundFormat());
    JSObject::DumpForSnapshot(vec);
}

void JSRelativeTimeFormat::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Locale"), GetLocale());
    vec.emplace_back(CString("NumberingSystem"), GetNumberingSystem());
    vec.emplace_back(CString("Style"), JSTaggedValue(static_cast<int>(GetStyle())));
    vec.emplace_back(CString("Numeric"), JSTaggedValue(static_cast<int>(GetNumeric())));
    vec.emplace_back(CString("IcuField"), GetIcuField());
    JSObject::DumpForSnapshot(vec);
}

void JSNumberFormat::DumpForSnapshot(std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 20;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("Locale"), GetLocale());
    vec.emplace_back(CString("NumberingSystem"), GetNumberingSystem());
    vec.emplace_back(CString("Style"), JSTaggedValue(static_cast<int>(GetStyle())));
    vec.emplace_back(CString("Currency"), GetCurrency());
    vec.emplace_back(CString("CurrencyDisplay"), JSTaggedValue(static_cast<int>(GetCurrencyDisplay())));
    vec.emplace_back(CString("CurrencySign"), JSTaggedValue(static_cast<int>(GetCurrencySign())));
    vec.emplace_back(CString("Unit"), GetUnit());
    vec.emplace_back(CString("UnitDisplay"), JSTaggedValue(static_cast<int>(GetUnitDisplay())));
    vec.emplace_back(CString("MinimumIntegerDigits"), GetMinimumIntegerDigits());
    vec.emplace_back(CString("MinimumFractionDigits"), GetMinimumFractionDigits());
    vec.emplace_back(CString("MaximumFractionDigits"), GetMaximumFractionDigits());
    vec.emplace_back(CString("MinimumSignificantDigits"), GetMinimumSignificantDigits());
    vec.emplace_back(CString("MaximumSignificantDigits"), GetMaximumSignificantDigits());
    vec.emplace_back(CString("UseGrouping"), GetUseGrouping());
    vec.emplace_back(CString("RoundingType"), JSTaggedValue(static_cast<int>(GetRoundingType())));
    vec.emplace_back(CString("Notation"), JSTaggedValue(static_cast<int>(GetNotation())));
    vec.emplace_back(CString("CompactDisplay"), JSTaggedValue(static_cast<int>(GetCompactDisplay())));
    vec.emplace_back(CString("SignDisplay"), JSTaggedValue(static_cast<int>(GetSignDisplay())));
    vec.emplace_back(CString("BoundFormat"), GetBoundFormat());
    vec.emplace_back(CString("IcuField"), GetIcuField());
    JSObject::DumpForSnapshot(vec);
}

void JSCollator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 9;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("IcuField"), GetIcuField());
    vec.emplace_back(CString("Locale"), GetLocale());
    vec.emplace_back(CString("Collation"), GetCollation());
    vec.emplace_back(CString("BoundCompare"), GetBoundCompare());
    vec.emplace_back(CString("CaseFirst"), JSTaggedValue(static_cast<int>(GetCaseFirst())));
    vec.emplace_back(CString("Usage"), JSTaggedValue(static_cast<int>(GetUsage())));
    vec.emplace_back(CString("Sensitivity"), JSTaggedValue(static_cast<int>(GetSensitivity())));
    vec.emplace_back(CString("IgnorePunctuation"), JSTaggedValue(GetIgnorePunctuation()));
    vec.emplace_back(CString("Numeric"), JSTaggedValue(GetNumeric()));
    JSObject::DumpForSnapshot(vec);
}

void JSPluralRules::DumpForSnapshot(std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 10;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("Locale"), GetLocale());
    vec.emplace_back(CString("MinimumIntegerDigits"), GetMinimumIntegerDigits());
    vec.emplace_back(CString("MinimumFractionDigits"), GetMinimumFractionDigits());
    vec.emplace_back(CString("MaximumFractionDigits"), GetMaximumFractionDigits());
    vec.emplace_back(CString("MinimumSignificantDigits"), GetMinimumSignificantDigits());
    vec.emplace_back(CString("MaximumSignificantDigits"), GetMaximumSignificantDigits());
    vec.emplace_back(CString("RoundingType"), JSTaggedValue(static_cast<int>(GetRoundingType())));
    vec.emplace_back(CString("IcuPR"), GetIcuPR());
    vec.emplace_back(CString("IcuNF"), GetIcuNF());
    vec.emplace_back(CString("Type"), JSTaggedValue(static_cast<int>(GetType())));
    JSObject::DumpForSnapshot(vec);
}

void JSDisplayNames::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Locale"), GetLocale());
    vec.emplace_back(CString("Type"), JSTaggedValue(static_cast<int>(GetType())));
    vec.emplace_back(CString("Style"), JSTaggedValue(static_cast<int>(GetStyle())));
    vec.emplace_back(CString("Fallback"), JSTaggedValue(static_cast<int>(GetFallback())));
    vec.emplace_back(CString("IcuLDN"), GetIcuLDN());
    JSObject::DumpForSnapshot(vec);
}

void JSSegmenter::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Locale"), GetLocale());
    vec.emplace_back(CString("Granularity"), JSTaggedValue(static_cast<int>(GetGranularity())));
    vec.emplace_back(CString("IcuField"), GetIcuField());
    JSObject::DumpForSnapshot(vec);
}

void JSSegments::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("SegmentsString"), GetSegmentsString());
    vec.emplace_back(CString("UnicodeString"), GetUnicodeString());
    vec.emplace_back(CString("Granularity"), JSTaggedValue(static_cast<int>(GetGranularity())));
    vec.emplace_back(CString("IcuField"), GetIcuField());
    JSObject::DumpForSnapshot(vec);
}

void JSSegmentIterator::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("IteratedString"), GetIteratedString());
    vec.emplace_back(CString("UnicodeString"), GetUnicodeString());
    vec.emplace_back(CString("Granularity"), JSTaggedValue(static_cast<int>(GetGranularity())));
    vec.emplace_back(CString("IcuField"), GetIcuField());
    JSObject::DumpForSnapshot(vec);
}

void JSListFormat::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Locale"), GetLocale());
    vec.emplace_back(CString("Type"), JSTaggedValue(static_cast<int>(GetType())));
    vec.emplace_back(CString("Style"), JSTaggedValue(static_cast<int>(GetStyle())));
    vec.emplace_back(CString("IcuLF"), GetIcuLF());
    JSObject::DumpForSnapshot(vec);
}
#endif
void JSGeneratorObject::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("GeneratorContext"), GetGeneratorContext());
    vec.emplace_back(CString("ResumeResult"), GetResumeResult());
    vec.emplace_back(CString("GeneratorState"), JSTaggedValue(static_cast<int>(GetGeneratorState())));
    vec.emplace_back(CString("ResumeMode"), JSTaggedValue(static_cast<int>(GetResumeMode())));
    JSObject::DumpForSnapshot(vec);
}

void JSAsyncGeneratorObject::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("GeneratorContext"), GetGeneratorContext());
    vec.emplace_back(CString("AsyncGeneratorQueue"), GetAsyncGeneratorQueue());
    vec.emplace_back(CString("GeneratorBrand"), GetGeneratorBrand());
    vec.emplace_back(CString("ResumeResult"), GetResumeResult());
    vec.emplace_back(CString("AsyncGeneratorState"), JSTaggedValue(static_cast<int>(GetAsyncGeneratorState())));
    vec.emplace_back(CString("ResumeMode"), JSTaggedValue(static_cast<int>(GetResumeMode())));
    JSObject::DumpForSnapshot(vec);
}

void JSAsyncFuncObject::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Promise"), GetPromise());
    JSObject::DumpForSnapshot(vec);
}

void GeneratorContext::DumpForSnapshot(std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 8;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("RegsArray"), GetRegsArray());
    vec.emplace_back(CString("Method"), GetMethod());
    vec.emplace_back(CString("This"), GetThis());
    vec.emplace_back(CString("Acc"), GetAcc());
    vec.emplace_back(CString("GeneratorObject"), GetGeneratorObject());
    vec.emplace_back(CString("LexicalEnv"), GetLexicalEnv());
    vec.emplace_back(CString("NRegs"),  JSTaggedValue(GetNRegs()));
    vec.emplace_back(CString("BCOffset"),  JSTaggedValue(GetBCOffset()));
}

void EnumCache::DumpForSnapshot(std::vector<Reference> &vec) const
{
    constexpr int16_t NUM_OF_ITEMS = 4;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("EnumCacheOwn"), GetEnumCacheOwn());
    vec.emplace_back(CString("EnumCacheAll"), GetEnumCacheAll());
    vec.emplace_back(CString("ProtoChainInfoEnumCache"), GetProtoChainInfoEnumCache());
    vec.emplace_back(CString("EnumCacheKind"), JSTaggedValue(GetEnumCacheKind()));
}

void ProtoChangeMarker::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("HasChanged"), JSTaggedValue(GetHasChanged()));
}

void MarkerCell::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("IsDetectorInvalid"), JSTaggedValue(GetIsDetectorInvalid()));
}

void ProtoChangeDetails::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ChangeListener"), GetChangeListener());
    vec.emplace_back(CString("RegisterIndex"), JSTaggedValue(GetRegisterIndex()));
}

void MachineCode::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("InstructionSizeInBytes"), JSTaggedValue(GetInstructionSizeInBytes()));
}

void TrackInfo::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back("ElementsKind", JSTaggedValue(static_cast<uint32_t>(GetElementsKind())));

    vec.emplace_back(CString("CachedHClass"), GetCachedHClass());
    vec.emplace_back(CString("CachedFunc"), GetCachedFunc());
    vec.emplace_back(CString("ArrayLength"), JSTaggedValue(GetArrayLength()));
}

void ClassInfoExtractor::DumpForSnapshot(std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 6;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("NonStaticKeys"), GetNonStaticKeys());
    vec.emplace_back(CString("NonStaticProperties"), GetNonStaticProperties());
    vec.emplace_back(CString("NonStaticElements"), GetNonStaticElements());
    vec.emplace_back(CString("StaticKeys"), GetStaticKeys());
    vec.emplace_back(CString("StaticProperties"), GetStaticProperties());
    vec.emplace_back(CString("StaticElements"), GetStaticElements());
    vec.emplace_back(CString("ConstructorMethod"), GetConstructorMethod());
    vec.emplace_back(CString("BitField"), JSTaggedValue(GetBitField()));
}

void SourceTextModule::DumpForSnapshot(std::vector<Reference> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 14;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back(CString("Environment"), GetEnvironment());
    vec.emplace_back(CString("Namespace"), GetNamespace());
    vec.emplace_back(CString("ModuleRequests"), GetModuleRequests());
    vec.emplace_back(CString("RequestedModules"), GetRequestedModules());
    vec.emplace_back(CString("ImportEntries"), GetImportEntries());
    vec.emplace_back(CString("LocalExportEntries"), GetLocalExportEntries());
    vec.emplace_back(CString("IndirectExportEntries"), GetIndirectExportEntries());
    vec.emplace_back(CString("StarExportEntries"), GetStarExportEntries());
    vec.emplace_back(CString("Status"), JSTaggedValue(static_cast<int32_t>(GetStatus())));
    vec.emplace_back(CString("Exception"), GetException());
    vec.emplace_back(CString("DFSIndex"), JSTaggedValue(GetDFSIndex()));
    vec.emplace_back(CString("DFSAncestorIndex"), JSTaggedValue(GetDFSAncestorIndex()));
    vec.emplace_back(CString("NameDictionary"), GetNameDictionary());
    vec.emplace_back(CString("CycleRoot"), GetCycleRoot());
    vec.emplace_back(CString("TopLevelCapability"), GetTopLevelCapability());
    vec.emplace_back(CString("AsyncParentModules"), GetAsyncParentModules());
    vec.emplace_back(CString("SendableEnv"), GetSendableEnv());
    vec.emplace_back(CString("HasTLA"), JSTaggedValue(GetHasTLA()));
    vec.emplace_back(CString("AsyncEvaluatingOrdinal"), JSTaggedValue(GetAsyncEvaluatingOrdinal()));
    vec.emplace_back(CString("PendingAsyncDependencies"), JSTaggedValue(GetPendingAsyncDependencies()));
}

void ImportEntry::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ModuleRequestIndex"), JSTaggedValue(GetModuleRequestIndex()));
    vec.emplace_back(CString("ImportName"), GetImportName());
    vec.emplace_back(CString("LocalName"), GetLocalName());
}

void LocalExportEntry::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ExportName"), GetExportName());
    vec.emplace_back(CString("LocalName"), GetLocalName());
}

void IndirectExportEntry::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ExportName"), GetExportName());
    vec.emplace_back(CString("ModuleRequest"), JSTaggedValue(GetModuleRequestIndex()));
    vec.emplace_back(CString("ImportName"), GetImportName());
}

void StarExportEntry::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ModuleRequest"), JSTaggedValue(GetModuleRequestIndex()));
}

void ResolvedBinding::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Module"), GetModule());
    vec.emplace_back(CString("BindingName"), GetBindingName());
}

void ResolvedIndexBinding::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Module"), GetModule());
    vec.emplace_back(CString("Index"), JSTaggedValue(GetIndex()));
}

void ResolvedRecordIndexBinding::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ModuleRecord"), GetModuleRecord());
    vec.emplace_back(CString("AbcFileName"), GetAbcFileName());
    vec.emplace_back(CString("Index"), JSTaggedValue(GetIndex()));
}

void ResolvedRecordBinding::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ModuleRecord"), GetModuleRecord());
    vec.emplace_back(CString("BindingName"), GetBindingName());
}

void ModuleNamespace::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Module"), GetModule());
    vec.emplace_back(CString("Exports"), GetExports());
    vec.emplace_back(CString("DeregisterProcession"), GetDeregisterProcession());
    JSObject::DumpForSnapshot(vec);
}

void NativeModuleFailureInfo::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("ArkNativeModuleFailureInfo"), GetArkNativeModuleFailureInfo());
    JSObject::DumpForSnapshot(vec);
}

void CjsModule::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Id"), GetId());
    vec.emplace_back(CString("Path"), GetPath());
    vec.emplace_back(CString("Exports"), GetExports());
    vec.emplace_back(CString("Filename"), GetFilename());
    vec.emplace_back(CString("BitField"), JSTaggedValue(GetBitField()));
    JSObject::DumpForSnapshot(vec);
}

void CjsExports::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Exports"), GetExports());
    JSObject::DumpForSnapshot(vec);
}

void CjsRequire::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Cache"), GetCache());
    vec.emplace_back(CString("Parent"), GetParent());
    JSObject::DumpForSnapshot(vec);
}

void ClassLiteral::DumpForSnapshot(std::vector<Reference> &vec) const
{
    vec.emplace_back(CString("Array"), GetArray());
    vec.emplace_back(CString("IsAOTUsed"), JSTaggedValue(GetIsAOTUsed()));
    if (!GetArray().IsUndefined()) {
        DumpArrayClass(TaggedArray::Cast(GetArray().GetTaggedObject()), vec);
    }
}
}  // namespace panda::ecmascript
