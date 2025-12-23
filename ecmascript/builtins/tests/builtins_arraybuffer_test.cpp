/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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
#include "ecmascript/builtins/builtins_arraybuffer.h"

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"

#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsArrayBufferTest : public BaseTestWithScope<false> {
};

// new ArrayBuffer(8)
HWTEST_F_L0(BuiltinsArrayBufferTest, Constructor1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> arrayBuffer(thread, env->GetArrayBufferFunction().GetTaggedValue());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, arrayBuffer.GetTaggedValue(), 6);
    ecmaRuntimeCallInfo->SetFunction(arrayBuffer.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(8)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsArrayBuffer::ArrayBufferConstructor(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsECMAObject());
    TestHelper::TearDownFrame(thread, prev);
}

// (new ArrayBuffer(5)).byteLength
HWTEST_F_L0(BuiltinsArrayBufferTest, byteLength1)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsArrayBuffer(thread, 5);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsArrayBuffer::GetByteLength(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(5).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// (new ArrayBuffer(10)).slice(1, 5).bytelength
HWTEST_F_L0(BuiltinsArrayBufferTest, slice1)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsArrayBuffer::Slice(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<JSArrayBuffer> arrBuf1(thread,
                                     JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(result1.GetRawData())));
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(arrBuf1.GetTaggedValue());
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = BuiltinsArrayBuffer::GetByteLength(ecmaRuntimeCallInfo1);

    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(4).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

/*

@tc.name: isView1

@tc.desc: Judging input parameter

@tc.type: FUNC
*/
HWTEST_F_L0(BuiltinsArrayBufferTest, isView1)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsArrayBuffer::IsView(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_FALSE(result.JSTaggedValue::ToBoolean());
}

/*

@tc.name: isView2

@tc.desc: Judging input parameter

@tc.type: FUNC
*/
HWTEST_F_L0(BuiltinsArrayBufferTest, isView2)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread,
    JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsArrayBuffer::IsView(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_FALSE(result.JSTaggedValue::ToBoolean());
}

/*

@tc.name: isView2

@tc.desc: Judging input parameter

@tc.type: FUNC
*/
HWTEST_F_L0(BuiltinsArrayBufferTest, isView3)
{
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);

    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsArrayBuffer::IsView(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_FALSE(result.JSTaggedValue::ToBoolean());
}

/*

@tc.name: isView2

@tc.desc: Judging input parameter

@tc.type: FUNC
*/
HWTEST_F_L0(BuiltinsArrayBufferTest, isView4)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(2));
    array->Set(thread, 1, JSTaggedValue(3));
    array->Set(thread, 2, JSTaggedValue(4));

    [[maybe_unused]] JSHandle<JSTaggedValue> obj =
    JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateTypedArray(thread, array));

    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsArrayBuffer::IsView(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.JSTaggedValue::ToBoolean());
}

/*

@tc.name: isView2

@tc.desc: Judging input parameter

@tc.type: FUNC
*/
HWTEST_F_L0(BuiltinsArrayBufferTest, isView5)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewSTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(2));
    array->Set(thread, 1, JSTaggedValue(3));
    array->Set(thread, 2, JSTaggedValue(4));

    [[maybe_unused]] JSHandle<JSTaggedValue> obj =
    JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateSharedTypedArray(thread, array));

    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsArrayBuffer::IsView(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_FALSE(result.JSTaggedValue::ToBoolean());
}

/*

@tc.name: Species

@tc.desc: Species Function

@tc.type: FUNC
*/
HWTEST_F_L0(BuiltinsArrayBufferTest, Species)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsArrayBuffer(thread, 10);
    JSHandle<JSArrayBuffer> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(arrBuf.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsArrayBuffer::Species(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.IsECMAObject());
}

/*

@tc.name: IsDetached
@tc.desc: IsDetached Function
@tc.type: FUNC
*/
HWTEST_F_L0(BuiltinsArrayBufferTest, IsDetached)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsArrayBuffer(thread, 10);
    JSHandle<JSTaggedValue> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    bool result = BuiltinsArrayBuffer::IsDetachedBuffer(thread, arrBuf.GetTaggedValue());
    ASSERT_FALSE(result);
}
/*

@tc.name: IsDetached
@tc.desc: CloneArrayBuffer Function
@tc.type: FUNC
*/
HWTEST_F_L0(BuiltinsArrayBufferTest, CloneArrayBuffer)
{
    JSTaggedValue tagged = BuiltTestUtil::CreateBuiltinsArrayBuffer(thread, 10);
    JSHandle<JSTaggedValue> arrBuf(thread, JSArrayBuffer::Cast(reinterpret_cast<TaggedObject *>(tagged.GetRawData())));
    JSHandle<JSTaggedValue> constructor(thread, JSTaggedValue::Undefined());
    BuiltinsArrayBuffer::CloneArrayBuffer(thread, arrBuf, 0, constructor);
    bool result = BuiltinsArrayBuffer::IsDetachedBuffer(thread, arrBuf.GetTaggedValue());
    ASSERT_FALSE(result);
}
}  // namespace panda::test
