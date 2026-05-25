/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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
#include "ecmascript/containers/containers_vector.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_vector.h"
#include "ecmascript/js_api/js_api_vector_iterator.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_object-inl.h"
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
class ContainersVectorTest : public testing::Test {
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
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 1);
            JSHandle<JSTaggedValue> vector = GetCallArg(argv, 2); // 2 means the secode arg
            if (!vector->IsUndefined()) {
                if (value->IsNumber()) {
                    TaggedArray *elements = TaggedArray::Cast(JSAPIVector::Cast(vector.GetTaggedValue().
                                            GetTaggedObject())->GetElements(thread).GetTaggedObject());
                    JSTaggedValue result = elements->Get(thread, key->GetInt());
                    EXPECT_EQ(result, value.GetTaggedValue());
                }
            }
            return JSTaggedValue::Undefined();
        }

        static JSTaggedValue TestReplaceAllElementsFunc(EcmaRuntimeCallInfo *argv)
        {
            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 0);
            JSHandle<JSTaggedValue> index = GetCallArg(argv, 1);
            JSHandle<JSTaggedValue> vector = GetCallArg(argv, 2); // 2 means the secode arg
            if (!vector->IsUndefined()) {
                if (value->IsNumber()) {
                    JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(value->GetInt() * 2)); // 2 means mul by 2
                    JSHandle<JSAPIVector>::Cast(vector)->Set(thread, index->GetNumber(), newValue.GetTaggedValue());
                    return newValue.GetTaggedValue();
                }
            }
            return JSTaggedValue::Undefined();
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
                JSHandle<JSAPIVector> vector = JSHandle<JSAPIVector>::Cast(containerVal);
                for (int i = 0; i < 3; i++) { // 3: remove 3 elements
                    int32_t idx = static_cast<int32_t>(vector->GetSize()) - 1;
                    JSAPIVector::RemoveByIndex(thread, vector, idx);
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
                JSHandle<JSAPIVector> vector = JSHandle<JSAPIVector>::Cast(containerVal);
                while (vector->GetSize() > 1) {
                    int32_t idx = static_cast<int32_t>(vector->GetSize()) - 1;
                    JSAPIVector::RemoveByIndex(thread, vector, idx);
                }
                JSAPIVector::TrimToCurrentLength(thread, vector);
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
                JSHandle<JSAPIVector> vector = JSHandle<JSAPIVector>::Cast(containerVal);
                for (int i = 0; i < 5; i++) { // 5: add 5 extra elements
                    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(999)); // 999 extra 999s preserved
                    JSAPIVector::Add(thread, vector, value);
                }
            }
            if (a->IsNumber() && b->IsNumber()) {
                return JSTaggedValue(a->GetNumber() - b->GetNumber());
            }
            return JSTaggedValue(0);
        }
    };
protected:
    JSTaggedValue InitializeVectorConstructor()
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
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::Vector)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPIVector> CreateJSAPIVector(JSTaggedValue compare = JSTaggedValue::Undefined())
    {
        JSHandle<JSTaggedValue> compareHandle(thread, compare);
        JSHandle<JSFunction> newTarget(thread, InitializeVectorConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, compareHandle.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersVector::VectorConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIVector> vector(thread, result);
        return vector;
    }
};

// new vector
HWTEST_F_L0(ContainersVectorTest, VectorConstructor)
{
    // Initialize twice and return directly the second time
    InitializeVectorConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeVectorConstructor());

    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersVector::VectorConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.IsJSAPIVector());
    
    JSHandle<JSAPIVector> setHandle(thread, result);
    JSTaggedValue resultProto = JSTaggedValue::GetPrototype(thread, JSHandle<JSTaggedValue>(setHandle));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype(thread);
    ASSERT_EQ(resultProto, funcProto);
    int size = setHandle->GetSize();
    ASSERT_EQ(size, 0);
     
    // test VectorConstructor exception
    objCallInfo->SetNewTarget(JSTaggedValue::Undefined());
    CONTAINERS_API_EXCEPTION_TEST(ContainersVector, VectorConstructor, objCallInfo);
}

// add  has
HWTEST_F_L0(ContainersVectorTest, AddAndHas)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), i + 1);
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);

    // test add insert string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    std::string myKey("mykey");
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS + i + 1);
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS * 2);

    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::Insert(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS * 2 + i + 1);
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS * 3);

    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        std::string ikey = myKey + std::to_string(i);
        key.Update(factory->NewFromStdString(ikey).GetTaggedValue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
}

// getIndexOf  getLastIndexOf
HWTEST_F_L0(ContainersVectorTest, GetFirstValueAndGetLastValue)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        // double
        result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), 2 * (i + 1));
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS * 2);

    // getIndexOf
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetIndexOf(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(2 * i));
    }
    // getLastIndexOf
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetLastIndexOf(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(2 * i + 1));
    }
    // getFirstElement
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetFirstElement(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
    }
    // getLastElement
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetLastElement(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(7));
    }
}

// removeByIndex  remove  removeByRange  clear  isEmpty
HWTEST_F_L0(ContainersVectorTest, RemoveByIndexAndRemoveAndRemoveRangeAndClear)
{
    // add
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), i + 1);
    }

    // removeByIndex
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::RemoveByIndex(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(0));
        EXPECT_EQ(vector->GetSize(), 7);
    }
    // remove fail
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Remove(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsFalse());
        EXPECT_EQ(vector->GetSize(), 7);
    }
    // remove success
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Remove(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), 6);
    }
    // removeByRange
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(4));
        callInfo->SetCallArg(1, JSTaggedValue(2));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::RemoveByRange(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(vector->GetSize(), 6);
    }
    // isEmpty
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::IsEmpty(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsFalse());
    }
    // clear
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::Clear(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(vector->GetSize(), 0);
    }
    // isEmpty
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::IsEmpty(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
}

// replaceAllElements  forEach
HWTEST_F_L0(ContainersVectorTest, ReplaceAllElementsAndForEach)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), i + 1);
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);

    // forEach
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSAPIVector> vec = CreateJSAPIVector();
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());
        callInfo->SetCallArg(1, vec.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::ForEach(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
            JSTaggedValue result = ContainersVector::Get(callInfo1);
            TestHelper::TearDownFrame(thread, prev1);
            EXPECT_EQ(result, JSTaggedValue(i));
        }
    }

    // replaceAllElements
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        JSHandle<JSFunction> func =
            factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestReplaceAllElementsFunc));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::ReplaceAllElements(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
            JSTaggedValue result = ContainersVector::Get(callInfo1);
            TestHelper::TearDownFrame(thread, prev1);
            EXPECT_EQ(result, JSTaggedValue(i * 2));
        }
    }
}

// sort
HWTEST_F_L0(ContainersVectorTest, Sort)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = ELEMENT_NUMS - 1; i >= 0; i--) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);

    // sort
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::Sort(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
            JSTaggedValue result = ContainersVector::Get(callInfo1);
            TestHelper::TearDownFrame(thread, prev1);
            EXPECT_EQ(result, JSTaggedValue(i));
        }
    }
}

// sort with custom descending comparator
HWTEST_F_L0(ContainersVectorTest, SortWithComparator)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestSortDescFunc));
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, func.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::Sort(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vector.GetTaggedValue());
            callInfo1->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
            JSTaggedValue result = ContainersVector::Get(callInfo1);
            TestHelper::TearDownFrame(thread, prev1);
            EXPECT_EQ(result, JSTaggedValue(ELEMENT_NUMS - 1 - i));
        }
    }
}

// sort on empty and single-element containers
HWTEST_F_L0(ContainersVectorTest, SortEmptyAndSingleElement)
{
    // empty
    {
        JSHandle<JSAPIVector> vector = CreateJSAPIVector();
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::Sort(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        EXPECT_EQ(vector->GetSize(), 0);
    }
    // single element
    {
        JSHandle<JSAPIVector> vector = CreateJSAPIVector();
        auto callInfoAdd = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfoAdd->SetFunction(JSTaggedValue::Undefined());
        callInfoAdd->SetThis(vector.GetTaggedValue());
        callInfoAdd->SetCallArg(0, JSTaggedValue(42));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfoAdd);
        JSTaggedValue addResult = ContainersVector::Add(callInfoAdd);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(addResult.IsTrue());

        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::Sort(callInfo);
        TestHelper::TearDownFrame(thread, prev1);

        EXPECT_EQ(vector->GetSize(), 1);
        auto callInfoGet = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfoGet->SetFunction(JSTaggedValue::Undefined());
        callInfoGet->SetThis(vector.GetTaggedValue());
        callInfoGet->SetCallArg(0, JSTaggedValue(0));

        [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, callInfoGet);
        JSTaggedValue getResult = ContainersVector::Get(callInfoGet);
        TestHelper::TearDownFrame(thread, prev2);
        EXPECT_EQ(getResult, JSTaggedValue(42));
    }
}

// sort preserves container size and handles duplicate elements
HWTEST_F_L0(ContainersVectorTest, SortDuplicateElements)
{
    constexpr int32_t ELEMENT_NUMS = 6;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    // insert: 3, 1, 2, 1, 3, 2
    int32_t values[ELEMENT_NUMS] = {3, 1, 2, 1, 3, 2};
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(values[i]));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);

    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(vector.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    ContainersVector::Sort(callInfo);
    TestHelper::TearDownFrame(thread, prev);

    // size unchanged after sort
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);
    // sorted result: 1, 1, 2, 2, 3, 3
    int32_t expected[ELEMENT_NUMS] = {1, 1, 2, 2, 3, 3};
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(vector.GetTaggedValue());
        callInfo1->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
        JSTaggedValue result = ContainersVector::Get(callInfo1);
        TestHelper::TearDownFrame(thread, prev1);
        EXPECT_EQ(result, JSTaggedValue(expected[i]));
    }
}

// clone  convertToArray  copyToArray
HWTEST_F_L0(ContainersVectorTest, CloneAndConvertToArrayAndCopyToArray)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), i + 1);
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);
    // clone
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue vec = ContainersVector::Clone(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIVector> handleVec(thread, vec);
        EXPECT_EQ(handleVec->GetSize(), vector->GetSize());

        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vec);
            callInfo1->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
            JSTaggedValue result = ContainersVector::Get(callInfo1);
            TestHelper::TearDownFrame(thread, prev1);
            EXPECT_EQ(result, JSTaggedValue(i));
        }
    }
    // convertToArray
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue arr = ContainersVector::ConvertToArray(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(arr.IsJSArray());
        JSHandle<JSTaggedValue> handleArr(thread, arr);
        JSHandle<TaggedArray> taggedArr = JSArray::ToTaggedArray(thread, handleArr);
        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            JSTaggedValue result = taggedArr->Get(thread, i);
            EXPECT_EQ(result, JSTaggedValue(i));
        }
    }
    // copyToArray
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSArray> array = factory->NewJSArray();
        JSHandle<TaggedArray> arrayElement = factory->NewTaggedArray(ELEMENT_NUMS, JSTaggedValue::Hole());
        array->SetElements(thread, arrayElement);
        array->SetArrayLength(thread, static_cast<uint32_t>(ELEMENT_NUMS));
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, array.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::CopyToArray(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(array->IsJSArray());
        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(array.GetTaggedValue());
            callInfo1->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
            int result = JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(array), i).GetValue()->GetInt();
            TestHelper::TearDownFrame(thread, prev1);
            EXPECT_EQ(result, i);
        }

        JSHandle<JSArray> longArray = factory->NewJSArray();
        JSHandle<TaggedArray> longArrayElement = factory->NewTaggedArray(ELEMENT_NUMS * 2, JSTaggedValue::Hole());
        longArray->SetElements(thread, longArrayElement);
        longArray->SetArrayLength(thread, static_cast<uint32_t>(ELEMENT_NUMS * 2));
        auto callInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo2->SetFunction(JSTaggedValue::Undefined());
        callInfo2->SetThis(vector.GetTaggedValue());
        callInfo2->SetCallArg(0, longArray.GetTaggedValue());

        [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, callInfo2);
        ContainersVector::CopyToArray(callInfo2);
        TestHelper::TearDownFrame(thread, prev2);
        EXPECT_TRUE(longArray->IsJSArray());
        for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
            auto callInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo3->SetFunction(JSTaggedValue::Undefined());
            callInfo3->SetThis(longArray.GetTaggedValue());
            callInfo3->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev4 = TestHelper::SetupFrame(thread, callInfo3);
            int result1 = JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(longArray), i).GetValue()->GetInt();
            TestHelper::TearDownFrame(thread, prev4);
            EXPECT_EQ(result1, i);
        }
        for (int32_t i = ELEMENT_NUMS; i < ELEMENT_NUMS * 2; i++) {
            auto callInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo4->SetFunction(JSTaggedValue::Undefined());
            callInfo4->SetThis(longArray.GetTaggedValue());
            callInfo4->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev5 = TestHelper::SetupFrame(thread, callInfo4);
            JSHandle<JSTaggedValue> result2 = JSArray::GetProperty(thread,
                JSHandle<JSTaggedValue>(longArray), i).GetValue();
            TestHelper::TearDownFrame(thread, prev5);
            EXPECT_EQ(result2.GetTaggedValue(), JSTaggedValue::Undefined());
        }
    }
}

// subVector  getCapacity  trimToCurrentLength  increaseCapacityTo
HWTEST_F_L0(ContainersVectorTest, SubVectorAndGetCapacityAndTrimToCurrentLengthAndIncreaseCapacityTo)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), i + 1);
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);

    // getCapacity
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        int32_t capacity = ContainersVector::GetCapacity(callInfo).GetInt();
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(capacity, 10);
    }
    // increaseCapacityTo
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(20));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::IncreaseCapacityTo(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    // getCapacity
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        int32_t newCapacity1 = ContainersVector::GetCapacity(callInfo).GetInt();
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(newCapacity1, 20);
    }
    // trimToCurrentLength
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::TrimToCurrentLength(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    // getCapacity
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        int32_t newCapacity2 = ContainersVector::GetCapacity(callInfo).GetInt();
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(newCapacity2, 8);
    }
    // subVector
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(2));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue vec = ContainersVector::SubVector(callInfo);
        for (int32_t i = 0; i < 2; i++) {
            auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
            callInfo1->SetFunction(JSTaggedValue::Undefined());
            callInfo1->SetThis(vec);
            callInfo1->SetCallArg(0, JSTaggedValue(i));

            [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
            JSTaggedValue result = ContainersVector::Get(callInfo1);
            TestHelper::TearDownFrame(thread, prev);
            EXPECT_EQ(result, JSTaggedValue(i));
        }
    }
}

// toString  getLastIndexFrom  getIndexFrom
HWTEST_F_L0(ContainersVectorTest, ToStringAndGetLastIndexFromAndGetIndexFrom)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
        EXPECT_EQ(vector->GetSize(), i + 1);
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);
    // toString
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::ToString(callInfo);
        
        EXPECT_TRUE(result.IsString());
    }
    // getLastIndexFrom
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(3));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetLastIndexFrom(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::SameValue(thread, result, JSTaggedValue(1)));
    }
    // getIndexFrom
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(3));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetIndexFrom(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::SameValue(thread, result, JSTaggedValue(-1)));
    }
    // getIndexFrom
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(10));
        callInfo->SetCallArg(1, JSTaggedValue(0));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetIndexFrom(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::SameValue(thread, result, JSTaggedValue(-1)));
    }
}

HWTEST_F_L0(ContainersVectorTest, ProxyOfGetSizeSetLength)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    JSHandle<JSProxy> proxy = CreateJSProxyHandle(thread);
    proxy->SetTarget(thread, vector.GetTaggedValue());
    callInfo->SetThis(proxy.GetTaggedValue());

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue retult = ContainersVector::GetSize(callInfo);
        TestHelper::TearDownFrame(thread, prev1);
        EXPECT_EQ(retult, JSTaggedValue(i + 1));
    }

    
    // SetLength
    {
        callInfo->SetCallArg(0, JSTaggedValue(NODE_NUMBERS * 2));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::SetLength(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetSize(callInfo);
        TestHelper::TearDownFrame(thread, prev1);

        EXPECT_TRUE(JSTaggedValue::SameValue(thread, result, JSTaggedValue(NODE_NUMBERS * 2)));
    }

    // SetLength
    {
        callInfo->SetCallArg(0, JSTaggedValue(NODE_NUMBERS / 2));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersVector::SetLength(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::GetSize(callInfo);
        TestHelper::TearDownFrame(thread, prev1);

        EXPECT_TRUE(JSTaggedValue::SameValue(thread, result, JSTaggedValue(NODE_NUMBERS / 2)));
    }
}

HWTEST_F_L0(ContainersVectorTest, ExceptionReturn1)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, Insert);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, SetLength);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, IncreaseCapacityTo);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, Get);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, GetIndexFrom);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, Add);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, GetCapacity);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, GetIndexOf);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, IsEmpty);
 
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(vector.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersVector, Insert, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(vector.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersVector, SetLength, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(vector.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersVector, IncreaseCapacityTo, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(vector.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersVector, Get, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(vector.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersVector, GetIndexFrom, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(-1));
        CONTAINERS_API_EXCEPTION_TEST(ContainersVector, SetLength, callInfo);
    }
}

HWTEST_F_L0(ContainersVectorTest, ExceptionReturn2)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, GetLastIndexFrom);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, RemoveByIndex);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, RemoveByRange);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, GetLastElement);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, GetLastIndexOf);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, Remove);
    
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(vector.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersVector, GetLastIndexFrom, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(vector.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersVector, RemoveByIndex, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(vector.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersVector, RemoveByRange, callInfo);
    }
}

HWTEST_F_L0(ContainersVectorTest, ExceptionReturn3)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, Set);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, SubVector);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, ReplaceAllElements);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, ToString);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, GetSize);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, ForEach);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, TrimToCurrentLength);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, Clear);

    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(vector.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersVector, Set, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(vector.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersVector, SubVector, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(vector.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersVector, ReplaceAllElements, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(-1));
        CONTAINERS_API_EXCEPTION_TEST(ContainersVector, Set, callInfo);
    }
}

HWTEST_F_L0(ContainersVectorTest, ExceptionReturn4)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, Clone);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, Has);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, CopyToArray);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, ConvertToArray);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, GetFirstElement);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, Sort);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersVector, GetIteratorObj);
}

// sort large array to trigger TimSort merge path (> MIN_MERGE=32)
HWTEST_F_L0(ContainersVectorTest, SortLargeArray)
{
    constexpr int32_t ELEMENT_NUMS = 64;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = ELEMENT_NUMS - 1; i >= 0; i--) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestSortAscFunc));
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(vector.GetTaggedValue());
    callInfo->SetCallArg(0, func.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    ContainersVector::Sort(callInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(vector.GetTaggedValue());
        callInfo1->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
        JSTaggedValue result = ContainersVector::Get(callInfo1);
        TestHelper::TearDownFrame(thread, prev1);
        EXPECT_EQ(result, JSTaggedValue(i));
    }
}

// sort already-sorted container (idempotent)
HWTEST_F_L0(ContainersVectorTest, SortAlreadySorted)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }

    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(vector.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    ContainersVector::Sort(callInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(vector.GetTaggedValue());
        callInfo1->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
        JSTaggedValue result = ContainersVector::Get(callInfo1);
        TestHelper::TearDownFrame(thread, prev1);
        EXPECT_EQ(result, JSTaggedValue(i));
    }
}

// sort with two-element container (boundary between <2 early return and full sort)
HWTEST_F_L0(ContainersVectorTest, SortTwoElements)
{
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(5));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(3));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(vector.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    ContainersVector::Sort(callInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(vector->GetSize(), 2);
    {
        auto callInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo1->SetFunction(JSTaggedValue::Undefined());
        callInfo1->SetThis(vector.GetTaggedValue());
        callInfo1->SetCallArg(0, JSTaggedValue(0));

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo1);
        JSTaggedValue result = ContainersVector::Get(callInfo1);
        TestHelper::TearDownFrame(thread, prev1);
        EXPECT_EQ(result, JSTaggedValue(3));
    }
}

// sort with removeByIndex during comparator callback (tests logical length restoration)
HWTEST_F_L0(ContainersVectorTest, SortWithRemoveDuringCallback)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = ELEMENT_NUMS - 1; i >= 0; i--) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);

    JSHandle<JSTaggedValue> containerHandle(thread, vector.GetTaggedValue());
    g_sortTargetSlot = containerHandle.GetAddress();
    g_sortCompareCount = 0;

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> func =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestSortWithRemoveFunc));
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(vector.GetTaggedValue());
    callInfo->SetCallArg(0, func.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    ContainersVector::Sort(callInfo);
    TestHelper::TearDownFrame(thread, prev);

    // Sort should complete safely: 8 sorted elements written back, length restored
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        EXPECT_EQ(JSAPIVector::Get(thread, vector, i), JSTaggedValue(i));
    }
    g_sortTargetSlot = 0;
}

// sort with trimToCurrentLength during comparator callback (tests backing store re-allocation)
HWTEST_F_L0(ContainersVectorTest, SortWithTrimDuringCallback)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = ELEMENT_NUMS - 1; i >= 0; i--) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);

    JSHandle<JSTaggedValue> containerHandle(thread, vector.GetTaggedValue());
    g_sortTargetSlot = containerHandle.GetAddress();
    g_sortCompareCount = 0;

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> func =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestSortWithTrimFunc));
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(vector.GetTaggedValue());
    callInfo->SetCallArg(0, func.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    ContainersVector::Sort(callInfo);
    TestHelper::TearDownFrame(thread, prev);

    // Backing store was trimmed during callback; new array should have been allocated
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        EXPECT_EQ(JSAPIVector::Get(thread, vector, i), JSTaggedValue(i));
    }
    g_sortTargetSlot = 0;
}

// sort with add during comparator callback (tests preservation of added elements)
HWTEST_F_L0(ContainersVectorTest, SortWithAddDuringCallback)
{
    constexpr int32_t ELEMENT_NUMS = 8;
    constexpr int32_t EXTRA = 5;
    JSHandle<JSAPIVector> vector = CreateJSAPIVector();
    for (int32_t i = ELEMENT_NUMS - 1; i >= 0; i--) {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(vector.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersVector::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS);

    JSHandle<JSTaggedValue> containerHandle(thread, vector.GetTaggedValue());
    g_sortTargetSlot = containerHandle.GetAddress();
    g_sortCompareCount = 0;

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> func =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestSortWithAddFunc));
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(vector.GetTaggedValue());
    callInfo->SetCallArg(0, func.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    ContainersVector::Sort(callInfo);
    TestHelper::TearDownFrame(thread, prev);

    // First 8 should be sorted [0..7], then 5 extra 999s preserved
    EXPECT_EQ(vector->GetSize(), ELEMENT_NUMS + EXTRA);
    for (int32_t i = 0; i < ELEMENT_NUMS; i++) {
        EXPECT_EQ(JSAPIVector::Get(thread, vector, i), JSTaggedValue(i));
    }
    for (int32_t i = 0; i < EXTRA; i++) {
        EXPECT_EQ(JSAPIVector::Get(thread, vector, ELEMENT_NUMS + i), JSTaggedValue(999)); // 999 extra 999s preserved
    }
    g_sortTargetSlot = 0;
}
} // namespace panda::test