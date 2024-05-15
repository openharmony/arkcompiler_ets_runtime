/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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
#ifndef ECMA_CONTAINER_COMMON_H
#define ECMA_CONTAINER_COMMON_H
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/global_env.h"
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
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/tests/test_common.h"

namespace panda::test {

using namespace panda;
using namespace panda::ecmascript;
using ecmascript::base::BuiltinsBase;

constexpr uint32_t g_defaultSize = 8;

class EcmaContainerCommon {
public:
    static JSObject *JSObjectTestCreate(JSThread *thread)
    {
        [[maybe_unused]] ecmascript::EcmaHandleScope scope(thread);
        EcmaVM *ecmaVM = thread->GetEcmaVM();
        auto globalEnv = ecmaVM->GetGlobalEnv();
        JSHandle<JSTaggedValue> jsFunc = globalEnv->GetObjectFunction();
        JSHandle<JSObject> newObj =
            ecmaVM->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(jsFunc), jsFunc);
        return *newObj;
    }

    static JSAPIArrayList *CreateArrayList(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::ArrayList);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIArrayList> arrayList(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<TaggedArray> taggedArray = factory->NewTaggedArray(JSAPIArrayList::DEFAULT_CAPACITY_LENGTH);
        arrayList->SetElements(thread, taggedArray);
        return *arrayList;
    }

    static JSAPIPlainArray *CreatePlainArray(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::PlainArray);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIPlainArray> plainArray(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<JSTaggedValue> keyArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(g_defaultSize));
        JSHandle<JSTaggedValue> valueArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(g_defaultSize));
        plainArray->SetKeys(thread, keyArray);
        plainArray->SetValues(thread, valueArray);
        return *plainArray;
    }

    static JSAPIDeque *CreateJSApiDeque(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::Deque);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIDeque> deque(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<TaggedArray> newElements = factory->NewTaggedArray(JSAPIDeque::DEFAULT_CAPACITY_LENGTH);
        deque->SetElements(thread, newElements);
        return *deque;
    }

    static JSAPIHashMap *CreateHashMap(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::HashMap);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIHashMap> map(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSTaggedValue hashMapArray = TaggedHashArray::Create(thread);
        map->SetTable(thread, hashMapArray);
        map->SetSize(0);
        return *map;
    }

    static JSAPIHashSet *CreateHashSet(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::HashSet);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIHashSet> set(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSTaggedValue hashSetArray = TaggedHashArray::Create(thread);
        set->SetTable(thread, hashSetArray);
        set->SetSize(0);
        return *set;
    }

    static JSAPILightWeightMap *CreateLightWeightMap(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::LightWeightMap);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPILightWeightMap> lightWeightMap = JSHandle<JSAPILightWeightMap>::Cast(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<JSTaggedValue> hashArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(g_defaultSize));
        JSHandle<JSTaggedValue> keyArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(g_defaultSize));
        JSHandle<JSTaggedValue> valueArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(g_defaultSize));
        lightWeightMap->SetHashes(thread, hashArray);
        lightWeightMap->SetKeys(thread, keyArray);
        lightWeightMap->SetValues(thread, valueArray);
        lightWeightMap->SetLength(0);
        return *lightWeightMap;
    }

    static JSAPILightWeightSet *CreateLightWeightSet(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::LightWeightSet);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPILightWeightSet> lightweightSet = JSHandle<JSAPILightWeightSet>::Cast(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<JSTaggedValue> hashArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(g_defaultSize));
        JSHandle<JSTaggedValue> valueArray = JSHandle<JSTaggedValue>(factory->NewTaggedArray(g_defaultSize));
        lightweightSet->SetHashes(thread, hashArray);
        lightweightSet->SetValues(thread, valueArray);
        lightweightSet->SetLength(0);  // 0 means the value
        return *lightweightSet;
    }

    static JSAPILinkedList *CreateLinkedList(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::LinkedList);

        JSHandle<JSTaggedValue> contianer(thread, result);
        JSHandle<JSAPILinkedList> linkedList = JSHandle<JSAPILinkedList>::Cast(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(contianer), contianer));
        JSTaggedValue doubleList = TaggedDoubleList::Create(thread);
        linkedList->SetDoubleList(thread, doubleList);
        return *linkedList;
    }

    static JSAPIList *CreateList(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::List);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIList> list(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSTaggedValue singleList = TaggedSingleList::Create(thread);
        list->SetSingleList(thread, singleList);
        return *list;
    }

    static JSHandle<JSAPIQueue> CreateQueue(JSThread *thread, int capacaty)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::Queue);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIQueue> jsQueue(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<TaggedArray> newElements = factory->NewTaggedArray(capacaty);
        jsQueue->SetElements(thread, newElements);
        jsQueue->SetLength(thread, JSTaggedValue(0));
        jsQueue->SetFront(0);
        jsQueue->SetTail(0);
        return jsQueue;
    }

    static JSHandle<JSAPIStack> CreateJSApiStack(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::Stack);

        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIStack> jsStack(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        jsStack->SetTop(-1);
        return jsStack;
    }

    static JSHandle<JSAPITreeMap> CreateTreeMap(JSThread *thread)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::TreeMap);

        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPITreeMap> jsTreeMap(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSTaggedValue internal = TaggedTreeMap::Create(thread);
        jsTreeMap->SetTreeMap(thread, internal);
        return jsTreeMap;
    }
};
};
#endif
