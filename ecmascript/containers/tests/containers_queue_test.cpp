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

#include "ecmascript/containers/containers_queue.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_queue.h"
#include "ecmascript/js_api/js_api_queue_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/containers/tests/containers_test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace panda::test {
class ContainersQueueTest : public testing::Test {
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
    JSTaggedValue InitializeQueueConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::Queue)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPIQueue> CreateJSAPIQueue(JSTaggedValue compare = JSTaggedValue::Undefined())
    {
        JSHandle<JSTaggedValue> compareHandle(thread, compare);
        JSHandle<JSFunction> newTarget(thread, InitializeQueueConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, compareHandle.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersQueue::QueueConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIQueue> queue(thread, result);
        return queue;
    }
};

HWTEST_F_L0(ContainersQueueTest, QueueConstructor)
{
    InitializeQueueConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeQueueConstructor());

    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersQueue::QueueConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsJSAPIQueue());
    JSHandle<JSAPIQueue> queue(thread, result);
    JSTaggedValue resultProto = JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(queue));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype();
    ASSERT_EQ(resultProto, funcProto);

    // test QueueConstructor exception
    objCallInfo->SetNewTarget(JSTaggedValue::Undefined());
    CONTAINERS_API_EXCEPTION_TEST(ContainersQueue, QueueConstructor, objCallInfo);
}

HWTEST_F_L0(ContainersQueueTest, ProxyOfGetSize)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIQueue> queue = CreateJSAPIQueue();
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    JSHandle<JSProxy> proxy = CreateJSProxyHandle(thread);
    proxy->SetTarget(thread, queue.GetTaggedValue());
    callInfo->SetThis(proxy.GetTaggedValue());

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersQueue::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue retult = ContainersQueue::GetSize(callInfo);
        TestHelper::TearDownFrame(thread, prev1);
        EXPECT_EQ(retult, JSTaggedValue(i + 1));
    }
}

HWTEST_F_L0(ContainersQueueTest, ExceptionReturn)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersQueue, Add);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersQueue, GetFirst);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersQueue, Pop);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersQueue, ForEach);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersQueue, GetIteratorObj);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersQueue, GetSize);
}
}  // namespace panda::test
