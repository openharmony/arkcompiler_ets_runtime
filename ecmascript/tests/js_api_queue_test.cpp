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

#include "ecmascript/js_api/js_api_queue.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSAPIQueueTest : public testing::Test {
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

protected:
    JSHandle<JSAPIQueue> CreateQueue(int capacaty = JSAPIQueue::DEFAULT_CAPACITY_LENGTH)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
        ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(containers::ContainerTag::Queue)));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = containers::ContainersPrivate::Load(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSTaggedValue> constructor(thread, result);
        JSHandle<JSAPIQueue> jsQueue(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(constructor), constructor));
        JSHandle<TaggedArray> newElements = factory->NewTaggedArray(capacaty);
        jsQueue->SetElements(thread, newElements);
        jsQueue->SetLength(thread, JSTaggedValue(0));
        jsQueue->SetFront(0);
        jsQueue->SetTail(0);
        return jsQueue;
    }
};

HWTEST_F_L0(JSAPIQueueTest, queueCreate)
{
    JSHandle<JSAPIQueue> jsQueue = CreateQueue();
    EXPECT_TRUE(*jsQueue != nullptr);
}

HWTEST_F_L0(JSAPIQueueTest, AddAndHasAndSetAndGet)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPIQueue> jsQueue = CreateQueue();

    std::string queueValue("queuevalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = queueValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIQueue::Add(thread, jsQueue, value);
    }
    EXPECT_EQ(jsQueue->GetSize(), DEFAULT_LENGTH);
    EXPECT_EQ(JSAPIQueue::GetArrayLength(thread, jsQueue), DEFAULT_LENGTH);

    // test Set, Has and Get
    std::string ivalue = queueValue + std::to_string(10);
    value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
    EXPECT_FALSE(jsQueue->Has(value.GetTaggedValue()));
    jsQueue->Set(thread, 0, value.GetTaggedValue());
    EXPECT_EQ(jsQueue->Get(thread, 0), value.GetTaggedValue());
    EXPECT_TRUE(jsQueue->Has(value.GetTaggedValue()));

    // test Get exception
    JSTaggedValue result = jsQueue->Get(thread, DEFAULT_LENGTH);
    EXPECT_EQ(result, JSTaggedValue::Exception());
    EXPECT_EXCEPTION();
}

HWTEST_F_L0(JSAPIQueueTest, PopFirstAndGetFirst)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPIQueue> jsQueue = CreateQueue();

    // test GetFirst and pop of empty queue
    EXPECT_EQ(JSAPIQueue::GetFirst(thread, jsQueue), JSTaggedValue::Undefined());
    EXPECT_EQ(JSAPIQueue::Pop(thread, jsQueue), JSTaggedValue::Undefined());

    std::string queueValue("queuevalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = queueValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIQueue::Add(thread, jsQueue, value);
    }

    // test GetFirst
    EXPECT_EQ(JSAPIQueue::GetArrayLength(thread, jsQueue), DEFAULT_LENGTH);
    std::string firstValue = queueValue + std::to_string(0U);
    value.Update(factory->NewFromStdString(firstValue).GetTaggedValue());
    EXPECT_TRUE(JSTaggedValue::SameValue(
                JSHandle<JSTaggedValue>(thread, JSAPIQueue::GetFirst(thread, jsQueue)), value));
    // test Pop
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = queueValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        EXPECT_TRUE(JSTaggedValue::SameValue(
            JSHandle<JSTaggedValue>(thread, JSAPIQueue::Pop(thread, jsQueue)), value));
        EXPECT_EQ(JSAPIQueue::GetArrayLength(thread, jsQueue), (DEFAULT_LENGTH - i - 1U));
    }
}

HWTEST_F_L0(JSAPIQueueTest, OwnKeys)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPIQueue> jsQueue = CreateQueue();

    std::string queueValue("queuevalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = queueValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIQueue::Add(thread, jsQueue, value);
    }
    JSHandle<TaggedArray> arrayKey = JSAPIQueue::OwnKeys(thread, jsQueue);
    EXPECT_EQ(arrayKey->GetLength(), DEFAULT_LENGTH);
    for (int32_t i = 0; i < static_cast<int32_t>(DEFAULT_LENGTH); i++) {
        ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*(base::NumberHelper::NumberToString(thread, JSTaggedValue(i))),
            EcmaString::Cast(arrayKey->Get(i).GetTaggedObject())));
    }
}

HWTEST_F_L0(JSAPIQueueTest, GetNextPosition)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPIQueue> jsQueue = CreateQueue();

    std::string queueValue("queuevalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = queueValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIQueue::Add(thread, jsQueue, value);
    }
    // test GetNextPosition
    EXPECT_EQ(jsQueue->GetSize(), DEFAULT_LENGTH);
    for (uint32_t i = 0; i < DEFAULT_LENGTH;) {
        std::string ivalue = queueValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        EXPECT_EQ(jsQueue->Get(thread, i), value.GetTaggedValue());
        i = jsQueue->GetNextPosition(i);
    }
}

HWTEST_F_L0(JSAPIQueueTest, GetOwnProperty)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSAPIQueue> jsQueue = CreateQueue();

    std::string queueValue("queuevalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = queueValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIQueue::Add(thread, jsQueue, value);
    }
    // test GetOwnProperty
    int testInt = 1;
    JSHandle<JSTaggedValue> queueKey1(thread, JSTaggedValue(testInt));
    EXPECT_TRUE(JSAPIQueue::GetOwnProperty(thread, jsQueue, queueKey1));
    testInt = 9;
    JSHandle<JSTaggedValue> queueKey2(thread, JSTaggedValue(testInt));
    EXPECT_FALSE(JSAPIQueue::GetOwnProperty(thread, jsQueue, queueKey2));
    EXPECT_EXCEPTION();

    // test GetOwnProperty exception
    JSHandle<JSTaggedValue> undefined(thread, JSTaggedValue::Undefined());
    EXPECT_FALSE(JSAPIQueue::GetOwnProperty(thread, jsQueue, undefined));
    EXPECT_EXCEPTION();
}

/**
 * @tc.name: GetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIQueueTest, GetProperty)
{
    JSHandle<JSAPIQueue> jsQueue = CreateQueue();
    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIQueue::Add(thread, jsQueue, value);
    }
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        OperationResult getPropertyRes = JSAPIQueue::GetProperty(thread, jsQueue, key);
        EXPECT_EQ(getPropertyRes.GetValue().GetTaggedValue(), JSTaggedValue(i));
    }
}

/**
 * @tc.name: SetProperty
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIQueueTest, SetProperty)
{
    JSHandle<JSAPIQueue> jsQueue = CreateQueue();
    uint32_t elementsNums = 8;
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i));
        JSAPIQueue::Add(thread, jsQueue, value);
    }
    for (uint32_t i = 0; i < elementsNums; i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(i));
        JSHandle<JSTaggedValue> value(thread, JSTaggedValue(i * 2)); // 2 : It means double
        bool setPropertyRes = JSAPIQueue::SetProperty(thread, jsQueue, key, value);
        EXPECT_EQ(setPropertyRes, true);
    }
    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(-1));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(-1));
    EXPECT_FALSE(JSAPIQueue::SetProperty(thread, jsQueue, key, value));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(elementsNums));
    EXPECT_FALSE(JSAPIQueue::SetProperty(thread, jsQueue, key1, value));
}

/**
 * @tc.name: GrowCapacity
 * @tc.desc:
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIQueueTest, GrowCapacity)
{
    JSHandle<JSAPIQueue> jsQueue = CreateQueue(0);
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(0));
    JSHandle<TaggedArray> element(thread, jsQueue->GetElements());
    EXPECT_EQ(element->GetLength(), 0U);
    JSAPIQueue::Add(thread, jsQueue, value);
    JSHandle<TaggedArray> newElement(thread, jsQueue->GetElements());
    EXPECT_EQ(newElement->GetLength(), static_cast<uint32_t>(JSAPIQueue::DEFAULT_CAPACITY_LENGTH));
}
}  // namespace panda::test