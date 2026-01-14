/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "ecmascript/containers/containers_buffer.h"
#include "ecmascript/containers/containers_private.h"
#include "ecmascript/containers/tests/containers_test_helper.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_api/js_api_buffer.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"
#include "gtest/gtest.h"
#include "macros.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::containers;
namespace panda::test {
class ContainersBufferTest : public testing::Test {
public:
    static uint32_t GetArgvCount(uint32_t setCount)
    {
        return setCount * 2;  // 2 means the every arg cover 2 length
    }
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
    JSTaggedValue InitializeBufferConstructor()
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> globalObject = env->GetJSGlobalObject();
        JSHandle<JSTaggedValue> key(factory->NewFromASCII("ArkPrivate"));
        JSHandle<JSTaggedValue> value =
            JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(globalObject), key).GetValue();
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                                 ContainersBufferTest::GetArgvCount(3));
        objCallInfo->SetFunction(JSTaggedValue::Undefined());
        objCallInfo->SetThis(value.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int>(ContainerTag::FastBuffer)));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersPrivate::Load(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        return result;
    }
    JSHandle<JSAPIFastBuffer> CreateJSAPIBuffer(uint32_t length = JSAPIFastBuffer::DEFAULT_CAPACITY_LENGTH)
    {
        JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                                 ContainersBufferTest::GetArgvCount(3));
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetCallArg(0, JSTaggedValue(length));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersBuffer::BufferConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIFastBuffer> buffer(thread, result);
        return buffer;
    }
    JSHandle<JSAPIFastBuffer> CreateJSAPIBuffer(std::string str)
    {
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<EcmaString> strHandle = factory->NewFromStdString(str);
        JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
        auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                                 ContainersBufferTest::GetArgvCount(3));
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetCallArg(0, strHandle.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersBuffer::BufferConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIFastBuffer> buffer(thread, result);
        return buffer;
    }
    JSHandle<JSTypedArray> NewUint8Array(uint32_t length)
    {
        JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
        JSHandle<JSTaggedValue> handleTagValFunc = env->GetUint8ArrayFunction();
        ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
        JSHandle<JSObject> obj =
            factory->NewJSObjectByConstructor(JSHandle<JSFunction>(handleTagValFunc), handleTagValFunc);
        DataViewType arrayType = DataViewType::UINT8;
        JSTypedArray::Cast(*obj)->SetTypedArrayName(thread, thread->GlobalConstants()->GetUint8ArrayString());
        if (length > 0) {
            TypedArrayHelper::AllocateTypedArrayBuffer(thread, obj, length, arrayType);
        } else {
            auto jsTypedArray = JSTypedArray::Cast(*obj);
            jsTypedArray->SetContentType(ContentType::Number);
            jsTypedArray->SetByteLength(0);
            jsTypedArray->SetByteOffset(0);
            jsTypedArray->SetArrayLength(0);
        }
        return JSHandle<JSTypedArray>(obj);
    }
};

HWTEST_F_L0(ContainersBufferTest, BufferConstructor001)
{
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                             6);
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetCallArg(0, JSTaggedValue(JSAPIFastBuffer::DEFAULT_CAPACITY_LENGTH));
    EXPECT_EQ(objCallInfo->GetArgsNumber(), 1);
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    EXPECT_EQ(objCallInfo->GetArgsNumber(), 1);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.IsJSAPIBuffer());
    JSHandle<JSAPIFastBuffer> buffer(thread, result);
    JSTaggedValue resultProto = JSObject::GetPrototype(thread, JSHandle<JSObject>::Cast(buffer));
    JSTaggedValue funcProto = newTarget->GetFunctionPrototype(thread);
    ASSERT_EQ(resultProto, funcProto);
    int length = buffer->GetLength();
    ASSERT_EQ(length, JSAPIFastBuffer::DEFAULT_CAPACITY_LENGTH);
    objCallInfo->SetNewTarget(JSTaggedValue::Undefined());
    CONTAINERS_API_EXCEPTION_TEST(ContainersBuffer, BufferConstructor, objCallInfo);
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructor002)
{
    InitializeBufferConstructor();
    std::string ivalue = "Test";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> strHandle = factory->NewFromStdString(ivalue);
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
        ContainersBufferTest::GetArgvCount(4));
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetCallArg(0, strHandle.GetTaggedValue());
    objCallInfo->SetCallArg(1, strHandle.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructor003)
{
    InitializeBufferConstructor();
    std::string ivalue = "Test";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> strHandle = factory->NewFromStdString(ivalue);
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
        ContainersBufferTest::GetArgvCount(4));
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetCallArg(0, strHandle.GetTaggedValue());
    objCallInfo->SetCallArg(1, JSTaggedValue(1));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_NE(result, JSTaggedValue::Exception());
    ASSERT_TRUE(result.IsJSAPIBuffer());
}


HWTEST_F_L0(ContainersBufferTest, BufferConstructor004)
{
    EcmaVM *ecmaVMPtr = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVMPtr->GetFactory();
    JSHandle<JSFunction> handleFuncArrayBuf(ecmaVMPtr->GetGlobalEnv()->GetArrayBufferFunction());
    JSHandle<JSTaggedValue> handleTagValFuncArrayBuf(handleFuncArrayBuf);
    uint32_t lengthDataArrayBuf1 = 8;
    JSHandle<JSArrayBuffer> handleArrayBuf1(
        factory->NewJSObjectByConstructor(handleFuncArrayBuf, handleTagValFuncArrayBuf));
    handleArrayBuf1->SetArrayBufferByteLength(lengthDataArrayBuf1);
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
        ContainersBufferTest::GetArgvCount(5));
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetCallArg(0, handleArrayBuf1.GetTaggedValue());
    objCallInfo->SetCallArg(1, JSTaggedValue(0));
    objCallInfo->SetCallArg(1, JSTaggedValue(3));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_NE(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructor005)
{
    JSHandle<JSAPIFastBuffer> result = CreateJSAPIBuffer(0);
    ASSERT_EQ(result->GetLength(), 0);
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructor006)
{
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcde");
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
        ContainersBufferTest::GetArgvCount(5));
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetCallArg(0, buffer.GetTaggedValue());
    objCallInfo->SetCallArg(1, JSTaggedValue(0));
    objCallInfo->SetCallArg(2, JSTaggedValue(2));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_NE(result, JSTaggedValue::Exception());
    ASSERT_TRUE(result.IsJSAPIBuffer());
}

HWTEST_F_L0(ContainersBufferTest, Length_001)
{
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer();
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          ContainersBufferTest::GetArgvCount(2));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(buffer->GetLength(), JSAPIFastBuffer::DEFAULT_CAPACITY_LENGTH);
}

HWTEST_F_L0(ContainersBufferTest, WriteInt32BEAndReadInt32BETest001)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0x12345678));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteInt32BE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(3));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadInt32BE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x12345678);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteInt32BEAndReadInt32BETest002)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0x12345678));
        callInfo->SetCallArg(1, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteInt32BE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(3));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadInt32BE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x12345678);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteInt16BEAndReadInt16BETest001)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0x1234));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteInt16BE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(3));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadInt16BE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x1234);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteInt16BEAndReadInt16BETest002)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0x1234));
        callInfo->SetCallArg(1, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteInt16LE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(3));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadInt16BE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x3412);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteInt32LEAndReadIntTest001)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0x12345678));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteInt32LE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(3));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadInt32LE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x12345678);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(3));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadInt32BE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x78563412);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteInt32LEAndReadIntTest002)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0x12345678));
        callInfo->SetCallArg(1, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteInt32LE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(3));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadInt32LE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x12345678);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(3));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadInt32BE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x78563412);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteIntLEAndReadIntTest001)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(5));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0x1234));
        callInfo->SetCallArg(1, JSTaggedValue(1));
        callInfo->SetCallArg(2, JSTaggedValue(2));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteIntLE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(2));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadIntLE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x1234);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(2));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadIntBE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x3412);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteIntLEAndReadIntTest002)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(5));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0x123456));
        callInfo->SetCallArg(1, JSTaggedValue(1));
        callInfo->SetCallArg(2, JSTaggedValue(3));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteIntLE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(3));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadIntLE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x123456);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(3));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadIntBE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x563412);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteIntAndReadIntTest003)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(5));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0x123456));
        callInfo->SetCallArg(1, JSTaggedValue(1));
        callInfo->SetCallArg(2, JSTaggedValue(3));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteIntBE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(3));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadIntBE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x123456);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(3));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadIntLE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x563412);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteIntAndReadIntTest004)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    uint64_t value = 0x12345678ABCD;
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(5));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(value)));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(6));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteIntBE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(6));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadIntBE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(static_cast<int64_t>(res.GetDouble()), value);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteIntAndReadIntTest005)
{
    constexpr uint32_t BUFFER_SIZE = 10;
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(BUFFER_SIZE);
    // 0x12345678ABCD : test value
    uint64_t value = 0x12345678ABCD;
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                  ContainersBufferTest::GetArgvCount(5));  // 5 : five Args in callInfo
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        // 0 : the first parameter
        callInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(value)));
        // 1 : the second parameter
        callInfo->SetCallArg(1, JSTaggedValue(0));
        // 2 : 6 : the third parameter; write 6 bytes
        callInfo->SetCallArg(2, JSTaggedValue(6));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteIntLE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        // 0 : the first parameter
        callInfo->SetCallArg(0, JSTaggedValue(0));
        // 1 : 6 : the second parameter; read 6 bytes
        callInfo->SetCallArg(1, JSTaggedValue(6));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadIntLE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(static_cast<int64_t>(res.GetDouble()), value);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteIntAndReadIntTest006)
{
    constexpr uint32_t BUFFER_SIZE = 10;
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(BUFFER_SIZE);
    // 0x12345678ABCD : test value
    uint64_t value = 0x12345678ABCD;
    // -55338634693614 : test result
    int64_t ret = -55338634693614;
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                  ContainersBufferTest::GetArgvCount(5));  // 5 : five Args in callInfo
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        // 0 : the first parameter
        callInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(value)));
        // 1 : the second parameter
        callInfo->SetCallArg(1, JSTaggedValue(0));
        // 2 : 6 : the third parameter; write 6 bytes
        callInfo->SetCallArg(2, JSTaggedValue(6));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteIntBE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                  ContainersBufferTest::GetArgvCount(4));  // 4 : four Args in callInfo
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        // 0 : the first parameter
        callInfo->SetCallArg(0, JSTaggedValue(0));
        // 1 : 6 : the second parameter; read 6 bytes
        callInfo->SetCallArg(1, JSTaggedValue(6));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadIntLE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(static_cast<int64_t>(res.GetDouble()), ret);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteIntAndReadIntTest007)
{
    constexpr uint32_t BUFFER_SIZE = 10;
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(BUFFER_SIZE);
    // 0x1234567890ab : test value
    int64_t value = 0x1234567890ab;
    // -0x546f87a9cbee : test result
    int64_t ret = -0x546f87a9cbee;
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                  ContainersBufferTest::GetArgvCount(5));  // 5 : five Args in callInfo
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        // 0 : the first parameter
        callInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(value)));
        // 1 : the second parameter
        callInfo->SetCallArg(1, JSTaggedValue(0));
        // 2 : 6 : the third parameter; write 6 bytes
        callInfo->SetCallArg(2, JSTaggedValue(6));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteIntBE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo =
            TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                  ContainersBufferTest::GetArgvCount(4));  // 4 : four Args in callInfo
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        // 0 : the first parameter
        callInfo->SetCallArg(0, JSTaggedValue(0));
        // 1 : 6 : the second parameter; read 6 bytes
        callInfo->SetCallArg(1, JSTaggedValue(6));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadIntLE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(static_cast<int64_t>(res.GetDouble()), ret);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteUIntLEAndReadUIntTest001)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(5));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0x1234));
        callInfo->SetCallArg(1, JSTaggedValue(1));
        callInfo->SetCallArg(2, JSTaggedValue(2));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteUIntLE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(2));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadUIntLE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x1234);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(2));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadUIntBE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x3412);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteUIntBEAndReadUIntTest001)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(5));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0x1234));
        callInfo->SetCallArg(1, JSTaggedValue(1));
        callInfo->SetCallArg(2, JSTaggedValue(2));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteUIntBE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(2));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadUIntLE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x3412);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(1));
        callInfo->SetCallArg(1, JSTaggedValue(2));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadUIntBE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetInt(), 0x1234);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteDoubleAndReadDoubleTest001)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    double value = 112512.1919810;
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(5));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(value));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(6));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteFloat64LE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(6));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadFloat64LE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetNumber(), value);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteDoubleAndReadDoubleTest002)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    double value = 112512.1919810;
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(5));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(value));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(6));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteFloat64BE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        callInfo->SetCallArg(1, JSTaggedValue(6));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue res = ContainersBuffer::ReadFloat64BE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_EQ(res.GetNumber(), value);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteFloatAndReadFloatTest001)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    double value = 123.45;
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(value));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteFloat32LE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(3));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        double res = ContainersBuffer::ReadFloat32LE(callInfo).GetDouble();
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_TRUE(std::fabs(res - value) < 1e-4);
    }
}

HWTEST_F_L0(ContainersBufferTest, WriteFloatAndReadFloatTest002)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    double value = 123.45;
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(4));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(value));
        callInfo->SetCallArg(1, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        ContainersBuffer::WriteFloat32BE(callInfo);
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                              ContainersBufferTest::GetArgvCount(3));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(buf.GetTaggedValue());
        callInfo->SetCallArg(0, JSTaggedValue(0));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        double res = ContainersBuffer::ReadFloat32BE(callInfo).GetDouble();
        TestHelper::TearDownFrame(thread, prev);
        ASSERT_TRUE(std::fabs(res - value) < 1e-4);
    }
}

HWTEST_F_L0(ContainersBufferTest, CreateFromArrayTest001)
{
    JSHandle<JSTaggedValue> arr =
        JSHandle<JSTaggedValue>(thread, JSArray::ArrayCreate(thread, JSTaggedNumber(10))->GetTaggedObject());
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                             ContainersBufferTest::GetArgvCount(4));
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetCallArg(0, arr.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSAPIFastBuffer> buffer(thread, result);
    ASSERT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest001)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest002)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Equals(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest003)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::IndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest004)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Includes(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest005)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::LastIndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest006)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Fill(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest007)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    std::string ivalue = "Test";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> strHandle = factory->NewFromStdString(ivalue);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, strHandle.GetTaggedValue());
    callInfo->SetCallArg(1, strHandle.GetTaggedValue());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Write(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest008)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    std::string ivalue = "Test";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> strHandle = factory->NewFromStdString(ivalue);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, strHandle.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::ToString(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest009)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Copy(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest010)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::WriteUIntLE(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest011)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::WriteUIntBE(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest012)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::ReadUIntBE(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest013)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::ReadUIntLE(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest014)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::ReadIntLE(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest015)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::ReadIntBE(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest016)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::WriteIntBE(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest017)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Hole());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::WriteIntLE(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest018)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    std::string ivalue = "Test";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> strHandle = factory->NewFromStdString(ivalue);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, strHandle.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue::Hole());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, strHandle.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Write(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest019)
{
    std::string ivalue = "Test";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> strHandle = factory->NewFromStdString(ivalue);
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, strHandle.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, strHandle.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::IndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest020)
{
    std::string ivalue = "Test";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> strHandle = factory->NewFromStdString(ivalue);
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, strHandle.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, strHandle.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::LastIndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest021)
{
    std::string ivalue = "Test";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> strHandle = factory->NewFromStdString(ivalue);
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, strHandle.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, strHandle.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Includes(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest022)
{
    std::string ivalue = "Test";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> strHandle = factory->NewFromStdString(ivalue);
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, strHandle.GetTaggedValue());
    callInfo->SetCallArg(1, strHandle.GetTaggedValue());
    callInfo->SetCallArg(2, JSTaggedValue::Hole());
    callInfo->SetCallArg(3, JSTaggedValue::Hole());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Fill(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, PassInvalidParameterTest023)
{
    std::string ivalue = "Test";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> strHandle = factory->NewFromStdString(ivalue);
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, strHandle.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, JSTaggedValue(2));
    callInfo->SetCallArg(3, strHandle.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Fill(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, OOBTest001)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer("cdefg");
    JSHandle<JSAPIFastBuffer> target = CreateJSAPIBuffer("abcde");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(7));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(3));
    callInfo->SetCallArg(2, JSTaggedValue(2));
    callInfo->SetCallArg(3, JSTaggedValue(3));
    callInfo->SetCallArg(4, JSTaggedValue(2));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(0));
}

HWTEST_F_L0(ContainersBufferTest, OOBTest002)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer("cdefg");
    JSHandle<JSAPIFastBuffer> target = CreateJSAPIBuffer("abcde");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(7));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, JSTaggedValue(2));
    callInfo->SetCallArg(3, JSTaggedValue(3));
    callInfo->SetCallArg(4, JSTaggedValue(2));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(-1));
}

HWTEST_F_L0(ContainersBufferTest, OOBTest003)
{
    JSHandle<JSAPIFastBuffer> buf = CreateJSAPIBuffer("cdefg");
    JSHandle<JSAPIFastBuffer> target = CreateJSAPIBuffer("abcde");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(7));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buf.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(3));
    callInfo->SetCallArg(2, JSTaggedValue(2));
    callInfo->SetCallArg(3, JSTaggedValue(0));
    callInfo->SetCallArg(4, JSTaggedValue(2));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(1));
}

HWTEST_F_L0(ContainersBufferTest, CopyTest001)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(10);
    std::string ivalue = "invalid";
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> invalidTarget = factory->NewFromStdString(ivalue);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, invalidTarget.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, JSTaggedValue(0));
    callInfo->SetCallArg(3, JSTaggedValue(10));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Copy(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, CopyTest002)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("HelloWorld");
    
    JSHandle<JSTypedArray> arr = NewUint8Array(15);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, arr.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(5));
    callInfo->SetCallArg(2, JSTaggedValue(3));
    callInfo->SetCallArg(3, JSTaggedValue(8));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Copy(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(5));
}

HWTEST_F_L0(ContainersBufferTest, CopyTest003)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("OpenHarmony");
    JSHandle<JSAPIFastBuffer> dst = CreateJSAPIBuffer(20);
    
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, dst.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(2));
    callInfo->SetCallArg(2, JSTaggedValue(4));
    callInfo->SetCallArg(3, JSTaggedValue(9));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Copy(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(5));
}

HWTEST_F_L0(ContainersBufferTest, CopyTest004)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(5);
    JSHandle<JSAPIFastBuffer> dst = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, dst.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, JSTaggedValue(5));
    callInfo->SetCallArg(3, JSTaggedValue(10));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Copy(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, CopyTest005)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(10);
    JSHandle<JSAPIFastBuffer> dst = CreateJSAPIBuffer(5);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, dst.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(5));
    callInfo->SetCallArg(2, JSTaggedValue(0));
    callInfo->SetCallArg(3, JSTaggedValue(5));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Copy(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(0));
}

HWTEST_F_L0(ContainersBufferTest, CopyTest006)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("12345");
    JSHandle<JSAPIFastBuffer> dst = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, dst.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, JSTaggedValue(3));
    callInfo->SetCallArg(3, JSTaggedValue(2));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Copy(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(0));
}

HWTEST_F_L0(ContainersBufferTest, CopyTest007)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(10);
    JSHandle<JSAPIFastBuffer> dst = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, dst.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(-1));
    callInfo->SetCallArg(2, JSTaggedValue(0));
    callInfo->SetCallArg(3, JSTaggedValue(5));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Copy(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, CopyTest008)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(5);
    JSHandle<JSAPIFastBuffer> dst = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, dst.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, JSTaggedValue(0));
    callInfo->SetCallArg(3, JSTaggedValue(10));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Copy(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, CopyTest009)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("1234567890");
    JSHandle<JSAPIFastBuffer> dst = CreateJSAPIBuffer(5);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, dst.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(2));
    callInfo->SetCallArg(2, JSTaggedValue(3));
    callInfo->SetCallArg(3, JSTaggedValue(8));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Copy(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(3));
}

HWTEST_F_L0(ContainersBufferTest, CopyTest010)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(10);
    JSHandle<JSAPIFastBuffer> dst = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, dst.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue::True());
    callInfo->SetCallArg(2, JSTaggedValue::False());
    callInfo->SetCallArg(3, JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Copy(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(10));
}

HWTEST_F_L0(ContainersBufferTest, CopyTest011)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(10);
    JSHandle<JSTypedArray> targetArray = NewUint8Array(10);
    JSHandle<JSAPIFastBuffer> targetBuffer = CreateJSAPIBuffer(10);
    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                                ContainersBufferTest::GetArgvCount(6));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(src.GetTaggedValue());
        callInfo->SetCallArg(0, targetArray.GetTaggedValue());
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(0));
        callInfo->SetCallArg(3, JSTaggedValue(5));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBuffer::Copy(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue(5));
    }

    {
        auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                                ContainersBufferTest::GetArgvCount(6));
        callInfo->SetFunction(JSTaggedValue::Undefined());
        callInfo->SetThis(targetArray.GetTaggedValue());
        callInfo->SetCallArg(0, targetBuffer.GetTaggedValue());
        callInfo->SetCallArg(1, JSTaggedValue(0));
        callInfo->SetCallArg(2, JSTaggedValue(0));
        callInfo->SetCallArg(3, JSTaggedValue(5));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
        JSTaggedValue result = ContainersBuffer::Copy(callInfo);
        TestHelper::TearDownFrame(thread, prev);
        EXPECT_EQ(result, JSTaggedValue::Exception());
    }
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructorTest001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayBuffer> buffer = factory->NewJSSharedArrayBuffer(10);
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    callInfo->SetFunction(newTarget.GetTaggedValue());
    callInfo->SetNewTarget(newTarget.GetTaggedValue());
    callInfo->SetCallArg(0, buffer.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(2));
    callInfo->SetCallArg(2, JSTaggedValue(5));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_NE(result, JSTaggedValue::Exception());
    ASSERT_TRUE(result.IsJSAPIBuffer());
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructorTest002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayBuffer> ab = factory->NewJSSharedArrayBuffer(10);
    ab->Detach(thread);
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(newTarget.GetTaggedValue());
    callInfo->SetNewTarget(newTarget.GetTaggedValue());
    callInfo->SetCallArg(0, ab.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructorTest003)
{
    JSHandle<JSTypedArray> u8array = NewUint8Array(8);
    bool isuin = false;
    isuin = u8array.GetTaggedValue().IsJSUint8Array();
    EXPECT_EQ(isuin, true);
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(newTarget.GetTaggedValue());
    callInfo->SetNewTarget(newTarget.GetTaggedValue());
    callInfo->SetCallArg(0, u8array.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_NE(result, JSTaggedValue::Exception());
    JSHandle<JSAPIFastBuffer> buffer(thread, result);
    EXPECT_EQ(buffer->GetLength(), 8);
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructorTest004)
{
    JSHandle<JSTaggedValue> arr =
        JSHandle<JSTaggedValue>(thread, JSArray::ArrayCreate(thread, JSTaggedNumber(10))->GetTaggedObject());
    bool isuin = false;
    isuin = arr.GetTaggedValue().IsJSArray();
    EXPECT_EQ(isuin, true);
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    callInfo->SetFunction(newTarget.GetTaggedValue());
    callInfo->SetNewTarget(newTarget.GetTaggedValue());
    callInfo->SetCallArg(0, arr.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructorTest005)
{
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcde");
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
        ContainersBufferTest::GetArgvCount(5));
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetCallArg(0, buffer.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(objCallInfo);
    EXPECT_NE(result, JSTaggedValue::Exception());
    JSHandle<JSAPIFastBuffer> dest(thread, result);
    EXPECT_EQ(dest->GetLength(), buffer->GetLength());
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructorTest006)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(5);
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    callInfo->SetFunction(newTarget.GetTaggedValue());
    callInfo->SetNewTarget(newTarget.GetTaggedValue());
    callInfo->SetCallArg(0, src.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(-1));
    callInfo->SetCallArg(2, JSTaggedValue(2));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSAPIFastBuffer> buffer(thread, result);
    EXPECT_EQ(buffer->GetLength(), 5);
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructorTest007)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(5);
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    callInfo->SetFunction(newTarget.GetTaggedValue());
    callInfo->SetNewTarget(newTarget.GetTaggedValue());
    callInfo->SetCallArg(0, src.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(10));
    callInfo->SetCallArg(2, JSTaggedValue(2));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSAPIBuffer());
    JSHandle<JSAPIFastBuffer> buffer(thread, result);
    EXPECT_EQ(buffer->GetLength(), 5);
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructorTest008)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(5);
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    callInfo->SetFunction(newTarget.GetTaggedValue());
    callInfo->SetNewTarget(newTarget.GetTaggedValue());
    callInfo->SetCallArg(0, src.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue::True());
    callInfo->SetCallArg(2, JSTaggedValue(2));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSAPIBuffer());
    JSHandle<JSAPIFastBuffer> buffer(thread, result);
    EXPECT_EQ(buffer->GetLength(), 5);
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructorTest009)
{
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo->SetFunction(newTarget.GetTaggedValue());
    callInfo->SetNewTarget(newTarget.GetTaggedValue());
    callInfo->SetCallArg(0, thread->GetEcmaVM()->GetFactory()->GetEmptyString().GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructorTest010)
{
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo->SetFunction(newTarget.GetTaggedValue());
    callInfo->SetNewTarget(newTarget.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, BufferConstructorTest011)
{
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetNewTarget(JSTaggedValue::Undefined());
    callInfo->SetCallArg(0, JSTaggedValue(10));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, CompareTest001)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("abc");
    JSHandle<EcmaString> invalidTarget = thread->GetEcmaVM()->GetFactory()->NewFromASCII("invalid");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, invalidTarget.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, CompareTest002)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("abcde");
    JSHandle<JSTypedArray> target = NewUint8Array(5);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    
    EXPECT_EQ(result, JSTaggedValue(1));
}

HWTEST_F_L0(ContainersBufferTest, CompareTest003)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("");
    JSHandle<JSAPIFastBuffer> target = CreateJSAPIBuffer("");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(0));
}

HWTEST_F_L0(ContainersBufferTest, CompareTest004)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("abcdef");
    JSHandle<JSAPIFastBuffer> target = CreateJSAPIBuffer("abcdef");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(0));
}

HWTEST_F_L0(ContainersBufferTest, CompareTest005)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(5);
    JSHandle<JSAPIFastBuffer> target = CreateJSAPIBuffer(5);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(7));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, JSTaggedValue(10));
    callInfo->SetCallArg(3, JSTaggedValue(0));
    callInfo->SetCallArg(4, JSTaggedValue(3));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, CompareTest006)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("open_harmony");
    JSHandle<JSAPIFastBuffer> target = CreateJSAPIBuffer("open_source");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(7));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, JSTaggedValue(5));
    callInfo->SetCallArg(3, JSTaggedValue(0));
    callInfo->SetCallArg(4, JSTaggedValue(5));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(0));
}

HWTEST_F_L0(ContainersBufferTest, CompareTest007)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("hello");
    JSHandle<JSAPIFastBuffer> target = CreateJSAPIBuffer("helloworld");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(-1));
}

HWTEST_F_L0(ContainersBufferTest, CompareTest008)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(5);
    JSHandle<JSAPIFastBuffer> target = CreateJSAPIBuffer(5);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(7));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(-1));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

HWTEST_F_L0(ContainersBufferTest, CompareTest009)
{
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("ABCDEF");
    JSHandle<JSAPIFastBuffer> target = CreateJSAPIBuffer("ABCDEF");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                            ContainersBufferTest::GetArgvCount(7));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue::True());
    callInfo->SetCallArg(2, JSTaggedValue::False());
    callInfo->SetCallArg(3, JSTaggedValue::Undefined());
    callInfo->SetCallArg(4, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(0));
}

};  // namespace panda::test
