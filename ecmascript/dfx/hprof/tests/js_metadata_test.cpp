/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "ecmascript/byte_array.h"
#include "ecmascript/dfx/native_module_failure_info.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/profile_type_info.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jobs/pending_job.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_async_from_sync_iterator.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_bitvector.h"
#include "ecmascript/js_api/js_api_bitvector_iterator.h"
#include "ecmascript/js_api/js_api_buffer.h"
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
#include "ecmascript/js_arguments.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_hclass.h"
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
#include "ecmascript/js_segmenter.h"
#include "ecmascript/js_segments.h"
#include "ecmascript/js_segment_iterator.h"
#include "ecmascript/js_set.h"
#include "ecmascript/js_displaynames.h"
#include "ecmascript/js_list_format.h"
#include "ecmascript/js_set_iterator.h"
#include "ecmascript/js_string_iterator.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/js_weak_ref.h"
#include "ecmascript/marker_cell.h"
#include "ecmascript/js_typed_array.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/class_literal.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/module/js_module_record.h"
#include "ecmascript/napi/include/jsnapi.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/global_env.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/require/js_cjs_require.h"
#include "ecmascript/require/js_cjs_exports.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/module/js_shared_module.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/shared_objects/js_sendable_arraybuffer.h"
#include "ecmascript/shared_objects/js_shared_array_iterator.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/shared_objects/js_shared_map_iterator.h"
#include "ecmascript/shared_objects/js_shared_set.h"
#include "ecmascript/shared_objects/js_shared_set_iterator.h"
#include "ecmascript/shared_objects/js_shared_typed_array.h"
#include "ecmascript/tagged_hash_table.h"
#include "gtest/gtest.h"
#include <cstddef>
#include <fstream>
#include <string>
#include <unordered_map>
#include <utility>
#include "cJSON.h"


namespace panda::test {
using namespace panda::ecmascript;
using ArrayVisitor = const std::function<bool(const cJSON *, int)>;
class JSMetadataTest : public testing::Test {
public:
    void SetUp() override
    {
        GTEST_LOG_(INFO) << "JSMetadataTest Setup";
    }

    void TearDown() override
    {
        GTEST_LOG_(INFO) << "JSMetadataTest TearDown";
    }
};

class JSMetadataTestHelper {
public:
    explicit JSMetadataTestHelper()
    {
        // { typeName: [all fields' name of this type in the same order as declared in .h files + typeName] }
        // typeName: Type name in JSType Enum
        fieldNameTable_ = {
            {JSType::INVALID, {"INVALID"}},
            {JSType::HCLASS, {
                "BitField", "BitField1", "Proto", "Layout", "Transitions", "Parent", "ProtoChangeMarker",
                "ProtoChangeDetails", "EnumCache", "DependentInfos",
                "ProfilerType", "HCLASS"}},
            {JSType::ACCESSOR_DATA, {"Getter", "Setter", "ACCESSOR_DATA"}},
            {JSType::AOT_LITERAL_INFO, {"AOT_LITERAL_INFO"}},
            {JSType::ASYNC_GENERATOR_REQUEST, {"Completion", "Capability", "ASYNC_GENERATOR_REQUEST"}},
            {JSType::ASYNC_ITERATOR_RECORD, {"Iterator", "NextMethod", "ASYNC_ITERATOR_RECORD"}},
            {JSType::BIGINT, {"BIGINT"}},
            {JSType::BYTE_ARRAY, {"BYTE_ARRAY"}},
            {JSType::CELL_RECORD, {"WeakRefTarget", "HeldValue", "CELL_RECORD"}},
            {JSType::CLASS_INFO_EXTRACTOR, {
                "NonStaticKeys", "NonStaticProperties", "NonStaticElements", "StaticKeys",
                "StaticProperties", "StaticElements",
                "ConstructorMethod", "CLASS_INFO_EXTRACTOR"}},
            {JSType::CLASS_LITERAL, {"Array", "CLASS_LITERAL"}},
            {JSType::COMPLETION_RECORD, {"Value", "COMPLETION_RECORD"}},
            {JSType::CONSTANT_POOL, {"CONSTANT_POOL"}},
            {JSType::COW_MUTANT_TAGGED_ARRAY, {"COW_MUTANT_TAGGED_ARRAY"}},
            {JSType::COW_TAGGED_ARRAY, {"COW_TAGGED_ARRAY"}},
            {JSType::EXTRA_PROFILE_TYPE_INFO, {"ReceiverObject", "HolderObject", "EXTRA_PROFILE_TYPE_INFO"}},
            {JSType::FUNCTION_TEMPLATE, {"Method", "Module", "RawProfileTypeInfo", "FUNCTION_TEMPLATE"}},
            {JSType::GLOBAL_ENV, {"GLOBAL_ENV"}},
            {JSType::IMPORTENTRY_RECORD, {"ImportName", "LocalName", "ModuleRequestIndex", "IMPORTENTRY_RECORD"}},
            {JSType::INDIRECT_EXPORTENTRY_RECORD, {"ExportName", "ImportName",
                                                   "ModuleRequestIndex", "INDIRECT_EXPORTENTRY_RECORD"}},
            {JSType::INTERNAL_ACCESSOR, {"Getter", "Setter", "INTERNAL_ACCESSOR"}},
            {JSType::JS_AGGREGATE_ERROR, {"Properties", "Elements", "JS_AGGREGATE_ERROR"}},
            {JSType::JS_API_ARRAYLIST_ITERATOR, {"IteratedArrayList", "JS_API_ARRAYLIST_ITERATOR"}},
            {JSType::JS_API_ARRAY_LIST, {"Length", "JS_API_ARRAY_LIST"}},
            {JSType::JS_API_BITVECTOR, {"NativePointer", "JS_API_BITVECTOR"}},
            {JSType::JS_API_BITVECTOR_ITERATOR, {"IteratedBitVector", "JS_API_BITVECTOR_ITERATOR"}},
            {JSType::JS_API_DEQUE, {"JS_API_DEQUE"}},
            {JSType::JS_API_DEQUE_ITERATOR, {"IteratedDeque", "JS_API_DEQUE_ITERATOR"}},
            {JSType::JS_API_HASHMAP_ITERATOR, {"IteratedHashMap", "TaggedQueue",
                                               "CurrentNodeResult", "JS_API_HASHMAP_ITERATOR"}},
            {JSType::JS_API_HASHSET_ITERATOR, {"IteratedHashSet", "TaggedQueue",
                                               "CurrentNodeResult", "JS_API_HASHSET_ITERATOR"}},
            {JSType::JS_API_HASH_MAP, {"Table", "JS_API_HASH_MAP"}},
            {JSType::JS_API_HASH_SET, {"Table", "JS_API_HASH_SET"}},
            {JSType::JS_API_LIGHT_WEIGHT_MAP, {"Hashes", "Keys", "Values", "JS_API_LIGHT_WEIGHT_MAP"}},
            {JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR, {"IteratedLightWeightMap", "JS_API_LIGHT_WEIGHT_MAP_ITERATOR"}},
            {JSType::JS_API_LIGHT_WEIGHT_SET, {"Hashes", "Values", "JS_API_LIGHT_WEIGHT_SET"}},
            {JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR, {"IteratedLightWeightSet", "JS_API_LIGHT_WEIGHT_SET_ITERATOR"}},
            {JSType::JS_API_LINKED_LIST, {"DoubleList", "JS_API_LINKED_LIST"}},
            {JSType::JS_API_LINKED_LIST_ITERATOR, {"IteratedLinkedList", "JS_API_LINKED_LIST_ITERATOR"}},
            {JSType::JS_API_LIST, {"SingleList", "JS_API_LIST"}},
            {JSType::JS_API_LIST_ITERATOR, {"IteratedList", "JS_API_LIST_ITERATOR"}},
            {JSType::JS_API_PLAIN_ARRAY, {"Keys", "Values", "JS_API_PLAIN_ARRAY"}},
            {JSType::JS_API_FAST_BUFFER, {"FastBufferData", "JS_API_FAST_BUFFER"}},
            {JSType::JS_API_PLAIN_ARRAY_ITERATOR, {"IteratedPlainArray", "JS_API_PLAIN_ARRAY_ITERATOR"}},
            {JSType::JS_API_QUEUE, {"Length", "JS_API_QUEUE"}},
            {JSType::JS_API_QUEUE_ITERATOR, {"IteratedQueue", "JS_API_QUEUE_ITERATOR"}},
            {JSType::JS_API_STACK, {"JS_API_STACK"}},
            {JSType::JS_API_STACK_ITERATOR, {"IteratedStack", "JS_API_STACK_ITERATOR"}},
            {JSType::JS_API_TREEMAP_ITERATOR, {"IteratedMap", "Entries", "JS_API_TREEMAP_ITERATOR"}},
            {JSType::JS_API_TREESET_ITERATOR, {"IteratedSet", "Entries", "JS_API_TREESET_ITERATOR"}},
            {JSType::JS_API_TREE_MAP, {"TreeMap", "JS_API_TREE_MAP"}},
            {JSType::JS_API_TREE_SET, {"TreeSet", "JS_API_TREE_SET"}},
            {JSType::JS_API_VECTOR, {"JS_API_VECTOR"}},
            {JSType::JS_API_VECTOR_ITERATOR, {"IteratedVector", "JS_API_VECTOR_ITERATOR"}},
            {JSType::JS_ARGUMENTS, {"JS_ARGUMENTS"}},
            {JSType::JS_ARRAY, {"Length", "TrackInfo", "JS_ARRAY"}},
            {JSType::JS_ARRAY_BUFFER, {"ArrayBufferData", "JS_ARRAY_BUFFER"}},
            {JSType::JS_ARRAY_ITERATOR, {"IteratedArray", "JS_ARRAY_ITERATOR"}},
            {JSType::JS_ASYNCITERATOR, {"Properties", "Elements", "JS_ASYNCITERATOR"}},
            {JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION, {"AsyncContext", "JS_ASYNC_AWAIT_STATUS_FUNCTION"}},
            {JSType::JS_ASYNC_FROM_SYNC_ITERATOR, {"SyncIteratorRecord", "JS_ASYNC_FROM_SYNC_ITERATOR"}},
            {JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION, {"Done", "JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION"}},
            {JSType::JS_ASYNC_FUNCTION, {"JS_ASYNC_FUNCTION"}},
            {JSType::JS_ASYNC_FUNC_OBJECT, {"Promise", "JS_ASYNC_FUNC_OBJECT"}},
            {JSType::JS_ASYNC_GENERATOR_FUNCTION, {"JS_ASYNC_GENERATOR_FUNCTION"}},
            {JSType::JS_ASYNC_GENERATOR_OBJECT, {"GeneratorContext", "AsyncGeneratorQueue", "GeneratorBrand",
                                                 "ResumeResult", "JS_ASYNC_GENERATOR_OBJECT"}},
            {JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN, {
                "AsyncGeneratorObject", "JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN"}},
            {JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION, {"JS_ASYNC_MODULE_FULFILLED_FUNCTION"}},
            {JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION, {"JS_ASYNC_MODULE_REJECTED_FUNCTION"}},
            {JSType::JS_BIGINT64_ARRAY, {"ViewedArrayBufferOrByteArray", "TypedArrayName", "JS_BIGINT64_ARRAY"}},
            {JSType::JS_BIGUINT64_ARRAY, {"ViewedArrayBufferOrByteArray", "TypedArrayName", "JS_BIGUINT64_ARRAY"}},
            {JSType::JS_BOUND_FUNCTION, {"BoundTarget", "BoundThis", "BoundArguments", "JS_BOUND_FUNCTION"}},
            {JSType::JS_CJS_EXPORTS, {"Exports", "JS_CJS_EXPORTS"}},
            {JSType::JS_CJS_MODULE, {"Id", "Path", "Exports", "Filename", "JS_CJS_MODULE"}},
            {JSType::JS_CJS_REQUIRE, {"Cache", "Parent", "JS_CJS_REQUIRE"}},
            {JSType::JS_COLLATOR, {"IcuFiled", "Locale", "Collation", "BoundCompare", "JS_COLLATOR"}},
            {JSType::JS_DATA_VIEW, {"DataView", "ViewedArrayBuffer", "JS_DATA_VIEW"}},
            {JSType::JS_DATE, {"TimeValue", "LocalOffset", "JS_DATE"}},
            {JSType::JS_DATE_TIME_FORMAT, {"Locale", "Calendar", "NumberingSystem", "TimeZone", "LocaleIcu",
                                           "SimpleDateTimeFormatIcu", "Iso8601",
                                           "BoundFormat", "JS_DATE_TIME_FORMAT"}},
            {JSType::JS_DISPLAYNAMES, {"Locale", "IcuLDN", "JS_DISPLAYNAMES"}},
            {JSType::JS_ERROR, {"Properties", "Elements", "JS_ERROR"}},
            {JSType::JS_EVAL_ERROR, {"Properties", "Elements", "JS_EVAL_ERROR"}},
            {JSType::JS_FINALIZATION_REGISTRY, {"CleanupCallback", "NoUnregister",
                                                "MaybeUnregister", "Next", "Prev", "JS_FINALIZATION_REGISTRY"}},
            {JSType::JS_FLOAT32_ARRAY, {"ViewedArrayBufferOrByteArray", "TypedArrayName", "JS_FLOAT32_ARRAY"}},
            {JSType::JS_FLOAT64_ARRAY, {"ViewedArrayBufferOrByteArray", "TypedArrayName", "JS_FLOAT64_ARRAY"}},
            {JSType::JS_FORIN_ITERATOR, {"Object", "CachedHClass", "Keys", "JS_FORIN_ITERATOR"}},
            {JSType::JS_FUNCTION, {"ProtoOrHClass", "LexicalEnv", "MachineCode", "BaselineCode", "RawProfileTypeInfo",
                                   "HomeObject", "Module", "JS_FUNCTION"}},
            {JSType::JS_FUNCTION_BASE, {"Method", "JS_FUNCTION_BASE"}},
            {JSType::JS_GENERATOR_CONTEXT, {"RegsArray", "Method", "This", "Acc",
                                            "GeneratorObject", "LexicalEnv", "JS_GENERATOR_CONTEXT"}},
            {JSType::JS_GENERATOR_FUNCTION, {"JS_GENERATOR_FUNCTION"}},
            {JSType::JS_GENERATOR_OBJECT, {"GeneratorContext", "ResumeResult", "JS_GENERATOR_OBJECT"}},
            {JSType::JS_GLOBAL_OBJECT, {"JS_GLOBAL_OBJECT"}},
            {JSType::JS_INT16_ARRAY, {"ViewedArrayBufferOrByteArray", "TypedArrayName", "JS_INT16_ARRAY"}},
            {JSType::JS_INT32_ARRAY, {"ViewedArrayBufferOrByteArray", "TypedArrayName", "JS_INT32_ARRAY"}},
            {JSType::JS_INT8_ARRAY, {"ViewedArrayBufferOrByteArray", "TypedArrayName", "JS_INT8_ARRAY"}},
            {JSType::JS_INTL, {"FallbackSymbol", "JS_INTL"}},
            {JSType::JS_INTL_BOUND_FUNCTION, {"NumberFormat", "DateTimeFormat",
                                              "Collator", "JS_INTL_BOUND_FUNCTION"}},
            {JSType::JS_ITERATOR, {"Properties", "Elements", "JS_ITERATOR"}},
            {JSType::JS_LIST_FORMAT, {"Locale", "IcuLF", "JS_LIST_FORMAT"}},
            {JSType::JS_LOCALE, {"IcuField", "JS_LOCALE"}},
            {JSType::JS_MAP, {"LinkedMap", "JS_MAP"}},
            {JSType::JS_MAP_ITERATOR, {"IteratorMap", "JS_MAP_ITERATOR"}},
            {JSType::JS_MODULE_NAMESPACE, {"Module", "Exports", "DeregisterProcession", "JS_MODULE_NAMESPACE"}},
            {JSType::JS_NATIVE_POINTER, {"BindingSize", "JS_NATIVE_POINTER"}},
            {JSType::JS_NUMBER_FORMAT, {"Locale", "NumberingSystem", "Currency", "Unit", "MinimumIntegerDigits",
                                              "MinimumFractionDigits", "MaximumFractionDigits",
                                              "MinimumSignificantDigits", "MaximumSignificantDigits",
                                              "UseGrouping", "BoundFormat", "IcuField", "JS_NUMBER_FORMAT"}},
            {JSType::JS_OBJECT, {"Properties", "Elements", "JS_OBJECT"}},
            {JSType::JS_OOM_ERROR, {"Properties", "Elements", "JS_OOM_ERROR"}},
            {JSType::JS_PLURAL_RULES, {"Locale", "MinimumIntegerDigits",
                                       "MinimumFractionDigits", "MaximumFractionDigits",
                                       "MinimumSignificantDigits", "MaximumSignificantDigits",
                                       "IcuPR", "IcuNF", "JS_PLURAL_RULES"}},
            {JSType::JS_PRIMITIVE_REF, {"Value", "JS_PRIMITIVE_REF"}},
            {JSType::JS_PROMISE, {"PromiseResult", "PromiseFulfillReactions", "PromiseRejectReactions", "JS_PROMISE"}},
            {JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION, {"Index", "Values", "Capabilities", "RemainingElements",
                                                               "AlreadyCalled",
                                                               "JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION"}},
            {JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION, {"AlreadyCalled", "Values", "Capability",
                                                               "RemainingElements",
                                                               "JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION"}},
            {JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION, {"Errors", "Capability", "RemainingElements",
                                                              "AlreadyCalled",
                                                              "JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION"}},
            {JSType::JS_PROMISE_EXECUTOR_FUNCTION, {"Capability", "JS_PROMISE_EXECUTOR_FUNCTION"}},
            {JSType::JS_PROMISE_FINALLY_FUNCTION, {"Constructor", "OnFinally", "JS_PROMISE_FINALLY_FUNCTION"}},
            {JSType::JS_PROMISE_REACTIONS_FUNCTION, {"Promise", "AlreadyResolved", "JS_PROMISE_REACTIONS_FUNCTION"}},
            {JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION, {"Result",
                                                                  "JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION"}},
            {JSType::JS_PROXY, {"Target", "Handler", "Method", "PrivateField", "JS_PROXY"}},
            {JSType::JS_PROXY_REVOC_FUNCTION, {"RevocableProxy", "JS_PROXY_REVOC_FUNCTION"}},
            {JSType::JS_RANGE_ERROR, {"Properties", "Elements", "JS_RANGE_ERROR"}},
            {JSType::JS_REALM, {"Value", "GlobalEnv", "JS_REALM"}},
            {JSType::JS_REFERENCE_ERROR, {"Properties", "Elements", "JS_REFERENCE_ERROR"}},
            {JSType::JS_REG_EXP, {"ByteCodeBuffer", "OriginalSource", "OriginalFlags", "GroupName", "JS_REG_EXP"}},
            {JSType::JS_REG_EXP_ITERATOR, {"IteratingRegExp", "IteratedString", "JS_REG_EXP_ITERATOR"}},
            {JSType::JS_RELATIVE_TIME_FORMAT, {"Locale", "NumberingSystem", "IcuField", "JS_RELATIVE_TIME_FORMAT"}},
            {JSType::JS_SEGMENTER, {"Locale", "IcuField", "JS_SEGMENTER"}},
            {JSType::JS_SEGMENTS, {"IcuField", "SegmentsString", "UnicodeString", "JS_SEGMENTS"}},
            {JSType::JS_SEGMENT_ITERATOR, {"IcuField", "IteratedString", "UnicodeString", "JS_SEGMENT_ITERATOR"}},
            {JSType::JS_SENDABLE_ARRAY_BUFFER, {"ArrayBufferData", "JS_SENDABLE_ARRAY_BUFFER"}},
            {JSType::JS_SET, {"LinkedSet", "JS_SET"}},
            {JSType::JS_SET_ITERATOR, {"IteratorSet", "JS_SET_ITERATOR"}},
            {JSType::JS_SHARED_ARRAY, {"TrackInfo", "JS_SHARED_ARRAY"}},
            {JSType::JS_SHARED_ARRAY_BUFFER, {"ArrayBufferData", "JS_SHARED_ARRAY_BUFFER"}},
            {JSType::JS_SHARED_ARRAY_ITERATOR, {"IteratedArray", "JS_SHARED_ARRAY_ITERATOR"}},
            {JSType::JS_SHARED_ASYNC_FUNCTION, {"JS_SHARED_ASYNC_FUNCTION"}},
            {JSType::JS_SHARED_BIGINT64_ARRAY, {"JS_SHARED_BIGINT64_ARRAY"}},
            {JSType::JS_SHARED_BIGUINT64_ARRAY, {"JS_SHARED_BIGUINT64_ARRAY"}},
            {JSType::JS_SHARED_FLOAT32_ARRAY, {"JS_SHARED_FLOAT32_ARRAY"}},
            {JSType::JS_SHARED_FLOAT64_ARRAY, {"JS_SHARED_FLOAT64_ARRAY"}},
            {JSType::JS_SHARED_FUNCTION, {"JS_SHARED_FUNCTION"}},
            {JSType::JS_SHARED_INT16_ARRAY, {"JS_SHARED_INT16_ARRAY"}},
            {JSType::JS_SHARED_INT32_ARRAY, {"JS_SHARED_INT32_ARRAY"}},
            {JSType::JS_SHARED_INT8_ARRAY, {"JS_SHARED_INT8_ARRAY"}},
            {JSType::JS_SHARED_MAP, {"LinkedMap", "JS_SHARED_MAP"}},
            {JSType::JS_SHARED_MAP_ITERATOR, {"IteratorMap", "JS_SHARED_MAP_ITERATOR"}},
            {JSType::JS_SHARED_OBJECT, {"JS_SHARED_OBJECT"}},
            {JSType::JS_SHARED_SET, {"LinkedSet", "JS_SHARED_SET"}},
            {JSType::JS_SHARED_SET_ITERATOR, {"IteratorSet", "JS_SHARED_SET_ITERATOR"}},
            {JSType::JS_SHARED_TYPED_ARRAY, {"JS_SHARED_TYPED_ARRAY"}},
            {JSType::JS_SHARED_UINT16_ARRAY, {"JS_SHARED_UINT16_ARRAY"}},
            {JSType::JS_SHARED_UINT32_ARRAY, {"JS_SHARED_UINT32_ARRAY"}},
            {JSType::JS_SHARED_UINT8_ARRAY, {"JS_SHARED_UINT8_ARRAY"}},
            {JSType::JS_SHARED_UINT8_CLAMPED_ARRAY, {"JS_SHARED_UINT8_CLAMPED_ARRAY"}},
            {JSType::JS_STRING_ITERATOR, {"IteratedString", "JS_STRING_ITERATOR"}},
            {JSType::JS_SYNTAX_ERROR, {"Properties", "Elements", "JS_SYNTAX_ERROR"}},
            {JSType::JS_TERMINATION_ERROR, {"Properties", "Elements", "JS_TERMINATION_ERROR"}},
            {JSType::JS_TYPED_ARRAY, {"ViewedArrayBufferOrByteArray", "TypedArrayName", "JS_TYPED_ARRAY"}},
            {JSType::JS_TYPE_ERROR, {"Properties", "Elements", "JS_TYPE_ERROR"}},
            {JSType::JS_UINT16_ARRAY, {"ViewedArrayBufferOrByteArray", "TypedArrayName", "JS_UINT16_ARRAY"}},
            {JSType::JS_UINT32_ARRAY, {"ViewedArrayBufferOrByteArray", "TypedArrayName", "JS_UINT32_ARRAY"}},
            {JSType::JS_UINT8_ARRAY, {"ViewedArrayBufferOrByteArray", "TypedArrayName", "JS_UINT8_ARRAY"}},
            {JSType::JS_UINT8_CLAMPED_ARRAY, {"ViewedArrayBufferOrByteArray",
                                              "TypedArrayName", "JS_UINT8_CLAMPED_ARRAY"}},
            {JSType::JS_URI_ERROR, {"Properties", "Elements", "JS_URI_ERROR"}},
            {JSType::JS_WEAK_MAP, {"LinkedMap", "JS_WEAK_MAP"}},
            {JSType::JS_WEAK_REF, {"WeakObject", "JS_WEAK_REF"}},
            {JSType::JS_WEAK_SET, {"LinkedSet", "JS_WEAK_SET"}},
            {JSType::LEXICAL_ENV, {"LEXICAL_ENV"}},
            {JSType::LINE_STRING, {"LINE_STRING"}},
            {JSType::LINKED_NODE, {"Next", "LINKED_NODE"}},
            {JSType::LOCAL_EXPORTENTRY_RECORD, {"ExportName", "LocalName", "LOCAL_EXPORTENTRY_RECORD"}},
            {JSType::MACHINE_CODE_OBJECT, {"MACHINE_CODE_OBJECT"}},
            {JSType::MARKER_CELL, {"MARKER_CELL"}},
            {JSType::METHOD, {"ConstantPool", "METHOD"}},
            {JSType::MICRO_JOB_QUEUE, {"PromiseJobQueue", "ScriptJobQueue", "MICRO_JOB_QUEUE"}},
            {JSType::MODULE_RECORD, {"MODULE_RECORD"}},
            {JSType::MUTANT_TAGGED_ARRAY, {"MUTANT_TAGGED_ARRAY"}},
            {JSType::NATIVE_MODULE_FAILURE_INFO, {"ArkNativeModuleFailureInfo", "NATIVE_MODULE_FAILURE_INFO"}},
            {JSType::PENDING_JOB, {"Job", "Arguments", "PENDING_JOB"}},
            {JSType::PROFILE_TYPE_INFO, {"PROFILE_TYPE_INFO"}},
            {JSType::PROFILE_TYPE_INFO_CELL_0, {"Value", "MachineCode", "BaselineCode", "Handle",
                                                "PROFILE_TYPE_INFO_CELL_0"}},
            {JSType::PROFILE_TYPE_INFO_CELL_1, {"Value", "MachineCode", "BaselineCode", "Handle",
                                                "PROFILE_TYPE_INFO_CELL_1"}},
            {JSType::PROFILE_TYPE_INFO_CELL_N, {"Value", "MachineCode", "BaselineCode", "Handle",
                                                "PROFILE_TYPE_INFO_CELL_N"}},
            {JSType::PROGRAM, {"MainFunction", "PROGRAM"}},
            {JSType::PROMISE_CAPABILITY, {"Promise", "Resolve", "Reject", "PROMISE_CAPABILITY"}},
            {JSType::PROMISE_ITERATOR_RECORD, {"Iterator", "PROMISE_ITERATOR_RECORD"}},
            {JSType::PROMISE_REACTIONS, {"PromiseCapability", "Handler", "PROMISE_REACTIONS"}},
            {JSType::PROMISE_RECORD, {"Value", "PROMISE_RECORD"}},
            {JSType::PROPERTY_BOX, {"Value", "PROPERTY_BOX"}},
            {JSType::PROTOTYPE_HANDLER, {"HandlerInfo", "ProtoCell", "Holder",
                                         "AccessorJSFunction", "PROTOTYPE_HANDLER"}},
            {JSType::PROTOTYPE_INFO, {"ChangeListener", "PROTOTYPE_INFO"}},
            {JSType::ENUM_CACHE, {"ENUM_CACHE"}},
            {JSType::PROTO_CHANGE_MARKER, {"PROTO_CHANGE_MARKER"}},
            {JSType::RB_TREENODE, {"Left", "Right", "IsRed", "RB_TREENODE"}},
            {JSType::RESOLVEDBINDING_RECORD, {"Module", "BindingName", "RESOLVEDBINDING_RECORD"}},
            {JSType::RESOLVEDINDEXBINDING_RECORD, {"Module", "Index", "BitField", "RESOLVEDINDEXBINDING_RECORD"}},
            {JSType::RESOLVEDRECORDBINDING_RECORD, {"ModuleRecord", "BindingName", "RESOLVEDRECORDBINDING_RECORD"}},
            {JSType::RESOLVEDRECORDINDEXBINDING_RECORD, {"ModuleRecord",
                                                         "AbcFileName", "RESOLVEDRECORDINDEXBINDING_RECORD"}},
            {JSType::RESOLVING_FUNCTIONS_RECORD, {"ResolveFunction", "RejectFunction", "RESOLVING_FUNCTIONS_RECORD"}},
            {JSType::SENDABLE_ENV, {"SENDABLE_ENV"}},
            {JSType::SFUNCTION_ENV, {"SFUNCTION_ENV"}},
            {JSType::JS_XREF_OBJECT, {"JS_XREF_OBJECT"}},
            {JSType::SLICED_STRING, {"Parent", "SLICED_STRING"}},
            {JSType::SOURCE_TEXT_MODULE_RECORD, {"Environment", "Namespace", "ModuleRequests", "RequestedModules",
                                                 "ImportEntries", "LocalExportEntries",
                                                 "IndirectExportEntries", "StarExportEntries",
                                                 "NameDictionary", "CycleRoot", "TopLevelCapability",
                                                 "AsyncParentModules", "SendableEnv", "Exception",
                                                 "SOURCE_TEXT_MODULE_RECORD"}},
            {JSType::STAR_EXPORTENTRY_RECORD, {"ModuleRequest", "STAR_EXPORTENTRY_RECORD"}},
            {JSType::STORE_TS_HANDLER, {"HandlerInfo", "ProtoCell", "Holder", "STORE_TS_HANDLER"}},
            {JSType::SYMBOL, {"Description", "SYMBOL"}},
            {JSType::TAGGED_ARRAY, {"Length", "Data", "TAGGED_ARRAY"}},
            {JSType::TAGGED_DICTIONARY, {"TAGGED_DICTIONARY"}},
            {JSType::TEMPLATE_MAP, {"TEMPLATE_MAP"}},
            {JSType::TRACK_INFO, {"CachedHClass", "CachedFunc", "TRACK_INFO"}},
            {JSType::TRANSITION_HANDLER, {"HandlerInfo", "TransitionHClass", "TRANSITION_HANDLER"}},
            {JSType::TRANS_WITH_PROTO_HANDLER, {"HandlerInfo", "TransitionHClass",
                                                "ProtoCell", "TRANS_WITH_PROTO_HANDLER"}},
            {JSType::TREE_STRING, {"First", "Second", "TREE_STRING"}},
            {JSType::COMPOSITE_BASE_CLASS, {"COMPOSITE_BASE_CLASS"}},
            {JSType::FREE_OBJECT_WITH_ONE_FIELD, {"FREE_OBJECT_WITH_ONE_FIELD"}},
            {JSType::FREE_OBJECT_WITH_NONE_FIELD, {"FREE_OBJECT_WITH_NONE_FIELD"}},
            {JSType::FREE_OBJECT_WITH_TWO_FIELD, {"FREE_OBJECT_WITH_TWO_FIELD"}},
            {JSType::VTABLE, {"VTABLE"}}
        };
        // { typeName: [all fields' start offset of this type in the same order as declared in .h files + endOffset] }
        // endOffset: LAST_OFFSET - (start offset of the first field of this type)
        fieldOffsetTable_ = {
            {JSType::HCLASS, {
                JSHClass::BIT_FIELD_OFFSET, JSHClass::BIT_FIELD1_OFFSET, JSHClass::PROTOTYPE_OFFSET,
                JSHClass::LAYOUT_OFFSET, JSHClass::TRANSTIONS_OFFSET, JSHClass::PARENT_OFFSET,
                JSHClass::PROTO_CHANGE_MARKER_OFFSET, JSHClass::PROTO_CHANGE_DETAILS_OFFSET,
                JSHClass::ENUM_CACHE_OFFSET,
                JSHClass::DEPENDENT_INFOS_OFFSET, JSHClass::PROFILE_TYPE_OFFSET,
                JSHClass::LAST_OFFSET - JSHClass::BIT_FIELD_OFFSET}},
            {JSType::ACCESSOR_DATA, {AccessorData::GETTER_OFFSET, AccessorData::SETTER_OFFSET,
                                     AccessorData::SIZE - AccessorData::GETTER_OFFSET}},
            {JSType::AOT_LITERAL_INFO, {TaggedArray::LAST_OFFSET - TaggedArray::LENGTH_OFFSET}},
            {JSType::ASYNC_GENERATOR_REQUEST, {
                AsyncGeneratorRequest::COMPLETION_OFFSET,
                AsyncGeneratorRequest::CAPABILITY_OFFSET,
                AsyncGeneratorRequest::SIZE - AsyncGeneratorRequest::COMPLETION_OFFSET}},
            {JSType::ASYNC_ITERATOR_RECORD, {AsyncIteratorRecord::ITERATOR_OFFSET,
                                             AsyncIteratorRecord::NEXTMETHOD_OFFSET,
                                             AsyncIteratorRecord::SIZE - AsyncIteratorRecord::ITERATOR_OFFSET}},
            {JSType::BIGINT, {BigInt::LAST_OFFSET - BigInt::LENGTH_OFFSET}},
            {JSType::BYTE_ARRAY, {ByteArray::LAST_OFFSET - ByteArray::ARRAY_LENGTH_OFFSET}},
            {JSType::CELL_RECORD, {CellRecord::WEAKREF_TARGET_OFFSET,
                                   CellRecord::HELD_VALUE_OFFSET,
                                   CellRecord::SIZE - CellRecord::WEAKREF_TARGET_OFFSET}},
            {JSType::CLASS_INFO_EXTRACTOR, {
                ClassInfoExtractor::PROTOTYPE_HCLASS_OFFSET,
                ClassInfoExtractor::NON_STATIC_PROPERTIES_OFFSET,
                ClassInfoExtractor::NON_STATIC_ELEMENTS_OFFSET,
                ClassInfoExtractor::CONSTRUCTOR_HCLASS_OFFSET,
                ClassInfoExtractor::STATIC_PROPERTIES_OFFSET,
                ClassInfoExtractor::STATIC_ELEMENTS_OFFSET,
                ClassInfoExtractor::CONSTRUCTOR_METHOD_OFFSET,
                ClassInfoExtractor::SIZE - ClassInfoExtractor::PROTOTYPE_HCLASS_OFFSET}},
            {JSType::CLASS_LITERAL, {ClassLiteral::ARRAY_OFFSET,
                                     ClassLiteral::SIZE - ClassLiteral::ARRAY_OFFSET}},
            {JSType::COMPLETION_RECORD, {CompletionRecord::VALUE_OFFSET,
                                         CompletionRecord::SIZE - CompletionRecord::VALUE_OFFSET}},
            {JSType::CONSTANT_POOL, {0}},
            {JSType::COW_MUTANT_TAGGED_ARRAY, {0}},
            {JSType::COW_TAGGED_ARRAY, {0}},
            {JSType::EXTRA_PROFILE_TYPE_INFO, {
                ExtraProfileTypeInfo::RECEIVER_OBJECT_OFFSET,
                ExtraProfileTypeInfo::HOLDER_OBJECT_OFFSET,
                ExtraProfileTypeInfo::LAST_OFFSET - ExtraProfileTypeInfo::RECEIVER_OBJECT_OFFSET}},
            {JSType::FUNCTION_TEMPLATE, {FunctionTemplate::METHOD_OFFSET,
                                         FunctionTemplate::MODULE_OFFSET,
                                         FunctionTemplate::RAW_PROFILE_TYPE_INFO_OFFSET,
                                         FunctionTemplate::SIZE - FunctionTemplate::METHOD_OFFSET}},
            {JSType::GLOBAL_ENV, {GlobalEnv::SIZE - GlobalEnv::HEADER_SIZE}},
            {JSType::IMPORTENTRY_RECORD, {ImportEntry::IMPORT_ENTRY_OFFSET,
                                          ImportEntry::LOCAL_NAME_OFFSET,
                                          ImportEntry::MODULE_REQUEST_INDEX_OFFSET,
                                          ImportEntry::SIZE - ImportEntry::IMPORT_ENTRY_OFFSET}},
            {JSType::INDIRECT_EXPORTENTRY_RECORD, {
                IndirectExportEntry::INDIRECT_EXPORT_ENTRY_OFFSET,
                IndirectExportEntry::IMPORT_NAME_OFFSET,
                IndirectExportEntry::MODULE_REQUEST_INDEX_OFFSET,
                IndirectExportEntry::SIZE - IndirectExportEntry::INDIRECT_EXPORT_ENTRY_OFFSET}},
            {JSType::INTERNAL_ACCESSOR, {InternalAccessor::GETTER_OFFSET,
                                         InternalAccessor::SETTER_OFFSET,
                                         InternalAccessor::SIZE - InternalAccessor::GETTER_OFFSET}},
            {JSType::JS_AGGREGATE_ERROR, {JSObject::PROPERTIES_OFFSET,
                                          JSObject::ELEMENTS_OFFSET,
                                          JSObject::SIZE - JSObject::PROPERTIES_OFFSET}},
            {JSType::JS_API_ARRAYLIST_ITERATOR, {
                JSAPIArrayListIterator::ITERATED_ARRAYLIST_OFFSET,
                JSAPIArrayListIterator::SIZE - JSAPIArrayListIterator::ITERATED_ARRAYLIST_OFFSET}},
            {JSType::JS_API_ARRAY_LIST, {JSAPIArrayList::LENGTH_OFFSET,
                                         JSAPIArrayList::SIZE - JSAPIArrayList::LENGTH_OFFSET}},
            {JSType::JS_API_BITVECTOR, {JSAPIBitVector::NATIVE_POINTER_OFFSET,
                                        JSAPIBitVector::LAST_OFFSET - JSAPIBitVector::NATIVE_POINTER_OFFSET}},
            {JSType::JS_API_BITVECTOR_ITERATOR, {
                JSAPIBitVectorIterator::ITERATED_BITVECTOR_OFFSET,
                JSAPIBitVectorIterator::SIZE - JSAPIBitVectorIterator::ITERATED_BITVECTOR_OFFSET}},
            {JSType::JS_API_DEQUE, {JSAPIDeque::ENDL_OFFSET - JSAPIDeque::FIRST_OFFSET}},
            {JSType::JS_API_DEQUE_ITERATOR, {JSAPIDequeIterator::ITERATED_DEQUE_OFFSET,
                                             JSAPIDequeIterator::SIZE - JSAPIDequeIterator::ITERATED_DEQUE_OFFSET}},
            {JSType::JS_API_HASHMAP_ITERATOR, {
                JSAPIHashMapIterator::ITERATED_HASHMAP_OFFSET,
                JSAPIHashMapIterator::TAGGED_QUEUE_OFFSET,
                JSAPIHashMapIterator::CURRENT_NODE_RESULT,
                JSAPIHashMapIterator::LAST_OFFSET - JSAPIHashMapIterator::ITERATED_HASHMAP_OFFSET}},
            {JSType::JS_API_HASHSET_ITERATOR, {
                JSAPIHashSetIterator::ITERATED_HASHSET_OFFSET,
                JSAPIHashSetIterator::TAGGED_QUEUE_OFFSET,
                JSAPIHashSetIterator::CURRENT_NODE_RESULT,
                JSAPIHashSetIterator::SIZE - JSAPIHashSetIterator::ITERATED_HASHSET_OFFSET}},
            {JSType::JS_API_HASH_MAP, {JSAPIHashMap::HASHMAP_TABLE_INDEX,
                                       JSAPIHashMap::SIZE - JSAPIHashMap::HASHMAP_TABLE_INDEX}},
            {JSType::JS_API_HASH_SET, {JSAPIHashSet::HASHSET_TABLE_INDEX,
                                       JSAPIHashSet::SIZE - JSAPIHashSet::HASHSET_TABLE_INDEX}},
            {JSType::JS_API_LIGHT_WEIGHT_MAP, {
                JSAPILightWeightMap::LWP_HASHES_OFFSET,
                JSAPILightWeightMap::LWP_KEYS_OFFSET,
                JSAPILightWeightMap::LWP_VALUES_OFFSET,
                JSAPILightWeightMap::SIZE - JSAPILightWeightMap::LWP_HASHES_OFFSET}},
            {JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR, {
                JSAPILightWeightMapIterator::ITERATED_LIGHT_WEIGHT_MAP_OFFSET,
                JSAPILightWeightMapIterator::LAST_OFFSET - JSAPILightWeightMapIterator::ITERATED_LIGHT_WEIGHT_MAP_OFFSET}},
            {JSType::JS_API_LIGHT_WEIGHT_SET, {
                JSAPILightWeightSet::HASHES_OFFSET,
                JSAPILightWeightSet::VALUES_OFFSET,
                JSAPILightWeightSet::SIZE - JSAPILightWeightSet::HASHES_OFFSET}},
            {JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR, {
                JSAPILightWeightSetIterator::ITERATED_LIGHT_WEIGHT_SET_OFFSET,
                JSAPILightWeightSetIterator::LAST_OFFSET - JSAPILightWeightSetIterator::ITERATED_LIGHT_WEIGHT_SET_OFFSET}},
            {JSType::JS_API_LINKED_LIST, {JSAPILinkedList::DOUBLE_LIST_OFFSET,
                                          JSAPILinkedList::SIZE - JSAPILinkedList::DOUBLE_LIST_OFFSET}},
            {JSType::JS_API_LINKED_LIST_ITERATOR, {
                JSAPILinkedListIterator::ITERATED_LINKED_LIST_OFFSET,
                JSAPILinkedListIterator::LAST_OFFSET - JSAPILinkedListIterator::ITERATED_LINKED_LIST_OFFSET}},
            {JSType::JS_API_LIST, {JSAPIList::SINGLY_LIST_OFFSET,
                                   JSAPIList::SIZE - JSAPIList::SINGLY_LIST_OFFSET}},
            {JSType::JS_API_LIST_ITERATOR, {
                JSAPIListIterator::ITERATED_LIST_OFFSET,
                JSAPIListIterator::LAST_OFFSET - JSAPIListIterator::ITERATED_LIST_OFFSET}},
            {JSType::JS_API_PLAIN_ARRAY, {JSAPIPlainArray::KEYS_OFFSET,
                                          JSAPIPlainArray::VALUES_OFFSET,
                                          JSAPIPlainArray::SIZE - JSAPIPlainArray::KEYS_OFFSET}},
            {JSType::JS_API_FAST_BUFFER, {JSAPIFastBuffer::TYPEDARRAY_OFFSET}},
            {JSType::JS_API_PLAIN_ARRAY_ITERATOR, {
                JSAPIPlainArrayIterator::ITERATED_PLAIN_ARRAY_OFFSET,
                JSAPIPlainArrayIterator::LAST_OFFSET - JSAPIPlainArrayIterator::ITERATED_PLAIN_ARRAY_OFFSET}},
            {JSType::JS_API_QUEUE, {JSAPIQueue::LENGTH_OFFSET,
                                    JSAPIQueue::LAST_OFFSET - JSAPIQueue::LENGTH_OFFSET}},
            {JSType::JS_API_QUEUE_ITERATOR, {
                JSAPIQueueIterator::ITERATED_QUEUE_OFFSET,
                JSAPIQueueIterator::SIZE - JSAPIQueueIterator::ITERATED_QUEUE_OFFSET}},
            {JSType::JS_API_STACK, {JSAPIStack::SIZE - JSAPIStack::TOP_OFFSET}},
            {JSType::JS_API_STACK_ITERATOR, {
                JSAPIStackIterator::ITERATED_STACK_OFFSET,
                JSAPIStackIterator::SIZE - JSAPIStackIterator::ITERATED_STACK_OFFSET}},
            {JSType::JS_API_TREEMAP_ITERATOR, {
                JSAPITreeMapIterator::ITERATED_MAP_OFFSET,
                JSAPITreeMapIterator::ITERATED_ENTRIES_OFFSET,
                JSAPITreeMapIterator::LAST_OFFSET - JSAPITreeMapIterator::ITERATED_MAP_OFFSET}},
            {JSType::JS_API_TREESET_ITERATOR, {
                JSAPITreeSetIterator::ITERATED_SET_OFFSET,
                JSAPITreeSetIterator::ITERATED_ENTRIES_OFFSET,
                JSAPITreeSetIterator::LAST_OFFSET - JSAPITreeSetIterator::ITERATED_SET_OFFSET}},
            {JSType::JS_API_TREE_MAP, {JSAPITreeMap::TREE_MAP_OFFSET,
                                       JSAPITreeMap::SIZE - JSAPITreeMap::TREE_MAP_OFFSET}},
            {JSType::JS_API_TREE_SET, {JSAPITreeSet::TREE_SET_OFFSET,
                                       JSAPITreeSet::SIZE - JSAPITreeSet::TREE_SET_OFFSET}},
            {JSType::JS_API_VECTOR, {JSAPIVector::SIZE - JSAPIVector::ELEMENT_COUNT_OFFSET}},
            {JSType::JS_API_VECTOR_ITERATOR, {
                JSAPIVectorIterator::ITERATED_VECTOR_OFFSET,
                JSAPIVectorIterator::SIZE - JSAPIVectorIterator::ITERATED_VECTOR_OFFSET}},
            {JSType::JS_ARGUMENTS, {0}},
            {JSType::JS_ARRAY, {JSArray::LENGTH_OFFSET,
                                JSArray::TRACK_INFO_OFFSET,
                                JSArray::SIZE - JSArray::LENGTH_OFFSET}},
            {JSType::JS_ARRAY_BUFFER, {JSArrayBuffer::DATA_OFFSET,
                                       JSArrayBuffer::LAST_OFFSET - JSArrayBuffer::DATA_OFFSET}},
            {JSType::JS_ARRAY_ITERATOR, {JSArrayIterator::ITERATED_ARRAY_OFFSET,
                                         JSArrayIterator::LAST_OFFSET - JSArrayIterator::ITERATED_ARRAY_OFFSET}},
            {JSType::JS_ASYNCITERATOR, {JSObject::PROPERTIES_OFFSET,
                                        JSObject::ELEMENTS_OFFSET,
                                        JSObject::SIZE - JSObject::PROPERTIES_OFFSET}},
            {JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION, {
                JSAsyncAwaitStatusFunction::ASYNC_CONTEXT_OFFSET,
                JSAsyncAwaitStatusFunction::SIZE - JSAsyncAwaitStatusFunction::ASYNC_CONTEXT_OFFSET}},
            {JSType::JS_ASYNC_FROM_SYNC_ITERATOR, {
                JSAsyncFromSyncIterator::SYNC_ITERATOR_RECORD,
                JSAsyncFromSyncIterator::SIZE - JSAsyncFromSyncIterator::SYNC_ITERATOR_RECORD}},
            {JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION, {
                JSAsyncFromSyncIterUnwarpFunction::DONE_OFFSET,
                JSAsyncFromSyncIterUnwarpFunction::SIZE - JSAsyncFromSyncIterUnwarpFunction::DONE_OFFSET}},
            {JSType::JS_ASYNC_FUNCTION, {JSAsyncFunction::SIZE - JSAsyncFunction::SIZE}},
            {JSType::JS_ASYNC_FUNC_OBJECT, {JSAsyncFuncObject::GENERATOR_PROMISE_OFFSET,
                                            JSAsyncFuncObject::SIZE - JSAsyncFuncObject::GENERATOR_PROMISE_OFFSET}},
            {JSType::JS_ASYNC_GENERATOR_FUNCTION, {JSAsyncGeneratorFunction::SIZE - JSAsyncGeneratorFunction::SIZE}},
            {JSType::JS_ASYNC_GENERATOR_OBJECT, {
                JSAsyncGeneratorObject::GENERATOR_CONTEXT_OFFSET,
                JSAsyncGeneratorObject::ASYNC_GENERATOR_QUEUE_OFFSET,
                JSAsyncGeneratorObject::GENERATOR_OFFSET,
                JSAsyncGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET,
                JSAsyncGeneratorObject::SIZE - JSAsyncGeneratorObject::GENERATOR_CONTEXT_OFFSET}},
            {JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN, {
                JSAsyncGeneratorResNextRetProRstFtn::ASYNC_GENERATOR_OBJECT_OFFSET,
                JSAsyncGeneratorResNextRetProRstFtn::SIZE - JSAsyncGeneratorResNextRetProRstFtn::ASYNC_GENERATOR_OBJECT_OFFSET}},
            {JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION, {
                JSAsyncModuleFulfilledFunction::SIZE - JSAsyncModuleFulfilledFunction::SIZE}},
            {JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION, {
                JSAsyncModuleRejectedFunction::SIZE - JSAsyncModuleRejectedFunction::SIZE}},
            {JSType::JS_BIGINT64_ARRAY, {JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                                         JSTypedArray::TYPED_ARRAY_NAME_OFFSET,
                                         JSTypedArray::SIZE - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET}},
            {JSType::JS_BIGUINT64_ARRAY, {JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                                          JSTypedArray::TYPED_ARRAY_NAME_OFFSET,
                                          JSTypedArray::SIZE - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET}},
            {JSType::JS_BOUND_FUNCTION, {JSBoundFunction::BOUND_TARGET_OFFSET,
                                         JSBoundFunction::BOUND_THIS_OFFSET,
                                         JSBoundFunction::BOUND_ARGUMENTS_OFFSET,
                                         JSBoundFunction::SIZE - JSBoundFunction::BOUND_TARGET_OFFSET}},
            {JSType::JS_CJS_EXPORTS, {CjsExports::JS_CJS_EXPORTS_OFFSET,
                                      CjsExports::SIZE - CjsExports::JS_CJS_EXPORTS_OFFSET}},
            {JSType::JS_CJS_MODULE, {CjsModule::JS_CJS_MODULE_OFFSET,
                                     CjsModule::ID_OFFSET,
                                     CjsModule::PATH_OFFSET,
                                     CjsModule::EXPORTS_OFFSET,
                                     CjsModule::SIZE - CjsModule::JS_CJS_MODULE_OFFSET}},
            {JSType::JS_CJS_REQUIRE, {CjsRequire::JS_CJS_REQUIRE_OFFSET,
                                      CjsRequire::CACHE_OFFSET,
                                      CjsRequire::SIZE - CjsRequire::JS_CJS_REQUIRE_OFFSET}},
            {JSType::JS_COLLATOR, {JSCollator::ICU_FIELD_OFFSET,
                                   JSCollator::LOCALE_OFFSET,
                                   JSCollator::COLLATION_OFFSET,
                                   JSCollator::BOUND_COMPARE_OFFSET,
                                   JSCollator::SIZE - JSCollator::ICU_FIELD_OFFSET}},
            {JSType::JS_DATA_VIEW, {JSDataView::DATA_VIEW_OFFSET,
                                    JSDataView::VIEW_ARRAY_BUFFER_OFFSET,
                                    JSDataView::LAST_OFFSET - JSDataView::DATA_VIEW_OFFSET}},
            {JSType::JS_DATE, {JSDate::TIME_VALUE_OFFSET,
                               JSDate::LOCAL_TIME_OFFSET,
                               JSDate::SIZE - JSDate::TIME_VALUE_OFFSET}},
            {JSType::JS_DATE_TIME_FORMAT, {JSDateTimeFormat::LOCALE_OFFSET,
                                           JSDateTimeFormat::CALENDAR_OFFSET,
                                           JSDateTimeFormat::NUMBER_STRING_SYSTEM_OFFSET,
                                           JSDateTimeFormat::TIME_ZONE_OFFSET,
                                           JSDateTimeFormat::LOCALE_ICU_OFFSET,
                                           JSDateTimeFormat::SIMPLE_DATE_TIME_FORMAT_ICU_OFFSET,
                                           JSDateTimeFormat::ISO8601_OFFSET,
                                           JSDateTimeFormat::BOUND_FORMAT_OFFSET,
                                           JSDateTimeFormat::SIZE - JSDateTimeFormat::LOCALE_OFFSET}},
            {JSType::JS_DISPLAYNAMES, {JSDisplayNames::LOCALE_OFFSET,
                                       JSDisplayNames::ICU_LOCALE_DISPPLAY_NAMES,
                                       JSDisplayNames::SIZE - JSDisplayNames::LOCALE_OFFSET}},
            {JSType::JS_ERROR, {JSObject::PROPERTIES_OFFSET,
                                JSObject::ELEMENTS_OFFSET,
                                JSObject::SIZE - JSObject::PROPERTIES_OFFSET}},
            {JSType::JS_EVAL_ERROR, {JSObject::PROPERTIES_OFFSET,
                                     JSObject::ELEMENTS_OFFSET,
                                     JSObject::SIZE - JSObject::PROPERTIES_OFFSET}},
            {JSType::JS_FINALIZATION_REGISTRY, {
                JSFinalizationRegistry::CLEANUP_CALLBACK_OFFSET,
                JSFinalizationRegistry::NO_UNREGISTER_OFFSET,
                JSFinalizationRegistry::MAYBE_UNREGISTER_OFFSET,
                JSFinalizationRegistry::NEXT_OFFSET,
                JSFinalizationRegistry::PREV_OFFSET,
                JSFinalizationRegistry::SIZE - JSFinalizationRegistry::CLEANUP_CALLBACK_OFFSET}},
            {JSType::JS_FLOAT32_ARRAY, {JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                                        JSTypedArray::TYPED_ARRAY_NAME_OFFSET,
                                        JSTypedArray::SIZE - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET}},
            {JSType::JS_FLOAT64_ARRAY, {JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                                        JSTypedArray::TYPED_ARRAY_NAME_OFFSET,
                                        JSTypedArray::SIZE - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET}},
            {JSType::JS_FORIN_ITERATOR, {JSForInIterator::OBJECT_OFFSET,
                                         JSForInIterator::CACHED_HCLASS_OFFSET,
                                         JSForInIterator::KEYS_OFFSET,
                                         JSForInIterator::LAST_OFFSET - JSForInIterator::OBJECT_OFFSET}},
            {JSType::JS_FUNCTION, {JSFunction::PROTO_OR_DYNCLASS_OFFSET,
                                   JSFunction::LEXICAL_ENV_OFFSET,
                                   JSFunction::MACHINECODE_OFFSET,
                                   JSFunction::BASELINECODE_OFFSET,
                                   JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET,
                                   JSFunction::HOME_OBJECT_OFFSET,
                                   JSFunction::ECMA_MODULE_OFFSET,
                                   JSFunction::LAST_OFFSET - JSFunction::PROTO_OR_DYNCLASS_OFFSET}},
            {JSType::JS_FUNCTION_BASE, {JSFunctionBase::METHOD_OFFSET,
                                        JSFunctionBase::LAST_OFFSET - JSFunctionBase::METHOD_OFFSET}},
            {JSType::JS_GENERATOR_CONTEXT, {
                GeneratorContext::GENERATOR_REGS_ARRAY_OFFSET,
                GeneratorContext::GENERATOR_METHOD_OFFSET,
                GeneratorContext::GENERATOR_THIS_OFFSET,
                GeneratorContext::GENERATOR_ACC_OFFSET,
                GeneratorContext::GENERATOR_GENERATOR_OBJECT_OFFSET,
                GeneratorContext::GENERATOR_LEXICALENV_OFFSET,
                GeneratorContext::LAST_OFFSET - GeneratorContext::GENERATOR_REGS_ARRAY_OFFSET}},
            {JSType::JS_GENERATOR_FUNCTION, {JSGeneratorFunction::SIZE - JSGeneratorFunction::SIZE}},
            {JSType::JS_GENERATOR_OBJECT, {
                JSGeneratorObject::GENERATOR_CONTEXT_OFFSET,
                JSGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET,
                JSGeneratorObject::SIZE - JSGeneratorObject::GENERATOR_CONTEXT_OFFSET}},
            {JSType::JS_GLOBAL_OBJECT, {JSGlobalObject::SIZE - JSGlobalObject::SIZE}},
            {JSType::JS_INT16_ARRAY, {JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                                      JSTypedArray::TYPED_ARRAY_NAME_OFFSET,
                                      JSTypedArray::SIZE - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET}},
            {JSType::JS_INT32_ARRAY, {JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                                      JSTypedArray::TYPED_ARRAY_NAME_OFFSET,
                                      JSTypedArray::SIZE - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET}},
            {JSType::JS_INT8_ARRAY, {JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                                     JSTypedArray::TYPED_ARRAY_NAME_OFFSET,
                                     JSTypedArray::SIZE - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET}},
            {JSType::JS_INTL, {JSIntl::FALLBACK_SYMBOL,
                               JSIntl::SIZE - JSIntl::FALLBACK_SYMBOL}},
            {JSType::JS_INTL_BOUND_FUNCTION, {JSIntlBoundFunction::NUMBER_FORMAT_OFFSET,
                                              JSIntlBoundFunction::DATETIME_FORMAT_OFFSET,
                                              JSIntlBoundFunction::COLLATOR_OFFSET,
                                              JSIntlBoundFunction::SIZE - JSIntlBoundFunction::NUMBER_FORMAT_OFFSET}},
            {JSType::JS_ITERATOR, {JSObject::PROPERTIES_OFFSET,
                                   JSObject::ELEMENTS_OFFSET,
                                   JSObject::SIZE - JSObject::PROPERTIES_OFFSET}},
            {JSType::JS_LIST_FORMAT, {JSListFormat::LOCALE_OFFSET,
                                      JSListFormat::ICU_LIST_FORMAT,
                                      JSListFormat::SIZE - JSListFormat::LOCALE_OFFSET}},
            {JSType::JS_LOCALE, {JSLocale::ICU_FIELD_OFFSET,
                                 JSLocale::SIZE - JSLocale::ICU_FIELD_OFFSET}},
            {JSType::JS_MAP, {JSMap::LINKED_MAP_OFFSET,
                              JSMap::SIZE - JSMap::LINKED_MAP_OFFSET}},
            {JSType::JS_MAP_ITERATOR, {JSMapIterator::ITERATED_MAP_OFFSET,
                                       JSMapIterator::LAST_OFFSET - JSMapIterator::ITERATED_MAP_OFFSET}},
            {JSType::JS_MODULE_NAMESPACE, {ModuleNamespace::MODULE_OFFSET,
                                           ModuleNamespace::EXPORTS_OFFSET,
                                           ModuleNamespace::DEREGISTER_PROCESSION_OFFSET,
                                           ModuleNamespace::SIZE - ModuleNamespace::MODULE_OFFSET}},
            {JSType::JS_NATIVE_POINTER, {JSNativePointer::DATA_SIZE_OFFSET,
                                         JSNativePointer::LAST_OFFSET - JSNativePointer::POINTER_OFFSET}},
            {JSType::JS_NUMBER_FORMAT, {JSNumberFormat::LOCALE_OFFSET,
                                        JSNumberFormat::NUMBER_STRING_SYSTEM_OFFSET,
                                        JSNumberFormat::CURRENCY_OFFSET,
                                        JSNumberFormat::UNIT_OFFSET,
                                        JSNumberFormat::MINIMUM_INTEGER_DIGITS_OFFSET,
                                        JSNumberFormat::MINIMUM_FRACTION_DIGITS_OFFSET,
                                        JSNumberFormat::MAXIMUM_FRACTION_DIGITS_OFFSET,
                                        JSNumberFormat::MINIMUM_SIGNIFICANT_DIGITS_OFFSET,
                                        JSNumberFormat::MAXIMUM_SIGNIFICANT_DIGITS_OFFSET,
                                        JSNumberFormat::USER_GROUPING_OFFSET,
                                        JSNumberFormat::BOUND_FORMAT_OFFSET,
                                        JSNumberFormat::ICU_FIELD_OFFSET,
                                        JSNumberFormat::SIZE - JSNumberFormat::LOCALE_OFFSET}},
            {JSType::JS_OBJECT, {JSObject::PROPERTIES_OFFSET,
                                 JSObject::ELEMENTS_OFFSET,
                                 JSObject::SIZE - JSObject::PROPERTIES_OFFSET}},
            {JSType::JS_OOM_ERROR, {JSObject::PROPERTIES_OFFSET,
                                    JSObject::ELEMENTS_OFFSET,
                                    JSObject::SIZE - JSObject::PROPERTIES_OFFSET}},
            {JSType::JS_PLURAL_RULES, {JSPluralRules::LOCALE_OFFSET,
                                       JSPluralRules::MINIMUM_INTEGER_DIGITS_OFFSET,
                                       JSPluralRules::MINIMUM_FRACTION_DIGITS_OFFSET,
                                       JSPluralRules::MAXIMUM_FRACTION_DIGITS_OFFSET,
                                       JSPluralRules::MININUM_SIGNIFICANT_DIGITS_OFFSET,
                                       JSPluralRules::MAXINUM_SIGNIFICANT_DIGITS_OFFSET,
                                       JSPluralRules::ICU_PLURAL_RULES_OFFSET,
                                       JSPluralRules::ICU_NUMBER_FORMAT_OFFSET,
                                       JSPluralRules::SIZE - JSPluralRules::LOCALE_OFFSET}},
            {JSType::JS_PRIMITIVE_REF, {JSPrimitiveRef::VALUE_OFFSET,
                                        JSPrimitiveRef::SIZE - JSPrimitiveRef::VALUE_OFFSET}},
            {JSType::JS_PROMISE, {JSPromise::PROMISE_RESULT_OFFSET,
                                  JSPromise::PROMISE_FULFILL_REACTIONS_OFFSET,
                                  JSPromise::PROMISE_REJECT_REACTIONS_OFFSET,
                                  JSPromise::SIZE - JSPromise::PROMISE_RESULT_OFFSET}},
            {JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION, {
                JSPromiseAllResolveElementFunction::INDEX_OFFSET,
                JSPromiseAllResolveElementFunction::VALUES_OFFSET,
                JSPromiseAllResolveElementFunction::CAPABILITIES_OFFSET,
                JSPromiseAllResolveElementFunction::REMAINING_ELEMENTS_OFFSET,
                JSPromiseAllResolveElementFunction::ALREADY_CALLED_OFFSET,
                JSPromiseAllResolveElementFunction::SIZE - JSPromiseAllResolveElementFunction::INDEX_OFFSET}},
            {JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION, {
                JSPromiseAllSettledElementFunction::ALREADY_CALLED_OFFSET,
                JSPromiseAllSettledElementFunction::VALUES_OFFSET,
                JSPromiseAllSettledElementFunction::CAPABILITY_OFFSET,
                JSPromiseAllSettledElementFunction::REMAINING_ELEMENTS_OFFSET,
                JSPromiseAllSettledElementFunction::SIZE - JSPromiseAllSettledElementFunction::ALREADY_CALLED_OFFSET}},
            {JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION, {
                JSPromiseAnyRejectElementFunction::ERRORS_OFFSET,
                JSPromiseAnyRejectElementFunction::CAPABILITY_OFFSET,
                JSPromiseAnyRejectElementFunction::REMAINING_ELEMENTS_OFFSET,
                JSPromiseAnyRejectElementFunction::ALREADY_CALLED_OFFSET,
                JSPromiseAnyRejectElementFunction::SIZE - JSPromiseAnyRejectElementFunction::ERRORS_OFFSET}},
            {JSType::JS_PROMISE_EXECUTOR_FUNCTION, {
                JSPromiseExecutorFunction::CAPABILITY_OFFSET,
                JSPromiseExecutorFunction::SIZE - JSPromiseExecutorFunction::CAPABILITY_OFFSET}},
            {JSType::JS_PROMISE_FINALLY_FUNCTION, {
                JSPromiseFinallyFunction::CONSTRUCTOR_OFFSET,
                JSPromiseFinallyFunction::ONFINALLY_OFFSET,
                JSPromiseFinallyFunction::SIZE - JSPromiseFinallyFunction::CONSTRUCTOR_OFFSET}},
            {JSType::JS_PROMISE_REACTIONS_FUNCTION, {
                JSPromiseReactionsFunction::PROMISE_OFFSET,
                JSPromiseReactionsFunction::ALREADY_RESOLVED_OFFSET,
                JSPromiseReactionsFunction::SIZE - JSPromiseReactionsFunction::PROMISE_OFFSET}},
            {JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION, {
                JSPromiseValueThunkOrThrowerFunction::RESULT_OFFSET,
                JSPromiseValueThunkOrThrowerFunction::SIZE - JSPromiseValueThunkOrThrowerFunction::RESULT_OFFSET}},
            {JSType::JS_PROXY, {JSProxy::TARGET_OFFSET, JSProxy::HANDLER_OFFSET, JSProxy::METHOD_OFFSET,
                                JSProxy::PRIVATE_FIELD_OFFSET, JSProxy::SIZE - JSProxy::TARGET_OFFSET}},
            {JSType::JS_PROXY_REVOC_FUNCTION, {
                JSProxyRevocFunction::REVOCABLE_PROXY_OFFSET,
                JSProxyRevocFunction::SIZE - JSProxyRevocFunction::REVOCABLE_PROXY_OFFSET}},
            {JSType::JS_RANGE_ERROR, {JSObject::PROPERTIES_OFFSET, JSObject::ELEMENTS_OFFSET,
                                      JSObject::SIZE - JSObject::PROPERTIES_OFFSET}},
            {JSType::JS_REALM, {JSRealm::VALUE_OFFSET, JSRealm::GLOBAL_ENV_OFFSET,
                                JSRealm::SIZE - JSRealm::VALUE_OFFSET}},
            {JSType::JS_REFERENCE_ERROR, {JSObject::PROPERTIES_OFFSET, JSObject::ELEMENTS_OFFSET,
                                          JSObject::SIZE - JSObject::PROPERTIES_OFFSET}},
            {JSType::JS_REG_EXP, {JSRegExp::REGEXP_BYTE_CODE_OFFSET, JSRegExp::ORIGINAL_SOURCE_OFFSET,
                                  JSRegExp::ORIGINAL_FLAGS_OFFSET, JSRegExp::GROUP_NAME_OFFSET,
                                  JSRegExp::SIZE - JSRegExp::REGEXP_BYTE_CODE_OFFSET}},
            {JSType::JS_REG_EXP_ITERATOR, {
                JSRegExpIterator::ITERATING_REGEXP_OFFSET,
                JSRegExpIterator::ITERATED_STRING_OFFSET,
                JSRegExpIterator::SIZE - JSRegExpIterator::ITERATING_REGEXP_OFFSET}},
            {JSType::JS_RELATIVE_TIME_FORMAT, {
                JSRelativeTimeFormat::LOCALE_OFFSET,
                JSRelativeTimeFormat::NUMBERING_SYSTEM_OFFSET,
                JSRelativeTimeFormat::ICU_FIELD_OFFSET,
                JSRelativeTimeFormat::SIZE - JSRelativeTimeFormat::LOCALE_OFFSET}},
            {JSType::JS_SEGMENTER, {JSSegmenter::LOCALE_OFFSET,
                                    JSSegmenter::ICU_FIELD_OFFSET,
                                    JSSegmenter::SIZE - JSSegmenter::LOCALE_OFFSET}},
            {JSType::JS_SEGMENTS, {JSSegments::ICU_FIELD_OFFSET, JSSegments::SEGMENTS_STRING_OFFSET,
                                   JSSegments::UNICODE_STRING_OFFSET,
                                   JSSegments::SIZE - JSSegments::ICU_FIELD_OFFSET}},
            {JSType::JS_SEGMENT_ITERATOR, {JSSegmentIterator::ICU_FIELD_OFFSET,
                                           JSSegmentIterator::SEGMENTS_STRING_OFFSET,
                                           JSSegmentIterator::UNICODE_STRING_OFFSET,
                                           JSSegmentIterator::SIZE - JSSegmentIterator::ICU_FIELD_OFFSET}},
            {JSType::JS_SENDABLE_ARRAY_BUFFER, {
                JSSendableArrayBuffer::DATA_OFFSET,
                JSSendableArrayBuffer::LAST_OFFSET - JSSendableArrayBuffer::DATA_OFFSET}},
            {JSType::JS_SET, {JSSet::LINKED_SET_OFFSET, JSSet::SIZE - JSSet::LINKED_SET_OFFSET}},
            {JSType::JS_SET_ITERATOR, {JSSetIterator::ITERATED_SET_OFFSET,
                                       JSSetIterator::LAST_OFFSET - JSSetIterator::ITERATED_SET_OFFSET}},
            {JSType::JS_SHARED_ARRAY, {JSSharedArray::TRACK_INFO_OFFSET,
                                       JSSharedArray::SIZE - JSSharedArray::LENGTH_OFFSET}},
            {JSType::JS_SHARED_ARRAY_BUFFER, {JSArrayBuffer::DATA_OFFSET,
                                              JSArrayBuffer::LAST_OFFSET - JSArrayBuffer::DATA_OFFSET}},
            {JSType::JS_SHARED_ARRAY_ITERATOR, {
                JSSharedArrayIterator::ITERATED_ARRAY_OFFSET,
                JSSharedArrayIterator::LAST_OFFSET - JSSharedArrayIterator::ITERATED_ARRAY_OFFSET}},
            {JSType::JS_SHARED_ASYNC_FUNCTION, {JSAsyncFunction::SIZE - JSAsyncFunction::SIZE}},
            {JSType::JS_SHARED_BIGINT64_ARRAY, {
                JSSharedTypedArray::SIZE - JSSharedTypedArray::MOD_RECORD_OFFSET}},
            {JSType::JS_SHARED_BIGUINT64_ARRAY, {
                JSSharedTypedArray::SIZE - JSSharedTypedArray::MOD_RECORD_OFFSET}},
            {JSType::JS_SHARED_FLOAT32_ARRAY, {
                JSSharedTypedArray::SIZE - JSSharedTypedArray::MOD_RECORD_OFFSET}},
            {JSType::JS_SHARED_FLOAT64_ARRAY, {
                JSSharedTypedArray::SIZE - JSSharedTypedArray::MOD_RECORD_OFFSET}},
            {JSType::JS_SHARED_FUNCTION, {JSSharedFunction::SIZE - JSSharedFunction::SIZE}},
            {JSType::JS_SHARED_INT16_ARRAY, {
                JSSharedTypedArray::SIZE - JSSharedTypedArray::MOD_RECORD_OFFSET}},
            {JSType::JS_SHARED_INT32_ARRAY, {
                JSSharedTypedArray::SIZE - JSSharedTypedArray::MOD_RECORD_OFFSET}},
            {JSType::JS_SHARED_INT8_ARRAY, {
                JSSharedTypedArray::SIZE - JSSharedTypedArray::MOD_RECORD_OFFSET}},
            {JSType::JS_SHARED_MAP, {JSSharedMap::LINKED_MAP_OFFSET,
                                     JSSharedMap::SIZE - JSSharedMap::LINKED_MAP_OFFSET}},
            {JSType::JS_SHARED_MAP_ITERATOR, {
                JSSharedMapIterator::ITERATED_MAP_OFFSET,
                JSSharedMapIterator::LAST_OFFSET - JSSharedMapIterator::ITERATED_MAP_OFFSET}},
            {JSType::JS_SHARED_OBJECT, {JSSharedObject::SIZE - JSSharedObject::SIZE}},
            {JSType::JS_SHARED_SET, {JSSharedSet::LINKED_SET_OFFSET,
                                     JSSharedSet::SIZE - JSSharedSet::LINKED_SET_OFFSET}},
            {JSType::JS_SHARED_SET_ITERATOR, {
                JSSharedSetIterator::ITERATED_SET_OFFSET,
                JSSharedSetIterator::LAST_OFFSET - JSSharedSetIterator::ITERATED_SET_OFFSET}},
            {JSType::JS_SHARED_TYPED_ARRAY, {
                JSSharedTypedArray::SIZE - JSSharedTypedArray::MOD_RECORD_OFFSET}},
            {JSType::JS_SHARED_UINT16_ARRAY, {
                JSSharedTypedArray::SIZE - JSSharedTypedArray::MOD_RECORD_OFFSET}},
            {JSType::JS_SHARED_UINT32_ARRAY, {
                JSSharedTypedArray::SIZE - JSSharedTypedArray::MOD_RECORD_OFFSET}},
            {JSType::JS_SHARED_UINT8_ARRAY, {
                JSSharedTypedArray::SIZE - JSSharedTypedArray::MOD_RECORD_OFFSET}},
            {JSType::JS_SHARED_UINT8_CLAMPED_ARRAY, {
                JSSharedTypedArray::SIZE - JSSharedTypedArray::MOD_RECORD_OFFSET}},
            {JSType::JS_STRING_ITERATOR, {
                JSStringIterator::ITERATED_STRING_OFFSET,
                JSStringIterator::SIZE - JSStringIterator::ITERATED_STRING_OFFSET}},
            {JSType::JS_SYNTAX_ERROR, {JSObject::PROPERTIES_OFFSET, JSObject::ELEMENTS_OFFSET,
                                       JSObject::SIZE - JSObject::PROPERTIES_OFFSET}},
            {JSType::JS_TERMINATION_ERROR, {JSObject::PROPERTIES_OFFSET, JSObject::ELEMENTS_OFFSET,
                                            JSObject::SIZE - JSObject::PROPERTIES_OFFSET}},
            {JSType::JS_TYPED_ARRAY, {JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET, JSTypedArray::TYPED_ARRAY_NAME_OFFSET,
                                      JSTypedArray::SIZE - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET}},
            {JSType::JS_TYPE_ERROR, {JSObject::PROPERTIES_OFFSET, JSObject::ELEMENTS_OFFSET,
                                     JSObject::SIZE - JSObject::PROPERTIES_OFFSET}},
            {JSType::JS_UINT16_ARRAY, {
                JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET, JSTypedArray::TYPED_ARRAY_NAME_OFFSET,
                JSTypedArray::SIZE - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET}},
            {JSType::JS_UINT32_ARRAY, {
                JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET, JSTypedArray::TYPED_ARRAY_NAME_OFFSET,
                JSTypedArray::SIZE - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET}},
            {JSType::JS_UINT8_ARRAY, {
                JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET, JSTypedArray::TYPED_ARRAY_NAME_OFFSET,
                JSTypedArray::SIZE - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET}},
            {JSType::JS_UINT8_CLAMPED_ARRAY, {
                JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                JSTypedArray::TYPED_ARRAY_NAME_OFFSET,
                JSTypedArray::SIZE - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET}},
            {JSType::JS_URI_ERROR, {JSObject::PROPERTIES_OFFSET, JSObject::ELEMENTS_OFFSET,
                                    JSObject::SIZE - JSObject::PROPERTIES_OFFSET}},
            {JSType::JS_WEAK_MAP, {
                JSWeakMap::LINKED_MAP_OFFSET, JSWeakMap::SIZE - JSWeakMap::LINKED_MAP_OFFSET}},
            {JSType::JS_WEAK_REF, {
                JSWeakRef::WEAK_OBJECT_OFFSET, JSWeakRef::SIZE - JSWeakRef::WEAK_OBJECT_OFFSET}},
            {JSType::JS_WEAK_SET, {
                JSWeakSet::LINKED_SET_OFFSET, JSWeakSet::SIZE - JSWeakSet::LINKED_SET_OFFSET}},
            {JSType::LEXICAL_ENV, {TaggedArray::SIZE - TaggedArray::SIZE}},
            {JSType::LINE_STRING, {LineString::SIZE - LineString::SIZE}},
            {JSType::LINKED_NODE, {LinkedNode::NEXT_OFFSET, LinkedNode::LAST_OFFSET - LinkedNode::NEXT_OFFSET}},
            {JSType::LOCAL_EXPORTENTRY_RECORD, {
                LocalExportEntry::LOCAL_EXPORT_ENTRY_OFFSET,
                LocalExportEntry::LOCAL_NAME_OFFSET,
                LocalExportEntry::SIZE - LocalExportEntry::LOCAL_EXPORT_ENTRY_OFFSET}},
            {JSType::MACHINE_CODE_OBJECT, {MachineCode::LAST_OFFSET - MachineCode::INS_SIZE_OFFSET}},
            {JSType::MARKER_CELL, {MarkerCell::SIZE - MarkerCell::BIT_FIELD_OFFSET}},
            {JSType::METHOD, {Method::CONSTANT_POOL_OFFSET, Method::LAST_OFFSET - Method::CONSTANT_POOL_OFFSET}},
            {JSType::MICRO_JOB_QUEUE, {job::MicroJobQueue::PROMISE_JOB_QUEUE_OFFSET,
                                       job::MicroJobQueue::SCRIPT_JOB_QUEUE_OFFSET,
                                       job::MicroJobQueue::SIZE - job::MicroJobQueue::PROMISE_JOB_QUEUE_OFFSET}},
            {JSType::MODULE_RECORD, {ModuleRecord::SIZE - ModuleRecord::SIZE}},
            {JSType::MUTANT_TAGGED_ARRAY, {MutantTaggedArray::SIZE - MutantTaggedArray::SIZE}},
            {JSType::NATIVE_MODULE_FAILURE_INFO, {
                NativeModuleFailureInfo::ARK_NATIVE_MODULE_FAILURE_INFO_OFFSET,
                NativeModuleFailureInfo::SIZE - NativeModuleFailureInfo::ARK_NATIVE_MODULE_FAILURE_INFO_OFFSET}},
            {JSType::PENDING_JOB, {job::PendingJob::JOB_OFFSET,
                                   job::PendingJob::ARGUMENT_OFFSET,
                                   job::PendingJob::SIZE - job::PendingJob::ARGUMENT_OFFSET}},
            {JSType::PROFILE_TYPE_INFO, {ProfileTypeInfo::SIZE - ProfileTypeInfo::SIZE}},
            {JSType::PROFILE_TYPE_INFO_CELL_0, {
                ProfileTypeInfoCell::VALUE_OFFSET,
                ProfileTypeInfoCell::MACHINE_CODE_OFFSET,
                ProfileTypeInfoCell::BASELINE_CODE_OFFSET,
                ProfileTypeInfoCell::HANDLE_OFFSET,
                ProfileTypeInfoCell::LAST_OFFSET - ProfileTypeInfoCell::VALUE_OFFSET}},
            {JSType::PROFILE_TYPE_INFO_CELL_1, {
                ProfileTypeInfoCell::VALUE_OFFSET,
                ProfileTypeInfoCell::MACHINE_CODE_OFFSET,
                ProfileTypeInfoCell::BASELINE_CODE_OFFSET,
                ProfileTypeInfoCell::HANDLE_OFFSET,
                ProfileTypeInfoCell::LAST_OFFSET - ProfileTypeInfoCell::VALUE_OFFSET}},
            {JSType::PROFILE_TYPE_INFO_CELL_N, {
                ProfileTypeInfoCell::VALUE_OFFSET,
                ProfileTypeInfoCell::MACHINE_CODE_OFFSET,
                ProfileTypeInfoCell::BASELINE_CODE_OFFSET,
                ProfileTypeInfoCell::HANDLE_OFFSET,
                ProfileTypeInfoCell::LAST_OFFSET - ProfileTypeInfoCell::VALUE_OFFSET}},
            {JSType::PROGRAM, {Program::MAIN_FUNCTION_OFFSET, Program::SIZE - Program::MAIN_FUNCTION_OFFSET}},
            {JSType::PROMISE_CAPABILITY, {PromiseCapability::PROMISE_OFFSET, PromiseCapability::RESOLVE_OFFSET,
                                          PromiseCapability::REJECT_OFFSET,
                                          PromiseCapability::SIZE - PromiseCapability::PROMISE_OFFSET}},
            {JSType::PROMISE_ITERATOR_RECORD, {PromiseIteratorRecord::ITERATOR_OFFSET,
                                               PromiseIteratorRecord::SIZE - PromiseIteratorRecord::ITERATOR_OFFSET}},
            {JSType::PROMISE_REACTIONS, {PromiseReaction::PROMISE_CAPABILITY_OFFSET,
                                         PromiseReaction::HANDLER_OFFSET,
                                         PromiseReaction::SIZE - PromiseReaction::PROMISE_CAPABILITY_OFFSET}},
            {JSType::PROMISE_RECORD, {PromiseRecord::VALUE_OFFSET,
                                      PromiseRecord::SIZE - PromiseRecord::VALUE_OFFSET}},
            {JSType::PROPERTY_BOX, {PropertyBox::VALUE_OFFSET,
                                    PropertyBox::SIZE - PropertyBox::VALUE_OFFSET}},
            {JSType::PROTOTYPE_HANDLER, {PrototypeHandler::HANDLER_INFO_OFFSET,
                                         PrototypeHandler::PROTO_CELL_OFFSET,
                                         PrototypeHandler::HOLDER_OFFSET,
                                         PrototypeHandler::ACCESSOR_JSFUNCTION_OFFSET,
                                         PrototypeHandler::SIZE - PrototypeHandler::HANDLER_INFO_OFFSET}},
            {JSType::PROTOTYPE_INFO, {ProtoChangeDetails::CHANGE_LISTENER_OFFSET,
                                      ProtoChangeDetails::SIZE - ProtoChangeDetails::CHANGE_LISTENER_OFFSET}},
            {JSType::PROTO_CHANGE_MARKER, {ProtoChangeMarker::SIZE - ProtoChangeMarker::BIT_FIELD_OFFSET}},
            {JSType::RB_TREENODE, {RBTreeNode::LEFT_OFFSET, RBTreeNode::RIGHT_OFFSET, RBTreeNode::ISRED_OFFSET,
                                   RBTreeNode::SIZE - RBTreeNode::LEFT_OFFSET}},
            {JSType::RESOLVEDBINDING_RECORD, {ResolvedBinding::MODULE_OFFSET, ResolvedBinding::BINDING_NAME_OFFSET,
                                              ResolvedBinding::SIZE - ResolvedBinding::MODULE_OFFSET}},
            {JSType::RESOLVEDINDEXBINDING_RECORD, {
                ResolvedIndexBinding::MODULE_OFFSET,
                ResolvedIndexBinding::INDEX_OFFSET,
                ResolvedIndexBinding::BIT_FIELD_OFFSET,
                ResolvedIndexBinding::SIZE - ResolvedIndexBinding::MODULE_OFFSET}},
            {JSType::RESOLVEDRECORDBINDING_RECORD, {
                ResolvedRecordBinding::MODULE_RECORD_OFFSET,
                ResolvedRecordBinding::BINDING_NAME_OFFSET,
                ResolvedRecordBinding::SIZE - ResolvedRecordBinding::MODULE_RECORD_OFFSET}},
            {JSType::RESOLVEDRECORDINDEXBINDING_RECORD, {
                ResolvedRecordIndexBinding::MODULE_RECORD_INDEX_OFFSET,
                ResolvedRecordIndexBinding::ABC_FILE_NAME,
                ResolvedRecordIndexBinding::SIZE - ResolvedRecordIndexBinding::MODULE_RECORD_INDEX_OFFSET}},
            {JSType::RESOLVING_FUNCTIONS_RECORD, {
                ResolvingFunctionsRecord::RESOLVE_FUNCTION_OFFSET,
                ResolvingFunctionsRecord::REJECT_FUNCTION_OFFSET,
                ResolvingFunctionsRecord::SIZE - ResolvingFunctionsRecord::RESOLVE_FUNCTION_OFFSET}},
            {JSType::SENDABLE_ENV, {TaggedArray::SIZE - TaggedArray::SIZE}},
            {JSType::SFUNCTION_ENV, {TaggedArray::SIZE - TaggedArray::SIZE}},
            {JSType::JS_XREF_OBJECT, {JSObject::SIZE - JSObject::SIZE}},
            {JSType::SLICED_STRING, {SlicedString::PARENT_OFFSET, SlicedString::SIZE - SlicedString::PARENT_OFFSET}},
            {JSType::SOURCE_TEXT_MODULE_RECORD, {
                SourceTextModule::SOURCE_TEXT_MODULE_OFFSET,
                SourceTextModule::NAMESPACE_OFFSET,
                SourceTextModule::MODULE_REQUESTS_OFFSET,
                SourceTextModule::REQUESTED_MODULES_OFFSET,
                SourceTextModule::IMPORT_ENTRIES_OFFSET,
                SourceTextModule::LOCAL_EXPORT_ENTTRIES_OFFSET,
                SourceTextModule::INDIRECT_EXPORT_ENTTRIES_OFFSET,
                SourceTextModule::START_EXPORT_ENTTRIES_OFFSET,
                SourceTextModule::NAME_DICTIONARY_OFFSET,
                SourceTextModule::CYCLE_ROOT_OFFSET,
                SourceTextModule::TOP_LEVEL_CAPABILITY_OFFSET,
                SourceTextModule::ASYNC_PARENT_MODULES_OFFSET,
                SourceTextModule::SENDABLE_ENV_OFFSET,
                SourceTextModule::EXCEPTION_OFFSET,
                SourceTextModule::SIZE - SourceTextModule::SOURCE_TEXT_MODULE_OFFSET}},
            {JSType::STAR_EXPORTENTRY_RECORD, {StarExportEntry::STAR_EXPORT_ENTRY_OFFSET,
                                               StarExportEntry::SIZE - StarExportEntry::STAR_EXPORT_ENTRY_OFFSET}},
            {JSType::STORE_TS_HANDLER, {StoreAOTHandler::HANDLER_INFO_OFFSET,
                                        StoreAOTHandler::PROTO_CELL_OFFSET,
                                        StoreAOTHandler::HOLDER_OFFSET,
                                        StoreAOTHandler::SIZE - StoreAOTHandler::HANDLER_INFO_OFFSET}},
            {JSType::SYMBOL, {JSSymbol::DESCRIPTION_OFFSET,
                              JSSymbol::SIZE - JSSymbol::DESCRIPTION_OFFSET}},
            {JSType::TAGGED_ARRAY, {TaggedArray::LENGTH_OFFSET, TaggedArray::DATA_OFFSET,
                                    TaggedArray::SIZE - TaggedArray::SIZE}},
            {JSType::TAGGED_DICTIONARY, {TaggedArray::LAST_OFFSET - TaggedArray::LENGTH_OFFSET}},
            {JSType::TEMPLATE_MAP, {TaggedArray::LAST_OFFSET - TaggedArray::LENGTH_OFFSET}},
            {JSType::TRACK_INFO, {TrackInfo::CACHED_HCLASS_OFFSET, TrackInfo::CACHED_FUNC_OFFSET,
                                  TrackInfo::LAST_OFFSET - TrackInfo::CACHED_HCLASS_OFFSET}},
            {JSType::TRANSITION_HANDLER, {TransitionHandler::HANDLER_INFO_OFFSET,
                                          TransitionHandler::TRANSITION_HCLASS_OFFSET,
                                          TransitionHandler::SIZE - TransitionHandler::HANDLER_INFO_OFFSET}},
            {JSType::TRANS_WITH_PROTO_HANDLER, {
                TransWithProtoHandler::HANDLER_INFO_OFFSET,
                TransWithProtoHandler::TRANSITION_HCLASS_OFFSET,
                TransWithProtoHandler::PROTO_CELL_OFFSET,
                TransWithProtoHandler::SIZE - TransWithProtoHandler::HANDLER_INFO_OFFSET}},
            {JSType::TREE_STRING, {TreeString::FIRST_OFFSET, TreeString::SECOND_OFFSET,
                                   TreeString::SIZE - TreeString::FIRST_OFFSET}},
            {JSType::VTABLE, {TaggedArray::LAST_OFFSET - TaggedArray::LENGTH_OFFSET}}
        };
        // { typeName: [all parents of this type]}
        parentsTable_ = {
            {JSType::HCLASS, {"TAGGED_OBJECT"}},
            {JSType::ACCESSOR_DATA, {"RECORD"}},
            {JSType::AOT_LITERAL_INFO, {"TAGGED_OBJECT"}},
            {JSType::ASYNC_GENERATOR_REQUEST, {"RECORD"}},
            {JSType::ASYNC_ITERATOR_RECORD, {"RECORD"}},
            {JSType::BIGINT, {"TAGGED_OBJECT"}},
            {JSType::BYTE_ARRAY, {"TAGGED_OBJECT"}},
            {JSType::CELL_RECORD, {"RECORD"}},
            {JSType::CLASS_INFO_EXTRACTOR, {"TAGGED_OBJECT"}},
            {JSType::CLASS_LITERAL, {"TAGGED_OBJECT"}},
            {JSType::COMPLETION_RECORD, {"RECORD"}},
            {JSType::CONSTANT_POOL, {"TAGGED_ARRAY"}},
            {JSType::COW_MUTANT_TAGGED_ARRAY, {"MUTANT_TAGGED_ARRAY"}},
            {JSType::COW_TAGGED_ARRAY, {"TAGGED_ARRAY"}},
            {JSType::EXTRA_PROFILE_TYPE_INFO, {"TAGGED_OBJECT"}},
            {JSType::FUNCTION_TEMPLATE, {"TAGGED_OBJECT"}},
            {JSType::GLOBAL_ENV, {"TAGGED_ARRAY"}},
            {JSType::IMPORTENTRY_RECORD, {"RECORD"}},
            {JSType::INDIRECT_EXPORTENTRY_RECORD, {"RECORD"}},
            {JSType::INTERNAL_ACCESSOR, {"RECORD"}},
            {JSType::JS_AGGREGATE_ERROR, {"ECMA_OBJECT"}},
            {JSType::JS_API_ARRAYLIST_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_API_ARRAY_LIST, {"JS_OBJECT"}},
            {JSType::JS_API_BITVECTOR, {"JS_OBJECT"}},
            {JSType::JS_API_BITVECTOR_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_API_DEQUE, {"JS_OBJECT"}},
            {JSType::JS_API_DEQUE_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_API_HASHMAP_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_API_HASHSET_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_API_HASH_MAP, {"JS_OBJECT"}},
            {JSType::JS_API_HASH_SET, {"JS_OBJECT"}},
            {JSType::JS_API_LIGHT_WEIGHT_MAP, {"JS_OBJECT"}},
            {JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_API_LIGHT_WEIGHT_SET, {"JS_OBJECT"}},
            {JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_API_LINKED_LIST, {"JS_OBJECT"}},
            {JSType::JS_API_LINKED_LIST_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_API_LIST, {"JS_OBJECT"}},
            {JSType::JS_API_LIST_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_API_PLAIN_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_API_FAST_BUFFER, {"JS_OBJECT"}},
            {JSType::JS_API_PLAIN_ARRAY_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_API_QUEUE, {"JS_OBJECT"}},
            {JSType::JS_API_QUEUE_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_API_STACK, {"JS_OBJECT"}},
            {JSType::JS_API_STACK_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_API_TREEMAP_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_API_TREESET_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_API_TREE_MAP, {"JS_OBJECT"}},
            {JSType::JS_API_TREE_SET, {"JS_OBJECT"}},
            {JSType::JS_API_VECTOR, {"JS_OBJECT"}},
            {JSType::JS_API_VECTOR_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_ARGUMENTS, {"JS_OBJECT"}},
            {JSType::JS_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_ARRAY_BUFFER, {"JS_OBJECT"}},
            {JSType::JS_ARRAY_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_ASYNCITERATOR, {"ECMA_OBJECT"}},
            {JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_ASYNC_FROM_SYNC_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_ASYNC_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_ASYNC_FUNC_OBJECT, {"JS_GENERATOR_OBJECT"}},
            {JSType::JS_ASYNC_GENERATOR_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_ASYNC_GENERATOR_OBJECT, {"JS_OBJECT"}},
            {JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN, {"JS_FUNCTION"}},
            {JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_BIGINT64_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_BIGUINT64_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_BOUND_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_CJS_EXPORTS, {"JS_OBJECT"}},
            {JSType::JS_CJS_MODULE, {"JS_OBJECT"}},
            {JSType::JS_CJS_REQUIRE, {"JS_OBJECT"}},
            {JSType::JS_COLLATOR, {"JS_OBJECT"}},
            {JSType::JS_DATA_VIEW, {"JS_OBJECT"}},
            {JSType::JS_DATE, {"JS_OBJECT"}},
            {JSType::JS_DATE_TIME_FORMAT, {"JS_OBJECT"}},
            {JSType::JS_DISPLAYNAMES, {"JS_OBJECT"}},
            {JSType::JS_ERROR, {"ECMA_OBJECT"}},
            {JSType::JS_EVAL_ERROR, {"ECMA_OBJECT"}},
            {JSType::JS_FINALIZATION_REGISTRY, {"JS_OBJECT"}},
            {JSType::JS_FLOAT32_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_FLOAT64_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_FORIN_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_FUNCTION, {"JS_FUNCTION_BASE"}},
            {JSType::JS_FUNCTION_BASE, {"JS_OBJECT"}},
            {JSType::JS_GENERATOR_CONTEXT, {"TAGGED_OBJECT"}},
            {JSType::JS_GENERATOR_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_GENERATOR_OBJECT, {"JS_OBJECT"}},
            {JSType::JS_GLOBAL_OBJECT, {"JS_OBJECT"}},
            {JSType::JS_INT16_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_INT32_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_INT8_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_INTL, {"JS_OBJECT"}},
            {JSType::JS_INTL_BOUND_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_ITERATOR, {"ECMA_OBJECT"}},
            {JSType::JS_LIST_FORMAT, {"JS_OBJECT"}},
            {JSType::JS_LOCALE, {"JS_OBJECT"}},
            {JSType::JS_MAP, {"JS_OBJECT"}},
            {JSType::JS_MAP_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_MODULE_NAMESPACE, {"JS_OBJECT"}},
            {JSType::JS_NATIVE_POINTER, {"TAGGED_OBJECT"}},
            {JSType::JS_NUMBER_FORMAT, {"JS_OBJECT"}},
            {JSType::JS_OBJECT, {"ECMA_OBJECT"}},
            {JSType::JS_OOM_ERROR, {"ECMA_OBJECT"}},
            {JSType::JS_PLURAL_RULES, {"JS_OBJECT"}},
            {JSType::JS_PRIMITIVE_REF, {"JS_OBJECT"}},
            {JSType::JS_PROMISE, {"JS_OBJECT"}},
            {JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_PROMISE_EXECUTOR_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_PROMISE_FINALLY_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_PROMISE_REACTIONS_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_PROXY, {"ECMA_OBJECT"}},
            {JSType::JS_PROXY_REVOC_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_RANGE_ERROR, {"ECMA_OBJECT"}},
            {JSType::JS_REALM, {"JS_OBJECT"}},
            {JSType::JS_REFERENCE_ERROR, {"ECMA_OBJECT"}},
            {JSType::JS_REG_EXP, {"JS_OBJECT"}},
            {JSType::JS_REG_EXP_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_RELATIVE_TIME_FORMAT, {"JS_OBJECT"}},
            {JSType::JS_SEGMENTER, {"JS_OBJECT"}},
            {JSType::JS_SEGMENTS, {"JS_OBJECT"}},
            {JSType::JS_SEGMENT_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_SENDABLE_ARRAY_BUFFER, {"JS_OBJECT"}},
            {JSType::JS_SET, {"JS_OBJECT"}},
            {JSType::JS_SET_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_SHARED_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_SHARED_ARRAY_BUFFER, {"JS_OBJECT"}},
            {JSType::JS_SHARED_ARRAY_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_SHARED_ASYNC_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_SHARED_BIGINT64_ARRAY, {"JS_TYPED_ARRAY"}},
            {JSType::JS_SHARED_BIGUINT64_ARRAY, {"JS_TYPED_ARRAY"}},
            {JSType::JS_SHARED_FLOAT32_ARRAY, {"JS_TYPED_ARRAY"}},
            {JSType::JS_SHARED_FLOAT64_ARRAY, {"JS_TYPED_ARRAY"}},
            {JSType::JS_SHARED_FUNCTION, {"JS_FUNCTION"}},
            {JSType::JS_SHARED_INT16_ARRAY, {"JS_TYPED_ARRAY"}},
            {JSType::JS_SHARED_INT32_ARRAY, {"JS_TYPED_ARRAY"}},
            {JSType::JS_SHARED_INT8_ARRAY, {"JS_TYPED_ARRAY"}},
            {JSType::JS_SHARED_MAP, {"JS_OBJECT"}},
            {JSType::JS_SHARED_MAP_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_SHARED_OBJECT, {"JS_OBJECT"}},
            {JSType::JS_SHARED_SET, {"JS_OBJECT"}},
            {JSType::JS_SHARED_SET_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_SHARED_TYPED_ARRAY, {"JS_TYPED_ARRAY"}},
            {JSType::JS_SHARED_UINT16_ARRAY, {"JS_TYPED_ARRAY"}},
            {JSType::JS_SHARED_UINT32_ARRAY, {"JS_TYPED_ARRAY"}},
            {JSType::JS_SHARED_UINT8_ARRAY, {"JS_TYPED_ARRAY"}},
            {JSType::JS_SHARED_UINT8_CLAMPED_ARRAY, {"JS_TYPED_ARRAY"}},
            {JSType::JS_STRING_ITERATOR, {"JS_OBJECT"}},
            {JSType::JS_SYNTAX_ERROR, {"ECMA_OBJECT"}},
            {JSType::JS_TERMINATION_ERROR, {"ECMA_OBJECT"}},
            {JSType::JS_TYPED_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_TYPE_ERROR, {"ECMA_OBJECT"}},
            {JSType::JS_UINT16_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_UINT32_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_UINT8_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_UINT8_CLAMPED_ARRAY, {"JS_OBJECT"}},
            {JSType::JS_URI_ERROR, {"ECMA_OBJECT"}},
            {JSType::JS_WEAK_MAP, {"JS_OBJECT"}},
            {JSType::JS_WEAK_REF, {"JS_OBJECT"}},
            {JSType::JS_WEAK_SET, {"JS_OBJECT"}},
            {JSType::LEXICAL_ENV, {"TAGGED_ARRAY"}},
            {JSType::LINE_STRING, {"ECMA_STRING"}},
            {JSType::LINKED_NODE, {"TAGGED_NODE"}},
            {JSType::LOCAL_EXPORTENTRY_RECORD, {"RECORD"}},
            {JSType::MACHINE_CODE_OBJECT, {"TAGGED_OBJECT"}},
            {JSType::MARKER_CELL, {"TAGGED_OBJECT"}},
            {JSType::METHOD, {"TAGGED_OBJECT"}},
            {JSType::MICRO_JOB_QUEUE, {"RECORD"}},
            {JSType::MODULE_RECORD, {"RECORD"}},
            {JSType::MUTANT_TAGGED_ARRAY, {"TAGGED_ARRAY"}},
            {JSType::NATIVE_MODULE_FAILURE_INFO, {"JS_OBJECT"}},
            {JSType::PENDING_JOB, {"RECORD"}},
            {JSType::PROFILE_TYPE_INFO, {"TAGGED_ARRAY"}},
            {JSType::PROFILE_TYPE_INFO_CELL_0, {"TAGGED_OBJECT"}},
            {JSType::PROFILE_TYPE_INFO_CELL_1, {"TAGGED_OBJECT"}},
            {JSType::PROFILE_TYPE_INFO_CELL_N, {"TAGGED_OBJECT"}},
            {JSType::PROGRAM, {"ECMA_OBJECT"}},
            {JSType::PROMISE_CAPABILITY, {"RECORD"}},
            {JSType::PROMISE_ITERATOR_RECORD, {"RECORD"}},
            {JSType::PROMISE_REACTIONS, {"RECORD"}},
            {JSType::PROMISE_RECORD, {"RECORD"}},
            {JSType::PROPERTY_BOX, {"TAGGED_OBJECT"}},
            {JSType::PROTOTYPE_HANDLER, {"TAGGED_OBJECT"}},
            {JSType::PROTOTYPE_INFO, {"TAGGED_OBJECT"}},
            {JSType::PROTO_CHANGE_MARKER, {"TAGGED_OBJECT"}},
            {JSType::RB_TREENODE, {"TAGGED_NODE"}},
            {JSType::RESOLVEDBINDING_RECORD, {"RECORD"}},
            {JSType::RESOLVEDINDEXBINDING_RECORD, {"RECORD"}},
            {JSType::RESOLVEDRECORDBINDING_RECORD, {"RECORD"}},
            {JSType::RESOLVEDRECORDINDEXBINDING_RECORD, {"RECORD"}},
            {JSType::RESOLVING_FUNCTIONS_RECORD, {"RECORD"}},
            {JSType::SENDABLE_ENV, {"TAGGED_ARRAY"}},
            {JSType::SFUNCTION_ENV, {"TAGGED_ARRAY"}},
            {JSType::JS_XREF_OBJECT, {"JS_OBJECT"}},
            {JSType::SLICED_STRING, {"ECMA_STRING"}},
            {JSType::SOURCE_TEXT_MODULE_RECORD, {"MODULE_RECORD"}},
            {JSType::STAR_EXPORTENTRY_RECORD, {"RECORD"}},
            {JSType::STORE_TS_HANDLER, {"TAGGED_OBJECT"}},
            {JSType::SYMBOL, {"TAGGED_OBJECT"}},
            {JSType::TAGGED_ARRAY, {"TAGGED_OBJECT"}},
            {JSType::TAGGED_DICTIONARY, {"TAGGED_ARRAY"}},
            {JSType::TEMPLATE_MAP, {"TAGGED_OBJECT"}},
            {JSType::TRACK_INFO, {"TAGGED_OBJECT"}},
            {JSType::TRANSITION_HANDLER, {"TAGGED_OBJECT"}},
            {JSType::TRANS_WITH_PROTO_HANDLER, {"TAGGED_OBJECT"}},
            {JSType::TREE_STRING, {"ECMA_STRING"}},
            {JSType::VTABLE, {"TAGGED_OBJECT"}}
        };
        // { typeName: [size of all fields' in the same order as declared in .h files]}
        fieldSizeTable_ = {
            {JSType::HCLASS, {JSHClass::BIT_FIELD1_OFFSET - JSHClass::BIT_FIELD_OFFSET,
                              JSHClass::PROTOTYPE_OFFSET - JSHClass::BIT_FIELD1_OFFSET,
                              JSHClass::LAYOUT_OFFSET - JSHClass::PROTOTYPE_OFFSET,
                              JSHClass::TRANSTIONS_OFFSET - JSHClass::LAYOUT_OFFSET,
                              JSHClass::PARENT_OFFSET - JSHClass::TRANSTIONS_OFFSET,
                              JSHClass::PROTO_CHANGE_MARKER_OFFSET - JSHClass::PARENT_OFFSET,
                              JSHClass::PROTO_CHANGE_DETAILS_OFFSET - JSHClass::PROTO_CHANGE_MARKER_OFFSET,
                              JSHClass::ENUM_CACHE_OFFSET - JSHClass::PROTO_CHANGE_DETAILS_OFFSET,
                              JSHClass::DEPENDENT_INFOS_OFFSET - JSHClass::ENUM_CACHE_OFFSET,
                              JSHClass::PROFILE_TYPE_OFFSET - JSHClass::DEPENDENT_INFOS_OFFSET,
                              JSHClass::LAST_OFFSET - JSHClass::PROFILE_TYPE_OFFSET}},
            {JSType::ACCESSOR_DATA, {AccessorData::SETTER_OFFSET - AccessorData::GETTER_OFFSET,
                                     AccessorData::SIZE - AccessorData::SETTER_OFFSET}},
            {JSType::AOT_LITERAL_INFO, {}},
            {JSType::ASYNC_GENERATOR_REQUEST, {
                AsyncGeneratorRequest::CAPABILITY_OFFSET - AsyncGeneratorRequest::COMPLETION_OFFSET,
                AsyncGeneratorRequest::SIZE - AsyncGeneratorRequest::CAPABILITY_OFFSET}},
            {JSType::ASYNC_ITERATOR_RECORD, {
                AsyncIteratorRecord::NEXTMETHOD_OFFSET - AsyncIteratorRecord::ITERATOR_OFFSET,
                AsyncIteratorRecord::BIT_FIELD_OFFSET - AsyncIteratorRecord::NEXTMETHOD_OFFSET}},
            {JSType::BIGINT, {}},
            {JSType::BYTE_ARRAY, {}},
            {JSType::CELL_RECORD, {CellRecord::HELD_VALUE_OFFSET - CellRecord::WEAKREF_TARGET_OFFSET,
                                   CellRecord::SIZE - CellRecord::HELD_VALUE_OFFSET}},
            {JSType::CLASS_INFO_EXTRACTOR, {
                ClassInfoExtractor::NON_STATIC_PROPERTIES_OFFSET - ClassInfoExtractor::PROTOTYPE_HCLASS_OFFSET,
                ClassInfoExtractor::NON_STATIC_ELEMENTS_OFFSET - ClassInfoExtractor::NON_STATIC_PROPERTIES_OFFSET,
                ClassInfoExtractor::CONSTRUCTOR_HCLASS_OFFSET - ClassInfoExtractor::NON_STATIC_ELEMENTS_OFFSET,
                ClassInfoExtractor::STATIC_PROPERTIES_OFFSET - ClassInfoExtractor::CONSTRUCTOR_HCLASS_OFFSET,
                ClassInfoExtractor::STATIC_ELEMENTS_OFFSET - ClassInfoExtractor::STATIC_PROPERTIES_OFFSET,
                ClassInfoExtractor::CONSTRUCTOR_METHOD_OFFSET - ClassInfoExtractor::STATIC_ELEMENTS_OFFSET,
                ClassInfoExtractor::BIT_FIELD_OFFSET - ClassInfoExtractor::CONSTRUCTOR_METHOD_OFFSET}},
            {JSType::CLASS_LITERAL, {ClassLiteral::IS_AOT_USED_OFFSET - ClassLiteral::ARRAY_OFFSET}},
            {JSType::COMPLETION_RECORD, {CompletionRecord::BIT_FIELD_OFFSET - CompletionRecord::VALUE_OFFSET}},
            {JSType::CONSTANT_POOL, {}},
            {JSType::COW_MUTANT_TAGGED_ARRAY, {}},
            {JSType::COW_TAGGED_ARRAY, {}},
            {JSType::EXTRA_PROFILE_TYPE_INFO, {
                ExtraProfileTypeInfo::HOLDER_OBJECT_OFFSET - ExtraProfileTypeInfo::RECEIVER_OBJECT_OFFSET,
                ExtraProfileTypeInfo::LAST_OFFSET - ExtraProfileTypeInfo::HOLDER_OBJECT_OFFSET}},
            {JSType::FUNCTION_TEMPLATE, {
                FunctionTemplate::MODULE_OFFSET - FunctionTemplate::METHOD_OFFSET,
                FunctionTemplate::RAW_PROFILE_TYPE_INFO_OFFSET - FunctionTemplate::MODULE_OFFSET,
                FunctionTemplate::LENGTH_OFFSET - FunctionTemplate::RAW_PROFILE_TYPE_INFO_OFFSET}},
            {JSType::GLOBAL_ENV, {}},
            {JSType::IMPORTENTRY_RECORD, {
                ImportEntry::LOCAL_NAME_OFFSET - ImportEntry::IMPORT_ENTRY_OFFSET,
                ImportEntry::MODULE_REQUEST_INDEX_OFFSET - ImportEntry::LOCAL_NAME_OFFSET,
                ImportEntry::SIZE - ImportEntry::IMPORT_ENTRY_OFFSET}},
            {JSType::INDIRECT_EXPORTENTRY_RECORD, {
                IndirectExportEntry::IMPORT_NAME_OFFSET - IndirectExportEntry::INDIRECT_EXPORT_ENTRY_OFFSET,
                IndirectExportEntry::MODULE_REQUEST_INDEX_OFFSET - IndirectExportEntry::IMPORT_NAME_OFFSET,
                IndirectExportEntry::SIZE - IndirectExportEntry::INDIRECT_EXPORT_ENTRY_OFFSET}},
            {JSType::INTERNAL_ACCESSOR, {
                InternalAccessor::SETTER_OFFSET - InternalAccessor::GETTER_OFFSET,
                InternalAccessor::SIZE - InternalAccessor::SETTER_OFFSET}},
            {JSType::JS_AGGREGATE_ERROR, {
                JSObject::ELEMENTS_OFFSET - JSObject::PROPERTIES_OFFSET,
                JSObject::SIZE - JSObject::ELEMENTS_OFFSET}},
            {JSType::JS_API_ARRAYLIST_ITERATOR, {
                JSAPIArrayListIterator::NEXT_INDEX_OFFSET - JSAPIArrayListIterator::ITERATED_ARRAYLIST_OFFSET}},
            {JSType::JS_API_ARRAY_LIST, {JSAPIArrayList::SIZE - JSAPIArrayList::LENGTH_OFFSET}},
            {JSType::JS_API_BITVECTOR, {JSAPIBitVector::LENGTH_OFFSET - JSAPIBitVector::NATIVE_POINTER_OFFSET}},
            {JSType::JS_API_BITVECTOR_ITERATOR, {
                JSAPIBitVectorIterator::NEXT_INDEX_OFFSET - JSAPIBitVectorIterator::ITERATED_BITVECTOR_OFFSET}},
            {JSType::JS_API_DEQUE, {}},
            {JSType::JS_API_DEQUE_ITERATOR, {
                JSAPIDequeIterator::NEXT_INDEX_OFFSET - JSAPIDequeIterator::ITERATED_DEQUE_OFFSET}},
            {JSType::JS_API_HASHMAP_ITERATOR, {
                JSAPIHashMapIterator::TAGGED_QUEUE_OFFSET - JSAPIHashMapIterator::ITERATED_HASHMAP_OFFSET,
                JSAPIHashMapIterator::CURRENT_NODE_RESULT - JSAPIHashMapIterator::TAGGED_QUEUE_OFFSET,
                JSAPIHashMapIterator::NEXT_INDEX_OFFSET - JSAPIHashMapIterator::CURRENT_NODE_RESULT
            }},
            {JSType::JS_API_HASHSET_ITERATOR, {
                JSAPIHashSetIterator::TAGGED_QUEUE_OFFSET - JSAPIHashSetIterator::ITERATED_HASHSET_OFFSET,
                JSAPIHashSetIterator::CURRENT_NODE_RESULT - JSAPIHashSetIterator::TAGGED_QUEUE_OFFSET,
                JSAPIHashSetIterator::NEXT_INDEX_OFFSET - JSAPIHashSetIterator::CURRENT_NODE_RESULT}},
            {JSType::JS_API_HASH_MAP, {JSAPIHashMap::HASHMAP_SIZE_OFFSET- JSAPIHashMap::HASHMAP_TABLE_INDEX}},
            {JSType::JS_API_HASH_SET, {JSAPIHashSet::HASHSET_SIZE_OFFSET- JSAPIHashSet::HASHSET_TABLE_INDEX}},
            {JSType::JS_API_LIGHT_WEIGHT_MAP, {
                JSAPILightWeightMap::LWP_KEYS_OFFSET - JSAPILightWeightMap::LWP_HASHES_OFFSET,
                JSAPILightWeightMap::LWP_VALUES_OFFSET - JSAPILightWeightMap::LWP_KEYS_OFFSET,
                JSAPILightWeightMap::LWP_LENGTH_OFFSET - JSAPILightWeightMap::LWP_VALUES_OFFSET}},
            {JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR, {
                JSAPILightWeightMapIterator::NEXT_INDEX_OFFSET - JSAPILightWeightMapIterator::ITERATED_LIGHT_WEIGHT_MAP_OFFSET}},
            {JSType::JS_API_LIGHT_WEIGHT_SET, {
                JSAPILightWeightSet::VALUES_OFFSET - JSAPILightWeightSet::HASHES_OFFSET,
                JSAPILightWeightSet::LENGTH_OFFSET - JSAPILightWeightSet::VALUES_OFFSET}},
            {JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR, {
                JSAPILightWeightSetIterator::NEXT_INDEX_OFFSET - JSAPILightWeightSetIterator::ITERATED_LIGHT_WEIGHT_SET_OFFSET}},
            {JSType::JS_API_LINKED_LIST, {JSAPILinkedList::SIZE - JSAPILinkedList::DOUBLE_LIST_OFFSET}},
            {JSType::JS_API_LINKED_LIST_ITERATOR, {
                JSAPILinkedListIterator::DATA_INDEX_OFFSET - JSAPILinkedListIterator::ITERATED_LINKED_LIST_OFFSET}},
            {JSType::JS_API_LIST, {JSAPIList::BIT_FIELD_OFFSET - JSAPIList::SINGLY_LIST_OFFSET}},
            {JSType::JS_API_LIST_ITERATOR, {
                JSAPIListIterator::DATA_INDEX_OFFSET - JSAPIListIterator::ITERATED_LIST_OFFSET}},
            {JSType::JS_API_PLAIN_ARRAY, {JSAPIPlainArray::VALUES_OFFSET - JSAPIPlainArray::KEYS_OFFSET,
                                          JSAPIPlainArray::LENGTH_OFFSET - JSAPIPlainArray::VALUES_OFFSET}},
            {JSType::JS_API_FAST_BUFFER, {JSAPIFastBuffer::BUFFER_LENGTH_OFFSET - JSAPIFastBuffer::TYPEDARRAY_OFFSET}},
            {JSType::JS_API_PLAIN_ARRAY_ITERATOR, {
                JSAPIPlainArrayIterator::NEXT_INDEX_OFFSET - JSAPIPlainArrayIterator::ITERATED_PLAIN_ARRAY_OFFSET}},
            {JSType::JS_API_QUEUE, {JSAPIQueue::FRONT_OFFSET - JSAPIQueue::LENGTH_OFFSET}},
            {JSType::JS_API_QUEUE_ITERATOR, {
                JSAPIQueueIterator::NEXT_INDEX_OFFSET - JSAPIQueueIterator::ITERATED_QUEUE_OFFSET}},
            {JSType::JS_API_STACK, {}},
            {JSType::JS_API_STACK_ITERATOR, {
                JSAPIStackIterator::NEXT_INDEX_OFFSET - JSAPIStackIterator::ITERATED_STACK_OFFSET}},
            {JSType::JS_API_TREEMAP_ITERATOR, {
                JSAPITreeMapIterator::ITERATED_ENTRIES_OFFSET - JSAPITreeMapIterator::ITERATED_MAP_OFFSET,
                JSAPITreeMapIterator::NEXT_INDEX_OFFSET - JSAPITreeMapIterator::ITERATED_ENTRIES_OFFSET}},
            {JSType::JS_API_TREESET_ITERATOR, {
                JSAPITreeSetIterator::ITERATED_ENTRIES_OFFSET - JSAPITreeSetIterator::ITERATED_SET_OFFSET,
                JSAPITreeSetIterator::NEXT_INDEX_OFFSET - JSAPITreeSetIterator::ITERATED_ENTRIES_OFFSET}},
            {JSType::JS_API_TREE_MAP, {JSAPITreeMap::SIZE - JSAPITreeMap::TREE_MAP_OFFSET}},
            {JSType::JS_API_TREE_SET, {JSAPITreeSet::SIZE - JSAPITreeSet::TREE_SET_OFFSET}},
            {JSType::JS_API_VECTOR, {}},
            {JSType::JS_API_VECTOR_ITERATOR, {
                JSAPIVectorIterator::NEXT_INDEX_OFFSET - JSAPIVectorIterator::ITERATED_VECTOR_OFFSET}},
            {JSType::JS_ARGUMENTS, {}},
            {JSType::JS_ARRAY, {JSArray::TRACE_INDEX_OFFSET - JSArray::LENGTH_OFFSET,
                                JSArray::SIZE - JSArray::TRACK_INFO_OFFSET}},
            {JSType::JS_ARRAY_BUFFER, {JSArrayBuffer::BYTE_LENGTH_OFFSET - JSArrayBuffer::DATA_OFFSET}},
            {JSType::JS_ARRAY_ITERATOR, {
                JSArrayIterator::NEXT_INDEX_OFFSET - JSArrayIterator::ITERATED_ARRAY_OFFSET}},
            {JSType::JS_ASYNCITERATOR, {JSObject::ELEMENTS_OFFSET - JSObject::PROPERTIES_OFFSET,
                                        JSObject::SIZE - JSObject::ELEMENTS_OFFSET}},
            {JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION, {
                JSAsyncAwaitStatusFunction::SIZE - JSAsyncAwaitStatusFunction::ASYNC_CONTEXT_OFFSET}},
            {JSType::JS_ASYNC_FROM_SYNC_ITERATOR, {
                JSAsyncFromSyncIterator::SIZE - JSAsyncFromSyncIterator::SYNC_ITERATOR_RECORD}},
            {JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION, {
                JSAsyncFromSyncIterUnwarpFunction::SIZE - JSAsyncFromSyncIterUnwarpFunction::DONE_OFFSET}},
            {JSType::JS_ASYNC_FUNCTION, {}},
            {JSType::JS_ASYNC_FUNC_OBJECT, {
                JSAsyncFuncObject::SIZE - JSAsyncFuncObject::GENERATOR_PROMISE_OFFSET}},
            {JSType::JS_ASYNC_GENERATOR_FUNCTION, {}},
            {JSType::JS_ASYNC_GENERATOR_OBJECT, {
                JSAsyncGeneratorObject::ASYNC_GENERATOR_QUEUE_OFFSET - JSAsyncGeneratorObject::GENERATOR_CONTEXT_OFFSET,
                JSAsyncGeneratorObject::GENERATOR_OFFSET - JSAsyncGeneratorObject::ASYNC_GENERATOR_QUEUE_OFFSET,
                JSAsyncGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET - JSAsyncGeneratorObject::GENERATOR_OFFSET,
                JSAsyncGeneratorObject::BIT_FIELD_OFFSET - JSAsyncGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET}},
            {JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN, {
                JSAsyncGeneratorResNextRetProRstFtn::SIZE - JSAsyncGeneratorResNextRetProRstFtn::ASYNC_GENERATOR_OBJECT_OFFSET}},
            {JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION, {}},
            {JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION, {}},
            {JSType::JS_BIGINT64_ARRAY, {
                JSTypedArray::TYPED_ARRAY_NAME_OFFSET - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                JSTypedArray::BYTE_LENGTH_OFFSET - JSTypedArray::TYPED_ARRAY_NAME_OFFSET}},
            {JSType::JS_BIGUINT64_ARRAY, {
                JSTypedArray::TYPED_ARRAY_NAME_OFFSET - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                JSTypedArray::BYTE_LENGTH_OFFSET - JSTypedArray::TYPED_ARRAY_NAME_OFFSET}},
            {JSType::JS_BOUND_FUNCTION, {
                JSBoundFunction::BOUND_THIS_OFFSET - JSBoundFunction::BOUND_TARGET_OFFSET,
                JSBoundFunction::BOUND_ARGUMENTS_OFFSET - JSBoundFunction::BOUND_THIS_OFFSET,
                JSBoundFunction::SIZE - JSBoundFunction::BOUND_ARGUMENTS_OFFSET}},
            {JSType::JS_CJS_EXPORTS, {CjsExports::SIZE - CjsExports::JS_CJS_EXPORTS_OFFSET}},
            {JSType::JS_CJS_MODULE, {CjsModule::ID_OFFSET - CjsModule::JS_CJS_MODULE_OFFSET,
                                     CjsModule::PATH_OFFSET - CjsModule::ID_OFFSET,
                                     CjsModule::EXPORTS_OFFSET - CjsModule::PATH_OFFSET,
                                     CjsModule::BIT_FIELD_OFFSET - CjsModule::EXPORTS_OFFSET}},
            {JSType::JS_CJS_REQUIRE, {CjsRequire::CACHE_OFFSET - CjsRequire::JS_CJS_REQUIRE_OFFSET,
                                      CjsRequire::SIZE - CjsRequire::CACHE_OFFSET}},
            {JSType::JS_COLLATOR, {JSCollator::LOCALE_OFFSET - JSCollator::ICU_FIELD_OFFSET,
                                   JSCollator::COLLATION_OFFSET - JSCollator::LOCALE_OFFSET,
                                   JSCollator::BOUND_COMPARE_OFFSET - JSCollator::COLLATION_OFFSET,
                                   JSCollator::BIT_FIELD_OFFSET - JSCollator::BOUND_COMPARE_OFFSET}},
            {JSType::JS_DATA_VIEW, {JSDataView::VIEW_ARRAY_BUFFER_OFFSET - JSDataView::DATA_VIEW_OFFSET,
                                    JSDataView::BYTE_LENGTH_OFFSET - JSDataView::VIEW_ARRAY_BUFFER_OFFSET}},
            {JSType::JS_DATE, {JSDate::LOCAL_TIME_OFFSET - JSDate::TIME_VALUE_OFFSET,
                               JSDate::SIZE - JSDate::LOCAL_TIME_OFFSET}},
            {JSType::JS_DATE_TIME_FORMAT, {
                JSDateTimeFormat::CALENDAR_OFFSET - JSDateTimeFormat::LOCALE_OFFSET,
                JSDateTimeFormat::NUMBER_STRING_SYSTEM_OFFSET - JSDateTimeFormat::CALENDAR_OFFSET,
                JSDateTimeFormat::TIME_ZONE_OFFSET - JSDateTimeFormat::NUMBER_STRING_SYSTEM_OFFSET,
                JSDateTimeFormat::LOCALE_ICU_OFFSET - JSDateTimeFormat::TIME_ZONE_OFFSET,
                JSDateTimeFormat::SIMPLE_DATE_TIME_FORMAT_ICU_OFFSET - JSDateTimeFormat::LOCALE_ICU_OFFSET,
                JSDateTimeFormat::ISO8601_OFFSET - JSDateTimeFormat::SIMPLE_DATE_TIME_FORMAT_ICU_OFFSET,
                JSDateTimeFormat::BOUND_FORMAT_OFFSET - JSDateTimeFormat::ISO8601_OFFSET,
                JSDateTimeFormat::BIT_FIELD_OFFSET - JSDateTimeFormat::BOUND_FORMAT_OFFSET}},
            {JSType::JS_DISPLAYNAMES, {JSDisplayNames::ICU_LOCALE_DISPPLAY_NAMES - JSDisplayNames::LOCALE_OFFSET,
                                       JSDisplayNames::BIT_FIELD_OFFSET - JSDisplayNames::ICU_LOCALE_DISPPLAY_NAMES}},
            {JSType::JS_ERROR, {JSObject::ELEMENTS_OFFSET - JSObject::PROPERTIES_OFFSET,
                                JSObject::SIZE - JSObject::ELEMENTS_OFFSET}},
            {JSType::JS_EVAL_ERROR, {JSObject::ELEMENTS_OFFSET - JSObject::PROPERTIES_OFFSET,
                                     JSObject::SIZE - JSObject::ELEMENTS_OFFSET}},
            {JSType::JS_FINALIZATION_REGISTRY, {
                JSFinalizationRegistry::NO_UNREGISTER_OFFSET - JSFinalizationRegistry::CLEANUP_CALLBACK_OFFSET,
                JSFinalizationRegistry::MAYBE_UNREGISTER_OFFSET - JSFinalizationRegistry::NO_UNREGISTER_OFFSET,
                JSFinalizationRegistry::NEXT_OFFSET - JSFinalizationRegistry::MAYBE_UNREGISTER_OFFSET,
                JSFinalizationRegistry::PREV_OFFSET - JSFinalizationRegistry::NEXT_OFFSET,
                JSFinalizationRegistry::SIZE - JSFinalizationRegistry::PREV_OFFSET}},
            {JSType::JS_FLOAT32_ARRAY, {
                JSTypedArray::TYPED_ARRAY_NAME_OFFSET - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                JSTypedArray::BYTE_LENGTH_OFFSET - JSTypedArray::TYPED_ARRAY_NAME_OFFSET}},
            {JSType::JS_FLOAT64_ARRAY, {
                JSTypedArray::TYPED_ARRAY_NAME_OFFSET - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                JSTypedArray::BYTE_LENGTH_OFFSET - JSTypedArray::TYPED_ARRAY_NAME_OFFSET}},
            {JSType::JS_FORIN_ITERATOR, {
                JSForInIterator::CACHED_HCLASS_OFFSET - JSForInIterator::OBJECT_OFFSET,
                JSForInIterator::KEYS_OFFSET - JSForInIterator::CACHED_HCLASS_OFFSET,
                JSForInIterator::CACHE_KIND_OFFSET - JSForInIterator::KEYS_OFFSET}},
            {JSType::JS_FUNCTION, {
                JSFunction::LEXICAL_ENV_OFFSET - JSFunction::PROTO_OR_DYNCLASS_OFFSET,
                JSFunction::MACHINECODE_OFFSET - JSFunction::LEXICAL_ENV_OFFSET,
                JSFunction::BASELINECODE_OFFSET - JSFunction::MACHINECODE_OFFSET,
                JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET - JSFunction::BASELINECODE_OFFSET,
                JSFunction::HOME_OBJECT_OFFSET - JSFunction::RAW_PROFILE_TYPE_INFO_OFFSET,
                JSFunction::ECMA_MODULE_OFFSET - JSFunction::HOME_OBJECT_OFFSET,
                JSFunction::WORK_NODE_POINTER_OFFSET - JSFunction::ECMA_MODULE_OFFSET}},
            {JSType::JS_FUNCTION_BASE, {JSFunctionBase::CODE_ENTRY_OFFSET - JSFunctionBase::METHOD_OFFSET}},
            {JSType::JS_GENERATOR_CONTEXT, {
                GeneratorContext::GENERATOR_METHOD_OFFSET - GeneratorContext::GENERATOR_REGS_ARRAY_OFFSET,
                GeneratorContext::GENERATOR_THIS_OFFSET - GeneratorContext::GENERATOR_METHOD_OFFSET,
                GeneratorContext::GENERATOR_ACC_OFFSET - GeneratorContext::GENERATOR_THIS_OFFSET,
                GeneratorContext::GENERATOR_GENERATOR_OBJECT_OFFSET - GeneratorContext::GENERATOR_ACC_OFFSET,
                GeneratorContext::GENERATOR_LEXICALENV_OFFSET - GeneratorContext::GENERATOR_GENERATOR_OBJECT_OFFSET,
                GeneratorContext::GENERATOR_NREGS_OFFSET - GeneratorContext::GENERATOR_LEXICALENV_OFFSET}},
            {JSType::JS_GENERATOR_FUNCTION, {}},
            {JSType::JS_GENERATOR_OBJECT, {
                JSGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET - JSGeneratorObject::GENERATOR_CONTEXT_OFFSET,
                JSGeneratorObject::TASK_INFO_OFFSET - JSGeneratorObject::GENERATOR_RESUME_RESULT_OFFSET}},
            {JSType::JS_GLOBAL_OBJECT, {}},
            {JSType::JS_INT16_ARRAY, {
                JSTypedArray::TYPED_ARRAY_NAME_OFFSET - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                JSTypedArray::BYTE_LENGTH_OFFSET - JSTypedArray::TYPED_ARRAY_NAME_OFFSET}},
            {JSType::JS_INT32_ARRAY, {
                JSTypedArray::TYPED_ARRAY_NAME_OFFSET - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                JSTypedArray::BYTE_LENGTH_OFFSET - JSTypedArray::TYPED_ARRAY_NAME_OFFSET}},
            {JSType::JS_INT8_ARRAY, {
                JSTypedArray::TYPED_ARRAY_NAME_OFFSET - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                JSTypedArray::BYTE_LENGTH_OFFSET - JSTypedArray::TYPED_ARRAY_NAME_OFFSET}},
            {JSType::JS_INTL, {JSIntl::SIZE - JSIntl::FALLBACK_SYMBOL}},
            {JSType::JS_INTL_BOUND_FUNCTION, {
                JSIntlBoundFunction::DATETIME_FORMAT_OFFSET - JSIntlBoundFunction::NUMBER_FORMAT_OFFSET,
                JSIntlBoundFunction::COLLATOR_OFFSET - JSIntlBoundFunction::DATETIME_FORMAT_OFFSET,
                JSIntlBoundFunction::SIZE - JSIntlBoundFunction::COLLATOR_OFFSET}},
            {JSType::JS_ITERATOR, {JSObject::ELEMENTS_OFFSET - JSObject::PROPERTIES_OFFSET,
                                   JSObject::SIZE - JSObject::ELEMENTS_OFFSET}},
            {JSType::JS_LIST_FORMAT, {JSListFormat::ICU_LIST_FORMAT - JSListFormat::LOCALE_OFFSET,
                                      JSListFormat::BIT_FIELD_OFFSET - JSListFormat::ICU_LIST_FORMAT}},
            {JSType::JS_LOCALE, {JSLocale::SIZE - JSLocale::ICU_FIELD_OFFSET}},
            {JSType::JS_MAP, {JSMap::SIZE - JSMap::LINKED_MAP_OFFSET}},
            {JSType::JS_MAP_ITERATOR, {JSMapIterator::NEXT_INDEX_OFFSET - JSMapIterator::ITERATED_MAP_OFFSET}},
            {JSType::JS_MODULE_NAMESPACE, {
                ModuleNamespace::EXPORTS_OFFSET - ModuleNamespace::MODULE_OFFSET,
                ModuleNamespace::DEREGISTER_PROCESSION_OFFSET - ModuleNamespace::EXPORTS_OFFSET,
                ModuleNamespace::SIZE - ModuleNamespace::DEREGISTER_PROCESSION_OFFSET}},
            {JSType::JS_NATIVE_POINTER, {
                JSNativePointer::FLAG_OFFSET- JSNativePointer::DATA_SIZE_OFFSET}},
            {JSType::JS_NUMBER_FORMAT, {
                JSNumberFormat::NUMBER_STRING_SYSTEM_OFFSET - JSNumberFormat::LOCALE_OFFSET,
                JSNumberFormat::CURRENCY_OFFSET - JSNumberFormat::NUMBER_STRING_SYSTEM_OFFSET,
                JSNumberFormat::UNIT_OFFSET - JSNumberFormat::CURRENCY_OFFSET,
                JSNumberFormat::MINIMUM_INTEGER_DIGITS_OFFSET - JSNumberFormat::UNIT_OFFSET,
                JSNumberFormat::MINIMUM_FRACTION_DIGITS_OFFSET - JSNumberFormat::MINIMUM_INTEGER_DIGITS_OFFSET,
                JSNumberFormat::MAXIMUM_FRACTION_DIGITS_OFFSET - JSNumberFormat::MINIMUM_FRACTION_DIGITS_OFFSET,
                JSNumberFormat::MINIMUM_SIGNIFICANT_DIGITS_OFFSET - JSNumberFormat::MAXIMUM_FRACTION_DIGITS_OFFSET,
                JSNumberFormat::MAXIMUM_SIGNIFICANT_DIGITS_OFFSET - JSNumberFormat::MINIMUM_SIGNIFICANT_DIGITS_OFFSET,
                JSNumberFormat::USER_GROUPING_OFFSET - JSNumberFormat::MAXIMUM_SIGNIFICANT_DIGITS_OFFSET,
                JSNumberFormat::BOUND_FORMAT_OFFSET - JSNumberFormat::USER_GROUPING_OFFSET,
                JSNumberFormat::ICU_FIELD_OFFSET - JSNumberFormat::BOUND_FORMAT_OFFSET,
                JSNumberFormat::BIT_FIELD_OFFSET - JSNumberFormat::ICU_FIELD_OFFSET}},
            {JSType::JS_OBJECT, {
                JSObject::ELEMENTS_OFFSET - JSObject::PROPERTIES_OFFSET,
                JSObject::SIZE - JSObject::ELEMENTS_OFFSET}},
            {JSType::JS_OOM_ERROR, {
                JSObject::ELEMENTS_OFFSET - JSObject::PROPERTIES_OFFSET,
                JSObject::SIZE - JSObject::ELEMENTS_OFFSET}},
            {JSType::JS_PLURAL_RULES, {
                JSPluralRules::MINIMUM_INTEGER_DIGITS_OFFSET - JSPluralRules::LOCALE_OFFSET,
                JSPluralRules::MINIMUM_FRACTION_DIGITS_OFFSET - JSPluralRules::MINIMUM_INTEGER_DIGITS_OFFSET,
                JSPluralRules::MAXIMUM_FRACTION_DIGITS_OFFSET - JSPluralRules::MINIMUM_FRACTION_DIGITS_OFFSET,
                JSPluralRules::MININUM_SIGNIFICANT_DIGITS_OFFSET - JSPluralRules::MAXIMUM_FRACTION_DIGITS_OFFSET,
                JSPluralRules::MAXINUM_SIGNIFICANT_DIGITS_OFFSET - JSPluralRules::MININUM_SIGNIFICANT_DIGITS_OFFSET,
                JSPluralRules::ICU_PLURAL_RULES_OFFSET - JSPluralRules::MAXINUM_SIGNIFICANT_DIGITS_OFFSET,
                JSPluralRules::ICU_NUMBER_FORMAT_OFFSET - JSPluralRules::ICU_PLURAL_RULES_OFFSET,
                JSPluralRules::BIT_FIELD_OFFSET - JSPluralRules::ICU_NUMBER_FORMAT_OFFSET}},
            {JSType::JS_PRIMITIVE_REF, {JSPrimitiveRef::SIZE - JSPrimitiveRef::VALUE_OFFSET}},
            {JSType::JS_PROMISE, {
                JSPromise::PROMISE_FULFILL_REACTIONS_OFFSET - JSPromise::PROMISE_RESULT_OFFSET,
                JSPromise::PROMISE_REJECT_REACTIONS_OFFSET - JSPromise::PROMISE_FULFILL_REACTIONS_OFFSET,
                JSPromise::BIT_FIELD_OFFSET - JSPromise::PROMISE_REJECT_REACTIONS_OFFSET}},
            {JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION, {
                JSPromiseAllResolveElementFunction::VALUES_OFFSET - JSPromiseAllResolveElementFunction::INDEX_OFFSET,
                JSPromiseAllResolveElementFunction::CAPABILITIES_OFFSET - JSPromiseAllResolveElementFunction::VALUES_OFFSET,
                JSPromiseAllResolveElementFunction::REMAINING_ELEMENTS_OFFSET - JSPromiseAllResolveElementFunction::CAPABILITIES_OFFSET,
                JSPromiseAllResolveElementFunction::ALREADY_CALLED_OFFSET - JSPromiseAllResolveElementFunction::REMAINING_ELEMENTS_OFFSET,
                JSPromiseAllResolveElementFunction::SIZE - JSPromiseAllResolveElementFunction::ALREADY_CALLED_OFFSET}},
            {JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION, {
                JSPromiseAllSettledElementFunction::VALUES_OFFSET - JSPromiseAllSettledElementFunction::ALREADY_CALLED_OFFSET,
                JSPromiseAllSettledElementFunction::CAPABILITY_OFFSET - JSPromiseAllSettledElementFunction::VALUES_OFFSET,
                JSPromiseAllSettledElementFunction::REMAINING_ELEMENTS_OFFSET - JSPromiseAllSettledElementFunction::CAPABILITY_OFFSET,
                JSPromiseAllSettledElementFunction::INDEX_OFFSET - JSPromiseAllSettledElementFunction::REMAINING_ELEMENTS_OFFSET}},
            {JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION, {
                JSPromiseAnyRejectElementFunction::CAPABILITY_OFFSET - JSPromiseAnyRejectElementFunction::ERRORS_OFFSET,
                JSPromiseAnyRejectElementFunction::REMAINING_ELEMENTS_OFFSET - JSPromiseAnyRejectElementFunction::CAPABILITY_OFFSET,
                JSPromiseAnyRejectElementFunction::ALREADY_CALLED_OFFSET - JSPromiseAnyRejectElementFunction::REMAINING_ELEMENTS_OFFSET,
                JSPromiseAnyRejectElementFunction::INDEX_OFFSET - JSPromiseAnyRejectElementFunction::ALREADY_CALLED_OFFSET}},
            {JSType::JS_PROMISE_EXECUTOR_FUNCTION, {
                JSPromiseExecutorFunction::SIZE - JSPromiseExecutorFunction::CAPABILITY_OFFSET}},
            {JSType::JS_PROMISE_FINALLY_FUNCTION, {
                JSPromiseFinallyFunction::ONFINALLY_OFFSET - JSPromiseFinallyFunction::CONSTRUCTOR_OFFSET,
                JSPromiseFinallyFunction::SIZE - JSPromiseFinallyFunction::ONFINALLY_OFFSET}},
            {JSType::JS_PROMISE_REACTIONS_FUNCTION, {
                JSPromiseReactionsFunction::ALREADY_RESOLVED_OFFSET - JSPromiseReactionsFunction::PROMISE_OFFSET,
                JSPromiseReactionsFunction::SIZE - JSPromiseReactionsFunction::ALREADY_RESOLVED_OFFSET}},
            {JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION, {
                JSPromiseValueThunkOrThrowerFunction::SIZE - JSPromiseValueThunkOrThrowerFunction::RESULT_OFFSET}},
            {JSType::JS_PROXY, {JSProxy::HANDLER_OFFSET - JSProxy::TARGET_OFFSET,
                                JSProxy::METHOD_OFFSET - JSProxy::HANDLER_OFFSET,
                                JSProxy::PRIVATE_FIELD_OFFSET - JSProxy::METHOD_OFFSET,
                                JSProxy::BIT_FIELD_OFFSET - JSProxy::PRIVATE_FIELD_OFFSET}},
            {JSType::JS_PROXY_REVOC_FUNCTION, {
                JSProxyRevocFunction::SIZE - JSProxyRevocFunction::REVOCABLE_PROXY_OFFSET}},
            {JSType::JS_RANGE_ERROR, {JSObject::ELEMENTS_OFFSET - JSObject::PROPERTIES_OFFSET,
                                      JSObject::SIZE - JSObject::ELEMENTS_OFFSET}},
            {JSType::JS_REALM, {JSRealm::GLOBAL_ENV_OFFSET - JSRealm::VALUE_OFFSET,
                                JSRealm::SIZE - JSRealm::GLOBAL_ENV_OFFSET}},
            {JSType::JS_REFERENCE_ERROR, {JSObject::ELEMENTS_OFFSET - JSObject::PROPERTIES_OFFSET,
                                          JSObject::SIZE - JSObject::ELEMENTS_OFFSET}},
            {JSType::JS_REG_EXP, {JSRegExp::ORIGINAL_SOURCE_OFFSET - JSRegExp::REGEXP_BYTE_CODE_OFFSET,
                                 JSRegExp::ORIGINAL_FLAGS_OFFSET - JSRegExp::ORIGINAL_SOURCE_OFFSET,
                                 JSRegExp::GROUP_NAME_OFFSET - JSRegExp::ORIGINAL_FLAGS_OFFSET,
                                 JSRegExp::LENGTH_OFFSET - JSRegExp::GROUP_NAME_OFFSET}},
            {JSType::JS_REG_EXP_ITERATOR, {
                JSRegExpIterator::ITERATED_STRING_OFFSET - JSRegExpIterator::ITERATING_REGEXP_OFFSET,
                JSRegExpIterator::BIT_FIELD_OFFSET - JSRegExpIterator::ITERATED_STRING_OFFSET}},
            {JSType::JS_RELATIVE_TIME_FORMAT, {
                JSRelativeTimeFormat::NUMBERING_SYSTEM_OFFSET - JSRelativeTimeFormat::LOCALE_OFFSET,
                JSRelativeTimeFormat::ICU_FIELD_OFFSET - JSRelativeTimeFormat::NUMBERING_SYSTEM_OFFSET,
                JSRelativeTimeFormat::BIT_FIELD_OFFSET - JSRelativeTimeFormat::ICU_FIELD_OFFSET}},
            {JSType::JS_SEGMENTER, {JSSegmenter::ICU_FIELD_OFFSET - JSSegmenter::LOCALE_OFFSET,
                                    JSSegmenter::BIT_FIELD_OFFSET - JSSegmenter::ICU_FIELD_OFFSET}},
            {JSType::JS_SEGMENTS, {JSSegments::SEGMENTS_STRING_OFFSET - JSSegments::ICU_FIELD_OFFSET,
                                   JSSegments::UNICODE_STRING_OFFSET - JSSegments::SEGMENTS_STRING_OFFSET,
                                   JSSegments::BIT_FIELD_OFFSET - JSSegments::UNICODE_STRING_OFFSET}},
            {JSType::JS_SEGMENT_ITERATOR, {
                JSSegmentIterator::SEGMENTS_STRING_OFFSET - JSSegmentIterator::ICU_FIELD_OFFSET,
                JSSegmentIterator::UNICODE_STRING_OFFSET - JSSegmentIterator::SEGMENTS_STRING_OFFSET,
                JSSegmentIterator::BIT_FIELD_OFFSET - JSSegmentIterator::UNICODE_STRING_OFFSET}},
            {JSType::JS_SENDABLE_ARRAY_BUFFER, {
                JSSendableArrayBuffer::BYTE_LENGTH_OFFSET- JSSendableArrayBuffer::DATA_OFFSET}},
            {JSType::JS_SET, {JSSet::SIZE - JSSet::LINKED_SET_OFFSET}},
            {JSType::JS_SET_ITERATOR, {
                JSSetIterator::NEXT_INDEX_OFFSET - JSSetIterator::ITERATED_SET_OFFSET}},
            {JSType::JS_SHARED_ARRAY, {
                JSSharedArray::MOD_RECORD_OFFSET - JSSharedArray::TRACK_INFO_OFFSET}},
            {JSType::JS_SHARED_ARRAY_BUFFER, {
                JSArrayBuffer::BYTE_LENGTH_OFFSET - JSArrayBuffer::DATA_OFFSET}},
            {JSType::JS_SHARED_ARRAY_ITERATOR, {
                JSSharedArrayIterator::NEXT_INDEX_OFFSET - JSSharedArrayIterator::ITERATED_ARRAY_OFFSET}},
            {JSType::JS_SHARED_ASYNC_FUNCTION, {}},
            {JSType::JS_SHARED_BIGINT64_ARRAY, {}},
            {JSType::JS_SHARED_BIGUINT64_ARRAY, {}},
            {JSType::JS_SHARED_FLOAT32_ARRAY, {}},
            {JSType::JS_SHARED_FLOAT64_ARRAY, {}},
            {JSType::JS_SHARED_FUNCTION, {}},
            {JSType::JS_SHARED_INT16_ARRAY, {}},
            {JSType::JS_SHARED_INT32_ARRAY, {}},
            {JSType::JS_SHARED_INT8_ARRAY, {}},
            {JSType::JS_SHARED_MAP, {
                JSSharedMap::MOD_RECORD_OFFSET - JSSharedMap::LINKED_MAP_OFFSET}},
            {JSType::JS_SHARED_MAP_ITERATOR, {
                JSSharedMapIterator::NEXT_INDEX_OFFSET - JSSharedMapIterator::ITERATED_MAP_OFFSET}},
            {JSType::JS_SHARED_OBJECT, {}},
            {JSType::JS_SHARED_SET, {
                JSSharedSet::MOD_RECORD_OFFSET - JSSharedSet::LINKED_SET_OFFSET}},
            {JSType::JS_SHARED_SET_ITERATOR, {
                JSSharedSetIterator::NEXT_INDEX_OFFSET - JSSharedSetIterator::ITERATED_SET_OFFSET}},
            {JSType::JS_SHARED_TYPED_ARRAY, {}},
            {JSType::JS_SHARED_UINT16_ARRAY, {}},
            {JSType::JS_SHARED_UINT32_ARRAY, {}},
            {JSType::JS_SHARED_UINT8_ARRAY, {}},
            {JSType::JS_SHARED_UINT8_CLAMPED_ARRAY, {}},
            {JSType::JS_STRING_ITERATOR, {
                JSStringIterator::STRING_ITERATOR_NEXT_INDEX_OFFSET - JSStringIterator::ITERATED_STRING_OFFSET}},
            {JSType::JS_SYNTAX_ERROR, {
                JSObject::ELEMENTS_OFFSET - JSObject::PROPERTIES_OFFSET, JSObject::SIZE - JSObject::ELEMENTS_OFFSET}},
            {JSType::JS_TERMINATION_ERROR, {
                JSObject::ELEMENTS_OFFSET - JSObject::PROPERTIES_OFFSET, JSObject::SIZE - JSObject::ELEMENTS_OFFSET}},
            {JSType::JS_TYPED_ARRAY, {
                JSTypedArray::TYPED_ARRAY_NAME_OFFSET - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                JSTypedArray::BYTE_LENGTH_OFFSET - JSTypedArray::TYPED_ARRAY_NAME_OFFSET}},
            {JSType::JS_TYPE_ERROR, {
                JSObject::ELEMENTS_OFFSET - JSObject::PROPERTIES_OFFSET, JSObject::SIZE - JSObject::ELEMENTS_OFFSET}},
            {JSType::JS_UINT16_ARRAY, {
                JSTypedArray::TYPED_ARRAY_NAME_OFFSET - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                JSTypedArray::BYTE_LENGTH_OFFSET - JSTypedArray::TYPED_ARRAY_NAME_OFFSET}},
            {JSType::JS_UINT32_ARRAY, {
                JSTypedArray::TYPED_ARRAY_NAME_OFFSET - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                JSTypedArray::BYTE_LENGTH_OFFSET - JSTypedArray::TYPED_ARRAY_NAME_OFFSET}},
            {JSType::JS_UINT8_ARRAY, {
                JSTypedArray::TYPED_ARRAY_NAME_OFFSET - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                JSTypedArray::BYTE_LENGTH_OFFSET - JSTypedArray::TYPED_ARRAY_NAME_OFFSET}},
            {JSType::JS_UINT8_CLAMPED_ARRAY, {
                JSTypedArray::TYPED_ARRAY_NAME_OFFSET - JSTypedArray::VIEWED_ARRAY_BUFFER_OFFSET,
                JSTypedArray::BYTE_LENGTH_OFFSET - JSTypedArray::TYPED_ARRAY_NAME_OFFSET}},
            {JSType::JS_URI_ERROR, {
                JSObject::ELEMENTS_OFFSET - JSObject::PROPERTIES_OFFSET,
                JSObject::SIZE - JSObject::ELEMENTS_OFFSET}},
            {JSType::JS_WEAK_MAP, {JSWeakMap::SIZE - JSWeakMap::LINKED_MAP_OFFSET}},
            {JSType::JS_WEAK_REF, {JSWeakRef::SIZE - JSWeakRef::WEAK_OBJECT_OFFSET}},
            {JSType::JS_WEAK_SET, {JSWeakSet::SIZE - JSWeakSet::LINKED_SET_OFFSET}},
            {JSType::LEXICAL_ENV, {}},
            {JSType::LINE_STRING, {}},
            {JSType::LINKED_NODE, {LinkedNode::SIZE - LinkedNode::NEXT_OFFSET}},
            {JSType::LOCAL_EXPORTENTRY_RECORD, {
                LocalExportEntry::LOCAL_NAME_OFFSET - LocalExportEntry::LOCAL_EXPORT_ENTRY_OFFSET,
                LocalExportEntry::LOCAL_INDEX_OFFSET - LocalExportEntry::LOCAL_NAME_OFFSET}},
            {JSType::MACHINE_CODE_OBJECT, {}},
            {JSType::MARKER_CELL, {}},
            {JSType::METHOD, {Method::CALL_FIELD_OFFSET - Method::CONSTANT_POOL_OFFSET}},
            {JSType::MICRO_JOB_QUEUE, {
                job::MicroJobQueue::SCRIPT_JOB_QUEUE_OFFSET - job::MicroJobQueue::PROMISE_JOB_QUEUE_OFFSET,
                job::MicroJobQueue::SIZE - job::MicroJobQueue::SCRIPT_JOB_QUEUE_OFFSET}},
            {JSType::MODULE_RECORD, {}},
            {JSType::MUTANT_TAGGED_ARRAY, {}},
            {JSType::NATIVE_MODULE_FAILURE_INFO, {
                NativeModuleFailureInfo::SIZE - NativeModuleFailureInfo::ARK_NATIVE_MODULE_FAILURE_INFO_OFFSET}},
            {JSType::PENDING_JOB, {job::PendingJob::ARGUMENT_OFFSET - job::PendingJob::JOB_OFFSET,
                                   job::PendingJob::SIZE - job::PendingJob::ARGUMENT_OFFSET}},
            {JSType::PROFILE_TYPE_INFO, {}},
            {JSType::PROFILE_TYPE_INFO_CELL_0, {
                ProfileTypeInfoCell::MACHINE_CODE_OFFSET - ProfileTypeInfoCell::VALUE_OFFSET,
                ProfileTypeInfoCell::BASELINE_CODE_OFFSET - ProfileTypeInfoCell::MACHINE_CODE_OFFSET,
                ProfileTypeInfoCell::HANDLE_OFFSET - ProfileTypeInfoCell::BASELINE_CODE_OFFSET,
                ProfileTypeInfoCell::LAST_OFFSET - ProfileTypeInfoCell::HANDLE_OFFSET}},
            {JSType::PROFILE_TYPE_INFO_CELL_1, {
                ProfileTypeInfoCell::MACHINE_CODE_OFFSET - ProfileTypeInfoCell::VALUE_OFFSET,
                ProfileTypeInfoCell::BASELINE_CODE_OFFSET - ProfileTypeInfoCell::MACHINE_CODE_OFFSET,
                ProfileTypeInfoCell::HANDLE_OFFSET - ProfileTypeInfoCell::BASELINE_CODE_OFFSET,
                ProfileTypeInfoCell::LAST_OFFSET - ProfileTypeInfoCell::HANDLE_OFFSET}},
            {JSType::PROFILE_TYPE_INFO_CELL_N, {
                ProfileTypeInfoCell::MACHINE_CODE_OFFSET - ProfileTypeInfoCell::VALUE_OFFSET,
                ProfileTypeInfoCell::BASELINE_CODE_OFFSET - ProfileTypeInfoCell::MACHINE_CODE_OFFSET,
                ProfileTypeInfoCell::HANDLE_OFFSET - ProfileTypeInfoCell::BASELINE_CODE_OFFSET,
                ProfileTypeInfoCell::LAST_OFFSET - ProfileTypeInfoCell::HANDLE_OFFSET}},
            {JSType::PROGRAM, {Program::SIZE - Program::MAIN_FUNCTION_OFFSET}},
            {JSType::PROMISE_CAPABILITY, {
                PromiseCapability::RESOLVE_OFFSET - PromiseCapability::PROMISE_OFFSET,
                PromiseCapability::REJECT_OFFSET - PromiseCapability::RESOLVE_OFFSET,
                PromiseCapability::SIZE - PromiseCapability::REJECT_OFFSET}},
            {JSType::PROMISE_ITERATOR_RECORD, {
                PromiseIteratorRecord::BIT_FIELD_OFFSET - PromiseIteratorRecord::ITERATOR_OFFSET}},
            {JSType::PROMISE_REACTIONS, {
                PromiseReaction::HANDLER_OFFSET - PromiseReaction::PROMISE_CAPABILITY_OFFSET,
                PromiseReaction::BIT_FIELD_OFFSET - PromiseReaction::HANDLER_OFFSET}},
            {JSType::PROMISE_RECORD, {PromiseRecord::SIZE - PromiseRecord::VALUE_OFFSET}},
            {JSType::PROPERTY_BOX, {PropertyBox::SIZE - PropertyBox::VALUE_OFFSET}},
            {JSType::PROTOTYPE_HANDLER, {
                PrototypeHandler::PROTO_CELL_OFFSET - PrototypeHandler::HANDLER_INFO_OFFSET,
                PrototypeHandler::HOLDER_OFFSET - PrototypeHandler::PROTO_CELL_OFFSET,
                PrototypeHandler::ACCESSOR_JSFUNCTION_OFFSET - PrototypeHandler::HOLDER_OFFSET,
                PrototypeHandler::ACCESSOR_METHOD_ID_OFFSET - PrototypeHandler::PrototypeHandler::ACCESSOR_JSFUNCTION_OFFSET}},
            {JSType::PROTOTYPE_INFO, {
                ProtoChangeDetails::REGISTER_INDEX_OFFSET - ProtoChangeDetails::CHANGE_LISTENER_OFFSET}},
            {JSType::PROTO_CHANGE_MARKER, {}},
            {JSType::RB_TREENODE, {
                RBTreeNode::RIGHT_OFFSET - RBTreeNode::LEFT_OFFSET,
                RBTreeNode::ISRED_OFFSET - RBTreeNode::RIGHT_OFFSET,
                RBTreeNode::COUNT_OFFSET - RBTreeNode::ISRED_OFFSET}},
            {JSType::RESOLVEDBINDING_RECORD, {
                ResolvedBinding::BINDING_NAME_OFFSET - ResolvedBinding::MODULE_OFFSET,
                ResolvedBinding::SIZE - ResolvedBinding::BINDING_NAME_OFFSET}},
            {JSType::RESOLVEDINDEXBINDING_RECORD, {
                ResolvedIndexBinding::INDEX_OFFSET - ResolvedIndexBinding::MODULE_OFFSET,
                ResolvedIndexBinding::BIT_FIELD_OFFSET - ResolvedIndexBinding::INDEX_OFFSET,
                ResolvedIndexBinding::END_OFFSET - ResolvedIndexBinding::BIT_FIELD_OFFSET}},
            {JSType::RESOLVEDRECORDBINDING_RECORD, {
                ResolvedRecordBinding::BINDING_NAME_OFFSET - ResolvedRecordBinding::MODULE_RECORD_OFFSET,
                ResolvedRecordBinding::SIZE - ResolvedRecordBinding::BINDING_NAME_OFFSET}},
            {JSType::RESOLVEDRECORDINDEXBINDING_RECORD, {
                ResolvedRecordIndexBinding::ABC_FILE_NAME - ResolvedRecordIndexBinding::MODULE_RECORD_INDEX_OFFSET,
                ResolvedRecordIndexBinding::INDEX_OFFSET - ResolvedRecordIndexBinding::ABC_FILE_NAME}},
            {JSType::RESOLVING_FUNCTIONS_RECORD, {
                ResolvingFunctionsRecord::REJECT_FUNCTION_OFFSET - ResolvingFunctionsRecord::RESOLVE_FUNCTION_OFFSET,
                ResolvingFunctionsRecord::SIZE - ResolvingFunctionsRecord::REJECT_FUNCTION_OFFSET}},
            {JSType::SENDABLE_ENV, {}},
            {JSType::SFUNCTION_ENV, {}},
            {JSType::JS_XREF_OBJECT, {}},
            {JSType::SLICED_STRING, {SlicedString::STARTINDEX_AND_FLAGS_OFFSET - SlicedString::PARENT_OFFSET}},
            {JSType::SOURCE_TEXT_MODULE_RECORD, {
                SourceTextModule::NAMESPACE_OFFSET - SourceTextModule::SOURCE_TEXT_MODULE_OFFSET,
                SourceTextModule::MODULE_REQUESTS_OFFSET - SourceTextModule::NAMESPACE_OFFSET,
                SourceTextModule::REQUESTED_MODULES_OFFSET - SourceTextModule::MODULE_REQUESTS_OFFSET,
                SourceTextModule::IMPORT_ENTRIES_OFFSET - SourceTextModule::REQUESTED_MODULES_OFFSET,
                SourceTextModule::LOCAL_EXPORT_ENTTRIES_OFFSET - SourceTextModule::IMPORT_ENTRIES_OFFSET,
                SourceTextModule::INDIRECT_EXPORT_ENTTRIES_OFFSET - SourceTextModule::LOCAL_EXPORT_ENTTRIES_OFFSET,
                SourceTextModule::START_EXPORT_ENTTRIES_OFFSET - SourceTextModule::INDIRECT_EXPORT_ENTTRIES_OFFSET,
                SourceTextModule::NAME_DICTIONARY_OFFSET - SourceTextModule::START_EXPORT_ENTTRIES_OFFSET,
                SourceTextModule::CYCLE_ROOT_OFFSET - SourceTextModule::NAME_DICTIONARY_OFFSET,
                SourceTextModule::TOP_LEVEL_CAPABILITY_OFFSET - SourceTextModule::CYCLE_ROOT_OFFSET,
                SourceTextModule::ASYNC_PARENT_MODULES_OFFSET - SourceTextModule::TOP_LEVEL_CAPABILITY_OFFSET,
                SourceTextModule::SENDABLE_ENV_OFFSET - SourceTextModule::ASYNC_PARENT_MODULES_OFFSET,
                SourceTextModule::EXCEPTION_OFFSET - SourceTextModule::SENDABLE_ENV_OFFSET,
                SourceTextModule::DFS_ANCESTOR_INDEX_OFFSET - SourceTextModule::EXCEPTION_OFFSET}},
            {JSType::STAR_EXPORTENTRY_RECORD, {StarExportEntry::SIZE - StarExportEntry::STAR_EXPORT_ENTRY_OFFSET}},
            {JSType::STORE_TS_HANDLER, {StoreAOTHandler::PROTO_CELL_OFFSET - StoreAOTHandler::HANDLER_INFO_OFFSET,
                                        StoreAOTHandler::HOLDER_OFFSET - StoreAOTHandler::PROTO_CELL_OFFSET,
                                        StoreAOTHandler::SIZE - StoreAOTHandler::HOLDER_OFFSET}},
            {JSType::SYMBOL, {JSSymbol::HASHFIELD_OFFSET - JSSymbol::DESCRIPTION_OFFSET}},
            {JSType::TAGGED_ARRAY, {
                TaggedArray::EXTRA_LENGTH_OFFSET - TaggedArray::LENGTH_OFFSET, TaggedArray::LENGTH_OFFSET}},
            {JSType::TAGGED_DICTIONARY, {}},
            {JSType::TEMPLATE_MAP, {}},
            {JSType::TRACK_INFO, {
                TrackInfo::CACHED_FUNC_OFFSET - TrackInfo::CACHED_HCLASS_OFFSET,
                TrackInfo::BIT_FIELD_OFFSET - TrackInfo::CACHED_FUNC_OFFSET}},
            {JSType::TRANSITION_HANDLER, {
                TransitionHandler::TRANSITION_HCLASS_OFFSET - TransitionHandler::HANDLER_INFO_OFFSET,
                TransitionHandler::SIZE - TransitionHandler::TRANSITION_HCLASS_OFFSET}},
            {JSType::TRANS_WITH_PROTO_HANDLER, {
                TransWithProtoHandler::TRANSITION_HCLASS_OFFSET - TransWithProtoHandler::HANDLER_INFO_OFFSET,
                TransWithProtoHandler::PROTO_CELL_OFFSET - TransWithProtoHandler::TRANSITION_HCLASS_OFFSET,
                TransWithProtoHandler::SIZE - TransWithProtoHandler::PROTO_CELL_OFFSET}},
            {JSType::TREE_STRING, {TreeString::SECOND_OFFSET - TreeString::FIRST_OFFSET,
                                   TreeString::SIZE - TreeString::SECOND_OFFSET}},
            {JSType::VTABLE, {}}
        };
    }

    ~JSMetadataTestHelper() = default;

    enum MetadataStatus {
        UNINITIALIZED = 0,
        INITIALIZED
    };

    struct Metadata {
        std::string name {};
        std::unordered_map<std::string, std::pair<int, int>> offsets {};
        int endOffset {};
        std::vector<std::string> parents {};
        int status {MetadataStatus::UNINITIALIZED};
    };

    void ReadAndParseTypeEnums(const std::string &filePath, CVector<std::string> &typeEnums)
    {
        std::ifstream jsonFile(filePath);
        if (!jsonFile.is_open()) {
            std::cout << "Fail to open type enums file: " << filePath << std::endl;
            return;
        }
        std::stringstream buffer;
        buffer << jsonFile.rdbuf();
        auto json = cJSON_ParseWithOpts(buffer.str().c_str(), nullptr, true);
        if (!json) {
            std::cout << "Fail to parse type enums cJSON" << std::endl;
            jsonFile.close();
            return;
        }
        jsonFile.close();
        cJSON *typeEnumsJson = cJSON_GetObjectItem(json, "type_enum");
        if (typeEnumsJson == nullptr) {
            std::cout << "Fail to get item \"type_enums\" from cJSON" << std::endl;
        }
        auto typeEnumsVisitor = [&typeEnums](const cJSON *item, int index) -> bool {
            typeEnums.push_back(item->string);
            return true;
        };
        if (!VisitArrayItems(typeEnumsJson, typeEnumsVisitor)) {
            std::cout << "Fail to get type enums: " << filePath << std::endl;
            return;
        }
    }

    void ReadAndParseMetadataJson(const std::string &filePath, Metadata &metadata)
    {
        // Open specific metadata json file and using cJSON to parse
        std::ifstream jsonFile(filePath);
        if (!jsonFile.is_open()) {
            std::cout << "Fail to open metadata file: " << filePath << std::endl;
            return;
        }
        std::stringstream buffer;
        buffer << jsonFile.rdbuf();
        auto json = cJSON_ParseWithOpts(buffer.str().c_str(), nullptr, true);
        if (!json) {
            std::cout << "Fail to read metadata file: " << filePath << std::endl;
            jsonFile.close();
            return;
        }
        jsonFile.close();
        // Get "name" from json and set it to metadata.name
        if (!GetStringFromJson(json, "name", metadata.name)) {
            std::cout << "Fail to read name: " << filePath << std::endl;
            return;
        }
        // Get "end_offset" from json and set it to metadata.endoffset
        if (!GetIntFromJson(json, "end_offset", metadata.endOffset)) {
            std::cout << "Fail to read endOffset: " << filePath << std::endl;
            return;
        }
        // Get "offsets" and "parents" arrays from json
        cJSON *offsets {};
        cJSON *parents {};
        if (!GetArrayFromJson(json, "offsets", &offsets)) {
            std::cout << "Fail to read offsets array: " << filePath << std::endl;
            return;
        }
        if (!GetArrayFromJson(json, "parents", &parents)) {
            std::cout << "Fail to read parents array: " << filePath << std::endl;
            return;
        }
        // Iterate over those two arrays and set items to metadata
        auto parentVisitor = [&filePath, &metadata](const cJSON *item, int index) -> bool {
            if (!cJSON_IsString(item)) {
                std::cout << "Fail to visit parent: " << filePath << ", parents index: " << index << std::endl;
                return false;
            }
            metadata.parents.emplace_back(item->valuestring);
            return true;
        };
        auto offsetVisitor = [&filePath, &metadata, this](const cJSON *item, int index) -> bool {
            std::string key {};
            int offset {};
            int size {};
            if (!GetStringFromJson(item, "name", key)) {
                std::cout << "Fail to visit name: " << filePath << ", offsets index " << index << std::endl;
                return false;
            }
            if (!GetIntFromJson(item, "offset", offset)) {
                std::cout << "Fail to visit offset: " << filePath << ", offsets index " << index << std::endl;
                return false;
            }
            if (!GetIntFromJson(item, "size", size)) {
                std::cout << "Fail to visit size: " << filePath << ", offsets index " << index << std::endl;
                return false;
            }
            metadata.offsets[key] = std::make_pair(offset, size);
            return true;
        };
        if (!VisitArrayItems(offsets, offsetVisitor)) {
            std::cout << "Fail to get offsets: " << filePath << std::endl;
            return;
        }
        if (!VisitArrayItems(parents, parentVisitor)) {
            std::cout << "Fail to get parents: " << filePath << std::endl;
            return;
        }
        // Set metadata status to INITIALIZED
        metadata.status = MetadataStatus::INITIALIZED;
    }

    bool Test(JSType type, Metadata &metadata)
    {
        auto fieldNames = GetFieldNamesByType(type);
        auto fieldOffsets = GetFieldOffsetsByType(type);
        auto sizeTable = GetFieldSizesByType(type);
        // Fields count should be the same as offsets'
        if (fieldNames.size() != fieldOffsets.size()) {
            std::cout << "Type fields' number inconsistent with offsets' number" << std::endl;
            return false;
        }
        // check each [name, offset] pair
        int index = 0;
        for (; index < fieldNames.size(); index++) {
            if (index == fieldNames.size() - 1) {
                // Check typename and endOffset, stored at the last index of each array
                if (!CheckTypeNameAndEndOffset(metadata.name, fieldNames[index],
                                               metadata.endOffset, fieldOffsets[index])) {
                    return false;
                }
            } else {
                // Check individual fields
                if (!CheckField(fieldNames[index], fieldOffsets[index], metadata, fieldOffsets[0], sizeTable[index])) {
                    return false;
                }
            }
        }
        // If any field left in metadata that has not been checked, return false
        if (!CheckLeftoverFieldInMetadata(metadata)) {
            return false;
        }
        // Check if parent array is the same
        if (!CheckParents(type, metadata)) {
            return false;
        }

        return true;
    }

    bool TestForTypeEnums(const CVector<std::string> typeEnums)
    {
        uint8_t type = 0;
        for (auto typeName : typeEnums) {
            CString typeDesc = JSHClass::GetJSTypeDesc(static_cast<JSType>(type));
            if (typeDesc.empty()) {
                std::cout << "JSType " << int(type) << " got "
                          << typeName << ", missed in JSTYPE_DECL" << std::endl;
                return false;
            }
            if (typeName != typeDesc.c_str()) {
                std::cout << "JSType " << int(type) << " expected "
                          << typeDesc << ", but got "
                          << typeName << std::endl;
                return false;
            }
            ++type;
        }
        return true;
    }

    bool TestForDictionaryLayout(const std::string &filePath)
    {
        std::ifstream jsonFile(filePath);
        if (!jsonFile.is_open()) {
            std::cout << "Fail to open metadata file: " << filePath << std::endl;
            return false;
        }
        std::stringstream buffer;
        buffer << jsonFile.rdbuf();
        auto json = cJSON_ParseWithOpts(buffer.str().c_str(), nullptr, true);
        if (!json) {
            std::cout << "Fail to read metadata file: " << filePath << std::endl;
            jsonFile.close();
            return false;
        }
        jsonFile.close();

        int value;
        if (!GetIntFromJson(json, "key_index", value)) {
            std::cout << "Fail to get key_index: " << filePath << std::endl;
            return false;
        }
        if (value != NameDictionary::ENTRY_KEY_INDEX) {
            std::cout << "Dictionary layout key_index inconsistent: " << value << " ";
            std::cout << NameDictionary::ENTRY_KEY_INDEX << std::endl;
            return false;
        }
        if (!GetIntFromJson(json, "value_index", value)) {
            std::cout << "Fail to get value_index: " << filePath << std::endl;
            return false;
        }
        if (value != NameDictionary::ENTRY_VALUE_INDEX) {
            std::cout << "Dictionary layout value_index inconsistent: " << value << " ";
            std::cout << NameDictionary::ENTRY_VALUE_INDEX << std::endl;
            return false;
        }
        if (!GetIntFromJson(json, "detail_index", value)) {
            std::cout << "Fail to get detail_index: " << filePath << std::endl;
            return false;
        }
        if (value != NameDictionary::ENTRY_DETAILS_INDEX) {
            std::cout << "Dictionary layout detail_index inconsistent: " << value << " ";
            std::cout << NameDictionary::ENTRY_DETAILS_INDEX << std::endl;
            return false;
        }
        if (!GetIntFromJson(json, "entry_size", value)) {
            std::cout << "Fail to get entry_size: " << filePath << std::endl;
            return false;
        }
        if (value != NameDictionary::ENTRY_SIZE) {
            std::cout << "Dictionary layout entry_size inconsistent: " << value << " ";
            std::cout << NameDictionary::ENTRY_SIZE << std::endl;
            return false;
        }
        if (!GetIntFromJson(json, "header_size", value)) {
            std::cout << "Fail to get header_size: " << filePath << std::endl;
            return false;
        }
        if (value != NameDictionary::TABLE_HEADER_SIZE) {
            std::cout << "Dictionary layout header_size inconsistent: " << value << " ";
            std::cout << NameDictionary::TABLE_HEADER_SIZE << std::endl;
            return false;
        }
        return true;
    }

    bool TestForTypeRange(std::string &filePath)
    {
        std::ifstream jsonFile(filePath);
        if (!jsonFile.is_open()) {
            std::cout << "Fail to open metadata file: " << filePath << std::endl;
            return false;
        }
        std::stringstream buffer;
        buffer << jsonFile.rdbuf();
        auto json = cJSON_ParseWithOpts(buffer.str().c_str(), nullptr, true);
        if (!json) {
            std::cout << "Fail to read metadata file: " << filePath << std::endl;
            jsonFile.close();
            return false;
        }
        jsonFile.close();

        std::string value;
        if (!GetStringFromJson(json, "string_first", value)) {
            std::cout << "Fail to read string_first: " << filePath << std::endl;
            return false;
        }
        if (value != "LINE_STRING") {
            std::cout << "string_first inconsistent" << std::endl;
            return false;
        }
        if (!GetStringFromJson(json, "string_last", value)) {
            std::cout << "Fail to read string_last: " << filePath << std::endl;
            return false;
        }
        if (value != "TREE_STRING") {
            std::cout << "string_last inconsistent" << std::endl;
            return false;
        }
        if (!GetStringFromJson(json, "js_object_first", value)) {
            std::cout << "Fail to read js_object_first: " << filePath << std::endl;
            return false;
        }
        if (value != "JS_OBJECT") {
            std::cout << "js_object_first inconsistent" << std::endl;
            return false;
        }
        if (!GetStringFromJson(json, "js_object_last", value)) {
            std::cout << "Fail to read js_object_last: " << filePath << std::endl;
            return false;
        }
        if (value != "JS_GLOBAL_OBJECT") {
            std::cout << "js_object_last inconsistent" << std::endl;
            return false;
        }
        return true;
    }

    static std::unordered_map<JSType, std::vector<std::string>> fieldNameTable_;
    static std::unordered_map<JSType, std::vector<size_t>> fieldOffsetTable_;
    static std::unordered_map<JSType, std::vector<std::string>> parentsTable_;
    static std::unordered_map<JSType, std::vector<size_t>> fieldSizeTable_;
private:
    std::vector<std::string> GetFieldNamesByType(JSType type)
    {
        return HasTypeInFieldNameTable(type) ? fieldNameTable_.at(type) : std::vector<std::string>();
    }

    std::vector<size_t> GetFieldOffsetsByType(JSType type)
    {
        return HasTypeInFieldOffsetTable(type) ? fieldOffsetTable_.at(type) : std::vector<size_t>();
    }

    std::vector<std::string> GetParentsByType(JSType type)
    {
        return HasTypeInParentsTable(type) ? parentsTable_.at(type) : std::vector<std::string>();
    }

    std::vector<size_t> GetFieldSizesByType(JSType type)
    {
        return HasTypeInFieldSizesTable(type) ? fieldSizeTable_.at(type) : std::vector<size_t>();
    }

    bool HasTypeInFieldNameTable(JSType type)
    {
        return !fieldNameTable_.empty() && fieldNameTable_.find(type) != fieldNameTable_.end();
    }

    bool HasTypeInFieldOffsetTable(JSType type)
    {
        return !fieldOffsetTable_.empty() && fieldOffsetTable_.find(type) != fieldOffsetTable_.end();
    }

    bool HasTypeInFieldSizesTable(JSType type)
    {
        return !fieldSizeTable_.empty() && fieldSizeTable_.find(type) != fieldSizeTable_.end();
    }

    bool HasTypeInParentsTable(JSType type)
    {
        return !parentsTable_.empty() && parentsTable_.find(type) != parentsTable_.end();
    }

    bool CheckTypeNameAndEndOffset(std::string &metaName, std::string &typeName, int metaEndOffset, int endOffset)
    {
        if (metaName != typeName || metaEndOffset != endOffset) {
            std::cout << "Type name or endOffset inconsistent" << std::endl;
            std::cout << "[" << metaName << "   " << metaEndOffset << "] [";
            std::cout << typeName << "   " << endOffset << "]" << std::endl;
            return false;
        }
        return true;
    }

    bool CheckField(std::string &fieldName, int fieldOffset, Metadata &metadata, int offsetAlign, int size)
    {
        if (metadata.offsets.find(fieldName) == metadata.offsets.end()) {
            // a field is defined in the code but not in metadata, ignored for now
            std::cout << "Type  " << metadata.name << " Field " << fieldName;
            std::cout << " is not defined in metadata" << std::endl;
            return true;
        }
        offsetAlign = metadata.name == "JS_NATIVE_POINTER" ? JSNativePointer::POINTER_OFFSET :
                      metadata.name == "JS_SHARED_ARRAY" ? JSSharedArray::LENGTH_OFFSET : offsetAlign;
        if ((fieldOffset - offsetAlign) != metadata.offsets[fieldName].first) {
            std::cout << "Type  " << metadata.name << " Field " << fieldName << " offset inconsistent" << std::endl;
            std::cout << "Metadata: " << metadata.offsets[fieldName].first;
            std::cout << ", Class definition: " << fieldOffset << std::endl;
            return false;
        }
        if (size != metadata.offsets[fieldName].second) {
            std::cout << "Type  " << metadata.name << " Field " << fieldName << " size inconsistent" << std::endl;
            std::cout << "Metadata: " << metadata.offsets[fieldName].second;
            std::cout << ", Class definition: " << size << std::endl;
            return false;
        }
        // remove fieldName in metadata that has been checked
        metadata.offsets.erase(fieldName);
        return true;
    }

    bool CheckLeftoverFieldInMetadata(Metadata &metadata)
    {
        if (!metadata.offsets.empty()) {
            std::cout << "Following fields of Type " << metadata.name;
            std::cout << ", defined in metadata has not been checked: " << std::endl;
            DumpFieldsInMetadata(metadata);
            return false;
        }
        return true;
    }

    bool CheckParents(JSType type, Metadata &metadata)
    {
        auto parents = GetParentsByType(type);
        if (parents.size() != metadata.parents.size()) {
            std::cout << "Type  " << metadata.name << " parents inconsistent" << std::endl;
            return false;
        }
        int index = 0;
        for (; index < parents.size(); index++) {
            if (parents.at(index) != metadata.parents.at(index)) {
                std::cout << "Type  " << metadata.name << " parents inconsistent" << std::endl;
                return false;
            }
        }
        return true;
    }

    void DumpFieldsInMetadata(Metadata &metadata)
    {
        for (const auto &field : metadata.offsets) {
            std::cout << "Field: " << field.first << " Offset: ";
            std::cout << field.second.first << " Size: " << field.second.second << std::endl;
        }
    }

    bool GetStringFromJson(const cJSON *json, const char *key, std::string &value)
    {
        auto item = cJSON_GetObjectItem(json, key);
        if (!item || !cJSON_IsString(item)) {
            return false;
        }
        value = item->valuestring;
        return true;
    }

    bool GetIntFromJson(const cJSON *json, const char *key, int &value)
    {
        auto item = cJSON_GetObjectItem(json, key);
        if (!item || !cJSON_IsNumber(item)) {
            return false;
        }
        value = static_cast<int>(item->valuedouble);
        return true;
    }

    bool GetArrayFromJson(const cJSON *json, const char *key, cJSON **value)
    {
        auto item = cJSON_GetObjectItem(json, key);
        if (!item || !cJSON_IsArray(item)) {
            return false;
        }
        *value = item;
        return true;
    }

    bool VisitArrayItems(const cJSON *json, ArrayVisitor &visitor)
    {
        int size = cJSON_GetArraySize(json);
        if (size < 0) {
            return false;
        }
        for (int index = 0; index < size; index++) {
            cJSON *item = cJSON_GetArrayItem(json, index);
            if (!visitor(item, index)) {
                return false;
            }
        }
        return true;
    }
};

std::unordered_map<JSType, std::vector<std::string>> JSMetadataTestHelper::fieldNameTable_ {};
std::unordered_map<JSType, std::vector<size_t>> JSMetadataTestHelper::fieldOffsetTable_ {};
std::unordered_map<JSType, std::vector<std::string>> JSMetadataTestHelper::parentsTable_ {};
std::unordered_map<JSType, std::vector<size_t>> JSMetadataTestHelper::fieldSizeTable_ {};

HWTEST_F_L0(JSMetadataTest, TestTypeEnumOrder)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"type_enums.json";
    CVector<std::string> typeEnums {};

    tester.ReadAndParseTypeEnums(metadataFilePath, typeEnums);
    ASSERT_EQ(typeEnums.size(), static_cast<size_t>(JSType::TYPE_LAST) + 1);
    ASSERT_TRUE(tester.TestForTypeEnums(typeEnums));
}

HWTEST_F_L0(JSMetadataTest, TestHClassMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"hclass.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::HCLASS, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestAccessorDataMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"accessor_data.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::ACCESSOR_DATA, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestAotLiteralInfoMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"aot_literal_info.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::AOT_LITERAL_INFO, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestAsyncGeneratorRequestMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"async_generator_request.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::ASYNC_GENERATOR_REQUEST, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestAsyncIteratorRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"async_iterator_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::ASYNC_ITERATOR_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestBigintMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"bigint.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::BIGINT, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestByteArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"byte_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::BYTE_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestCellRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"cell_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::CELL_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestClassInfoExtractorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"class_info_extractor.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::CLASS_INFO_EXTRACTOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestClassLiteralMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"class_literal.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::CLASS_LITERAL, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestCompletionRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"completion_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::COMPLETION_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestConstantPoolMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"constant_pool.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::CONSTANT_POOL, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestCowMutantTaggedArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"cow_mutant_tagged_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::COW_MUTANT_TAGGED_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestCowTaggedArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"cow_tagged_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::COW_TAGGED_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestExtraProfileTypeInfoMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"extra_profile_type_info.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::EXTRA_PROFILE_TYPE_INFO, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestFunctionTemplateMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"function_template.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::FUNCTION_TEMPLATE, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestGlobalEnvMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"global_env.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::GLOBAL_ENV, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestImportentryRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"importentry_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::IMPORTENTRY_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestIndirectExportentryRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"indirect_exportentry_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::INDIRECT_EXPORTENTRY_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestInternalAccessorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"internal_accessor.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::INTERNAL_ACCESSOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsAggregateErrorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_aggregate_error.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_AGGREGATE_ERROR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiArraylistIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_arraylist_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_ARRAYLIST_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiArrayListMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_array_list.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_ARRAY_LIST, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiBitvectorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_bitvector.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_BITVECTOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiBitvectorIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_bitvector_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_BITVECTOR_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiDequeMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_deque.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_DEQUE, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiDequeIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_deque_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_DEQUE_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiHashmapIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_hashmap_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_HASHMAP_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiHashsetIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_hashset_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_HASHSET_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiHashMapMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_hash_map.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_HASH_MAP, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiHashSetMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_hash_set.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_HASH_SET, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiLightWeightMapMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_light_weight_map.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_LIGHT_WEIGHT_MAP, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiLightWeightMapIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_light_weight_map_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiLightWeightSetMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_light_weight_set.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_LIGHT_WEIGHT_SET, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiLightWeightSetIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_light_weight_set_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiLinkedListMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_linked_list.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_LINKED_LIST, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiLinkedListIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_linked_list_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_LINKED_LIST_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiListMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_list.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_LIST, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiListIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_list_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_LIST_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiPlainArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_plain_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_PLAIN_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiPlainArrayIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_plain_array_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_PLAIN_ARRAY_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiQueueMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_queue.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_QUEUE, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiQueueIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_queue_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_QUEUE_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiStackMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_stack.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_STACK, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiStackIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_stack_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_STACK_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiTreemapIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_treemap_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_TREEMAP_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiTreesetIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_treeset_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_TREESET_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiTreeMapMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_tree_map.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_TREE_MAP, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiTreeSetMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_tree_set.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_TREE_SET, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiVectorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_vector.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_VECTOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsApiVectorIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_api_vector_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_API_VECTOR_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsArgumentsMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_arguments.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ARGUMENTS, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsArrayBufferMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_array_buffer.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ARRAY_BUFFER, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsArrayIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_array_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ARRAY_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsAsynciteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_asynciterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ASYNCITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsAsyncAwaitStatusFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_async_await_status_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsAsyncFromSyncIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_async_from_sync_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ASYNC_FROM_SYNC_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsAsyncFromSyncIterUnwarpFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_async_from_sync_iter_unwarp_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsAsyncFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_async_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ASYNC_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsAsyncFuncObjectMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_async_func_object.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ASYNC_FUNC_OBJECT, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsAsyncGeneratorFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_async_generator_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ASYNC_GENERATOR_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsAsyncGeneratorObjectMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_async_generator_object.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ASYNC_GENERATOR_OBJECT, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsAsyncGeneratorResumeNextReturnProcessorRstFtnMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath =
        METADATA_SOURCE_FILE_DIR"js_async_generator_resume_next.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsAsyncModuleFulfilledFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_async_module_fulfilled_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsAsyncModuleRejectedFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_async_module_rejected_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsBigint64ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_bigint64_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_BIGINT64_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsBiguint64ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_biguint64_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_BIGUINT64_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsBoundFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_bound_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_BOUND_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsCjsExportsMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_cjs_exports.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_CJS_EXPORTS, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsCjsModuleMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_cjs_module.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_CJS_MODULE, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsCjsRequireMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_cjs_require.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_CJS_REQUIRE, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsCollatorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_collator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_COLLATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsDataViewMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_data_view.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_DATA_VIEW, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsDateMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_date.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_DATE, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsDateTimeFormatMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_date_time_format.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_DATE_TIME_FORMAT, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsDisplaynamesMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_displaynames.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_DISPLAYNAMES, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsErrorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_error.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ERROR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsEvalErrorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_eval_error.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_EVAL_ERROR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsFinalizationRegistryMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_finalization_registry.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_FINALIZATION_REGISTRY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsFloat32ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_float32_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_FLOAT32_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsFloat64ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_float64_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_FLOAT64_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsForinIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_forin_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_FORIN_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsFunctionMetadata)
{
#if defined(PANDA_TARGET_AMD64) || defined(PANDA_TARGET_ARM64)
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_FUNCTION, metadata));
#endif
}

HWTEST_F_L0(JSMetadataTest, TestJsFunctionBaseMetadata)
{
#if defined(PANDA_TARGET_AMD64) || defined(PANDA_TARGET_ARM64)
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_function_base.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_FUNCTION_BASE, metadata));
#endif
}

HWTEST_F_L0(JSMetadataTest, TestJsGeneratorContextMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_generator_context.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_GENERATOR_CONTEXT, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsGeneratorFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_generator_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_GENERATOR_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsGeneratorObjectMetadata)
{
#if defined(PANDA_TARGET_AMD64) || defined(PANDA_TARGET_ARM64)
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_generator_object.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_GENERATOR_OBJECT, metadata));
#endif
}

HWTEST_F_L0(JSMetadataTest, TestJsGlobalObjectMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_global_object.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_GLOBAL_OBJECT, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsInt16ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_int16_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_INT16_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsInt32ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_int32_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_INT32_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsInt8ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_int8_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_INT8_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsIntlMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_intl.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_INTL, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsIntlBoundFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_intl_bound_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_INTL_BOUND_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsListFormatMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_list_format.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_LIST_FORMAT, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsLocaleMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_locale.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_LOCALE, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsMapMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_map.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_MAP, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsMapIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_map_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_MAP_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsModuleNamespaceMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_module_namespace.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_MODULE_NAMESPACE, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsNativePointerMetadata)
{
#if defined(PANDA_TARGET_AMD64) || defined(PANDA_TARGET_ARM64)
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_native_pointer.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_NATIVE_POINTER, metadata));
#endif
}

HWTEST_F_L0(JSMetadataTest, TestJsNumberFormatMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_number_format.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_NUMBER_FORMAT, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsObjectMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_object.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_OBJECT, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsOomErrorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_oom_error.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_OOM_ERROR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsPluralRulesMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_plural_rules.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_PLURAL_RULES, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsPrimitiveRefMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_primitive_ref.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_PRIMITIVE_REF, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsPromiseMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_promise.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_PROMISE, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsPromiseAllResolveElementFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_promise_all_resolve_element_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsPromiseAllSettledElementFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_promise_all_settled_element_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsPromiseAnyRejectElementFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_promise_any_reject_element_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsPromiseExecutorFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_promise_executor_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_PROMISE_EXECUTOR_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsPromiseFinallyFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_promise_finally_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_PROMISE_FINALLY_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsPromiseReactionsFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_promise_reactions_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_PROMISE_REACTIONS_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsPromiseValueThunkOrThrowerFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_promise_value_thunk_or_thrower_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsProxyMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_proxy.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_PROXY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsProxyRevocFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_proxy_revoc_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_PROXY_REVOC_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsRangeErrorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_range_error.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_RANGE_ERROR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsRealmMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_realm.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_REALM, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsReferenceErrorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_reference_error.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_REFERENCE_ERROR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsRegExpMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_reg_exp.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_REG_EXP, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsRegExpIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_reg_exp_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_REG_EXP_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsRelativeTimeFormatMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_relative_time_format.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_RELATIVE_TIME_FORMAT, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSegmenterMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_segmenter.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SEGMENTER, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSegmentsMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_segments.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SEGMENTS, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSegmentIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_segment_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SEGMENT_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSendableArrayBufferMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_sendable_array_buffer.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SENDABLE_ARRAY_BUFFER, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSetMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_set.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SET, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSetIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_set_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SET_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedArrayBufferMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_array_buffer.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_ARRAY_BUFFER, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedArrayIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_array_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_ARRAY_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedAsyncFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_async_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_ASYNC_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedBigint64ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_bigint64_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_BIGINT64_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedBiguint64ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_biguint64_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_BIGUINT64_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedFloat32ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_float32_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_FLOAT32_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedFloat64ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_float64_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_FLOAT64_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedFunctionMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_function.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_FUNCTION, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedInt16ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_int16_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_INT16_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedInt32ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_int32_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_INT32_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedInt8ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_int8_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_INT8_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedMapMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_map.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_MAP, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedMapIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_map_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_MAP_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedObjectMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_object.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_OBJECT, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedSetMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_set.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_SET, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedSetIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_set_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_SET_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedTypedArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_typed_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_TYPED_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedUint16ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_uint16_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_UINT16_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedUint32ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_uint32_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_UINT32_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedUint8ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_uint8_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_UINT8_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSharedUint8ClampedArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_shared_uint8_clamped_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SHARED_UINT8_CLAMPED_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsStringIteratorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_string_iterator.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_STRING_ITERATOR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsSyntaxErrorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_syntax_error.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_SYNTAX_ERROR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsTerminationErrorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_termination_error.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_TERMINATION_ERROR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsTypedArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_typed_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_TYPED_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsTypeErrorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_type_error.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_TYPE_ERROR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsUint16ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_uint16_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_UINT16_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsUint32ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_uint32_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_UINT32_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsUint8ArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_uint8_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_UINT8_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsUint8ClampedArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_uint8_clamped_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_UINT8_CLAMPED_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsUriErrorMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_uri_error.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_URI_ERROR, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsWeakMapMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_weak_map.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_WEAK_MAP, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsWeakRefMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_weak_ref.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_WEAK_REF, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsWeakSetMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"js_weak_set.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_WEAK_SET, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestLexicalEnvMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"lexical_env.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::LEXICAL_ENV, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestLineStringMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"line_string.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::LINE_STRING, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestLinkedNodeMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"linked_node.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::LINKED_NODE, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestLocalExportentryRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"local_exportentry_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::LOCAL_EXPORTENTRY_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestMachineCodeObjectMetadata)
{
#if defined(PANDA_TARGET_AMD64) || defined(PANDA_TARGET_ARM64)
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"machine_code_object.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::MACHINE_CODE_OBJECT, metadata));
#endif
}

HWTEST_F_L0(JSMetadataTest, TestMarkerCellMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"marker_cell.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::MARKER_CELL, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestMethodMetadata)
{
#if defined(PANDA_TARGET_AMD64) || defined(PANDA_TARGET_ARM64)
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"method.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::METHOD, metadata));
#endif
}

HWTEST_F_L0(JSMetadataTest, TestMicroJobQueueMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"micro_job_queue.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::MICRO_JOB_QUEUE, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestModuleRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"module_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::MODULE_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestMutantTaggedArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"mutant_tagged_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::MUTANT_TAGGED_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestNativeModuleFailureInfoMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"native_module_failure_info.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::NATIVE_MODULE_FAILURE_INFO, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestProfileTypeInfoMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"profile_type_info.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::PROFILE_TYPE_INFO, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestProfileTypeInfoCell0Metadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"profile_type_info_cell_0.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::PROFILE_TYPE_INFO_CELL_0, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestProfileTypeInfoCell1Metadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"profile_type_info_cell_1.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::PROFILE_TYPE_INFO_CELL_1, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestProfileTypeInfoCellNMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"profile_type_info_cell_n.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::PROFILE_TYPE_INFO_CELL_N, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestProgramMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"program.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::PROGRAM, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestPromiseCapabilityMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"promise_capability.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::PROMISE_CAPABILITY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestPromiseIteratorRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"promise_iterator_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::PROMISE_ITERATOR_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestPromiseReactionsMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"promise_reactions.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::PROMISE_REACTIONS, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestPromiseRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"promise_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::PROMISE_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestPropertyBoxMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"property_box.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::PROPERTY_BOX, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestPrototypeHandlerMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"prototype_handler.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::PROTOTYPE_HANDLER, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestPrototypeInfoMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"prototype_info.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::PROTOTYPE_INFO, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestProtoChangeMarkerMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"proto_change_marker.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::PROTO_CHANGE_MARKER, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestRbTreenodeMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"rb_treenode.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::RB_TREENODE, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestResolvedbindingRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"resolvedbinding_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::RESOLVEDBINDING_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestResolvedindexbindingRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"resolvedindexbinding_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::RESOLVEDINDEXBINDING_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestResolvedrecordbindingRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"resolvedrecordbinding_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::RESOLVEDRECORDBINDING_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestResolvedrecordindexbindingRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"resolvedrecordindexbinding_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::RESOLVEDRECORDINDEXBINDING_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestResolvingFunctionsRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"resolving_functions_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::RESOLVING_FUNCTIONS_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestSendableEnvMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"sendable_env.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::SENDABLE_ENV, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestSFunctionEnvMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"sfunction_env.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::SFUNCTION_ENV, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestJsXrefObjectMetadata) {
    JSMetadataTestHelper tester{};
    std::string metadataFilePath =
        METADATA_SOURCE_FILE_DIR "js_xref_object.json";
    JSMetadataTestHelper::Metadata metadata{};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::JS_XREF_OBJECT, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestSlicedStringMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"sliced_string.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::SLICED_STRING, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestSourceTextModuleRecordMetadata)
{
#if defined(PANDA_TARGET_AMD64) || defined(PANDA_TARGET_ARM64)
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"source_text_module_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::SOURCE_TEXT_MODULE_RECORD, metadata));
#endif
}

HWTEST_F_L0(JSMetadataTest, TestStarExportentryRecordMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"star_exportentry_record.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::STAR_EXPORTENTRY_RECORD, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestStoreAOTHandlerMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"store_ts_handler.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::STORE_TS_HANDLER, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestSymbolMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"symbol.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::SYMBOL, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestTaggedArrayMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"tagged_array.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::TAGGED_ARRAY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestTaggedDictionaryMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"tagged_dictionary.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::TAGGED_DICTIONARY, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestTemplateMapMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"template_map.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::TEMPLATE_MAP, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestTrackInfoMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"track_info.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::TRACK_INFO, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestTransitionHandlerMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"transition_handler.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::TRANSITION_HANDLER, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestTransWithProtoHandlerMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"trans_with_proto_handler.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::TRANS_WITH_PROTO_HANDLER, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestTreeStringMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"tree_string.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::TREE_STRING, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestVtableMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"vtable.json";
    JSMetadataTestHelper::Metadata metadata {};

    tester.ReadAndParseMetadataJson(metadataFilePath, metadata);
    ASSERT_TRUE(metadata.status == JSMetadataTestHelper::INITIALIZED);
    ASSERT_TRUE(tester.Test(JSType::VTABLE, metadata));
}

HWTEST_F_L0(JSMetadataTest, TestDictionaryLayoutMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"dictionary_layout.json";
    ASSERT_TRUE(tester.TestForDictionaryLayout(metadataFilePath));
}

HWTEST_F_L0(JSMetadataTest, TestTypeRangeMetadata)
{
    JSMetadataTestHelper tester {};
    std::string metadataFilePath = METADATA_SOURCE_FILE_DIR"type_range.json";
    ASSERT_TRUE(tester.TestForTypeRange(metadataFilePath));
}
}
