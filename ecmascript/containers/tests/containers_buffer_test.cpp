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
                                                                 ContainersBufferTest::GetArgvCount(4));
        objCallInfo->SetFunction(newTarget.GetTaggedValue());
        objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
        objCallInfo->SetThis(JSTaggedValue::Undefined());
        objCallInfo->SetCallArg(0, JSTaggedValue(length));
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
        JSTaggedValue result = ContainersBuffer::BufferConstructor(objCallInfo);
        TestHelper::TearDownFrame(thread, prev);
        JSHandle<JSAPIFastBuffer> buffer(thread, result);
        return buffer;
    }
};

HWTEST_F_L0(ContainersBufferTest, BufferConstructor)
{
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                             ContainersBufferTest::GetArgvCount(4));
    objCallInfo->SetFunction(newTarget.GetTaggedValue());
    objCallInfo->SetNewTarget(newTarget.GetTaggedValue());
    objCallInfo->SetThis(JSTaggedValue::Undefined());
    objCallInfo->SetCallArg(0, JSTaggedValue(JSAPIFastBuffer::DEFAULT_CAPACITY_LENGTH));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
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
    objCallInfo->SetThis(JSTaggedValue::Undefined());
    objCallInfo->SetCallArg(0, arr.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSAPIFastBuffer> buffer(thread, result);
    ASSERT_EQ(result, JSTaggedValue::Exception());
}

};  // namespace panda::test
