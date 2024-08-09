/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "builtin_test_util.h"
#include "ecmascript/builtins/builtins_sendable_arraybuffer.h"

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"

#include "ecmascript/shared_objects/js_sendable_arraybuffer.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsSendableArrayBufferTest : public BaseTestWithScope<false> {
};


void SendableArrayBufferSliceTest(JSThread *thread, JSTaggedValue start, JSTaggedValue end, JSTaggedValue res)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsSendableArrayBuffer(thread, 10);
    JSHandle<JSSendableArrayBuffer> arrBuf(
        thread, JSSendableArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, start);
    ecmaRuntimeCallInfo->SetCallArg(1, end);

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsSendableArrayBuffer::Slice(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSSendableArrayBuffer> arrBuf1(
        thread, JSSendableArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData())));
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(arrBuf1.GetTaggedValue());
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = BuiltinsSendableArrayBuffer::GetByteLength(ecmaRuntimeCallInfo1);

    ASSERT_EQ(result2.GetRawData(), res.GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// new SendableArrayBuffer(8)
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, Constructor1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> arrayBuffer(thread, env->GetSBuiltininArrayBufferFunction().GetTaggedValue());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, arrayBuffer.GetTaggedValue(), 6);
    ecmaRuntimeCallInfo->SetFunction(arrayBuffer.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(8)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSendableArrayBuffer::ArrayBufferConstructor(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsECMAObject());
    TestHelper::TearDownFrame(thread, prev);
}

// Constructor: Throw TypeError
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, Constructor2)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSendableArrayBuffer::ArrayBufferConstructor(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::Exception().GetRawData());
}

// (new SendableArrayBuffer(5)).byteLength
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, byteLength1)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsSendableArrayBuffer(thread, 5);
    JSHandle<JSSendableArrayBuffer> arrBuf(
        thread, JSSendableArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSendableArrayBuffer::GetByteLength(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(5).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// Species
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, species1)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsSendableArrayBuffer(thread, 5);
    JSHandle<JSSendableArrayBuffer> arrBuf(
        thread, JSSendableArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSendableArrayBuffer::Species(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), arrBuf.GetTaggedValue().GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// (new SendableArrayBuffer(10)).slice(1, 5).bytelength
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, slice1)
{
    SendableArrayBufferSliceTest(
        thread, JSTaggedValue(static_cast<int32_t>(1)), JSTaggedValue(static_cast<int32_t>(5)), JSTaggedValue(4));
}

// (new SendableArrayBuffer(10)).slice(-1, undefine).bytelength
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, slice2)
{
    SendableArrayBufferSliceTest(
        thread, JSTaggedValue(static_cast<int32_t>(-1)), JSTaggedValue::Undefined(), JSTaggedValue(1));
}

// (new SendableArrayBuffer(10)).slice(1, -2).bytelength
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, slice3)
{
    SendableArrayBufferSliceTest(
        thread, JSTaggedValue(static_cast<int32_t>(1)), JSTaggedValue(static_cast<int32_t>(-2)), JSTaggedValue(7));
}

// slice :this is not obj
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, slice4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSendableArrayBuffer::Slice(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::Exception().GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// slice :this is not sendable array
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, slice5)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf(
        thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSendableArrayBuffer::Slice(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::Exception().GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// slice :this is detach sendable array
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, slice6)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsSendableArrayBuffer(thread, 10);
    JSHandle<JSSendableArrayBuffer> arrBuf(
        thread, JSSendableArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    arrBuf->SetArrayBufferData(thread, JSTaggedValue::Null());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSendableArrayBuffer::Slice(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::Exception().GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// IsView(new SharedTypedArray())
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, IsView1)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewSTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(2));
    array->Set(thread, 1, JSTaggedValue(3));
    array->Set(thread, 2, JSTaggedValue(4));

    [[maybe_unused]] JSHandle<JSTaggedValue> obj =
        JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateSharedTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSendableArrayBuffer::IsView(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(true).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// IsView: Arg is not EcmaObj
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, IsView2)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewSTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(2));
    array->Set(thread, 1, JSTaggedValue(3));
    array->Set(thread, 2, JSTaggedValue(4));

    [[maybe_unused]] JSHandle<JSTaggedValue> obj =
        JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateSharedTypedArray(thread, array));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSendableArrayBuffer::IsView(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(false).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// IsView: arg is not dataView or SharedTypedArray
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, IsView3)
{
    ASSERT_NE(thread, nullptr);
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewSTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(2));
    array->Set(thread, 1, JSTaggedValue(3));
    array->Set(thread, 2, JSTaggedValue(4));

    [[maybe_unused]] JSHandle<JSTaggedValue> obj =
        JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateSharedTypedArray(thread, array));
    [[maybe_unused]] JSHandle<JSTaggedValue> obj1 =
        JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateBuiltinsSendableArrayBuffer(thread, 3));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, obj1.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSendableArrayBuffer::IsView(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(false).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// IsDetachBuffer: is byteArray
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, IsDetachBuffer1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<ByteArray> byteArray = factory->NewByteArray(5, 8);
    bool result = BuiltinsSendableArrayBuffer::IsDetachedBuffer(byteArray.GetTaggedValue());
    ASSERT_EQ(JSTaggedValue(result).GetRawData(), JSTaggedValue(false).GetRawData());
}

// IsDetachBuffer: dataSlot is null
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, IsDetachBuffer2)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsSendableArrayBuffer(thread, 5);
    JSHandle<JSSendableArrayBuffer> arrBuf(
        thread, JSSendableArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    arrBuf->SetArrayBufferData(thread, JSTaggedValue::Null());
    bool result = BuiltinsSendableArrayBuffer::IsDetachedBuffer(tagged);
    ASSERT_EQ(JSTaggedValue(result).GetRawData(), JSTaggedValue(true).GetRawData());
}

// IsDetachBuffer: dataSlot is not null
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, IsDetachBuffer3)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsSendableArrayBuffer(thread, 5);
    JSHandle<JSSendableArrayBuffer> arrBuf(
        thread, JSSendableArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    arrBuf->SetArrayBufferData(thread, JSTaggedValue(1));
    bool result = BuiltinsSendableArrayBuffer::IsDetachedBuffer(tagged);
    ASSERT_EQ(JSTaggedValue(result).GetRawData(), JSTaggedValue(false).GetRawData());
}

// AllocateSendableArrayBuffer: bytelength is greater than INT_MAX
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, AllocateSendableArrayBuffer1)
{
    uint64_t byteLength = static_cast<uint64_t>(INT_MAX) + 1;

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> arrayBuffer(thread, env->GetSBuiltininArrayBufferFunction().GetTaggedValue());
    JSTaggedValue result = BuiltinsSendableArrayBuffer::AllocateSendableArrayBuffer(
        thread, JSHandle<JSTaggedValue>(arrayBuffer), byteLength);
    ASSERT_EQ(JSTaggedValue(result).GetRawData(), JSTaggedValue::Exception().GetRawData());
}

// AllocateSendableArrayBuffer
HWTEST_F_L0(BuiltinsSendableArrayBufferTest, AllocateSendableArrayBuffer2)
{
    uint32_t byteLength = 8;
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> arrayBuffer(thread, env->GetSBuiltininArrayBufferFunction().GetTaggedValue());
    JSTaggedValue result = BuiltinsSendableArrayBuffer::AllocateSendableArrayBuffer(
        thread, JSHandle<JSTaggedValue>(arrayBuffer), byteLength);
    ASSERT_TRUE(result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSendableArrayBufferTest, CloneArrayBuffer1)
{
    uint32_t srcByteOffset = 0;
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsSendableArrayBuffer(thread, 5);
    JSHandle<JSSendableArrayBuffer> arrBuf(
        thread, JSSendableArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    arrBuf->SetArrayBufferData(thread, JSTaggedValue::Null());
    JSHandle<JSTaggedValue> constructor(thread, JSTaggedValue::Undefined());
    JSTaggedValue result = BuiltinsSendableArrayBuffer::CloneArrayBuffer(
        thread, JSHandle<JSTaggedValue>(thread, tagged), srcByteOffset, constructor);
    ASSERT_EQ(JSTaggedValue(result).GetRawData(), JSTaggedValue::Exception().GetRawData());
}

HWTEST_F_L0(BuiltinsSendableArrayBufferTest, CloneArrayBuffer2)
{
    uint32_t srcByteOffset = 0;

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> arrayBuffer(thread, env->GetSBuiltininArrayBufferFunction().GetTaggedValue());

    JSHandle<ByteArray> byteArray = factory->NewByteArray(5, 8);
    JSTaggedValue result = BuiltinsSendableArrayBuffer::CloneArrayBuffer(
        thread, JSHandle<JSTaggedValue>(byteArray), srcByteOffset, JSHandle<JSTaggedValue>(arrayBuffer));

    JSHandle<JSSendableArrayBuffer> arrBuf(
        thread, JSSendableArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData())));
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(arrBuf.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = BuiltinsSendableArrayBuffer::GetByteLength(ecmaRuntimeCallInfo1);

    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(40).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsSendableArrayBufferTest, CloneArrayBuffer3)
{
    uint32_t srcByteOffset = 0;

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsSendableArrayBuffer(thread, 6);
    JSHandle<JSFunction> arrayBuffer(thread, env->GetSBuiltininArrayBufferFunction().GetTaggedValue());
    JSTaggedValue result = BuiltinsSendableArrayBuffer::CloneArrayBuffer(
        thread, JSHandle<JSTaggedValue>(thread, tagged), srcByteOffset, JSHandle<JSTaggedValue>(arrayBuffer));

    JSHandle<JSSendableArrayBuffer> arrBuf(
        thread, JSSendableArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(result.GetRawData())));
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(arrBuf.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = BuiltinsSendableArrayBuffer::GetByteLength(ecmaRuntimeCallInfo1);

    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(6).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}
}  // namespace panda::test
