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

#include "ecmascript/builtins/builtins_array.h"

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

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
using namespace panda::ecmascript::base;
constexpr int32_t INT_VALUE_0 = 0;
constexpr int32_t INT_VALUE_1 = 1;
constexpr int32_t INT_VALUE_2 = 2;
constexpr int32_t INT_VALUE_3 = 3;
constexpr int32_t INT_VALUE_4 = 4;
constexpr int32_t INT_VALUE_666 = 666;
constexpr uint32_t RUNTIME_CALL_INFO_PARA_0 = 0;
constexpr uint32_t RUNTIME_CALL_INFO_PARA_1 = 1;
constexpr uint32_t RUNTIME_CALL_INFO_PARA_NUM_4 = 4;
constexpr uint32_t RUNTIME_CALL_INFO_PARA_NUM_8 = 8;
constexpr uint32_t RUNTIME_CALL_INFO_PARA_NUM_10 = 10;

enum class ArrayIndex {
    ARRAY_INDEX_0,
    ARRAY_INDEX_1,
    ARRAY_INDEX_2,
    ARRAY_INDEX_3
};

namespace panda::test {
using Array = ecmascript::builtins::BuiltinsArray;
class BuiltinsArrayTest : public BaseTestWithScope<false> {
public:
    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForEachFunc(EcmaRuntimeCallInfo *argv)
        {
            JSHandle<JSTaggedValue> key = GetCallArg(argv, 0);
            if (key->IsUndefined()) {
                return JSTaggedValue::Undefined();
            }
            JSArray *jsArray = JSArray::Cast(GetThis(argv)->GetTaggedObject());
            uint32_t length = jsArray->GetArrayLength() + 1U;
            jsArray->SetArrayLength(argv->GetThread(), length);
            return JSTaggedValue::Undefined();
        }

        static JSTaggedValue TestEveryFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                if (GetCallArg(argv, 0)->GetInt() > 10) { // 10 : test case
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestMapFunc(EcmaRuntimeCallInfo *argv)
        {
            int accumulator = GetCallArg(argv, 0)->GetInt();
            accumulator = accumulator * 2; // 2 : mapped to 2 times the original value
            return BuiltinsBase::GetTaggedInt(accumulator);
        }

        static JSTaggedValue TestFlatMapFunc(EcmaRuntimeCallInfo *argv)
        {
            int accumulator = GetCallArg(argv, 0)->GetInt();
            accumulator = accumulator * 2; // 2 : mapped to 2 times the original value

            JSThread *thread = argv->GetThread();
            JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
            JSArray *arr =
                JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
            EXPECT_TRUE(arr != nullptr);
            JSHandle<JSObject> obj(thread, arr);
            auto property = JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle);
            EXPECT_EQ(property.GetValue()->GetInt(), 0);

            JSHandle<JSTaggedValue> key(thread, JSTaggedValue(0));
            PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(accumulator)),
                                                                    true, true, true);
            JSArray::DefineOwnProperty(thread, obj, key, desc);
            return obj.GetTaggedValue();
        }

        static JSTaggedValue TestFindFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                // 10 : test case
                if (GetCallArg(argv, 0)->GetInt() > 10) {
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestFindIndexFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                // 10 : test case
                if (GetCallArg(argv, 0)->GetInt() > 10) {
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestFindLastFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                // 20 : test case
                if (GetCallArg(argv, 0)->GetInt() > 20) {
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestFindLastIndexFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                // 20 : test case
                if (GetCallArg(argv, 0)->GetInt() > 20) {
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }

        static JSTaggedValue TestReduceFunc(EcmaRuntimeCallInfo *argv)
        {
            int accumulator = GetCallArg(argv, 0)->GetInt();
            accumulator = accumulator + GetCallArg(argv, 1)->GetInt();
            return BuiltinsBase::GetTaggedInt(accumulator);
        }

        static JSTaggedValue TestReduceRightFunc(EcmaRuntimeCallInfo *argv)
        {
            int accumulator = GetCallArg(argv, 0)->GetInt();
            accumulator = accumulator + GetCallArg(argv, 1)->GetInt();
            return BuiltinsBase::GetTaggedInt(accumulator);
        }

        static JSTaggedValue TestSomeFunc(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                if (GetCallArg(argv, 0)->GetInt() > 10) { // 10 : test case
                    return GetTaggedBoolean(true);
                }
            }
            return GetTaggedBoolean(false);
        }
    };
};

JSTaggedValue CreateBuiltinsArrayJSObject(JSThread *thread, const CString keyCStr)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();

    JSHandle<JSTaggedValue> hclass = globalEnv->GetObjectFunction();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> obj(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(hclass), hclass));
    JSHandle<JSTaggedValue> key(factory->NewFromASCII(&keyCStr[0]));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSObject::SetProperty(thread, obj, key, value);
    return obj.GetTaggedValue();
}

HWTEST_F_L0(BuiltinsArrayTest, ArrayConstructor)
{
    JSHandle<JSFunction> array(thread->GetEcmaVM()->GetGlobalEnv()->GetArrayFunction());
    JSHandle<JSObject> globalObject(thread, thread->GetEcmaVM()->GetGlobalEnv()->GetGlobalObject());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::ArrayConstructor(ecmaRuntimeCallInfo1);
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

std::vector<JSHandle<JSTaggedValue>> DefineOwnPropertyCommonTest(JSThread* thread, JSHandle<JSObject>& obj,
    std::vector<int>& vals)
{
    std::vector<JSHandle<JSTaggedValue>> keys;
    for (size_t i = 0; i < vals.size(); i++) {
        keys.push_back(JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<int>(i))));
        PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(vals[i])), true, true, true);
        JSArray::DefineOwnProperty(thread, obj, keys[i], desc0);
    }
    return keys;
}

std::vector<JSHandle<JSTaggedValue>> DefineOwnPropertyCommonTest(JSThread* thread, JSHandle<JSTaggedValue>& obj,
    std::vector<int>& vals)
{
    JSHandle<JSObject> jsObj(obj);
    std::vector<JSHandle<JSTaggedValue>> keys;
    for (size_t i = 0; i < vals.size(); i++) {
        keys.push_back(JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<int>(i))));
        PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(vals[i])), true, true, true);
        JSArray::DefineOwnProperty(thread, jsObj, keys[i], desc0);
    }
    return keys;
}

void CheckKeyValueCommon(JSThread* thread, JSHandle<JSObject>& valueHandle, PropertyDescriptor& descRes,
    std::vector<JSHandle<JSTaggedValue>>& keys, std::vector<int32_t>& vals)
{
    for (size_t i = 0; i < vals.size(); i++) {
        JSObject::GetOwnProperty(thread, valueHandle, keys[i], descRes);
        ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(vals[i]));
    }
}

// 22.1.2.1 Array.from ( items [ , mapfn [ , thisArg ] ] )
HWTEST_F_L0(BuiltinsArrayTest, From)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0))->GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
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
    CheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);
}

// 22.1.2.2 Array.isArray(arg)
HWTEST_F_L0(BuiltinsArrayTest, IsArray)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

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

HWTEST_F_L0(BuiltinsArrayTest, Of)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSFunction> array(env->GetArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(array.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(1)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Of(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    std::vector<JSHandle<JSTaggedValue>> keys{key0, key1, key2};
    std::vector<int> vals{1, 3, 5};
    CheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsArrayTest, Species)
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

HWTEST_F_L0(BuiltinsArrayTest, Concat)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    std::vector<int> descVals{1, 2, 3};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    JSArray *arr1 = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr1 != nullptr);
    JSHandle<JSObject> obj1(thread, arr1);

    std::vector<int> descVals2{4, 5, 6};
    keys = DefineOwnPropertyCommonTest(thread, obj1, descVals2);

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
        JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue()->GetInt(), 6);
    JSObject::GetOwnProperty(thread, valueHandle, key7, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(6));
}

// 22.1.3.3 new Array(1,2,3,4,5).CopyWithin(0,3,5)
HWTEST_F_L0(BuiltinsArrayTest, CopyWithin)
{
    std::vector<int> descVals{1, 2, 3, 4, 5};
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(0)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(3)));
    ecmaRuntimeCallInfo1->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::CopyWithin(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    std::vector<int> vals{4, 5, 3, 4, 5};
    CheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsArrayTest, Every)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{100, 200, 300};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    JSHandle<JSArray> jsArray = factory->NewJSArray();
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestEveryFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::Every(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_EQ(result2.GetRawData(), JSTaggedValue::True().GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, Map)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{50, 200, 3};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestMapFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Map(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    std::vector<int> vals{100, 400, 6};
    EXPECT_EQ(
        JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue()->GetInt(), 3);
    CheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsArrayTest, Reverse)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{50, 200, 3};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Reverse(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    EXPECT_EQ(
        JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue()->GetInt(), 3);
    std::vector<int> vals{3, 200, 50};
    CheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 3);
    CheckKeyValueCommon(thread, obj, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsArrayTest, COWArrayReverse)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{50, 200, 3};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    JSHandle<JSArray> cloneArray = thread->GetEcmaVM()->GetFactory()->CloneArrayLiteral(JSHandle<JSArray>(obj));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(cloneArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Reverse(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    EXPECT_EQ(
        JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue()->GetInt(), 3);
    
    std::vector<int> vals{3, 200, 50};
    CheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);

    JSHandle<JSObject> cloneHandle(cloneArray);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(cloneArray),
        lengthKeyHandle).GetValue()->GetInt(), 3);
    CheckKeyValueCommon(thread, cloneHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsArrayTest, Slice)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);
    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

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
    EXPECT_EQ(
        JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue()->GetInt(), 3);
    std::vector<int> vals{2, 3, 4};
    CheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsArrayTest, Splice)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);
    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

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
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 4);

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    EXPECT_EQ(
        JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue()->GetInt(), 2);
    JSObject::GetOwnProperty(thread, valueHandle, keys[0], descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(2));
}

// 22.1.3.6 new Array(1,2,3,4,5).Fill(0,1,3)
HWTEST_F_L0(BuiltinsArrayTest, Fill)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3, 4, 5};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

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
    CheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);
}

// 22.1.3.6 new Array(1,2,3,4,5).Fill(0,1,3)
HWTEST_F_L0(BuiltinsArrayTest, COWArrayFill)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<TaggedArray> values(factory->NewTaggedArray(5));

    for (int i = 0; i < 5; i++) {
        values->Set(thread, i, JSTaggedValue(i + 1));
    }
    JSHandle<JSArray> array(JSArray::CreateArrayFromList(thread, values));
    JSHandle<JSArray> cloneArray = factory->CloneArrayLiteral(array);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(cloneArray.GetTaggedValue());
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
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(3));
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(4));

    std::vector<JSHandle<JSTaggedValue>> keys{key0, key1, key2, key3, key4};
    std::vector<int32_t> vals{1, 0, 0, 4, 5};
    CheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsArrayTest, Find)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> vals{1, 102, 3};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, vals);
    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFindFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::Find(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue(102).GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, FindIndex)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 30};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFindIndexFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::FindIndex(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, ForEach)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForEachFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::ForEach(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_EQ(result2.GetRawData(), JSTaggedValue::VALUE_UNDEFINED);
    EXPECT_EQ(jsArray->GetArrayLength(), 3U);
}

#define ARRAY_DEFINE_OWN_PROPERTY(dest, index, value)                                                               \
    do {                                                                                                            \
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(index));                                                  \
        PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(value)), true, true, true);   \
        JSArray::DefineOwnProperty(thread, dest, key, desc);                                                        \
    } while (false)

#define ARRAY_BUILTIN_METHOD_TEST_CASE_ARG0(method, target, expected)                                               \
    do {                                                                                                            \
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);    \
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());                                               \
        ecmaRuntimeCallInfo->SetThis((target).GetTaggedValue());                                                    \
                                                                                                                    \
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);                           \
        JSTaggedValue result = Array::method(ecmaRuntimeCallInfo);                                                  \
        TestHelper::TearDownFrame(thread, prev);                                                                    \
        ASSERT_TRUE(JSTaggedValue::StrictEqual(result, JSTaggedValue(expected)));                                   \
    } while (false)

#define ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(method, target, expected, arg0)                                         \
    do {                                                                                                            \
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);    \
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());                                               \
        ecmaRuntimeCallInfo->SetThis((target).GetTaggedValue());                                                    \
        ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(arg0));                                                    \
                                                                                                                    \
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);                           \
        JSTaggedValue result = Array::method(ecmaRuntimeCallInfo);                                                  \
        TestHelper::TearDownFrame(thread, prev);                                                                    \
        ASSERT_TRUE(JSTaggedValue::StrictEqual(result, JSTaggedValue(expected)));                                   \
    } while (false)

#define ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(method, target, expected, arg0, arg1)                                   \
    do {                                                                                                            \
        auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);    \
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());                                               \
        ecmaRuntimeCallInfo->SetThis((target).GetTaggedValue());                                                    \
        ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(arg0));                                                    \
        ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(arg1));                                                    \
                                                                                                                    \
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);                           \
        JSTaggedValue result = Array::method(ecmaRuntimeCallInfo);                                                  \
        TestHelper::TearDownFrame(thread, prev);                                                                    \
        ASSERT_TRUE(JSTaggedValue::StrictEqual(result, JSTaggedValue(expected)));                                   \
    } while (false)

// 22.1.3.11 Array.IndexOf(searchElement [ , fromIndex ])
HWTEST_F_L0(BuiltinsArrayTest, IndexOf)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    // arr = [1, 2, 3, 4, 3, 0, 2.0, +0.0, 3.0, -0.0, <hole>, <hole>, undefined]
    ARRAY_DEFINE_OWN_PROPERTY(obj, 0, 1);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 1, 2);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 2, 3);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 3, 4);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 4, 3);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 5, 0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 6, 2.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 7, +0.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 8, 3.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 9, -0.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 12, JSTaggedValue::Undefined());

    // arr.indexOf(3, 0) == 2
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, 2, 3, 0);
    // arr.indexOf(3, 3) == 4
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, 4, 3, 3);
    // arr.indexOf(5, 0) == -1
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, -1, 5, 0);
    // arr.indexOf(3) == 2
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(IndexOf, obj, 2, 3);

    // Expects int32_t(x) and double(x) to be strictly equal
    // arr.indexOf(3.0) == 2
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(IndexOf, obj, 2, 3.0);
    // arr.indexOf(3, 5) == 8
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, 8, 3, 5);

    // Expects 0, +0.0, -0.0 to be strictly equal
    // arr.indexOf(+0.0) == 5
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(IndexOf, obj, 5, +0.0);
    // arr.indexOf(-0.0) == 5
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(IndexOf, obj, 5, -0.0);
    // arr.indexOf(0, 6) == 7
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, 7, 0, 6);
    // arr.indexOf(-0.0, 6) == 7
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, 7, -0.0, 6);
    // arr.indexOf(0, 8) == 9
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, 9, 0, 8);
    // arr.indexOf(+0.0, 8) == 9
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(IndexOf, obj, 9, +0.0, 8);

    // Expects undefined to be found
    // arr.indexOf() == 12, where the first argument is undefined
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG0(IndexOf, obj, 12);
}

// 22.1.3.14 Array.LastIndexOf(searchElement [ , fromIndex ])
HWTEST_F_L0(BuiltinsArrayTest, LastIndexOf)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    // arr = [1, 2, 3, 4, 3, 0, 2.0, +0.0, 3.0, -0.0, <hole>, <hole>, undefined, <hole>, <hole>, -1]
    ARRAY_DEFINE_OWN_PROPERTY(obj, 0, 1);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 1, 2);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 2, 3);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 3, 4);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 4, 3);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 5, 0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 6, 2.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 7, +0.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 8, 3.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 9, -0.0);
    ARRAY_DEFINE_OWN_PROPERTY(obj, 12, JSTaggedValue::Undefined());
    ARRAY_DEFINE_OWN_PROPERTY(obj, 15, -1);

    // arr.lastIndexOf(3, 4) == 4
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, 4, 3, 4);
    // arr.lastIndexOf(3, 3) == 2
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, 2, 3, 3);
    // arr.lastIndexOf(5, 4) == -1
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, -1, 5, 4);

    // Expects int32_t(x) and double(x) to be strictly equal
    // arr.lastIndexOf(3) == 8
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(LastIndexOf, obj, 8, 3);
    // arr.lastIndexOf(1.0) == 0
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(LastIndexOf, obj, 0, 1.0);

    // Expects 0, +0.0, -0.0 to be strictly equal
    // arr.indexOf(+0.0) == 9
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(LastIndexOf, obj, 9, +0.0);
    // arr.indexOf(0) == 9
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG1(LastIndexOf, obj, 9, 0);
    // arr.indexOf(0, 8) == 7
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, 7, 0, 8);
    // arr.indexOf(-0.0, 8) == 7
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, 7, -0.0, 8);
    // arr.indexOf(-0.0, 6) == 5
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, 5, -0.0, 6);
    // arr.indexOf(+0.0, 6) == 5
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG2(LastIndexOf, obj, 5, +0.0, 6);

    // Expects undefined to be found
    // arr.indexOf() == 12, where the first argument is undefined
    ARRAY_BUILTIN_METHOD_TEST_CASE_ARG0(LastIndexOf, obj, 12);
}

// 22.1.3.11 new Array().Pop()
HWTEST_F_L0(BuiltinsArrayTest, Pop)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

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
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(obj.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    result = Array::Pop(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(3).GetRawData());
}

// 22.1.3.11 new Array().Pop()
HWTEST_F_L0(BuiltinsArrayTest, COWArrayPop)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<TaggedArray> values(factory->NewTaggedArray(3));

    for (int i = 0; i < 3; i++) {
        values->Set(thread, i, JSTaggedValue(i + 1));
    }
    JSHandle<JSArray> array(JSArray::CreateArrayFromList(thread, values));
    JSHandle<JSArray> cloneArray = factory->CloneArrayLiteral(array);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(cloneArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    auto result = Array::Pop(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(3).GetRawData());
}

// 22.1.3.11 new Array(1,2,3).Push(...items)
HWTEST_F_L0(BuiltinsArrayTest, Push)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key2, desc2);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(4)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Push(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetNumber(), 5);

    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 5);
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(3));
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key3).GetValue()->GetInt(), 4);
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(4));
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key4).GetValue()->GetInt(), 5);
}

HWTEST_F_L0(BuiltinsArrayTest, Reduce)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestReduceFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(10)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Reduce(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(16).GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, ReduceRight)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestReduceRightFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(10)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::ReduceRight(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(16).GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, Shift)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Shift(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(1).GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, COWArrayShift)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<TaggedArray> values(factory->NewTaggedArray(3));

    for (int i = 0; i < 3; i++) {
        values->Set(thread, i, JSTaggedValue(i + 1));
    }
    JSHandle<JSArray> array(JSArray::CreateArrayFromList(thread, values));
    JSHandle<JSArray> cloneArray = factory->CloneArrayLiteral(array);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(cloneArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Shift(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(1).GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, Some)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 20, 3};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestSomeFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::Some(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue::True().GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, Sort)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{3, 2, 1};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::Sort(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result2.IsECMAObject());
    JSHandle<JSTaggedValue> resultArr =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result2.GetRawData())));
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, keys[0]).GetValue()->GetInt(), 1);
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, keys[1]).GetValue()->GetInt(), 2);
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, keys[2]).GetValue()->GetInt(), 3);
}

HWTEST_F_L0(BuiltinsArrayTest, Unshift)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 2, 3};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(4)));
    ecmaRuntimeCallInfo1->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Unshift(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(5)).GetRawData());

    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 5);
    JSHandle<JSTaggedValue> key3(thread, JSTaggedValue(0));
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key3).GetValue()->GetInt(), 4);
    JSHandle<JSTaggedValue> key4(thread, JSTaggedValue(1));
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), key4).GetValue()->GetInt(), 5);
}

HWTEST_F_L0(BuiltinsArrayTest, Join)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, obj, lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{2, 3, 4};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

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

HWTEST_F_L0(BuiltinsArrayTest, ToString)
{
    std::vector<int> descVals{2, 3, 4};
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, obj, lengthKeyHandle).GetValue()->GetInt(), 0);
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

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

HWTEST_F_L0(BuiltinsArrayTest, Includes)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSTaggedValue> obj(thread, arr);
    std::vector<int> descVals{2, 3, 4};
    EXPECT_EQ(JSArray::GetProperty(thread, obj, lengthKeyHandle).GetValue()->GetInt(), 0);
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

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

    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo4->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(2)));
    ecmaRuntimeCallInfo4->SetCallArg(1, JSTaggedValue(static_cast<int32_t>(5)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4);
    result = Array::Includes(ecmaRuntimeCallInfo4);
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

// es12 23.1.3.10 new Array(1,[2,3]).flat()
HWTEST_F_L0(BuiltinsArrayTest, Flat)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr1 = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr1 != nullptr);
    JSHandle<JSObject> obj1(thread, arr1);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj1), lengthKeyHandle).GetValue()->GetInt(), 0);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj1, key0, desc0);

    JSArray *arr2 = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr2 != nullptr);
    JSHandle<JSObject> obj2(thread, arr2);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj2), lengthKeyHandle).GetValue()->GetInt(), 0);

    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj2, key0, desc1);

    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc2(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(3)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj2, key1, desc2);

    PropertyDescriptor desc3(thread, JSHandle<JSTaggedValue>(thread, obj2.GetTaggedValue()), true, true, true);
    JSArray::DefineOwnProperty(thread, obj1, key1, desc3);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj1.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::Flat(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    JSObject::GetOwnProperty(thread, valueHandle, key0, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(1));
    JSObject::GetOwnProperty(thread, valueHandle, key1, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(2));
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSObject::GetOwnProperty(thread, valueHandle, key2, descRes);
    ASSERT_EQ(descRes.GetValue().GetTaggedValue(), JSTaggedValue(3));
}

// es12 23.1.3.10 new Array(1,50,3]).flatMap(x => [x * 2])
HWTEST_F_L0(BuiltinsArrayTest, FlatMap)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    std::vector<int> descVals{1, 50, 3};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFlatMapFunc));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::FlatMap(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    EXPECT_EQ(
        JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue()->GetInt(), 3);
    std::vector<int> vals{2, 100, 6};
    CheckKeyValueCommon(thread, valueHandle, descRes, keys, vals);
}

HWTEST_F_L0(BuiltinsArrayTest, At)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    std::vector<int> descVals{2, 3, 4};
    JSHandle<JSTaggedValue> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, obj, lengthKeyHandle).GetValue()->GetInt(), 0);
    
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

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

    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo4->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetCallArg(0, JSTaggedValue(static_cast<int32_t>(-1)));
    [[maybe_unused]] auto prev4 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4);
    result = Array::At(ecmaRuntimeCallInfo4);
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

HWTEST_F_L0(BuiltinsArrayTest, With)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj),
                                   lengthKeyHandle).GetValue()->GetInt(), INT_VALUE_0);

    std::vector<int> descVals{0, 1, 2};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), RUNTIME_CALL_INFO_PARA_NUM_8);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(RUNTIME_CALL_INFO_PARA_0,
        JSTaggedValue(static_cast<uint32_t>((ArrayIndex::ARRAY_INDEX_1))));
    ecmaRuntimeCallInfo1->SetCallArg(RUNTIME_CALL_INFO_PARA_1, JSTaggedValue(INT_VALUE_3));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::With(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsECMAObject());
    JSHandle<JSTaggedValue> resultArr =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result.GetRawData())));
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, keys[0]).GetValue()->GetInt(), INT_VALUE_0);
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, keys[1]).GetValue()->GetInt(), INT_VALUE_3);
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, keys[2]).GetValue()->GetInt(), INT_VALUE_2);
}

HWTEST_F_L0(BuiltinsArrayTest, ToSorted)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj),
                                   lengthKeyHandle).GetValue()->GetInt(), INT_VALUE_0);
    std::vector<int> descVals{3, 2, 1};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), RUNTIME_CALL_INFO_PARA_NUM_4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::ToSorted(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result2.IsECMAObject());
    JSHandle<JSTaggedValue> resultArr =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result2.GetRawData())));
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, keys[0]).GetValue()->GetInt(), INT_VALUE_1);
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, keys[1]).GetValue()->GetInt(), INT_VALUE_2);
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, keys[2]).GetValue()->GetInt(), INT_VALUE_3);
}

HWTEST_F_L0(BuiltinsArrayTest, ToSpliced)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(INT_VALUE_0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj),
                                   lengthKeyHandle).GetValue()->GetInt(), INT_VALUE_0);
    std::vector<int> descVals{0, 1, 2};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), RUNTIME_CALL_INFO_PARA_NUM_10);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(INT_VALUE_0, JSTaggedValue(INT_VALUE_1));
    ecmaRuntimeCallInfo1->SetCallArg(INT_VALUE_1, JSTaggedValue(INT_VALUE_1));
    ecmaRuntimeCallInfo1->SetCallArg(INT_VALUE_2, JSTaggedValue(INT_VALUE_666));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result2 = Array::ToSpliced(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result2.IsECMAObject());
    JSHandle<JSTaggedValue> resultArr =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result2.GetRawData())));
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, keys[0]).GetValue()->GetInt(), INT_VALUE_0);
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, keys[1]).GetValue()->GetInt(), INT_VALUE_666);
    EXPECT_EQ(JSArray::GetProperty(thread, resultArr, keys[2]).GetValue()->GetInt(), INT_VALUE_2);
}

HWTEST_F_L0(BuiltinsArrayTest, FindLast)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);
    // arr [50, 40, 2]
    std::vector<int> descVals{50, 40, 2};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFindLastFunc));

    auto ecmaRuntimeCallInfo1 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 8 means 2 call args
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::FindLast(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result.GetRawData(), JSTaggedValue(40).GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, FindLastIndex)
{
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj), lengthKeyHandle).GetValue()->GetInt(), 0);

    // arr [50, 40, 30]
    std::vector<int> descVals{50, 40, 30};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    JSHandle<JSArray> jsArray(JSArray::ArrayCreate(thread, JSTaggedNumber(0)));
    JSHandle<JSFunction> func = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestFindLastIndexFunc));

    auto ecmaRuntimeCallInfo1 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 8 means 2 call args
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(1, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::FindLastIndex(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());
}

HWTEST_F_L0(BuiltinsArrayTest, ToReversed)
{
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj),
                                   lengthKeyHandle).GetValue()->GetInt(), INT_VALUE_0);
    
    std::vector<int> descVals{50, 200, 3};
    auto keys = DefineOwnPropertyCommonTest(thread, obj, descVals);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), INT_VALUE_4);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result = Array::ToReversed(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());

    PropertyDescriptor descRes(thread);
    JSHandle<JSObject> valueHandle(thread, value);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle),
                                   lengthKeyHandle).GetValue()->GetInt(), INT_VALUE_3);

    std::vector<int> vals2{3, 200, 50};
    CheckKeyValueCommon(thread, valueHandle, descRes, keys, vals2);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(obj),
                                   lengthKeyHandle).GetValue()->GetInt(), INT_VALUE_3);

    std::vector<int> vals{50, 200, 3};
    CheckKeyValueCommon(thread, obj, descRes, keys, vals);
}
}  // namespace panda::test
