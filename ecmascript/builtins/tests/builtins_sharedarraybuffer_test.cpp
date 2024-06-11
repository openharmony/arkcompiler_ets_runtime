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

#include "builtin_test_util.h"
#include "ecmascript/builtins/builtins_sharedarraybuffer.h"

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/builtins/builtins_arraybuffer.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using BuiltinsArrayBuffer = builtins::BuiltinsArrayBuffer;

namespace panda::test {
class BuiltinsSharedArrayBufferTest : public BaseTestWithScope<false> {
};

JSArrayBuffer *CreateJSArrayBuffer(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> target = env->GetArrayBufferFunction();
    JSHandle<JSArrayBuffer> jsArrayBuffer =
        JSHandle<JSArrayBuffer>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(target), target));
    return *jsArrayBuffer;
}

// new ArrayBuffer(20)
HWTEST_F_L0(BuiltinsSharedArrayBufferTest, Constructor1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> sharedArrayBuffer(thread, env->GetSharedArrayBufferFunction().GetTaggedValue());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, sharedArrayBuffer.GetTaggedValue(), 6);
    ecmaRuntimeCallInfo->SetFunction(sharedArrayBuffer.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(20)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSharedArrayBuffer::SharedArrayBufferConstructor(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsECMAObject());
}

// (new ArrayBuffer(18)).byteLength
HWTEST_F_L0(BuiltinsSharedArrayBufferTest, byteLength1)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsSharedArrayBuffer(thread, 18);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSharedArrayBuffer::GetByteLength(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(18).GetRawData());
}

// (new ArrayBuffer(10)).slice(1, 5).bytelength
HWTEST_F_L0(BuiltinsSharedArrayBufferTest, slice1)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsSharedArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsSharedArrayBuffer::Slice(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSArrayBuffer> arrBuf1(thread,
                                    JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData())));
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(arrBuf1.GetTaggedValue());
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = BuiltinsSharedArrayBuffer::GetByteLength(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(4).GetRawData());

    // IsShared false
    JSTaggedValue tagged1 = BuiltTestUtil::CreateBuiltinsSharedArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf2(thread, JSArrayBuffer::Cast(
                                    reinterpret_cast<TaggedObject *>(tagged1.GetRawData())));
    arrBuf2->SetShared(false);
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(arrBuf2.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo2->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result3 = BuiltinsSharedArrayBuffer::Slice(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(thread->HasPendingException());
    EXPECT_EQ(result3, JSTaggedValue::Exception());
    thread->ClearException();
}

HWTEST_F_L0(BuiltinsSharedArrayBufferTest, IsSharedArrayBuffer)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsSharedArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf(thread,
                                   JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, arrBuf.GetTaggedValue()); // JSTaggedValue(static_cast<int32_t>(1))

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSharedArrayBuffer::IsSharedArrayBuffer(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result, JSTaggedValue::True());
    // Is Not ECMAObject
    ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    result = BuiltinsSharedArrayBuffer::IsSharedArrayBuffer(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result, JSTaggedValue::False());
    // Is Not sharedbuffer
    JSHandle<JSArrayBuffer> jsArrayBuffer(thread, CreateJSArrayBuffer(thread));
    ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, jsArrayBuffer.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    result = BuiltinsSharedArrayBuffer::IsSharedArrayBuffer(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result, JSTaggedValue::False());

    // sharedbuffer IsNull
    JSTaggedValue tagged1 = BuiltTestUtil::CreateBuiltinsSharedArrayBuffer(thread, 0);
    JSHandle<JSArrayBuffer> arrBuf1(thread,
                                    JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged1.GetRawData())));
    arrBuf1->SetArrayBufferData(thread, JSTaggedValue::Null());
    ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, arrBuf1.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    result = BuiltinsSharedArrayBuffer::IsSharedArrayBuffer(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result, JSTaggedValue::False());

    //  ArrayBuffer is not shared
    JSTaggedValue tagged2 = BuiltTestUtil::CreateBuiltinsSharedArrayBuffer(thread, 0);
    JSHandle<JSArrayBuffer> arrBuf2(thread,
                                    JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged2.GetRawData())));
    arrBuf2->SetShared(false);
    ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, arrBuf2.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsSharedArrayBuffer::IsSharedArrayBuffer(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1, JSTaggedValue::False());
}

HWTEST_F_L0(BuiltinsSharedArrayBufferTest, IsShared)
{
    // is not SharedBuffer
    JSHandle<JSArrayBuffer> jsArrayBuffer(thread, CreateJSArrayBuffer(thread));
    bool result = BuiltinsSharedArrayBuffer::IsShared(jsArrayBuffer.GetTaggedValue());
    ASSERT_EQ(result, false);
    // no BufferData
    JSTaggedValue tagged1 = BuiltTestUtil::CreateBuiltinsSharedArrayBuffer(thread, 0);
    JSHandle<JSArrayBuffer> arrBuf1(thread,
                                    JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged1.GetRawData())));
    arrBuf1->SetArrayBufferData(thread, JSTaggedValue::Null());
    bool result1 = BuiltinsSharedArrayBuffer::IsShared(arrBuf1.GetTaggedValue());
    ASSERT_EQ(result1, false);
}
}  // namespace panda::test
