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

#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_vector.h"
#include "ecmascript/js_api/js_api_vector_iterator.h"
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
class JSAPIVectorTest : public BaseTestWithScope<false> {
protected:
    JSAPIVector *CreateVector()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        auto result = TestCommon::CreateContainerTaggedValue(thread, containers::ContainerTag::Vector);
        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIVector> vector(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        vector->SetLength(0);
        return *vector;
    }

    JSHandle<JSAPIVector> TestCommon(JSMutableHandle<JSTaggedValue>& value, std::string& myValue, uint32_t nums)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSAPIVector> toor(thread, CreateVector());

        for (uint32_t i = 0; i < nums; i++) {
            std::string ivalue = myValue + std::to_string(i);
            value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
            bool result = JSAPIVector::Add(thread, toor, value);
            EXPECT_TRUE(result);
            EXPECT_EQ(toor->IsEmpty(), false);
        }
        return toor;
    }
};

HWTEST_F_L0(JSAPIVectorTest, vectorCreate)
{
    JSAPIVector *vector = CreateVector();
    EXPECT_TRUE(vector != nullptr);
}

HWTEST_F_L0(JSAPIVectorTest, AddGetHas)
{
    constexpr uint32_t NODE_NUMBERS = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIVector> toor(thread, CreateVector());

    // test Has of empty vector
    value.Update(JSTaggedValue(NODE_NUMBERS));
    EXPECT_FALSE(toor->Has(thread, value.GetTaggedValue()));

    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        bool result = JSAPIVector::Add(thread, toor, value);
        EXPECT_TRUE(result);
        EXPECT_EQ(JSAPIVector::Get(thread, toor, i), value.GetTaggedValue());
        EXPECT_TRUE(toor->Has(thread, value.GetTaggedValue()));
    }
    value.Update(JSTaggedValue(NODE_NUMBERS));
    EXPECT_FALSE(toor->Has(thread, value.GetTaggedValue()));
    EXPECT_EQ(static_cast<uint32_t>(toor->GetSize()), NODE_NUMBERS);

    // test Get exception
    JSAPIVector::Get(thread, toor, -1);
    EXPECT_EXCEPTION();
    JSAPIVector::Get(thread, toor, static_cast<int32_t>(NODE_NUMBERS));
    EXPECT_EXCEPTION();

    toor->Dump(thread);
}

HWTEST_F_L0(JSAPIVectorTest, RemoveByIndexAndRemove)
{
    constexpr uint32_t NODE_NUMBERS = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    std::string myValue("myvalue");
    auto toor = TestCommon(value, myValue, NODE_NUMBERS);

    for (int32_t i = NODE_NUMBERS / 2; i > 0; i--) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSTaggedValue gValue = JSAPIVector::RemoveByIndex(thread, toor, i);
        EXPECT_EQ(gValue, value.GetTaggedValue());
        bool delResult = JSAPIVector::Remove(thread, toor, value);
        EXPECT_FALSE(delResult);
    }

    // test RemoveByIndex exception
    JSTaggedValue result = JSAPIVector::RemoveByIndex(thread, toor, -1);
    EXPECT_EQ(result, JSTaggedValue::Exception());
    EXPECT_EXCEPTION();
    JSTaggedValue result1 = JSAPIVector::RemoveByIndex(thread, toor, NODE_NUMBERS);
    EXPECT_EQ(result1, JSTaggedValue::Exception());
    EXPECT_EXCEPTION();

    toor->Dump(thread);
}

HWTEST_F_L0(JSAPIVectorTest, RemoveByRangeAndGetHeapUsedSize)
{
    uint32_t NODE_NUMBERS = 9;
    uint32_t fromIndex = 1;
    uint32_t toIndex = 3;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    std::string myValue("myvalue");
    auto toor = TestCommon(value, myValue, NODE_NUMBERS);
    JSAPIVector::RemoveByRange(thread, toor, fromIndex, toIndex);
    // test heap is normal after RemoveByRange, GetLiveObjectSize will iterate heap
    instance->GetHeap()->GetLiveObjectSize();
    EXPECT_EQ(toor->GetLength(), static_cast<int32_t>(NODE_NUMBERS - (toIndex - fromIndex)));
}

HWTEST_F_L0(JSAPIVectorTest, ClearAndisEmpty)
{
    constexpr uint32_t NODE_NUMBERS = 9;
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    std::string myValue("myvalue");
    auto toor = TestCommon(value, myValue, NODE_NUMBERS);

    JSAPIVector::Clear(thread, toor);
    EXPECT_EQ(toor->IsEmpty(), true);

    toor->Dump(thread);
}

HWTEST_F_L0(JSAPIVectorTest, GetIndexOf)
{
    constexpr uint32_t NODE_NUMBERS = 9;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());

    JSHandle<JSAPIVector> toor(thread, CreateVector());

    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        bool result = JSAPIVector::Add(thread, toor, value);
        EXPECT_TRUE(result);
        EXPECT_EQ(JSAPIVector::GetIndexOf(thread, toor, value), static_cast<int>(i));
    }

    toor->Dump(thread);
}

HWTEST_F_L0(JSAPIVectorTest, GetOwnProperty)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPIVector> toor(thread, CreateVector());

    std::string vectorvalue("vectorvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = vectorvalue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIVector::Add(thread, toor, value);
    }
    // test GetOwnProperty
    int testInt = 1;
    JSHandle<JSTaggedValue> vectorKey1(thread, JSTaggedValue(testInt));
    EXPECT_TRUE(JSAPIVector::GetOwnProperty(thread, toor, vectorKey1));
    testInt = 20;
    JSHandle<JSTaggedValue> vectorKey2(thread, JSTaggedValue(testInt));
    EXPECT_FALSE(JSAPIVector::GetOwnProperty(thread, toor, vectorKey2));
    EXPECT_EXCEPTION();

    // test GetOwnProperty exception
    JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
    EXPECT_FALSE(JSAPIVector::GetOwnProperty(thread, toor, undefined));
    EXPECT_EXCEPTION();
}

/**
 * @tc.name: GetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIVectorTest, GetProperty)
{
    JSHandle<JSAPIVector> toor(thread, CreateVector());
    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIVector::Add(thread, toor, value);
    }
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        OperationResult getPropertyRes = JSAPIVector::GetProperty(thread, toor, key);
        EXPECT_EQ(getPropertyRes.GetValue().GetTaggedValue(), JSTaggedValue(i));
    }

    // test GetProperty exception
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(-1));
    JSAPIVector::GetProperty(thread, toor, key1);
    EXPECT_EXCEPTION();
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(elementsNums));
    JSAPIVector::GetProperty(thread, toor, key2);
    EXPECT_EXCEPTION();
}

/**
 * @tc.name: SetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIVectorTest, SetProperty)
{
    JSHandle<JSAPIVector> toor(thread, CreateVector());
    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIVector::Add(thread, toor, value);
    }
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i * 2)); // 2 : It means double
        bool setPropertyRes = JSAPIVector::SetProperty(thread, toor, key, value);
        EXPECT_EQ(setPropertyRes, true);
    }
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(-1));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(0));
    EXPECT_FALSE(JSAPIVector::SetProperty(thread, toor, key, value));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(elementsNums));
    EXPECT_FALSE(JSAPIVector::SetProperty(thread, toor, key1, value));
}

/**
 * @tc.name: TrimToCurrentLength
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIVectorTest, IncreaseCapacityToTrimToCurrentLength)
{
    JSHandle<JSAPIVector> toor(thread, CreateVector());
    uint32_t elementsNums = 20;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIVector::Add(thread, toor, value);
    }
    JSAPIVector::IncreaseCapacityTo(thread, toor, 80);
    JSHandle<TaggedArray> elementData(thread, toor->GetElements(thread));
    EXPECT_EQ(static_cast<int>(elementData->GetLength()), 80);
    JSAPIVector::TrimToCurrentLength(thread, toor);
    JSHandle<TaggedArray> newElementData(thread, toor->GetElements(thread));
    EXPECT_EQ(newElementData->GetLength(), elementsNums);

    // test IncreaseCapacityTo exception
    JSAPIVector::IncreaseCapacityTo(thread, toor, -1);
    EXPECT_EXCEPTION();
}

/**
 * @tc.name: Insert
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIVectorTest, Insert)
{
    JSHandle<JSAPIVector> toor(thread, CreateVector());
    uint32_t elementsNums = 20;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIVector::Insert(thread, toor, value, 0);
    }

    // check
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        EXPECT_EQ(JSAPIVector::Get(
            thread, toor, static_cast<int32_t>(i)), JSTaggedValue(elementsNums - i - 1));
    }

    // test Insert exception
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(elementsNums));
    JSAPIVector::Insert(thread, toor, value, -1);
    EXPECT_EXCEPTION();

    JSAPIVector::Insert(thread, toor, value, static_cast<int32_t>(elementsNums + 1));
    EXPECT_EXCEPTION();
}

/**
 * @tc.name: SetLength, GetIndexFrom, GetLastElement, GetLastIndexOf and GetLastIndexFrom
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIVectorTest, SetLengthGetIndexFromGetLastElementGetLastIndexOf)
{
    JSHandle<JSAPIVector> toor(thread, CreateVector());

    // test GetLastElement of empty vector
    EXPECT_EQ(toor->GetLastElement(thread), JSTaggedValue::Undefined());

    // test GetLastIndexOf of empty vector
    uint32_t elementsNums = 20;
    JSHandle<JSTaggedValue> obj(thread, JSTaggedValue(elementsNums - 1));
    EXPECT_EQ(JSAPIVector::GetLastIndexOf(thread, toor, obj), -1);

    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIVector::Add(thread, toor, value);
    }

    // test GetIndexFrom
    EXPECT_EQ(JSAPIVector::GetIndexFrom(thread, toor, obj, -1), static_cast<int32_t>(elementsNums - 1));

    EXPECT_EQ(JSAPIVector::GetIndexFrom(thread, toor, obj, elementsNums), -1);
    EXPECT_EXCEPTION();

    // test GetLastElement
    EXPECT_EQ(toor->GetLastElement(thread), JSTaggedValue(elementsNums - 1));

    // test GetLastIndexOf
    EXPECT_EQ(JSAPIVector::GetLastIndexOf(thread, toor, obj), static_cast<int32_t>(elementsNums - 1));

    // test GetLastIndexFrom
    EXPECT_EQ(JSAPIVector::GetLastIndexFrom(
        thread, toor, obj, elementsNums - 1), static_cast<int32_t>(elementsNums - 1));

    EXPECT_EQ(JSAPIVector::GetLastIndexFrom(thread, toor, obj, elementsNums), -1);
    EXPECT_EXCEPTION();

    JSHandle<JSTaggedValue> obj1(thread, JSTaggedValue(-elementsNums));
    EXPECT_EQ(JSAPIVector::GetLastIndexFrom(thread, toor, obj1, -1), -1);

    // test SetLength
    JSAPIVector::SetLength(thread, toor, elementsNums * 3);
    EXPECT_EQ(toor->GetLength(), static_cast<int32_t>(elementsNums * 3));
}

/**
 * @tc.name: RemoveByRange
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIVectorTest, ExceptionOfRemoveByRange)
{
    JSHandle<JSAPIVector> toor(thread, CreateVector());
    uint32_t elementsNums = 20;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIVector::Add(thread, toor, value);
    }

    // test RemoveByRange exception
    // toIndex <= fromIndex
    JSTaggedValue result1 = JSAPIVector::RemoveByRange(thread, toor, 0, 0);
    EXPECT_EQ(result1, JSTaggedValue::Exception());
    EXPECT_EXCEPTION();

    // from < 0
    JSTaggedValue result2 = JSAPIVector::RemoveByRange(thread, toor, -1, 0);
    EXPECT_EQ(result2, JSTaggedValue::Exception());
    EXPECT_EXCEPTION();

    // fromIndex >= length
    JSTaggedValue result3 = JSAPIVector::RemoveByRange(thread, toor, elementsNums, elementsNums * 2);
    EXPECT_EQ(result3, JSTaggedValue::Exception());
    EXPECT_EXCEPTION();
}

/**
 * @tc.name: SubVector
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIVectorTest, ExceptionOfSubVector)
{
    JSHandle<JSAPIVector> toor(thread, CreateVector());
    uint32_t elementsNums = 20;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIVector::Add(thread, toor, value);
    }

    // test SubVector exception
    // from < 0
    JSAPIVector::SubVector(thread, toor, -1, 0);
    EXPECT_EXCEPTION();

    // toIndex < 0
    JSAPIVector::SubVector(thread, toor, 0, -1);
    EXPECT_EXCEPTION();

    // fromIndex >= length
    JSAPIVector::SubVector(thread, toor, elementsNums, 0);
    EXPECT_EXCEPTION();

    // ToIndex >= length
    JSAPIVector::SubVector(thread, toor, 0, elementsNums);
    EXPECT_EXCEPTION();

    // toIndex <= fromIndex
    JSAPIVector::SubVector(thread, toor, elementsNums - 1, 0);
    EXPECT_EXCEPTION();
}

/**
 * @tc.name: OwnKeys
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIVectorTest, OwnKeys)
{
    JSHandle<JSAPIVector> toor(thread, CreateVector());
    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIVector::Add(thread, toor, value);
    }
    JSHandle<TaggedArray> keyArray = JSAPIVector::OwnKeys(thread, toor);
    EXPECT_TRUE(keyArray->GetClass()->IsTaggedArray());
    EXPECT_TRUE(keyArray->GetLength() == elementsNums);
    for (uint32_t i = 0; i < elementsNums; i++) {
        ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread,
            *(base::NumberHelper::NumberToString(thread, JSTaggedValue(i))),
            EcmaString::Cast(keyArray->Get(thread, i).GetTaggedObject())
        ));
    }
}

/**
 * @tc.name: GetFirstElement
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIVectorTest, GetFirstElement)
{
    JSHandle<JSAPIVector> toor(thread, CreateVector());
    EXPECT_EQ(JSAPIVector::GetFirstElement(thread, toor), JSTaggedValue::Undefined());

    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIVector::Add(thread, toor, value);
    }
    EXPECT_EQ(JSAPIVector::GetFirstElement(thread, toor), JSTaggedValue(0));
}
}  // namespace panda::test
