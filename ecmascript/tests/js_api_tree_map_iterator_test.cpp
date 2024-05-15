/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/js_api/js_api_tree_map_iterator.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_tree_map.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/tagged_tree.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSAPITreeMapIteratorTest : public BaseTestWithScope<false> {
protected:
    JSHandle<JSAPITreeMap> CreateTreeMap()
    {
        return EcmaContainerCommon::CreateTreeMap(thread);
    }

    JSHandle<JSAPITreeMap> TestKeyValueCommon(std::vector<JSMutableHandle<JSTaggedValue>> &keyValue,
                                              std::vector<std::string> &mapKeyValue, uint32_t len,
                                              bool valueNext = false)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSAPITreeMap> jsTreeMap = CreateTreeMap();
        for (uint32_t i = 0; i < len; i++) {
            std::string ikey = mapKeyValue[0] + std::to_string(i);
            std::string ivalue = mapKeyValue[1] + std::to_string(valueNext ? i + 1U : i);
            keyValue[0].Update(factory->NewFromStdString(ikey).GetTaggedValue());
            keyValue[1].Update(factory->NewFromStdString(ivalue).GetTaggedValue());
            JSAPITreeMap::Set(thread, jsTreeMap, keyValue[0], keyValue[1]);
        }
        return jsTreeMap;
    }
};

/**
 * @tc.name: SetIterationKind
 * @tc.desc: Call the "SetIterationKind" function, check whether the result returned through "GetIterationKind"
 *           function from the JSAPITreeMapIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeMapIteratorTest, SetIterationKind)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPITreeMap> jsTreeMap = CreateTreeMap();
    EXPECT_TRUE(*jsTreeMap != nullptr);
    JSHandle<JSAPITreeMapIterator> treeMapIterator =
        factory->NewJSAPITreeMapIterator(jsTreeMap, IterationKind::KEY);
    EXPECT_EQ(treeMapIterator->GetIterationKind(), IterationKind::KEY);
    treeMapIterator->SetIterationKind(IterationKind::VALUE);
    EXPECT_EQ(treeMapIterator->GetIterationKind(), IterationKind::VALUE);
    treeMapIterator->SetIterationKind(IterationKind::KEY_AND_VALUE);
    EXPECT_EQ(treeMapIterator->GetIterationKind(), IterationKind::KEY_AND_VALUE);
}

/**
 * @tc.name: SetIteratedMap
 * @tc.desc: Call the "SetIteratedMap" function, check whether the result returned through "GetIteratedMap"
 *           function from the JSAPITreeMapIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeMapIteratorTest, SetIteratedMap)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPITreeMap> jsTreeMap = CreateTreeMap();
    EXPECT_TRUE(*jsTreeMap != nullptr);
    JSHandle<JSAPITreeMapIterator> treeMapIterator =
        factory->NewJSAPITreeMapIterator(jsTreeMap, IterationKind::VALUE);

    std::string mapKey("mapkey");
    std::string mapValue("mapvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ikey = mapKey + std::to_string(i);
        std::string ivalue = mapValue + std::to_string(i + 2U);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPITreeMap::Set(thread, jsTreeMap, key, value);
    }
    treeMapIterator->SetIteratedMap(thread, jsTreeMap.GetTaggedValue());
    JSHandle<JSAPITreeMap> treeMapTo(thread, JSAPITreeMap::Cast(treeMapIterator->GetIteratedMap().GetTaggedObject()));
    EXPECT_EQ(treeMapTo->GetSize(), static_cast<int>(DEFAULT_LENGTH));
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ikey = mapKey + std::to_string(i);
        std::string ivalue = mapValue + std::to_string(i + 2U);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSTaggedValue result = JSAPITreeMap::Get(thread, treeMapTo, key);
        EXPECT_EQ(result, value.GetTaggedValue());
    }
}

/**
 * @tc.name: SetNextIndex
 * @tc.desc: Call the "SetNextIndex" function, check whether the result returned through "GetNextIndex"
 *           function from the JSAPITreeMapIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeMapIteratorTest, SetNextIndex)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPITreeMap> jsTreeMap = CreateTreeMap();
    EXPECT_TRUE(*jsTreeMap != nullptr);
    JSHandle<JSAPITreeMapIterator> treeMapIterator =
        factory->NewJSAPITreeMapIterator(jsTreeMap, IterationKind::KEY_AND_VALUE);
    EXPECT_EQ(treeMapIterator->GetNextIndex(), 0U);

    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        treeMapIterator->SetNextIndex(i);
        EXPECT_EQ(treeMapIterator->GetNextIndex(), i);
    }
}

/**
 * @tc.name: CreateTreeMapIterator
 * @tc.desc: Create TreeMap iterator, check whether the result returned through "IsJSAPITreeMapIterator"
 *           function from the JSAPITreeMapIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeMapIteratorTest, CreateTreeMapIterator)
{
    JSHandle<JSAPITreeMap> jsTreeMap = CreateTreeMap();
    EXPECT_TRUE(*jsTreeMap != nullptr);
    JSHandle<JSTaggedValue> treeMapVal(jsTreeMap);
    // Create Iterator with KEY
    JSHandle<JSTaggedValue> treeMapIterator =
        JSAPITreeMapIterator::CreateTreeMapIterator(thread, treeMapVal, IterationKind::KEY);
    EXPECT_TRUE(*treeMapIterator != nullptr);
    EXPECT_TRUE(treeMapIterator->IsJSAPITreeMapIterator());
    // Create Iterator with VALUE
    treeMapIterator = JSAPITreeMapIterator::CreateTreeMapIterator(thread, treeMapVal, IterationKind::VALUE);
    EXPECT_TRUE(*treeMapIterator != nullptr);
    EXPECT_TRUE(treeMapIterator->IsJSAPITreeMapIterator());
    // Create Iterator with KEY_AND_VALUE
    treeMapIterator = JSAPITreeMapIterator::CreateTreeMapIterator(thread, treeMapVal, IterationKind::KEY_AND_VALUE);
    EXPECT_TRUE(*treeMapIterator != nullptr);
    EXPECT_TRUE(treeMapIterator->IsJSAPITreeMapIterator());
}

/**
 * @tc.name: Next
 * @tc.desc: Create an iterator of JSAPITreeMap,and then loop through the elements(key,value and keyAndvalue) of the
 *           iterator to check whether the elements through "Next" function are consistent.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeMapIteratorTest, KEY_Next)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::vector<JSMutableHandle<JSTaggedValue>> keyValue {key, value};
    std::string mapKey("mapkey");
    std::string mapValue("mapvalue");
    std::vector<std::string> mapKeyValue{mapKey, mapValue};
    JSHandle<JSAPITreeMap> jsTreeMap = TestKeyValueCommon(keyValue, mapKeyValue, DEFAULT_LENGTH);
    // Create Iterator with KEY
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPITreeMapIterator> treeMapKeyIterator =
        factory->NewJSAPITreeMapIterator(jsTreeMap, IterationKind::KEY);
    // traversal iterator
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(treeMapKeyIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPITreeMapIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        std::string resultKey = mapKey + std::to_string(i);
        keyValue[0].Update(factory->NewFromStdString(resultKey).GetTaggedValue());
        EXPECT_EQ(JSTaggedValue::SameValue(
            JSObject::GetProperty(thread, resultObj, valueStr).GetValue(), keyValue[0]), true);
        EXPECT_EQ(treeMapKeyIterator->GetNextIndex(), (i + 1U));
    }
}

HWTEST_F_L0(JSAPITreeMapIteratorTest, VALUE_Next)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    std::string mapKey("mapkey");
    std::string mapValue("mapvalue");
    std::vector<std::string> mapKeyValue{mapKey, mapValue};
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::vector<JSMutableHandle<JSTaggedValue>> keyValue {key, value};
    JSHandle<JSAPITreeMap> jsTreeMap = TestKeyValueCommon(keyValue, mapKeyValue, DEFAULT_LENGTH, true);
    // Create Iterator with VALUE
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPITreeMapIterator> treeMapKeyIterator =
        factory->NewJSAPITreeMapIterator(jsTreeMap, IterationKind::VALUE);
    // traversal iterator
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(treeMapKeyIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPITreeMapIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        std::string resultKey = mapValue + std::to_string(i + 1U);
        keyValue[1].Update(factory->NewFromStdString(resultKey).GetTaggedValue()); // 1 : value index
        EXPECT_EQ(JSTaggedValue::SameValue(
            JSObject::GetProperty(thread, resultObj, valueStr).GetValue(), keyValue[1]), true); // 1 : value index
        EXPECT_EQ(treeMapKeyIterator->GetNextIndex(), (i + 1U));
    }
}

HWTEST_F_L0(JSAPITreeMapIteratorTest, KEY_AND_VALUE_Next)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    JSHandle<JSAPITreeMap> jsTreeMap = CreateTreeMap();
    std::string mapKey("mapkey");
    std::string mapValue("mapvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ikey = mapKey + std::to_string(i + 1U);
        std::string ivalue = mapValue + std::to_string(i + 2U);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPITreeMap::Set(thread, jsTreeMap, key, value);
    }
    // Create Iterator with KEY_AND_VALUE
    JSHandle<JSAPITreeMapIterator> treeMapKeyIterator =
        factory->NewJSAPITreeMapIterator(jsTreeMap, IterationKind::KEY_AND_VALUE);
    // traversal iterator
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(treeMapKeyIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPITreeMapIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        std::string resultKeyAndValue = mapKey + std::to_string(i + 1U);
        value.Update(factory->NewFromStdString(resultKeyAndValue).GetTaggedValue());

        JSHandle<JSTaggedValue> keyValueArr(JSObject::GetProperty(thread, resultObj, valueStr).GetValue());
        for (int index = 0; index < 2; index++) {
            JSHandle<JSTaggedValue> indexValue(thread, JSTaggedValue(index));
            EXPECT_EQ(JSTaggedValue::SameValue(
                JSObject::GetProperty(thread, keyValueArr, indexValue).GetValue(), value), true);
            resultKeyAndValue = mapValue + std::to_string(i + 2U);
            value.Update(factory->NewFromStdString(resultKeyAndValue).GetTaggedValue());
        }
        EXPECT_EQ(treeMapKeyIterator->GetNextIndex(), (i + 1U));
    }
}

/**
 * @tc.name: Next and CreateTreeMapIterator
 * @tc.desc: test special return of Next and CreateTreeMapIterator,
 *           including throw exception and return undefined
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPITreeMapIteratorTest, SpecailReturnOfNextCreateTreeMapIterator)
{
    JSHandle<JSAPITreeMap> jsTreeMap = CreateTreeMap();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPITreeMapIterator> treeMapIterator = factory->NewJSAPITreeMapIterator(
        jsTreeMap, IterationKind::KEY_AND_VALUE);
    treeMapIterator->SetIteratedMap(thread, JSTaggedValue::Undefined());

    // test Next exception
    {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPITreeMapIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::Exception());
        EXPECT_EXCEPTION();
    }

    // test Next return undefined
    {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(treeMapIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPITreeMapIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, thread->GlobalConstants()->GetUndefinedIterResult());
    }

    // test CreateTreeMapIterator exception
    {
        JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
        JSHandle<JSTaggedValue> result =
            JSAPITreeMapIterator::CreateTreeMapIterator(thread, undefined, IterationKind::KEY_AND_VALUE);
        EXPECT_EQ(result.GetTaggedValue(), JSTaggedValue::Exception());
        EXPECT_EXCEPTION();
    }
}
}  // namespace panda::ecmascript
