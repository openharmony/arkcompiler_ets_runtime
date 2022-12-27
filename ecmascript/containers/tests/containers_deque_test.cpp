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

#include "ecmascript/containers/containers_deque.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_deque.h"
#include "ecmascript/js_api/js_api_deque_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/containers/tests/containers_test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace panda::test {
class ContainersDequeTest : public testing::Test {
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
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 1);
            JSHandle<JSTaggedValue> deque = GetCallArg(argv, 2); // 2 means the secode arg
            if (!deque->IsUndefined()) {
                if (value->IsNumber()) {
                    TaggedArray *elements = TaggedArray::Cast(JSAPIDeque::Cast(deque.GetTaggedValue().
                                            GetTaggedObject())->GetElements().GetTaggedObject());
                    JSTaggedValue result = elements->Get(key->GetInt());
                    EXPECT_EQ(result, value.GetTaggedValue());
                }
            }
            return JSTaggedValue::True();
        }
    };
protected:
    JSTaggedValue InitializeDequeConstructor()
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
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::Deque)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPIDeque> CreateJSAPIDeque(JSTaggedValue compare = JSTaggedValue::Undefined())
    {
        JSHandle<JSTaggedValue> compareHandle(thread, compare);
        JSHandle<JSFunction> newTarget(thread, InitializeDequeConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, compareHandle.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersDeque::DequeConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIDeque> deque(thread, result);
        return deque;
    }
};

HWTEST_F_L0(ContainersDequeTest, DequeConstructor)
{
    InitializeDequeConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeDequeConstructor());

    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersDeque::DequeConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsJSAPIDeque());
    JSHandle<JSAPIDeque> deque(thread, result);
    JSTaggedValue resultProto = JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(deque));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype();
    ASSERT_EQ(resultProto, funcProto);

    // test DequeConstructor exception
    objCallInfo->SetNewTarget(JSTaggedValue::Undefined());
    CONTAINERS_API_EXCEPTION_TEST(ContainersDeque, DequeConstructor, objCallInfo);
}

HWTEST_F_L0(ContainersDequeTest, InsertFrontAndGetFirst)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIDeque> deque = CreateJSAPIDeque();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(deque.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersDeque::InsertFront(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(ContainersDeque::GetFirst(callInfo), JSTaggedValue(i));
    }
}

HWTEST_F_L0(ContainersDequeTest, InsertEndAndGetLast)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIDeque> deque = CreateJSAPIDeque();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(deque.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersDeque::InsertEnd(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(ContainersDeque::GetLast(callInfo), JSTaggedValue(i));
    }
}

HWTEST_F_L0(ContainersDequeTest, Has)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIDeque> deque = CreateJSAPIDeque();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(deque.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersDeque::InsertEnd(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }

    int num = 7;
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(deque.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersDeque::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
    }
    num = 7;
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(deque.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i + 8));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersDeque::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::False());
    }
}

HWTEST_F_L0(ContainersDequeTest, PopFirst)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIDeque> deque = CreateJSAPIDeque();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(deque.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersDeque::InsertFront(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(ContainersDeque::PopFirst(callInfo), JSTaggedValue(i));
    }
}

HWTEST_F_L0(ContainersDequeTest, PopLast)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIDeque> deque = CreateJSAPIDeque();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(deque.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersDeque::InsertEnd(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(ContainersDeque::PopLast(callInfo), JSTaggedValue(i));
    }
}

HWTEST_F_L0(ContainersDequeTest, ForEach)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIDeque> deque = CreateJSAPIDeque();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(deque.GetTaggedValue());
            callInfo->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            ContainersDeque::InsertEnd(callInfo);
            TestHelper::TearDownFrame(thread, prev);
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIDeque> dlist = CreateJSAPIDeque();
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(deque.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, dlist.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersDeque::ForEach(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
}

HWTEST_F_L0(ContainersDequeTest, ProxyOfGetSize)
{
    constexpr uint32_t NODE_NUMBERS = 10;
    JSHandle<JSAPIDeque> deque = CreateJSAPIDeque();
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    JSHandle<JSProxy> proxy = CreateJSProxyHandle(thread);
    proxy->SetTarget(thread, deque.GetTaggedValue());
    callInfo->SetThis(proxy.GetTaggedValue());

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersDeque::InsertFront(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue retult = ContainersDeque::GetSize(callInfo);
        TestHelper::TearDownFrame(thread, prev1);
        EXPECT_EQ(retult, JSTaggedValue(i + 1));
    }
}

HWTEST_F_L0(ContainersDequeTest, ExceptionReturn)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersDeque, InsertFront);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersDeque, InsertEnd);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersDeque, GetFirst);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersDeque, GetLast);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersDeque, Has);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersDeque, PopFirst);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersDeque, PopLast);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersDeque, ForEach);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersDeque, GetIteratorObj);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersDeque, GetSize);
}
}  // namespace panda::test
