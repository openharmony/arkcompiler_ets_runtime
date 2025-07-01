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

#include "ecmascript/js_api/js_api_plain_array.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_plain_array_iterator.h"
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
class JSAPIPlainArrayTest : public BaseTestWithScope<false> {
protected:
    JSAPIPlainArray *CreatePlainArray()
    {
        return EcmaContainerCommon::CreatePlainArray(thread);
    }
    JSHandle<JSAPIPlainArray> GetIndexOfKeyAndGeIndexOfValueGetArray(JSMutableHandle<JSTaggedValue>& value,
        uint32_t numbers)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());

        JSHandle<JSAPIPlainArray> array(thread, CreatePlainArray());
        EXPECT_TRUE(array->IsEmpty());
        std::string myValue("myvalue");
        for (uint32_t i = 0; i < numbers; i++) {
            uint32_t ikey = 100 + i;
            std::string ivalue = myValue + std::to_string(i);
            key.Update(JSTaggedValue(ikey));
            value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
            JSAPIPlainArray::Add(thread, array, key, value);
        }
        EXPECT_EQ(array->GetSize(), static_cast<int>(numbers));
        EXPECT_FALSE(array->IsEmpty());
        return array;
    }

    JSHandle<JSAPIPlainArray> PropertyCommon(uint32_t elementsNums)
    {
        JSHandle<JSAPIPlainArray> plainArray(thread, CreatePlainArray());
        for (uint32_t i = 0; i < elementsNums; i++) {
            JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
            JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
            JSAPIPlainArray::Add(thread, plainArray, key, value);
        }
        return plainArray;
    }
};

HWTEST_F_L0(JSAPIPlainArrayTest, PlainArrayCreate)
{
    JSAPIPlainArray *plainArray = CreatePlainArray();
    EXPECT_TRUE(plainArray != nullptr);
}

HWTEST_F_L0(JSAPIPlainArrayTest, PA_AddAndGetKeyAtAndClear)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test JSAPIPlainArray
    JSHandle<JSAPIPlainArray> array(thread, CreatePlainArray());
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i = i + 2) {
        uint32_t ikey = 100 + i;
        std::string ivalue = myValue + std::to_string(i);

        key.Update(JSTaggedValue(ikey));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        JSAPIPlainArray::Add(thread, array, key, value);
    }
    for (uint32_t i = 1; i < NODE_NUMBERS; i = i + 2) {
        uint32_t ikey = 100 + i;
        std::string ivalue = myValue + std::to_string(i);

        key.Update(JSTaggedValue(ikey));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        JSAPIPlainArray::Add(thread, array, key, value);
    }
    EXPECT_EQ(array->GetSize(), static_cast<int>(NODE_NUMBERS));

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        uint32_t ikey = 100 + i;
        std::string ivalue = myValue + std::to_string(i);
        key.Update(JSTaggedValue(ikey));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        // test getKeyAt
        JSTaggedValue gvalue = array->GetKeyAt(thread, i);
        EXPECT_EQ(gvalue, key.GetTaggedValue());
    }
    EXPECT_EQ(array->GetKeyAt(thread, -1), JSTaggedValue::Undefined());
    EXPECT_EQ(array->GetKeyAt(thread, NODE_NUMBERS), JSTaggedValue::Undefined());
    // test clear
    array->Clear(thread);
    EXPECT_EQ(array->GetSize(), 0); // 0 means the value
}

HWTEST_F_L0(JSAPIPlainArrayTest, PA_CloneAndHasAndGet)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test JSAPIPlainArray
    JSHandle<JSAPIPlainArray> array(thread, CreatePlainArray());
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        uint32_t ikey = 100 + i;
        std::string ivalue = myValue + std::to_string(i);
        key.Update(JSTaggedValue(ikey));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIPlainArray::Add(thread, array, key, value);
    }
    EXPECT_EQ(array->GetSize(), static_cast<int>(NODE_NUMBERS));

    // test clone
    JSHandle<JSAPIPlainArray> newArray(thread, CreatePlainArray());
    EXPECT_EQ(newArray->GetSize(), 0); // 0 means the value
    newArray = JSAPIPlainArray::Clone(thread, array);
    EXPECT_EQ(newArray->GetSize(), static_cast<int>(NODE_NUMBERS));

    // test has
    key.Update(JSTaggedValue(103)); // 103 means the value
    int32_t lkey = 103;
    bool result = array->Has(thread, lkey);
    EXPECT_TRUE(result);
    EXPECT_FALSE(array->Has(thread, lkey * 2));

    // test get
    myValue = std::string("myvalue3");
    value.Update(factory->NewFromStdString(myValue).GetTaggedValue());
    EXPECT_TRUE(
        JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread, array->Get(thread, key.GetTaggedValue())), value));
}

HWTEST_F_L0(JSAPIPlainArrayTest, PA_GetIndexOfKeyAndGeIndexOfValueAndIsEmptyAndRemoveRangeFrom)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    auto array = GetIndexOfKeyAndGeIndexOfValueGetArray(value, NODE_NUMBERS);
    value.Update(JSTaggedValue(103)); // 103 means the value
    int32_t lvalue = 103;
    JSTaggedValue value2 = array->GetIndexOfKey(thread, lvalue);
    EXPECT_EQ(value2.GetNumber(), 3); // 3 means the value
    EXPECT_EQ(array->GetIndexOfKey(thread, lvalue * 2), JSTaggedValue(-1));

    std::string myValue = "myvalue2";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    value.Update(factory->NewFromStdString(myValue).GetTaggedValue());
    JSTaggedValue value3 = array->GetIndexOfValue(thread, value.GetTaggedValue());
    EXPECT_EQ(value3.GetNumber(), 2); // 2 means the value
    EXPECT_EQ(array->GetIndexOfValue(thread, JSTaggedValue(0)), JSTaggedValue(-1));

    value.Update(JSTaggedValue(1));
    int32_t batchSize = 3; // 3 means the value
    lvalue = 1;
    value3 = array->RemoveRangeFrom(thread, lvalue, batchSize);
    EXPECT_EQ(value3.GetNumber(), 3); // 3 means the value
    EXPECT_EQ(array->GetSize(), static_cast<int>(NODE_NUMBERS - 3));

    // test RemoveRangeFrom exception
    array->RemoveRangeFrom(thread, lvalue, -1);
}

HWTEST_F_L0(JSAPIPlainArrayTest, PA_RemvoeAnrRemvoeAtAndSetValueAtAndGetValueAt)
{
    constexpr uint32_t NODE_NUMBERS = 8; // 8 means the value
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    auto array = GetIndexOfKeyAndGeIndexOfValueGetArray(value, NODE_NUMBERS);
    // test Remove
    std::string myValue = "myvalue2";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    value.Update(factory->NewFromStdString(myValue).GetTaggedValue());
    JSTaggedValue taggedValue =
        array->Remove(thread, JSTaggedValue(102)); // 102 means the value
    EXPECT_TRUE(JSTaggedValue::Equal(thread, value, JSHandle<JSTaggedValue>(thread, taggedValue)));
    EXPECT_EQ(array->Remove(thread, JSTaggedValue(-1)), JSTaggedValue::Undefined());
    EXPECT_EQ(array->Remove(thread, JSTaggedValue(100 + NODE_NUMBERS)), JSTaggedValue::Undefined());

    // test RemoveAt
    myValue = "myvalue4";
    value.Update(factory->NewFromStdString(myValue).GetTaggedValue());
    taggedValue =
        array->RemoveAt(thread, JSTaggedValue(3)); // 3 means the value
    EXPECT_TRUE(JSTaggedValue::Equal(thread, value, JSHandle<JSTaggedValue>(thread, taggedValue)));
    EXPECT_EQ(array->GetSize(), static_cast<int>(NODE_NUMBERS - 2));
    EXPECT_EQ(array->RemoveAt(thread, JSTaggedValue(-1)), JSTaggedValue::Undefined());
    EXPECT_EQ(array->RemoveAt(thread, JSTaggedValue(NODE_NUMBERS)), JSTaggedValue::Undefined());

    // test SetValueAt
    myValue = "myvalue14";
    value.Update(factory->NewFromStdString(myValue).GetTaggedValue());
    array->SetValueAt(thread, JSTaggedValue(3), value.GetTaggedValue()); // 3 means the value
    int32_t lvalue = 3; // 3 means the value
    taggedValue = array->GetValueAt(thread, lvalue);
    EXPECT_TRUE(JSTaggedValue::Equal(thread, value, JSHandle<JSTaggedValue>(thread, taggedValue)));
}

HWTEST_F_L0(JSAPIPlainArrayTest, PA_GetOwnProperty)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIPlainArray> toor(thread, CreatePlainArray());
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    std::string plainArrayvalue("plainArrayvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        uint32_t ikey = 100 + i;
        std::string ivalue = plainArrayvalue + std::to_string(i);
        key.Update(JSTaggedValue(ikey));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIPlainArray::Add(thread, toor, key, value);
    }
    // test GetOwnProperty
    int testInt = 100 + 1;
    JSHandle<JSTaggedValue> plainArrayKey1(thread, JSTaggedValue(testInt));
    EXPECT_TRUE(JSAPIPlainArray::GetOwnProperty(thread, toor, plainArrayKey1));
    testInt = 100 + 20;
    JSHandle<JSTaggedValue> plainArrayKey2(thread, JSTaggedValue(testInt));
    EXPECT_FALSE(JSAPIPlainArray::GetOwnProperty(thread, toor, plainArrayKey2));
}

HWTEST_F_L0(JSAPIPlainArrayTest, PA_ToString)
{
    constexpr uint32_t NODE_NUMBERS = 3; // 3 means the value
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    // test JSAPIPlainArray
    JSHandle<JSAPIPlainArray> array(thread, CreatePlainArray());
    JSTaggedValue result1 = JSAPIPlainArray::ToString(thread, array);
    JSHandle<EcmaString> resultHandle1(thread, result1);
    JSHandle<EcmaString> det = thread->GetEcmaVM()->GetFactory()->NewFromASCII("");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, resultHandle1, det), 0);
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        uint32_t ikey = i;
        std::string ivalue = std::to_string(i);
        key.Update(JSTaggedValue(ikey));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIPlainArray::Add(thread, array, key, value);
    }
    JSHandle<EcmaString> str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0:0,1:1,2:2");
    JSTaggedValue result = JSAPIPlainArray::ToString(thread, array);
    JSHandle<EcmaString> resultHandle(thread, result);

    ASSERT_EQ(EcmaStringAccessor::Compare(instance, resultHandle, str), 0);
}

/**
 * @tc.name: GetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIPlainArrayTest, GetProperty)
{
    uint32_t elementsNums = 8;
    auto plainArray = PropertyCommon(elementsNums);
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        OperationResult getPropertyRes = JSAPIPlainArray::GetProperty(thread, plainArray, key);
        EXPECT_EQ(getPropertyRes.GetValue().GetTaggedValue(), JSTaggedValue(i));
    }
}

/**
 * @tc.name: SetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIPlainArrayTest, SetProperty)
{
    uint32_t elementsNums = 8;
    auto plainArray = PropertyCommon(elementsNums);
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i * 2)); // 2 : It means double
        bool setPropertyRes = JSAPIPlainArray::SetProperty(thread, plainArray, key, value);
        EXPECT_EQ(setPropertyRes, true);
    }
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(-1));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(-1));
    EXPECT_FALSE(JSAPIPlainArray::SetProperty(thread, plainArray, key, value));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(elementsNums));
    EXPECT_FALSE(JSAPIPlainArray::SetProperty(thread, plainArray, key1, value));
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(int64_t(-9007199254740992))); // Out-of-Bounds test
    EXPECT_FALSE(JSAPIPlainArray::SetProperty(thread, plainArray, key1, value));
}
}  // namespace panda::test
