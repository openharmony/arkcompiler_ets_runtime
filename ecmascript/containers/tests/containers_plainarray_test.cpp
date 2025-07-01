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

#include "ecmascript/containers/containers_plainarray.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_plain_array_iterator.h"
#include "ecmascript/js_api/js_api_plain_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/containers/tests/containers_test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;

namespace panda::test {
class ContainersPlainArrayTest : public testing::Test {
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
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);  // 0 means the value
            JSHandle<JSTaggedValue> value = GetCallArg(argv, 1); // 1 means the value
            JSHandle<JSAPIPlainArray> plainArray(GetCallArg(argv, 2)); // 2 means the value
            if (key->IsNumber()) {
                JSHandle<JSTaggedValue> newValue(thread, JSTaggedValue(value->GetInt() * 2)); // 2 means the value
                JSAPIPlainArray::Add(thread, plainArray, key, newValue);
            }

            return JSTaggedValue::True();
        }
    };
protected:
    JSTaggedValue InitializePlainArrayConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();

        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);   // 6 means the value
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::PlainArray)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);

        return result;
    }

    JSHandle<JSAPIPlainArray> CreateJSAPIPlainArray()
    {
        JSHandle<JSFunction> newTarget(thread, InitializePlainArrayConstructor());
        auto objCallInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);   // 4 means the value
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPlainArray::PlainArrayConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIPlainArray> plain(thread, result);
        return plain;
    }

    JSTaggedValue PlainArrayAdd(JSHandle<JSAPIPlainArray> plainArray, JSTaggedValue index, JSTaggedValue value)
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 4 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(plainArray.GetTaggedValue());
        callInfo->SetCallArg(0, index);
        callInfo->SetCallArg(1, value);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }

    JSTaggedValue PlainArrayRemoveRangeFrom(JSHandle<JSAPIPlainArray> plainArray, JSTaggedValue index,
                                            JSTaggedValue size)
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(plainArray.GetTaggedValue());
        callInfo->SetCallArg(0, index);
        callInfo->SetCallArg(1, size);

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::RemoveRangeFrom(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }
};

HWTEST_F_L0(ContainersPlainArrayTest, PlainArrayConstructor)
{
    InitializePlainArrayConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializePlainArrayConstructor());
    auto objCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);    // 4 means the value
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersPlainArray::PlainArrayConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsJSAPIPlainArray());
    JSHandle<JSAPIPlainArray> arrayHandle(thread, result);
    JSTaggedValue resultProto = JSObject::GetPrototype(thread, JSHandle<JSObject>::Cast(arrayHandle));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype(thread);
    ASSERT_EQ(resultProto, funcProto);
    int size = arrayHandle->GetSize();
    ASSERT_EQ(size, 0);

    // test PlainArrayConstructor exception
    objCallInfo->SetNewTarget(JSTaggedValue::Undefined());
    CONTAINERS_API_EXCEPTION_TEST(ContainersPlainArray, PlainArrayConstructor, objCallInfo);
}

HWTEST_F_L0(ContainersPlainArrayTest, AddAndHas)
{
    constexpr uint32_t NODE_NUMBERS = 8;    // 8 means the value

    JSHandle<JSAPIPlainArray> tArray1 = CreateJSAPIPlainArray();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);   // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tArray1.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsUndefined());
        EXPECT_EQ(tArray1->GetSize(), static_cast<int>(i + 1));
    }
    EXPECT_EQ(tArray1->GetSize(), static_cast<int>(NODE_NUMBERS));
    // test has
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);   // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tArray1.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Has(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsTrue());
    }
    EXPECT_EQ(tArray1->GetSize(), static_cast<int>(NODE_NUMBERS));
    // test add string
    JSHandle<JSAPIPlainArray> tArray = CreateJSAPIPlainArray();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        key.Update(JSTaggedValue(i));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);   // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tArray.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());
        callInfo->SetCallArg(1, value.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsUndefined());
    }
    EXPECT_EQ(tArray->GetSize(), static_cast<int>(NODE_NUMBERS));
    // test get
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string ivalue = myValue + std::to_string(i);
        key.Update(JSTaggedValue(i));
        value.Update(factory->NewFromStdString(ivalue).GetTaggedValue());

        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);   // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(tArray.GetTaggedValue());
        callInfo->SetCallArg(0, key.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Get(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(JSTaggedValue::Equal(thread, JSHandle<JSTaggedValue>(thread, result), value));
    }
}

HWTEST_F_L0(ContainersPlainArrayTest, Iterator)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIPlainArray> array = CreateJSAPIPlainArray();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);   // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(array.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsUndefined());
        EXPECT_EQ(array->GetSize(), static_cast<int>(i + 1));
    }
    // test iterator
    {
        auto callInf = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        callInf->SetFunction(JSTaggedValue::Undefined());
        callInf->SetThis(array.GetTaggedValue());
        [[maybe_unused]] auto pre = TestHelper::SetupFrame(thread, callInf);
        JSHandle<JSTaggedValue> iter(thread, ContainersPlainArray::GetIteratorObj(callInf));
        TestHelper::TearDownFrame(thread, pre);
        EXPECT_TRUE(iter->IsJSAPIPlainArrayIterator());

        JSHandle<JSTaggedValue> first(thread, JSTaggedValue(0));
        JSHandle<JSTaggedValue> second(thread, JSTaggedValue(1));
        JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Undefined());
        JSMutableHandle<JSTaggedValue> entries(thread, JSTaggedValue::Undefined());
        for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
            auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
            callInfo->SetFunction(JSTaggedValue::Undefined());
            callInfo->SetThis(iter.GetTaggedValue());

            [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
            result.Update(JSAPIPlainArrayIterator::Next(callInfo));
            TestHelper::TearDownFrame(thread, prev);
            entries.Update(JSIterator::IteratorValue(thread, result).GetTaggedValue());
            EXPECT_EQ(static_cast<int>(i), JSObject::GetProperty(thread, entries, first).GetValue()->GetInt());
            EXPECT_EQ(static_cast<int>(i + 1), JSObject::GetProperty(thread, entries, second).GetValue()->GetInt());
        }
    }
    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);   // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(array.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(100 + i));
        callInfo->SetCallArg(1, value.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsUndefined());
        EXPECT_EQ(array->GetSize(), static_cast<int>(NODE_NUMBERS + i + 1));
    }
    EXPECT_EQ(array->GetSize(), static_cast<int>(NODE_NUMBERS * 2));
}

HWTEST_F_L0(ContainersPlainArrayTest, GetIndexOfKeyAndGetIndexOfValue)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIPlainArray> pArray = CreateJSAPIPlainArray();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 4 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(pArray.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsUndefined());
        EXPECT_EQ(pArray->GetSize(), static_cast<int>(i + 1));
    }
    // test GetIndexOfKey
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 6 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(pArray.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(2));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::GetIndexOfKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(2));
    }
    // test GetIndexOfValue
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 6 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(pArray.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(4));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::GetIndexOfValue(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(3));
    }
    // test add string
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSMutableHandle<JSTaggedValue> value(thread, JSTaggedValue::Undefined());
    std::string myValue("myvalue");
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        std::string iValue = myValue + std::to_string(i);
        value.Update(factory->NewFromStdString(iValue).GetTaggedValue());
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 8 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(pArray.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(100 + i));
        callInfo->SetCallArg(1, value.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_TRUE(result.IsUndefined());
        EXPECT_EQ(pArray->GetSize(), static_cast<int>(NODE_NUMBERS + i + 1));
    }
    EXPECT_EQ(pArray->GetSize(), static_cast<int>(NODE_NUMBERS * 2));
    // test GetIndexOfKey
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 6 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(pArray.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(102));

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::GetIndexOfKey(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(10));
    }
    // test GetIndexOfValue
    {
        std::string tValue("myvalue3");
        value.Update(factory->NewFromStdString(tValue).GetTaggedValue());
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 6 means the value
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(pArray.GetTaggedValue());
        callInfo->SetCallArg(0, value.GetTaggedValue());

        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersPlainArray::GetIndexOfValue(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(11));
    }
}

HWTEST_F_L0(ContainersPlainArrayTest, RemoveRangeFrom)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIPlainArray> pArray = CreateJSAPIPlainArray();
    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        JSTaggedValue result = PlainArrayAdd(pArray, JSTaggedValue(i), JSTaggedValue(i + 1));
        EXPECT_TRUE(result.IsUndefined());
        EXPECT_EQ(pArray->GetSize(), static_cast<int>(i + 1));
    }

    // remove success
    {
        JSTaggedValue result = PlainArrayRemoveRangeFrom(pArray, JSTaggedValue(2), JSTaggedValue(2));
        EXPECT_EQ(result, JSTaggedValue(2));
        EXPECT_EQ(pArray->GetSize(), static_cast<int>(NODE_NUMBERS - 2));
        for (uint32_t i = 0; i < NODE_NUMBERS - 2; i++) {
            if (i < 2) {
                EXPECT_EQ(pArray->Get(thread, JSTaggedValue(i)), JSTaggedValue(i + 1));
            } else {
                EXPECT_EQ(pArray->Get(thread, JSTaggedValue(i + 2)), JSTaggedValue(i + 3));
            }
        }
    }

    // input index type error
    {
        JSTaggedValue result = PlainArrayRemoveRangeFrom(pArray, JSTaggedValue::Undefined(), JSTaggedValue(2));
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }

    // input size type error
    {
        JSTaggedValue result = PlainArrayRemoveRangeFrom(pArray, JSTaggedValue(2), JSTaggedValue::Undefined());
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }

    // input index out of range
    {
        JSTaggedValue result = PlainArrayRemoveRangeFrom(pArray, JSTaggedValue(NODE_NUMBERS + 1), JSTaggedValue(2));
        EXPECT_TRUE(thread->HasPendingException());
        EXPECT_EQ(result, JSTaggedValue::Exception());
        thread->ClearException();
    }
}

HWTEST_F_L0(ContainersPlainArrayTest, ProxyOfGetSize)
{
    constexpr uint32_t NODE_NUMBERS = 8;
    JSHandle<JSAPIPlainArray> proxyArrayList = CreateJSAPIPlainArray();
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    JSHandle<JSProxy> proxy = CreateJSProxyHandle(thread);
    proxy->SetTarget(thread, proxyArrayList.GetTaggedValue());
    callInfo->SetThis(proxy.GetTaggedValue());

    for (uint32_t i = 0; i < NODE_NUMBERS; i++) {
        callInfo->SetCallArg(0, JSTaggedValue(i));
        callInfo->SetCallArg(1, JSTaggedValue(i + 1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersPlainArray::Add(callInfo);
        TestHelper::TearDownFrame(thread, prev);

        [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue retult = ContainersPlainArray::GetSize(callInfo);
        TestHelper::TearDownFrame(thread, prev1);
        EXPECT_EQ(retult, JSTaggedValue(i + 1));
    }
}

HWTEST_F_L0(ContainersPlainArrayTest, ExceptionReturn1)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, Add);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, Has);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, Get);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, GetIndexOfKey);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, Clear);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, Clone);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, GetIteratorObj);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, ForEach);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, ToString);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, GetIndexOfValue);

    JSHandle<JSAPIPlainArray> plianArray = CreateJSAPIPlainArray();
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(plianArray.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersPlainArray, Add, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(plianArray.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersPlainArray, Has, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(plianArray.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersPlainArray, Get, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(plianArray.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersPlainArray, GetIndexOfKey, callInfo);
    }
}

HWTEST_F_L0(ContainersPlainArrayTest, ExceptionReturn2)
{
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, GetKeyAt);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, Remove);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, RemoveAt);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, SetValueAt);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, GetValueAt);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, IsEmpty);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, RemoveRangeFrom);
    CONTAINERS_API_TYPE_MISMATCH_EXCEPTION_TEST(ContainersPlainArray, GetSize);

    JSHandle<JSAPIPlainArray> plianArray = CreateJSAPIPlainArray();
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(plianArray.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersPlainArray, GetKeyAt, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(plianArray.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersPlainArray, Remove, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(plianArray.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersPlainArray, RemoveAt, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(plianArray.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersPlainArray, SetValueAt, callInfo);
    }
    {
        auto callInfo = NewEmptyCallInfo(thread);
        callInfo->SetThis(plianArray.GetTaggedValue());
        CONTAINERS_API_EXCEPTION_TEST(ContainersPlainArray, GetValueAt, callInfo);
    }
}
}
