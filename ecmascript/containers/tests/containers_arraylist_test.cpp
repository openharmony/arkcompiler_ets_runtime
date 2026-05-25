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


#include "ecmascript/containers/containers_arraylist.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_arraylist.h"
#include "ecmascript/js_api/js_api_arraylist_iterator.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/containers/tests/containers_test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

// Static state for sort callback re-entrancy tests
static uintptr_t g_sortTargetSlot = 0;
static int g_sortCompareCount = 0;

namespace panda::test {
class ContainersArrayListTest : public testing::Test {
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
            JSHandle<JSTaggedValue> arrayList = GetCallArg(argv, 1);
            if (!arrayList->IsUndefined()) {
                if (value->IsNumber()) {
                    TaggedArray *elements = TaggedArray::Cast(JSAPIArrayList::Cast(arrayList.GetTaggedValue().
                                            GetTaggedObject())->GetElements(thread).GetTaggedObject());
                    JSTaggedValue result = elements->Get(thread, value->GetInt());
                    EXPECT_EQ(result, value.GetTaggedValue());
                }
            }
            return JSTaggedValue::True();
        }

        static JSTaggedValue TestSortDescFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> a = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> b = GetCallArg(argv, 1);
            if (a->IsNumber() && b->IsNumber()) {
                double diff = b->GetNumber() - a->GetNumber();
                return JSTaggedValue(diff);
            }
            return JSTaggedValue(0);
        }

        static JSTaggedValue TestSortAscFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> a = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> b = GetCallArg(argv, 1);
            if (a->IsNumber() && b->IsNumber()) {
                double diff = a->GetNumber() - b->GetNumber();
                return JSTaggedValue(diff);
            }
            return JSTaggedValue(0);
        }

        // Comparator that removes 3 elements from the end on the first call.
        // Tests the else-branch logical-length restoration path.
        static JSTaggedValue TestSortWithRemoveFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> a = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> b = GetCallArg(argv, 1);
            if (g_sortCompareCount++ == 0 && g_sortTargetSlot != 0) {
                JSHandle<JSTaggedValue> containerVal(g_sortTargetSlot);
                JSHandle<JSAPIArrayList> arrayList = JSHandle<JSAPIArrayList>::Cast(containerVal);
                for (int i = 0; i < 3; i++) { // 3: remove 3 elements
                    int idx = static_cast<int>(arrayList->GetSize(thread)) - 1;
                    JSAPIArrayList::RemoveByIndex(thread, arrayList, idx);
                }
            }
            if (a->IsNumber() && b->IsNumber()) {
                return JSTaggedValue(a->GetNumber() - b->GetNumber());
            }
            return JSTaggedValue(0);
        }

        // Comparator that removes all but one element and trims the backing store.
        // Tests the if-branch (dstElements->GetLength() < length) re-allocation path.
        static JSTaggedValue TestSortWithTrimFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> a = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> b = GetCallArg(argv, 1);
            if (g_sortCompareCount++ == 0 && g_sortTargetSlot != 0) {
                JSHandle<JSTaggedValue> containerVal(g_sortTargetSlot);
                JSHandle<JSAPIArrayList> arrayList = JSHandle<JSAPIArrayList>::Cast(containerVal);
                while (arrayList->GetSize(thread) > 1) {
                    int idx = static_cast<int>(arrayList->GetSize(thread)) - 1;
                    JSAPIArrayList::RemoveByIndex(thread, arrayList, idx);
                }
                JSAPIArrayList::TrimToCurrentLength(thread, arrayList);
            }
            if (a->IsNumber() && b->IsNumber()) {
                return JSTaggedValue(a->GetNumber() - b->GetNumber());
            }
            return JSTaggedValue(0);
        }

        // Comparator that adds 5 extra elements on the first call.
        // Tests the else-branch where currLen > length (added elements preserved).
        static JSTaggedValue TestSortWithAddFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> a = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> b = GetCallArg(argv, 1);
            if (g_sortCompareCount++ == 0 && g_sortTargetSlot != 0) {
                JSHandle<JSTaggedValue> containerVal(g_sortTargetSlot);
                JSHandle<JSAPIArrayList> arrayList = JSHandle<JSAPIArrayList>::Cast(containerVal);
                for (int i = 0; i < 5; i++) { // 5: add 5 extra elements
                    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(999)); // 999 extra 999s preserved
                    JSAPIArrayList::Add(thread, arrayList, value);
                }
            }
            if (a->IsNumber() && b->IsNumber()) {
                return JSTaggedValue(a->GetNumber() - b->GetNumber());
            }
            return JSTaggedValue(0);
        }
    };
protected:
    JSTaggedValue InitializeArrayListConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();
        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 6 means the value
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(
            0, JSTaggedValue(static_cast<int>(ContainerTag::ArrayList))); // 0 means the argument
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPIArrayList> CreateJSAPIArrayList()
    {
        JSHandle<JSFunction> newTarget(thread, InitializeArrayListConstructor());
        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4); // 4 means the value
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersArrayList::ArrayListConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIArrayList> ArrayList(thread, result);
        return ArrayList;
    }

    JSTaggedValue ArrayListAdd(JSHandle<JSAPIArrayList> arrayList, JSTaggedValue value)
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 4 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(arrayList.GetTaggedValue());
        callInfo->SetCallArg(0, value);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersArrayList::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }

    JSTaggedValue ArrayListRemoveByRange(JSHandle<JSAPIArrayList> arrayList, JSTaggedValue startIndex,
                                         JSTaggedValue endIndex)
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 6 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(arrayList.GetTaggedValue());
        callInfo->SetCallArg(0, startIndex);
        callInfo->SetCallArg(1, endIndex);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersArrayList::RemoveByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }

    JSTaggedValue ArrayListSubArrayList(JSHandle<JSAPIArrayList> arrayList, JSTaggedValue startIndex,
                                         JSTaggedValue endIndex)
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(arrayList.GetTaggedValue());
        callInfo->SetCallArg(0, startIndex);
        callInfo->SetCallArg(1, endIndex);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersArrayList::SubArrayList(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }
};

HWTEST_F_L0(ContainersArrayListTest, ArrayListConstructor)
{
    InitializeArrayListConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeArrayListConstructor());
    auto objCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);   // 4 means the value
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersArrayList::ArrayListConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsJSAPIArrayList());
    JSHandle<JSAPIArrayList> arrayList(thread, result);
    JSTaggedValue resultProto = JSObject::GetPrototype(thread, JSHandle<JSObject>::Cast(arrayList));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype(thread);
    ASSERT_EQ(resultProto, funcProto);
    int length = arrayList->GetLength(thread).GetArrayLength();
    ASSERT_EQ(length, 0);   // 0 means the value

    // test ArrayListConstructor exception
    objCallInfo->SetNewTarget(JSTaggedValue::Undefined());
    CONTAINERS_API_EXCEPTION_TEST(ContainersArrayList, ArrayListConstructor, objCallInfo);
}

HWTEST_F_L0(ContainersArrayListTest, RemoveByRange)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        JSTaggedValue result = ArrayListAdd(arrayList, JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(arrayList->GetSize(thread), static_cast<int>(i + 1));
    }

    // remove success
    {
        JSTaggedValue result = ArrayListRemoveByRange(arrayList, JSTaggedValue(1), JSTaggedValue(3));
        EXPECT_EQ(result, JSTaggedValue::Undefined());
        EXPECT_EQ(arrayList->GetSize(thread), static_cast<int>(NODE_NUMBERS - 2));
        for (uint32_t i = 0; i < NODE_NUMBERS - 2; i++) {
            if (i < 1) {
                EXPECT_EQ(arrayList->Get(thread, i), JSTaggedValue(i));
            } else {
                EXPECT_EQ(arrayList->Get(thread, i), JSTaggedValue(i + 2));
            }
        }
    }

    // input startIndex type error
    {
        JSTaggedValue result = ArrayListRemoveByRange(arrayList, JSTaggedValue::Undefined(), JSTaggedValue(3));
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }

    // input endIndex type error
    {
        JSTaggedValue result = ArrayListRemoveByRange(arrayList, JSTaggedValue(1), JSTaggedValue::Undefined());
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }
}

HWTEST_F_L0(ContainersArrayListTest, SubArrayList)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        JSTaggedValue result = ArrayListAdd(arrayList, JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
        EXPECT_EQ(arrayList->GetSize(thread), static_cast<int>(i + 1));
    }

    // input startIndex type error
    {
        JSTaggedValue result = ArrayListSubArrayList(arrayList, JSTaggedValue::Undefined(), JSTaggedValue(2));
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }

    // input endIndex type error
    {
        JSTaggedValue result = ArrayListSubArrayList(arrayList, JSTaggedValue(2), JSTaggedValue::Undefined());
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }

    // success
    {
        JSTaggedValue newArrayList = ArrayListSubArrayList(arrayList, JSTaggedValue(1), JSTaggedValue(3));
        JSHandle<TaggedArray> elements(thread,
            JSAPIArrayList::Cast(newArrayList.GetTaggedObject())->GetElements(thread));
        EXPECT_EQ(elements->GetLength(), static_cast<uint32_t>(2)); // length = 3 - 1
        EXPECT_EQ(elements->Get(thread, 0), JSTaggedValue(1));
        EXPECT_EQ(elements->Get(thread, 1), JSTaggedValue(2));
    }
}

HWTEST_F_L0(ContainersArrayListTest, GetAndSet)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(arrayList.GetTaggedValue());

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo->SetCallArg(0, JSTaggedValue(i));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersArrayList::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo->SetCallArg(0, JSTaggedValue(i));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue retult = ContainersArrayList::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(retult, JSTaggedValue(i));
    }

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(-i - 1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersArrayList::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo->SetCallArg(0, JSTaggedValue(i));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue retult = ContainersArrayList::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(retult, JSTaggedValue(-i - 1));
    }
}

HWTEST_F_L0(ContainersArrayListTest, ProxyOfGetSetAndGetSize)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIArrayList> proxyArrayList = CreateJSAPIArrayList();
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    JSHandle<JSProxy> proxy = CreateJSProxyHandle(thread);
    proxy->SetTarget(thread, proxyArrayList.GetTaggedValue());
    callInfo->SetThis(proxy.GetTaggedValue());

    // ArrayList proxy GetSize
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo->SetCallArg(0, JSTaggedValue(i));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersArrayList::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue retult = ContainersArrayList::GetSize(callInfo);
        TestHelper::TearDownFrame(thread, prev1);
        EXPECT_EQ(retult, JSTaggedValue(i + 1));
    }

    // ArrayList proxy Set
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(-i - 1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersArrayList::Set(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    // ArrayList proxy Get and verify
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo->SetCallArg(0, JSTaggedValue(i));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue retult = ContainersArrayList::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(retult, JSTaggedValue(-i - 1));
    }
}

HWTEST_F_L0(ContainersArrayListTest, ExceptionReturn1)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, Insert);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, IncreaseCapacityTo);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, Add);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, Clear);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, Clone);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, Has);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, GetCapacity);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, TrimToCurrentLength);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, Get);

    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(arrayList.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersArrayList, Insert, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(arrayList.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersArrayList, IncreaseCapacityTo, callInfo);
    }
}

HWTEST_F_L0(ContainersArrayListTest, ExceptionReturn2)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, RemoveByIndex);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, ReplaceAllElements);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, GetIndexOf);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, IsEmpty);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, GetLastIndexOf);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, Remove);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, RemoveByRange);

    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(arrayList.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersArrayList, RemoveByIndex, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(arrayList.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersArrayList, ReplaceAllElements, callInfo);
    }
}

HWTEST_F_L0(ContainersArrayListTest, ExceptionReturn3)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, Sort);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, Set);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, SubArrayList);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, GetSize);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, ConvertToArray);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, ForEach);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersArrayList, GetIteratorObj);

    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(arrayList.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersArrayList, Sort, callInfo);
    }
}

// sort with default comparator (undefined)
HWTEST_F_L0(ContainersArrayListTest, SortDefaultComparator)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    // insert in reverse order
    for (int32_t i = ELEMENT_NUMS - 1; i >= 0; i--) {
        JSTaggedValue result = ArrayListAdd(arrayList, JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
    }
    EXPECT_EQ(arrayList->GetSize(thread), ELEMENT_NUMS);

    // sort with undefined comparator
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(arrayList.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersArrayList::Sort(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            EXPECT_EQ(arrayList->Get(thread, i), JSTaggedValue(i));
        }
    }
}

// sort with custom comparator
HWTEST_F_L0(ContainersArrayListTest, SortWithComparator)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        JSTaggedValue result = ArrayListAdd(arrayList, JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
    }

    // sort with custom descending comparator
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestSortDescFunc));
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(arrayList.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersArrayList::Sort(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            EXPECT_EQ(arrayList->Get(thread, i), JSTaggedValue(ELEMENT_NUMS - 1 - i));
        }
    }
}

// sort on empty and single-element containers
HWTEST_F_L0(ContainersArrayListTest, SortEmptyAndSingleElement)
{
    // empty
    {
        JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(arrayList.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersArrayList::Sort(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        EXPECT_EQ(arrayList->GetSize(thread), 0);
    }
    // single element
    {
        JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
        JSTaggedValue result = ArrayListAdd(arrayList, JSTaggedValue(42));
        EXPECT_EQ(result, JSTaggedValue::True());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(arrayList.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersArrayList::Sort(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        EXPECT_EQ(arrayList->GetSize(thread), 1);
        EXPECT_EQ(arrayList->Get(thread, 0), JSTaggedValue(42));
    }
}

// sort preserves container size and handles duplicate elements
HWTEST_F_L0(ContainersArrayListTest, SortDuplicateElements)
{
    constexpr int32_t ELEMENT_NUMS = 6;
    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    // insert: 3, 1, 2, 1, 3, 2
    int32_t values[ELEMENT_NUMS] = {3, 1, 2, 1, 3, 2};
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        JSTaggedValue result = ArrayListAdd(arrayList, JSTaggedValue(values[i]));
        EXPECT_EQ(result, JSTaggedValue::True());
    }
    EXPECT_EQ(arrayList->GetSize(thread), ELEMENT_NUMS);

    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(arrayList.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    ContainersArrayList::Sort(callInfo);
    TestHelper::TearDownFrame(thread, prev);

    // size unchanged after sort
    EXPECT_EQ(arrayList->GetSize(thread), ELEMENT_NUMS);
    // sorted result: 1, 1, 2, 2, 3, 3
    int32_t expected[ELEMENT_NUMS] = {1, 1, 2, 2, 3, 3};
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        EXPECT_EQ(arrayList->Get(thread, i), JSTaggedValue(expected[i]));
    }
}

// sort large array to trigger TimSort merge path (> MIN_MERGE=32)
HWTEST_F_L0(ContainersArrayListTest, SortLargeArray)
{
    constexpr int32_t ELEMENT_NUMS = 64;
    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    // insert in reverse order
    for (int32_t i = ELEMENT_NUMS - 1; i >= 0; i--) {
        JSTaggedValue result = ArrayListAdd(arrayList, JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
    }
    EXPECT_EQ(arrayList->GetSize(thread), ELEMENT_NUMS);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestSortAscFunc));
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(arrayList.GetTaggedValue());
    callInfo->SetCallArg(0, func.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    ContainersArrayList::Sort(callInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(arrayList->GetSize(thread), ELEMENT_NUMS);
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        EXPECT_EQ(arrayList->Get(thread, i), JSTaggedValue(i));
    }
}

// sort already-sorted container (idempotent)
HWTEST_F_L0(ContainersArrayListTest, SortAlreadySorted)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        JSTaggedValue result = ArrayListAdd(arrayList, JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
    }

    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(arrayList.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    ContainersArrayList::Sort(callInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(arrayList->GetSize(thread), ELEMENT_NUMS);
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        EXPECT_EQ(arrayList->Get(thread, i), JSTaggedValue(i));
    }
}

// sort with two-element container (boundary between <2 early return and full sort)
HWTEST_F_L0(ContainersArrayListTest, SortTwoElements)
{
    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    JSTaggedValue r1 = ArrayListAdd(arrayList, JSTaggedValue(5));
    JSTaggedValue r2 = ArrayListAdd(arrayList, JSTaggedValue(3));
    EXPECT_EQ(r1, JSTaggedValue::True());
    EXPECT_EQ(r2, JSTaggedValue::True());

    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(arrayList.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    ContainersArrayList::Sort(callInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(arrayList->GetSize(thread), 2);
    EXPECT_EQ(arrayList->Get(thread, 0), JSTaggedValue(3));
    EXPECT_EQ(arrayList->Get(thread, 1), JSTaggedValue(5));
}

// sort with removeByIndex during comparator callback (tests logical length restoration)
HWTEST_F_L0(ContainersArrayListTest, SortWithRemoveDuringCallback)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    for (int32_t i = ELEMENT_NUMS - 1; i >= 0; i--) {
        JSTaggedValue result = ArrayListAdd(arrayList, JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
    }
    EXPECT_EQ(arrayList->GetSize(thread), ELEMENT_NUMS);

    JSHandle<JSTaggedValue> containerHandle(thread, arrayList.GetTaggedValue());
    g_sortTargetSlot = containerHandle.GetAddress();
    g_sortCompareCount = 0;

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> func =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestSortWithRemoveFunc));
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(arrayList.GetTaggedValue());
    callInfo->SetCallArg(0, func.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    ContainersArrayList::Sort(callInfo);
    TestHelper::TearDownFrame(thread, prev);

    // Sort should complete safely: 8 sorted elements written back, length restored
    EXPECT_EQ(arrayList->GetSize(thread), ELEMENT_NUMS);
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        EXPECT_EQ(arrayList->Get(thread, i), JSTaggedValue(i));
    }
    g_sortTargetSlot = 0;
}

// sort with trimToCurrentLength during comparator callback (tests backing store re-allocation)
HWTEST_F_L0(ContainersArrayListTest, SortWithTrimDuringCallback)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    for (int32_t i = ELEMENT_NUMS - 1; i >= 0; i--) {
        JSTaggedValue result = ArrayListAdd(arrayList, JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
    }
    EXPECT_EQ(arrayList->GetSize(thread), ELEMENT_NUMS);

    JSHandle<JSTaggedValue> containerHandle(thread, arrayList.GetTaggedValue());
    g_sortTargetSlot = containerHandle.GetAddress();
    g_sortCompareCount = 0;

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> func =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestSortWithTrimFunc));
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(arrayList.GetTaggedValue());
    callInfo->SetCallArg(0, func.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    ContainersArrayList::Sort(callInfo);
    TestHelper::TearDownFrame(thread, prev);

    // Backing store was trimmed during callback; new array should have been allocated
    EXPECT_EQ(arrayList->GetSize(thread), ELEMENT_NUMS);
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        EXPECT_EQ(arrayList->Get(thread, i), JSTaggedValue(i));
    }
    g_sortTargetSlot = 0;
}

// sort with add during comparator callback (tests preservation of added elements)
HWTEST_F_L0(ContainersArrayListTest, SortWithAddDuringCallback)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    constexpr int32_t EXTRA = 5;
    JSHandle<JSAPIArrayList> arrayList = CreateJSAPIArrayList();
    for (int32_t i = ELEMENT_NUMS - 1; i >= 0; i--) {
        JSTaggedValue result = ArrayListAdd(arrayList, JSTaggedValue(i));
        EXPECT_EQ(result, JSTaggedValue::True());
    }
    EXPECT_EQ(arrayList->GetSize(thread), ELEMENT_NUMS);

    JSHandle<JSTaggedValue> containerHandle(thread, arrayList.GetTaggedValue());
    g_sortTargetSlot = containerHandle.GetAddress();
    g_sortCompareCount = 0;

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> func =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestSortWithAddFunc));
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(arrayList.GetTaggedValue());
    callInfo->SetCallArg(0, func.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    ContainersArrayList::Sort(callInfo);
    TestHelper::TearDownFrame(thread, prev);

    // First 8 should be sorted [0..7], then 5 extra 999s preserved
    EXPECT_EQ(arrayList->GetSize(thread), ELEMENT_NUMS + EXTRA);
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        EXPECT_EQ(arrayList->Get(thread, i), JSTaggedValue(i));
    }
    for (int32_t i = 0; i < EXTRA; i++) {
        EXPECT_EQ(arrayList->Get(thread, ELEMENT_NUMS + i), JSTaggedValue(999)); // 999 extra 999s preserved
    }
    g_sortTargetSlot = 0;
}
}  // namespace panda::test
