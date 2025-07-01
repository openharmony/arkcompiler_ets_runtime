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

#include "arkcompiler/ets_runtime/ecmascript/containers/containers_bitvector.h"
#include "arkcompiler/ets_runtime/ecmascript/js_api/js_api_bitvector.h"
#include "arkcompiler/ets_runtime/ecmascript/js_api/js_api_bitvector_iterator.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/containers/tests/containers_test_helper.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;
namespace panda::test {
class ContainersBitVectorTest : public testing::Test {
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
    EcmaVM* instance { nullptr };
    EcmaHandleScope* scope { nullptr };
    JSThread* thread { nullptr };

protected:
    JSTaggedValue InitializeBitVectorConstructor()
    {
        ObjectFactory* factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();
        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::BitVector)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }
    JSHandle<JSAPIBitVector> CreateJSAPIBitVector()
    {
        JSHandle<JSFunction> newTarget(thread, InitializeBitVectorConstructor());
        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersBitVector::BitVectorConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIBitVector> bitVector(thread, result);
        return bitVector;
    }
    void Push(JSHandle<JSAPIBitVector> bitVector)
    {
        constexpr uint32_t NODE_NUMBERS = 64;
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            if (i >= 32) { // 32 means half bitvector length
                auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
                callInfo->SetFunction(JSTaggedValue::Undefined());
                callInfo->SetThis(bitVector.GetTaggedValue());
                callInfo->SetCallArg(0, JSTaggedValue(1));
                [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
                ContainersBitVector::Push(callInfo);
                TestHelper::TearDownFrame(thread, prev);
            } else {
                auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
                callInfo->SetFunction(JSTaggedValue::Undefined());
                callInfo->SetThis(bitVector.GetTaggedValue());
                callInfo->SetCallArg(0, JSTaggedValue(0));
                [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
                ContainersBitVector::Push(callInfo);
                TestHelper::TearDownFrame(thread, prev);
            }
        }
    }
    JSTaggedValue Has(JSHandle<JSAPIBitVector> bitVector, int number, int startIndex, int endIndex)
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(number));
        callInfo->SetCallArg(1, JSTaggedValue(startIndex));
        callInfo->SetCallArg(2, JSTaggedValue(endIndex)); // 2 means third args.
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }
};
/**
   * @tc.number: _BitVector_BitVevtorConstructor_Func_001
   * @tc.name: test_init_bitvector
   * @tc.desc: A constructor used to create a BitVector object.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, BitVectorConstructor)
{
    InitializeBitVectorConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBitVectorConstructor());
    auto objCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());
    objCallInfo->SetCallArg(0, JSTaggedValue(10));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersBitVector::BitVectorConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.IsJSAPIBitVector());
    JSHandle<JSAPIBitVector> bitVector(thread, result);
    JSTaggedValue resultProto = JSObject::GetPrototype(thread, JSHandle<JSObject>::Cast(bitVector));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype(thread);
    ASSERT_EQ(resultProto, funcProto);
    int length = bitVector->GetLength();
    ASSERT_EQ(length, 10);
    objCallInfo->SetNewTarget(JSTaggedValue::Undefined());
    CONTAINERS_API_EXCEPTION_TEST(ContainersBitVector, BitVectorConstructor, objCallInfo);
}
/**
   * @tc.number: _BitVector_push_Func_001
   * @tc.name: test_push
   * @tc.desc: Appends the bit element to the end of this bit vector.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, Push_001)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), i + 1);
    }
}
/**
   * @tc.number: _BitVector_pop_Func_001
   * @tc.name: test_pop
   * @tc.desc: Retrieves and removes the bit element to the end of this bit vector.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, Pop_001)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), i + 1);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Pop(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(1));
    }
}
/**
   * @tc.number: _BitVector_has_Func_001
   * @tc.name: test_has
   * @tc.desc: Check if bit vector contains a particular bit element
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, Has_001)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
    }
}
/**
   * @tc.number: _BitVector_has_Func_002
   * @tc.name: test_has
   * @tc.desc: Check if bit vector contains a particular bit element
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 2
   */
HWTEST_F_L0(ContainersBitVectorTest, Has_002)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(1.1));
        callInfo->SetCallArg(2, JSTaggedValue(1.1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }
}
/**
   * @tc.number: _BitVector_has_Func_003
   * @tc.name: test_has
   * @tc.desc: Check if bit vector contains a particular bit element
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 1
   */
HWTEST_F_L0(ContainersBitVectorTest, Has_003)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    Push(bitVector);
    JSTaggedValue result1 = Has(bitVector, 1, 0, 31);
    EXPECT_EQ(result1, JSTaggedValue::False());
    JSTaggedValue result2 = Has(bitVector, 0, 0, 32);
    EXPECT_EQ(result2, JSTaggedValue::True());
    JSTaggedValue result3 = Has(bitVector, 1, 32, 64);
    EXPECT_EQ(result3, JSTaggedValue::True());
    JSTaggedValue result4 = Has(bitVector, 0, 32, 63);
    EXPECT_EQ(result4, JSTaggedValue::False());
}
/**
   * @tc.number: _BitVector_setBitsByRange_Func_001
   * @tc.name: test_setBitsByRange
   * @tc.desc: Sets a range of bits in a bit vector to a particular element.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, SetBitsByRange_001)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::SetBitsByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Pop(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
    }
}
/**
   * @tc.number: _BitVector_setBitsByRange_Func_002
   * @tc.name: test_setBitsByRange
   * @tc.desc: Sets a range of bits in a bit vector to a particular element.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 2
   */
HWTEST_F_L0(ContainersBitVectorTest, SetBitsByRange_002)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(1.1));
        callInfo->SetCallArg(2, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::SetBitsByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }
}
/**
   * @tc.number: _BitVector_setBitsByRange_Func_003
   * @tc.name: test_setBitsByRange
   * @tc.desc: Sets a range of bits in a bit vector to a particular element.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 1
   */
HWTEST_F_L0(ContainersBitVectorTest, SetBitsByRange_003)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    Push(bitVector);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(32));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::SetBitsByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(32));
        callInfo->SetCallArg(2, JSTaggedValue(64));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::SetBitsByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    constexpr uint32_t NODE_NUMBERS = 64;
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        if (i >= 32) { // 32 means half bitvector length
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(bitVector.GetTaggedValue());
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result0 = ContainersBitVector::Pop(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result0, JSTaggedValue(1));
        } else {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(bitVector.GetTaggedValue());
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result1 = ContainersBitVector::Pop(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result1, JSTaggedValue(0));
        }
    }
}
/**
   * @tc.number: _BitVector_setAllBits_Func_001
   * @tc.name: test_setAllBits
   * @tc.desc: Sets all of bits in a bit vector to a particular element.
   * @tc.size: MediumTest MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, SetAllBits_001)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::SetAllBits(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(bitVector->GetSize(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Pop(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(1));
    }
}
/**
   * @tc.number: _BitVector_setAllBits_Func_002
   * @tc.name: test_setAllBits
   * @tc.desc: Sets all of bits in a bit vector to a particular element.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 1
   */
HWTEST_F_L0(ContainersBitVectorTest, SetAllBits_002)
{
    constexpr uint32_t NODE_NUMBERS = 64;
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    EXPECT_EQ(bitVector->GetSize(), 64);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::SetAllBits(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Pop(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(1));
    }
}
/**
   * @tc.number: _BitVector_setAllBits_Func_003
   * @tc.name: test_setAllBits
   * @tc.desc: Sets all of bits in a bit vector to a particular element.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 1
   */
HWTEST_F_L0(ContainersBitVectorTest, SetAllBits_003)
{
    constexpr uint32_t NODE_NUMBERS = 64;
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    EXPECT_EQ(bitVector->GetSize(), 64);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::SetAllBits(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Pop(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
    }
}
/**
   * @tc.number: _BitVector_getBitsByRange_Func_001
   * @tc.name: test_getBitsByRange
   * @tc.desc: Returns the bit values in a range of indices in a bit vector.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, GetBitsByRange_001)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetBitsByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSNativePointer> np(
            thread, JSAPIBitVector::Cast(result.GetTaggedObject())
                        ->GetNativePointer(thread));
        auto elements =
            reinterpret_cast<std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *>(np->GetExternalPointer());
        EXPECT_EQ((*elements)[0].test(0), 1);
    }
}
/**
   * @tc.number: _BitVector_getBitsByRange_Func_002
   * @tc.name: test_getBitsByRange
   * @tc.desc: Returns the bit values in a range of indices in a bit vector.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 2
   */
HWTEST_F_L0(ContainersBitVectorTest, GetBitsByRange_002)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1.1));
        callInfo->SetCallArg(1, JSTaggedValue(1.1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetBitsByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }
}
/**
   * @tc.number: _BitVector_getBitsByRange_Func_003
   * @tc.name: test_getBitsByRange
   * @tc.desc: Returns the bit values in a range of indices in a bit vector.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 1
   */
HWTEST_F_L0(ContainersBitVectorTest, GetBitsByRange_003)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    Push(bitVector);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0)); //range 0 to 31
        callInfo->SetCallArg(1, JSTaggedValue(31));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetBitsByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSNativePointer> np(
            thread, JSAPIBitVector::Cast(result.GetTaggedObject())
                        ->GetNativePointer(thread));
        auto elements =
            reinterpret_cast<std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *>(np->GetExternalPointer());
        EXPECT_EQ((*elements)[0].test(0), 0);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(32));
        callInfo->SetCallArg(1, JSTaggedValue(64));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetBitsByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSNativePointer> np(
            thread, JSAPIBitVector::Cast(result.GetTaggedObject())
                        ->GetNativePointer(thread));
        auto elements =
            reinterpret_cast<std::vector<std::bitset<JSAPIBitVector::BIT_SET_LENGTH>> *>(np->GetExternalPointer());
        EXPECT_EQ((*elements)[0].test(0), 1);
    }
}
/**
   * @tc.number: _BitVector_resize_Func_001
   * @tc.name: test_resize
   * @tc.desc: Resize the bitVector's length.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, Resize_01)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), i + 1);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(5));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::Resize(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    EXPECT_EQ(bitVector->GetLength(), 5);
}
/**
   * @tc.number: _BitVector_resize_Func_002
   * @tc.name: test_resize
   * @tc.desc: Resize the bitVector's length.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, Resize_02)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), i + 1);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(8));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::Resize(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    EXPECT_EQ(bitVector->GetLength(), 8);
}
/**
   * @tc.number: _BitVector_resize_Func_003
   * @tc.name: test_resize
   * @tc.desc: Resize the bitVector's length.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, Resize_03)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), i + 1);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(10));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::Resize(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    EXPECT_EQ(bitVector->GetLength(), 10);
    for (uint32_t i = 0; i < 2; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Pop(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
    }
}
/**
   * @tc.number: _BitVector_resize_Func_004
   * @tc.name: test_resize
   * @tc.desc: Resize the bitVector's length.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 2
   */
HWTEST_F_L0(ContainersBitVectorTest, Resize_04)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), i + 1);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1.1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Resize(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }
}
/**
   * @tc.number: _BitVector_getBitCountByRange_Func_001
   * @tc.name: test_getBitCountByRange
   * @tc.desc: Counts the number of times a certain bit element occurs within a range of bits in a bit vector.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, GetBitCountByRange_001)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetBitCountByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(1));
    }
}
/**
   * @tc.number: _BitVector_getBitCountByRange_Func_002
   * @tc.name: test_getBitCountByRange
   * @tc.desc: Counts the number of times a certain bit element occurs within a range of bits in a bit vector.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 2
   */
HWTEST_F_L0(ContainersBitVectorTest, GetBitCountByRange_002)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(1.1));
        callInfo->SetCallArg(2, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetBitCountByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }
}
/**
   * @tc.number: _BitVector_getBitCountByRange_Func_003
   * @tc.name: test_getBitCountByRange
   * @tc.desc: Counts the number of times a certain bit element occurs within a range of bits in a bit vector.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 1
   */
HWTEST_F_L0(ContainersBitVectorTest, GetBitCountByRange_003)
{
    constexpr uint32_t NODE_NUMBERS = 64;
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        if (i >= 32) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(bitVector.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(1));
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            ContainersBitVector::Push(callInfo);
            TestHelper::TearDownFrame(thread, prev);
        } else {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(bitVector.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(0));
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            ContainersBitVector::Push(callInfo);
            TestHelper::TearDownFrame(thread, prev);
        }
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(64));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetBitCountByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(32));
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(64));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetBitCountByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(32));
    }
}
/**
   * @tc.number: _BitVector_getIndexOf_Func_001
   * @tc.name: test_getIndexOf
   * @tc.desc: Locates the first occurrence of a certain bit value within a range of bits in a bit vector.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, GetIndexOf_001)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetIndexOf(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
    }
}
/**
   * @tc.number: _BitVector_getIndexOf_Func_002
   * @tc.name: test_getIndexOf
   * @tc.desc: Locates the first occurrence of a certain bit value within a range of bits in a bit vector.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 2
   */
HWTEST_F_L0(ContainersBitVectorTest, GetIndexOf_002)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(1.1));
        callInfo->SetCallArg(2, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetIndexOf(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }
}
/**
   * @tc.number: _BitVector_getIndexOf_Func_003
   * @tc.name: test_getIndexOf
   * @tc.desc: Locates the first occurrence of a certain bit value within a range of bits in a bit vector.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 1
   */
HWTEST_F_L0(ContainersBitVectorTest, GetIndexOf_003)
{
    constexpr uint32_t NODE_NUMBERS = 64;
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        if (i >= 32) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(bitVector.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(1));
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            ContainersBitVector::Push(callInfo);
            TestHelper::TearDownFrame(thread, prev);
        } else {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(bitVector.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(0));
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            ContainersBitVector::Push(callInfo);
            TestHelper::TearDownFrame(thread, prev);
        }
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(64));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetIndexOf(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(32));
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(64));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetIndexOf(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
    }
}
/**
   * @tc.number: _BitVector_getLastIndexOf_Func_001
   * @tc.name: test_getLastIndexOf
   * @tc.desc: Locates the last occurrence of a certain bit value within a range of bits in a bit vector.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, GetLastIndexOf_001)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetLastIndexOf(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
    }
}
/**
   * @tc.number: _BitVector_getLastIndexOf_Func_002
   * @tc.name: test_getLastIndexOf
   * @tc.desc: Locates the last occurrence of a certain bit value within a range of bits in a bit vector.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 2
   */
HWTEST_F_L0(ContainersBitVectorTest, GetLastIndexOf_002)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(1.1));
        callInfo->SetCallArg(2, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetLastIndexOf(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }
}
/**
   * @tc.number: _BitVector_getLastIndexOf_Func_003
   * @tc.name: test_getLastIndexOf
   * @tc.desc: Locates the last occurrence of a certain bit value within a range of bits in a bit vector.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 1
   */
HWTEST_F_L0(ContainersBitVectorTest, GetLastIndexOf_003)
{
    constexpr uint32_t NODE_NUMBERS = 64;
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        if (i >= 32) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(bitVector.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(1));
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            ContainersBitVector::Push(callInfo);
            TestHelper::TearDownFrame(thread, prev);
        } else {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(bitVector.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(0));
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            ContainersBitVector::Push(callInfo);
            TestHelper::TearDownFrame(thread, prev);
        }
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(64));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetLastIndexOf(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(63));
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(64));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::GetLastIndexOf(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(31));
    }
}
/**
   * @tc.number: _BitVector_flipBitByIndex_Func_001
   * @tc.name: test_flipBitByIndex
   * @tc.desc: Flips the bit value by index in a bit vector.(Flip 0 to 1, flip 1 to 0).
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, FlipBitByIndex_001)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::FlipBitByIndex(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Pop(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
    }
}
/**
   * @tc.number: _BitVector_flipBitByIndex_Func_002
   * @tc.name: test_flipBitByIndex
   * @tc.desc: Flips the bit value by index in a bit vector.(Flip 0 to 1, flip 1 to 0).
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 2
   */
HWTEST_F_L0(ContainersBitVectorTest, FlipBitByIndex_2)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1)); //input 1

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1.1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::FlipBitByIndex(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }
}
/**
   * @tc.number: _BitVector_flipBitByIndex_Func_003
   * @tc.name: test_flipBitByIndex
   * @tc.desc: Flips the bit value by index in a bit vector.(Flip 0 to 1, flip 1 to 0).
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 1
   */
HWTEST_F_L0(ContainersBitVectorTest, FlipBitByIndex_003)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    Push(bitVector);
    constexpr uint32_t NODE_NUMBERS = 64;
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::FlipBitByIndex(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        if (i >= 32) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(bitVector.GetTaggedValue());
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result0 = ContainersBitVector::Pop(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result0, JSTaggedValue(1));
        } else {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(bitVector.GetTaggedValue());
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result1 = ContainersBitVector::Pop(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result1, JSTaggedValue(0));
        }
    }
}
/**
   * @tc.number: _BitVector_flipBitsByRange_Func_001
   * @tc.name: test_flipBitsByRange
   * @tc.desc: Flips a range of bit values in a bit vector.(Flip 0 to 1, flip 1 to 0).
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, FlipBitsByRange_001)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::FlipBitsByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Pop(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
    }
}
/**
   * @tc.number: _BitVector_flipBitsByRange_Func_002
   * @tc.name: test_flipBitsByRange
   * @tc.desc: Flips a range of bit values in a bit vector.(Flip 0 to 1, flip 1 to 0).
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 2
   */
HWTEST_F_L0(ContainersBitVectorTest, FlipBitsByRange_002)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetLength(), 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1.1));
        callInfo->SetCallArg(1, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::FlipBitsByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }
}
/**
   * @tc.number: _BitVector_flipBitsByRange_Func_003
   * @tc.name: test_flipBitsByRange
   * @tc.desc: Flips a range of bit values in a bit vector.(Flip 0 to 1, flip 1 to 0).
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 1
   */
HWTEST_F_L0(ContainersBitVectorTest, FlipBitsByRange_003)
{
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    Push(bitVector);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(32));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::FlipBitsByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(32));
        callInfo->SetCallArg(1, JSTaggedValue(64));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBitVector::FlipBitsByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    constexpr uint32_t NODE_NUMBERS = 64;
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        if (i >= 32) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(bitVector.GetTaggedValue());
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result0 = ContainersBitVector::Pop(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result0, JSTaggedValue(1));
        } else {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(bitVector.GetTaggedValue());
            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            JSTaggedValue result1 = ContainersBitVector::Pop(callInfo);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result1, JSTaggedValue(0));
        }
    }
}
/**
   * @tc.number: _BitVector_GetIteratorObj_Func_001
   * @tc.name: test_GetIteratorObj
   * @tc.desc: Returns an iterator.Each item of the iterator is a Javascript Object.
   * @tc.size: MediumTest
   * @tc.type: Function
   * @tc.level: Level 0
   */
HWTEST_F_L0(ContainersBitVectorTest, GetIteratorObj)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIBitVector> bitVector = CreateJSAPIBitVector();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(bitVector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBitVector::Push(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(bitVector->GetSize(), static_cast<int>(i + 1));
    }
    auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo1->SetFunction(JSTaggedValue::Undefined());
    callInfo1->SetThis(bitVector.GetTaggedValue());
    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
    JSHandle<JSTaggedValue> iterValues(thread, ContainersBitVector::GetIteratorObj(callInfo1));
    TestHelper::TearDownFrame(thread, prev1);
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterValues.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        result.Update(JSAPIBitVectorIterator::Next(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(static_cast<int>(1), JSIterator::IteratorValue(thread, result)->GetInt());
    }
}
};
