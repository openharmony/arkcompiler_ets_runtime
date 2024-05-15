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

#include "ecmascript/ecma_string.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_lightweightmap.h"
#include "ecmascript/js_api/js_api_lightweightmap_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;

using namespace panda::ecmascript;

using namespace panda::ecmascript::containers;

namespace panda::test {
class JSAPILightWeightMapTest : public BaseTestWithScope<false> {
public:
    const static int DEFAULT_SIZE = 8;

protected:
    JSAPILightWeightMap *CreateLightWeightMap()
    {
        return EcmaContainerCommon::CreateLightWeightMap(thread);
    }

    JSHandle<JSAPILightWeightMap> TestCommon(JSMutableHandle<JSTaggedValue>& value, std::string& myValue,
        uint32_t numbers)
    {
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        std::string myKey("mykey");
        JSHandle<JSAPILightWeightMap> lwm(thread, CreateLightWeightMap());
        for (uint32_t i = 0; i < numbers; i++) {
            std::string ikey = myKey + std::to_string(i);
            std::string ivalue = myValue + std::to_string(i);
            key.Update(factory->NewFromStdString(ikey).GetTaggedValue());
            value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
            JSAPILightWeightMap::Set(thread, lwm, key, value);
            EXPECT_TRUE(JSAPILightWeightMap::GetIndexOfKey(thread, lwm, key) != -1);
            EXPECT_TRUE(JSAPILightWeightMap::GetIndexOfValue(thread, lwm, value) != -1);
            uint32_t length = lwm->GetLength();
            EXPECT_EQ(length, i + 1);
        }
        return lwm;
    }

    JSHandle<JSAPILightWeightMap> RemoveCommon(std::vector<JSHandle<JSTaggedValue>>& keys,
        std::vector<JSHandle<JSTaggedValue>>& values)
    {
        JSHandle<JSAPILightWeightMap> lwm(thread, CreateLightWeightMap());
        JSHandle<TaggedArray> valueArray(thread, JSTaggedValue(TaggedArray::Cast(lwm->GetValues().GetTaggedObject())));

        for (int i = 1; i <= 3; i++) {  // 3: key value count; 1: start key
            JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
            JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i + 1));
            JSAPILightWeightMap::Set(thread, lwm, key, value);
            keys.push_back(key);
            values.push_back(value);
        }
        return lwm;
    }
};

HWTEST_F_L0(JSAPILightWeightMapTest, LightWeightMapCreate)
{
    JSAPILightWeightMap *lightWeightMap = CreateLightWeightMap();
    EXPECT_TRUE(lightWeightMap != nullptr);
}

HWTEST_F_L0(JSAPILightWeightMapTest, SetHasKeyGetHasValue)
{
    JSAPILightWeightMap *lightWeightMap = CreateLightWeightMap();

    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(2));
    JSHandle<JSAPILightWeightMap> lwm(thread, lightWeightMap);
    JSAPILightWeightMap::Set(thread, lwm, key, value);
    EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread,
        JSAPILightWeightMap::Get(thread, lwm, key)), value));

    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(3));
    JSAPILightWeightMap::Set(thread, lwm, key1, value1);

    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(3));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(4));
    JSAPILightWeightMap::Set(thread, lwm, key2, value2);

     // test species
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> key3 = env->GetSpeciesSymbol();
    JSHandle<JSTaggedValue> value3(thread, JSTaggedValue(5));
    JSAPILightWeightMap::Set(thread, lwm, key3, value3);

    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(10));
    JSHandle<JSTaggedValue> value4(thread, JSTaggedValue(10));
    JSAPILightWeightMap::Set(thread, lwm, key4, value4);
    EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread,
        JSAPILightWeightMap::Get(thread, lwm, key4)), value4));

    // change value on Existed key
    JSHandle<JSTaggedValue> value5(thread, JSTaggedValue(100));
    JSAPILightWeightMap::Set(thread, lwm, key4, value5);
    EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread,
        JSAPILightWeightMap::Get(thread, lwm, key4)), value5));

    EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread,
        JSAPILightWeightMap::Get(thread, lwm, key)), value));

    EXPECT_EQ(JSAPILightWeightMap::HasKey(thread, lwm, key1), JSTaggedValue::True());
    EXPECT_EQ(JSAPILightWeightMap::HasKey(thread, lwm, key), JSTaggedValue::True());
    EXPECT_EQ(JSAPILightWeightMap::HasKey(thread, lwm, key3), JSTaggedValue::True());
    EXPECT_EQ(JSAPILightWeightMap::HasKey(thread, lwm, value), JSTaggedValue::True());
}

HWTEST_F_L0(JSAPILightWeightMapTest, GetIndexOfKeyAndGetIndexOfValue)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::string myValue("myvalue");
    auto lwm = TestCommon(value, myValue, NODE_NUMBERS);
    std::string ivalue = myValue + std::to_string(NODE_NUMBERS);
    value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
    EXPECT_TRUE(JSAPILightWeightMap::GetIndexOfValue(thread, lwm, value) == -1);
}

HWTEST_F_L0(JSAPILightWeightMapTest, IsEmptyGetKeyAtGetValue)
{
    JSHandle<JSAPILightWeightMap> lwm(thread, CreateLightWeightMap());

    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(2));
    JSAPILightWeightMap::Set(thread, lwm, key, value);

    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(3));
    JSAPILightWeightMap::Set(thread, lwm, key1, value1);

    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(3));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(4));
    JSAPILightWeightMap::Set(thread, lwm, key2, value2);

    JSHandle<JSTaggedValue> result =
        JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::GetValueAt(thread, lwm, 0));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, value));
    result = JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::GetValueAt(thread, lwm, 1));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, value1));
    result = JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::GetValueAt(thread, lwm, 2));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, value2));

    result = JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::GetKeyAt(thread, lwm, 0));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, key));
    result = JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::GetKeyAt(thread, lwm, 1));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, key1));
    result = JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::GetKeyAt(thread, lwm, 2));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, key2));

    EXPECT_EQ(lwm->IsEmpty(), JSTaggedValue::False());
    JSAPILightWeightMap::Clear(thread, lwm);
    EXPECT_EQ(lwm->IsEmpty(), JSTaggedValue::True());
}

HWTEST_F_L0(JSAPILightWeightMapTest, Remove)
{
    std::vector<JSHandle<JSTaggedValue>> keys{};
    std::vector<JSHandle<JSTaggedValue>> values{};
    auto lwm = RemoveCommon(keys, values);

    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(4));

    JSHandle<JSTaggedValue> result =
        JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::Remove(thread, lwm, keys[2])); // 2 : key index
    JSHandle<JSTaggedValue> resultNoExist =
        JSHandle<JSTaggedValue>(thread, JSAPILightWeightMap::Remove(thread, lwm, key3));
    EXPECT_TRUE(JSTaggedValue::Equal(thread, result, values[2])); // 2 : value index
    bool isKeyExist = true;
    if (resultNoExist->IsUndefined()) {
        isKeyExist = false;
    }
    EXPECT_FALSE(isKeyExist);
}

HWTEST_F_L0(JSAPILightWeightMapTest, RemoveAt)
{
    std::vector<JSHandle<JSTaggedValue>> keys{};
    std::vector<JSHandle<JSTaggedValue>> values{};
    auto lwm = RemoveCommon(keys, values);
    int32_t removeIndex = JSAPILightWeightMap::GetIndexOfKey(thread, lwm, keys[1]); // 1 : key
    EXPECT_EQ(JSAPILightWeightMap::RemoveAt(thread, lwm, removeIndex), JSTaggedValue::True());
    JSHandle<JSTaggedValue> result(thread, JSAPILightWeightMap::Get(thread, lwm, keys[1])); // 1 : key
    bool isSuccessRemove = false;
    if (result->IsUndefined()) {
        isSuccessRemove = true;
    }
    EXPECT_TRUE(isSuccessRemove);
    EXPECT_EQ(JSAPILightWeightMap::HasValue(thread, lwm, values[1]), JSTaggedValue::False());
    EXPECT_TRUE(lwm->GetLength() == 2);

    EXPECT_EQ(JSAPILightWeightMap::RemoveAt(thread, lwm, -1), JSTaggedValue::False());
    EXPECT_EQ(JSAPILightWeightMap::RemoveAt(thread, lwm, 10), JSTaggedValue::False());
}

HWTEST_F_L0(JSAPILightWeightMapTest, SetValueAt)
{
    JSHandle<JSAPILightWeightMap> lwm(thread, CreateLightWeightMap());

    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(2));
    JSAPILightWeightMap::Set(thread, lwm, key, value);
    EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread,
        JSAPILightWeightMap::Get(thread, lwm, key)), value));

    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(3));
    JSAPILightWeightMap::Set(thread, lwm, key1, value1);

    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(3));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(4));
    JSAPILightWeightMap::Set(thread, lwm, key2, value2);

    JSHandle<JSTaggedValue> value3(thread, JSTaggedValue(5));

    int32_t index = JSAPILightWeightMap::GetIndexOfKey(thread, lwm, key);
    JSAPILightWeightMap::SetValueAt(thread, lwm, index, value3);
    EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread,
        JSAPILightWeightMap::Get(thread, lwm, key)), value3));
}

HWTEST_F_L0(JSAPILightWeightMapTest, GetStateOfKey)
{
    JSHandle<JSAPILightWeightMap> lwm(thread, CreateLightWeightMap());

    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(1));
    JSAPILightWeightMap::Set(thread, lwm, key1, value1);
    KeyState keyState1 = JSAPILightWeightMap::GetStateOfKey(thread, lwm, key1);
    EXPECT_TRUE(keyState1.existed);

    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    KeyState keyState2 = JSAPILightWeightMap::GetStateOfKey(thread, lwm, key2);
    EXPECT_FALSE(keyState2.existed);

    // hash Collision
    std::vector<double> setVector = {0.0, 1224.0, 1285.0, 1463.0, 4307.0, 5135.0,
                                     5903.0, 6603.0, 6780.0, 8416.0, 9401.0, 9740.0};
    for (uint32_t i = 0; i < setVector.size() - 1; i++) {
        JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(setVector[i]));
        JSHandle<JSTaggedValue> value3(thread, JSTaggedValue(setVector[i]));
        JSAPILightWeightMap::Set(thread, lwm, key3, value3);
    }

    // check
    for (uint32_t i = 0; i < setVector.size() - 1; i++) {
        JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(setVector[i]));
        KeyState keyState4 = JSAPILightWeightMap::GetStateOfKey(thread, lwm, key4);
        EXPECT_TRUE(keyState4.existed);
    }
    JSHandle<JSTaggedValue> key5(thread, JSTaggedValue(setVector[setVector.size() - 1]));
    KeyState keyState5 = JSAPILightWeightMap::GetStateOfKey(thread, lwm, key5);
    EXPECT_FALSE(keyState5.existed);

    JSHandle<JSTaggedValue> key6(thread, JSTaggedValue(0));
    KeyState keyState6 = JSAPILightWeightMap::GetStateOfKey(thread, lwm, key6);
    EXPECT_TRUE(keyState6.existed);
}

HWTEST_F_L0(JSAPILightWeightMapTest, IncreaseCapacityTo)
{
    constexpr uint32_t NODE_NUMBERS = 10;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myValue("myvalue");
    auto lwm = TestCommon(value, myValue, NODE_NUMBERS);
    EXPECT_EQ(JSAPILightWeightMap::IncreaseCapacityTo(thread, lwm, 15), JSTaggedValue::True());
    EXPECT_EQ(JSAPILightWeightMap::IncreaseCapacityTo(thread, lwm, 9), JSTaggedValue::False());
}

HWTEST_F_L0(JSAPILightWeightMapTest, Iterator)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILightWeightMap> lwm(thread, CreateLightWeightMap());

    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        key.Update(JSTaggedValue(i));
        value.Update(JSTaggedValue(i + 1));
        JSAPILightWeightMap::Set(thread, lwm, key, value);
    }

    // test key or value
    JSHandle<JSTaggedValue> keyIter(factory->NewJSAPILightWeightMapIterator(lwm, IterationKind::KEY));
    JSHandle<JSTaggedValue> valueIter(factory->NewJSAPILightWeightMapIterator(lwm, IterationKind::VALUE));
    JSMutableHandle<JSTaggedValue> keyIterResult(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> valueIterResult(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> keyHandle(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        keyIterResult.Update(JSIterator::IteratorStep(thread, keyIter).GetTaggedValue());
        valueIterResult.Update(JSIterator::IteratorStep(thread, valueIter).GetTaggedValue());
        JSTaggedValue k = JSIterator::IteratorValue(thread, keyIterResult).GetTaggedValue();
        keyHandle.Update(k);
        JSTaggedValue v = JSIterator::IteratorValue(thread, valueIterResult).GetTaggedValue();
        EXPECT_EQ(JSAPILightWeightMap::HasKey(thread, lwm, keyHandle), JSTaggedValue::True());
        EXPECT_EQ(JSAPILightWeightMap::Get(thread, lwm, keyHandle), v);
    }

    // test key and value
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        JSTaggedValue k = JSTaggedValue(i);
        JSTaggedValue v = JSTaggedValue(i + 1);
        keyHandle.Update(k);
        EXPECT_EQ(JSAPILightWeightMap::HasKey(thread, lwm, keyHandle), JSTaggedValue::True());
        EXPECT_EQ(JSAPILightWeightMap::Get(thread, lwm, keyHandle), v);
    }
}

HWTEST_F_L0(JSAPILightWeightMapTest, IsEmptyHasValueHasAll)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILightWeightMap> lwp(thread, CreateLightWeightMap());
    JSHandle<JSAPILightWeightMap> hasAllLwp(thread, CreateLightWeightMap());
    JSMutableHandle<JSTaggedValue> key1(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value1(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value2(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value3(thread, JSTaggedValue::Undefined());

    std::string tValue;
    // test IsEmpty
    EXPECT_EQ(lwp->IsEmpty(), JSTaggedValue::True());
    // test Set
    std::string myKey1("mykey");
    std::string myValue1("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey1 = myKey1 + std::to_string(i);
        std::string iValue1 = myValue1 + std::to_string(i + 1);
        key1.Update(factory->NewFromStdString(iKey1).GetTaggedValue());
        value1.Update(factory->NewFromStdString(iValue1).GetTaggedValue());
        JSAPILightWeightMap::Set(thread, lwp, key1, value1);
    }
    EXPECT_EQ(lwp->GetLength(), NODE_NUMBERS);

    // test HasValue
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        tValue = myValue1 + std::to_string(i + 1);
        value1.Update(factory->NewFromStdString(tValue).GetTaggedValue());
        EXPECT_EQ(JSAPILightWeightMap::HasValue(thread, lwp, value1), JSTaggedValue::True());
    }
    tValue = myValue1 + std::to_string(NODE_NUMBERS + 1);
    value1.Update(factory->NewFromStdString(tValue).GetTaggedValue());
    EXPECT_EQ(JSAPILightWeightMap::HasValue(thread, lwp, value1), JSTaggedValue::False());

    // test HasAll
    for (uint32_t i = 0; i < NODE_NUMBERS - 5; i++) {
        if (i == 1) {
            std::string mykey2("destKey");
            std::string myValue2("destValue");
            std::string iKey2 = mykey2 + std::to_string(i);
            std::string iValue2 = myValue2 + std::to_string(i);
            key1.Update(factory->NewFromStdString(iKey2).GetTaggedValue());
            value1.Update(factory->NewFromStdString(iValue2).GetTaggedValue());
            JSAPILightWeightMap::Set(thread, hasAllLwp, key1, value1);
        } else {
            std::string iKey = myKey1 + std::to_string(i);
            std::string iValue = myValue1 + std::to_string(i + 1);
            key1.Update(factory->NewFromStdString(iValue).GetTaggedValue());
            value1.Update(factory->NewFromStdString(iValue).GetTaggedValue());
            JSAPILightWeightMap::Set(thread, hasAllLwp, key1, value2);
        }
    }
    EXPECT_EQ(hasAllLwp->GetLength(), NODE_NUMBERS - 5);
    EXPECT_EQ(JSAPILightWeightMap::HasAll(thread, lwp, hasAllLwp), JSTaggedValue::False());
    EXPECT_EQ(JSAPILightWeightMap::HasAll(thread, hasAllLwp, lwp), JSTaggedValue::False());
}

/**
 * @tc.name: GetIteratorObj
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPILightWeightMapTest, GetIteratorObj)
{
    JSHandle<JSAPILightWeightMap> lwp(thread, CreateLightWeightMap());
    JSHandle<JSTaggedValue> iteratorObj(thread, JSAPILightWeightMap::GetIteratorObj(
        thread, lwp, IterationKind::KEY_AND_VALUE));
    EXPECT_TRUE(iteratorObj->IsJSAPILightWeightMapIterator());
}
}  // namespace panda::test
