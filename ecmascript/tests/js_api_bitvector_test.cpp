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

#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_api/js_api_bitvector.h"
#include "ecmascript/global_env.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/containers/containers_errors.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSAPIBitVectorTest : public testing::Test {
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
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};

    class TestClass : public base::BuiltinsBase {
    public:
        static std::pair<uint32_t, uint32_t> ComputeElementIdAndBitId(uint32_t index)
        {
            uint32_t elementId = index >> 6;
            uint32_t bitId = index & 0x3FULL;
            return std::make_pair(elementId, bitId);
        }
        static JSTaggedValue GetBit(std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *elements, uint32_t index)
        {
            std::pair<uint32_t, uint32_t> pair = ComputeElementIdAndBitId(index);
            uint32_t elementId = pair.first;
            uint32_t bitId = pair.second;
            int32_t bit = (*elements)[elementId].test(bitId);
            return JSTaggedValue(bit);
        }
    };
protected:
    JSAPIBitVector *CreateBitVector()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::BitVector)));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = containers::ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIBitVector> bitVector(
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        auto *newBitSetVector = new std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>>();
        auto deleter = []([[maybe_unused]] void *env, void *pointer, [[maybe_unused]] void *data) {
            if (pointer == nullptr) {
                return;
            }
            delete reinterpret_cast<std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *>(pointer);
        };
        JSHandle<JSNativePointer> pointer = factory->NewSJSNativePointer(newBitSetVector, deleter, newBitSetVector);
        bitVector->SetNativePointer(thread, pointer);
        return *bitVector;
    }
};

HWTEST_F_L0(JSAPIBitVectorTest, CreateBitVector)
{
    JSAPIBitVector *bitVector = CreateBitVector();
    EXPECT_TRUE(bitVector != nullptr);
}

/**
 * @tc.name: Push
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, Push)
{
    uint32_t increasedLength = 5;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements =
        reinterpret_cast<std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *>(np->GetExternalPointer());
    for (uint32_t i = 0; i < increasedLength; i++) {
        EXPECT_EQ(TestClass::GetBit(elements, i), JSTaggedValue(1));
    }
}

/**
 * @tc.name: Pop
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, Pop)
{
    uint32_t increasedLength = 5;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSTaggedValue res = JSAPIBitVector::Pop(thread, bitVector);
        EXPECT_EQ(res, JSTaggedValue(1));
    }
}

/**
 * @tc.name: Set
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, Set)
{
    uint32_t increasedLength = 5;
    uint32_t index = 3;

    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(0));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements =
        reinterpret_cast<std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *>(np->GetExternalPointer());
    JSTaggedValue res = bitVector->Set(thread, index, JSTaggedValue(1));
    EXPECT_EQ(res, JSTaggedValue::Undefined());
    EXPECT_EQ(TestClass::GetBit(elements, index), JSTaggedValue(1));
}

/**
 * @tc.name: Get
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, Get)
{
    uint32_t increasedLength = 5;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSTaggedValue res = bitVector->Get(thread, i);
        EXPECT_EQ(res, JSTaggedValue(1));
    }
}

/**
 * @tc.name: Has
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, Has)
{
    uint32_t increasedLength = 5;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(5));
    bool res = JSAPIBitVector::Has(thread, bitVector, value, value1, value2);
    EXPECT_TRUE(res);
}

/**
 * @tc.name: Has_instance
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, Has_instance)
{
    uint32_t increasedLength = 5;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    bool res = bitVector->Has(JSTaggedValue(1));
    EXPECT_TRUE(res);
}

/**
 * @tc.name: Has_Testboundary
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, Has_Testboundary)
{
    uint32_t increasedLength = 8;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(7));
    bool res = JSAPIBitVector::Has(thread, bitVector, value, value, value1);
    EXPECT_FALSE(res);
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value3(thread, JSTaggedValue(8));
    JSAPIBitVector::Push(thread, bitVector, value2);
    res = JSAPIBitVector::Has(thread, bitVector, value2, value, value3);
    EXPECT_TRUE(res);
}

/**
 * @tc.name: SetBitsByRange
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, SetBitsByRange)
{
    uint32_t increasedLength = 5;
    uint32_t startIndex = 1;
    uint32_t endIndex = 4;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(0));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(startIndex));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(endIndex));
    JSTaggedValue res = JSAPIBitVector::SetBitsByRange(thread, bitVector, value, value1, value2);
    EXPECT_EQ(res, JSTaggedValue::Undefined());

    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements =
        reinterpret_cast<std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *>(np->GetExternalPointer());
    for (uint32_t i = startIndex; i < endIndex; i++) {
        EXPECT_EQ(TestClass::GetBit(elements, i), JSTaggedValue(1));
    }
}

/**
 * @tc.name: GetBitsByRange
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, GetBitsByRange)
{
    uint32_t increasedLength = 5;
    uint32_t startIndex = 1;
    uint32_t endIndex = 4;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(startIndex));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(endIndex));
    JSTaggedValue res = JSAPIBitVector::GetBitsByRange(thread, bitVector, value1, value2);
    JSHandle<JSAPIBitVector> getBitVector(thread, res);
    EXPECT_EQ(getBitVector->GetLength(), endIndex - startIndex);
    for (uint32_t i = 0; i < endIndex - startIndex; i++) {
        EXPECT_EQ(getBitVector->Get(thread, i), JSTaggedValue(1));
    }
}

/**
 * @tc.name: SetAllBits
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, SetAllBits)
{
    uint32_t increasedLength = 5;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(0));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSTaggedValue res = JSAPIBitVector::SetAllBits(thread, bitVector, value);
    EXPECT_EQ(res, JSTaggedValue::Undefined());
    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements =
        reinterpret_cast<std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *>(np->GetExternalPointer());
    for (uint32_t i = 0; i < increasedLength; i++) {
        EXPECT_EQ(TestClass::GetBit(elements, i), JSTaggedValue(1));
    }
}

/**
 * @tc.name: GetBitCountByRange
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, GetBitCountByRange)
{
    uint32_t increasedLength = 5;
    uint32_t startIndex = 1;
    uint32_t endIndex = 4;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(startIndex));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(endIndex));
    JSTaggedValue res = JSAPIBitVector::GetBitCountByRange(thread, bitVector, value, value1, value2);
    EXPECT_EQ(res, JSTaggedValue(3));
}

/**
 * @tc.name: GetIndexOf
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, GetIndexOf)
{
    uint32_t increasedLength = 5;
    uint32_t startIndex = 1;
    uint32_t endIndex = 4;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(startIndex));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(endIndex));
    int res = JSAPIBitVector::GetIndexOf(thread, bitVector, value, value1, value2);
    EXPECT_EQ(res, startIndex);
}

/**
 * @tc.name: GetLastIndexOf
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, GetLastIndexOf)
{
    uint32_t increasedLength = 5;
    uint32_t startIndex = 1;
    uint32_t endIndex = 4;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(startIndex));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(endIndex));
    int res = JSAPIBitVector::GetLastIndexOf(thread, bitVector, value, value1, value2);
    EXPECT_EQ(res, endIndex - 1);
}

/**
 * @tc.name: FlipBitByIndex
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, FlipBitByIndex)
{
    uint32_t increasedLength = 5;
    uint32_t index = 3;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    JSTaggedValue res = JSAPIBitVector::FlipBitByIndex(thread, bitVector, index);

    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements =
        reinterpret_cast<std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *>(np->GetExternalPointer());
    EXPECT_EQ(res, JSTaggedValue::Undefined());
    EXPECT_EQ(TestClass::GetBit(elements, index), JSTaggedValue(0));
}

/**
 * @tc.name: FlipBitsByRange
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, FlipBitsByRange)
{
    uint32_t increasedLength = 5;
    uint32_t startIndex = 1;
    uint32_t endIndex = 4;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(startIndex));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(endIndex));
    JSTaggedValue res = JSAPIBitVector::FlipBitsByRange(thread, bitVector, value1, value2);
    EXPECT_EQ(res, JSTaggedValue::Undefined());

    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements =
        reinterpret_cast<std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *>(np->GetExternalPointer());
    for (uint32_t i = startIndex; i < endIndex; i++) {
        EXPECT_EQ(TestClass::GetBit(elements, i), JSTaggedValue(0));
    }
}

/**
 * @tc.name: Resize
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, Resize)
{
    uint32_t increasedLength = 5;
    int newLength = 10;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }

    JSAPIBitVector::Resize(thread, bitVector, newLength);
    EXPECT_EQ(bitVector->GetLength(), newLength);

    JSHandle<JSNativePointer> np(thread, bitVector->GetNativePointer());
    auto elements =
        reinterpret_cast<std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *>(np->GetExternalPointer());
    for (uint32_t i = increasedLength; i < newLength; i++) {
        EXPECT_EQ(TestClass::GetBit(elements, i), JSTaggedValue(0));
    }
}

/**
 * @tc.name: OwnKeys
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, OwnKeys)
{
    uint32_t increasedLength = 5;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }

    JSHandle<TaggedArray> keys = JSAPIBitVector::OwnKeys(thread, bitVector);
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*(base::NumberHelper::NumberToString(thread, JSTaggedValue(0))),
        EcmaString::Cast(keys->Get(0).GetTaggedObject())));
}

/**
 * @tc.name: GetOwnProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, GetOwnProperty)
{
    uint32_t increasedLength = 5;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }

    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        bool getOwnPropertyRes = JSAPIBitVector::GetOwnProperty(thread, bitVector, key);
        EXPECT_EQ(getOwnPropertyRes, true);
    }
}

/**
 * @tc.name: GetIteratorObj
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, GetIteratorObj)
{
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    JSHandle<JSTaggedValue> iteratorObj(thread, JSAPIBitVector::GetIteratorObj(thread, bitVector));
    EXPECT_TRUE(iteratorObj->IsJSAPIBitVectorIterator());
}

/**
 * @tc.name: GetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, GetProperty)
{
    uint32_t increasedLength = 5;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        JSAPIBitVector::Push(thread, bitVector, value);
    }

    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        OperationResult getPropertyRes = JSAPIBitVector::GetProperty(thread, bitVector, key);
        EXPECT_EQ(getPropertyRes.GetValue().GetTaggedValue(), JSTaggedValue(1));
    }
}

/**
 * @tc.name: SetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIBitVectorTest, SetProperty)
{
    uint32_t increasedLength = 5;
    JSHandle<JSAPIBitVector> bitVector(thread, CreateBitVector());
    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(0));
        JSAPIBitVector::Push(thread, bitVector, value);
    }

    for (uint32_t i = 0; i < increasedLength; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
        bool setPropertyRes = JSAPIBitVector::SetProperty(thread, bitVector, key, value);
        EXPECT_EQ(setPropertyRes, true);
    }
}
}