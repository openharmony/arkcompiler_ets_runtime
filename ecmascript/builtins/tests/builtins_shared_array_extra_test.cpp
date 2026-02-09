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

#include "ecmascript/builtins/builtins_shared_array.h"

#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/shared_objects/js_shared_array_iterator.h"
#include "ecmascript/js_array_iterator.h"

#include "ecmascript/builtins/builtins_shared_map.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/shared_objects/js_shared_map_iterator.h"
#include "ecmascript/builtins/builtins_shared_set.h"
#include "ecmascript/shared_objects/js_shared_set.h"
#include "ecmascript/shared_objects/js_shared_set_iterator.h"

#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"

#include "ecmascript/object_factory.h"
#include "ecmascript/object_operator.h"
#include "ecmascript/tests/test_helper.h"
#include "ecmascript/builtins/tests/builtin_test_util.h"
#include "ecmascript/builtins/builtins_object.h"
#include "ecmascript/js_array.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using namespace panda::ecmascript::base;

enum class ArrayIndex {
    ARRAY_INDEX_0,
    ARRAY_INDEX_1,
    ARRAY_INDEX_2,
    ARRAY_INDEX_3
};

namespace panda::test {
using Array = ecmascript::builtins::BuiltinsSharedArray;
class BuiltinsSharedArrayExtraTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, ArrayConstructor)
{
    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    JSHandle<JSObject> globalObject(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 10);
    ecmaRuntimeCallInfo->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = Array::ArrayConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSObject::GetOwnProperty(thread, valueHandle, key0, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(1));
    JSObject::GetOwnProperty(thread, valueHandle, key1, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(3));
    JSObject::GetOwnProperty(thread, valueHandle, key2, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(5));
}

// Array.from ( items [ , mapfn [ , thisArg ] ] )
HWTEST_F_L0(BuiltinsSharedArrayExtraTest, From)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 6);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::From(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    std::vector<int> vals{1, 2, 3, 4, 5};
    BuiltTestUtil::SharedArrayCheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);
}


HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Species)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> array(env->GetArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(globalObject.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Species(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Concat)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    std::vector<int> descVals{1, 2, 3};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSSharedArray *arr1 = JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSSharedArray>();
    EXPECT_TRUE(arr1 != nullptr);
    JSHandle<JSObject> obj1(thread, arr1);

    std::vector<int> descVals2{4, 5, 6};
    keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj1, descVals2);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, obj1.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Concat(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    JSHandle<JSTaggedValue> key7(thread, JSTaggedValue(5));
    EXPECT_EQ(
        JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle,
                                   SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 6);

    JSObject::GetOwnProperty(thread, valueHandle, key7, descRes);
}


HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Slice)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))  \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);
    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(4)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Slice(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle,
                                         SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 3);
    std::vector<int> vals{2, 3, 4};
    BuiltTestUtil::SharedArrayCheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Splice)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);
    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(100)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Splice(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
                                         SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 4);

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    EXPECT_EQ(
        JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle,
                                   SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 2);
    JSObject::GetOwnProperty(thread, valueHandle, keys[0], descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(2));
}

// new Array(1,2,3,4,5).Fill(0,1,3)
HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Fill)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(3)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Fill(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    std::vector<int32_t> vals{1, 0, 0, 4, 5};
    BuiltTestUtil::SharedArrayCheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);
}


// new Array().Pop()
HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Pop)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
                                         SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 0);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Pop(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSSharedArray::DefineOwnProperty(thread, obj, key0, desc0, SCheckMode::SKIP);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSSharedArray::DefineOwnProperty(thread, obj, key1, desc1, SCheckMode::SKIP);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSSharedArray::DefineOwnProperty(thread, obj, key2, desc2, SCheckMode::SKIP);

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(obj.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    result = Array::Pop(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(3).GetRawData());
}

// new Array(1,2,3).Push(...items)
HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Push)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
                        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSSharedArray::DefineOwnProperty(thread, obj, key0, desc0, SCheckMode::SKIP);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSSharedArray::DefineOwnProperty(thread, obj, key1, desc1, SCheckMode::SKIP);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSSharedArray::DefineOwnProperty(thread, obj, key2, desc2, SCheckMode::SKIP);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(4)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Push(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetNumber(), 5);

    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
                                         SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 5);
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(3));
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key3, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 4);
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(4));
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key4, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 5);
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, DefineOwnProperty_Array)
{
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)) \
                        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(arr)), true, true, true);
    bool res = JSSharedArray::DefineOwnProperty(thread, obj, key0, desc0, SCheckMode::SKIP);
    ASSERT_TRUE(!res);
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Sort_Exception)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::Sort(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result2.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Sort_Exception_1)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(1)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 1);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::Sort(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result2.IsECMAObject());
}


HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Create1)
{
    static int32_t len = 3;
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(len));
    ecmaRuntimeCallInfo1->SetCallArg(1, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Create(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.IsECMAObject());
    EXPECT_EQ(JSSharedArray::Cast(result.GetTaggedObject())->GetArrayLength(), len);
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Create2)
{
    static double len = 100;
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(len));
    ecmaRuntimeCallInfo1->SetCallArg(1, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Create(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.IsECMAObject());
    EXPECT_EQ(JSSharedArray::Cast(result.GetTaggedObject())->GetArrayLength(), len);
}

// Array.isArray(arg)
HWTEST_F_L0(BuiltinsSharedArrayExtraTest, IsArray)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::IsArray(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    result = Array::IsArray(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, GetIterator)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(obj.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);

    // test Values()
    JSTaggedValue result = Array::Values(ecmaRuntimeCallInfo);
    JSHandle<JSSharedArrayIterator> iter(thread, result);
    EXPECT_EQ(IterationKind::VALUE, iter->GetIterationKind());

    // test Keys()
    JSTaggedValue result1 = Array::Keys(ecmaRuntimeCallInfo);
    JSHandle<JSArrayIterator> iter1(thread, result1);
    EXPECT_EQ(IterationKind::KEY, iter1->GetIterationKind());

    // test entries()
    JSTaggedValue result2 = Array::Entries(ecmaRuntimeCallInfo);
    JSHandle<JSSharedArrayIterator> iter2(thread, result2);
    EXPECT_EQ(IterationKind::KEY_AND_VALUE, iter2->GetIterationKind());
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Unscopables)
{
    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Unscopables(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, ShrinkTo1)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(10))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 6);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::ShrinkTo(ecmaRuntimeCallInfo1);
    EXPECT_EQ(result, JSTaggedValue::Undefined());
    EXPECT_EQ(arr->GetArrayLength(), 3U);
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, ShrinkTo2)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(10))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 6);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<double>(3)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::ShrinkTo(ecmaRuntimeCallInfo1);
    EXPECT_EQ(result, JSTaggedValue::Undefined());
    EXPECT_EQ(arr->GetArrayLength(), 3U);
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, ExtendTo1)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo1->SetCallArg(1, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::ExtendTo(ecmaRuntimeCallInfo1);
    ASSERT_EQ(result, JSTaggedValue::Undefined());
    EXPECT_EQ(arr->GetArrayLength(), 3U);
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, ExtendTo2)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetSharedArrayFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<double>(3)));
    ecmaRuntimeCallInfo1->SetCallArg(1, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::ExtendTo(ecmaRuntimeCallInfo1);
    ASSERT_EQ(result, JSTaggedValue::Undefined());
    EXPECT_EQ(arr->GetArrayLength(), 3U);
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, IncludeInSortedValue_Len_0)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 0);
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);

    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(0));
    auto res = JSSharedArray::IncludeInSortedValue(thread, JSHandle<JSTaggedValue>(obj), value1);
    ASSERT_TRUE(!res);
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, DefineProperty)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(2))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> function(thread, globalEnv->GetObjectFunction().GetObject<JSFunction>());

    JSHandle<JSFunction> objFunc(globalEnv->GetObjectFunction());
    JSHandle<JSObject> attHandle =
        ecmaVM->GetFactory()->NewJSObjectByConstructor(JSHandle<JSFunction>(function), function);

    JSHandle<JSTaggedValue> key(thread, JSTaggedValue(1));

    auto objCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    objCallInfo->SetFunction(JSTaggedValue::Undefined());
    objCallInfo->SetThis(JSTaggedValue::Undefined());
    objCallInfo->SetCallArg(0, obj.GetTaggedValue());
    objCallInfo->SetCallArg(1, key.GetTaggedValue());
    objCallInfo->SetCallArg(2, attHandle.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, objCallInfo);
    JSTaggedValue result = BuiltinsObject::DefineProperty(objCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsECMAObject());
    EXPECT_EQ(arr->GetArrayLength(), 2U);
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Of_Three)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> array(env->GetSharedArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo1->SetCallArg(1, env->GetArrayFunction().GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(6)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Of(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsException());
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Of_Four)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> array(env->GetArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, array.GetTaggedValue(), 10);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo1->SetCallArg(1, env->GetArrayFunction().GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(6)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Of(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsException());
}


HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Shift)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Shift(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(1).GetRawData());
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Sort)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{3, 2, 1};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::Sort(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result2.IsECMAObject());
    JSHandle<JSTaggedValue> resultArr =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result2.GetRawData())));
    EXPECT_EQ(JSSharedArray::GetProperty(thread, resultArr, keys[0], SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 1);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, resultArr, keys[1], SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 2);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, resultArr, keys[2], SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 3);
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Unshift)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(4)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Unshift(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(5)).GetRawData());

    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle,
                                         SCheckMode::SKIP, SCheckMode::SKIP).GetValue()->GetInt(), 5);
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(0));
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key3, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 4);
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(1));
    EXPECT_EQ(JSSharedArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key4, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 5);
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, ToString)
{
    std::vector<int> descVals{2, 3, 4};
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, obj, lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<EcmaString> str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("2,3,4");
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::ToString(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));

    ASSERT_EQ(EcmaStringAccessor::Compare(instance, resultHandle, str), 0);
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Includes_one)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);
    std::vector<int> descVals{2, 3, 4};
    EXPECT_EQ(JSSharedArray::GetProperty(thread, obj, lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    [[maybe_unused]] JSTaggedValue result = Array::Includes(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.JSTaggedValue::ToBoolean());  // new Int8Array[2,3,4].includes(2)

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    result = Array::Includes(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(!result.JSTaggedValue::ToBoolean());  // new Int8Array[2,3,4].includes(1)

    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo3->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(1)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3);
    result = Array::Includes(ecmaRuntimeCallInfo3);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.JSTaggedValue::ToBoolean());  // new Int8Array[2,3,4].includes(3, 1)
}


HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Includes_two)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);
    std::vector<int> descVals{2, 3, 4};
    EXPECT_EQ(JSSharedArray::GetProperty(thread, obj, lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo4->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo4->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4);
    [[maybe_unused]] JSTaggedValue result = Array::Includes(ecmaRuntimeCallInfo4);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(!result.JSTaggedValue::ToBoolean());  // new Int8Array[2,3,4].includes(2, 5)

    auto ecmaRuntimeCallInfo5 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo5->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo5->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo5->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo5->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(-2)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo5);
    result = Array::Includes(ecmaRuntimeCallInfo5);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(!result.JSTaggedValue::ToBoolean());  // new Int8Array[2,3,4].includes(2, -2)
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, At_ONE)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    std::vector<int> descVals{2, 3, 4};
    JSHandle<JSTaggedValue> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, obj, lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);
    
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));
    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::At(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev1);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(2).GetRawData());

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));
    [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    result = Array::At(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev2);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(4).GetRawData());

    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(3)));
    [[maybe_unused]] auto prev3 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3);
    result = Array::At(ecmaRuntimeCallInfo3);
    TestHelper::TearDownFrame(thread, prev3);
    ASSERT_EQ(result, JSTaggedValue::Undefined());
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, At_TWO)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    std::vector<int> descVals{2, 3, 4};
    JSHandle<JSTaggedValue> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, obj, lengthKeyHandle, SCheckMode::SKIP, SCheckMode::SKIP). \
                                        GetValue()->GetInt(), 0);
    
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo4->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-1)));
    [[maybe_unused]] auto prev4 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4);
    JSTaggedValue result = Array::At(ecmaRuntimeCallInfo4);
    TestHelper::TearDownFrame(thread, prev4);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(4).GetRawData());

    auto ecmaRuntimeCallInfo5 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo5->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo5->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo5->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-3)));
    [[maybe_unused]] auto prev5 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo5);
    result = Array::At(ecmaRuntimeCallInfo5);
    TestHelper::TearDownFrame(thread, prev5);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(2).GetRawData());

    auto ecmaRuntimeCallInfo6 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo6->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo6->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo6->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-4)));
    [[maybe_unused]] auto prev6 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo6);
    result = Array::At(ecmaRuntimeCallInfo6);
    TestHelper::TearDownFrame(thread, prev6);
    ASSERT_EQ(result, JSTaggedValue::Undefined());
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, SetPropertyTest001)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(5)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);

    std::vector<int> descVals{2, 3, 4, 5, 7};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(static_cast<int32_t>(5)));
    EXPECT_EQ(true, JSSharedArray::SetProperty(thread, obj, 0, value, true, SCheckMode::CHECK));
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, SetPropertyTest002)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(5)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);

    std::vector<int> descVals{2, 3, 4, 5, 7};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(static_cast<int32_t>(10)));
    EXPECT_EQ(false, JSSharedArray::SetProperty(thread, obj, 5, value, true, SCheckMode::CHECK));
}

HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Join)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);
    EXPECT_EQ(JSSharedArray::GetProperty(thread, obj, lengthKeyHandle, SCheckMode::SKIP,
                                         SCheckMode::SKIP).GetValue()->GetInt(), 0);

    std::vector<int> descVals{2, 3, 4};
    auto keys = BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    JSHandle<EcmaString> str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("2,3,4");
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Join(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));

    ASSERT_EQ(EcmaStringAccessor::Compare(instance, resultHandle, str), 0);
}

// Test At with positive index out of bounds
HWTEST_F_L0(BuiltinsSharedArrayExtraTest, At_PositiveIndexOutOfBounds)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    std::vector<int> descVals{1, 2, 3};
    BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(10)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = Array::At(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_EQ(result, JSTaggedValue::Undefined());
}

// Test At with negative index out of bounds
HWTEST_F_L0(BuiltinsSharedArrayExtraTest, At_NegativeIndexOutOfBounds)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    std::vector<int> descVals{1, 2, 3};
    BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-10)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = Array::At(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_EQ(result, JSTaggedValue::Undefined());
}

// Test CopyWithin with negative target
HWTEST_F_L0(BuiltinsSharedArrayExtraTest, CopyWithin_NegativeTarget)
{
    std::vector<int> descVals{1, 2, 3, 4, 5};
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread,
                                             JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-2)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = Array::CopyWithin(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsECMAObject());
}

// Test Includes with empty array
HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Includes_EmptyArray)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = Array::Includes(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_FALSE(result.ToBoolean());
}

// Test LastIndexOf with empty array
HWTEST_F_L0(BuiltinsSharedArrayExtraTest, LastIndexOf_EmptyArray)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread, JSTaggedNumber(0)) \
        .GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = Array::LastIndexOf(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(-1).GetRawData());
}

// Test Reverse with empty array
HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Reverse_EmptyArray)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread,
                                             JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = Array::Reverse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsECMAObject());
    EXPECT_EQ(arr->GetArrayLength(), 0U);
}

// Test Reverse with single element
HWTEST_F_L0(BuiltinsSharedArrayExtraTest, Reverse_SingleElement)
{
    JSSharedArray *arr = JSSharedArray::Cast(JSSharedArray::ArrayCreate(thread,
                                             JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    std::vector<int> descVals{42};
    BuiltTestUtil::SharedArrayDefineOwnPropertyTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = Array::Reverse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsECMAObject());
    EXPECT_EQ(arr->GetArrayLength(), 1U);
}

}
