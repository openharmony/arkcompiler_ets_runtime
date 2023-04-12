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

#include <codecvt>
#include <iomanip>
#include <iostream>
#include <string>

#include "ecmascript/accessor_data.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/interpreter/frame_handler.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jobs/pending_job.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/class_literal.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_api/js_api_deque_iterator.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_api/js_api_hashmap_iterator.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_hashset_iterator.h"
#include "ecmascript/js_api/js_api_lightweightmap.h"
#include "ecmascript/js_api/js_api_lightweightmap_iterator.h"
#include "ecmascript/js_api/js_api_lightweightset.h"
#include "ecmascript/js_api/js_api_lightweightset_iterator.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_linked_list_iterator.h"
#include "ecmascript/js_api/js_api_list.h"
#include "ecmascript/js_api/js_api_list_iterator.h"
#include "ecmascript/js_api/js_api_plain_array.h"
#include "ecmascript/js_api/js_api_plain_array_iterator.h"
#include "ecmascript/js_api/js_api_queue.h"
#include "ecmascript/js_api/js_api_queue_iterator.h"
#include "ecmascript/js_api/js_api_stack.h"
#include "ecmascript/js_api/js_api_stack_iterator.h"
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_api/js_api_tree_map_iterator.h"
#include "ecmascript/js_api/js_api_tree_set.h"
#include "ecmascript/js_api/js_api_tree_set_iterator.h"
#include "ecmascript/js_api/js_api_vector.h"
#include "ecmascript/js_api/js_api_vector_iterator.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_async_from_sync_iterator.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_global_object.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_promise.h"
#include "ecmascript/js_realm.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/js_regexp_iterator.h"
#include "ecmascript/js_relative_time_format.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_tagged_number.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/js_weak_ref.h"
#include "ecmascript/layout_info-inl.h"
#include "ecmascript/lexical_env.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/module/js_module_namespace.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/require/js_cjs_module_cache.h"
#include "ecmascript/require/js_cjs_require.h"
#include "ecmascript/require/js_cjs_exports.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tagged_hash_array.h"
#include "ecmascript/tagged_list.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/template_map.h"
#include "ecmascript/transitions_dictionary.h"
#include "ecmascript/ts_types/ts_type.h"
#include "ecmascript/js_displaynames.h"
#include "ecmascript/js_list_format.h"

namespace panda::ecmascript {
using MicroJobQueue = panda::ecmascript::job::MicroJobQueue;
using PendingJob = panda::ecmascript::job::PendingJob;

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
        case JSType::TAGGED_DICTIONARY:
            return "TaggedDictionary";
        case JSType::CONSTANT_POOL:
            return "ConstantPool";
        case JSType::COW_TAGGED_ARRAY:
            return "COWArray";
        case JSType::STRING:
            return "BaseString";
        case JSType::JS_NATIVE_POINTER:
            return "NativePointer";
        case JSType::JS_OBJECT:
            return "Object";
        case JSType::JS_FUNCTION_BASE:
            return "Function Base";
        case JSType::JS_FUNCTION:
            return "Function";
        case JSType::JS_ERROR:
            return "Error";
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
        case JSType::JS_REG_EXP:
            return "Regexp";
        case JSType::JS_SET:
            return "Set";
        case JSType::JS_MAP:
            return "Map";
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
        case JSType::JS_SET_ITERATOR:
            return "SetIterator";
        case JSType::JS_ARRAY_ITERATOR:
            return "ArrayIterator";
        case JSType::JS_STRING_ITERATOR:
            return "StringIterator";
        case JSType::JS_REG_EXP_ITERATOR:
            return "RegExpIterator";
        case JSType::JS_ARRAY_BUFFER:
            return "ArrayBuffer";
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
        case JSType::JS_LIST_FORMAT:
            return "JSListFormat";
        case JSType::JS_GENERATOR_OBJECT:
            return "JSGeneratorObject";
        case JSType::JS_ASYNC_GENERATOR_OBJECT:
            return "JSAsyncGeneratorObject";
        case JSType::JS_GENERATOR_CONTEXT:
            return "JSGeneratorContext";
        case JSType::PROTO_CHANGE_MARKER:
            return "ProtoChangeMarker";
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
        case JSType::TS_OBJECT_TYPE:
            return "TSObjectType";
        case JSType::TS_CLASS_TYPE:
            return "TSClassType";
        case JSType::TS_INTERFACE_TYPE:
            return "TSInterfaceType";
        case JSType::TS_CLASS_INSTANCE_TYPE:
            return "TSClassInstanceType";
        case JSType::TS_UNION_TYPE:
            return "TSUnionType";
        case JSType::TS_FUNCTION_TYPE:
            return "TSFunctionType";
        case JSType::TS_ARRAY_TYPE:
            return "TSArrayType";
        case JSType::TS_ITERATOR_INSTANCE_TYPE:
            return "TSIteratorInstanceType";
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
        default: {
            CString ret = "unknown type ";
            return ret + static_cast<char>(type);
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
        DumpStringClass(EcmaString::Cast(sym->GetDescription().GetTaggedObject()), os);
    } else {
        UNREACHABLE();
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
        LayoutInfo *layoutInfo = LayoutInfo::Cast(attrs.GetTaggedObject());
        layoutInfo->Dump(os);
    }
    os << " - Transitions :" << std::setw(DUMP_TYPE_OFFSET);
    JSTaggedValue transtions = jshclass->GetTransitions();
    transtions.DumpTaggedValue(os);
    os << "\n";
    if (withDetail && !transtions.IsNull()) {
        transtions.Dump(os);
    }

    os << " - Flags : " << std::setw(DUMP_TYPE_OFFSET);
    os << "IsCtor :" << std::boolalpha << jshclass->IsConstructor();
    os << "| IsCallable :" << std::boolalpha << jshclass->IsCallable();
    os << "| IsExtensible :" << std::boolalpha << jshclass->IsExtensible();
    os << "| ElementRepresentation :" << static_cast<int>(jshclass->GetElementRepresentation());
    os << "| NumberOfProps :" << std::dec << jshclass->NumberOfProps();
    os << "| InlinedProperties :" << std::dec << jshclass->GetInlinedProperties();
    os << "| IsTS :" << std::boolalpha << jshclass->IsTS();
    os << "\n";
}

static void DumpClass(TaggedObject *obj, std::ostream &os)
{
    JSHClass *hclass = obj->GetClass();
    os << "JSHClass :" << std::setw(DUMP_TYPE_OFFSET) << " klass_(" << std::hex << hclass << ")\n";
    DumpHClass(hclass, os, true);
}

static void DumpAttr(const PropertyAttributes &attr, bool fastMode, std::ostream &os)
{
    if (attr.IsAccessor()) {
        os << "(Accessor) ";
    }

    os << "Attr(";
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

    os << " InlinedProps: " << attr.IsInlinedProps();

    if (fastMode) {
        os << " Order: " << std::dec << attr.GetOffset();
        os << " SortedIndex: " << std::dec << attr.GetSortedIndex();
    } else {
        os << " Order: " << std::dec << attr.GetDictionaryOrder();
    }
}

static void DumpObject(TaggedObject *obj, std::ostream &os)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto jsHclass = obj->GetClass();
    JSType type = jsHclass->GetObjectType();

    switch (type) {
        case JSType::HCLASS:
            return DumpClass(obj, os);
        case JSType::TAGGED_ARRAY:
        case JSType::TAGGED_DICTIONARY:
        case JSType::TEMPLATE_MAP:
        case JSType::LEXICAL_ENV:
        case JSType::COW_TAGGED_ARRAY:
            DumpArrayClass(TaggedArray::Cast(obj), os);
            break;
        case JSType::CONSTANT_POOL:
            DumpConstantPoolClass(ConstantPool::Cast(obj), os);
            break;
        case JSType::STRING:
            DumpStringClass(EcmaString::Cast(obj), os);
            os << "\n";
            break;
        case JSType::JS_NATIVE_POINTER:
            break;
        case JSType::JS_OBJECT:
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
        case JSType::JS_ARGUMENTS:
            JSObject::Cast(obj)->Dump(os);
            break;
        case JSType::JS_FUNCTION_BASE:
            JSFunctionBase::Cast(obj)->Dump(os);
            break;
        case JSType::GLOBAL_ENV:
            GlobalEnv::Cast(obj)->Dump(os);
            break;
        case JSType::ACCESSOR_DATA:
            break;
        case JSType::JS_FUNCTION:
            JSFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_BOUND_FUNCTION:
            JSBoundFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_SET:
            JSSet::Cast(obj)->Dump(os);
            break;
        case JSType::JS_MAP:
            JSMap::Cast(obj)->Dump(os);
            break;
        case JSType::JS_WEAK_SET:
            JSWeakSet::Cast(obj)->Dump(os);
            break;
        case JSType::JS_WEAK_MAP:
            JSWeakMap::Cast(obj)->Dump(os);
            break;
        case JSType::JS_WEAK_REF:
            JSWeakRef::Cast(obj)->Dump(os);
            break;
        case JSType::JS_FINALIZATION_REGISTRY:
            JSFinalizationRegistry::Cast(obj)->Dump(os);
            break;
        case JSType::CELL_RECORD:
            CellRecord::Cast(obj)->Dump(os);
            break;
        case JSType::JS_REG_EXP:
            JSRegExp::Cast(obj)->Dump(os);
            break;
        case JSType::JS_DATE:
            JSDate::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ARRAY:
            JSArray::Cast(obj)->Dump(os);
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
            JSTypedArray::Cast(obj)->Dump(os);
            break;
        case JSType::BIGINT:
            BigInt::Cast(obj)->Dump(os);
            break;
        case JSType::BYTE_ARRAY:
            ByteArray::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PROXY:
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
            JSPromise::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PROMISE_REACTIONS_FUNCTION:
            JSPromiseReactionsFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PROMISE_EXECUTOR_FUNCTION:
            JSPromiseExecutorFunction::Cast(obj)->Dump(os);
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
            JSAsyncFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION:
            JSAsyncAwaitStatusFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_GENERATOR_FUNCTION:
            JSGeneratorFunction::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ASYNC_GENERATOR_FUNCTION:
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
        case JSType::JS_SET_ITERATOR:
            JSSetIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_REG_EXP_ITERATOR:
            JSRegExpIterator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ARRAY_ITERATOR:
            JSArrayIterator::Cast(obj)->Dump(os);
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
            StoreTSHandler::Cast(obj)->Dump(os);
            break;
        case JSType::PROPERTY_BOX:
            PropertyBox::Cast(obj)->Dump(os);
            break;
        case JSType::JS_REALM:
            JSRealm::Cast(obj)->Dump(os);
            break;
        case JSType::JS_INTL:
            JSIntl::Cast(obj)->Dump(os);
            break;
        case JSType::JS_LOCALE:
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
            JSCollator::Cast(obj)->Dump(os);
            break;
        case JSType::JS_PLURAL_RULES:
            JSPluralRules::Cast(obj)->Dump(os);
            break;
        case JSType::JS_DISPLAYNAMES:
            JSDisplayNames::Cast(obj)->Dump(os);
            break;
        case JSType::JS_LIST_FORMAT:
            JSListFormat::Cast(obj)->Dump(os);
            break;
        case JSType::JS_GENERATOR_OBJECT:
            JSGeneratorObject::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ASYNC_GENERATOR_OBJECT:
            JSAsyncGeneratorObject::Cast(obj)->Dump(os);
            break;
        case JSType::JS_ASYNC_FUNC_OBJECT:
            JSAsyncFuncObject::Cast(obj)->Dump(os);
            break;
        case JSType::JS_GENERATOR_CONTEXT:
            GeneratorContext::Cast(obj)->Dump(os);
            break;
        case JSType::PROTOTYPE_INFO:
            ProtoChangeDetails::Cast(obj)->Dump(os);
            break;
        case JSType::PROTO_CHANGE_MARKER:
            ProtoChangeMarker::Cast(obj)->Dump(os);
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
        case JSType::TS_OBJECT_TYPE:
            TSObjectType::Cast(obj)->Dump(os);
            break;
        case JSType::TS_CLASS_TYPE:
            TSClassType::Cast(obj)->Dump(os);
            break;
        case JSType::TS_INTERFACE_TYPE:
            TSInterfaceType::Cast(obj)->Dump(os);
            break;
        case JSType::TS_CLASS_INSTANCE_TYPE:
            TSClassInstanceType::Cast(obj)->Dump(os);
            break;
        case JSType::TS_UNION_TYPE:
            TSUnionType::Cast(obj)->Dump(os);
            break;
        case JSType::TS_FUNCTION_TYPE:
            TSFunctionType::Cast(obj)->Dump(os);
            break;
        case JSType::TS_ARRAY_TYPE:
            TSArrayType::Cast(obj)->Dump(os);
            break;
        case JSType::TS_ITERATOR_INSTANCE_TYPE:
            TSIteratorInstanceType::Cast(obj)->Dump(os);
            break;
        case JSType::LINKED_NODE:
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
            UNREACHABLE();
            break;
    }

    DumpHClass(jsHclass, os, false);
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
        os << "----------Dump Weak Referent----------"
           << "\n";
    }

    JSType type = obj->GetClass()->GetObjectType();
    if (type == JSType::STRING) {
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

void JSTaggedValue::Dump(std::ostream &os) const
{
    DumpTaggedValue(os);
    os << "\n";

    if (IsHeapObject()) {
        TaggedObject *obj = IsWeak() ? GetTaggedWeakRef() : GetTaggedObject();
        DumpObject(obj, os);
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

void NumberDictionary::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            JSTaggedValue val(GetValue(hashIndex));
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET)
               << static_cast<uint32_t>(JSTaggedNumber(key).GetNumber()) << ": ";
            val.DumpTaggedValue(os);
            os << " ";
            DumpAttr(GetAttributes(hashIndex), false, os);
            os << "\n";
        }
    }
}

void NameDictionary::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            JSTaggedValue val(GetValue(hashIndex));
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            DumpPropertyKey(key, os);
            os << ": ";
            val.DumpTaggedValue(os);
            os << " ";
            DumpAttr(GetAttributes(hashIndex), false, os);
            os << "\n";
        }
    }
}

void GlobalDictionary::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole()) {
            JSTaggedValue val(GetValue(hashIndex));
            os << std::right << std::setw(DUMP_PROPERTY_OFFSET);
            DumpPropertyKey(key, os);
            os << " : ";
            val.DumpTaggedValue(os);
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
    GetElement(5).D();
    os << "head-pre: ";
    // 6 : 6 first element per ptr
    GetElement(6).D();
    os << "\n";
    int i = 0;
    int next = GetElement(5).GetInt();
    while (capacity > i) {
        os << " value: ";
        GetElement(next).DumpTaggedValue(os);
        os << " next: ";
        // 1 : 1 current element next ptr offset
        GetElement(next + 1).D();
        os << " pre: ";
        // 2 : 2 current element pre ptr offset
        GetElement(next + 2).D();
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
    GetElement(5).D();
    os << "\n";
    int i = 0;
    int next = GetElement(5).GetInt();
    while (capacity > i) {
        os << " value: ";
        GetElement(next).DumpTaggedValue(os);
        os << " next: ";
        // 1 : 1 current element next ptr offset
        GetElement(next + 1).D();
        os << "\n";
        next = GetElement(next + 1).GetInt();
        i++;
    }
}

void JSObject::Dump(std::ostream &os) const
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHClass *jshclass = GetJSHClass();
    os << " - hclass: " << std::hex << jshclass << "\n";
    os << " - prototype: ";
    jshclass->GetPrototype().DumpTaggedValue(os);
    os << "\n";

    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    os << " - elements: " << std::hex << elements;
    if (elements->GetLength() == 0) {
        os << " NONE\n";
    } else if (!elements->IsDictionaryMode()) {
        DumpArrayClass(elements, os);
    } else {
        NumberDictionary *dict = NumberDictionary::Cast(elements);
        os << " <NumberDictionary[" << std::dec << dict->EntriesCount() << "]>\n";
        dict->Dump(os);
    }

    TaggedArray *properties = TaggedArray::Cast(GetProperties().GetTaggedObject());
    os << " - properties: " << std::hex << properties;
    if (IsJSGlobalObject()) {
        GlobalDictionary *dict = GlobalDictionary::Cast(properties);
        os << " <GlobalDictionary[" << std::dec << dict->EntriesCount() << "]>\n";
        dict->Dump(os);
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
                val = GetPropertyInlinedProps(i);
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

void TaggedArray::Dump(std::ostream &os) const
{
    DumpArrayClass(this, os);
}

void AccessorData::Dump(std::ostream &os) const
{
    auto *hclass = GetClass();
    if (hclass->GetObjectType() == JSType::INTERNAL_ACCESSOR) {
        os << " - Getter: " << reinterpret_cast<void *>(GetGetter().GetTaggedObject()) << "\n";
        os << " - Setter: " << reinterpret_cast<void *>(GetSetter().GetTaggedObject()) << "\n";
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

void ConstantPool::Dump(std::ostream &os) const
{
    DumpArrayClass(this, os);
}

void JSFunction::Dump(std::ostream &os) const
{
    os << " - ProtoOrHClass: ";
    GetProtoOrHClass().Dump(os);
    os << "\n";
    os << " - LexicalEnv: ";
    GetLexicalEnv().Dump(os);
    os << "\n";
    os << " - HomeObject: ";
    GetHomeObject().Dump(os);
    os << "\n";
    os << " - FunctionExtraInfo: ";
    GetFunctionExtraInfo().Dump(os);
    os << "\n";
    os << " - Module: ";
    GetModule().Dump(os);
    os << "\n";
    os << " - Method: ";
    GetMethod().Dump(os);
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

void JSAPITreeMap::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    TaggedTreeMap *map = TaggedTreeMap::Cast(GetTreeMap().GetTaggedObject());
    map->DumpForSnapshot(vec);

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

void JSAPITreeMapIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    TaggedTreeMap *map =
        TaggedTreeMap::Cast(JSAPITreeMap::Cast(GetIteratedMap().GetTaggedObject())->GetTreeMap().GetTaggedObject());
    map->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
    vec.emplace_back("IterationKind", JSTaggedValue(static_cast<int>(GetIterationKind())));
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

    int capacity = NumberOfElements() + NumberOfDeletedElements();
    for (int index = 0; index < capacity; index++) {
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

void JSAPITreeSet::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    TaggedTreeSet *set = TaggedTreeSet::Cast(GetTreeSet().GetTaggedObject());
    set->DumpForSnapshot(vec);

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

void JSAPITreeSetIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    TaggedTreeSet *set =
        TaggedTreeSet::Cast(JSAPITreeSet::Cast(GetIteratedSet().GetTaggedObject())->GetTreeSet().GetTaggedObject());
    set->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
    vec.emplace_back("IterationKind", JSTaggedValue(static_cast<int>(GetIterationKind())));
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

    int capacity = NumberOfElements() + NumberOfDeletedElements();
    for (int index = 0; index < capacity; index++) {
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

void JSAPIPlainArray::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPIPlainArrayIterator::Dump(std::ostream &os) const
{
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(GetIteratedPlainArray().GetTaggedObject());
    os << " - length: " << std::dec << array->GetSize() << "\n";
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    JSObject::Dump(os);
}

void JSAPIPlainArrayIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSAPIPlainArray *array = JSAPIPlainArray::Cast(GetIteratedPlainArray().GetTaggedObject());
    array->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSForInIterator::Dump(std::ostream &os) const
{
    os << " - Object : ";
    GetObject().DumpTaggedValue(os);
    os << "\n";
    os << " - WasVisited : " << GetWasVisited();
    os << "\n";
    os << " - VisitedKeys : ";
    GetVisitedKeys().DumpTaggedValue(os);
    os << "\n";
    os << " - RemainingKeys : ";
    GetRemainingKeys().DumpTaggedValue(os);
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
    GetNoUnregister().D();
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

void JSRegExpIterator::Dump(std::ostream &os) const
{
    os << " - IteratingRegExp: ";
    GetIteratingRegExp().D();
    os << "\n";
    os << " - IteratedString: ";
    GetIteratedString().D();
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

void JSArrayIterator::Dump(std::ostream &os) const
{
    JSArray *array = JSArray::Cast(GetIteratedArray().GetTaggedObject());
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
    int capacity = GetSize();
    os << " - length: " << std::dec << capacity << "\n";
    int i = 0;
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

void JSAPIHashMap::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPIHashSet::Dump(std::ostream &os) const
{
    TaggedHashArray *hashArray = TaggedHashArray::Cast(GetTable().GetTaggedObject());
    os << " - elements: " << std::dec << GetSize() << "\n";
    os << " - table capacity: " << std::dec << static_cast<int>(hashArray->GetLength()) << "\n";
    JSObject::Dump(os);
}

void JSAPIHashSet::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPIHashMapIterator::Dump(std::ostream &os) const
{
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(os);
}

void JSAPIHashMapIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPIHashSetIterator::Dump(std::ostream &os) const
{
    os << " - nextIndex: " << std::dec << GetNextIndex() << "\n";
    os << " - IterationKind: " << std::dec << static_cast<int>(GetIterationKind()) << "\n";
    JSObject::Dump(os);
}

void JSAPIHashSetIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
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
    JSObject::Dump(os);
    list->Dump(os);
}

void JSAPIList::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    TaggedSingleList *map = TaggedSingleList::Cast(GetSingleList().GetTaggedObject());
    map->DumpForSnapshot(vec);

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

void JSAPIListIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    TaggedSingleList *list = TaggedSingleList::Cast(GetIteratedList().GetTaggedObject());
    list->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
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

void JSAPILinkedList::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    TaggedDoubleList *map = TaggedDoubleList::Cast(GetDoubleList().GetTaggedObject());
    map->DumpForSnapshot(vec);

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

void JSAPILinkedListIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    TaggedDoubleList *linkedList = TaggedDoubleList::Cast(GetIteratedLinkedList().GetTaggedObject());
    linkedList->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
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
    GetViewedArrayBuffer().Dump(os);
    os << " - typed-array-name: ";
    GetTypedArrayName().Dump(os);
    os << " - byte-length: " << GetByteLength();
    os << " - byte-offset: " << GetByteOffset();
    os << " - array-length: " << GetArrayLength();
    JSObject::Dump(os);
}

void ByteArray::Dump(std::ostream &os) const
{
    os << " - length: " << GetLength();
    os << " - size: " << GetSize();
}

void JSRegExp::Dump(std::ostream &os) const
{
    os << "\n";
    os << " - ByteCodeBuffer: ";
    GetByteCodeBuffer().D();
    os << "\n";
    os << " - OriginalSource: ";
    GetOriginalSource().D();
    os << "\n";
    os << " - OriginalFlags: ";
    GetOriginalFlags().D();
    os << "\n";
    os << " - GroupName: ";
    GetGroupName().D();
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

void COWTaggedArray::Dump(std::ostream &os) const
{
    DumpArrayClass(this, os);
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
    os << " - AttachSymbol: ";
    GetAttachSymbol().GetTaggedValue().Dump(os);
    os << " - DetachSymbol: ";
    GetDetachSymbol().GetTaggedValue().Dump(os);
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
    os << " - NumberFormat: ";
    GetNumberFormat().Dump(os);
    os << "\n";
    os << " - DateTimeFormat: ";
    GetDateTimeFormat().Dump(os);
    os << "\n";
    os << " - Collator: ";
    GetCollator().Dump(os);
    os << "\n";
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
    GetHandlerInfo().Dump(os);
    os << "\n";
    os << " - Holder: ";
    GetHandlerInfo().Dump(os);
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
    os << " - Holder: ";
    GetHandlerInfo().Dump(os);
    os << "\n";
}

void StoreTSHandler::Dump(std::ostream &os) const
{
    os << " - HandlerInfo: ";
    GetHandlerInfo().Dump(os);
    os << "\n";
    os << " - ProtoCell: ";
    GetHandlerInfo().Dump(os);
    os << "\n";
    os << " - Holder: ";
    GetHandlerInfo().Dump(os);
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

void ProtoChangeMarker::Dump(std::ostream &os) const
{
    os << " - HasChanged: " << GetHasChanged() << "\n";
}

void ProtoChangeDetails::Dump(std::ostream &os) const
{
    os << " - ChangeListener: ";
    GetChangeListener().Dump(os);
    os << "\n";
    os << " - RegisterIndex: " << GetRegisterIndex();
    os << "\n";
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

void TSObjectType::Dump(std::ostream &os) const
{
    os << " - TSObjectType globalTSTypeRef: ";
    GlobalTSTypeRef gt = GetGT();
    uint64_t globalTSTypeRef = gt.GetType();
    os << globalTSTypeRef;
    os << "\n";
    os << " - TSObjectType moduleId: ";
    uint32_t moduleId = gt.GetModuleId();
    os << moduleId;
    os << "\n";
    os << " - TSObjectType localTypeId: ";
    uint32_t localTypeId = gt.GetLocalId();
    os << localTypeId;
    os << "\n";
    os << "  - ObjLayoutInfo: ";
    DumpArrayClass(TaggedArray::Cast(GetObjLayoutInfo().GetTaggedObject()), os);
    os << "  - HClass: ";
    GetHClass().Dump(os);
}

void TSClassType::Dump(std::ostream &os) const
{
    os << " - Dump TSClassType - " << "\n";
    os << " - TSClassType globalTSTypeRef: ";
    GlobalTSTypeRef gt = GetGT();
    uint64_t globalTSTypeRef = gt.GetType();
    os << globalTSTypeRef;
    os << "\n";
    os << " - TSClassType moduleId: ";
    uint32_t moduleId = gt.GetModuleId();
    os << moduleId;
    os << "\n";
    os << " - TSClassType localTypeId: ";
    uint32_t localTypeId = gt.GetLocalId();
    os << localTypeId;
    os << "\n";
    os << " - ExtensionTypeGT: ";
    GlobalTSTypeRef extensionGT = GetExtensionGT();
    if (extensionGT.IsDefault()) {
        os << " (base class type) ";
    } else {
        os << extensionGT.GetType();
    }
    os << "\n";

    CString hasLinked = GetHasLinked() ? "true" : "false";
    os << " - HasLinked: " << hasLinked  << "\n";

    os << " - InstanceType: " << "\n";
    if (GetInstanceType().IsTSObjectType()) {
        TSObjectType *instanceType = TSObjectType::Cast(GetInstanceType().GetTaggedObject());
        instanceType->Dump(os);
        os << "\n";
    }

    os << " - ConstructorType: " << "\n";
    if (GetConstructorType().IsTSObjectType()) {
        TSObjectType *constructorType = TSObjectType::Cast(GetConstructorType().GetTaggedObject());
        constructorType->Dump(os);
        os << "\n";
    }

    os << " - PrototypeType: " << "\n";
    if (GetPrototypeType().IsTSObjectType()) {
        TSObjectType *prototypeType = TSObjectType::Cast(GetPrototypeType().GetTaggedObject());
        prototypeType->Dump(os);
        os << "\n";
    }
}

void TSInterfaceType::Dump(std::ostream &os) const
{
    os << " - Dump Interface Type - " << "\n";
    os << " - TSInterfaceType globalTSTypeRef: ";
    GlobalTSTypeRef gt = GetGT();
    uint64_t globalTSTypeRef = gt.GetType();
    os << globalTSTypeRef;
    os << "\n";
    os << " - TSInterfaceType moduleId: ";
    uint32_t moduleId = gt.GetModuleId();
    os << moduleId;
    os << "\n";
    os << " - TSInterfaceType localTypeId: ";
    uint32_t localTypeId = gt.GetLocalId();
    os << localTypeId;
    os << "\n";
    os << " - Extends TypeId: " << "\n";
    if (TaggedArray::Cast(GetExtends().GetTaggedObject())->GetLength() == 0) {
        os << " (base interface type) "<< "\n";
    }
    DumpArrayClass(TaggedArray::Cast(GetExtends().GetTaggedObject()), os);

    os << " - Fields: " << "\n";
    if (GetFields().IsTSObjectType()) {
        TSObjectType *fieldsType = TSObjectType::Cast(GetFields().GetTaggedObject());
        fieldsType->Dump(os);
        os << "\n";
    }
}

void TSClassInstanceType::Dump(std::ostream &os) const
{
    os << " - Dump ClassInstance Type - " << "\n";
    os << " - TSClassInstanceType globalTSTypeRef: ";
    GlobalTSTypeRef gt = GetGT();
    uint64_t globalTSTypeRef = gt.GetType();
    os << globalTSTypeRef;
    os << "\n";
    os << " - TSClassInstanceType moduleId: ";
    uint32_t moduleId = gt.GetModuleId();
    os << moduleId;
    os << "\n";
    os << " - TSClassInstanceType localTypeId: ";
    uint32_t localTypeId = gt.GetLocalId();
    os << localTypeId;
    os << "\n";

    os << " - createClassType GT: ";
    GlobalTSTypeRef createClassTypeGT = GetClassGT();
    os << createClassTypeGT.GetType();
    os << "\n";
}

void TSUnionType::Dump(std::ostream &os) const
{
    os << " - Dump UnionType Type - " << "\n";
    os << " - TSUnionType globalTSTypeRef: ";
    GlobalTSTypeRef gt = GetGT();
    uint64_t globalTSTypeRef = gt.GetType();
    os << globalTSTypeRef;
    os << "\n";
    os << " - TSUnionType moduleId: ";
    uint32_t moduleId = gt.GetModuleId();
    os << moduleId;
    os << "\n";
    os << " - TSUnionType localTypeId: ";
    uint32_t localTypeId = gt.GetLocalId();
    os << localTypeId;
    os << "\n";
    os << " - TSUnionType TypeId: " << "\n";
    DumpArrayClass(TaggedArray::Cast(GetComponents().GetTaggedObject()), os);
}

void TSFunctionType::Dump(std::ostream &os) const
{
    os << " - Dump TSFunctionType - " << "\n";
    os << " - TSFunctionType globalTSTypeRef: ";
    GlobalTSTypeRef gt = GetGT();
    uint64_t globalTSTypeRef = gt.GetType();
    os << globalTSTypeRef;
    os << "\n";
    os << " - TSFunctionType moduleId: ";
    uint32_t moduleId = gt.GetModuleId();
    os << moduleId;
    os << "\n";
    os << " - TSFunctionType localTypeId: ";
    uint32_t localTypeId = gt.GetLocalId();
    os << localTypeId;
    os << "\n";
    os << " - TSFunctionType Name: ";
    JSTaggedValue name = GetName();
    if (name.IsString()) {
        os << ConvertToString(EcmaString::Cast(name.GetTaggedObject()));
    }
    os << "\n";
    os << " - TSFunctionType ParameterTypeIds: " << "\n";
    DumpArrayClass(TaggedArray::Cast(GetParameterTypes().GetTaggedObject()), os);
    os << " - TSFunctionType ReturnType: " << GetReturnGT().GetType() << "\n";
    os << " - TSFunctionType ThisType: " << GetThisGT().GetType() << "\n";
    TSFunctionType::Visibility visibility = GetVisibility();
    switch (visibility) {
        case TSFunctionType::Visibility::PUBLIC:
            os << " - Visibility: public";
            break;
        case TSFunctionType::Visibility::PRIVATE:
            os << " - Visibility: private";
            break;
        case TSFunctionType::Visibility::PROTECTED:
            os << " - Visibility: protected";
            break;
    }
    os << " | IsStatic: " << std::boolalpha << GetStatic();
    os << " | IsAsync: " << std::boolalpha << GetAsync();
    os << " | IsGenerator: " << std::boolalpha << GetGenerator();
    os << " | IsGetterSetter: " << std::boolalpha << GetIsGetterSetter();
    os << "\n";
}

void TSArrayType::Dump(std::ostream &os) const
{
    os << " - Dump TSArrayType - " << "\n";
    os << " - TSArrayType globalTSTypeRef: ";
    GlobalTSTypeRef gt = GetGT();
    os << gt.GetType();
    os << "\n";
    os << " - TSArrayType ElementGT: ";
    os <<  GetElementGT().GetType();
    os << "\n";
}

void TSIteratorInstanceType::Dump(std::ostream &os) const
{
    os << " - Dump IteratorInstance Type - " << "\n";
    os << " - TSIteratorInstanceType globalTSTypeRef: ";
    GlobalTSTypeRef gt = GetGT();
    uint64_t globalTSTypeRef = gt.GetType();
    os << globalTSTypeRef;
    os << "\n";
    os << " - TSIteratorInstanceType moduleId: ";
    uint32_t moduleId = gt.GetModuleId();
    os << moduleId;
    os << "\n";
    os << " - TSIteratorInstanceType localTypeId: ";
    uint32_t localTypeId = gt.GetLocalId();
    os << localTypeId;
    os << "\n";

    os << " - TSIteratorInstanceType KindGT: ";
    os << GetKindGT().GetType();
    os << "\n";

    os << " - TSIteratorInstanceType ElementGT: ";
    os << GetElementGT().GetType();
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
    GetEcmaModuleFilename().Dump(os);
    os << "\n";
    os << " - EcmaModuleRecordName: ";
    GetEcmaModuleRecordName().Dump(os);
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
    os << " - EvaluationError: ";
    os << GetEvaluationError();
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
}

void ImportEntry::Dump(std::ostream &os) const
{
    os << " - ModuleRequest: ";
    GetModuleRequest().Dump(os);
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
    os << " - ModuleRequest: ";
    GetModuleRequest().Dump(os);
    os << "\n";
    os << " - ImportName: ";
    GetImportName().Dump(os);
    os << "\n";
}

void StarExportEntry::Dump(std::ostream &os) const
{
    os << " - ModuleRequest: ";
    GetModuleRequest().Dump(os);
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

void ModuleNamespace::Dump(std::ostream &os) const
{
    os << " - Exports: ";
    GetExports().Dump(os);
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
                val = GetPropertyInlinedProps(i);
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
    os << " - ProfileTypeInfo: ";
    GetProfileTypeInfo().Dump(os);
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
static void DumpArrayClass(const TaggedArray *arr,
                           std::vector<std::pair<CString, JSTaggedValue>> &vec)
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

static void DumpConstantPoolClass(const ConstantPool *arr,
                                  std::vector<std::pair<CString, JSTaggedValue>> &vec)
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

static void DumpStringClass(const EcmaString *str,
                            std::vector<std::pair<CString, JSTaggedValue>> &vec)
{
    vec.emplace_back("string", JSTaggedValue(str));
}

static void DumpClass(TaggedObject *obj,
                         std::vector<std::pair<CString, JSTaggedValue>> &vec)
{
    JSHClass *jshclass = obj->GetClass();
    vec.emplace_back("__proto__", jshclass->GetPrototype());
}

static void DumpObject(TaggedObject *obj,
                       std::vector<std::pair<CString, JSTaggedValue>> &vec, bool isVmMode)
{
    DISALLOW_GARBAGE_COLLECTION;
    auto jsHclass = obj->GetClass();
    JSType type = jsHclass->GetObjectType();

    switch (type) {
        case JSType::HCLASS:
            DumpClass(obj, vec);
            return;
        case JSType::TAGGED_ARRAY:
        case JSType::TAGGED_DICTIONARY:
        case JSType::LEXICAL_ENV:
        case JSType::COW_TAGGED_ARRAY:
            DumpArrayClass(TaggedArray::Cast(obj), vec);
            return;
        case JSType::CONSTANT_POOL:
            DumpConstantPoolClass(ConstantPool::Cast(obj), vec);
            return;
        case JSType::STRING:
            DumpStringClass(EcmaString::Cast(obj), vec);
            return;
        case JSType::JS_NATIVE_POINTER:
            return;
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
        case JSType::JS_ARGUMENTS:
        case JSType::JS_GLOBAL_OBJECT:
            JSObject::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_FUNCTION_BASE:
        case JSType::JS_FUNCTION:
            JSFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_BOUND_FUNCTION:
            JSBoundFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_SET:
            JSSet::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_MAP:
            JSMap::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_WEAK_SET:
            JSWeakSet::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_WEAK_MAP:
            JSWeakMap::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_WEAK_REF:
            JSWeakRef::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_FINALIZATION_REGISTRY:
            JSFinalizationRegistry::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::CELL_RECORD:
            CellRecord::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_REG_EXP:
            JSRegExp::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_DATE:
            JSDate::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_ARRAY:
            JSArray::Cast(obj)->DumpForSnapshot(vec);
            return;
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
            return;
        case JSType::BIGINT:
            BigInt::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::BYTE_ARRAY:
            ByteArray::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_PROXY:
            JSProxy::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_PRIMITIVE_REF:
            JSPrimitiveRef::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::SYMBOL:
            JSSymbol::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::ACCESSOR_DATA:
        case JSType::INTERNAL_ACCESSOR:
            AccessorData::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_DATA_VIEW:
            JSDataView::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::PROMISE_REACTIONS:
            PromiseReaction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::PROMISE_CAPABILITY:
            PromiseCapability::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::PROMISE_ITERATOR_RECORD:
            PromiseIteratorRecord::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::PROMISE_RECORD:
            PromiseRecord::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::RESOLVING_FUNCTIONS_RECORD:
            ResolvingFunctionsRecord::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_PROMISE:
            JSPromise::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_PROMISE_REACTIONS_FUNCTION:
            JSPromiseReactionsFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_PROMISE_EXECUTOR_FUNCTION:
            JSPromiseExecutorFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::ASYNC_GENERATOR_REQUEST:
            AsyncGeneratorRequest::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::ASYNC_ITERATOR_RECORD:
            AsyncIteratorRecord::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_ASYNC_FROM_SYNC_ITERATOR:
            JSAsyncFromSyncIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION:
            JSAsyncFromSyncIterUnwarpFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION:
            JSPromiseAllResolveElementFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION:
            JSPromiseAnyRejectElementFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION:
            JSPromiseAllSettledElementFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_PROMISE_FINALLY_FUNCTION:
            JSPromiseFinallyFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION:
            JSPromiseValueThunkOrThrowerFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN:
            JSAsyncGeneratorResNextRetProRstFtn::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::MICRO_JOB_QUEUE:
            MicroJobQueue::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::PENDING_JOB:
            PendingJob::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::COMPLETION_RECORD:
            CompletionRecord::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_ITERATOR:
        case JSType::JS_ASYNCITERATOR:
        case JSType::JS_FORIN_ITERATOR:
        case JSType::JS_MAP_ITERATOR:
        case JSType::JS_SET_ITERATOR:
        case JSType::JS_ARRAY_ITERATOR:
        case JSType::JS_STRING_ITERATOR:
        case JSType::JS_REG_EXP_ITERATOR:
        case JSType::JS_ARRAY_BUFFER:
            JSArrayBuffer::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_SHARED_ARRAY_BUFFER:
            JSArrayBuffer::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_PROXY_REVOC_FUNCTION:
            JSProxyRevocFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_ASYNC_FUNCTION:
            JSAsyncFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION:
            JSAsyncAwaitStatusFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_GENERATOR_FUNCTION:
            JSGeneratorFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_ASYNC_GENERATOR_FUNCTION:
            JSAsyncGeneratorFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_INTL_BOUND_FUNCTION:
            JSIntlBoundFunction::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_REALM:
            JSRealm::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_INTL:
            JSIntl::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_LOCALE:
            JSLocale::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_DATE_TIME_FORMAT:
            JSDateTimeFormat::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_RELATIVE_TIME_FORMAT:
            JSRelativeTimeFormat::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_NUMBER_FORMAT:
            JSNumberFormat::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_CJS_MODULE:
            CjsModule::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_CJS_EXPORTS:
            CjsExports::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_CJS_REQUIRE:
            CjsRequire::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_COLLATOR:
            JSCollator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_PLURAL_RULES:
            JSPluralRules::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_DISPLAYNAMES:
            JSDisplayNames::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_LIST_FORMAT:
            JSListFormat::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_GENERATOR_OBJECT:
            JSGeneratorObject::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_ASYNC_GENERATOR_OBJECT:
            JSAsyncGeneratorObject::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_ASYNC_FUNC_OBJECT:
            JSAsyncFuncObject::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_GENERATOR_CONTEXT:
            GeneratorContext::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_ARRAY_LIST:
            JSAPIArrayList::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_ARRAYLIST_ITERATOR:
            JSAPIArrayListIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::LINKED_NODE:
        case JSType::RB_TREENODE:
            return;
        case JSType::JS_API_HASH_MAP:
            JSAPIHashMap::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_HASH_SET:
            JSAPIHashSet::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_HASHMAP_ITERATOR:
            JSAPIHashMapIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_HASHSET_ITERATOR:
            JSAPIHashSetIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_LIGHT_WEIGHT_MAP:
            JSAPILightWeightMap::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR:
            JSAPILightWeightMapIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_LIGHT_WEIGHT_SET:
            JSAPILightWeightSet::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR:
            JSAPILightWeightSetIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_TREE_MAP:
            JSAPITreeMap::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_TREE_SET:
            JSAPITreeSet::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_TREEMAP_ITERATOR:
            JSAPITreeMapIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_TREESET_ITERATOR:
            JSAPITreeSetIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_VECTOR:
            JSAPIVector::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_VECTOR_ITERATOR:
            JSAPIVectorIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_QUEUE:
            JSAPIQueue::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_QUEUE_ITERATOR:
            JSAPIQueueIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_DEQUE:
            JSAPIDeque::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_DEQUE_ITERATOR:
            JSAPIDequeIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_STACK:
            JSAPIStack::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_STACK_ITERATOR:
            JSAPIStackIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_LIST:
            JSAPIList::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_LINKED_LIST:
            JSAPILinkedList::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_LIST_ITERATOR:
            JSAPIListIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_LINKED_LIST_ITERATOR:
            JSAPILinkedListIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::SOURCE_TEXT_MODULE_RECORD:
            SourceTextModule::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::IMPORTENTRY_RECORD:
            ImportEntry::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::LOCAL_EXPORTENTRY_RECORD:
            LocalExportEntry::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::INDIRECT_EXPORTENTRY_RECORD:
            IndirectExportEntry::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::STAR_EXPORTENTRY_RECORD:
            StarExportEntry::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::RESOLVEDBINDING_RECORD:
            ResolvedBinding::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::RESOLVEDINDEXBINDING_RECORD:
            ResolvedIndexBinding::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_MODULE_NAMESPACE:
            ModuleNamespace::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_PLAIN_ARRAY:
            JSAPIPlainArray::Cast(obj)->DumpForSnapshot(vec);
            return;
        case JSType::JS_API_PLAIN_ARRAY_ITERATOR:
            JSAPIPlainArrayIterator::Cast(obj)->DumpForSnapshot(vec);
            return;
        default:
            break;
    }
    if (isVmMode) {
        switch (type) {
            case JSType::PROPERTY_BOX:
                PropertyBox::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::TEMPLATE_MAP:
                DumpArrayClass(TaggedArray::Cast(obj), vec);
                return;
            case JSType::GLOBAL_ENV:
                GlobalEnv::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::PROTO_CHANGE_MARKER:
                ProtoChangeMarker::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::PROTOTYPE_INFO:
                ProtoChangeDetails::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::PROGRAM:
                Program::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::MACHINE_CODE_OBJECT:
                MachineCode::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::TRANSITION_HANDLER:
                TransitionHandler::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::TRANS_WITH_PROTO_HANDLER:
                TransWithProtoHandler::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::STORE_TS_HANDLER:
                StoreTSHandler::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::PROTOTYPE_HANDLER:
                PrototypeHandler::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::CLASS_INFO_EXTRACTOR:
                ClassInfoExtractor::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::TS_OBJECT_TYPE:
                TSObjectType::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::TS_CLASS_TYPE:
                TSClassType::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::TS_INTERFACE_TYPE:
                TSInterfaceType::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::TS_CLASS_INSTANCE_TYPE:
                TSClassInstanceType::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::TS_UNION_TYPE:
                TSUnionType::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::TS_FUNCTION_TYPE:
                TSFunctionType::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::TS_ARRAY_TYPE:
                TSArrayType::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::TS_ITERATOR_INSTANCE_TYPE:
                TSIteratorInstanceType::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::METHOD:
                Method::Cast(obj)->DumpForSnapshot(vec);
                return;
            case JSType::CLASS_LITERAL:
                ClassLiteral::Cast(obj)->DumpForSnapshot(vec);
                return;
            default:
                UNREACHABLE();
                break;
        }
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

void JSTaggedValue::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec, bool isVmMode) const
{
    if (IsHeapObject()) {
        return DumpObject(GetTaggedObject(), vec, isVmMode);
    }

    UNREACHABLE();
}

void NumberDictionary::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int size = Size();
    vec.reserve(vec.size() + size);
    for (int hashIndex = 0; hashIndex < size; hashIndex++) {
        JSTaggedValue key(GetKey(hashIndex));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            JSTaggedValue val(GetValue(hashIndex));
            CString str = ToCString(static_cast<uint32_t>(JSTaggedNumber(key).GetNumber()));
            vec.emplace_back(str, val);
        }
    }
}

void NameDictionary::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
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

void GlobalDictionary::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
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

void LinkedHashSet::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
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

void LinkedHashMap::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
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

void TaggedTreeMap::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfElements() + NumberOfDeletedElements();
    vec.reserve(vec.size() + capacity);
    for (int index = 0; index < capacity; index++) {
        JSTaggedValue key(GetKey(index));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            JSTaggedValue val = GetValue(index);
            CString str;
            KeyToStd(str, key);
            vec.emplace_back(str, val);
        }
    }
}

void TaggedTreeSet::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    int capacity = NumberOfElements() + NumberOfDeletedElements();
    vec.reserve(vec.size() + capacity);
    for (int index = 0; index < capacity; index++) {
        JSTaggedValue key(GetKey(index));
        if (!key.IsUndefined() && !key.IsHole() && !key.IsNull()) {
            CString str;
            KeyToStd(str, key);
            vec.emplace_back(str, JSTaggedValue::Hole());
        }
    }
}

void TaggedDoubleList::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
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

void TaggedSingleList::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
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

void JSObject::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    DISALLOW_GARBAGE_COLLECTION;
    JSHClass *jshclass = GetJSHClass();
    vec.emplace_back("__proto__", jshclass->GetPrototype());

    TaggedArray *elements = TaggedArray::Cast(GetElements().GetTaggedObject());
    if (elements->GetLength() == 0) {
    } else if (!elements->IsDictionaryMode()) {
        DumpArrayClass(elements, vec);
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

    if (!properties->IsDictionaryMode()) {
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
                val = GetPropertyInlinedProps(i);
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

void JSHClass::DumpForSnapshot([[maybe_unused]] std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
}

void JSFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("ProtoOrHClass", GetProtoOrHClass());
    vec.emplace_back("LexicalEnv", GetLexicalEnv());
    vec.emplace_back("HomeObject", GetHomeObject());
    vec.emplace_back("FunctionKind", JSTaggedValue(static_cast<int>(GetFunctionKind())));
    vec.emplace_back("FunctionExtraInfo", GetFunctionExtraInfo());
    JSObject::DumpForSnapshot(vec);
}

void Method::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("ConstantPool", GetConstantPool());
    vec.emplace_back("ProfileTypeInfo", GetProfileTypeInfo());
}

void Program::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("MainFunction", GetMainFunction());
}

void ConstantPool::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    DumpArrayClass(this, vec);
}

void COWTaggedArray::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    DumpArrayClass(this, vec);
}

void JSBoundFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSObject::DumpForSnapshot(vec);

    vec.emplace_back("BoundTarget", GetBoundTarget());
    vec.emplace_back("BoundThis", GetBoundThis());
    vec.emplace_back("BoundArguments", GetBoundArguments());
}

void JSPrimitiveRef::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("subValue", GetValue());
    JSObject::DumpForSnapshot(vec);
}

void BigInt::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Length", JSTaggedValue(GetLength()));
    vec.emplace_back("Sign", JSTaggedValue(GetSign()));
}

void JSDate::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("time", GetTime());
    vec.emplace_back("localOffset", GetLocalOffset());

    JSObject::DumpForSnapshot(vec);
}

void JSMap::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    LinkedHashMap *map = LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject());
    map->DumpForSnapshot(vec);

    JSObject::DumpForSnapshot(vec);
}

void JSForInIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Object", GetObject());
    vec.emplace_back("WasVisited", JSTaggedValue(GetWasVisited()));
    vec.emplace_back("VisitedKeys", GetVisitedKeys());
    vec.emplace_back("RemainingKeys", GetRemainingKeys());
    JSObject::DumpForSnapshot(vec);
}

void JSMapIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    LinkedHashMap *map = LinkedHashMap::Cast(GetIteratedMap().GetTaggedObject());
    map->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
    vec.emplace_back("IterationKind", JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(vec);
}

void JSSet::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    LinkedHashSet *set = LinkedHashSet::Cast(GetLinkedSet().GetTaggedObject());
    set->DumpForSnapshot(vec);

    JSObject::DumpForSnapshot(vec);
}

void JSWeakMap::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    LinkedHashMap *map = LinkedHashMap::Cast(GetLinkedMap().GetTaggedObject());
    map->DumpForSnapshot(vec);

    JSObject::DumpForSnapshot(vec);
}

void JSWeakSet::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    LinkedHashSet *set = LinkedHashSet::Cast(GetLinkedSet().GetTaggedObject());
    set->DumpForSnapshot(vec);

    JSObject::DumpForSnapshot(vec);
}

void JSWeakRef::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("WeakObject", GetWeakObject());
    JSObject::DumpForSnapshot(vec);
}

void JSFinalizationRegistry::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("CleanupCallback", GetCleanupCallback());
    LinkedHashMap *map = LinkedHashMap::Cast(GetMaybeUnregister().GetTaggedObject());
    map->DumpForSnapshot(vec);
    vec.emplace_back("MaybeUnregister", GetMaybeUnregister());
    JSObject::DumpForSnapshot(vec);
}

void CellRecord::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("WeakRefTarget", GetWeakRefTarget());
    vec.emplace_back("HeldValue", GetHeldValue());
}

void JSSetIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    LinkedHashSet *set = LinkedHashSet::Cast(GetIteratedSet().GetTaggedObject());
    set->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
    vec.emplace_back("IterationKind", JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(vec);
}

void JSArray::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPIArrayList::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPIArrayListIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSAPIArrayList *arraylist = JSAPIArrayList::Cast(GetIteratedArrayList().GetTaggedObject());
    arraylist->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPILightWeightMap::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPILightWeightMapIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSAPILightWeightMap *map =
        JSAPILightWeightMap::Cast(GetIteratedLightWeightMap().GetTaggedObject());
    map->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
    vec.emplace_back("IterationKind", JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIQueue::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPIQueueIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSAPIQueue *queue = JSAPIQueue::Cast(GetIteratedQueue().GetTaggedObject());
    queue->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIDeque::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPIDequeIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSAPIDeque *deque = JSAPIDeque::Cast(GetIteratedDeque().GetTaggedObject());
    deque->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSAPILightWeightSet::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPILightWeightSetIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSAPILightWeightSet *set =
        JSAPILightWeightSet::Cast(GetIteratedLightWeightSet().GetTaggedObject());
    set->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
    vec.emplace_back("IterationKind", JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIStack::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPIStackIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSAPIStack *stack = JSAPIStack::Cast(GetIteratedStack().GetTaggedObject());
    stack->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSArrayIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSArray *array = JSArray::Cast(GetIteratedArray().GetTaggedObject());
    array->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
    vec.emplace_back("IterationKind", JSTaggedValue(static_cast<int>(GetIterationKind())));
    JSObject::DumpForSnapshot(vec);
}

void JSAPIVector::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSObject::DumpForSnapshot(vec);
}

void JSAPIVectorIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSAPIVector *vector = JSAPIVector::Cast(GetIteratedVector().GetTaggedObject());
    vector->DumpForSnapshot(vec);
    vec.emplace_back("NextIndex", JSTaggedValue(GetNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSStringIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("IteratedString", GetIteratedString());
    vec.emplace_back("StringIteratorNextIndex", JSTaggedValue(GetStringIteratorNextIndex()));
    JSObject::DumpForSnapshot(vec);
}

void JSTypedArray::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 5;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back("viewed-array-buffer", GetViewedArrayBuffer());
    vec.emplace_back("typed-array-name", GetTypedArrayName());
    vec.emplace_back("byte-length", JSTaggedValue(GetByteLength()));
    vec.emplace_back("byte-offset", JSTaggedValue(GetByteOffset()));
    vec.emplace_back("array-length", JSTaggedValue(GetArrayLength()));
}

void ByteArray::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("length", JSTaggedValue(GetLength()));
    vec.emplace_back("size", JSTaggedValue(GetSize()));
}

void JSRegExp::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("originalSource", GetOriginalSource());
    vec.emplace_back("originalFlags", GetOriginalFlags());
    vec.emplace_back("groupName", GetGroupName());
    JSObject::DumpForSnapshot(vec);
}

void JSRegExpIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("IteratingRegExp", GetIteratingRegExp());
    vec.emplace_back("IteratedString", GetIteratedString());
    vec.emplace_back("Global", JSTaggedValue(GetGlobal()));
    vec.emplace_back("Unicode", JSTaggedValue(GetUnicode()));
    vec.emplace_back("Done", JSTaggedValue(GetDone()));
    JSObject::DumpForSnapshot(vec);
}

void JSProxy::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("target", GetTarget());
    vec.emplace_back("handler", GetHandler());
}

void JSSymbol::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("hash-field", JSTaggedValue(GetHashField()));
    vec.emplace_back("flags", JSTaggedValue(GetFlags()));
    vec.emplace_back("description", GetDescription());
}

void AccessorData::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("getter", GetGetter());
    vec.emplace_back("setter", GetSetter());
}

void LexicalEnv::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    DumpArrayClass(this, vec);
}

void GlobalEnv::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    auto globalConst = GetJSThread()->GlobalConstants();
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 137;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back("ObjectFunction", GetObjectFunction().GetTaggedValue());
    vec.emplace_back("FunctionFunction", GetFunctionFunction().GetTaggedValue());
    vec.emplace_back("NumberFunction", GetNumberFunction().GetTaggedValue());
    vec.emplace_back("BigIntFunction", GetBigIntFunction().GetTaggedValue());
    vec.emplace_back("DateFunction", GetDateFunction().GetTaggedValue());
    vec.emplace_back("BooleanFunction", GetBooleanFunction().GetTaggedValue());
    vec.emplace_back("ErrorFunction", GetErrorFunction().GetTaggedValue());
    vec.emplace_back("ArrayFunction", GetArrayFunction().GetTaggedValue());
    vec.emplace_back("TypedArrayFunction", GetTypedArrayFunction().GetTaggedValue());
    vec.emplace_back("Int8ArrayFunction", GetInt8ArrayFunction().GetTaggedValue());
    vec.emplace_back("Uint8ArrayFunction", GetUint8ArrayFunction().GetTaggedValue());
    vec.emplace_back("Uint8ClampedArrayFunction", GetUint8ClampedArrayFunction().GetTaggedValue());
    vec.emplace_back("Int16ArrayFunction", GetInt16ArrayFunction().GetTaggedValue());
    vec.emplace_back("Uint16ArrayFunction", GetUint16ArrayFunction().GetTaggedValue());
    vec.emplace_back("Int32ArrayFunction", GetInt32ArrayFunction().GetTaggedValue());
    vec.emplace_back("Uint32ArrayFunction", GetUint32ArrayFunction().GetTaggedValue());
    vec.emplace_back("Float32ArrayFunction", GetFloat32ArrayFunction().GetTaggedValue());
    vec.emplace_back("Float64ArrayFunction", GetFloat64ArrayFunction().GetTaggedValue());
    vec.emplace_back("ArrayBufferFunction", GetArrayBufferFunction().GetTaggedValue());
    vec.emplace_back("SharedArrayBufferFunction", GetSharedArrayBufferFunction().GetTaggedValue());
    vec.emplace_back("SymbolFunction", GetSymbolFunction().GetTaggedValue());
    vec.emplace_back("RangeErrorFunction", GetRangeErrorFunction().GetTaggedValue());
    vec.emplace_back("ReferenceErrorFunction", GetReferenceErrorFunction().GetTaggedValue());
    vec.emplace_back("TypeErrorFunction", GetTypeErrorFunction().GetTaggedValue());
    vec.emplace_back("AggregateErrorFunction", GetAggregateErrorFunction().GetTaggedValue());
    vec.emplace_back("URIErrorFunction", GetURIErrorFunction().GetTaggedValue());
    vec.emplace_back("SyntaxErrorFunction", GetSyntaxErrorFunction().GetTaggedValue());
    vec.emplace_back("EvalErrorFunction", GetEvalErrorFunction().GetTaggedValue());
    vec.emplace_back("OOMErrorFunction", GetOOMErrorFunction().GetTaggedValue());
    vec.emplace_back("RegExpFunction", GetRegExpFunction().GetTaggedValue());
    vec.emplace_back("BuiltinsSetFunction", GetBuiltinsSetFunction().GetTaggedValue());
    vec.emplace_back("BuiltinsMapFunction", GetBuiltinsMapFunction().GetTaggedValue());
    vec.emplace_back("BuiltinsWeakSetFunction", GetBuiltinsWeakSetFunction().GetTaggedValue());
    vec.emplace_back("BuiltinsWeakMapFunction", GetBuiltinsWeakMapFunction().GetTaggedValue());
    vec.emplace_back("BuiltinsWeakRefFunction", GetBuiltinsWeakRefFunction().GetTaggedValue());
    vec.emplace_back("BuiltinsFinalizationRegistryFunction",
        GetBuiltinsFinalizationRegistryFunction().GetTaggedValue());
    vec.emplace_back("MathFunction", GetMathFunction().GetTaggedValue());
    vec.emplace_back("AtomicsFunction", GetAtomicsFunction().GetTaggedValue());
    vec.emplace_back("JsonFunction", GetJsonFunction().GetTaggedValue());
    vec.emplace_back("StringFunction", GetStringFunction().GetTaggedValue());
    vec.emplace_back("ProxyFunction", GetProxyFunction().GetTaggedValue());
    vec.emplace_back("ReflectFunction", GetReflectFunction().GetTaggedValue());
    vec.emplace_back("AsyncFunction", GetAsyncFunction().GetTaggedValue());
    vec.emplace_back("AsyncFunctionPrototype", GetAsyncFunctionPrototype().GetTaggedValue());
    vec.emplace_back("JSGlobalObject", GetJSGlobalObject().GetTaggedValue());
    vec.emplace_back("EmptyArray", globalConst->GetEmptyArray());
    vec.emplace_back("EmptyString", globalConst->GetEmptyString());
    vec.emplace_back("EmptyTaggedQueue", globalConst->GetEmptyTaggedQueue());
    vec.emplace_back("PrototypeString", globalConst->GetPrototypeString());
    vec.emplace_back("HasInstanceSymbol", GetHasInstanceSymbol().GetTaggedValue());
    vec.emplace_back("IsConcatSpreadableSymbol", GetIsConcatSpreadableSymbol().GetTaggedValue());
    vec.emplace_back("ToStringTagSymbol", GetToStringTagSymbol().GetTaggedValue());
    vec.emplace_back("IteratorSymbol", GetIteratorSymbol().GetTaggedValue());
    vec.emplace_back("AsyncIteratorSymbol", GetAsyncIteratorSymbol().GetTaggedValue());
    vec.emplace_back("MatchSymbol", GetMatchSymbol().GetTaggedValue());
    vec.emplace_back("MatchAllSymbol", GetMatchAllSymbol().GetTaggedValue());
    vec.emplace_back("ReplaceSymbol", GetReplaceSymbol().GetTaggedValue());
    vec.emplace_back("SearchSymbol", GetSearchSymbol().GetTaggedValue());
    vec.emplace_back("SpeciesSymbol", GetSpeciesSymbol().GetTaggedValue());
    vec.emplace_back("SplitSymbol", GetSplitSymbol().GetTaggedValue());
    vec.emplace_back("ToPrimitiveSymbol", GetToPrimitiveSymbol().GetTaggedValue());
    vec.emplace_back("UnscopablesSymbol", GetUnscopablesSymbol().GetTaggedValue());
    vec.emplace_back("HoleySymbol", GetHoleySymbol().GetTaggedValue());
    vec.emplace_back("AttachSymbol", GetAttachSymbol().GetTaggedValue());
    vec.emplace_back("DetachSymbol", GetDetachSymbol().GetTaggedValue());
    vec.emplace_back("ConstructorString", globalConst->GetConstructorString());
    vec.emplace_back("IteratorPrototype", GetIteratorPrototype().GetTaggedValue());
    vec.emplace_back("ForinIteratorPrototype", GetForinIteratorPrototype().GetTaggedValue());
    vec.emplace_back("StringIterator", GetStringIterator().GetTaggedValue());
    vec.emplace_back("MapIteratorPrototype", GetMapIteratorPrototype().GetTaggedValue());
    vec.emplace_back("SetIteratorPrototype", GetSetIteratorPrototype().GetTaggedValue());
    vec.emplace_back("RegExpIteratorPrototype", GetRegExpIteratorPrototype().GetTaggedValue());
    vec.emplace_back("ArrayIteratorPrototype", GetArrayIteratorPrototype().GetTaggedValue());
    vec.emplace_back("StringIteratorPrototype", GetStringIteratorPrototype().GetTaggedValue());
    vec.emplace_back("LengthString", globalConst->GetLengthString());
    vec.emplace_back("ValueString", globalConst->GetValueString());
    vec.emplace_back("WritableString", globalConst->GetWritableString());
    vec.emplace_back("GetString", globalConst->GetGetString());
    vec.emplace_back("SetString", globalConst->GetSetString());
    vec.emplace_back("EnumerableString", globalConst->GetEnumerableString());
    vec.emplace_back("ConfigurableString", globalConst->GetConfigurableString());
    vec.emplace_back("NameString", globalConst->GetNameString());
    vec.emplace_back("ValueOfString", globalConst->GetValueOfString());
    vec.emplace_back("ToStringString", globalConst->GetToStringString());
    vec.emplace_back("ToLocaleStringString", globalConst->GetToLocaleStringString());
    vec.emplace_back("UndefinedString", globalConst->GetUndefinedString());
    vec.emplace_back("NullString", globalConst->GetNullString());
    vec.emplace_back("TrueString", globalConst->GetTrueString());
    vec.emplace_back("FalseString", globalConst->GetFalseString());
    vec.emplace_back("RegisterSymbols", GetRegisterSymbols().GetTaggedValue());
    vec.emplace_back("ThrowTypeError", GetThrowTypeError().GetTaggedValue());
    vec.emplace_back("GetPrototypeOfString", globalConst->GetGetPrototypeOfString());
    vec.emplace_back("SetPrototypeOfString", globalConst->GetSetPrototypeOfString());
    vec.emplace_back("IsExtensibleString", globalConst->GetIsExtensibleString());
    vec.emplace_back("PreventExtensionsString", globalConst->GetPreventExtensionsString());
    vec.emplace_back("GetOwnPropertyDescriptorString", globalConst->GetGetOwnPropertyDescriptorString());
    vec.emplace_back("DefinePropertyString", globalConst->GetDefinePropertyString());
    vec.emplace_back("HasString", globalConst->GetHasString());
    vec.emplace_back("DeletePropertyString", globalConst->GetDeletePropertyString());
    vec.emplace_back("EnumerateString", globalConst->GetEnumerateString());
    vec.emplace_back("OwnKeysString", globalConst->GetOwnKeysString());
    vec.emplace_back("ApplyString", globalConst->GetApplyString());
    vec.emplace_back("ProxyString", globalConst->GetProxyString());
    vec.emplace_back("RevokeString", globalConst->GetRevokeString());
    vec.emplace_back("ProxyConstructString", globalConst->GetProxyConstructString());
    vec.emplace_back("ProxyCallString", globalConst->GetProxyCallString());
    vec.emplace_back("DoneString", globalConst->GetDoneString());
    vec.emplace_back("NegativeZeroString", globalConst->GetNegativeZeroString());
    vec.emplace_back("NextString", globalConst->GetNextString());
    vec.emplace_back("PromiseThenString", globalConst->GetPromiseThenString());
    vec.emplace_back("PromiseFunction", GetPromiseFunction().GetTaggedValue());
    vec.emplace_back("PromiseReactionJob", GetPromiseReactionJob().GetTaggedValue());
    vec.emplace_back("PromiseResolveThenableJob", GetPromiseResolveThenableJob().GetTaggedValue());
    vec.emplace_back("DynamicImportJob", GetDynamicImportJob().GetTaggedValue());
    vec.emplace_back("ScriptJobString", globalConst->GetScriptJobString());
    vec.emplace_back("PromiseString", globalConst->GetPromiseString());
    vec.emplace_back("IdentityString", globalConst->GetIdentityString());
    vec.emplace_back("AsyncFunctionString", globalConst->GetAsyncFunctionString());
    vec.emplace_back("ThrowerString", globalConst->GetThrowerString());
    vec.emplace_back("Undefined", globalConst->GetUndefined());
    vec.emplace_back("ArrayListFunction", globalConst->GetArrayListFunction());
    vec.emplace_back("ArrayListIteratorPrototype", globalConst->GetArrayListIteratorPrototype());
    vec.emplace_back("HashMapIteratorPrototype", globalConst->GetHashMapIteratorPrototype());
    vec.emplace_back("HashSetIteratorPrototype", globalConst->GetHashSetIteratorPrototype());
    vec.emplace_back("LightWeightMapIteratorPrototype", globalConst->GetLightWeightMapIteratorPrototype());
    vec.emplace_back("LightWeightSetIteratorPrototype", globalConst->GetLightWeightSetIteratorPrototype());
    vec.emplace_back("TreeMapIteratorPrototype", globalConst->GetTreeMapIteratorPrototype());
    vec.emplace_back("TreeSetIteratorPrototype", globalConst->GetTreeSetIteratorPrototype());
    vec.emplace_back("VectorFunction", globalConst->GetVectorFunction());
    vec.emplace_back("VectorIteratorPrototype", globalConst->GetVectorIteratorPrototype());
    vec.emplace_back("QueueIteratorPrototype", globalConst->GetQueueIteratorPrototype());
    vec.emplace_back("PlainArrayIteratorPrototype", globalConst->GetPlainArrayIteratorPrototype());
    vec.emplace_back("DequeIteratorPrototype", globalConst->GetDequeIteratorPrototype());
    vec.emplace_back("StackIteratorPrototype", globalConst->GetStackIteratorPrototype());
    vec.emplace_back("LinkedListIteratorPrototype", globalConst->GetLinkedListIteratorPrototype());
    vec.emplace_back("ListIteratorPrototype", globalConst->GetListIteratorPrototype());
    vec.emplace_back("GlobalPatch", GetGlobalPatch().GetTaggedValue());
}

void JSDataView::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("data-view", GetDataView());
    vec.emplace_back("buffer", GetViewedArrayBuffer());
    vec.emplace_back("byte-length", JSTaggedValue(GetByteLength()));
    vec.emplace_back("byte-offset", JSTaggedValue(GetByteOffset()));
}

void JSArrayBuffer::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("buffer-data", GetArrayBufferData());
    vec.emplace_back("byte-length", JSTaggedValue(GetArrayBufferByteLength()));
    vec.emplace_back("shared", JSTaggedValue(GetShared()));
}

void PromiseReaction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("promise-capability", GetPromiseCapability());
    vec.emplace_back("handler", GetHandler());
    vec.emplace_back("type", JSTaggedValue(static_cast<int>(GetType())));
}

void PromiseCapability::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("promise", GetPromise());
    vec.emplace_back("resolve", GetResolve());
    vec.emplace_back("reject", GetReject());
}

void PromiseIteratorRecord::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("iterator", GetIterator());
    vec.emplace_back("done", JSTaggedValue(GetDone()));
}

void PromiseRecord::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("value", GetValue());
}

void ResolvingFunctionsRecord::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("resolve-function", GetResolveFunction());
    vec.emplace_back("reject-function", GetRejectFunction());
}

void AsyncGeneratorRequest::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("completion", GetCompletion());
    vec.emplace_back("capability", GetCapability());
}

void AsyncIteratorRecord::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("iterator", GetIterator());
    vec.emplace_back("nextmethod", GetNextMethod());
    vec.emplace_back("done", JSTaggedValue(GetDone()));
}

void JSAsyncFromSyncIterator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("synciteratorrecord", GetSyncIteratorRecord());
}

void JSAsyncFromSyncIterUnwarpFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("done", JSTaggedValue(GetDone()));
}

void JSPromise::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("promise-state", JSTaggedValue(static_cast<int>(GetPromiseState())));
    vec.emplace_back("promise-result", GetPromiseResult());
    vec.emplace_back("promise-fulfill-reactions", GetPromiseFulfillReactions());
    vec.emplace_back("promise-reject-reactions", GetPromiseRejectReactions());
    vec.emplace_back("promise-is-handled", JSTaggedValue(GetPromiseIsHandled()));
    JSObject::DumpForSnapshot(vec);
}

void JSPromiseReactionsFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("promise", GetPromise());
    vec.emplace_back("already-resolved", GetAlreadyResolved());
    JSObject::DumpForSnapshot(vec);
}

void JSAsyncGeneratorResNextRetProRstFtn::DumpForSnapshot(std::vector<std::pair<CString,
                                                          JSTaggedValue>> &vec) const
{
    vec.emplace_back("async-generator-object", GetAsyncGeneratorObject());
    JSObject::DumpForSnapshot(vec);
}

void JSPromiseExecutorFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("capability", GetCapability());
    JSObject::DumpForSnapshot(vec);
}

void JSPromiseAllResolveElementFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("index", GetIndex());
    vec.emplace_back("values", GetValues());
    vec.emplace_back("capabilities", GetCapabilities());
    vec.emplace_back("remaining-elements", GetRemainingElements());
    vec.emplace_back("already-called", GetAlreadyCalled());
    JSObject::DumpForSnapshot(vec);
}

void JSPromiseAnyRejectElementFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("index", JSTaggedValue(GetIndex()));
    vec.emplace_back("errors", GetErrors());
    vec.emplace_back("capability", GetCapability());
    vec.emplace_back("remaining-elements", GetRemainingElements());
    vec.emplace_back("already-called", GetAlreadyCalled());
    JSObject::DumpForSnapshot(vec);
}

void JSPromiseAllSettledElementFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("already-called", GetAlreadyCalled());
    vec.emplace_back("index", JSTaggedValue(GetIndex()));
    vec.emplace_back("values", GetValues());
    vec.emplace_back("capability", GetCapability());
    vec.emplace_back("remaining-elements", GetRemainingElements());
    JSObject::DumpForSnapshot(vec);
}

void JSPromiseFinallyFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("constructor", GetConstructor());
    vec.emplace_back("onFinally", GetOnFinally());
    JSObject::DumpForSnapshot(vec);
}

void JSPromiseValueThunkOrThrowerFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("result", GetResult());
    JSObject::DumpForSnapshot(vec);
}

void MicroJobQueue::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("promise-job-queue", GetPromiseJobQueue());
    vec.emplace_back("script-job-queue", GetScriptJobQueue());
}

void PendingJob::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("job", GetJob());
    vec.emplace_back("arguments", GetArguments());
}

void CompletionRecord::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("value", GetValue());
    vec.emplace_back("type", JSTaggedValue(static_cast<int>(GetType())));
}

void JSProxyRevocFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("RevocableProxy", GetRevocableProxy());
}

void JSAsyncFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSFunction::DumpForSnapshot(vec);
}

void JSAsyncAwaitStatusFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("AsyncContext", GetAsyncContext());
}

void JSGeneratorFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSFunction::DumpForSnapshot(vec);
}

void JSAsyncGeneratorFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    JSFunction::DumpForSnapshot(vec);
}

void JSIntlBoundFunction::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("NumberFormat", GetNumberFormat());
    vec.emplace_back("DateTimeFormat", GetDateTimeFormat());
    vec.emplace_back("Collator", GetCollator());
    JSObject::DumpForSnapshot(vec);
}

void PropertyBox::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Value", GetValue());
}

void PrototypeHandler::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("HandlerInfo", GetHandlerInfo());
    vec.emplace_back("ProtoCell", GetProtoCell());
    vec.emplace_back("Holder", GetHolder());
}

void TransitionHandler::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("HandlerInfo", GetHandlerInfo());
    vec.emplace_back("TransitionHClass", GetTransitionHClass());
}

void TransWithProtoHandler::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("HandlerInfo", GetHandlerInfo());
    vec.emplace_back("TransitionHClass", GetTransitionHClass());
    vec.emplace_back("ProtoCell", GetProtoCell());
}

void StoreTSHandler::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("HandlerInfo", GetHandlerInfo());
    vec.emplace_back("ProtoCell", GetProtoCell());
    vec.emplace_back("Holder", GetHolder());
}

void JSRealm::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Value", GetValue());
    vec.emplace_back("GLobalEnv", GetGlobalEnv());
    JSObject::DumpForSnapshot(vec);
}

void JSIntl::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("FallbackSymbol", GetFallbackSymbol());
    JSObject::DumpForSnapshot(vec);
}

void JSLocale::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("IcuField", GetIcuField());
    JSObject::DumpForSnapshot(vec);
}

void JSDateTimeFormat::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 11;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back("Locale", GetLocale());
    vec.emplace_back("Calendar", GetCalendar());
    vec.emplace_back("NumberingSystem", GetNumberingSystem());
    vec.emplace_back("TimeZone", GetTimeZone());
    vec.emplace_back("HourCycle", JSTaggedValue(static_cast<int>(GetHourCycle())));
    vec.emplace_back("LocaleIcu", GetLocaleIcu());
    vec.emplace_back("SimpleDateTimeFormatIcu", GetSimpleDateTimeFormatIcu());
    vec.emplace_back("Iso8601", GetIso8601());
    vec.emplace_back("DateStyle", JSTaggedValue(static_cast<int>(GetDateStyle())));
    vec.emplace_back("TimeStyle", JSTaggedValue(static_cast<int>(GetTimeStyle())));
    vec.emplace_back("BoundFormat", GetBoundFormat());
    JSObject::DumpForSnapshot(vec);
}

void JSRelativeTimeFormat::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Locale", GetLocale());
    vec.emplace_back("NumberingSystem", GetNumberingSystem());
    vec.emplace_back("Style", JSTaggedValue(static_cast<int>(GetStyle())));
    vec.emplace_back("Numeric", JSTaggedValue(static_cast<int>(GetNumeric())));
    vec.emplace_back("IcuField", GetIcuField());
    JSObject::DumpForSnapshot(vec);
}

void JSNumberFormat::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 20;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back("Locale", GetLocale());
    vec.emplace_back("NumberingSystem", GetNumberingSystem());
    vec.emplace_back("Style", JSTaggedValue(static_cast<int>(GetStyle())));
    vec.emplace_back("Currency", GetCurrency());
    vec.emplace_back("CurrencyDisplay", JSTaggedValue(static_cast<int>(GetCurrencyDisplay())));
    vec.emplace_back("CurrencySign", JSTaggedValue(static_cast<int>(GetCurrencySign())));
    vec.emplace_back("Unit", GetUnit());
    vec.emplace_back("UnitDisplay", JSTaggedValue(static_cast<int>(GetUnitDisplay())));
    vec.emplace_back("MinimumIntegerDigits", GetMinimumIntegerDigits());
    vec.emplace_back("MinimumFractionDigits", GetMinimumFractionDigits());
    vec.emplace_back("MaximumFractionDigits", GetMaximumFractionDigits());
    vec.emplace_back("MinimumSignificantDigits", GetMinimumSignificantDigits());
    vec.emplace_back("MaximumSignificantDigits", GetMaximumSignificantDigits());
    vec.emplace_back("UseGrouping", GetUseGrouping());
    vec.emplace_back("RoundingType", JSTaggedValue(static_cast<int>(GetRoundingType())));
    vec.emplace_back("Notation", JSTaggedValue(static_cast<int>(GetNotation())));
    vec.emplace_back("CompactDisplay", JSTaggedValue(static_cast<int>(GetCompactDisplay())));
    vec.emplace_back("SignDisplay", JSTaggedValue(static_cast<int>(GetSignDisplay())));
    vec.emplace_back("BoundFormat", GetBoundFormat());
    vec.emplace_back("IcuField", GetIcuField());
    JSObject::DumpForSnapshot(vec);
}

void JSCollator::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 9;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back("IcuField", GetIcuField());
    vec.emplace_back("Locale", GetLocale());
    vec.emplace_back("Collation", GetCollation());
    vec.emplace_back("BoundCompare", GetBoundCompare());
    vec.emplace_back("CaseFirst", JSTaggedValue(static_cast<int>(GetCaseFirst())));
    vec.emplace_back("Usage", JSTaggedValue(static_cast<int>(GetUsage())));
    vec.emplace_back("Sensitivity", JSTaggedValue(static_cast<int>(GetSensitivity())));
    vec.emplace_back("IgnorePunctuation", JSTaggedValue(GetIgnorePunctuation()));
    vec.emplace_back("Numeric", JSTaggedValue(GetNumeric()));
    JSObject::DumpForSnapshot(vec);
}

void JSPluralRules::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 10;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back("Locale", GetLocale());
    vec.emplace_back("MinimumIntegerDigits", GetMinimumIntegerDigits());
    vec.emplace_back("MinimumFractionDigits", GetMinimumFractionDigits());
    vec.emplace_back("MaximumFractionDigits", GetMaximumFractionDigits());
    vec.emplace_back("MinimumSignificantDigits", GetMinimumSignificantDigits());
    vec.emplace_back("MaximumSignificantDigits", GetMaximumSignificantDigits());
    vec.emplace_back("RoundingType", JSTaggedValue(static_cast<int>(GetRoundingType())));
    vec.emplace_back("IcuPR", GetIcuPR());
    vec.emplace_back("IcuNF", GetIcuNF());
    vec.emplace_back("Type", JSTaggedValue(static_cast<int>(GetType())));
    JSObject::DumpForSnapshot(vec);
}

void JSDisplayNames::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Locale", GetLocale());
    vec.emplace_back("Type", JSTaggedValue(static_cast<int>(GetType())));
    vec.emplace_back("Style", JSTaggedValue(static_cast<int>(GetStyle())));
    vec.emplace_back("Fallback", JSTaggedValue(static_cast<int>(GetFallback())));
    vec.emplace_back("IcuLDN", GetIcuLDN());
    JSObject::DumpForSnapshot(vec);
}

void JSListFormat::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Locale", GetLocale());
    vec.emplace_back("Type", JSTaggedValue(static_cast<int>(GetType())));
    vec.emplace_back("Style", JSTaggedValue(static_cast<int>(GetStyle())));
    vec.emplace_back("IcuLF", GetIcuLF());
    JSObject::DumpForSnapshot(vec);
}

void JSGeneratorObject::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("GeneratorContext", GetGeneratorContext());
    vec.emplace_back("ResumeResult", GetResumeResult());
    vec.emplace_back("GeneratorState", JSTaggedValue(static_cast<int>(GetGeneratorState())));
    vec.emplace_back("ResumeMode", JSTaggedValue(static_cast<int>(GetResumeMode())));
    JSObject::DumpForSnapshot(vec);
}

void JSAsyncGeneratorObject::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("GeneratorContext", GetGeneratorContext());
    vec.emplace_back("AsyncGeneratorQueue", GetAsyncGeneratorQueue());
    vec.emplace_back("GeneratorBrand", GetGeneratorBrand());
    vec.emplace_back("ResumeResult", GetResumeResult());
    vec.emplace_back("AsyncGeneratorState", JSTaggedValue(static_cast<int>(GetAsyncGeneratorState())));
    vec.emplace_back("ResumeMode", JSTaggedValue(static_cast<int>(GetResumeMode())));
    JSObject::DumpForSnapshot(vec);
}

void JSAsyncFuncObject::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Promise", GetPromise());
}

void GeneratorContext::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 8;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back("RegsArray", GetRegsArray());
    vec.emplace_back("Method", GetMethod());
    vec.emplace_back("This", GetThis());
    vec.emplace_back("Acc", GetAcc());
    vec.emplace_back("GeneratorObject", GetGeneratorObject());
    vec.emplace_back("LexicalEnv", GetLexicalEnv());
    vec.emplace_back("NRegs",  JSTaggedValue(GetNRegs()));
    vec.emplace_back("BCOffset",  JSTaggedValue(GetBCOffset()));
}

void ProtoChangeMarker::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Promise", JSTaggedValue(GetHasChanged()));
}

void ProtoChangeDetails::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("ChangeListener", GetChangeListener());
    vec.emplace_back("RegisterIndex", JSTaggedValue(GetRegisterIndex()));
}

void MachineCode::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("InstructionSizeInBytes", JSTaggedValue(GetInstructionSizeInBytes()));
}

void ClassInfoExtractor::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 6;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back("NonStaticKeys", GetNonStaticKeys());
    vec.emplace_back("NonStaticProperties", GetNonStaticProperties());
    vec.emplace_back("NonStaticElements", GetNonStaticElements());
    vec.emplace_back("StaticKeys", GetStaticKeys());
    vec.emplace_back("StaticProperties", GetStaticProperties());
    vec.emplace_back("StaticElements", GetStaticElements());
}

void TSObjectType::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("ObjLayoutInfo", GetObjLayoutInfo());
    vec.emplace_back("HClass", GetHClass());
}

void TSClassType::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 5;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back("InstanceType", GetInstanceType());
    vec.emplace_back("ConstructorType", GetConstructorType());
    vec.emplace_back("PrototypeType", GetPrototypeType());
    vec.emplace_back("ExtensionGT", JSTaggedValue(GetExtensionGT().GetType()));
    vec.emplace_back("HasLinked", JSTaggedValue(GetHasLinked()));
}

void TSInterfaceType::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Fields", GetFields());
    vec.emplace_back("Extends", GetExtends());
}

void TSClassInstanceType::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("ClassGT", JSTaggedValue(GetClassGT().GetType()));
}

void TSUnionType::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("ComponentTypes", GetComponents());
}

void TSFunctionType::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 5;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back("Name", GetName());
    vec.emplace_back("ParameterTypes", GetParameterTypes());
    vec.emplace_back("ReturnGT", JSTaggedValue(GetReturnGT().GetType()));
    vec.emplace_back("ThisGT", JSTaggedValue(GetThisGT().GetType()));
    vec.emplace_back("BitFiled", JSTaggedValue(GetBitField()));
}

void TSArrayType::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("ParameterTypeRef", JSTaggedValue(GetElementGT().GetType()));
}

void TSIteratorInstanceType::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("kindGT", JSTaggedValue(GetKindGT().GetType()));
    vec.emplace_back("elementGT", JSTaggedValue(GetElementGT().GetType()));
}

void SourceTextModule::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    // please update the NUM_OF_ITEMS if you change the items below
    constexpr int16_t NUM_OF_ITEMS = 14;
    vec.reserve(vec.size() + NUM_OF_ITEMS);
    vec.emplace_back("Environment", GetEnvironment());
    vec.emplace_back("Namespace", GetNamespace());
    vec.emplace_back("EcmaModuleFilename", GetEcmaModuleFilename());
    vec.emplace_back("EcmaModuleRecordName", GetEcmaModuleRecordName());
    vec.emplace_back("RequestedModules", GetRequestedModules());
    vec.emplace_back("ImportEntries", GetImportEntries());
    vec.emplace_back("LocalExportEntries", GetLocalExportEntries());
    vec.emplace_back("IndirectExportEntries", GetIndirectExportEntries());
    vec.emplace_back("StarExportEntries", GetStarExportEntries());
    vec.emplace_back("Status", JSTaggedValue(static_cast<int32_t>(GetStatus())));
    vec.emplace_back("EvaluationError", JSTaggedValue(GetEvaluationError()));
    vec.emplace_back("DFSIndex", JSTaggedValue(GetDFSIndex()));
    vec.emplace_back("DFSAncestorIndex", JSTaggedValue(GetDFSAncestorIndex()));
    vec.emplace_back("NameDictionary", GetNameDictionary());
}

void ImportEntry::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("ModuleRequest", GetModuleRequest());
    vec.emplace_back("ImportName", GetImportName());
    vec.emplace_back("LocalName", GetLocalName());
}

void LocalExportEntry::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("ExportName", GetExportName());
    vec.emplace_back("LocalName", GetLocalName());
}

void IndirectExportEntry::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("ExportName", GetExportName());
    vec.emplace_back("ModuleRequest", GetModuleRequest());
    vec.emplace_back("ImportName", GetImportName());
}

void StarExportEntry::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("ModuleRequest", GetModuleRequest());
}

void ResolvedBinding::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Module", GetModule());
    vec.emplace_back("BindingName", GetBindingName());
}

void ResolvedIndexBinding::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Module", GetModule());
    vec.emplace_back("Index", JSTaggedValue(GetIndex()));
}

void ModuleNamespace::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Module", GetModule());
    vec.emplace_back("Exports", GetExports());
}

void CjsModule::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Id", GetId());
    vec.emplace_back("Path", GetPath());
    vec.emplace_back("Exports", GetExports());
    vec.emplace_back("Filename", GetFilename());
}

void CjsExports::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Exports", GetExports());
}

void CjsRequire::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Cache", GetCache());
    vec.emplace_back("Parent", GetParent());
}

void ClassLiteral::DumpForSnapshot(std::vector<std::pair<CString, JSTaggedValue>> &vec) const
{
    vec.emplace_back("Array", GetArray());
    vec.emplace_back("IsAOTUsed", GetIsAOTUsed());
}
}  // namespace panda::ecmascript
