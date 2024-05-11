/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "ecmascript/dfx/hprof/heap_profiler.h"
#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#include "ecmascript/dfx/hprof/heap_snapshot.h"
#include "ecmascript/dfx/hprof/heap_snapshot_json_serializer.h"
#include "ecmascript/dfx/hprof/string_hashmap.h"
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

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define CHECK_DUMP_FIELDS(begin, end, num);                                           \
    LOG_ECMA_IF((num) != ((end) - (begin)) / JSTaggedValue::TaggedTypeSize(), FATAL) \
        << "Fields in obj are not in dump list. "

namespace panda::test {
class EcmaDumpTest : public testing::Test {
public:
    static void SetUpTestCase()
    {
        GTEST_LOG_(INFO) << "SetUpTestCase";
    }

    static void TearDownTestCase()
    {
        GTEST_LOG_(INFO) << "TearDownCase";
    }

    void SetUp() override
    {
        TestHelper::CreateEcmaVMWithScope(instance, thread, scope);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    ecmascript::EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
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

#define NEW_OBJECT_AND_DUMP(ClassName, TypeName)                                                \
    do {                                                                                        \
        JSHandle<JSHClass> class##ClassName =                                                   \
            factory->NewEcmaHClass(ClassName::SIZE, JSType::TypeName, proto);                   \
        JSHandle<JSObject> object##ClassName = factory->NewJSObjectWithInit(class##ClassName);  \
        DUMP_FOR_HANDLE(object##ClassName);                                                     \
    } while (false)

#define CHECK_AND_DUMP_HANDLE(JSType, JSHandleType, BeginSize, EndSize, Num, Func)           \
        case JSType: {                                                          \
            CHECK_DUMP_FIELDS(BeginSize, EndSize, Num);                                       \
            JSHandle<JSHandleType> jsHandle = Func;                                           \
            DUMP_FOR_HANDLE(jsHandle);                                                        \
            break;                                                                            \
            }                                                                                 

#define CHECK_DUMP_FIELDS_WITH_JSTYPE(JSType, BeginSize, EndSize, Num)\
            case JSType: {\
                CHECK_DUMP_FIELDS(BeginSize, EndSize, Num);\
                break;\
            }

#define CAST_CHECK_AND_DUMP_HANDLE(JSType, BeginSize, EndSize, Num, FuncCast)\
        case JSType: {                                                          \
            CHECK_DUMP_FIELDS(BeginSize, EndSize, Num);                         \
            JSHandle<JSHClass> jsHClass = JSHandle<JSHClass>::Cast(FuncCast); \
            JSHandle<JSObject> jsFunc =  factory->NewJSObjectWithInit(jsHClass);   \
            DUMP_FOR_HANDLE(jsFunc);                                                \
            break;                                                                  \
            }

#define CHECK_AND_DUMP_NEW_OBJECT(JSType, BeginSize, EndSize, Num, ClassName, TypeName)\
            case JSType: {\
                CHECK_DUMP_FIELDS(BeginSize, EndSize, Num);\
                NEW_OBJECT_AND_DUMP(ClassName, TypeName);\
                break;\
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

            CHECK_AND_DUMP_HANDLE(JSType::JS_REALM, JSRealm, JSObject::SIZE, JSRealm::SIZE, 2U, factory->NewJSRealm());
            case JSType::METHOD: {
#ifdef PANDA_TARGET_64
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), Method::SIZE, 6U);
#else
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), Method::SIZE, 5U);
#endif
                break;
            }
            CHECK_DUMP_FIELDS_WITH_JSTYPE(JSType::JS_FUNCTION_BASE, JSObject::SIZE, JSFunctionBase::SIZE, 3U);

            case JSType::JS_FUNCTION:
            case JSType::JS_SHARED_FUNCTION: {
                CHECK_DUMP_FIELDS(JSFunctionBase::SIZE, JSFunction::SIZE, 8U);
                JSHandle<JSTaggedValue> jsFunc = globalEnv->GetFunctionFunction();
                DUMP_FOR_HANDLE(jsFunc);
                break;
            }
            // CHECK_AND_DUMP_HANDLE(JSType::JS_SHARED_FUNCTION, JSTaggedValue, JSFunctionBase::SIZE, JSFunction::SIZE, 8U, globalEnv->GetFunctionFunction());

            CAST_CHECK_AND_DUMP_HANDLE(JSType::JS_PROXY_REVOC_FUNCTION, JSFunction::SIZE, JSProxyRevocFunction::SIZE, 1U, globalEnv->GetProxyRevocFunctionClass());
            CAST_CHECK_AND_DUMP_HANDLE(JSType::JS_PROMISE_REACTIONS_FUNCTION, JSFunction::SIZE, JSPromiseReactionsFunction::SIZE, 2U, globalEnv->GetPromiseReactionFunctionClass());
            CAST_CHECK_AND_DUMP_HANDLE(JSType::JS_PROMISE_EXECUTOR_FUNCTION, JSFunction::SIZE, JSPromiseExecutorFunction::SIZE, 1U, globalEnv->GetPromiseExecutorFunctionClass());
            CAST_CHECK_AND_DUMP_HANDLE(JSType::JS_ASYNC_MODULE_FULFILLED_FUNCTION, JSFunction::SIZE, JSAsyncModuleFulfilledFunction::SIZE, 0U, globalEnv->GetAsyncModuleFulfilledFunctionClass());
            CAST_CHECK_AND_DUMP_HANDLE(JSType::JS_ASYNC_MODULE_REJECTED_FUNCTION, JSFunction::SIZE, JSAsyncModuleRejectedFunction::SIZE, 0U, globalEnv->GetAsyncModuleRejectedFunctionClass());
            CAST_CHECK_AND_DUMP_HANDLE(JSType::JS_PROMISE_ALL_RESOLVE_ELEMENT_FUNCTION, JSFunction::SIZE, JSPromiseAllResolveElementFunction::SIZE, 5U, globalEnv->GetPromiseAllResolveElementFunctionClass());
            CAST_CHECK_AND_DUMP_HANDLE(JSType::JS_PROMISE_ANY_REJECT_ELEMENT_FUNCTION, JSFunction::SIZE, JSPromiseAnyRejectElementFunction::SIZE, 5U, globalEnv->GetPromiseAnyRejectElementFunctionClass());
            CAST_CHECK_AND_DUMP_HANDLE(JSType::JS_PROMISE_ALL_SETTLED_ELEMENT_FUNCTION, JSFunction::SIZE, JSPromiseAllSettledElementFunction::SIZE, 5U, globalEnv->GetPromiseAllSettledElementFunctionClass());
            CAST_CHECK_AND_DUMP_HANDLE(JSType::JS_PROMISE_FINALLY_FUNCTION, JSFunction::SIZE, JSPromiseFinallyFunction::SIZE, 2U, globalEnv->GetPromiseFinallyFunctionClass());
            CAST_CHECK_AND_DUMP_HANDLE(JSType::JS_PROMISE_VALUE_THUNK_OR_THROWER_FUNCTION, JSFunction::SIZE, JSPromiseValueThunkOrThrowerFunction::SIZE, 1U, globalEnv->GetPromiseValueThunkOrThrowerFunctionClass());

            CHECK_DUMP_FIELDS_WITH_JSTYPE(JSType::JS_ASYNC_GENERATOR_FUNCTION, JSFunction::SIZE, JSAsyncGeneratorFunction::SIZE, 0U);

            CHECK_DUMP_FIELDS_WITH_JSTYPE(JSType::JS_GENERATOR_FUNCTION, JSFunction::SIZE, JSGeneratorFunction::SIZE, 0U);


            CHECK_DUMP_FIELDS_WITH_JSTYPE(JSType::JS_ASYNC_FUNCTION, JSFunction::SIZE, JSAsyncFunction::SIZE, 0U);

             CHECK_AND_DUMP_HANDLE(JSType::JS_INTL_BOUND_FUNCTION, JSIntlBoundFunction, JSFunction::SIZE, JSIntlBoundFunction::SIZE, 3U, \
             factory->NewJSIntlBoundFunction(MethodIndex::BUILTINS_NUMBER_FORMAT_NUMBER_FORMAT_INTERNAL_FORMAT_NUMBER));

             CHECK_AND_DUMP_HANDLE(JSType::JS_ASYNC_AWAIT_STATUS_FUNCTION, JSAsyncAwaitStatusFunction, \
             JSFunction::SIZE, JSAsyncAwaitStatusFunction::SIZE, 1U, \
             factory->NewJSAsyncAwaitStatusFunction(MethodIndex::BUILTINS_PROMISE_HANDLER_ASYNC_AWAIT_FULFILLED));

            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_BOUND_FUNCTION, JSFunctionBase::SIZE, JSBoundFunction::SIZE, 3U, JSBoundFunction, JS_BOUND_FUNCTION);
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_REG_EXP, JSObject::SIZE, JSRegExp::SIZE, 5U, JSRegExp, JS_REG_EXP);

             CHECK_AND_DUMP_HANDLE(JSType::JS_SET, JSSet, JSObject::SIZE, JSSet::SIZE, 1U, \
             NewJSSet(thread, factory, proto));

             CHECK_AND_DUMP_HANDLE(JSType::JS_SHARED_SET, JSSharedSet, JSObject::SIZE, JSSharedSet::SIZE, 2U, \
             NewJSSharedSet(thread, factory));
            

             CHECK_AND_DUMP_HANDLE(JSType::JS_MAP, JSMap, JSObject::SIZE, JSMap::SIZE, 1U, \
             NewJSMap(thread, factory, proto));

             CHECK_AND_DUMP_HANDLE(JSType::JS_SHARED_MAP, JSSharedMap, JSObject::SIZE, JSSharedMap::SIZE, 2U, \
             NewJSSharedMap(thread, factory));

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
             CHECK_AND_DUMP_HANDLE(JSType::CELL_RECORD, CellRecord, Record::SIZE, CellRecord::SIZE, 2U, \
             factory->NewCellRecord());

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

             CHECK_AND_DUMP_HANDLE(JSType::JS_MAP_ITERATOR, JSMapIterator, JSObject::SIZE, JSMapIterator::SIZE, 2U, \
             factory->NewJSMapIterator(NewJSMap(thread, factory, proto), IterationKind::KEY));

            CHECK_AND_DUMP_HANDLE(JSType::JS_SHARED_MAP_ITERATOR, JSSharedMapIterator, JSObject::SIZE, JSSharedMapIterator::SIZE, 2U, \
            factory->NewJSMapIterator(NewJSSharedMap(thread, factory), IterationKind::KEY));

            CHECK_AND_DUMP_HANDLE(JSType::JS_SET_ITERATOR, JSSetIterator, JSObject::SIZE, JSSetIterator::SIZE, 2U, \
            factory->NewJSSetIterator(NewJSSet(thread, factory, proto), IterationKind::KEY));

            CHECK_AND_DUMP_HANDLE(JSType::JS_SHARED_SET_ITERATOR, JSSharedSetIterator, JSObject::SIZE, JSSharedSetIterator::SIZE, 2U, \
            factory->NewJSSetIterator(NewJSSharedSet(thread, factory), IterationKind::KEY));

            case JSType::JS_REG_EXP_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSRegExpIterator::SIZE, 3U);
                JSHandle<EcmaString> emptyString(thread->GlobalConstants()->GetHandledEmptyString());
                JSHandle<JSTaggedValue> jsRegExp(NewJSRegExp(thread, factory, proto));
                JSHandle<JSRegExpIterator> jsRegExpIter =
                    factory->NewJSRegExpIterator(jsRegExp, emptyString, false, false);
                DUMP_FOR_HANDLE(jsRegExpIter);
                break;
            }

            CHECK_AND_DUMP_HANDLE(JSType::JS_ARRAY_ITERATOR, JSArrayIterator, JSObject::SIZE, JSArrayIterator::SIZE, 2U, \
            factory->NewJSArrayIterator(JSHandle<JSObject>::Cast(factory->NewJSArray()), IterationKind::KEY));

            CHECK_AND_DUMP_HANDLE(JSType::JS_SHARED_ARRAY_ITERATOR, JSSharedArrayIterator, JSObject::SIZE, JSArrayIterator::SIZE, 2U, \
            factory->NewJSSharedArrayIterator(JSHandle<JSObject>::Cast(factory->NewJSSArray()), IterationKind::KEY));

            CHECK_AND_DUMP_HANDLE(JSType::JS_STRING_ITERATOR, JSTaggedValue, JSObject::SIZE, JSStringIterator::SIZE, 2U, \
            globalEnv->GetStringIterator());

            case JSType::JS_SHARED_ARRAY_BUFFER:
            case JSType::JS_ARRAY_BUFFER: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSArrayBuffer::SIZE, 2U);
                NEW_OBJECT_AND_DUMP(JSArrayBuffer, JS_ARRAY_BUFFER);
                break;
            }

            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_INTL, JSObject::SIZE, JSIntl::SIZE, 1U, JSIntl, JS_INTL);
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_LOCALE, JSObject::SIZE, JSLocale::SIZE, 1U, JSLocale, JS_LOCALE);
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_DATE_TIME_FORMAT, JSObject::SIZE, JSDateTimeFormat::SIZE, 9U, JSDateTimeFormat, JS_DATE_TIME_FORMAT);
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_RELATIVE_TIME_FORMAT, JSObject::SIZE, JSRelativeTimeFormat::SIZE, 4U, JSRelativeTimeFormat, JS_RELATIVE_TIME_FORMAT);
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_NUMBER_FORMAT, JSObject::SIZE, JSNumberFormat::SIZE, 13U, JSNumberFormat, JS_NUMBER_FORMAT);
            
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_COLLATOR, JSObject::SIZE, JSCollator::SIZE, 5U, JSCollator, JS_COLLATOR);
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_PLURAL_RULES, JSObject::SIZE, JSPluralRules::SIZE, 9U, JSPluralRules, JS_PLURAL_RULES);
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_DISPLAYNAMES, JSObject::SIZE, JSDisplayNames::SIZE, 3U, JSDisplayNames, JS_DISPLAYNAMES);
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_SEGMENTER, JSObject::SIZE, JSSegmenter::SIZE, 3U, JSSegmenter, JS_SEGMENTER);
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_SEGMENTS, JSObject::SIZE, JSSegments::SIZE, 4U, JSSegments, JS_SEGMENTS);
            
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_SEGMENT_ITERATOR, JSObject::SIZE, JSSegmentIterator::SIZE, 4U, JSSegmentIterator, JS_SEGMENTS);
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_LIST_FORMAT, JSObject::SIZE, JSListFormat::SIZE, 3U, JSListFormat, JS_LIST_FORMAT);
            // CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_ARRAY_BUFFER, JSObject::SIZE, JSArrayBuffer::SIZE, 2U, JSArrayBuffer, JS_ARRAY_BUFFER);
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_SENDABLE_ARRAY_BUFFER, JSObject::SIZE, JSSendableArrayBuffer::SIZE, 2U, JSSendableArrayBuffer, JS_SENDABLE_ARRAY_BUFFER);
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_PROMISE, JSObject::SIZE, JSPromise::SIZE, 4U, JSPromise, JS_PROMISE);
           
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_DATA_VIEW, JSObject::SIZE, JSDataView::SIZE, 3U, JSDataView, JS_DATA_VIEW);
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_GENERATOR_OBJECT, JSObject::SIZE, JSGeneratorObject::SIZE, 4U, JSGeneratorObject, JS_GENERATOR_OBJECT);
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_ASYNC_GENERATOR_OBJECT, JSObject::SIZE, JSAsyncGeneratorObject::SIZE, 5U, JSAsyncGeneratorObject, JS_ASYNC_GENERATOR_OBJECT);

            case JSType::JS_ASYNC_FUNC_OBJECT: {
                CHECK_DUMP_FIELDS(JSGeneratorObject::SIZE, JSAsyncFuncObject::SIZE, 1U);
                JSHandle<JSAsyncFuncObject> asyncFuncObject = factory->NewJSAsyncFuncObject();
                DUMP_FOR_HANDLE(asyncFuncObject);
                break;
            }

            CHECK_AND_DUMP_HANDLE(JSType::JS_ARRAY, JSArray, JSObject::SIZE, JSArray::SIZE, 2U, \
            factory->NewJSArray());

            CHECK_AND_DUMP_HANDLE(JSType::JS_SHARED_ARRAY, JSSharedArray, JSObject::SIZE, JSArray::SIZE, 2U, \
            factory->NewJSSArray());

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
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_PRIMITIVE_REF, JSObject::SIZE, JSPrimitiveRef::SIZE, 1U, JSPrimitiveRef, JS_PRIMITIVE_REF);
            CHECK_AND_DUMP_HANDLE(JSType::JS_GLOBAL_OBJECT, JSTaggedValue, JSObject::SIZE, JSGlobalObject::SIZE, 0U, \
            globalEnv->GetJSGlobalObject());

            case JSType::JS_PROXY: {
                CHECK_DUMP_FIELDS(ECMAObject::SIZE, JSProxy::SIZE, 5U);
                JSHandle<JSTaggedValue> emptyObj(thread, NewJSObject(thread, factory, globalEnv).GetTaggedValue());
                JSHandle<JSProxy> proxy = factory->NewJSProxy(emptyObj, emptyObj);
                DUMP_FOR_HANDLE(proxy);
                break;
            }

            CHECK_AND_DUMP_HANDLE(JSType::HCLASS, JSHClass, TaggedObject::TaggedObjectSize(), JSHClass::SIZE, 10U, \
            factory->NewEcmaHClass(JSHClass::SIZE, JSType::HCLASS, proto));
          
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
            case JSType::TAGGED_ARRAY:
            case JSType::VTABLE:
            case JSType::LEXICAL_ENV:
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

            CHECK_AND_DUMP_HANDLE(JSType::INTERNAL_ACCESSOR, AccessorData, Record::SIZE, AccessorData::SIZE, 2U, \
            factory->NewAccessorData());

            CHECK_AND_DUMP_HANDLE(JSType::SYMBOL, JSSymbol, TaggedObject::TaggedObjectSize(), JSSymbol::SIZE, 3U, \
            factory->NewJSSymbol());

            CHECK_AND_DUMP_HANDLE(JSType::JS_GENERATOR_CONTEXT, GeneratorContext, TaggedObject::TaggedObjectSize(), GeneratorContext::SIZE, 7U, \
            factory->NewGeneratorContext());

            CHECK_AND_DUMP_HANDLE(JSType::PROTOTYPE_HANDLER, PrototypeHandler, TaggedObject::TaggedObjectSize(), PrototypeHandler::SIZE, 4U, \
            factory->NewPrototypeHandler());

            CHECK_AND_DUMP_HANDLE(JSType::TRANSITION_HANDLER, TransitionHandler, TaggedObject::TaggedObjectSize(), TransitionHandler::SIZE, 2U, \
            factory->NewTransitionHandler());

            CHECK_AND_DUMP_HANDLE(JSType::TRANS_WITH_PROTO_HANDLER, TransWithProtoHandler, TaggedObject::TaggedObjectSize(), TransWithProtoHandler::SIZE, 3U, \
            factory->NewTransWithProtoHandler());

            CHECK_AND_DUMP_HANDLE(JSType::STORE_TS_HANDLER, StoreTSHandler, TaggedObject::TaggedObjectSize(), StoreTSHandler::SIZE, 3U, \
            factory->NewStoreTSHandler());

            CHECK_AND_DUMP_HANDLE(JSType::PROPERTY_BOX, PropertyBox, TaggedObject::TaggedObjectSize(), PropertyBox::SIZE, 1U, \
            factory->NewPropertyBox(globalConst->GetHandledEmptyArray()));

            CHECK_AND_DUMP_HANDLE(JSType::PROTO_CHANGE_MARKER, ProtoChangeMarker, TaggedObject::TaggedObjectSize(), ProtoChangeMarker::SIZE, 1U, \
            factory->NewProtoChangeMarker());

            CHECK_AND_DUMP_HANDLE(JSType::MARKER_CELL, MarkerCell, TaggedObject::TaggedObjectSize(), MarkerCell::SIZE, 1U, \
            factory->NewMarkerCell());

            CHECK_DUMP_FIELDS_WITH_JSTYPE(JSType::TRACK_INFO, TaggedObject::TaggedObjectSize(), TrackInfo::SIZE, 3U);
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

            CHECK_AND_DUMP_HANDLE(JSType::PROGRAM, Program, ECMAObject::SIZE, Program::SIZE, 1U, \
            factory->NewProgram());

            CHECK_AND_DUMP_HANDLE(JSType::PROMISE_CAPABILITY, PromiseCapability, Record::SIZE, PromiseCapability::SIZE, 3U, \
            factory->NewPromiseCapability());

            CHECK_AND_DUMP_HANDLE(JSType::PROMISE_RECORD, PromiseRecord, Record::SIZE, PromiseRecord::SIZE, 1U, \
            factory->NewPromiseRecord());

            CHECK_AND_DUMP_HANDLE(JSType::RESOLVING_FUNCTIONS_RECORD, ResolvingFunctionsRecord, Record::SIZE, ResolvingFunctionsRecord::SIZE, 2U, \
            factory->NewResolvingFunctionsRecord());

            CHECK_AND_DUMP_HANDLE(JSType::ASYNC_GENERATOR_REQUEST, AsyncGeneratorRequest, Record::SIZE, AsyncGeneratorRequest::SIZE, 2U, \
            factory->NewAsyncGeneratorRequest());

            case JSType::ASYNC_ITERATOR_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, AsyncIteratorRecord::SIZE, 3U);
                JSHandle<JSTaggedValue> emptyObj(thread, NewJSObject(thread, factory, globalEnv).GetTaggedValue());
                JSHandle<JSTaggedValue> emptyMethod(thread, NewJSObject(thread, factory, globalEnv).GetTaggedValue());
                JSHandle<AsyncIteratorRecord> asyncIteratorRecord =
                    factory->NewAsyncIteratorRecord(emptyObj, emptyMethod, false);
                DUMP_FOR_HANDLE(asyncIteratorRecord);
                break;
            }
            CHECK_AND_DUMP_NEW_OBJECT(JSType::JS_ASYNC_FROM_SYNC_ITERATOR, JSObject::SIZE, JSAsyncFromSyncIterator::SIZE, 1U, JSAsyncFromSyncIterator, JS_ASYNC_FROM_SYNC_ITERATOR);
            CHECK_DUMP_FIELDS_WITH_JSTYPE(JSType::JS_ASYNC_FROM_SYNC_ITER_UNWARP_FUNCTION, \
            JSFunction::SIZE, JSAsyncFromSyncIterUnwarpFunction::SIZE, 1U);

            CHECK_AND_DUMP_HANDLE(JSType::PROMISE_REACTIONS, PromiseReaction, Record::SIZE, PromiseReaction::SIZE, 3U, \
            factory->NewPromiseReaction());

            case JSType::PROMISE_ITERATOR_RECORD: {
                CHECK_DUMP_FIELDS(Record::SIZE, PromiseIteratorRecord::SIZE, 2U);
                JSHandle<JSTaggedValue> emptyObj(thread, NewJSObject(thread, factory, globalEnv).GetTaggedValue());
                JSHandle<PromiseIteratorRecord> promiseIter = factory->NewPromiseIteratorRecord(emptyObj, false);
                DUMP_FOR_HANDLE(promiseIter);
                break;
            }

            CHECK_AND_DUMP_HANDLE(JSType::MICRO_JOB_QUEUE, ecmascript::job::MicroJobQueue, Record::SIZE, ecmascript::job::MicroJobQueue::SIZE, 2U, \
            factory->NewMicroJobQueue());

            case JSType::PENDING_JOB: {
#if defined(ENABLE_HITRACE)
                CHECK_DUMP_FIELDS(Record::SIZE, ecmascript::job::PendingJob::SIZE, 6U);
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

            CHECK_AND_DUMP_HANDLE(JSType::COMPLETION_RECORD, CompletionRecord, Record::SIZE, CompletionRecord::SIZE, 2U, \
            factory->NewCompletionRecord(CompletionRecordType::NORMAL, globalConst->GetHandledEmptyArray()));

            case JSType::MACHINE_CODE_OBJECT: {
                CHECK_DUMP_FIELDS(TaggedObject::TaggedObjectSize(), MachineCode::SIZE, 5U);
                GTEST_LOG_(INFO) << "MACHINE_CODE_OBJECT not support new in MachineCodeSpace";
                break;
            }

            CHECK_AND_DUMP_HANDLE(JSType::CLASS_INFO_EXTRACTOR, ClassInfoExtractor, TaggedObject::TaggedObjectSize(), ClassInfoExtractor::SIZE, 8U, \
            factory->NewClassInfoExtractor(JSHandle<JSTaggedValue>(thread, JSTaggedValue::Undefined())));

            CHECK_AND_DUMP_HANDLE(JSType::JS_API_ARRAY_LIST, JSAPIArrayList, JSObject::SIZE, JSAPIArrayList::SIZE, 1U, \
            NewJSAPIArrayList(thread, factory, proto));

            case JSType::JS_API_ARRAYLIST_ITERATOR: {
                // 2 : 2 dump fileds number
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIArrayListIterator::SIZE, 2U);
                JSHandle<JSAPIArrayList> jsArrayList = NewJSAPIArrayList(thread, factory, proto);
                JSHandle<JSAPIArrayListIterator> jsArrayListIter = factory->NewJSAPIArrayListIterator(jsArrayList);
                DUMP_FOR_HANDLE(jsArrayListIter);
                break;
            }

            CHECK_DUMP_FIELDS_WITH_JSTYPE(JSType::LINKED_NODE, TaggedObject::TaggedObjectSize(), LinkedNode::SIZE, 4U);

            CHECK_DUMP_FIELDS_WITH_JSTYPE(JSType::RB_TREENODE, TaggedObject::TaggedObjectSize(), RBTreeNode::SIZE, 7U);

            CHECK_AND_DUMP_HANDLE(JSType::JS_API_HASH_MAP, JSAPIHashMap, JSObject::SIZE, JSAPIHashMap::SIZE, 2U, \
             NewJSAPIHashMap(thread, factory));

            CHECK_AND_DUMP_HANDLE(JSType::JS_API_HASH_SET, JSAPIHashSet, JSObject::SIZE, JSAPIHashSet::SIZE, 2U, \
             NewJSAPIHashSet(thread, factory));

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

            CHECK_AND_DUMP_HANDLE(JSType::JS_API_LIGHT_WEIGHT_MAP, JSAPILightWeightMap, JSObject::SIZE, JSAPILightWeightMap::SIZE, 4U, \
             NewJSAPILightWeightMap(thread, factory));

            case JSType::JS_API_LIGHT_WEIGHT_MAP_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPILightWeightMapIterator::SIZE, 2U);
                JSHandle<JSAPILightWeightMap> jSAPILightWeightMap = NewJSAPILightWeightMap(thread, factory);
                JSHandle<JSAPILightWeightMapIterator> jSAPILightWeightMapIterator =
                    factory->NewJSAPILightWeightMapIterator(jSAPILightWeightMap, IterationKind::KEY);
                DUMP_FOR_HANDLE(jSAPILightWeightMapIterator);
                break;
            }

            CHECK_AND_DUMP_HANDLE(JSType::JS_API_LIGHT_WEIGHT_SET, JSAPILightWeightSet, JSObject::SIZE, JSAPILightWeightSet::SIZE, 3U, NewJSAPILightWeightSet(thread, factory));
            CHECK_AND_DUMP_HANDLE(JSType::JS_API_LIGHT_WEIGHT_SET_ITERATOR, JSAPILightWeightSetIterator, JSObject::SIZE, JSAPILightWeightSetIterator::SIZE, 2U, factory->NewJSAPILightWeightSetIterator(NewJSAPILightWeightSet(thread, factory), IterationKind::KEY));
            CHECK_AND_DUMP_HANDLE(JSType::JS_API_QUEUE, JSAPIQueue, JSObject::SIZE, JSAPIQueue::SIZE, 2U, NewJSAPIQueue(thread, factory, proto));

            case JSType::JS_API_QUEUE_ITERATOR: {
                // 2 : 2 dump fileds number
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIQueueIterator::SIZE, 2U);
                JSHandle<JSAPIQueue> jsQueue = NewJSAPIQueue(thread, factory, proto);
                JSHandle<JSAPIQueueIterator> jsQueueIter =
                    factory->NewJSAPIQueueIterator(jsQueue);
                DUMP_FOR_HANDLE(jsQueueIter);
                break;
            }

            CHECK_AND_DUMP_HANDLE(JSType::JS_API_PLAIN_ARRAY, JSAPIPlainArray, JSObject::SIZE, JSAPIPlainArray::SIZE, 3U, NewJSAPIPlainArray(thread, factory));

            case JSType::JS_API_PLAIN_ARRAY_ITERATOR: {
                CHECK_DUMP_FIELDS(JSObject::SIZE, JSAPIPlainArrayIterator::SIZE, 2U);
                JSHandle<JSAPIPlainArray> jSAPIPlainArray = NewJSAPIPlainArray(thread, factory);
                JSHandle<JSAPIPlainArrayIterator> jSAPIPlainArrayIter =
                    factory->NewJSAPIPlainArrayIterator(jSAPIPlainArray, IterationKind::KEY);
                DUMP_FOR_HANDLE(jSAPIPlainArrayIter);
                break;
            }

            CHECK_AND_DUMP_HANDLE(JSType::JS_API_TREE_MAP, JSAPITreeMap, JSObject::SIZE, JSAPITreeMap::SIZE, 1U, NewJSAPITreeMap(thread, factory));
            CHECK_AND_DUMP_HANDLE(JSType::JS_API_TREE_SET, JSAPITreeSet, JSObject::SIZE, JSAPITreeSet::SIZE, 1U,\
            NewJSAPITreeSet(thread, factory));

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

            CHECK_AND_DUMP_HANDLE(JSType::JS_API_DEQUE, JSAPIDeque, JSObject::SIZE, JSAPIDeque::SIZE, 1U, NewJSAPIDeque(thread, factory, proto));
            CHECK_AND_DUMP_HANDLE(JSType::JS_API_DEQUE_ITERATOR, JSAPIDequeIterator, JSObject::SIZE, JSAPIDequeIterator::SIZE, 2U, factory->NewJSAPIDequeIterator(NewJSAPIDeque(thread, factory, proto)));
            CHECK_AND_DUMP_HANDLE(JSType::JS_API_STACK, JSAPIStack, JSObject::SIZE, JSAPIStack::SIZE, 1U, NewJSAPIStack(factory, proto));
            CHECK_AND_DUMP_HANDLE(JSType::JS_API_STACK_ITERATOR, JSAPIStackIterator, JSObject::SIZE, JSAPIStackIterator::SIZE, 2U, factory->NewJSAPIStackIterator(NewJSAPIStack(factory, proto)));
            
            CHECK_AND_DUMP_HANDLE(JSType::JS_API_VECTOR, JSAPIVector, JSObject::SIZE, JSAPIVector::SIZE, 1U, NewJSAPIVector(factory, proto));
            CHECK_AND_DUMP_HANDLE(JSType::JS_API_VECTOR_ITERATOR, JSAPIVectorIterator, JSObject::SIZE, JSAPIVectorIterator::SIZE, 2U, factory->NewJSAPIVectorIterator(NewJSAPIVector(factory, proto)));
            CHECK_AND_DUMP_HANDLE(JSType::JS_API_LIST, JSAPIList, JSObject::SIZE, JSAPIList::SIZE, 2U, NewJSAPIList(thread, factory));
            CHECK_AND_DUMP_HANDLE(JSType::JS_API_LINKED_LIST, JSAPILinkedList, JSObject::SIZE, JSAPILinkedList::SIZE, 1U, NewJSAPILinkedList(thread, factory));            
            
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

            CHECK_AND_DUMP_HANDLE(JSType::SOURCE_TEXT_MODULE_RECORD, SourceTextModule, ModuleRecord::SIZE, SourceTextModule::SIZE, 16U, factory->NewSourceTextModule());
            CHECK_AND_DUMP_HANDLE(JSType::IMPORTENTRY_RECORD, ImportEntry, Record::SIZE, ImportEntry::SIZE, 3U, factory->NewImportEntry());
            CHECK_AND_DUMP_HANDLE(JSType::LOCAL_EXPORTENTRY_RECORD, LocalExportEntry, Record::SIZE, LocalExportEntry::SIZE, 3U, factory->NewLocalExportEntry());
            CHECK_AND_DUMP_HANDLE(JSType::INDIRECT_EXPORTENTRY_RECORD, IndirectExportEntry, Record::SIZE, IndirectExportEntry::SIZE, 3U, factory->NewIndirectExportEntry());
            CHECK_AND_DUMP_HANDLE(JSType::STAR_EXPORTENTRY_RECORD, StarExportEntry, Record::SIZE, StarExportEntry::SIZE, 1U, factory->NewStarExportEntry());

            CHECK_AND_DUMP_HANDLE(JSType::RESOLVEDBINDING_RECORD, ResolvedBinding, Record::SIZE, ResolvedBinding::SIZE, 2U, factory->NewResolvedBindingRecord());
            CHECK_AND_DUMP_HANDLE(JSType::RESOLVEDINDEXBINDING_RECORD, ResolvedIndexBinding, Record::SIZE, ResolvedIndexBinding::SIZE, 2U, factory->NewResolvedIndexBindingRecord());
            CHECK_AND_DUMP_HANDLE(JSType::RESOLVEDRECORDINDEXBINDING_RECORD, ResolvedRecordIndexBinding, Record::SIZE, ResolvedRecordIndexBinding::SIZE, 2U, factory->NewSResolvedRecordIndexBindingRecord());
            CHECK_AND_DUMP_HANDLE(JSType::RESOLVEDRECORDBINDING_RECORD, ResolvedRecordBinding, Record::SIZE, ResolvedRecordBinding::SIZE, 2U, factory->NewSResolvedRecordBindingRecord());
            CHECK_AND_DUMP_HANDLE(JSType::JS_MODULE_NAMESPACE, ModuleNamespace, JSObject::SIZE, ModuleNamespace::SIZE, 3U, factory->NewModuleNamespace());

            CHECK_AND_DUMP_HANDLE(JSType::JS_CJS_EXPORTS, CjsExports, JSObject::SIZE, CjsExports::SIZE, 1U, factory->NewCjsExports());
            CHECK_AND_DUMP_HANDLE(JSType::JS_CJS_MODULE, CjsModule, JSObject::SIZE, CjsModule::SIZE, 5U, factory->NewCjsModule());
            CHECK_AND_DUMP_HANDLE(JSType::JS_CJS_REQUIRE, CjsRequire, JSObject::SIZE, CjsRequire::SIZE, 2U, factory->NewCjsRequire());

            case JSType::JS_ITERATOR:
            case JSType::JS_ASYNCITERATOR:
            case JSType::FREE_OBJECT_WITH_ONE_FIELD:
            case JSType::FREE_OBJECT_WITH_NONE_FIELD:
            case JSType::FREE_OBJECT_WITH_TWO_FIELD:
            case JSType::JS_NATIVE_POINTER: {
                break;
            }
            
            CHECK_DUMP_FIELDS_WITH_JSTYPE(JSType::JS_ASYNC_GENERATOR_RESUME_NEXT_RETURN_PROCESSOR_RST_FTN, \
            JSFunction::SIZE, JSAsyncGeneratorResNextRetProRstFtn::SIZE, 1U);

            CHECK_AND_DUMP_HANDLE(JSType::CLASS_LITERAL, ClassLiteral, TaggedObject::TaggedObjectSize(), ClassLiteral::SIZE, 2U, factory->NewClassLiteral());            
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
