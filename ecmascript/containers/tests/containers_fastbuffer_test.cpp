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
class ContainersFastBufferTest : public testing::Test {
public:
    static uint32_t GetArgvCount(uint32_t setCount)
    {
        return setCount * 2;  // 2 means every arg cover 2 length
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
                                                                 ContainersFastBufferTest::GetArgvCount(3));
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
                                                                 ContainersFastBufferTest::GetArgvCount(3));
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
                                                                 ContainersFastBufferTest::GetArgvCount(3));
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
        if (length > 0) {
            TypedArrayHelper::AllocateTypedArrayBuffer(thread, obj, length, arrayType);
        } else {
            auto jsTypedArray = JSTypedArray::Cast(*obj);
            jsTypedArray->SetContentType(arrayType);
            jsTypedArray->SetByteLength(0);
            jsTypedArray->SetByteOffset(0);
            jsTypedArray->SetArrayLength(0);
        }
        return JSHandle<JSTypedArray>(obj);
    }
};

// ==================== GetByteOffset / GetArrayBuffer / GetSize ====================

HWTEST_F_L0(ContainersFastBufferTest, GetByteOffset_001)
{
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(2));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::GetByteOffset(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(0));
}

HWTEST_F_L0(ContainersFastBufferTest, GetArrayBuffer_001)
{
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(2));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::GetArrayBuffer(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsArrayBuffer());
}

HWTEST_F_L0(ContainersFastBufferTest, GetSize_001)
{
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer(20);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(2));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::GetSize(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(20));
}

// ==================== Equals ====================

HWTEST_F_L0(ContainersFastBufferTest, Equals_001)
{
    // Same content buffers should be equal
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("abcde");
    JSHandle<JSAPIFastBuffer> target = CreateJSAPIBuffer("abcde");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(3));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Equals(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(true));
}

HWTEST_F_L0(ContainersFastBufferTest, Equals_002)
{
    // Different content buffers should not be equal
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("abcde");
    JSHandle<JSAPIFastBuffer> target = CreateJSAPIBuffer("fghij");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(3));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Equals(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(false));
}

HWTEST_F_L0(ContainersFastBufferTest, Equals_003)
{
    // Equals with Uint8Array (both zero-filled, same length)
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(3);
    JSHandle<JSTypedArray> target = NewUint8Array(3);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(3));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Equals(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    // Both zero-filled, should be equal
    EXPECT_EQ(result, JSTaggedValue(true));
}

// ==================== Fill ====================

HWTEST_F_L0(ContainersFastBufferTest, Fill_001)
{
    // Fill with number value
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x41));  // 'A'
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Fill(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSAPIBuffer());
}

HWTEST_F_L0(ContainersFastBufferTest, Fill_002)
{
    // Fill with number, start and end specified
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x42));  // 'B'
    callInfo->SetCallArg(1, JSTaggedValue(2));
    callInfo->SetCallArg(2, JSTaggedValue(8));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Fill(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSAPIBuffer());
}

HWTEST_F_L0(ContainersFastBufferTest, Fill_003)
{
    // Fill with string and valid encoding as 4th arg
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer(10);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> valStr = factory->NewFromASCII("a");
    JSHandle<EcmaString> encStr = factory->NewFromASCII("utf8");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, valStr.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, JSTaggedValue(10));
    callInfo->SetCallArg(3, encStr.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Fill(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSAPIBuffer());
}

HWTEST_F_L0(ContainersFastBufferTest, Fill_004)
{
    // Fill with string and valid encoding as 2nd arg (encoding-as-start pattern)
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer(10);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> valStr = factory->NewFromASCII("a");
    JSHandle<EcmaString> encStr = factory->NewFromASCII("utf8");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, valStr.GetTaggedValue());
    callInfo->SetCallArg(1, encStr.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Fill(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSAPIBuffer());
}

HWTEST_F_L0(ContainersFastBufferTest, Fill_005)
{
    // Fill with number where endIndex <= startIndex (should return buffer unchanged)
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x41));
    callInfo->SetCallArg(1, JSTaggedValue(5));
    callInfo->SetCallArg(2, JSTaggedValue(3));  // end < start
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Fill(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSAPIBuffer());
}

HWTEST_F_L0(ContainersFastBufferTest, Fill_006)
{
    // Fill zero-length buffer
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer(0);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x41));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Fill(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSAPIBuffer());
}

// ==================== IndexOf ====================

HWTEST_F_L0(ContainersFastBufferTest, IndexOf_001)
{
    // IndexOf with number value and no offset
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcde");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x63));  // 'c' = 0x63
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::IndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(2));
}

HWTEST_F_L0(ContainersFastBufferTest, IndexOf_002)
{
    // IndexOf with string value and encoding
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcde");
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> valStr = factory->NewFromASCII("cd");
    JSHandle<EcmaString> encStr = factory->NewFromASCII("utf8");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, valStr.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, encStr.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::IndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(2));
}

HWTEST_F_L0(ContainersFastBufferTest, IndexOf_003)
{
    // IndexOf with numeric offset
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcde");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x61));  // 'a'
    callInfo->SetCallArg(1, JSTaggedValue(2));     // offset=2
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::IndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    // 'a' not found after index 2
    EXPECT_EQ(result, JSTaggedValue(-1));
}

HWTEST_F_L0(ContainersFastBufferTest, IndexOf_004)
{
    // IndexOf with offset exceeding buffer length
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abc");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x61));
    callInfo->SetCallArg(1, JSTaggedValue(10));  // offset > length
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::IndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(-1));
}

HWTEST_F_L0(ContainersFastBufferTest, IndexOf_005)
{
    // IndexOf with negative offset (should normalize)
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcde");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x61));  // 'a'
    callInfo->SetCallArg(1, JSTaggedValue(-1));     // negative offset
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::IndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    // negative offset normalizes to length-1=4, 'a' at index 0 not found at >=4
    EXPECT_EQ(result, JSTaggedValue(-1));
}

HWTEST_F_L0(ContainersFastBufferTest, IndexOf_006)
{
    // IndexOf with buffer value
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcde");
    JSHandle<JSAPIFastBuffer> search = CreateJSAPIBuffer("cd");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, search.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::IndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(2));
}

HWTEST_F_L0(ContainersFastBufferTest, IndexOf_007)
{
    // IndexOf with value not found
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcde");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x7A));  // 'z'
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::IndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(-1));
}

// ==================== Includes ====================

HWTEST_F_L0(ContainersFastBufferTest, Includes_001)
{
    // Includes with number value found
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcde");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x63));  // 'c'
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Includes(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::True());
}

HWTEST_F_L0(ContainersFastBufferTest, Includes_002)
{
    // Includes with number value not found
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcde");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x7A));  // 'z'
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Includes(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::False());
}

HWTEST_F_L0(ContainersFastBufferTest, Includes_003)
{
    // Includes with offset
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcde");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x61));  // 'a'
    callInfo->SetCallArg(1, JSTaggedValue(2));      // offset=2
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Includes(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::False());
}

HWTEST_F_L0(ContainersFastBufferTest, Includes_004)
{
    // Includes with offset > length
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abc");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x61));
    callInfo->SetCallArg(1, JSTaggedValue(10));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Includes(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::False());
}

HWTEST_F_L0(ContainersFastBufferTest, Includes_005)
{
    // Includes with negative offset normalization
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcde");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x65));  // 'e'
    callInfo->SetCallArg(1, JSTaggedValue(-1));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Includes(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::True());
}

// ==================== LastIndexOf ====================

HWTEST_F_L0(ContainersFastBufferTest, LastIndexOf_001)
{
    // LastIndexOf with number value
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcabc");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x61));  // 'a'
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::LastIndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(3));
}

HWTEST_F_L0(ContainersFastBufferTest, LastIndexOf_002)
{
    // LastIndexOf with offset
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcabc");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x61));  // 'a'
    callInfo->SetCallArg(1, JSTaggedValue(2));     // offset=2
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::LastIndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(0));
}

HWTEST_F_L0(ContainersFastBufferTest, LastIndexOf_003)
{
    // LastIndexOf with offset < -length (should return -1)
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abc");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x61));
    callInfo->SetCallArg(1, JSTaggedValue(-10));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::LastIndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(-1));
}

HWTEST_F_L0(ContainersFastBufferTest, LastIndexOf_004)
{
    // LastIndexOf with negative offset (normalization)
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abcde");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(0x61));  // 'a'
    callInfo->SetCallArg(1, JSTaggedValue(-2));     // normalizes to 5-2=3
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::LastIndexOf(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(0));
}

// ==================== Write ====================

HWTEST_F_L0(ContainersFastBufferTest, Write_001)
{
    // Write string with default offset/encoding
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer(10);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> strHandle = factory->NewFromASCII("hello");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, strHandle.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Write(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(5));
}

HWTEST_F_L0(ContainersFastBufferTest, Write_002)
{
    // Write string with null/undefined second arg (uses default offset+encoding)
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer(10);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> strHandle = factory->NewFromASCII("hi");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, strHandle.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Write(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(2));
}

HWTEST_F_L0(ContainersFastBufferTest, Write_003)
{
    // Write string with valid encoding as second arg
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer(10);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> strHandle = factory->NewFromASCII("hi");
    JSHandle<EcmaString> encStr = factory->NewFromASCII("utf8");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, strHandle.GetTaggedValue());
    callInfo->SetCallArg(1, encStr.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Write(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(2));
}

HWTEST_F_L0(ContainersFastBufferTest, Write_004)
{
    // Write string with numeric offset and length
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer(10);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> strHandle = factory->NewFromASCII("hello");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, strHandle.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, JSTaggedValue(3));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Write(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(3));
}

HWTEST_F_L0(ContainersFastBufferTest, Write_005)
{
    // Write with non-string first arg (should return 0)
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, JSTaggedValue(42));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Write(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(0));
}

// ==================== ToString ====================

HWTEST_F_L0(ContainersFastBufferTest, ToString_001)
{
    // ToString with default encoding (utf8)
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("hello");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::ToString(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsString());
}

HWTEST_F_L0(ContainersFastBufferTest, ToString_002)
{
    // ToString with valid encoding
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("hello");
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> encStr = factory->NewFromASCII("ascii");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(0, encStr.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::ToString(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsString());
}

HWTEST_F_L0(ContainersFastBufferTest, ToString_003)
{
    // ToString with start and end parameters
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("hello");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(1));
    callInfo->SetCallArg(2, JSTaggedValue(4));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::ToString(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsString());
}

HWTEST_F_L0(ContainersFastBufferTest, ToString_004)
{
    // ToString with negative start
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("hello");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(-2));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::ToString(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsString());
}

HWTEST_F_L0(ContainersFastBufferTest, ToString_005)
{
    // ToString with start >= end (should return empty string)
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("hello");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(4));
    callInfo->SetCallArg(2, JSTaggedValue(2));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::ToString(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsString());
}

HWTEST_F_L0(ContainersFastBufferTest, ToString_006)
{
    // ToString with end less than full length
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("helloworld");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, JSTaggedValue(5));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::ToString(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsString());
}

// ==================== Compare ====================

HWTEST_F_L0(ContainersFastBufferTest, Compare_001)
{
    // Compare where tStart >= tEnd but sStart < sEnd (returns 1)
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("abc");
    JSHandle<JSAPIFastBuffer> target = CreateJSAPIBuffer("abc");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(7));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(3));  // targetStart=3 >= targetEnd
    callInfo->SetCallArg(2, JSTaggedValue(2));  // targetEnd=2
    callInfo->SetCallArg(3, JSTaggedValue(0));  // sourceStart
    callInfo->SetCallArg(4, JSTaggedValue(2));  // sourceEnd
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(1));
}

HWTEST_F_L0(ContainersFastBufferTest, Compare_002)
{
    // Compare where src longer than target (memcmp returns 0 but lengths differ, a > b)
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("abcde");
    JSHandle<JSAPIFastBuffer> target = CreateJSAPIBuffer("abc");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(3));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, target.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Compare(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(1));
}

// ==================== Copy ====================

HWTEST_F_L0(ContainersFastBufferTest, Copy_001)
{
    // Copy to Uint8Array target
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("hello");
    JSHandle<JSTypedArray> dstArray = NewUint8Array(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, dstArray.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, JSTaggedValue(0));
    callInfo->SetCallArg(3, JSTaggedValue(5));
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Copy(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(5));
}

HWTEST_F_L0(ContainersFastBufferTest, Copy_002)
{
    // Copy with targetStart >= dstLength (should return 0)
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("hello");
    JSHandle<JSAPIFastBuffer> dst = CreateJSAPIBuffer(3);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, dst.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(5));  // targetStart >= dst.length
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Copy(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(0));
}

HWTEST_F_L0(ContainersFastBufferTest, Copy_003)
{
    // Copy with sEnd <= sStart (should return 0)
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer("hello");
    JSHandle<JSAPIFastBuffer> dst = CreateJSAPIBuffer(10);
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(6));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(src.GetTaggedValue());
    callInfo->SetCallArg(0, dst.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(0));
    callInfo->SetCallArg(2, JSTaggedValue(3));
    callInfo->SetCallArg(3, JSTaggedValue(2));  // sourceEnd < sourceStart
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Copy(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue(0));
}

// ==================== BufferConstructor: ArrayBuffer with no byteOffset/length args ====================

HWTEST_F_L0(ContainersFastBufferTest, BufferConstructorFromArrayBufferNoOffset)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayBuffer> ab = factory->NewJSArrayBuffer(8);
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(3));
    callInfo->SetFunction(newTarget.GetTaggedValue());
    callInfo->SetNewTarget(newTarget.GetTaggedValue());
    callInfo->SetCallArg(0, ab.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_NE(result, JSTaggedValue::Exception());
    ASSERT_TRUE(result.IsJSAPIBuffer());
    JSHandle<JSAPIFastBuffer> buffer(thread, result);
    EXPECT_EQ(buffer->GetLength(), 8);
}

HWTEST_F_L0(ContainersFastBufferTest, BufferConstructorFromArrayBufferNoLength)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayBuffer> ab = factory->NewJSArrayBuffer(8);
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(newTarget.GetTaggedValue());
    callInfo->SetNewTarget(newTarget.GetTaggedValue());
    callInfo->SetCallArg(0, ab.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(2));  // byteOffset but no length
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_NE(result, JSTaggedValue::Exception());
    ASSERT_TRUE(result.IsJSAPIBuffer());
    JSHandle<JSAPIFastBuffer> buffer(thread, result);
    // FromArrayBuffer: min(length, srcLength - byteOffset) = min(8, 8-2) = 6
    EXPECT_EQ(buffer->GetLength(), 6);
}

HWTEST_F_L0(ContainersFastBufferTest, BufferConstructorFromArrayBufferOffsetExceeds)
{
    // byteOffset >= srcLength should throw range error (FromArrayBuffer line 274)
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSArrayBuffer> ab = factory->NewJSArrayBuffer(4);
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(newTarget.GetTaggedValue());
    callInfo->SetNewTarget(newTarget.GetTaggedValue());
    callInfo->SetCallArg(0, ab.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(10));  // offset > byteLength
    callInfo->SetCallArg(2, JSTaggedValue(2));   // length
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result, JSTaggedValue::Exception());
}

// ==================== Entries / Keys / Values ====================

HWTEST_F_L0(ContainersFastBufferTest, Entries_001)
{
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abc");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(2));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Entries(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSArrayIterator());
}

HWTEST_F_L0(ContainersFastBufferTest, Keys_001)
{
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abc");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(2));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Keys(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSArrayIterator());
}

HWTEST_F_L0(ContainersFastBufferTest, Values_001)
{
    JSHandle<JSAPIFastBuffer> buffer = CreateJSAPIBuffer("abc");
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(2));
    callInfo->SetFunction(JSTaggedValue::Undefined());
    callInfo->SetThis(buffer.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::Values(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSArrayIterator());
}

// ==================== AllocateFromBufferObject: offset+length exceeds byteLength ====================

HWTEST_F_L0(ContainersFastBufferTest, BufferConstructorShareFromBufferWithOffsetAndLength)
{
    // Valid offset + length within bounds
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(10);
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(newTarget.GetTaggedValue());
    callInfo->SetNewTarget(newTarget.GetTaggedValue());
    callInfo->SetCallArg(0, src.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(2));  // byteOffset
    callInfo->SetCallArg(2, JSTaggedValue(5));  // length
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.IsJSAPIBuffer());
    JSHandle<JSAPIFastBuffer> buffer(thread, result);
    // actualAvailable = 10 - 2 = 8, maxLength = min(8, 5) = 5
    EXPECT_EQ(buffer->GetLength(), 5);
    EXPECT_EQ(buffer->GetOffset(), 2);
}

HWTEST_F_L0(ContainersFastBufferTest, BufferConstructorShareFromBufferOffsetPlusLengthExceeds)
{
    // offset + length exceeds byteLength -> maxLength should be truncated
    JSHandle<JSAPIFastBuffer> src = CreateJSAPIBuffer(10);
    InitializeBufferConstructor();
    JSHandle<JSFunction> newTarget(thread, InitializeBufferConstructor());
    auto callInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(),
                                                          GetArgvCount(5));
    callInfo->SetFunction(newTarget.GetTaggedValue());
    callInfo->SetNewTarget(newTarget.GetTaggedValue());
    callInfo->SetCallArg(0, src.GetTaggedValue());
    callInfo->SetCallArg(1, JSTaggedValue(8));   // byteOffset=8
    callInfo->SetCallArg(2, JSTaggedValue(10));  // length=10, but only 2 bytes available
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, callInfo);
    JSTaggedValue result = ContainersBuffer::BufferConstructor(callInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.IsJSAPIBuffer());
    JSHandle<JSAPIFastBuffer> buffer(thread, result);
    // actualAvailable = 10 - 8 = 2, maxLength = min(2, 10) = 2
    EXPECT_EQ(buffer->GetLength(), 2);
    EXPECT_EQ(buffer->GetOffset(), 8);
}
};  // namespace panda::test