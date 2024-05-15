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

#include "ecmascript/js_api/js_api_lightweightset.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_lightweightset_iterator.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSAPILightWeightSetTest : public BaseTestWithScope<false> {
protected:
    JSAPILightWeightSet *CreateLightWeightSet()
    {
        return EcmaContainerCommon::CreateLightWeightSet(thread);
    }
};

HWTEST_F_L0(JSAPILightWeightSetTest, LightWeightSetCreate)
{
    JSAPILightWeightSet *lightweightSet = CreateLightWeightSet();
    EXPECT_TRUE(lightweightSet != nullptr);
}

HWTEST_F_L0(JSAPILightWeightSetTest, AddIncreaseCapacityAddAll)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    JSHandle<JSAPILightWeightSet> lws(thread, CreateLightWeightSet());
    JSHandle<JSAPILightWeightSet> srcLws(thread, CreateLightWeightSet());
    JSHandle<JSAPILightWeightSet> destLws(thread, CreateLightWeightSet());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test IncreaseCapacityTo
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        bool result = JSAPILightWeightSet::Add(thread, lws, JSHandle<JSTaggedValue>(thread, JSTaggedValue(i)));
        EXPECT_TRUE(result);
    }
    EXPECT_EQ(lws->GetSize(), NODE_NUMBERS);

    uint32_t tmp = NODE_NUMBERS * 2; // 2 means the value
    JSAPILightWeightSet::IncreaseCapacityTo(thread, lws, static_cast<int32_t>(tmp));
    uint32_t capacity = TaggedArray::Cast(lws->GetValues().GetTaggedObject())->GetLength();
    EXPECT_EQ(JSTaggedValue(capacity), JSTaggedValue(tmp));

    // test IncreaseCapacityTo exception
    JSAPILightWeightSet::IncreaseCapacityTo(thread, lws, 0);
    EXPECT_EXCEPTION();
    JSAPILightWeightSet::IncreaseCapacityTo(thread, lws, NODE_NUMBERS);
    EXPECT_EXCEPTION();

    // test AddAll
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        bool result = JSAPILightWeightSet::Add(thread, destLws, JSHandle<JSTaggedValue>(thread, JSTaggedValue(i)));
        EXPECT_TRUE(result);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS + 2; i++) {
        JSAPILightWeightSet::Add(thread, srcLws, JSHandle<JSTaggedValue>(thread, JSTaggedValue(i)));
    }
    bool result = JSAPILightWeightSet::AddAll(thread, destLws, JSHandle<JSTaggedValue>::Cast(srcLws));
    EXPECT_TRUE(result);
    tmp = NODE_NUMBERS + 2; // 2 means the value
    EXPECT_EQ(destLws->GetSize(), tmp);
}

HWTEST_F_L0(JSAPILightWeightSetTest, EqualClearNotEqual)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILightWeightSet> lws(thread, CreateLightWeightSet());
    JSHandle<JSAPILightWeightSet> equalLws(thread, CreateLightWeightSet());
    JSHandle<JSTaggedValue> jsArray = JSArray::ArrayCreate(thread, JSTaggedNumber(0));
    JSMutableHandle<JSTaggedValue> value1(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value2(thread, JSTaggedValue::Undefined());
    bool result = false;

    // test Equal of two empty lightweightset
    result = JSAPILightWeightSet::Equal(thread, lws, JSHandle<JSTaggedValue>::Cast(equalLws));
    EXPECT_FALSE(result);
    result = JSAPILightWeightSet::Equal(thread, lws, jsArray);
    EXPECT_FALSE(result);

    // test equal
    std::string myValue1("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue1 + std::to_string(i);
        value1.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        result = JSAPILightWeightSet::Add(thread, lws, value1);
        EXPECT_TRUE(result);
    }
    EXPECT_EQ(lws->GetSize(), NODE_NUMBERS);

    std::string myValue2("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue2 + std::to_string(i);
        value2.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        result = JSAPILightWeightSet::Add(thread, equalLws, value2);
        EXPECT_TRUE(result);
    }
    EXPECT_EQ(equalLws->GetSize(), NODE_NUMBERS);
    result = JSAPILightWeightSet::Equal(thread, lws, JSHandle<JSTaggedValue>::Cast(equalLws));
    EXPECT_FALSE(result);

    equalLws->Clear(thread);
    EXPECT_EQ(equalLws->GetSize(), static_cast<uint32_t>(0)); // 0 means the value

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue2 + std::to_string(i);
        if (i == 2) {
            LOG_ECMA(ERROR) << " {} " << iValue;
        } else {
            value2.Update(factory->NewFromStdString(iValue).GetTaggedValue());
            result = JSAPILightWeightSet::Add(thread, equalLws, value2);
            EXPECT_TRUE(result);
        }
    }
    EXPECT_EQ(equalLws->GetSize(), NODE_NUMBERS - 1);
    result = JSAPILightWeightSet::Equal(thread, lws, JSHandle<JSTaggedValue>::Cast(equalLws));
    EXPECT_FALSE(result);
}

HWTEST_F_L0(JSAPILightWeightSetTest, IsEmptyHasHasAll)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILightWeightSet> lws(thread, CreateLightWeightSet());
    JSHandle<JSAPILightWeightSet> hasAllLws(thread, CreateLightWeightSet());
    JSMutableHandle<JSTaggedValue> value1(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value2(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value3(thread, JSTaggedValue::Undefined());
    bool result = false;
    std::string tValue;
    // test IsEmpty
    result = lws->IsEmpty();
    EXPECT_TRUE(result);
    // test Has
    std::string myValue1("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue1 + std::to_string(i);
        value1.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        result = JSAPILightWeightSet::Add(thread, lws, value1);
        EXPECT_TRUE(result);
    }
    EXPECT_EQ(lws->GetSize(), NODE_NUMBERS);

    tValue = myValue1 + std::to_string(5);
    value1.Update(factory->NewFromStdString(tValue).GetTaggedValue());
    result = lws->Has(thread, value1);
    EXPECT_TRUE(result);
    tValue = myValue1 + std::to_string(NODE_NUMBERS);
    value1.Update(factory->NewFromStdString(tValue).GetTaggedValue());
    result = lws->Has(thread, value1);
    EXPECT_FALSE(result);

    std::string myValue2("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS - 5; i++) {
        if (i == 1) {
            std::string myValue3("destValue");
            std::string iValue3 = myValue3 + std::to_string(i);
            value3.Update(factory->NewFromStdString(iValue3).GetTaggedValue());
            result = JSAPILightWeightSet::Add(thread, hasAllLws, value3);
        } else {
            std::string iValue = myValue2 + std::to_string(i);
            value2.Update(factory->NewFromStdString(iValue).GetTaggedValue());
            result = JSAPILightWeightSet::Add(thread, hasAllLws, value2);
            EXPECT_TRUE(result);
        }
    }
    EXPECT_EQ(hasAllLws->GetSize(), NODE_NUMBERS - 5); // 5 means the value
    result = lws->HasAll(JSHandle<JSTaggedValue>::Cast(hasAllLws));
    EXPECT_FALSE(result);
    result = hasAllLws->HasAll(JSHandle<JSTaggedValue>::Cast(lws));
    EXPECT_FALSE(result);
}

HWTEST_F_L0(JSAPILightWeightSetTest, GetIndexOfRemoveRemoveAtGetValueAt)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILightWeightSet> lws(thread, CreateLightWeightSet());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    bool result = false;

    // test GetSize
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        result = JSAPILightWeightSet::Add(thread, lws, value);
    }
    EXPECT_EQ(lws->GetSize(), NODE_NUMBERS);

    // test GetIndexOf
    std::string tValue("myvalue5");
    value.Update(factory->NewFromStdString(tValue).GetTaggedValue());
    int32_t index = lws->GetIndexOf(thread, value);
    EXPECT_EQ(index, 5); // 5 means the value

    // test GetValueAt
    JSTaggedValue jsValue = lws->GetValueAt(5); // 5 means the value
    EXPECT_EQ(value.GetTaggedValue(), jsValue);

    // test Remove
    jsValue = lws->Remove(thread, value);
    EXPECT_EQ(value.GetTaggedValue(), jsValue);
    jsValue = lws->Remove(thread, value);
    EXPECT_EQ(jsValue, JSTaggedValue::Undefined());

    // test RemoveAt
    result = lws->RemoveAt(thread, 4); // 4 means the value
    EXPECT_EQ(lws->GetSize(), NODE_NUMBERS - 2); // 2 means the value
    EXPECT_TRUE(result);
    result = lws->RemoveAt(thread, -1);
    EXPECT_FALSE(result);
    result = lws->RemoveAt(thread, static_cast<int32_t>(NODE_NUMBERS));
    EXPECT_FALSE(result);
}

HWTEST_F_L0(JSAPILightWeightSetTest, Iterator)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILightWeightSet> lws(thread, CreateLightWeightSet());

    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i));
        JSAPILightWeightSet::Add(thread, lws, value);
    }

    JSHandle<JSTaggedValue> valueIter(factory->NewJSAPILightWeightSetIterator(lws, IterationKind::VALUE));
    JSMutableHandle<JSTaggedValue> valueIterResult(thread, JSTaggedValue::Undefined());
    for (int i = 0; i < static_cast<int>(NODE_NUMBERS); i++) {
        valueIterResult.Update(JSIterator::IteratorStep(thread, valueIter).GetTaggedValue());
        int v = JSIterator::IteratorValue(thread, valueIterResult)->GetInt();
        EXPECT_TRUE(v == i);
    }
}

HWTEST_F_L0(JSAPILightWeightSetTest, RBTreeGetHashIndex)
{
    std::vector<int> hashCollisionVector = {4307, 5135, 5903, 6603, 6780, 8416, 1224, 1285, 1463, 9401, 9740};
    uint32_t NODE_NUMBERS = static_cast<uint32_t>(hashCollisionVector.size());
    JSHandle<JSAPILightWeightSet> lws(thread, CreateLightWeightSet());

    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(hashCollisionVector[i]));
        JSAPILightWeightSet::Add(thread, lws, value);
    }
    int32_t size = static_cast<uint32_t>(lws->GetLength());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(hashCollisionVector[i]));
        int32_t index = lws->GetHashIndex(thread, value, size);
        EXPECT_TRUE(0 <= index && index < size);
    }
}

HWTEST_F_L0(JSAPILightWeightSetTest, SpecialReturnTestEnsureCapacityGetValueAtGetHashAt)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPILightWeightSet> lws(thread, CreateLightWeightSet());

    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i));
        JSAPILightWeightSet::Add(thread, lws, value);
    }

    // test special return of EnsureCapacity
    JSHandle<TaggedArray> array(thread, lws->GetValues());
    JSAPILightWeightSet::EnsureCapacity(thread, lws, 0);
    JSHandle<TaggedArray> newArray(thread, lws->GetValues());
    EXPECT_TRUE(array->GetLength() == newArray->GetLength());

    // test special return of GetValueAt
    JSTaggedValue result1 = lws->GetValueAt(-1);
    EXPECT_EQ(result1, JSTaggedValue::Undefined());
    JSTaggedValue result2 = lws->GetValueAt(static_cast<int32_t>(NODE_NUMBERS * 2));
    EXPECT_EQ(result2, JSTaggedValue::Undefined());

    // test special return of GetHashAt
    JSTaggedValue result3 = lws->GetHashAt(-1);
    EXPECT_EQ(result3, JSTaggedValue::Undefined());
    JSTaggedValue result4 = lws->GetHashAt(static_cast<int32_t>(NODE_NUMBERS * 2));
    EXPECT_EQ(result4, JSTaggedValue::Undefined());
}

HWTEST_F_L0(JSAPILightWeightSetTest, GetHashAtHasHash)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPILightWeightSet> lws(thread, CreateLightWeightSet());

    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> hash(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i));
        JSAPILightWeightSet::Add(thread, lws, value);
    }

    // test GetHashAt
    int32_t size = static_cast<int32_t>(lws->GetLength());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        hash.Update(JSTaggedValue(lws->Hash(thread, JSTaggedValue(i))));
        int32_t index = lws->GetHashIndex(thread, hash, size);
        JSTaggedValue getHash= lws->GetHashAt(index);
        EXPECT_EQ(getHash, hash.GetTaggedValue());
    }

    // test HasHash
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        hash.Update(JSTaggedValue(lws->Hash(thread, JSTaggedValue(i))));
        EXPECT_TRUE(lws->HasHash(hash));
    }
    hash.Update(JSTaggedValue(lws->Hash(thread, JSTaggedValue(NODE_NUMBERS))));
    EXPECT_FALSE(lws->HasHash(hash));
}

HWTEST_F_L0(JSAPILightWeightSetTest, ToString)
{
    constexpr uint32_t NODE_NUMBERS = 3;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPILightWeightSet> lws(thread, CreateLightWeightSet());
    JSTaggedValue result1 = JSAPILightWeightSet::ToString(thread, lws);
    JSHandle<EcmaString> resultHandle1(thread, result1);
    JSHandle<EcmaString> det = thread->GetEcmaVM()->GetFactory()->NewFromASCII("");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, resultHandle1, det), 0);
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        value.Update(JSTaggedValue(i));
        JSAPILightWeightSet::Add(thread, lws, value);
    }
    JSHandle<EcmaString> str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0,1,2");
    JSTaggedValue result = JSAPILightWeightSet::ToString(thread, lws);
    JSHandle<EcmaString> resultHandle(thread, result);
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, resultHandle, str), 0);
}
}  // namespace panda::test
