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
#include "ecmascript/containers/containers_hashset.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/js_api/js_api_hashset.h"
#include "ecmascript/js_api/js_api_hashset_iterator.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/containers/tests/containers_test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace panda::test {
class ContainersHashSetTest : public testing::Test {
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
    JSTaggedValue InitializeHashSetConstructor()
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
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::HashSet)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }

    JSHandle<JSAPIHashSet> CreateJSAPIHashSet()
    {
        JSHandle<JSFunction> newTarget(thread, InitializeHashSetConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersHashSet::HashSetConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIHashSet> map(thread, result);
        return map;
    }
};

// new HashSet()
HWTEST_F_L0(ContainersHashSetTest, HashSetConstructor)
{
    InitializeHashSetConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeHashSetConstructor());

    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersHashSet::HashSetConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsJSAPIHashSet());
    JSHandle<JSAPIHashSet> setHandle(thread, result);
    JSTaggedValue resultProto = JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(setHandle));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype(thread);
    ASSERT_EQ(resultProto, funcProto);
    int size = setHandle->GetSize();
    ASSERT_EQ(size, 0);

    // test HashSetConstructor exception
    objCallInfo->SetNewTarget(JSTaggedValue::Undefined());
    CONTAINERS_API_EXCEPTION_TEST(ContainersHashSet, HashSetConstructor, objCallInfo);
}

// hashset.add(key), hashset.has(key)
HWTEST_F_L0(ContainersHashSetTest, AddAndHas)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIHashSet> tSet = CreateJSAPIHashSet();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tSet.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashSet::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tSet->GetSize(), i + 1);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tSet.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashSet::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
    }
}

// hashset.remove(key)
HWTEST_F_L0(ContainersHashSetTest, Remove)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIHashSet> tSet = CreateJSAPIHashSet();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tSet.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashSet::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tSet->GetSize(), i + 1);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tSet.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(NODE_NUMBERS / 2));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue rValue = ContainersHashSet::Remove(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(rValue, JSTaggedValue::True());
        EXPECT_EQ(tSet->GetSize(), NODE_NUMBERS - 1);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tSet.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashSet::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        if (i == (NODE_NUMBERS / 2)) {
            EXPECT_EQ(result, JSTaggedValue::False());
        } else {
            EXPECT_EQ(result, JSTaggedValue::True());
        }
    }
    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tSet.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashSet::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tSet->GetSize(), NODE_NUMBERS + i);
    }
    {
        std::string iKey = myKey + std::to_string(NODE_NUMBERS / 2);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tSet.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue rValue = ContainersHashSet::Remove(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::SameValue(thread, rValue, JSTaggedValue::True()));
        EXPECT_EQ(tSet->GetSize(), NODE_NUMBERS * 2 - 2);
    }
}

// hashset.clear()
HWTEST_F_L0(ContainersHashSetTest, Clear)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIHashSet> tSet = CreateJSAPIHashSet();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tSet.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashSet::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tSet->GetSize(), i + 1);
    }

    // test clear
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tSet.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersHashSet::Clear(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(tSet->GetSize(), (uint32_t)0);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tSet.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashSet::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::False());
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tSet.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashSet::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tSet->GetSize(), i + 1);
    }

    // test clear
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tSet.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersHashSet::Clear(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(tSet->GetSize(), (uint32_t)0);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tSet.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersHashSet::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::False());
    }
}

// hashset.values(), hashset.entries()
HWTEST_F_L0(ContainersHashSetTest, KeysAndValuesAndEntries)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIHashSet> tSet = CreateJSAPIHashSet();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(tSet.GetTaggedValue());
        callInfo1->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo1);
        JSTaggedValue result = ContainersHashSet::Add(callInfo1);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(tSet->GetSize(), i + 1);
    }

    // test values
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(tSet.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSHandle<JSTaggedValue> iterValues(thread, ContainersHashSet::Values(callInfo));
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(iterValues->IsJSAPIHashSetIterator());
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo2->SetFunction(JSTaggedValue::Undefined());
        callInfo2->SetThis(iterValues.GetTaggedValue());

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo2);
        result.Update(JSAPIHashSetIterator::Next(callInfo2));
        TestHelper::TearDownFrame(thread, prev1);
        JSHandle<JSTaggedValue> iterValue = JSIterator::IteratorValue(thread, result);
        JSTaggedValue valueFlag = tSet->Has(thread, iterValue.GetTaggedValue());
        EXPECT_EQ(JSTaggedValue::True(), valueFlag);
    }

    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iKey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(iKey).GetTaggedValue());

        auto callInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo3->SetFunction(JSTaggedValue::Undefined());
        callInfo3->SetThis(tSet.GetTaggedValue());
        callInfo3->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, callInfo3);
        JSTaggedValue result1 = ContainersHashSet::Add(callInfo3);
        TestHelper::TearDownFrame(thread, prev2);
        EXPECT_TRUE(result1.IsTrue());
        EXPECT_EQ(tSet->GetSize(), NODE_NUMBERS + i + 1);
    }
    EXPECT_EQ(tSet->GetSize(), NODE_NUMBERS * 2);
    {
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            std::string iKey = myKey + std::to_string(i);
            key.Update(factory->NewFromStdString(iKey).GetTaggedValue());
            JSTaggedValue keyFlag = tSet->Has(thread, key.GetTaggedValue());
            EXPECT_EQ(JSTaggedValue::True(), keyFlag);
        }
    }
    // test entries
    {
        auto callInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo4->SetFunction(JSTaggedValue::Undefined());
        callInfo4->SetThis(tSet.GetTaggedValue());
        [[maybe_unused]] auto prev6 = TestHelper::SetupFrame(thread, callInfo4);
        JSHandle<JSTaggedValue> iter(thread, ContainersHashSet::Entries(callInfo4));
        TestHelper::TearDownFrame(thread, prev6);
        EXPECT_TRUE(iter->IsJSAPIHashSetIterator());

        JSHandle<JSTaggedValue> first(thread, JSTaggedValue(0));
        JSHandle<JSTaggedValue> second(thread, JSTaggedValue(1));
        JSMutableHandle<JSTaggedValue> result2(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> entries(thread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < NODE_NUMBERS * 2; i++) {
            auto callInfo6 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            callInfo6->SetFunction(JSTaggedValue::Undefined());
            callInfo6->SetThis(iter.GetTaggedValue());

            [[maybe_unused]] auto prev4 = TestHelper::SetupFrame(thread, callInfo6);
            result2.Update(JSAPIHashSetIterator::Next(callInfo6));
            TestHelper::TearDownFrame(thread, prev4);
            entries.Update(JSIterator::IteratorValue(thread, result2).GetTaggedValue());
            JSHandle<JSTaggedValue> iterValue = JSObject::GetProperty(thread, entries, second).GetValue();
            JSTaggedValue valueFlag = tSet->Has(thread, iterValue.GetTaggedValue());
            EXPECT_EQ(JSTaggedValue::True(), valueFlag);
        }
    }
}

HWTEST_F_L0(ContainersHashSetTest, ProxyOfGetLength)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIHashSet> hashSet = CreateJSAPIHashSet();
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    JSHandle<JSProxy> proxy = CreateJSProxyHandle(thread);
    proxy->SetTarget(thread, hashSet.GetTaggedValue());
    callInfo->SetThis(proxy.GetTaggedValue());

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersHashSet::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue retult = ContainersHashSet::GetLength(callInfo);
        TestHelper::TearDownFrame(thread, prev1);
        EXPECT_EQ(retult, JSTaggedValue(i + 1));
    }
}

HWTEST_F_L0(ContainersHashSetTest, ExceptionReturn)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashSet, Values);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashSet, Entries);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashSet, Add);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashSet, Remove);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashSet, Has);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashSet, Clear);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashSet, GetLength);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashSet, IsEmpty);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersHashSet, ForEach);
}
}
