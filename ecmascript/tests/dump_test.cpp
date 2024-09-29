/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/accessor_data.h"
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#include "ecmascript/dfx/hprof/heap_snapshot.h"
#include "ecmascript/dfx/hprof/heap_snapshot_json_serializer.h"
#include "ecmascript/dfx/hprof/string_hashmap.h"
#include "ecmascript/dfx/native_module_failure_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_dictionary-inl.h"
#include "ecmascript/global_env.h"
#include "ecmascript/ic/ic_handler.h"
#include "ecmascript/ic/property_box.h"
#include "ecmascript/ic/proto_change_details.h"
#include "ecmascript/jobs/micro_job_queue.h"
#include "ecmascript/jobs/pending_job.h"
#include "ecmascript/jspandafile/class_info_extractor.h"
#include "ecmascript/jspandafile/class_literal.h"
#include "ecmascript/jspandafile/program_object.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_api/js_api_deque_iterator.h"
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
#include "ecmascript/js_async_from_sync_iterator.h"
#include "ecmascript/js_async_function.h"
#include "ecmascript/js_async_generator_object.h"
#include "ecmascript/js_bigint.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_dataview.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/js_for_in_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_generator_object.h"
#include "ecmascript/js_global_object.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_api/js_api_hashmap.h"
#include "ecmascript/js_api/js_api_hashmap_iterator.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_hashset_iterator.h"
#include "ecmascript/js_intl.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/js_map.h"
#include "ecmascript/js_map_iterator.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/js_displaynames.h"
#include "ecmascript/js_list_format.h"
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
#include "ecmascript/marker_cell.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/machine_code.h"
#include "ecmascript/module/js_module_source_text.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/shared_objects/js_sendable_arraybuffer.h"
#include "ecmascript/shared_objects/js_shared_array_iterator.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/shared_objects/js_shared_map_iterator.h"
#include "ecmascript/shared_objects/js_shared_set.h"
#include "ecmascript/shared_objects/js_shared_set_iterator.h"
#include "ecmascript/shared_objects/js_shared_typed_array.h"
#include "ecmascript/stubs/runtime_stubs.h"
#include "ecmascript/tagged_array.h"
#include "ecmascript/tagged_dictionary.h"
#include "ecmascript/tagged_hash_array.h"
#include "ecmascript/tagged_list.h"
#include "ecmascript/tagged_node.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/template_map.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/transitions_dictionary.h"
#include "ecmascript/require/js_cjs_module.h"
#include "ecmascript/require/js_cjs_require.h"
#include "ecmascript/require/js_cjs_exports.h"
#include <sstream>

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;
using Array = panda::ecmascript::builtins::BuiltinsArray;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CHECK_DUMP_FIELDS(begin, end, num);                                                                     \
    LOG_ECMA_IF((num) != ((end) - (begin)) / JSTaggedValue::TaggedTypeSize() && sizeof(uintptr_t) == 8, FATAL)  \
        << "Fields in obj are not in dump list. "

namespace panda::test {
class EcmaDumpTest : public BaseTestWithScope<false> {
};

#ifndef NDEBUG
HWTEST_F_L0(EcmaDumpTest, Dump)
{
    std::ostringstream os;

    JSTaggedValue value1(100);
    value1.Dump(os);

    JSTaggedValue value2(100.0);
    JSTaggedValue(value2.GetRawData()).Dump(os);

    JSTaggedValue::Undefined().Dump(os);
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    env.GetTaggedValue().Dump(os);

    JSHandle<JSFunction> objFunc(env->GetObjectFunction());
    objFunc.GetTaggedValue().Dump(os);
}
#endif  // #ifndef NDEBUG

HWTEST_F_L0(EcmaDumpTest, DumpTaggedValueType)
{
    std::vector<JSHandle<JSTaggedValue>> loopVal {};
    std::vector<std::string> loopRes {};
    // [Int]
    int32_t a = 1;
    JSTaggedValue intVal = JSTaggedValue(a);
    loopVal.push_back(JSHandle<JSTaggedValue>(thread, intVal));
    loopRes.push_back("[Int]");
    // [Double]
    double b = 3.14;
    JSTaggedValue doubleVal = JSTaggedValue(b);
    loopVal.push_back(JSHandle<JSTaggedValue>(thread, doubleVal));
    loopRes.push_back("[Double]");
    // [Undefine]
    JSTaggedValue undefineVal = JSTaggedValue::Undefined();
    loopVal.push_back(JSHandle<JSTaggedValue>(thread, undefineVal));
    loopRes.push_back("[Special Value] : Undefined");
    // [Null]
    JSTaggedValue nullVal = JSTaggedValue::Null();
    loopVal.push_back(JSHandle<JSTaggedValue>(thread, nullVal));
    loopRes.push_back("[Special Value] : Null");
    // JSArray
    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetArrayFunction());
    JSHandle<JSObject> globalObject(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(globalObject.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue arrVal = Array::ArrayConstructor(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    loopVal.push_back(JSHandle<JSTaggedValue>(thread, arrVal));
    loopRes.push_back("[Array]");
    // Object
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = globalEnv->GetObjectFunction();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> obj(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    loopVal.push_back(obj);
    loopRes.push_back("[Object]");

    for (int i = 0; i < loopVal.size(); i++) {
        std::stringstream oss;
        loopVal[i].GetTaggedValue().DumpTaggedValueType(oss);
        ASSERT_EQ(oss.str(), loopRes[i]);
    }
}

static JSHandle<JSMap> NewJSMap(JSThread *thread, ObjectFactory *factory, JSHandle<JSTaggedValue> proto)
{
    JSHandle<JSHClass> mapClass = factory->NewEcmaHClass(JSMap::SIZE, JSType::JS_MAP, proto);
    JSHandle<JSMap> jsMap = JSHandle<JSMap>::Cast(factory->NewJSObjectWithInit(mapClass));
    JSHandle<LinkedHashMap> linkedMap(LinkedHashMap::Create(thread));
    jsMap->SetLinkedMap(thread, linkedMap);
    return jsMap;
}

static JSHandle<JSSharedMap> NewJSSharedMap(JSThread *thread, ObjectFactory *factory)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetSFunctionPrototype();
    auto emptySLayout = thread->GlobalConstants()->GetHandledEmptySLayoutInfo();
    JSHandle<JSHClass> mapClass = factory->NewSEcmaHClass(JSSharedMap::SIZE, 0,
        JSType::JS_SHARED_MAP, proto, emptySLayout);
    JSHandle<JSSharedMap> jsMap = JSHandle<JSSharedMap>::Cast(factory->NewJSObjectWithInit(mapClass));
    JSHandle<LinkedHashMap> linkedMap(
        LinkedHashMap::Create(thread, LinkedHashMap::MIN_CAPACITY, MemSpaceKind::SHARED));
    jsMap->SetLinkedMap(thread, linkedMap);
    jsMap->SetModRecord(0);
    return jsMap;
}

static JSHandle<JSSet> NewJSSet(JSThread *thread, ObjectFactory *factory, JSHandle<JSTaggedValue> proto)
{
    JSHandle<JSHClass> setClass = factory->NewEcmaHClass(JSSet::SIZE, JSType::JS_SET, proto);
    JSHandle<JSSet> jsSet = JSHandle<JSSet>::Cast(factory->NewJSObjectWithInit(setClass));
    JSHandle<LinkedHashSet> linkedSet(LinkedHashSet::Create(thread));
    jsSet->SetLinkedSet(thread, linkedSet);
    return jsSet;
}

static JSHandle<JSSharedSet> NewJSSharedSet(JSThread *thread, ObjectFactory *factory)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetSFunctionPrototype();
    auto emptySLayout = thread->GlobalConstants()->GetHandledEmptySLayoutInfo();
    JSHandle<JSHClass> setClass = factory->NewSEcmaHClass(JSSharedSet::SIZE, 0,
        JSType::JS_SHARED_SET, proto, emptySLayout);
    JSHandle<JSSharedSet> jsSet = JSHandle<JSSharedSet>::Cast(factory->NewJSObjectWithInit(setClass));
    JSHandle<LinkedHashSet> linkedSet(
        LinkedHashSet::Create(thread, LinkedHashSet::MIN_CAPACITY, MemSpaceKind::SHARED));
    jsSet->SetLinkedSet(thread, linkedSet);
    jsSet->SetModRecord(0);
    return jsSet;
}

static JSHandle<JSAPIHashMap> NewJSAPIHashMap(JSThread *thread, ObjectFactory *factory)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> mapClass = factory->NewEcmaHClass(JSAPIHashMap::SIZE, JSType::JS_API_HASH_MAP, proto);
    JSHandle<JSAPIHashMap> jsHashMap = JSHandle<JSAPIHashMap>::Cast(factory->NewJSObjectWithInit(mapClass));
    jsHashMap->SetTable(thread, TaggedHashArray::Create(thread));
    jsHashMap->SetSize(0);
    return jsHashMap;
}

static JSHandle<JSAPIHashSet> NewJSAPIHashSet(JSThread *thread, ObjectFactory *factory)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> setClass = factory->NewEcmaHClass(JSAPIHashSet::SIZE, JSType::JS_API_HASH_SET, proto);
    JSHandle<JSAPIHashSet> jsHashSet = JSHandle<JSAPIHashSet>::Cast(factory->NewJSObjectWithInit(setClass));
    jsHashSet->SetTable(thread, TaggedHashArray::Create(thread));
    jsHashSet->SetSize(0);
    return jsHashSet;
}

static JSHandle<JSAPITreeMap> NewJSAPITreeMap(JSThread *thread, ObjectFactory *factory)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> mapClass = factory->NewEcmaHClass(JSAPITreeMap::SIZE, JSType::JS_API_TREE_MAP, proto);
    JSHandle<JSAPITreeMap> jsTreeMap = JSHandle<JSAPITreeMap>::Cast(factory->NewJSObjectWithInit(mapClass));
    JSHandle<TaggedTreeMap> treeMap(thread, TaggedTreeMap::Create(thread));
    jsTreeMap->SetTreeMap(thread, treeMap);
    return jsTreeMap;
}

static JSHandle<JSAPITreeSet> NewJSAPITreeSet(JSThread *thread, ObjectFactory *factory)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> setClass = factory->NewEcmaHClass(JSAPITreeSet::SIZE, JSType::JS_API_TREE_SET, proto);
    JSHandle<JSAPITreeSet> jsTreeSet = JSHandle<JSAPITreeSet>::Cast(factory->NewJSObjectWithInit(setClass));
    JSHandle<TaggedTreeSet> treeSet(thread, TaggedTreeSet::Create(thread));
    jsTreeSet->SetTreeSet(thread, treeSet);
    return jsTreeSet;
}

static JSHandle<JSAPIPlainArray> NewJSAPIPlainArray(JSThread *thread, ObjectFactory *factory)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> mapClass = factory->NewEcmaHClass(JSAPIPlainArray::SIZE, JSType::JS_API_PLAIN_ARRAY, proto);
    JSHandle<JSAPIPlainArray> jSAPIPlainArray = JSHandle<JSAPIPlainArray>::Cast(factory->NewJSObjectWithInit(mapClass));
    JSHandle<TaggedArray> keys =
            JSAPIPlainArray::CreateSlot(thread, JSAPIPlainArray::DEFAULT_CAPACITY_LENGTH);
    JSHandle<TaggedArray> values =
            JSAPIPlainArray::CreateSlot(thread, JSAPIPlainArray::DEFAULT_CAPACITY_LENGTH);
    jSAPIPlainArray->SetKeys(thread, keys);
    jSAPIPlainArray->SetValues(thread, values);
    jSAPIPlainArray->SetLength(0);
    return jSAPIPlainArray;
}

static JSHandle<JSAPIList> NewJSAPIList(JSThread *thread, ObjectFactory *factory)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> listClass = factory->NewEcmaHClass(JSAPIList::SIZE, JSType::JS_API_LIST, proto);
    JSHandle<JSAPIList> jsAPIList = JSHandle<JSAPIList>::Cast(factory->NewJSObjectWithInit(listClass));
    JSHandle<JSTaggedValue> taggedSingleList(thread, TaggedSingleList::Create(thread));
    jsAPIList->SetSingleList(thread, taggedSingleList);
    return jsAPIList;
}

static JSHandle<JSAPILinkedList> NewJSAPILinkedList(JSThread *thread, ObjectFactory *factory)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> mapClass = factory->NewEcmaHClass(JSAPILinkedList::SIZE, JSType::JS_API_LINKED_LIST, proto);
    JSHandle<JSAPILinkedList> jsAPILinkedList = JSHandle<JSAPILinkedList>::Cast(factory->NewJSObjectWithInit(mapClass));
    JSHandle<JSTaggedValue> linkedlist(thread, TaggedDoubleList::Create(thread));
    jsAPILinkedList->SetDoubleList(thread, linkedlist);
    return jsAPILinkedList;
}

static JSHandle<JSObject> NewJSObject(JSThread *thread, ObjectFactory *factory, JSHandle<GlobalEnv> globalEnv)
{
    JSFunction *jsFunc = globalEnv->GetObjectFunction().GetObject<JSFunction>();
    JSHandle<JSTaggedValue> jsFunc1(thread, jsFunc);
    JSHandle<JSObject> jsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc1), jsFunc1);
    return jsObj;
}

static JSHandle<JSAPIArrayList> NewJSAPIArrayList(JSThread *thread, ObjectFactory *factory,
                                                  JSHandle<JSTaggedValue> proto)
{
    JSHandle<JSHClass> arrayListClass =
        factory->NewEcmaHClass(JSAPIArrayList::SIZE, JSType::JS_API_ARRAY_LIST, proto);
    JSHandle<JSAPIArrayList> jsArrayList = JSHandle<JSAPIArrayList>::Cast(factory->NewJSObjectWithInit(arrayListClass));
    jsArrayList->SetLength(thread, JSTaggedValue(0));
    return jsArrayList;
}

static JSHandle<JSAPIStack> NewJSAPIStack(ObjectFactory *factory, JSHandle<JSTaggedValue> proto)
{
    JSHandle<JSHClass> stackClass = factory->NewEcmaHClass(JSAPIStack::SIZE, JSType::JS_API_STACK, proto);
    JSHandle<JSAPIStack> jsStack = JSHandle<JSAPIStack>::Cast(factory->NewJSObjectWithInit(stackClass));
    jsStack->SetTop(0);
    return jsStack;
}

static JSHandle<JSRegExp> NewJSRegExp(JSThread *thread, ObjectFactory *factory, JSHandle<JSTaggedValue> proto)
{
    JSHandle<JSHClass> jSRegExpClass =
        factory->NewEcmaHClass(JSRegExp::SIZE, JSType::JS_REG_EXP, proto);
    JSHandle<JSRegExp> jSRegExp = JSHandle<JSRegExp>::Cast(factory->NewJSObject(jSRegExpClass));
    jSRegExp->SetByteCodeBuffer(thread, JSTaggedValue::Undefined());
    jSRegExp->SetOriginalSource(thread, JSTaggedValue::Undefined());
    jSRegExp->SetGroupName(thread, JSTaggedValue::Undefined());
    jSRegExp->SetOriginalFlags(thread, JSTaggedValue(0));
    jSRegExp->SetLength(0);
    return jSRegExp;
}

static JSHandle<JSAPILightWeightMap> NewJSAPILightWeightMap(JSThread *thread, ObjectFactory *factory)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> lwmapClass =
        factory->NewEcmaHClass(JSAPILightWeightMap::SIZE, JSType::JS_API_LIGHT_WEIGHT_MAP, proto);
    JSHandle<JSAPILightWeightMap> jSAPILightWeightMap =
        JSHandle<JSAPILightWeightMap>::Cast(factory->NewJSObjectWithInit(lwmapClass));
    JSHandle<JSTaggedValue> hashArray =
        JSHandle<JSTaggedValue>(factory->NewTaggedArray(JSAPILightWeightMap::DEFAULT_CAPACITY_LENGTH));
    JSHandle<JSTaggedValue> keyArray =
        JSHandle<JSTaggedValue>(factory->NewTaggedArray(JSAPILightWeightMap::DEFAULT_CAPACITY_LENGTH));
    JSHandle<JSTaggedValue> valueArray =
        JSHandle<JSTaggedValue>(factory->NewTaggedArray(JSAPILightWeightMap::DEFAULT_CAPACITY_LENGTH));
    jSAPILightWeightMap->SetHashes(thread, hashArray);
    jSAPILightWeightMap->SetKeys(thread, keyArray);
    jSAPILightWeightMap->SetValues(thread, valueArray);
    jSAPILightWeightMap->SetLength(0);
    return jSAPILightWeightMap;
}

static JSHandle<JSAPILightWeightSet> NewJSAPILightWeightSet(JSThread *thread, ObjectFactory *factory)
{
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> proto = globalEnv->GetObjectFunctionPrototype();
    JSHandle<JSHClass> setClass =
        factory->NewEcmaHClass(JSAPILightWeightSet::SIZE, JSType::JS_API_LIGHT_WEIGHT_SET, proto);
    JSHandle<JSAPILightWeightSet> jSAPILightWeightSet =
        JSHandle<JSAPILightWeightSet>::Cast(factory->NewJSObjectWithInit(setClass));
    JSHandle<TaggedArray> hashes =
        JSAPILightWeightSet::CreateSlot(thread, JSAPILightWeightSet::DEFAULT_CAPACITY_LENGTH);
    JSHandle<TaggedArray> values =
        JSAPILightWeightSet::CreateSlot(thread, JSAPILightWeightSet::DEFAULT_CAPACITY_LENGTH);
    jSAPILightWeightSet->SetHashes(thread, hashes);
    jSAPILightWeightSet->SetValues(thread, values);
    jSAPILightWeightSet->SetLength(0);
    return jSAPILightWeightSet;
}

static JSHandle<JSAPIQueue> NewJSAPIQueue(JSThread *thread, ObjectFactory *factory, JSHandle<JSTaggedValue> proto)
{
    JSHandle<JSHClass> queueClass = factory->NewEcmaHClass(JSAPIQueue::SIZE, JSType::JS_API_QUEUE, proto);
    JSHandle<JSAPIQueue> jsQueue = JSHandle<JSAPIQueue>::Cast(factory->NewJSObjectWithInit(queueClass));
    JSHandle<TaggedArray> newElements = factory->NewTaggedArray(JSAPIQueue::DEFAULT_CAPACITY_LENGTH);
    jsQueue->SetLength(thread, JSTaggedValue(0));
    jsQueue->SetFront(0);
    jsQueue->SetTail(0);
    jsQueue->SetElements(thread, newElements);
    return jsQueue;
}

static JSHandle<JSAPIDeque> NewJSAPIDeque(JSThread *thread, ObjectFactory *factory, JSHandle<JSTaggedValue> proto)
{
    JSHandle<JSHClass> dequeClass = factory->NewEcmaHClass(JSAPIDeque::SIZE, JSType::JS_API_DEQUE, proto);
    JSHandle<JSAPIDeque> jsDeque = JSHandle<JSAPIDeque>::Cast(factory->NewJSObjectWithInit(dequeClass));
    JSHandle<TaggedArray> newElements = factory->NewTaggedArray(JSAPIDeque::DEFAULT_CAPACITY_LENGTH);
    jsDeque->SetFirst(0);
    jsDeque->SetLast(0);
    jsDeque->SetElements(thread, newElements);
    return jsDeque;
}

static JSHandle<JSAPIVector> NewJSAPIVector(ObjectFactory *factory, JSHandle<JSTaggedValue> proto)
{
    JSHandle<JSHClass> vectorClass = factory->NewEcmaHClass(JSAPIVector::SIZE, JSType::JS_API_VECTOR, proto);
    JSHandle<JSAPIVector> jsVector = JSHandle<JSAPIVector>::Cast(factory->NewJSObjectWithInit(vectorClass));
    jsVector->SetLength(0);
    return jsVector;
}

HWTEST_F_L0(EcmaDumpTest, HeapProfileDump)
{
    [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);
    auto factory = thread->GetEcmaVM()->GetFactory();
    auto globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = const_cast<GlobalEnvConstants *>(thread->GlobalConstants());
    JSHandle<JSTaggedValue> proto = globalEnv->GetFunctionPrototype();
    std::vector<Reference> snapshotVector;
    std::ostringstream os;

#define DUMP_FOR_HANDLE(dumpHandle)                                                     \
    do {                                                                                \
        JSTaggedValue dumpValue = (dumpHandle).GetTaggedValue();                         \
        dumpValue.Dump(os);                                                             \
        dumpValue.DumpForSnapshot(snapshotVector);                                      \
        /* Testing runtime stubs: */                                                    \
        JSTaggedType dumpRawData = dumpValue.GetRawData();                              \
        uintptr_t hintStr = reinterpret_cast<uintptr_t>("Testing with: " #dumpHandle);  \
        RuntimeStubs::Dump(dumpRawData);                                                \
        RuntimeStubs::DebugDump(dumpRawData);                                           \
        RuntimeStubs::DumpWithHint(hintStr, dumpRawData);                               \
        RuntimeStubs::DebugDumpWithHint(hintStr, dumpRawData);                          \
    } while (false)

#if defined(PANDA_TARGET_AMD64) || defined(PANDA_TARGET_ARM64)
    static const unsigned int machineCodeSize = 39;
#else
    static const unsigned int machineCodeSize = 23;
#endif

#define NEW_OBJECT_AND_DUMP(ClassName, TypeName)                                                \
    do {                                                                                        \
        JSHandle<JSHClass> class##ClassName =                                                   \
            factory->NewEcmaHClass(ClassName::SIZE, JSType::TypeName, proto);                   \
        JSHandle<JSObject> object##ClassName = factory->NewJSObjectWithInit(class##ClassName);  \
        DUMP_FOR_HANDLE(object##ClassName);                                                     \
    } while (false)

    for (JSType type = JSType::JS_OBJECT; type <= JSType::TYPE_LAST; type = JSType(static_cast<int>(type) + 1)) {
        switch (type) {
            case JSType::JS_ERROR:
            case JSType::JS_EVAL_ERROR:
            case JSType::JS_RANGE_ERROR:
            case JSType::JS_TYPE_ERROR:
            case JSType::JS_AGGREGATE_ERROR:
            case JSType::JS_REFERENCE_ERROR:
            case JSType::JS_URI_ERROR:
            case JSType::JS_ARGUMENTS:
            case JSType::JS_SYNTAX_ERROR:
            case JSType::JS_OOM_ERROR:
            case JSType::JS_TERMINATION_ERROR:
            case JSType::JS_OBJECT:
            case JSType::JS_SHARED_OBJECT: {
                CHECK_DUMP_FIELDS(ECMAObject::SIZE, JSObject::SIZE, 2U);
                JSHandle<JSObject> jsObj = NewJSObject(thread, factory, globalEnv);
                DUMP_FOR_HANDLE(jsObj);
                break;
            }
            case JSType::JS_REALM: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSRealm::SIZE, 2U);
                JSHandle<JSRealm> jsRealm = factory->NewJSRealm();
                DUMP_FOR_HANDLE(jsRealm);
                break;
            }
            case JSType::METHOD: {
#ifdef PANDA_TARGET_64
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), Method::SIZE, 6U);
#else
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), Method::SIZE, 5U);
#endif
                break;
            }
            case JSType::JS_FUNCTION_BASE: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSFunctionBase::SIZE, 3U);
                break;
            }
            case JSType::JS_FUNCTION:
            case JSType::JS_SHARED_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunctionBase::SIZE, JSFunction::SIZE, 9U);
                JSHandle<JSTaggedValue> jsFunc = globalEnv->GetFunctionFunction();
                DUMP_FOR_HANDLE(jsFunc);
                break;
            }
            case JSType::JS_PROXY_REVOC_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSProxyRevocFunction::SIZE, 1U);
                JSHandle<JSHClass> proxyRevocClass =
                    JSHandle<JSHClass>::Cast(globalEnv->GetProxyRevocFunctionClass());
                JSHandle<JSObject> proxyRevocFunc = factory->NewJSObjectWithInit(proxyRevocClass);
                DUMP_FOR_HANDLE(proxyRevocFunc);
                break;
            }
            case JSType::JS_PROMISE_REACTIONS_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSPromiseReactionsFunction::SIZE, 2U);
                JSHandle<JSHClass> promiseReactClass =
                    JSHandle<JSHClass>::Cast(globalEnv->GetPromiseReactionFunctionClass());
                JSHandle<JSObject> promiseReactFunc = factory->NewJSObjectWithInit(promiseReactClass);
                DUMP_FOR_HANDLE(promiseReactFunc);
                break;
            }
            case JSType::JS_PROMISE_EXECUTOR_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSPromiseExecutorFunction::SIZE, 1U);
                JSHandle<JSHClass> promiseExeClass =
                    JSHandle<JSHClass>::Cast(globalEnv->GetPromiseExecutorFunctionClass());
                JSHandle<JSObject> promiseExeFunc = factory->NewJSObjectWithInit(promiseExeClass);
                DUMP_FOR_HANDLE(promiseExeFunc);
                break;
            }
            case JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSAsyncModuleFulfilledFunction::SIZE, 0U);
                JSHandle<JSHClass> moduleFulfilledClass =
                    JSHandle<JSHClass>::Cast(globalEnv->GetAsyncModuleFulfilledFunctionClass());
                JSHandle<JSObject> moduleFulfilledFunc = factory->NewJSObjectWithInit(moduleFulfilledClass);
                DUMP_FOR_HANDLE(moduleFulfilledFunc);
                break;
            }
            case JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSAsyncModuleRejectedFunction::SIZE, 0U);
                JSHandle<JSHClass> moduleRejectedClass =
                    JSHandle<JSHClass>::Cast(globalEnv->GetAsyncModuleRejectedFunctionClass());
                JSHandle<JSObject> moduleRejectedFunc = factory->NewJSObjectWithInit(moduleRejectedClass);
                DUMP_FOR_HANDLE(moduleRejectedFunc);
                break;
            }
            case JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSPromiseAllResolveElementFunction::SIZE, 5U);
                JSHandle<JSHClass> promiseAllClass =
                    JSHandle<JSHClass>::Cast(globalEnv->GetPromiseAllResolveElementFunctionClass());
                JSHandle<JSObject> promiseAllFunc = factory->NewJSObjectWithInit(promiseAllClass);
                DUMP_FOR_HANDLE(promiseAllFunc);
                break;
            }
            case JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSPromiseAnyRejectElementFunction::SIZE, 5U);
                JSHandle<JSHClass> promiseAnyClass =
                    JSHandle<JSHClass>::Cast(globalEnv->GetPromiseAnyRejectElementFunctionClass());
                JSHandle<JSObject> promiseAnyFunc = factory->NewJSObjectWithInit(promiseAnyClass);
                DUMP_FOR_HANDLE(promiseAnyFunc);
                break;
            }
            case JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSPromiseAllSettledElementFunction::SIZE, 5U);
                JSHandle<JSHClass> promiseAllSettledClass =
                    JSHandle<JSHClass>::Cast(globalEnv->GetPromiseAllSettledElementFunctionClass());
                JSHandle<JSObject> promiseAllSettledFunc = factory->NewJSObjectWithInit(promiseAllSettledClass);
                DUMP_FOR_HANDLE(promiseAllSettledFunc);
                break;
            }
            case JSType::JS_PROMISE_FINALLY_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSPromiseFinallyFunction::SIZE, 2U);
                JSHandle<JSHClass> promiseFinallyClass =
                    JSHandle<JSHClass>::Cast(globalEnv->GetPromiseFinallyFunctionClass());
                JSHandle<JSObject> promiseFinallyFunc = factory->NewJSObjectWithInit(promiseFinallyClass);
                DUMP_FOR_HANDLE(promiseFinallyFunc);
                break;
            }
            case JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSPromiseValueThunkOrThrowerFunction::SIZE, 1U);
                JSHandle<JSHClass> promiseValueClass =
                    JSHandle<JSHClass>::Cast(globalEnv->GetPromiseValueThunkOrThrowerFunctionClass());
                JSHandle<JSObject> promiseValueFunc = factory->NewJSObjectWithInit(promiseValueClass);
                DUMP_FOR_HANDLE(promiseValueFunc);
                break;
            }
            case JSType::JS_ASYNC_GENERATOR_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSAsyncGeneratorFunction::SIZE, 0U);
                break;
            }
            case JSType::JS_GENERATOR_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSGeneratorFunction::SIZE, 0U);
                break;
            }
            case JSType::JS_ASYNC_FUNCTION:
            case JSType::JS_SHARED_ASYNC_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSAsyncFunction::SIZE, 0U);
                break;
            }
            case JSType::JS_INTL_BOUND_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSIntlBoundFunction::SIZE, 3U);
                JSHandle<JSIntlBoundFunction> intlBoundFunc = factory->NewJSIntlBoundFunction(
                    MethodIndex::BUILTINS_NUMBER_FORMAT_NUMBER_FORMAT_INTERNAL_FORMAT_NUMBER);
                DUMP_FOR_HANDLE(intlBoundFunc);
                break;
            }
            case JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSAsyncAwaitStatusFunction::SIZE, 1U);
                JSHandle<JSAsyncAwaitStatusFunction> asyncAwaitFunc = factory->NewJSAsyncAwaitStatusFunction(
                    MethodIndex::BUILTINS_PROMISE_HANDLER_ASYNC_AWAIT_FULFILLED);
                DUMP_FOR_HANDLE(asyncAwaitFunc);
                break;
            }
            case JSType::JS_BOUND_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunctionBase::SIZE, JSBoundFunction::SIZE, 3U);
                NEW_OBJECT_AND_DUMP(JSBoundFunction, JS_BOUND_FUNCTION);
                break;
            }
            case JSType::JS_REG_EXP: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSRegExp::SIZE, 5U);
                NEW_OBJECT_AND_DUMP(JSRegExp, JS_REG_EXP);
                break;
            }
            case JSType::JS_SET: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSSet::SIZE, 1U);
                JSHandle<JSSet> jsSet = NewJSSet(thread, factory, proto);
                DUMP_FOR_HANDLE(jsSet);
                break;
            }
            case JSType::JS_SHARED_SET: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSSharedSet::SIZE, 2U);
                JSHandle<JSSharedSet> jsSet = NewJSSharedSet(thread, factory);
                DUMP_FOR_HANDLE(jsSet);
                break;
            }
            case JSType::JS_MAP: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSMap::SIZE, 1U);
                JSHandle<JSMap> jsMap = NewJSMap(thread, factory, proto);
                DUMP_FOR_HANDLE(jsMap);
                break;
            }
            case JSType::JS_SHARED_MAP: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSSharedMap::SIZE, 2U);
                JSHandle<JSSharedMap> jsMap = NewJSSharedMap(thread, factory);
                DUMP_FOR_HANDLE(jsMap);
                break;
            }
            case JSType::JS_WEAK_MAP: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSWeakMap::SIZE, 1U);
                JSHandle<JSHClass> weakMapClass = factory->NewEcmaHClass(JSWeakMap::SIZE, JSType::JS_WEAK_MAP, proto);
                JSHandle<JSWeakMap> jsWeakMap = JSHandle<JSWeakMap>::Cast(factory->NewJSObjectWithInit(weakMapClass));
                JSHandle<LinkedHashMap> weakLinkedMap(LinkedHashMap::Create(thread));
                jsWeakMap->SetLinkedMap(thread, weakLinkedMap);
                DUMP_FOR_HANDLE(jsWeakMap);
                break;
            }
            case JSType::JS_WEAK_SET: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSWeakSet::SIZE, 1U);
                JSHandle<JSHClass> weakSetClass = factory->NewEcmaHClass(JSWeakSet::SIZE, JSType::JS_WEAK_SET, proto);
                JSHandle<JSWeakSet> jsWeakSet = JSHandle<JSWeakSet>::Cast(factory->NewJSObjectWithInit(weakSetClass));
                JSHandle<LinkedHashSet> weakLinkedSet(LinkedHashSet::Create(thread));
                jsWeakSet->SetLinkedSet(thread, weakLinkedSet);
                DUMP_FOR_HANDLE(jsWeakSet);
                break;
            }
            case JSType::JS_WEAK_REF: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSWeakRef::SIZE, 1U);
                JSHandle<JSHClass> weakRefClass = factory->NewEcmaHClass(JSWeakRef::SIZE, JSType::JS_WEAK_REF, proto);
                JSHandle<JSWeakRef> jsWeakRef = JSHandle<JSWeakRef>::Cast(factory->NewJSObjectWithInit(weakRefClass));
                jsWeakRef->SetWeakObject(thread, JSTaggedValue::Undefined());
                DUMP_FOR_HANDLE(jsWeakRef);
                break;
            }
            case JSType::JS_FINALIZATION_REGISTRY: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSFinalizationRegistry::SIZE, 5U);
                JSHandle<JSHClass> finalizationRegistryClass =
                    factory->NewEcmaHClass(JSFinalizationRegistry::SIZE, JSType::JS_FINALIZATION_REGISTRY, proto);
                JSHandle<JSFinalizationRegistry> jsFinalizationRegistry =
                    JSHandle<JSFinalizationRegistry>::Cast(factory->NewJSObjectWithInit(finalizationRegistryClass));
                JSHandle<LinkedHashMap> weakLinkedMap(LinkedHashMap::Create(thread));
                jsFinalizationRegistry->SetMaybeUnregister(thread, weakLinkedMap);
                DUMP_FOR_HANDLE(jsFinalizationRegistry);
                break;
            }
            case JSType::CELL_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, CellRecord::SIZE, 2U);
                JSHandle<CellRecord> cellRecord = factory->NewCellRecord();
                DUMP_FOR_HANDLE(cellRecord);
                break;
            }
            case JSType::JS_DATE: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSDate::SIZE, 2U);
                JSHandle<JSHClass> dateClass = factory->NewEcmaHClass(JSDate::SIZE, JSType::JS_DATE, proto);
                JSHandle<JSDate> date = JSHandle<JSDate>::Cast(factory->NewJSObjectWithInit(dateClass));
                date->SetTimeValue(thread, JSTaggedValue(0.0));
                date->SetLocalOffset(thread, JSTaggedValue(0.0));
                DUMP_FOR_HANDLE(date);
                break;
            }
            case JSType::JS_FORIN_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSForInIterator::SIZE, 4U);
                JSHandle<JSTaggedValue> array(thread, factory->NewJSArray().GetTaggedValue());
                JSHandle<JSTaggedValue> keys(thread, factory->EmptyArray().GetTaggedValue());
                JSHandle<JSTaggedValue> hclass(thread, JSTaggedValue::Undefined());
                JSHandle<JSForInIterator> forInIter = factory->NewJSForinIterator(array, keys, hclass);
                DUMP_FOR_HANDLE(forInIter);
                break;
            }
            case JSType::JS_MAP_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSMapIterator::SIZE, 2U);
                JSHandle<JSMapIterator> jsMapIter =
                    factory->NewJSMapIterator(NewJSMap(thread, factory, proto), IterationKind::KEY);
                DUMP_FOR_HANDLE(jsMapIter);
                break;
            }
            case JSType::JS_SHARED_MAP_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSSharedMapIterator::SIZE, 2U);
                JSHandle<JSSharedMapIterator> jsMapIter =
                    factory->NewJSMapIterator(NewJSSharedMap(thread, factory), IterationKind::KEY);
                DUMP_FOR_HANDLE(jsMapIter);
                break;
            }
            case JSType::JS_SET_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSSetIterator::SIZE, 2U);
                JSHandle<JSSetIterator> jsSetIter =
                    factory->NewJSSetIterator(NewJSSet(thread, factory, proto), IterationKind::KEY);
                DUMP_FOR_HANDLE(jsSetIter);
                break;
            }
            case JSType::JS_SHARED_SET_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSSharedSetIterator::SIZE, 2U);
                JSHandle<JSSharedSetIterator> jsSetIter =
                    factory->NewJSSetIterator(NewJSSharedSet(thread, factory), IterationKind::KEY);
                DUMP_FOR_HANDLE(jsSetIter);
                break;
            }
            case JSType::JS_REG_EXP_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSRegExpIterator::SIZE, 3U);
                JSHandle<EcmaString> emptyString(thread->GlobalConstants()->GetHandledEmptyString());
                JSHandle<JSTaggedValue> jsRegExp(NewJSRegExp(thread, factory, proto));
                JSHandle<JSRegExpIterator> jsRegExpIter =
                    factory->NewJSRegExpIterator(jsRegExp, emptyString, false, false);
                DUMP_FOR_HANDLE(jsRegExpIter);
                break;
            }
            case JSType::JS_ARRAY_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSArrayIterator::SIZE, 2U);
                JSHandle<JSArrayIterator> arrayIter =
                    factory->NewJSArrayIterator(JSHandle<JSObject>::Cast(factory->NewJSArray()), IterationKind::KEY);
                DUMP_FOR_HANDLE(arrayIter);
                break;
            }
            case JSType::JS_SHARED_ARRAY_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSArrayIterator::SIZE, 2U);
                JSHandle<JSSharedArrayIterator> arrayIter = factory->NewJSSharedArrayIterator(
                    JSHandle<JSObject>::Cast(factory->NewJSSArray()), IterationKind::KEY);
                DUMP_FOR_HANDLE(arrayIter);
                break;
            }
            case JSType::JS_STRING_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSStringIterator::SIZE, 2U);
                JSHandle<JSTaggedValue> stringIter = globalEnv->GetStringIterator();
                DUMP_FOR_HANDLE(stringIter);
                break;
            }
            case JSType::JS_INTL: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSIntl::SIZE, 1U);
                NEW_OBJECT_AND_DUMP(JSIntl, JS_INTL);
                break;
            }
            case JSType::JS_LOCALE: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSLocale::SIZE, 1U);
                NEW_OBJECT_AND_DUMP(JSLocale, JS_LOCALE);
                break;
            }
            case JSType::JS_DATE_TIME_FORMAT: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSDateTimeFormat::SIZE, 9U);
                NEW_OBJECT_AND_DUMP(JSDateTimeFormat, JS_DATE_TIME_FORMAT);
                break;
            }
            case JSType::JS_RELATIVE_TIME_FORMAT: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSRelativeTimeFormat::SIZE, 4U);
                NEW_OBJECT_AND_DUMP(JSRelativeTimeFormat, JS_RELATIVE_TIME_FORMAT);
                break;
            }
            case JSType::JS_NUMBER_FORMAT: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSNumberFormat::SIZE, 13U);
                NEW_OBJECT_AND_DUMP(JSNumberFormat, JS_NUMBER_FORMAT);
                break;
            }
            case JSType::JS_COLLATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSCollator::SIZE, 5U);
                NEW_OBJECT_AND_DUMP(JSCollator, JS_COLLATOR);
                break;
            }
            case JSType::JS_PLURAL_RULES: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSPluralRules::SIZE, 9U);
                NEW_OBJECT_AND_DUMP(JSPluralRules, JS_PLURAL_RULES);
                break;
            }
            case JSType::JS_DISPLAYNAMES: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSDisplayNames::SIZE, 3U);
                NEW_OBJECT_AND_DUMP(JSDisplayNames, JS_DISPLAYNAMES);
                break;
            }
            case JSType::JS_SEGMENTER: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSSegmenter::SIZE, 3U);
                NEW_OBJECT_AND_DUMP(JSSegmenter, JS_SEGMENTER);
                break;
            }
            case JSType::JS_SEGMENTS: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSSegments::SIZE, 4U);
                NEW_OBJECT_AND_DUMP(JSSegments, JS_SEGMENTS);
                break;
            }
            case JSType::JS_SEGMENT_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSSegmentIterator::SIZE, 4U);
                NEW_OBJECT_AND_DUMP(JSSegmentIterator, JS_SEGMENTS);
                break;
            }
            case JSType::JS_LIST_FORMAT: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSListFormat::SIZE, 3U);
                NEW_OBJECT_AND_DUMP(JSListFormat, JS_LIST_FORMAT);
                break;
            }
            case JSType::JS_SHARED_ARRAY_BUFFER:
            case JSType::JS_ARRAY_BUFFER: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSArrayBuffer::SIZE, 2U);
                NEW_OBJECT_AND_DUMP(JSArrayBuffer, JS_ARRAY_BUFFER);
                break;
            }
            case JSType::JS_SENDABLE_ARRAY_BUFFER: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSSendableArrayBuffer::SIZE, 2U);
                NEW_OBJECT_AND_DUMP(JSSendableArrayBuffer, JS_SENDABLE_ARRAY_BUFFER);
                break;
            }
            case JSType::JS_PROMISE: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSPromise::SIZE, 4U);
                NEW_OBJECT_AND_DUMP(JSPromise, JS_PROMISE);
                break;
            }
            case JSType::JS_DATA_VIEW: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSDataView::SIZE, 3U);
                NEW_OBJECT_AND_DUMP(JSDataView, JS_DATA_VIEW);
                break;
            }
            case JSType::JS_GENERATOR_OBJECT: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSGeneratorObject::SIZE, 4U);
                NEW_OBJECT_AND_DUMP(JSGeneratorObject, JS_GENERATOR_OBJECT);
                break;
            }
            case JSType::JS_ASYNC_GENERATOR_OBJECT: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAsyncGeneratorObject::SIZE, 5U);
                NEW_OBJECT_AND_DUMP(JSAsyncGeneratorObject, JS_ASYNC_GENERATOR_OBJECT);
                break;
            }
            case JSType::JS_ASYNC_FUNC_OBJECT: {
                CHECK_DUMP_FIELDS(JSGeneratorObject::SIZE, JSAsyncFuncObject::SIZE, 1U);
                JSHandle<JSAsyncFuncObject> asyncFuncObject = factory->NewJSAsyncFuncObject();
                DUMP_FOR_HANDLE(asyncFuncObject);
                break;
            }
            case JSType::JS_ARRAY: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSArray::SIZE, 2U);
                JSHandle<JSArray> jsArray = factory->NewJSArray();
                DUMP_FOR_HANDLE(jsArray);
                break;
            }
            case JSType::JS_SHARED_ARRAY: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSArray::SIZE, 2U);
                JSHandle<JSSharedArray> jsArray = factory->NewJSSArray();
                DUMP_FOR_HANDLE(jsArray);
                break;
            }
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
            case JSType::JS_BIGUINT64_ARRAY: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSTypedArray::SIZE, 4U);
                NEW_OBJECT_AND_DUMP(JSTypedArray, JS_TYPED_ARRAY);
                break;
            }
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
            case JSType::JS_SHARED_BIGUINT64_ARRAY: {
                // Fixme(Gymee) Add test later
                break;
            }
            case JSType::JS_PRIMITIVE_REF: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSPrimitiveRef::SIZE, 1U);
                NEW_OBJECT_AND_DUMP(JSPrimitiveRef, JS_PRIMITIVE_REF);
                break;
            }
            case JSType::JS_GLOBAL_OBJECT: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSGlobalObject::SIZE, 0U);
                JSHandle<JSTaggedValue> globalObject = globalEnv->GetJSGlobalObject();
                DUMP_FOR_HANDLE(globalObject);
                break;
            }
            case JSType::JS_PROXY: {
                CHECK_DUMP_FIELDS(ECMAObject::SIZE, JSProxy::SIZE, 5U);
                JSHandle<JSTaggedValue> emptyObj(thread, NewJSObject(thread, factory, globalEnv).GetTaggedValue());
                JSHandle<JSProxy> proxy = factory->NewJSProxy(emptyObj, emptyObj);
                DUMP_FOR_HANDLE(proxy);
                break;
            }
            case JSType::HCLASS: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), JSHClass::SIZE, 9U);
                JSHandle<JSHClass> hclass = factory->NewEcmaHClass(JSHClass::SIZE, JSType::HCLASS, proto);
                DUMP_FOR_HANDLE(hclass);
                break;
            }
            case JSType::LINE_STRING:
            case JSType::CONSTANT_STRING:
            case JSType::TREE_STRING:
            case JSType::SLICED_STRING: {
                DUMP_FOR_HANDLE(globalEnv->GetObjectFunction());
                break;
            }
            case JSType::BIGINT: {
                DUMP_FOR_HANDLE(globalEnv->GetBigIntFunction());
                break;
            }
            case JSType::PROFILE_TYPE_INFO_CELL_0:
            case JSType::PROFILE_TYPE_INFO_CELL_1:
            case JSType::PROFILE_TYPE_INFO_CELL_N: {
                JSHandle<JSTaggedValue> handleUndefined(thread, JSTaggedValue::Undefined());
                JSHandle<ProfileTypeInfoCell> profileTypeInfoCell = factory->NewProfileTypeInfoCell(handleUndefined);
                DUMP_FOR_HANDLE(profileTypeInfoCell);
                break;
            }
            case JSType::FUNCTION_TEMPLATE: {
                auto method = JSFunction::Cast(globalEnv->GetTaggedObjectFunction())->GetMethod();
                JSHandle<Method> methodHandle(thread, method);
                JSHandle<JSTaggedValue> handleUndefined(thread, JSTaggedValue::Undefined());
                JSHandle<FunctionTemplate> funcTemp = factory->NewFunctionTemplate(methodHandle, handleUndefined, 0);
                DUMP_FOR_HANDLE(funcTemp);
                break;
            }
            case JSType::TAGGED_ARRAY:
            case JSType::VTABLE:
            case JSType::LEXICAL_ENV:
            case JSType::SENDABLE_ENV:
            case JSType::AOT_LITERAL_INFO: {
                JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(4);
                DUMP_FOR_HANDLE(taggedArray);
                break;
            }
            case JSType::CONSTANT_POOL: {
                JSHandle<ConstantPool> constantPool = factory->NewConstantPool(4);
                DUMP_FOR_HANDLE(constantPool);
                break;
            }
            case JSType::PROFILE_TYPE_INFO: {
                JSHandle<ProfileTypeInfo> info = factory->NewProfileTypeInfo(4);
                DUMP_FOR_HANDLE(info);
                break;
            }
            case JSType::EXTRA_PROFILE_TYPE_INFO: {
                JSHandle<ExtraProfileTypeInfo> info = factory->NewExtraProfileTypeInfo();
                DUMP_FOR_HANDLE(info);
                break;
            }
            case JSType::TAGGED_DICTIONARY: {
                JSHandle<TaggedArray> dict = factory->NewDictionaryArray(4);
                DUMP_FOR_HANDLE(dict);
                break;
            }
            case JSType::BYTE_ARRAY: {
                JSHandle<ByteArray> byteArray = factory->NewByteArray(4, 8);
                DUMP_FOR_HANDLE(byteArray);
                break;
            }
            case JSType::COW_TAGGED_ARRAY: {
                JSHandle<COWTaggedArray> dict = factory->NewCOWTaggedArray(4);
                DUMP_FOR_HANDLE(dict);
                break;
            }
            case JSType::MUTANT_TAGGED_ARRAY: {
                JSHandle<MutantTaggedArray> array = factory->NewMutantTaggedArray(4);
                DUMP_FOR_HANDLE(array);
                break;
            }
            case JSType::COW_MUTANT_TAGGED_ARRAY: {
                JSHandle<COWMutantTaggedArray> array = factory->NewCOWMutantTaggedArray(4);
                DUMP_FOR_HANDLE(array);
                break;
            }
            case JSType::GLOBAL_ENV: {
                DUMP_FOR_HANDLE(globalEnv);
                break;
            }
            case JSType::ACCESSOR_DATA:
            case JSType::INTERNAL_ACCESSOR: {
                CHECK_DUMP_FIELDS(Record::SIZE, AccessorData::SIZE, 2U);
                JSHandle<AccessorData> accessor = factory->NewAccessorData();
                DUMP_FOR_HANDLE(accessor);
                break;
            }
            case JSType::SYMBOL: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), JSSymbol::SIZE, 3U);
                JSHandle<JSSymbol> symbol = factory->NewJSSymbol();
                DUMP_FOR_HANDLE(symbol);
                break;
            }
            case JSType::JS_GENERATOR_CONTEXT: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), GeneratorContext::SIZE, 7U);
                JSHandle<GeneratorContext> genContext = factory->NewGeneratorContext();
                DUMP_FOR_HANDLE(genContext);
                break;
            }
            case JSType::PROTOTYPE_HANDLER: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), PrototypeHandler::SIZE, 5U);
                JSHandle<PrototypeHandler> protoHandler = factory->NewPrototypeHandler();
                DUMP_FOR_HANDLE(protoHandler);
                break;
            }
            case JSType::TRANSITION_HANDLER: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), TransitionHandler::SIZE, 2U);
                JSHandle<TransitionHandler> transitionHandler = factory->NewTransitionHandler();
                DUMP_FOR_HANDLE(transitionHandler);
                break;
            }
            case JSType::TRANS_WITH_PROTO_HANDLER: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), TransWithProtoHandler::SIZE, 3U);
                JSHandle<TransWithProtoHandler> transWithProtoHandler = factory->NewTransWithProtoHandler();
                DUMP_FOR_HANDLE(transWithProtoHandler);
                break;
            }
            case JSType::STORE_TS_HANDLER: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), StoreTSHandler::SIZE, 3U);
                JSHandle<StoreTSHandler> storeTSHandler = factory->NewStoreTSHandler();
                DUMP_FOR_HANDLE(storeTSHandler);
                break;
            }
            case JSType::PROPERTY_BOX: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), PropertyBox::SIZE, 1U);
                JSHandle<PropertyBox> PropertyBox = factory->NewPropertyBox(globalConst->GetHandledEmptyArray());
                DUMP_FOR_HANDLE(PropertyBox);
                break;
            }
            case JSType::PROTO_CHANGE_MARKER: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), ProtoChangeMarker::SIZE, 1U);
                JSHandle<ProtoChangeMarker> protoMaker = factory->NewProtoChangeMarker();
                DUMP_FOR_HANDLE(protoMaker);
                break;
            }
            case JSType::MARKER_CELL: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), MarkerCell::SIZE, 1U);
                JSHandle<MarkerCell> markerCell = factory->NewMarkerCell();
                DUMP_FOR_HANDLE(markerCell);
                break;
            }
            case JSType::TRACK_INFO: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), TrackInfo::SIZE, 3U);
                break;
            }
            case JSType::PROTOTYPE_INFO: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), ProtoChangeDetails::SIZE, 2U);
                JSHandle<ProtoChangeDetails> protoDetails = factory->NewProtoChangeDetails();
                DUMP_FOR_HANDLE(protoDetails);
                break;
            }
            case JSType::TEMPLATE_MAP: {
                JSHandle<JSTaggedValue> templateMap = globalEnv->GetTemplateMap();
                DUMP_FOR_HANDLE(templateMap);
                break;
            }
            case JSType::PROGRAM: {
                CHECK_DUMP_FIELDS(ECMAObject::SIZE, Program::SIZE, 1U);
                JSHandle<Program> program = factory->NewProgram();
                DUMP_FOR_HANDLE(program);
                break;
            }
            case JSType::PROMISE_CAPABILITY: {
                CHECK_DUMP_FIELDS(Record::SIZE, PromiseCapability::SIZE, 3U);
                JSHandle<PromiseCapability> promiseCapa = factory->NewPromiseCapability();
                DUMP_FOR_HANDLE(promiseCapa);
                break;
            }
            case JSType::PROMISE_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, PromiseRecord::SIZE, 1U);
                JSHandle<PromiseRecord> promiseRecord = factory->NewPromiseRecord();
                DUMP_FOR_HANDLE(promiseRecord);
                break;
            }
            case JSType::RESOLVING_FUNCTIONS_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, ResolvingFunctionsRecord::SIZE, 2U);
                JSHandle<ResolvingFunctionsRecord> ResolvingFunc = factory->NewResolvingFunctionsRecord();
                DUMP_FOR_HANDLE(ResolvingFunc);
                break;
            }
            case JSType::ASYNC_GENERATOR_REQUEST: {
                CHECK_DUMP_FIELDS(Record::SIZE, AsyncGeneratorRequest::SIZE, 2U);
                JSHandle<AsyncGeneratorRequest> asyncGeneratorRequest = factory->NewAsyncGeneratorRequest();
                DUMP_FOR_HANDLE(asyncGeneratorRequest);
                break;
            }
            case JSType::ASYNC_ITERATOR_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, AsyncIteratorRecord::SIZE, 3U);
                JSHandle<JSTaggedValue> emptyObj(thread, NewJSObject(thread, factory, globalEnv).GetTaggedValue());
                JSHandle<JSTaggedValue> emptyMethod(thread, NewJSObject(thread, factory, globalEnv).GetTaggedValue());
                JSHandle<AsyncIteratorRecord> asyncIteratorRecord =
                    factory->NewAsyncIteratorRecord(emptyObj, emptyMethod, false);
                DUMP_FOR_HANDLE(asyncIteratorRecord);
                break;
            }
            case JSType::JS_ASYNC_FROM_SYNC_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAsyncFromSyncIterator::SIZE, 1U);
                NEW_OBJECT_AND_DUMP(JSAsyncFromSyncIterator, JS_ASYNC_FROM_SYNC_ITERATOR);
                break;
            }
            case JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSAsyncFromSyncIterUnwarpFunction::SIZE, 1U);
                break;
            }
            case JSType::PROMISE_REACTIONS: {
                CHECK_DUMP_FIELDS(Record::SIZE, PromiseReaction::SIZE, 3U);
                JSHandle<PromiseReaction> promiseReact = factory->NewPromiseReaction();
                DUMP_FOR_HANDLE(promiseReact);
                break;
            }
            case JSType::PROMISE_ITERATOR_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, PromiseIteratorRecord::SIZE, 2U);
                JSHandle<JSTaggedValue> emptyObj(thread, NewJSObject(thread, factory, globalEnv).GetTaggedValue());
                JSHandle<PromiseIteratorRecord> promiseIter = factory->NewPromiseIteratorRecord(emptyObj, false);
                DUMP_FOR_HANDLE(promiseIter);
                break;
            }
            case JSType::MICRO_JOB_QUEUE: {
                CHECK_DUMP_FIELDS(Record::SIZE, ecmascript::job::MicroJobQueue::SIZE, 2U);
                JSHandle<ecmascript::job::MicroJobQueue> microJob = factory->NewMicroJobQueue();
                DUMP_FOR_HANDLE(microJob);
                break;
            }
            case JSType::PENDING_JOB: {
#if defined(ENABLE_HITRACE)
                CHECK_DUMP_FIELDS(Record::SIZE, ecmascript::job::PendingJob::SIZE, 7U);
#else
                CHECK_DUMP_FIELDS(Record::SIZE, ecmascript::job::PendingJob::SIZE, 2U);
#endif
                JSHandle<JSHClass> pendingClass(thread,
                    JSHClass::Cast(globalConst->GetPendingJobClass().GetTaggedObject()));
                JSHandle<TaggedObject> pendingJob(thread, factory->NewObject(pendingClass));
                ecmascript::job::PendingJob::Cast(*pendingJob)->SetJob(thread, JSTaggedValue::Undefined());
                ecmascript::job::PendingJob::Cast(*pendingJob)->SetArguments(thread, JSTaggedValue::Undefined());
                DUMP_FOR_HANDLE(pendingJob);
                break;
            }
            case JSType::COMPLETION_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, CompletionRecord::SIZE, 2U);
                JSHandle<CompletionRecord> comRecord =
                    factory->NewCompletionRecord(CompletionRecordType::NORMAL, globalConst->GetHandledEmptyArray());
                DUMP_FOR_HANDLE(comRecord);
                break;
            }
            case JSType::MACHINE_CODE_OBJECT: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), MachineCode::SIZE, machineCodeSize);
                GTEST_LOG_(INFO) << "MACHINE_CODE_OBJECT not support new in MachineCodeSpace";
                break;
            }
            case JSType::CLASS_INFO_EXTRACTOR: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), ClassInfoExtractor::SIZE, 8U);
                JSHandle<ClassInfoExtractor> classInfoExtractor = factory->NewClassInfoExtractor(
                    JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined()));
                DUMP_FOR_HANDLE(classInfoExtractor);
                break;
            }
            case JSType::JS_API_ARRAY_LIST: {
                // 1 : 1 dump fileds number
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIArrayList::SIZE, 1U);
                JSHandle<JSAPIArrayList> jsArrayList = NewJSAPIArrayList(thread, factory, proto);
                DUMP_FOR_HANDLE(jsArrayList);
                break;
            }
            case JSType::JS_API_ARRAYLIST_ITERATOR: {
                // 2 : 2 dump fileds number
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIArrayListIterator::SIZE, 2U);
                JSHandle<JSAPIArrayList> jsArrayList = NewJSAPIArrayList(thread, factory, proto);
                JSHandle<JSAPIArrayListIterator> jsArrayListIter = factory->NewJSAPIArrayListIterator(jsArrayList);
                DUMP_FOR_HANDLE(jsArrayListIter);
                break;
            }
            case JSType::JS_API_BITVECTOR: {
                break;
            }
            case JSType::JS_API_BITVECTOR_ITERATOR: {
                break;
            }
            case JSType::LINKED_NODE: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), LinkedNode::SIZE, 4U);
                break;
            }
            case JSType::RB_TREENODE: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), RBTreeNode::SIZE, 7U);
                break;
            }
            case JSType::JS_API_HASH_MAP: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIHashMap::SIZE, 2U);
                JSHandle<JSAPIHashMap> jsHashMap = NewJSAPIHashMap(thread, factory);
                DUMP_FOR_HANDLE(jsHashMap);
                break;
            }
            case JSType::JS_API_HASH_SET: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIHashSet::SIZE, 2U);
                JSHandle<JSAPIHashSet> jsHashSet = NewJSAPIHashSet(thread, factory);
                DUMP_FOR_HANDLE(jsHashSet);
                break;
            }
            case JSType::JS_API_HASHMAP_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIHashMapIterator::SIZE, 4U);
                JSHandle<JSAPIHashMap> jsHashMap = NewJSAPIHashMap(thread, factory);
                JSHandle<JSAPIHashMapIterator> jsHashMapIter =
                    factory->NewJSAPIHashMapIterator(jsHashMap, IterationKind::KEY);
                DUMP_FOR_HANDLE(jsHashMapIter);
                break;
            }
            case JSType::JS_API_HASHSET_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIHashSetIterator::SIZE, 5U);
                JSHandle<JSAPIHashSet> jsHashSet = NewJSAPIHashSet(thread, factory);
                JSHandle<JSAPIHashSetIterator> jsHashSetIter =
                    factory->NewJSAPIHashSetIterator(jsHashSet, IterationKind::KEY);
                DUMP_FOR_HANDLE(jsHashSetIter);
                break;
            }
            case JSType::JS_API_LIGHT_WEIGHT_MAP: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPILightWeightMap::SIZE, 4U);
                JSHandle<JSAPILightWeightMap> jSAPILightWeightMap = NewJSAPILightWeightMap(thread, factory);
                DUMP_FOR_HANDLE(jSAPILightWeightMap);
                break;
            }
            case JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPILightWeightMapIterator::SIZE, 2U);
                JSHandle<JSAPILightWeightMap> jSAPILightWeightMap = NewJSAPILightWeightMap(thread, factory);
                JSHandle<JSAPILightWeightMapIterator> jSAPILightWeightMapIterator =
                    factory->NewJSAPILightWeightMapIterator(jSAPILightWeightMap, IterationKind::KEY);
                DUMP_FOR_HANDLE(jSAPILightWeightMapIterator);
                break;
            }
            case JSType::JS_API_LIGHT_WEIGHT_SET: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPILightWeightSet::SIZE, 3U);
                JSHandle<JSAPILightWeightSet> jSAPILightWeightSet = NewJSAPILightWeightSet(thread, factory);
                DUMP_FOR_HANDLE(jSAPILightWeightSet);
                break;
            }
            case JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPILightWeightSetIterator::SIZE, 2U);
                JSHandle<JSAPILightWeightSetIterator> jSAPILightWeightSetIter =
                    factory->NewJSAPILightWeightSetIterator(NewJSAPILightWeightSet(thread, factory),
                                                            IterationKind::KEY);
                DUMP_FOR_HANDLE(jSAPILightWeightSetIter);
                break;
            }
            case JSType::JS_API_QUEUE: {
                // 2 : 2 dump fileds number
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIQueue::SIZE, 2U);
                JSHandle<JSAPIQueue> jsQueue = NewJSAPIQueue(thread, factory, proto);
                DUMP_FOR_HANDLE(jsQueue);
                break;
            }
            case JSType::JS_API_QUEUE_ITERATOR: {
                // 2 : 2 dump fileds number
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIQueueIterator::SIZE, 2U);
                JSHandle<JSAPIQueue> jsQueue = NewJSAPIQueue(thread, factory, proto);
                JSHandle<JSAPIQueueIterator> jsQueueIter =
                    factory->NewJSAPIQueueIterator(jsQueue);
                DUMP_FOR_HANDLE(jsQueueIter);
                break;
            }
            case JSType::JS_API_PLAIN_ARRAY: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIPlainArray::SIZE, 3U);
                JSHandle<JSAPIPlainArray> jSAPIPlainArray = NewJSAPIPlainArray(thread, factory);
                DUMP_FOR_HANDLE(jSAPIPlainArray);
                break;
            }
            case JSType::JS_API_PLAIN_ARRAY_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIPlainArrayIterator::SIZE, 2U);
                JSHandle<JSAPIPlainArray> jSAPIPlainArray = NewJSAPIPlainArray(thread, factory);
                JSHandle<JSAPIPlainArrayIterator> jSAPIPlainArrayIter =
                    factory->NewJSAPIPlainArrayIterator(jSAPIPlainArray, IterationKind::KEY);
                DUMP_FOR_HANDLE(jSAPIPlainArrayIter);
                break;
            }
            case JSType::JS_API_TREE_MAP: {
                // 1 : 1 dump fileds number
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPITreeMap::SIZE, 1U);
                JSHandle<JSAPITreeMap> jsTreeMap = NewJSAPITreeMap(thread, factory);
                DUMP_FOR_HANDLE(jsTreeMap);
                break;
            }
            case JSType::JS_API_TREE_SET: {
                // 1 : 1 dump fileds number
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPITreeSet::SIZE, 1U);
                JSHandle<JSAPITreeSet> jsTreeSet = NewJSAPITreeSet(thread, factory);
                DUMP_FOR_HANDLE(jsTreeSet);
                break;
            }
            case JSType::JS_API_TREEMAP_ITERATOR: {
                // 3 : 3 dump fileds number
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPITreeMapIterator::SIZE, 3U);
                JSHandle<JSAPITreeMap> jsTreeMap = NewJSAPITreeMap(thread, factory);
                JSHandle<JSAPITreeMapIterator> jsTreeMapIter =
                    factory->NewJSAPITreeMapIterator(jsTreeMap, IterationKind::KEY);
                DUMP_FOR_HANDLE(jsTreeMapIter);
                break;
            }
            case JSType::JS_API_TREESET_ITERATOR: {
                // 3 : 3 dump fileds number
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPITreeSetIterator::SIZE, 3U);
                JSHandle<JSAPITreeSet> jsTreeSet = NewJSAPITreeSet(thread, factory);
                JSHandle<JSAPITreeSetIterator> jsTreeSetIter =
                    factory->NewJSAPITreeSetIterator(jsTreeSet, IterationKind::KEY);
                DUMP_FOR_HANDLE(jsTreeSetIter);
                break;
            }
            case JSType::JS_API_DEQUE: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIDeque::SIZE, 1U);
                JSHandle<JSAPIDeque> jsDeque = NewJSAPIDeque(thread, factory, proto);
                DUMP_FOR_HANDLE(jsDeque);
                break;
            }
            case JSType::JS_API_DEQUE_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIDequeIterator::SIZE, 2U);
                JSHandle<JSAPIDequeIterator> jsDequeIter =
                    factory->NewJSAPIDequeIterator(NewJSAPIDeque(thread, factory, proto));
                DUMP_FOR_HANDLE(jsDequeIter);
                break;
            }
            case JSType::JS_API_STACK: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIStack::SIZE, 1U);
                JSHandle<JSAPIStack> jsStack = NewJSAPIStack(factory, proto);
                DUMP_FOR_HANDLE(jsStack);
                break;
            }
            case JSType::JS_API_STACK_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIStackIterator::SIZE, 2U);
                JSHandle<JSAPIStackIterator> jsStackIter =
                    factory->NewJSAPIStackIterator(NewJSAPIStack(factory, proto));
                DUMP_FOR_HANDLE(jsStackIter);
                break;
            }
            case JSType::JS_API_VECTOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIVector::SIZE, 1U);
                JSHandle<JSAPIVector> jsVector = NewJSAPIVector(factory, proto);
                DUMP_FOR_HANDLE(jsVector);
                break;
            }
            case JSType::JS_API_VECTOR_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIVectorIterator::SIZE, 2U);
                JSHandle<JSAPIVectorIterator> jsVectorIter =
                    factory->NewJSAPIVectorIterator(NewJSAPIVector(factory, proto));
                DUMP_FOR_HANDLE(jsVectorIter);
                break;
            }
            case JSType::JS_API_LIST: {
                // 1 : 1 dump fileds number
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIList::SIZE, 2U);
                JSHandle<JSAPIList> jsAPIList = NewJSAPIList(thread, factory);
                DUMP_FOR_HANDLE(jsAPIList);
                break;
            }
            case JSType::JS_API_LINKED_LIST: {
                // 1 : 1 dump fileds number
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPILinkedList::SIZE, 1U);
                JSHandle<JSAPILinkedList> jsAPILinkedList = NewJSAPILinkedList(thread, factory);
                DUMP_FOR_HANDLE(jsAPILinkedList);
                break;
            }
            case JSType::JS_API_LIST_ITERATOR: {
                // 2 : 2 dump fileds number
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIListIterator::SIZE, 2U);
                JSHandle<JSAPIList> jsAPIList = NewJSAPIList(thread, factory);
                JSHandle<JSAPIListIterator> jsAPIListIter = factory->NewJSAPIListIterator(jsAPIList);
                DUMP_FOR_HANDLE(jsAPIListIter);
                break;
            }
            case JSType::JS_API_LINKED_LIST_ITERATOR: {
                // 2 : 2 dump fileds number
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPILinkedListIterator::SIZE, 2U);
                JSHandle<JSAPILinkedList> jsAPILinkedList = NewJSAPILinkedList(thread, factory);
                JSHandle<JSAPILinkedListIterator> jsAPILinkedListIter =
                    factory->NewJSAPILinkedListIterator(jsAPILinkedList);
                DUMP_FOR_HANDLE(jsAPILinkedListIter);
                break;
            }
            case JSType::MODULE_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, ModuleRecord::SIZE, 0U);
                break;
            }
            case JSType::SOURCE_TEXT_MODULE_RECORD: {
                CHECK_DUMP_FIELDS(ModuleRecord::SIZE, SourceTextModule::SIZE, 18U);
                JSHandle<SourceTextModule> moduleSourceRecord = factory->NewSourceTextModule();
                DUMP_FOR_HANDLE(moduleSourceRecord);
                break;
            }
            case JSType::IMPORTENTRY_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, ImportEntry::SIZE, 3U);
                JSHandle<ImportEntry> importEntry = factory->NewImportEntry();
                DUMP_FOR_HANDLE(importEntry);
                break;
            }
            case JSType::LOCAL_EXPORTENTRY_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, LocalExportEntry::SIZE, 3U);
                JSHandle<LocalExportEntry> localExportEntry = factory->NewLocalExportEntry();
                DUMP_FOR_HANDLE(localExportEntry);
                break;
            }
            case JSType::INDIRECT_EXPORTENTRY_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, IndirectExportEntry::SIZE, 3U);
                JSHandle<IndirectExportEntry> indirectExportEntry = factory->NewIndirectExportEntry();
                DUMP_FOR_HANDLE(indirectExportEntry);
                break;
            }
            case JSType::STAR_EXPORTENTRY_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, StarExportEntry::SIZE, 1U);
                JSHandle<StarExportEntry> starExportEntry = factory->NewStarExportEntry();
                DUMP_FOR_HANDLE(starExportEntry);
                break;
            }
            case JSType::RESOLVEDBINDING_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, ResolvedBinding::SIZE, 2U);
                JSHandle<ResolvedBinding> resolvedBinding = factory->NewResolvedBindingRecord();
                DUMP_FOR_HANDLE(resolvedBinding);
                break;
            }
            case JSType::RESOLVEDINDEXBINDING_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, ResolvedIndexBinding::SIZE, 2U);
                JSHandle<ResolvedIndexBinding> resolvedBinding = factory->NewResolvedIndexBindingRecord();
                DUMP_FOR_HANDLE(resolvedBinding);
                break;
            }
            case JSType::RESOLVEDRECORDINDEXBINDING_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, ResolvedRecordIndexBinding::SIZE, 3U);
                JSHandle<ResolvedRecordIndexBinding> recordBinding = factory->NewSResolvedRecordIndexBindingRecord();
                DUMP_FOR_HANDLE(recordBinding);
                break;
            }
            case JSType::RESOLVEDRECORDBINDING_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, ResolvedRecordBinding::SIZE, 2U);
                JSHandle<ResolvedRecordBinding> recordBinding = factory->NewSResolvedRecordBindingRecord();
                DUMP_FOR_HANDLE(recordBinding);
                break;
            }
            case JSType::JS_MODULE_NAMESPACE: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, ModuleNamespace::SIZE, 3U);
                JSHandle<ModuleNamespace> moduleNamespace = factory->NewModuleNamespace();
                DUMP_FOR_HANDLE(moduleNamespace);
                break;
            }
            case JSType::NATIVE_MODULE_FAILURE_INFO: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, NativeModuleFailureInfo::SIZE, 1U);
                JSHandle<NativeModuleFailureInfo> nativeFailureInfo = factory->NewNativeModuleFailureInfo();
                DUMP_FOR_HANDLE(nativeFailureInfo);
                break;
            }
            case JSType::JS_CJS_EXPORTS: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, CjsExports::SIZE, 1U);
                JSHandle<CjsExports> cjsExports = factory->NewCjsExports();
                DUMP_FOR_HANDLE(cjsExports);
                break;
            }
            case JSType::JS_CJS_MODULE: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, CjsModule::SIZE, 5U);
                JSHandle<CjsModule> cjsModule = factory->NewCjsModule();
                DUMP_FOR_HANDLE(cjsModule);
                break;
            }
            case JSType::JS_CJS_REQUIRE: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, CjsRequire::SIZE, 2U);
                JSHandle<CjsRequire> cjsRequire = factory->NewCjsRequire();
                DUMP_FOR_HANDLE(cjsRequire);
                break;
            }
            case JSType::JS_ITERATOR:
            case JSType::JS_ASYNCITERATOR:
            case JSType::FREE_OBJECT_WITH_ONE_FIELD:
            case JSType::FREE_OBJECT_WITH_NONE_FIELD:
            case JSType::FREE_OBJECT_WITH_TWO_FIELD:
            case JSType::JS_NATIVE_POINTER: {
                break;
            }
            case JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN: {
                CHECK_DUMP_FIELDS(JSFunction::SIZE, JSAsyncGeneratorResNextRetProRstFtn::SIZE, 1U);
                break;
            }
            case JSType::CLASS_LITERAL: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), ClassLiteral::SIZE, 2U);
                JSHandle<ClassLiteral> classLiteral = factory->NewClassLiteral();
                DUMP_FOR_HANDLE(classLiteral);
                break;
            }
            default:
                LOG_ECMA_MEM(FATAL) << "JSType " << static_cast<int>(type) << " cannot be dumped.";
                UNREACHABLE();
                break;
        }
    }
#undef NEW_OBJECT_AND_DUMP
#undef DUMP_FOR_HANDLE
}
}  // namespace panda::test
