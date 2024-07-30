/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/js_map_iterator.h"
#include "ecmascript/global_env.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_map.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSMapIteratorTest : public BaseTestWithScope<false> {
};

JSHandle<JSMap> CreateJSMap(JSThread *thread)
{
    JSHandle<GlobalEnv> globaEnv = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> builtinsMapFunc = globaEnv->GetBuiltinsMapFunction();
    JSHandle<JSMap> jsMap(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(builtinsMapFunc), builtinsMapFunc));
    JSHandle<JSTaggedValue> linkedHashMap(LinkedHashMap::Create(thread));
    jsMap->SetLinkedMap(thread, linkedHashMap);
    return jsMap;
}

/**
 * @tc.name: CreateSetIterator
 * @tc.desc: Call "CreateSetIterator" function create SetIterator,Check whether the the SetIterator is JSMapIterator
 *           through "IsJSMapIterator" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSMapIteratorTest, CreateMapIterator)
{
    JSHandle<JSMap> jsMap = CreateJSMap(thread);

    IterationKind iterKind = IterationKind::KEY;
    JSHandle<JSTaggedValue> mapIteratorKey =
        JSMapIterator::CreateMapIterator(thread, JSHandle<JSTaggedValue>::Cast(jsMap), iterKind);
    EXPECT_TRUE(mapIteratorKey->IsJSMapIterator());

    iterKind = IterationKind::VALUE;
    JSHandle<JSTaggedValue> mapIteratorValue =
        JSMapIterator::CreateMapIterator(thread, JSHandle<JSTaggedValue>::Cast(jsMap), iterKind);
    EXPECT_TRUE(mapIteratorValue->IsJSMapIterator());

    iterKind = IterationKind::KEY_AND_VALUE;
    JSHandle<JSTaggedValue> mapIteratorKeyAndValue =
        JSMapIterator::CreateMapIterator(thread, JSHandle<JSTaggedValue>::Cast(jsMap), iterKind);
    EXPECT_TRUE(mapIteratorKeyAndValue->IsJSMapIterator());
}

/**
 * @tc.name: SetIteratedMap
 * @tc.desc: Create a JSMapIterator through calling "CreateMapIterator" function with the current thread. Check whether
 *           the returned value through calling "GetIteratedMap" function from the JSMapIterator is the same with the
 *           returned value through calling "GetLinkedMap" function. Call "SetIteratedMap" function change LinkedMap,
 *           Check whether the returned JSTaggedValue through calling "GetNextIndex" function from the JSMapIterator is
 *           the same with the changed value.
 * source JSMap.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSMapIteratorTest, SetIteratedMap)
{
    IterationKind iterKind = IterationKind::KEY;
    JSHandle<JSMap> jsMap1 = CreateJSMap(thread);
    JSHandle<JSMap> jsMap2 = CreateJSMap(thread);
    JSHandle<JSTaggedValue> mapIteratorVal =
        JSMapIterator::CreateMapIterator(thread, JSHandle<JSTaggedValue>::Cast(jsMap1), iterKind);
    JSHandle<JSMapIterator> jsMapIterator = JSHandle<JSMapIterator>::Cast(mapIteratorVal);

    EXPECT_EQ(jsMap1->GetLinkedMap(), jsMapIterator->GetIteratedMap());
    jsMapIterator->SetIteratedMap(thread, JSHandle<JSTaggedValue>(thread, jsMap2->GetLinkedMap()));
    EXPECT_NE(jsMap1->GetLinkedMap(), jsMapIterator->GetIteratedMap());
    EXPECT_EQ(jsMap2->GetLinkedMap(), jsMapIterator->GetIteratedMap());
}

/**
 * @tc.name: SetNextIndex
 * @tc.desc: Create a JSMapIterator through calling "CreateMapIterator" function with the current thread, Check
 *           whether the returned value through calling "GetNextIndex" function from the JSMapIterator is the same
 *           as JSTaggedValue(0) Call "SetNextIndex" to change NextIndex, Check whether the returned value through
 *           calling "GetNextIndex" function from the JSMapIterator is the same with the changed value.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSMapIteratorTest, SetNextIndex)
{
    JSHandle<JSMap> jsMap = CreateJSMap(thread);
    JSHandle<JSTaggedValue> mapValue(jsMap);
    JSHandle<JSTaggedValue> mapIteratorVal =
        JSMapIterator::CreateMapIterator(thread, mapValue, IterationKind::KEY);
    JSHandle<JSMapIterator> mapIterator = JSHandle<JSMapIterator>::Cast(mapIteratorVal);

    EXPECT_EQ(mapIterator->GetNextIndex(), 0U);
    mapIterator->SetNextIndex(1);
    EXPECT_EQ(mapIterator->GetNextIndex(), 1U);
}

/**
 * @tc.name: SetIterationKind
 * @tc.desc: Create a JSMapIterator through calling CreateMapIterator function with the current thread. Check whether
 *           the returned value through calling "GetIterationKind" function is the same with JSTaggedValue(0). Check
 *           whether the returned value through calling "GetIterationKind" function is within expectations after calling
 *           "SetIterationKind" with the current thread.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSMapIteratorTest, SetIterationKind)
{
    JSHandle<JSMap> jsMap = CreateJSMap(thread);
    JSHandle<JSTaggedValue> mapValue(jsMap);
    JSHandle<JSTaggedValue> mapIteratorVal =
        JSMapIterator::CreateMapIterator(thread, mapValue, IterationKind::KEY);
    JSHandle<JSMapIterator> mapIterator = JSHandle<JSMapIterator>::Cast(mapIteratorVal);
    EXPECT_EQ(mapIterator->GetIterationKind(), IterationKind::KEY);
    mapIterator->SetIterationKind(IterationKind::VALUE);
    EXPECT_EQ(mapIterator->GetIterationKind(), IterationKind::VALUE);
    mapIterator->SetIterationKind(IterationKind::KEY_AND_VALUE);
    EXPECT_EQ(mapIterator->GetIterationKind(), IterationKind::KEY_AND_VALUE);
}

/**
 * @tc.name: Update
 * @tc.desc: Call "NewJSMapIterator" function create MapIterator with emty IteratedMap, create other JSMap and add key
 *           to it,the old JSMap call "Rehash" function set new JSMap to the next table, then SetIterator call "Update"
 *           function upadate IteratedMap, check whether the IteratedMap is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSMapIteratorTest, Update)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSMap> jsMap1(thread, CreateJSMap(thread).GetTaggedValue());
    JSHandle<JSMap> jsMap2(thread, CreateJSMap(thread).GetTaggedValue());

    JSHandle<JSTaggedValue> keyHandle1(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> keyHandle2(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> keyHandle3(thread, JSTaggedValue(2));  // 2: means key and value
    // add key and value to jsMap1
    JSMap::Set(thread, jsMap1, keyHandle1, keyHandle1);
    JSMap::Set(thread, jsMap1, keyHandle2, keyHandle2);
    JSMap::Set(thread, jsMap1, keyHandle3, keyHandle3);

    JSHandle<LinkedHashMap> mapHandle1(thread, LinkedHashMap::Cast(jsMap1->GetLinkedMap().GetTaggedObject()));
    JSHandle<LinkedHashMap> mapHandle2(thread, LinkedHashMap::Cast(jsMap2->GetLinkedMap().GetTaggedObject()));
    mapHandle1->Rehash(thread, *mapHandle2);
    // create SetIterator with jsMap1
    JSHandle<JSMapIterator> mapIterator = factory->NewJSMapIterator(jsMap1, IterationKind::KEY);
    // update MapIterator
    mapIterator->Update(thread);
    LinkedHashMap *resultMap = LinkedHashMap::Cast(mapIterator->GetIteratedMap().GetTaggedObject());
    EXPECT_TRUE(resultMap->Has(thread, keyHandle1.GetTaggedValue()));
    EXPECT_TRUE(resultMap->Has(thread, keyHandle2.GetTaggedValue()));
    EXPECT_TRUE(resultMap->Has(thread, keyHandle3.GetTaggedValue()));
}

/**
 * @tc.name: Next
 * @tc.desc: Calling "Next" function get the next value from the MapIterator,Check whether the return value obtained
 *           by the function is the next value in the array element.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSMapIteratorTest, KEY_VALUE_Next)
{
    JSHandle<JSMap> jsMap = CreateJSMap(thread);
    int32_t iteratorLength = 5;
    for (int32_t i = 0; i < iteratorLength; i++) {
        JSMap::Set(thread, jsMap, JSHandle<JSTaggedValue>(thread, JSTaggedValue(i)),
            JSHandle<JSTaggedValue>(thread, JSTaggedValue(i + 5)));  // 5: means expected value
    }
    JSHandle<JSTaggedValue> mapValue(jsMap);
    JSHandle<JSMapIterator> mapIterator(JSMapIterator::CreateMapIterator(thread, mapValue, IterationKind::KEY));
    // 4: argv length
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetThis(mapIterator.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    for (int32_t i = 0; i < 5; i++) {
        JSHandle<JSTaggedValue> nextTagValResult(thread, JSMapIterator::Next(ecmaRuntimeCallInfo1));
        EXPECT_EQ(JSIterator::IteratorValue(thread, nextTagValResult)->GetInt(), i);
    }
    TestHelper::TearDownFrame(thread, prev);
    // change Iterationkind to VALUE
    mapIterator->SetIterationKind(IterationKind::VALUE);
    mapIterator->SetNextIndex(0);
    // 4: argv length
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo2->SetThis(mapIterator.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    for (int32_t i = 0; i < iteratorLength; i++) {
        JSHandle<JSTaggedValue> nextTagValResult(thread, JSMapIterator::Next(ecmaRuntimeCallInfo2));
        EXPECT_EQ(JSIterator::IteratorValue(thread, nextTagValResult)->GetInt(), i + 5);  // 5: means expected value
    }
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(JSMapIteratorTest, KEY_AND_VALUE_Next)
{
    JSHandle<JSMap> jsMap = CreateJSMap(thread);
    int32_t iteratorLength = 5;
    for (int32_t i = 0; i < iteratorLength; i++) {
        JSMap::Set(thread, jsMap, JSHandle<JSTaggedValue>(thread, JSTaggedValue(i)),
            JSHandle<JSTaggedValue>(thread, JSTaggedValue(i + 10)));   // 10: means expected value
    }
    JSHandle<JSTaggedValue> mapValue(jsMap);
    JSHandle<JSMapIterator> mapIterator(
        JSMapIterator::CreateMapIterator(thread, mapValue, IterationKind::KEY_AND_VALUE));
    // 4: argv length
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetThis(mapIterator.GetTaggedValue());
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    for (int32_t i = 0; i < iteratorLength; i++) {
        JSHandle<JSTaggedValue> nextTagValResult(thread, JSMapIterator::Next(ecmaRuntimeCallInfo));
        JSHandle<JSArray> iteratorVal(thread, JSIterator::IteratorValue(thread, nextTagValResult).GetTaggedValue());
        JSHandle<TaggedArray> elementsIterVal(thread, iteratorVal->GetElements());
        EXPECT_EQ(i, elementsIterVal->Get(thread, 0).GetInt());
        EXPECT_EQ(i + 10, elementsIterVal->Get(thread, 1).GetInt());  // 10: means expected value
    }
    TestHelper::TearDownFrame(thread, prev);
}
}  // namespace panda::test
