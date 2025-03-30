/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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
#include "ecmascript/builtins/builtins_shared_typedarray.h"

#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_arraybuffer.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_array_iterator.h"

#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"

#include "ecmascript/object_factory.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/tests/test_helper.h"
#include "builtin_test_util.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {

class BuiltinsSharedTypedArrayTest : public BaseTestWithScope<false> {
    public:
    class TestClass : public base::BuiltinsBase {
    public:

        static JSTaggedValue TestReduceRightFunc(EcmaRuntimeCallInfo *argv)
        {
            int accumulator = GetCallArg(argv, 0)->GetInt();
            accumulator = accumulator + GetCallArg(argv, 1)->GetInt();
            return BuiltinsBase::GetTaggedInt(accumulator);
        }
    };
};

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, TypedArrayBaseConstructor_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*typed_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::TypedArrayBaseConstructor(ecmaRuntimeCallInfo1);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Int8ArrayConstructor1_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> int8_array(env->GetInt8ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*int8_array), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*int8_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::Int8ArrayConstructor(ecmaRuntimeCallInfo1);
    ASSERT_TRUE(!result.IsECMAObject());
}


// Scenario2: Test when argv is not nullptr then Uint8ArrayConstructor returns not nullptr.
HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Uint8ArrayConstructor_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> uint8_array(env->GetUint8ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*uint8_array), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*uint8_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::Uint8ArrayConstructor(ecmaRuntimeCallInfo1);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Uint8ClampedArrayConstructor_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> int8c_array(env->GetUint8ClampedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*int8c_array), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*int8c_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::Uint8ClampedArrayConstructor(ecmaRuntimeCallInfo1);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Int16ArrayConstructor_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> int16_array(env->GetInt16ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*int16_array), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*int16_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::Int16ArrayConstructor(ecmaRuntimeCallInfo1);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Uint16ArrayConstructor_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> uint16_array(env->GetUint16ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*uint16_array), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*uint16_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::Uint16ArrayConstructor(ecmaRuntimeCallInfo1);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Int32ArrayConstructor_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> int32_array(env->GetInt32ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*int32_array), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*int32_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::Int32ArrayConstructor(ecmaRuntimeCallInfo1);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Uint32ArrayConstructor_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> uint32_array(env->GetUint32ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*uint32_array), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*uint32_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::Uint32ArrayConstructor(ecmaRuntimeCallInfo1);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Float32ArrayConstructor_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> float32_array(env->GetFloat32ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*float32_array), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*float32_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::Float32ArrayConstructor(ecmaRuntimeCallInfo1);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Float64ArrayConstructor_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> float64_array(env->GetFloat64ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*float64_array), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*float64_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::Float64ArrayConstructor(ecmaRuntimeCallInfo1);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, BigInt64ArrayConstructor_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> bigint64_array(env->GetBigInt64ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*bigint64_array), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*bigint64_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::BigInt64ArrayConstructor(ecmaRuntimeCallInfo1);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, BigUint64ArrayConstructor_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> biguint64_array(env->GetBigUint64ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*biguint64_array), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue(*biguint64_array));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsarray.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::BigUint64ArrayConstructor(ecmaRuntimeCallInfo1);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Of_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Of(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Of_Test2)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> array(env->GetSharedInt8ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(3)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::Of(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsJSSharedInt8Array());
    JSHandle<JSTaggedValue> resultArr = JSHandle<JSTaggedValue>(thread, result);
    EXPECT_EQ(JSSharedTypedArray::GetProperty(thread, resultArr, 0).GetValue()->GetInt(), 1);
    EXPECT_EQ(JSSharedTypedArray::GetProperty(thread, resultArr, 1).GetValue()->GetInt(), 2);
    EXPECT_EQ(JSSharedTypedArray::GetProperty(thread, resultArr, 2).GetValue()->GetInt(), 3);
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Species_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Species(argv);
    ASSERT_TRUE(result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, GetBuffer_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::GetBuffer(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, GetByteLength_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::GetByteLength(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, GetByteOffset_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::GetByteOffset(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, CopyWithin_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::CopyWithin(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Entries_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Entries(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Every_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Every(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Fill_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Fill(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Filter_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Filter(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Find_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Find(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, FindIndex_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::FindIndex(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, ForEach_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::ForEach(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, IndexOf_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::IndexOf(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, LastIndexOf_One)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));

    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);

    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::LastIndexOf(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, LastIndexOf_Two)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<TaggedArray> array(factory->NewSTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(1));
    array->Set(thread, 1, JSTaggedValue(2));
    array->Set(thread, 2, JSTaggedValue(3));

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateSharedTypedArray(thread, array));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(2));
    // find valid index 1
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::LastIndexOf(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(1).GetRawData());

    // find invalid index -1
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(5));
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    result = BuiltinsSharedTypedArray::LastIndexOf(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(-1).GetRawData());

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(obj.GetTaggedValue());
    // find valid index
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue(1));
    ecmaRuntimeCallInfo2->SetCallArg(1, JSTaggedValue(2));
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    result = BuiltinsSharedTypedArray::LastIndexOf(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(0).GetRawData());

    // find valid index
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue(3));
    ecmaRuntimeCallInfo2->SetCallArg(1, JSTaggedValue(1));
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    result = BuiltinsSharedTypedArray::LastIndexOf(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(-1).GetRawData());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Join_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Join(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Keys_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Keys(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, GetLength_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::GetLength(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Map_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Map(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Reduce_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Reduce(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, ReduceRight_One)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));

    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);

    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::ReduceRight(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, ReduceRight_Two)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<TaggedArray> array(factory->NewSTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(1));
    array->Set(thread, 1, JSTaggedValue(2));
    array->Set(thread, 2, JSTaggedValue(3));

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateSharedTypedArray(thread, array));

    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestReduceRightFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    // set initial value to 10
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(10)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::ReduceRight(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(16).GetRawData());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, ReduceRight_Three)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<TaggedArray> array(factory->NewSTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(1));
    array->Set(thread, 1, JSTaggedValue(2));
    array->Set(thread, 2, JSTaggedValue(3));

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateSharedTypedArray(thread, array));

    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestReduceRightFunc));
    // no initial value
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = BuiltinsSharedTypedArray::ReduceRight(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(6).GetRawData());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Reverse_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Reverse(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Set_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Set(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Slice_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Slice(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Some_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Some(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Sort_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Sort(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Subarray_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Subarray(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, ToString_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::ToLocaleString(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, ToString_Test2)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<TaggedArray> array(factory->NewSTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(1));
    array->Set(thread, 1, JSTaggedValue(2));
    array->Set(thread, 2, JSTaggedValue(3));

    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, BuiltTestUtil::CreateSharedTypedArray(thread, array));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSharedTypedArray::ToString(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));

    JSHandle<EcmaString> str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("1,2,3");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, resultHandle, str), 0);
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Values_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Values(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, ToStringTag_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::ToStringTag(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, At_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::At(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, Includes_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::Includes(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedTypedArrayTest, From_Test1)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> array(factory->NewTaggedArray(2));
    JSHandle<JSTaggedValue> val0(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> val1(thread, JSTaggedValue(2));
    
    array->Set(thread, 0, val0);
    array->Set(thread, 1, val1);
    
    JSHandle<JSTaggedValue> jsarray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> typed_array(env->GetTypedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto argv = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*typed_array), 6);
    argv->SetFunction(JSTaggedValue(*typed_array));
    argv->SetThis(JSTaggedValue(*globalObject));
    argv->SetCallArg(0, jsarray.GetTaggedValue());
    JSTaggedValue result = BuiltinsSharedTypedArray::From(argv);
    ASSERT_TRUE(!result.IsECMAObject());
}

} // namespace panda::test