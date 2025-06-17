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
#include "ecmascript/js_api/js_api_queue_iterator.h"
#include "ecmascript/js_api/js_api_queue.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda;
using namespace panda::ecmascript;

namespace panda::test {
class JSAPIQueueIteratorTest : public BaseTestWithScope<false> {
protected:
    JSHandle<JSAPIQueue> CreateQueue(int capacaty = JSAPIQueue::DEFAULT_CAPACITY_LENGTH)
    {
        return EcmaContainerCommon::CreateQueue(thread, capacaty);
    }
};

/**
 * @tc.name: Next
 * @tc.desc: Create an iterator of JSAPIQueue,and then loop through the elements of the iterator to check whether
 *           the elements are consistent through Next function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIQueueIteratorTest, Next)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIQueue> jsQueue = CreateQueue();
    EXPECT_TRUE(*jsQueue != nullptr);
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> valueStr = thread->GlobalConstants()->GetHandledValueString();
    // insert value
    std::string queueValue("keyvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = queueValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIQueue::Add(thread, jsQueue, value);
    }
    JSHandle<JSAPIQueueIterator> queueIterator = factory->NewJSAPIQueueIterator(jsQueue);
    for (uint32_t i = 0; i <= DEFAULT_LENGTH; i++) {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(queueIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPIQueueIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSObject> resultObj(thread, result);
        std::string resultValue = queueValue + std::to_string(i);
        if (i <= DEFAULT_LENGTH - 1U) {
            value.Update(factory->NewFromStdString(resultValue).GetTaggedValue());
            EXPECT_EQ(queueIterator->GetNextIndex(), i + 1U);
            EXPECT_EQ(JSTaggedValue::SameValue(thread,
            JSObject::GetProperty(thread, resultObj, valueStr).GetValue(), value), true);
        }
        else {
            EXPECT_TRUE(queueIterator->GetIteratedQueue(thread).IsUndefined());
            EXPECT_TRUE(JSObject::GetProperty(thread, resultObj, valueStr).GetValue()->IsUndefined());
        }
    }
}

/**
 * @tc.name: Next
 * @tc.desc: test special return of Next, including throw exception and return undefined
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIQueueIteratorTest, SpecialReturnOfNext)
{
    JSHandle<JSAPIQueue> jsQueue = CreateQueue();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIQueueIterator> queueIterator = factory->NewJSAPIQueueIterator(jsQueue);
    queueIterator->SetIteratedQueue(thread, JSTaggedValue::Undefined());

    // test Next exception
    {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPIQueueIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::Exception());
        EXPECT_EXCEPTION();
    }

    // test Next return undefined
    {
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(queueIterator.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        JSTaggedValue result = JSAPIQueueIterator::Next(ecmaRuntimeCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, thread->GetEcmaVM()->GetGlobalEnv()->GetUndefinedIteratorResult().GetTaggedValue());
    }
}


/**
 * @tc.name: SetIteratedQueue
 * @tc.desc: Call the "SetIteratedQueue" function, check whether the result returned through "GetIteratedQueue"
 *           function from the JSAPIQueueIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIQueueIteratorTest, SetIteratedQueue)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIQueue> jsQueue1 = CreateQueue();
    JSHandle<JSAPIQueue> jsQueue2 = CreateQueue();
    EXPECT_TRUE(*jsQueue1 != nullptr);
    EXPECT_TRUE(*jsQueue2 != nullptr);
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    // insert value
    std::string queueValue("keyvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = queueValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIQueue::Add(thread, jsQueue1, value);
    }

    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = queueValue + std::to_string(i + 1U);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIQueue::Add(thread, jsQueue2, value);
    }
    JSHandle<JSAPIQueueIterator> queueIterator = factory->NewJSAPIQueueIterator(jsQueue1);
    EXPECT_EQ(queueIterator->GetIteratedQueue(thread), jsQueue1.GetTaggedValue());

    queueIterator->SetIteratedQueue(thread, jsQueue2.GetTaggedValue());
    EXPECT_EQ(queueIterator->GetIteratedQueue(thread), jsQueue2.GetTaggedValue());
}

/**
 * @tc.name: SetNextIndex
 * @tc.desc: Call the "SetNextIndex" function, check whether the result returned through "GetNextIndex"
 *           function from the JSAPIQueueIterator is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSAPIQueueIteratorTest, SetNextIndex)
{
    constexpr uint32_t DEFAULT_LENGTH = 8;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIQueue> jsQueue = CreateQueue();
    EXPECT_TRUE(*jsQueue != nullptr);
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    // insert value
    std::string queueValue("keyvalue");
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        std::string ivalue = queueValue + std::to_string(i);
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());
        JSAPIQueue::Add(thread, jsQueue, value);
    }
    JSHandle<JSAPIQueueIterator> queueIterator = factory->NewJSAPIQueueIterator(jsQueue);
    for (uint32_t i = 0; i < DEFAULT_LENGTH; i++) {
        queueIterator->SetNextIndex(i);
        EXPECT_EQ(queueIterator->GetNextIndex(), i);
    }
}
}  // namespace panda::test
