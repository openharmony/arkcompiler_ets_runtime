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

#include "ecmascript/containers/containers_linked_list.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_linked_list.h"
#include "ecmascript/js_api/js_api_linked_list_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/containers/tests/containers_test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace panda::test {
class ContainersLinkedListTest : public testing::Test {
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
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
            JSHandle<JSTaggedValue> list = GetCallArg(argv, 2); // 2 means the secode arg
            if (!list->IsUndefined()) {
                if (index->IsNumber() && value->IsNumber()) {
                    JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(value->GetInt() * 2)); // 2 means mul by 2
                    JSAPILinkedList::Set(thread, JSHandle<JSAPILinkedList>::Cast(list), index->GetInt(), newValue);
                }
            }
            return JSTaggedValue::True();
        }
    };
protected:
    JSTaggedValue InitializeLinkedListConstructor()
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
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::LinkedList)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPILinkedList> CreateJSAPILinkedList(JSTaggedValue compare = JSTaggedValue::Undefined())
    {
        JSHandle<JSTaggedValue> compareHandle(thread, compare);
        JSHandle<JSFunction> newTarget(thread, InitializeLinkedListConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, compareHandle.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersLinkedList::LinkedListConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPILinkedList> linkedlist(thread, result);
        return linkedlist;
    }

    JSTaggedValue LinkedListInsert(JSHandle<JSAPILinkedList> linkedlist, JSTaggedValue index, JSTaggedValue value)
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedlist.GetTaggedValue());
        callInfo->SetCallArg(0, index);
        callInfo->SetCallArg(1, value);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLinkedList::Insert(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }

    JSTaggedValue LinkedListGet(JSHandle<JSAPILinkedList> linkedlist, JSTaggedValue index)
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedlist.GetTaggedValue());
        callInfo->SetCallArg(0, index);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLinkedList::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }

    JSTaggedValue LinkedListRemoveByIndex(JSHandle<JSAPILinkedList> linkedlist, JSTaggedValue index)
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedlist.GetTaggedValue());
        callInfo->SetCallArg(0, index);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLinkedList::RemoveByIndex(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }
};

HWTEST_F_L0(ContainersLinkedListTest, LinkedListConstructor)
{
    InitializeLinkedListConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeLinkedListConstructor());

    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersLinkedList::LinkedListConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsJSAPILinkedList());
    JSHandle<JSAPILinkedList> list(thread, result);
    JSTaggedValue resultProto = JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(list));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype();
    ASSERT_EQ(resultProto, funcProto);
    int size = list->Length();
    ASSERT_EQ(size, 0);

    // test PlainArrayConstructor exception
    objCallInfo->SetNewTarget(JSTaggedValue::Undefined());
    CONTAINERS_API_EXCEPTION_TEST(ContainersLinkedList, LinkedListConstructor, objCallInfo);
}

HWTEST_F_L0(ContainersLinkedListTest, InsertAndGet)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSTaggedValue result = JSTaggedValue::Hole();
    JSHandle<JSAPILinkedList> linkedlist = CreateJSAPILinkedList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        result = LinkedListInsert(linkedlist, JSTaggedValue(i), JSTaggedValue(5));
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(linkedlist->Length(), static_cast<int>(i + 1));
    }
    // Insert in position 0(first) with value 10
    result = LinkedListInsert(linkedlist, JSTaggedValue(0), JSTaggedValue(10));
    EXPECT_EQ(result, JSTaggedValue::True());
    EXPECT_EQ(linkedlist->Length(), static_cast<int>(NODE_NUMBERS + 1));

    // Insert in position NODE_NUMBERS / 2(middle) with value 10
    result = LinkedListInsert(linkedlist, JSTaggedValue(NODE_NUMBERS / 2), JSTaggedValue(10));
    EXPECT_EQ(result, JSTaggedValue::True());
    EXPECT_EQ(linkedlist->Length(), static_cast<int>(NODE_NUMBERS + 2));

    // Insert in position NODE_NUMBERS + 2(last) with value 10
    result = LinkedListInsert(linkedlist, JSTaggedValue(NODE_NUMBERS + 2), JSTaggedValue(10));
    EXPECT_EQ(result, JSTaggedValue::True());
    EXPECT_EQ(linkedlist->Length(), static_cast<int>(NODE_NUMBERS + 3));

    uint32_t length = static_cast<uint32_t>(linkedlist->Length());
    for (uint32_t i = 0; i < length; i++) {
        if (i == 0) {
            result = LinkedListGet(linkedlist, JSTaggedValue(i));
            EXPECT_EQ(result, JSTaggedValue(10));
        } else if (i == NODE_NUMBERS / 2) {
            result = LinkedListGet(linkedlist, JSTaggedValue(i));
            EXPECT_EQ(result, JSTaggedValue(10));
        } else if (i == NODE_NUMBERS + 2) {
            result = LinkedListGet(linkedlist, JSTaggedValue(i));
            EXPECT_EQ(result, JSTaggedValue(10));
        } else {
            result = LinkedListGet(linkedlist, JSTaggedValue(i));
            EXPECT_EQ(result, JSTaggedValue(5));
        }
    }
}

HWTEST_F_L0(ContainersLinkedListTest, Remove)
{
    constexpr uint32_t NODE_NUMBERS = 20;
    JSTaggedValue result = JSTaggedValue::Hole();
    JSHandle<JSAPILinkedList> linkedlist = CreateJSAPILinkedList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        result = LinkedListInsert(linkedlist, JSTaggedValue(i), JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(linkedlist->Length(), static_cast<int>(i + 1));
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedlist.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(NODE_NUMBERS / 2));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue rvalue = ContainersLinkedList::Remove(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(rvalue, JSTaggedValue::True());
        EXPECT_EQ(linkedlist->Length(), static_cast<int>(NODE_NUMBERS - 1));
    }
}

HWTEST_F_L0(ContainersLinkedListTest, RemoveByIndex)
{
    constexpr uint32_t NODE_NUMBERS = 20;
    JSTaggedValue result = JSTaggedValue::Hole();
    JSHandle<JSAPILinkedList> linkedlist = CreateJSAPILinkedList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        result = LinkedListInsert(linkedlist, JSTaggedValue(i), JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(linkedlist->Length(), static_cast<int>(i + 1));
    }

    // Remove index > (NODE_NUMBERS / 2)
    result = LinkedListRemoveByIndex(linkedlist, JSTaggedValue(16));
    EXPECT_EQ(result, JSTaggedValue(16));
    EXPECT_EQ(linkedlist->Length(), static_cast<int>(NODE_NUMBERS - 1));

    // Remove index < (NODE_NUMBERS / 2)
    result = LinkedListRemoveByIndex(linkedlist, JSTaggedValue(6));
    EXPECT_EQ(result, JSTaggedValue(6));
    EXPECT_EQ(linkedlist->Length(), static_cast<int>(NODE_NUMBERS - 2));
}

HWTEST_F_L0(ContainersLinkedListTest, RemoveFirst)
{
    constexpr uint32_t NODE_NUMBERS = 20;
    JSTaggedValue result = JSTaggedValue::Hole();
    JSHandle<JSAPILinkedList> linkedlist = CreateJSAPILinkedList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        result = LinkedListInsert(linkedlist, JSTaggedValue(i), JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(linkedlist->Length(), static_cast<int>(i + 1));
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedlist.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue rvalue = ContainersLinkedList::RemoveFirst(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(rvalue, JSTaggedValue(0));
        EXPECT_EQ(linkedlist->Length(), static_cast<int>(NODE_NUMBERS - 1));
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedlist.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(15));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue rvalue = ContainersLinkedList::RemoveFirstFound(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(rvalue, JSTaggedValue::True());
        EXPECT_EQ(linkedlist->Length(), static_cast<int>(NODE_NUMBERS - 2));
    }
}

HWTEST_F_L0(ContainersLinkedListTest, RemoveLast)
{
    constexpr uint32_t NODE_NUMBERS = 20;
    JSTaggedValue result = JSTaggedValue::Hole();
    JSHandle<JSAPILinkedList> linkedlist = CreateJSAPILinkedList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        result = LinkedListInsert(linkedlist, JSTaggedValue(i), JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(linkedlist->Length(), static_cast<int>(i + 1));
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedlist.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue rvalue = ContainersLinkedList::RemoveLast(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(rvalue, JSTaggedValue(19));
        EXPECT_EQ(linkedlist->Length(), static_cast<int>(NODE_NUMBERS - 1));
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedlist.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(8));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue rvalue = ContainersLinkedList::RemoveLastFound(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(rvalue, JSTaggedValue::True());
        EXPECT_EQ(linkedlist->Length(), static_cast<int>(NODE_NUMBERS - 2));
    }
}

HWTEST_F_L0(ContainersLinkedListTest, Clear)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPILinkedList> linkedlist = CreateJSAPILinkedList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedlist.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLinkedList::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(linkedlist->Length(), static_cast<int>(i + 1));
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedlist.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersLinkedList::Clear(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(linkedlist->Length(), 0);
    }
}

HWTEST_F_L0(ContainersLinkedListTest, Clone)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPILinkedList> linkedList = CreateJSAPILinkedList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedList.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLinkedList::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(linkedList->Length(), static_cast<int>(i + 1));
    }

    linkedList->Dump();

    auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo1->SetFunction(JSTaggedValue::Undefined());
    callInfo1->SetThis(linkedList.GetTaggedValue());
    JSTaggedValue newlinkedList = ContainersLinkedList::Clone(callInfo1);
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(newlinkedList);
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLinkedList::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i));
    }
}

HWTEST_F_L0(ContainersLinkedListTest, Values)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPILinkedList> linkedlist = CreateJSAPILinkedList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedlist.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLinkedList::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(linkedlist->Length(), static_cast<int>(i + 1));
    }

    // test values
    auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo1->SetFunction(JSTaggedValue::Undefined());
    callInfo1->SetThis(linkedlist.GetTaggedValue());
    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
    JSHandle<JSTaggedValue> iterValues(thread, ContainersLinkedList::GetIteratorObj(callInfo1));
    TestHelper::TearDownFrame(thread, prev1);
    EXPECT_TRUE(iterValues->IsJSAPILinkedListIterator());

    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(iterValues.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        result.Update(JSAPILinkedListIterator::Next(callInfo));
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(static_cast<int>(i), JSIterator::IteratorValue(thread, result)->GetInt());
    }
}

HWTEST_F_L0(ContainersLinkedListTest, ForEach)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPILinkedList> linkedlist = CreateJSAPILinkedList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedlist.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLinkedList::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(linkedlist->Length(), static_cast<int>(i + 1));
    }
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPILinkedList> newLinkedlist = CreateJSAPILinkedList();
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedlist.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, newLinkedlist.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersLinkedList::ForEach(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(linkedlist.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersLinkedList::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(i * 2));
    }
}

HWTEST_F_L0(ContainersLinkedListTest, ProxyOfLength)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPILinkedList> linkedlist = CreateJSAPILinkedList();
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    JSHandle<JSProxy> proxy = CreateJSProxyHandle(thread);
    proxy->SetTarget(thread, linkedlist.GetTaggedValue());
    callInfo->SetThis(proxy.GetTaggedValue());

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersLinkedList::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue retult = ContainersLinkedList::Length(callInfo);
        TestHelper::TearDownFrame(thread, prev1);
        EXPECT_EQ(retult, JSTaggedValue(i + 1));
    }
}

HWTEST_F_L0(ContainersLinkedListTest, ExceptionReturn1)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, Insert);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, Get);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, Add);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, AddFirst);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, GetFirst);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, GetLast);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, Length);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, Clear);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, Clone);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, Has);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, GetIndexOf);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, GetLastIndexOf);

    JSHandle<JSAPILinkedList> linkedlist = CreateJSAPILinkedList();
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(linkedlist.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersLinkedList, Insert, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(linkedlist.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersLinkedList, Get, callInfo);
    }
}

HWTEST_F_L0(ContainersLinkedListTest, ExceptionReturn2)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, RemoveByIndex);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, Set);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, Remove);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, RemoveFirst);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, RemoveFirstFound);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, RemoveLast);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, RemoveLastFound);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, ConvertToArray);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersLinkedList, ForEach);

    JSHandle<JSAPILinkedList> linkedlist = CreateJSAPILinkedList();
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(linkedlist.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersLinkedList, RemoveByIndex, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(linkedlist.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersLinkedList, Set, callInfo);
    }
}
}  // namespace panda::test
