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

#include "ecmascript/base/json_helper.h"
#include "ecmascript/builtins/builtins_json.h"

#include "gtest/gtest.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

#include "ecmascript/base/builtins_base.h"
#include "ecmascript/builtins/builtins_bigint.h"
#include "ecmascript/builtins/builtins_errors.h"
#include "ecmascript/builtins/builtins_proxy.h"
#include "ecmascript/builtins/builtins_typedarray.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_tagged_value_wrapper-inl.h"
#include "ecmascript/js_tagged_value_wrapper.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/shared_objects/js_shared_array.h"
#include "ecmascript/shared_objects/js_shared_map.h"
#include "ecmascript/shared_objects/js_shared_object.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsJsonTest : public BaseTestWithScope<false> {
public:
    class TestClass : public base::BuiltinsBase {
    public:
        static JSTaggedValue TestForCommon(EcmaRuntimeCallInfo *argv)
        {
            JSTaggedValue key = GetCallArg(argv, 0).GetTaggedValue();
            if (key.IsUndefined()) {
                return JSTaggedValue::Undefined();
            }
            JSTaggedValue value = GetCallArg(argv, 1).GetTaggedValue();
            if (value.IsUndefined()) {
                return JSTaggedValue::Undefined();
            }

            return JSTaggedValue(value);
        }

        static JSTaggedValue TestForParse(EcmaRuntimeCallInfo *argv)
        {
            return TestForCommon(argv);
        }

        static JSTaggedValue TestForParse1(EcmaRuntimeCallInfo *argv)
        {
            (void)argv;
            return JSTaggedValue::Undefined();
        }

        static JSTaggedValue TestForStringfy(EcmaRuntimeCallInfo *argv)
        {
            uint32_t argc = argv->GetArgsNumber();
            if (argc > 0) {
                return TestForCommon(argv);
            }

            return JSTaggedValue::Undefined();
        }
    };
};

JSTaggedValue CreateBuiltinJSObject1(JSThread *thread, const CString keyCStr)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> objectFunc(globalEnv->GetObjectFunction());

    JSHandle<JSObject> jsobject(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objectFunc), objectFunc));
    EXPECT_TRUE(*jsobject != nullptr);

    JSHandle<JSTaggedValue> key(factory->NewFromASCII(&keyCStr[0]));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsobject), key, value);

    CString str2 = "y";
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII(str2));
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue(2.5)); // 2.5 : test case
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsobject), key2, value2);

    CString str3 = "z";
    JSHandle<JSTaggedValue> key3(factory->NewFromASCII(str3));
    JSHandle<JSTaggedValue> value3(factory->NewFromASCII("abc"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(jsobject), key3, value3);

    return jsobject.GetTaggedValue();
}

JSHandle<JSObject> GetJsonBuiltinObject(JSThread *thread)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    return JSHandle<JSObject>::Cast(env->GetJsonFunction());
}

JSTaggedValue CallParseV2(JSThread *thread, const std::vector<JSTaggedValue> &args)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(
        thread, const_cast<std::vector<JSTaggedValue> &>(args), 10);  // 10: (fn+this+3 args)*2
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSendableJson::ParseSendable(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

JSTaggedValue CallParseV2WithArgc(JSThread *thread, const std::vector<JSTaggedValue> &args, uint32_t argc)
{
    int32_t maxArgLen = 2 * (static_cast<int32_t>(argc) + 2);  // 2: function and this
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(
        thread, const_cast<std::vector<JSTaggedValue> &>(args), maxArgLen);
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSendableJson::ParseSendable(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

JSTaggedValue CallLegacySendableParse(JSThread *thread, const std::vector<JSTaggedValue> &args)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(
        thread, const_cast<std::vector<JSTaggedValue> &>(args), 10);  // 10: (fn+this+3 args)*2
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSendableJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

JSTaggedValue CallParseBigInt(JSThread *thread, const std::vector<JSTaggedValue> &args)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(
        thread, const_cast<std::vector<JSTaggedValue> &>(args), 10);  // 10: (fn+this+3 args)*2
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsBigIntJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

void ClearExceptionIfAny(JSThread *thread, bool &hadException)
{
    hadException = false;
    if (thread->HasPendingException()) {
        hadException = true;
        thread->ClearException();
    }
}

JSHandle<JSObject> CreateMapModeOptions(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> objFunc(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());
    JSHandle<JSObject> options = factory->NewJSObjectByConstructor(objFunc);
    JSHandle<JSTaggedValue> typeKey(factory->NewFromASCII("parseReturnType"));
    JSHandle<JSTaggedValue> typeValue(thread, JSTaggedValue(1));  // 1: MAP
    JSObject::CreateDataProperty(thread, options, typeKey, typeValue);
    return options;
}

JSHandle<JSObject> CreateBigIntModeOptions(JSThread *thread, int mode)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSFunction> objFunc(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());
    JSHandle<JSObject> options = factory->NewJSObjectByConstructor(objFunc);
    JSHandle<JSTaggedValue> modeKey(factory->NewFromASCII("bigIntMode"));
    JSHandle<JSTaggedValue> modeValue(thread, JSTaggedValue(mode));
    JSObject::CreateDataProperty(thread, options, modeKey, modeValue);
    return options;
}

void ExpectParseV2SyntaxError(JSThread *thread, const std::vector<JSTaggedValue> &args, const char *expectedMsg)
{
    JSTaggedValue result = CallParseV2(thread, args);
    ASSERT_TRUE(result.IsException());
    ASSERT_TRUE(thread->HasPendingException());
    JSHandle<JSTaggedValue> exception(thread, thread->GetException());
    thread->ClearException();
    ASSERT_TRUE(exception->IsJSError());
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> nameKey(factory->NewFromASCII("name"));
    JSHandle<JSTaggedValue> msgKey(factory->NewFromASCII("message"));
    JSHandle<JSTaggedValue> name = JSObject::GetProperty(thread, exception, nameKey).GetValue();
    JSHandle<JSTaggedValue> msg = JSObject::GetProperty(thread, exception, msgKey).GetValue();
    ASSERT_TRUE(name->IsString());
    ASSERT_TRUE(msg->IsString());
    EXPECT_STREQ("SyntaxError", EcmaStringAccessor(JSHandle<EcmaString>(name)).ToCString(thread).c_str());
    CString message = EcmaStringAccessor(JSHandle<EcmaString>(msg)).ToCString(thread);
    EXPECT_NE(message.find(expectedMsg), CString::npos) << "message: " << message.c_str();
}
// Math.abs(-10)

HWTEST_F_L0(BuiltinsJsonTest, Parse10)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> msg(factory->NewFromASCII(
        "\t\r \n{\t\r \n \"property\"\t\r \n:\t\r \n{\t\r \n}\t\r \n,\t\r \n \"prop2\"\t\r \n:\t\r \n [\t\r \ntrue\t\r "
        "\n,\t\r \nnull\t\r \n,123.456\t\r \n] \t\r \n}\t\r \n"));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsJsonTest, Parse21)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> msg(factory->NewFromASCII("[100,2.5,\"abc\"]"));

    JSHandle<JSFunction> handleFunc = factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForParse));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, handleFunc.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsECMAObject());
}

HWTEST_F_L0(BuiltinsJsonTest, Parse)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> lengthKeyHandle = thread->GlobalConstants()->GetHandledLengthString();

    JSHandle<JSTaggedValue> msg(factory->NewFromASCII("[100,2.5,\"abc\"]"));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));
    std::vector<JSTaggedValue> args{str.GetTaggedValue()};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, args, 6);

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    JSHandle<JSObject> valueHandle(thread, value);
    JSHandle<JSTaggedValue> lenResult =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKeyHandle).GetValue();
    uint32_t length = JSTaggedValue::ToLength(thread, lenResult).ToUint32();
    EXPECT_EQ(length, 3U);
}

HWTEST_F_L0(BuiltinsJsonTest, Parse2)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> msg(factory->NewFromASCII("{\"epf\":100,\"key1\":200}"));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));

    std::vector<JSTaggedValue> args{str.GetTaggedValue()};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, args, 6);

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    JSHandle<JSObject> valueHandle(thread, value);

    JSHandle<TaggedArray> nameList(JSObject::EnumerableOwnNames(thread, valueHandle));
    JSHandle<JSArray> nameResult = JSArray::CreateArrayFromList(thread, nameList);

    JSHandle<JSTaggedValue> handleKey(nameResult);
    JSHandle<JSTaggedValue> lengthKey(factory->NewFromASCII("length"));
    JSHandle<JSTaggedValue> lenResult = JSObject::GetProperty(thread, handleKey, lengthKey).GetValue();
    uint32_t length = JSTaggedValue::ToLength(thread, lenResult).ToUint32();
    ASSERT_EQ(length, 2U);
}

HWTEST_F_L0(BuiltinsJsonTest, Parse3)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> str = factory->NewFromStdString("\"\\u0000\"");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    uint32_t length = EcmaStringAccessor(result).GetLength();
    ASSERT_EQ(length, 1U);
}

HWTEST_F_L0(BuiltinsJsonTest, Parse4)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> str = factory->NewFromStdString("{\n\t\"on\":\t0\n}");
    JSHandle<EcmaString> key = factory->NewFromStdString("on");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> value =
        JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>(thread, result), JSHandle<JSTaggedValue>(key))
            .GetValue();
    int32_t number = JSTaggedValue::ToInt32(thread, value);
    ASSERT_EQ(number, 0);
}


HWTEST_F_L0(BuiltinsJsonTest, Stringify11)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateBuiltinJSObject1(thread, "x"));
    JSHandle<JSFunction> handleFunc =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForStringfy));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, handleFunc.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify12)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateBuiltinJSObject1(thread, "x"));
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> handleFunc =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForStringfy));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, handleFunc.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, JSTaggedValue(static_cast<int32_t>(10)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify13)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateBuiltinJSObject1(thread, "x"));
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> handleFunc =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForStringfy));
    JSHandle<JSTaggedValue> msg(factory->NewFromASCII("tttt"));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));

    std::vector<JSTaggedValue> args{obj.GetTaggedValue(), handleFunc.GetTaggedValue(), str.GetTaggedValue()};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, args, 10);

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify14)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateBuiltinJSObject1(thread, "x"));
    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());

    JSHandle<JSObject> obj1(thread, arr);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> value0(factory->NewFromASCII("x"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key0, value0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("z"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key1, value1);

    JSHandle<JSTaggedValue> msg(factory->NewFromASCII("tttt"));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 10);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, obj1.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(2, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify)
{
    JSHandle<JSTaggedValue> obj = JSHandle<JSTaggedValue>(thread, CreateBuiltinJSObject1(thread, "x"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify1)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());

    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);
    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));

    JSHandle<JSTaggedValue> value(factory->NewFromASCII("def"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key0, value);

    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(200)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);

    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("abc"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key2, value2);

    JSHandle<JSFunction> handleFunc =
        factory->NewJSFunction(env, reinterpret_cast<void *>(TestClass::TestForStringfy));
    JSHandle<JSTaggedValue> msg(factory->NewFromASCII("tttt"));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));

    std::vector<JSTaggedValue> args{obj.GetTaggedValue(), handleFunc.GetTaggedValue(), str.GetTaggedValue()};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, args, 10);

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify2)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSArray *arr = JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> obj(thread, arr);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    PropertyDescriptor desc0(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(1)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    // 2.5 : test case
    PropertyDescriptor desc1(thread, JSHandle<JSTaggedValue>(thread, JSTaggedValue(2.5)), true, true, true);
    JSArray::DefineOwnProperty(thread, obj, key1, desc1);
    // 2 : test case
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value2(factory->NewFromASCII("abc"));
    JSObject::SetProperty(thread, JSHandle<JSTaggedValue>(obj), key2, value2);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify3)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    uint16_t data[1];
    data[0] = 0;
    JSHandle<EcmaString> str = factory->NewFromUtf16(data, 1);
    JSHandle<EcmaString> test = factory->NewFromStdString("\"\\u0000\"");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo);
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread, *test, EcmaString::Cast(result.GetTaggedObject())));
}

JSHandle<JSTaggedValue> CreateJSObject(JSThread *thread)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = globalEnv->GetObjectFunction();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> obj(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun));
    JSHandle<JSTaggedValue> key(factory->NewFromStdString("x"));
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1));
    JSObject::SetProperty(thread, obj, key, value);
    return obj;
}

JSHandle<JSTaggedValue> CreateProxy(JSThread *thread)
{
    JSHandle<JSTaggedValue> target = CreateJSObject(thread);
    JSHandle<JSTaggedValue> handler = CreateJSObject(thread);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Null(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, handler.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsProxy::ProxyConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return JSHandle<JSTaggedValue>(thread, result);
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify4)  // Test for proxy object
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> proxy = CreateProxy(thread);
    JSHandle<EcmaString> test = factory->NewFromStdString("{\"x\":1}");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, proxy.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo);
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread, *test, EcmaString::Cast(result.GetTaggedObject())));
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify5)  // Test for typedarray object
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();
    [[maybe_unused]] JSHandle<TaggedArray> array(factory->NewTaggedArray(3));
    array->Set(thread, 0, JSTaggedValue(2));
    array->Set(thread, 1, JSTaggedValue(3));
    array->Set(thread, 2, JSTaggedValue(4));

    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> jsArray(JSArray::CreateArrayFromList(thread, array));
    JSHandle<JSFunction> int8Func(env->GetInt8ArrayFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetNewTarget(JSTaggedValue(*int8Func));
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(*globalObject));
    ecmaRuntimeCallInfo1->SetCallArg(0, jsArray.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSHandle<JSTaggedValue> int8Array(thread, BuiltinsTypedArray::Int8ArrayConstructor(ecmaRuntimeCallInfo1));
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> test = factory->NewFromStdString("{\"0\":2,\"1\":3,\"2\":4}");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, int8Array.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result.IsString());
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread, *test, EcmaString::Cast(result.GetTaggedObject())));
}

HWTEST_F_L0(BuiltinsJsonTest, Stringify6)  // Test for bigint object
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("123456789123456789"));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, numericValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = BuiltinsBigInt::BigIntConstructor(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSTaggedValue> bigIntHandle(thread, result1);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, bigIntHandle.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    [[maybe_unused]] JSTaggedValue result = BuiltinsJson::Stringify(ecmaRuntimeCallInfo);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
}

HWTEST_F_L0(BuiltinsJsonTest, StringifyAndParse)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<JSTaggedValue> obj = CreateJSObject(thread);
    JSHandle<JSTaggedValue> ykey(factory->NewFromASCII("y"));
    JSHandle<JSTaggedValue> yvalue(thread, JSTaggedValue(2.2)); // 2.2: use to test double value
    JSObject::SetProperty(thread, obj, ykey, yvalue);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, obj.GetTaggedValue());
    JSMutableHandle<JSTaggedValue> result(thread, JSTaggedValue::Hole());
    {
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        result.Update(BuiltinsJson::Stringify(ecmaRuntimeCallInfo));
        TestHelper::TearDownFrame(thread, prev);
    }
    {
        ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
        ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo->SetCallArg(0, result.GetTaggedValue());
        [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
        result.Update(BuiltinsJson::Parse(ecmaRuntimeCallInfo));
        TestHelper::TearDownFrame(thread, prev);
    }
    ASSERT_TRUE(result->IsECMAObject());

    JSHandle<JSObject> resultObj(result);
    JSHandle<JSTaggedValue> key(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> res = JSObject::GetProperty(thread, resultObj, key).GetValue();
    ASSERT_TRUE(res->IsInt());
    ASSERT_EQ(res->GetInt(), 1);

    res = JSObject::GetProperty(thread, resultObj, ykey).GetValue();
    ASSERT_TRUE(res->IsDouble());
    ASSERT_EQ(res->GetDouble(), 2.2); // 2.2:use to test double value
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorMessageUtf16)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(
        factory->NewFromUtf8("{\"姓名\": \"小明\",\n \"age\": 30,\"表情\":\"aaaaaaaaaa😄🙃😇😄🙃😇😄🙃😇aaa\""));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    std::string str1 = "a*a*🙃*😄*😇*🙃*a*a\"";
    ASSERT_EQ(str1.length(), res.first.length());
    ASSERT_EQ(str1, res.first);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorMessageUtf8)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromUtf8(
        "{\"namxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxe\":\"tom\","
        "sex:\"F\",age:18,email:\"123@qq.com\"}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    std::string str1 = "x*x*x*e\":\"t*m\",s*x:\"F\",a*e:1*,";
    ASSERT_EQ(str1.length(), res.first.length());
    ASSERT_EQ(str1, res.first);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorMissingComma)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\":\"tom\"\"age\":18}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"n*m*\":\"t*m\"\"a*e\":1*}");
    ASSERT_EQ(res.second, 13U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorUnclosedObject)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\":\"tom\",\"age\":18"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "\":\"t*m\",\"a*e\":1*");
    ASSERT_EQ(res.second, 15U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorUnclosedArray)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("[1,2,3"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "[1,2,3");
    ASSERT_EQ(res.second, 5U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorTrailingCommaInObject)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\":\"tom\",\"age\":18,}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "\"t*m\",\"a*e\":1*,}");
    ASSERT_EQ(res.second, 15U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorTrailingCommaInArray)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("[1,2,3,]"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "[1,2,3,]");
    ASSERT_EQ(res.second, 7U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorMissingColon)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\"\"tom\"}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"n*m*\"\"t*m\"}");
    ASSERT_EQ(res.second, 7U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorInvalidStringEscape)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\":\"tom\\x\"}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"n*m*\":\"t*m*x\"}");
    ASSERT_EQ(res.second, 13U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorUnclosedString)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\":\"tom"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"n*m*\":\"t*m");
    ASSERT_EQ(res.second, 11U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorInvalidNumber)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"age\":01}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"a*e\":0*}");
    ASSERT_EQ(res.second, 8U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorInvalidLiteral)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"active\":tru}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"a*t*v*\":t*u}");
    ASSERT_EQ(res.second, 10U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorUnquotedKey)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{name:\"tom\"}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{n*m*:\"t*m\"}");
    ASSERT_EQ(res.second, 1U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorUnexpectedEnd)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\":"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"n*m*\":");
    ASSERT_EQ(res.second, 8U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorControlCharInString)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\":\"to\x01m\"}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"n*m*\":\"t*\x01*\"}");
    ASSERT_EQ(res.second, 11U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorNewlineInString)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\":\"tom\n\"}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"n*m*\":\"t*m\n\"}");
    ASSERT_EQ(res.second, 12U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorTabInString)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\":\"to\tm\"}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"n*m*\":\"t*\tm\"}");
    ASSERT_EQ(res.second, 11U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorNullByteInString)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    std::string data = "{\"name\":\"tom";
    data += '\x00';
    data += "\"}";
    JSHandle<EcmaString> str = factory->NewFromStdString(data);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"n*m*\":\"t*m*\"}");
    ASSERT_EQ(res.second, 12U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorMissingOpeningQuote)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{name:\"tom\"}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{n*m*:\"t*m\"}");
    ASSERT_EQ(res.second, 1U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorMissingClosingQuote)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name:\"tom\"}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"n*m*:\"t*m\"}");
    ASSERT_EQ(res.second, 8U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorSingleQuotes)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{'name':'tom'}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{'*a*e*:'*o*'}");
    ASSERT_EQ(res.second, 1U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorInvalidUnicodeEscape)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\":\"\\u00\"}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"n*m*\":\"\\*0*\"}");
    ASSERT_EQ(res.second, 10U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorInvalidHexInUnicode)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\":\"\\u00GG\"}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"n*m*\":\"\\*0*G*\"}");
    ASSERT_EQ(res.second, 13U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorEmptyString)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<EcmaString> str = factory->NewFromStdString("");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "");
    ASSERT_EQ(res.second, 0U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorWhitespaceOnly)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("   \t\n\r  "));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "   \t\n\r  ");
    ASSERT_EQ(res.second, 8U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorNegativeNumberWithLeadingZero)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"age\": 01}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"a*e\": 0*}");
    ASSERT_EQ(res.second, 9U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorMultipleDecimalPoints)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"v\": 1..}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"v\": 1*.}");
    ASSERT_EQ(res.second, 8U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorIncompleteScientificNotation)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"v\": 1e}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"v\": 1*}");
    ASSERT_EQ(res.second, 7U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorScientificNotationMissingExponent)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"v\": 1e+}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"v\": 1*+}");
    ASSERT_EQ(res.second, 8U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorNegativeSignWithoutNumber)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"v\": -}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"v\": -}");
    ASSERT_EQ(res.second, 6U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorCommentNotAllowed)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\": \"tom\" // comment}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"n*m*\": \"t*m\" /* c*m*e*t}");
    ASSERT_EQ(res.second, 15U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorNestedObjectError)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"outer\": {\"inner\": {\"key\": \"unclosed}}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "e*\": \"u*c*o*e*}}");
    ASSERT_EQ(res.second, 15U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorString)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("\"abcdefghijk"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "\"a*c*e*g*i*k");
    ASSERT_EQ(res.second, 11U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorNestedArrayError)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("[[1, 2, [3, 4, \"err]], 5]"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "3, 4, \"e*r]], 5]");
    ASSERT_EQ(res.second, 15U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorAtBeginning)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("@{\"name\": \"tom\"}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "@{\"n*m*\": \"t*m\"");
    ASSERT_EQ(res.second, 0U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorAtEnd)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\": \"tom\""));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{\"n*m*\": \"t*m\"");
    ASSERT_EQ(res.second, 13U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorSingleOpenBrace)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{");
    ASSERT_EQ(res.second, 1U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorSingleOpenBracket)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("["));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "[");
    ASSERT_EQ(res.second, 1U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorMultipleErrorsFirstPosition)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{'name': 'tom'}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "{'*a*e*: '*o*'}");
    ASSERT_EQ(res.second, 1U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorDuplicateKey)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    JSHandle<JSTaggedValue> str(factory->NewFromASCII("{\"name\": \"tom\", \"name\": \"jerry\"}"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_FALSE(hasPendingException);

    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_EQ(position, 0U);

    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    if (extraErrorMessage->IsString()) {
        ASSERT_EQ(EcmaStringAccessor(extraErrorMessage.GetTaggedValue()).GetLength(), 0U);
    }
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorVeryLongString)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    std::string longJson = "{\"key\": \"";
    for (int i = 0; i < 500; i++) {
        longJson += "a";
    }
    longJson += "\"}";
    JSHandle<EcmaString> str = factory->NewFromStdString(longJson);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_FALSE(hasPendingException);

    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_EQ(position, 0U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorVeryLongStringWithError)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    std::string longJson = "{\"key\": \"";
    for (int i = 0; i < 100; i++) {
        longJson += "a";
    }
    longJson += "\n";
    for (int i = 0; i < 100; i++) {
        longJson += "a";
    }
    longJson += "\"}";
    JSHandle<EcmaString> str = factory->NewFromStdString(longJson);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "a*a*a*a*a*a*a*a\na*a*a*a*a*a*a*");
    ASSERT_EQ(res.second, 15U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseErrorWithError)
{
    auto ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();

    std::string longJson = "{\"key\": \"";
    for (int i = 0; i < 100; i++) {
        longJson += "a";
    }
    longJson += "\n";
    for (int i = 0; i < 100; i++) {
        longJson += "a";
    }
    longJson += "\"}";
    JSHandle<EcmaString> str = factory->NewFromStdString(longJson);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsJson::Parse(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    bool hasPendingException = false;
    if (thread->HasPendingException()) {
        hasPendingException = true;
        thread->ClearException();
    }
    ASSERT_TRUE(hasPendingException);
    JSHandle<JSTaggedValue> extraErrorMessage(thread, thread->GetExtraErrorMessage());
    uint32_t position = thread->GetJsonErrorPosition();
    ASSERT_TRUE(extraErrorMessage->IsString());
    std::pair<std::string, uint32_t> res =
        base::JsonHelper::AnonymizeJsonString(thread, extraErrorMessage, position, 15);
    ASSERT_EQ(res.first, "a*a*a*a*a*a*a*a\na*a*a*a*a*a*a*");
    ASSERT_EQ(res.second, 15U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_BridgeDescriptor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> jsonObj = GetJsonBuiltinObject(thread);

    // V2 bridge：non-enumerable/non-writable/non-configurable + length 3
    JSHandle<JSTaggedValue> v2Key(factory->NewFromASCII("parseSendableV2"));
    PropertyDescriptor descV2(thread);
    ASSERT_TRUE(JSObject::GetOwnProperty(thread, jsonObj, v2Key, descV2));
    EXPECT_TRUE(descV2.HasValue());
    EXPECT_TRUE(descV2.GetValue()->IsCallable());
    EXPECT_FALSE(descV2.IsWritable());
    EXPECT_FALSE(descV2.IsEnumerable());
    EXPECT_FALSE(descV2.IsConfigurable());
    JSHandle<JSTaggedValue> lenKey(factory->NewFromASCII("length"));
    JSHandle<JSTaggedValue> lenV2 =
        JSObject::GetProperty(thread, descV2.GetValue(), lenKey).GetValue();
    EXPECT_EQ(lenV2->GetInt(), 3);

    JSHandle<JSTaggedValue> legacyKey(factory->NewFromASCII("parseSendable"));
    PropertyDescriptor descLegacy(thread);
    ASSERT_TRUE(JSObject::GetOwnProperty(thread, jsonObj, legacyKey, descLegacy));
    EXPECT_TRUE(descLegacy.HasValue());
    EXPECT_TRUE(descLegacy.GetValue()->IsCallable());
    EXPECT_TRUE(descLegacy.IsWritable());
    EXPECT_FALSE(descLegacy.IsEnumerable());
    EXPECT_TRUE(descLegacy.IsConfigurable());
    JSHandle<JSTaggedValue> lenLegacy =
        JSObject::GetProperty(thread, descLegacy.GetValue(), lenKey).GetValue();
    EXPECT_EQ(lenLegacy->GetInt(), 3);

    JSHandle<JSTaggedValue> jsonHandle(jsonObj);
    JSHandle<TaggedArray> nameList(JSObject::EnumerableOwnNames(thread, jsonObj));
    bool foundV2 = false;
    for (uint32_t i = 0; i < nameList->GetLength(); i++) {
        JSHandle<JSTaggedValue> name(thread, nameList->Get(thread, i));
        if (EcmaStringAccessor(EcmaString::Cast(name->GetTaggedObject())).ToCString(thread) ==
            "parseSendableV2") {
            foundV2 = true;
        }
    }
    EXPECT_FALSE(foundV2);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_ReviverContract_BasicArity)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> text(factory->NewFromASCII("{}"));

    std::vector<JSTaggedValue> noArgs{};
    JSTaggedValue resultNone = CallParseV2(thread, noArgs);
    EXPECT_TRUE(resultNone.IsException());
    bool hadException = false;
    ClearExceptionIfAny(thread, hadException);
    EXPECT_TRUE(hadException);

    std::vector<JSTaggedValue> args1{text.GetTaggedValue()};
    JSTaggedValue resultOk = CallParseV2(thread, args1);
    EXPECT_TRUE(resultOk.IsJSSharedObject());
    std::vector<JSTaggedValue> args2{text.GetTaggedValue(), JSTaggedValue::Undefined()};
    JSTaggedValue resultOk2 = CallParseV2(thread, args2);
    EXPECT_TRUE(resultOk2.IsJSSharedObject());
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_MapReturnMode)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> optionsMap = CreateMapModeOptions(thread);

    JSHandle<JSTaggedValue> text(factory->NewFromASCII(R"({"a":1,"b":2})"));
    std::vector<JSTaggedValue> mapArgs{text.GetTaggedValue(), JSTaggedValue::Undefined(),
                                       optionsMap.GetTaggedValue()};
    JSTaggedValue mapResult = CallParseV2(thread, mapArgs);
    ASSERT_TRUE(mapResult.IsJSSharedMap());
    JSHandle<JSSharedMap> sharedMap(thread, JSSharedMap::Cast(mapResult));
    EXPECT_EQ(JSSharedMap::GetSize(thread, sharedMap), 2U);

    JSHandle<JSTaggedValue> nestedText(factory->NewFromASCII(R"({"outer":{"inner":1}})"));
    std::vector<JSTaggedValue> nestedMapArgs{nestedText.GetTaggedValue(), JSTaggedValue::Undefined(),
                                             optionsMap.GetTaggedValue()};
    JSTaggedValue nestedMapResult = CallParseV2(thread, nestedMapArgs);
    ASSERT_TRUE(nestedMapResult.IsJSSharedMap());
    JSHandle<JSSharedMap> outerMap(thread, JSSharedMap::Cast(nestedMapResult));
    EXPECT_EQ(JSSharedMap::GetSize(thread, outerMap), 1U);
    JSHandle<JSTaggedValue> outerKey(factory->NewFromASCII("outer"));
    JSTaggedValue innerValue = JSSharedMap::Get(thread, outerMap, outerKey.GetTaggedValue());
    ASSERT_TRUE(innerValue.IsJSSharedMap());
    JSHandle<JSSharedMap> innerMap(thread, JSSharedMap::Cast(innerValue));
    EXPECT_EQ(JSSharedMap::GetSize(thread, innerMap), 1U);

    JSHandle<JSTaggedValue> emptyText(factory->NewFromASCII("{}"));
    std::vector<JSTaggedValue> emptyMapArgs{emptyText.GetTaggedValue(), JSTaggedValue::Undefined(),
                                            optionsMap.GetTaggedValue()};
    JSTaggedValue emptyMapResult = CallParseV2(thread, emptyMapArgs);
    ASSERT_TRUE(emptyMapResult.IsJSSharedMap());
    JSHandle<JSSharedMap> emptyMap(thread, JSSharedMap::Cast(emptyMapResult));
    EXPECT_EQ(JSSharedMap::GetSize(thread, emptyMap), 0U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_BigIntModeOptions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> numText(factory->NewFromASCII(R"({"n":1})"));
    JSHandle<JSTaggedValue> nKey(factory->NewFromASCII("n"));

    JSHandle<JSObject> optionsAlways = CreateBigIntModeOptions(thread, 2);
    std::vector<JSTaggedValue> bigArgs{numText.GetTaggedValue(), JSTaggedValue::Undefined(),
                                       optionsAlways.GetTaggedValue()};
    JSTaggedValue bigResult = CallParseV2(thread, bigArgs);
    ASSERT_TRUE(bigResult.IsJSSharedObject());
    JSHandle<JSTaggedValue> nVal = JSObject::GetProperty(thread,
        JSHandle<JSTaggedValue>(thread, bigResult), nKey).GetValue();
    EXPECT_TRUE(nVal->IsBigInt());

    for (int mode : {1, 3}) {  // 1: PARSE_AS_BIGINT, 3: invalid mode
        JSHandle<JSObject> optionsMode = CreateBigIntModeOptions(thread, mode);
        std::vector<JSTaggedValue> modeArgs{numText.GetTaggedValue(), JSTaggedValue::Undefined(),
                                            optionsMode.GetTaggedValue()};
        JSTaggedValue modeResult = CallParseV2(thread, modeArgs);
        ASSERT_TRUE(modeResult.IsJSSharedObject());
        JSHandle<JSTaggedValue> modeNVal = JSObject::GetProperty(thread,
            JSHandle<JSTaggedValue>(thread, modeResult), nKey).GetValue();
        EXPECT_TRUE(modeNVal->IsInt());
        EXPECT_FALSE(modeNVal->IsBigInt());
    }

    JSHandle<JSFunction> objFunc(thread->GetEcmaVM()->GetGlobalEnv()->GetObjectFunction());
    JSHandle<JSObject> optionsStrMode = factory->NewJSObjectByConstructor(objFunc);
    JSHandle<JSTaggedValue> modeKey(factory->NewFromASCII("bigIntMode"));
    JSHandle<JSTaggedValue> strModeValue(factory->NewFromASCII("2"));
    JSObject::CreateDataProperty(thread, optionsStrMode, modeKey, strModeValue);
    std::vector<JSTaggedValue> strModeArgs{numText.GetTaggedValue(), JSTaggedValue::Undefined(),
                                           optionsStrMode.GetTaggedValue()};
    JSTaggedValue strModeResult = CallParseV2(thread, strModeArgs);
    ASSERT_TRUE(strModeResult.IsJSSharedObject());
    JSHandle<JSTaggedValue> strModeNVal = JSObject::GetProperty(thread,
        JSHandle<JSTaggedValue>(thread, strModeResult), nKey).GetValue();
    EXPECT_TRUE(strModeNVal->IsInt());
    EXPECT_FALSE(strModeNVal->IsBigInt());
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_Utf16Path)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    uint16_t utf16Text[] = {0x7B, 0x22, 0x6B, 0x4E2D, 0x22, 0x3A, 0x22, 0x76, 0x22, 0x7D};
    JSHandle<JSTaggedValue> msg(factory->NewFromUtf16(utf16Text, sizeof(utf16Text) / sizeof(uint16_t)));
    JSHandle<EcmaString> str(JSTaggedValue::ToString(thread, msg));
    ASSERT_FALSE(EcmaStringAccessor(str).IsUtf8());
    std::vector<JSTaggedValue> args{str.GetTaggedValue()};
    JSTaggedValue result = CallParseV2(thread, args);
    ASSERT_TRUE(result.IsJSSharedObject());
    JSHandle<JSTaggedValue> obj(thread, result);
    JSHandle<JSTaggedValue> key(factory->NewFromUtf16(utf16Text + 2, 2));
    JSHandle<JSTaggedValue> val = JSObject::GetProperty(thread, obj, key).GetValue();
    ASSERT_TRUE(val->IsString());
    JSHandle<EcmaString> valStr(val);
    EXPECT_STREQ("v", EcmaStringAccessor(valStr).ToCString(thread).c_str());
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_EmptyObject)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> emptyText(factory->NewFromASCII("{}"));
    std::vector<JSTaggedValue> emptyArgs{emptyText.GetTaggedValue()};
    JSTaggedValue emptyResult = CallParseV2(thread, emptyArgs);
    ASSERT_TRUE(emptyResult.IsJSSharedObject());
    JSHandle<JSHClass> emptyHclass(thread, JSObject::Cast(emptyResult)->GetJSHClass());
    EXPECT_EQ(emptyHclass->GetInlinedProperties(), 0U);
    EXPECT_FALSE(emptyHclass->IsDictionaryMode());
    EXPECT_TRUE(JSTaggedValue::SameValue(thread, emptyHclass->GetPrototype(thread),
        JSHandle<JSFunction>(env->GetSObjectFunction())->GetFunctionPrototype(thread)));

    JSHandle<JSTaggedValue> nestedEmptyText(factory->NewFromASCII(R"({"a":{},"b":[]})"));
    std::vector<JSTaggedValue> nestedEmptyArgs{nestedEmptyText.GetTaggedValue()};
    JSTaggedValue nestedEmptyResult = CallParseV2(thread, nestedEmptyArgs);
    ASSERT_TRUE(nestedEmptyResult.IsJSSharedObject());
    JSHandle<JSTaggedValue> nestedEmptyObj(thread, nestedEmptyResult);
    JSHandle<JSTaggedValue> aKey(factory->NewFromASCII("a"));
    JSHandle<JSTaggedValue> bKey(factory->NewFromASCII("b"));
    EXPECT_TRUE(JSObject::GetProperty(thread, nestedEmptyObj, aKey).GetValue()->IsJSSharedObject());
    EXPECT_TRUE(JSObject::GetProperty(thread, nestedEmptyObj, bKey).GetValue()->IsJSSharedArray());
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_InlineProperties)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> aKey(factory->NewFromASCII("a"));

    JSHandle<JSTaggedValue> inlineText(factory->NewFromASCII(R"({"x":1,"y":"str","z":true,"w":null})"));
    std::vector<JSTaggedValue> inlineArgs{inlineText.GetTaggedValue()};
    JSTaggedValue inlineResult = CallParseV2(thread, inlineArgs);
    ASSERT_TRUE(inlineResult.IsJSSharedObject());
    JSHandle<JSObject> inlineObj(thread, inlineResult);
    JSHandle<JSHClass> inlineHclass(thread, JSObject::Cast(inlineResult)->GetJSHClass());
    EXPECT_FALSE(inlineHclass->IsDictionaryMode());
    EXPECT_EQ(inlineHclass->GetInlinedProperties(), 4U);
    JSHandle<JSTaggedValue> xKey(factory->NewFromASCII("x"));
    JSHandle<JSTaggedValue> yKey(factory->NewFromASCII("y"));
    JSHandle<JSTaggedValue> zKey(factory->NewFromASCII("z"));
    JSHandle<JSTaggedValue> wKey(factory->NewFromASCII("w"));
    EXPECT_TRUE(JSObject::GetProperty(thread, inlineObj, xKey).GetValue()->IsInt());
    JSHandle<JSTaggedValue> yVal = JSObject::GetProperty(thread, inlineObj, yKey).GetValue();
    ASSERT_TRUE(yVal->IsString());
    EXPECT_STREQ("str", EcmaStringAccessor(JSHandle<EcmaString>(yVal)).ToCString(thread).c_str());
    EXPECT_TRUE(JSObject::GetProperty(thread, inlineObj, zKey).GetValue()->IsTrue());
    EXPECT_TRUE(JSObject::GetProperty(thread, inlineObj, wKey).GetValue()->IsNull());

    JSHandle<JSTaggedValue> dupText(factory->NewFromASCII(R"({"a":1,"a":2})"));
    std::vector<JSTaggedValue> dupArgs{dupText.GetTaggedValue()};
    JSTaggedValue dupResult = CallParseV2(thread, dupArgs);
    ASSERT_TRUE(dupResult.IsJSSharedObject());
    JSHandle<JSTaggedValue> dupObj(thread, dupResult);
    JSHandle<JSTaggedValue> dupVal = JSObject::GetProperty(thread, dupObj, aKey).GetValue();
    ASSERT_TRUE(dupVal->IsInt());
    EXPECT_EQ(dupVal->GetInt(), 2);
    JSHandle<JSHClass> dupHclass(thread, JSObject::Cast(dupResult)->GetJSHClass());
    EXPECT_FALSE(dupHclass->IsDictionaryMode());
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_ElementIndexKeys)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> aKey(factory->NewFromASCII("a"));
    JSHandle<JSTaggedValue> idx0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> idx1(thread, JSTaggedValue(1));

    JSHandle<JSTaggedValue> elemText(factory->NewFromASCII(R"({"0":"x","1":"y"})"));
    std::vector<JSTaggedValue> elemArgs{elemText.GetTaggedValue()};
    JSTaggedValue elemResult = CallParseV2(thread, elemArgs);
    ASSERT_TRUE(elemResult.IsJSSharedObject());
    JSHandle<JSObject> elemObj(thread, elemResult);
    JSHandle<JSHClass> elemHclass(thread, JSObject::Cast(elemResult)->GetJSHClass());
    EXPECT_TRUE(elemHclass->IsDictionaryElement());
    JSHandle<JSTaggedValue> elem0 = JSObject::GetProperty(thread, elemObj, idx0).GetValue();
    ASSERT_TRUE(elem0->IsString());
    EXPECT_STREQ("x", EcmaStringAccessor(JSHandle<EcmaString>(elem0)).ToCString(thread).c_str());
    JSHandle<JSTaggedValue> elem1 = JSObject::GetProperty(thread, elemObj, idx1).GetValue();
    ASSERT_TRUE(elem1->IsString());
    EXPECT_STREQ("y", EcmaStringAccessor(JSHandle<EcmaString>(elem1)).ToCString(thread).c_str());

    JSHandle<JSTaggedValue> elemDupText(factory->NewFromASCII(R"({"0":1,"0":2})"));
    std::vector<JSTaggedValue> elemDupArgs{elemDupText.GetTaggedValue()};
    JSTaggedValue elemDupResult = CallParseV2(thread, elemDupArgs);
    ASSERT_TRUE(elemDupResult.IsJSSharedObject());
    JSHandle<JSObject> elemDupObj(thread, elemDupResult);
    JSHandle<JSTaggedValue> elemDupVal = JSObject::GetProperty(thread, elemDupObj, idx0).GetValue();
    ASSERT_TRUE(elemDupVal->IsInt());
    EXPECT_EQ(elemDupVal->GetInt(), 2);

    JSHandle<JSTaggedValue> mixedText(factory->NewFromASCII(R"({"a":1,"0":2})"));
    std::vector<JSTaggedValue> mixedArgs{mixedText.GetTaggedValue()};
    JSTaggedValue mixedResult = CallParseV2(thread, mixedArgs);
    ASSERT_TRUE(mixedResult.IsJSSharedObject());
    JSHandle<JSObject> mixedObj(thread, mixedResult);
    EXPECT_TRUE(JSObject::GetProperty(thread, mixedObj, aKey).GetValue()->IsInt());
    EXPECT_EQ(JSObject::GetProperty(thread, mixedObj, idx0).GetValue()->GetInt(), 2);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_ElementKeyBoundary)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> aKey(factory->NewFromASCII("a"));
    JSHandle<JSTaggedValue> bKey(factory->NewFromASCII("b"));
    JSHandle<JSTaggedValue> idx0(thread, JSTaggedValue(0));

    std::string json86 = "{";
    for (size_t i = 0; i < 86; i++) {
        if (i > 0) {
            json86 += ",";
        }
        json86 += "\"" + std::to_string(i) + "\":\"value-" + std::to_string(i) + "\"";
    }
    json86 += "}";
    JSHandle<JSTaggedValue> text86(factory->NewFromStdString(json86));
    std::vector<JSTaggedValue> args86{text86.GetTaggedValue()};
    JSTaggedValue result86 = CallParseV2(thread, args86);
    ASSERT_TRUE(result86.IsJSSharedObject());
    JSHandle<JSTaggedValue> obj86(thread, result86);
    JSHandle<JSTaggedValue> key85(thread, JSTaggedValue(85));
    JSHandle<JSTaggedValue> val85 = JSObject::GetProperty(thread, obj86, key85).GetValue();
    JSHandle<EcmaString> val85Str(val85);
    EXPECT_STREQ("value-85", EcmaStringAccessor(val85Str).ToCString(thread).c_str());

    JSHandle<JSTaggedValue> wsText(factory->NewFromASCII(R"({ "a" : 1 , "b" : [ 2 ] })"));
    std::vector<JSTaggedValue> wsArgs{wsText.GetTaggedValue()};
    JSTaggedValue wsResult = CallParseV2(thread, wsArgs);
    ASSERT_TRUE(wsResult.IsJSSharedObject());
    JSHandle<JSObject> wsObj(thread, wsResult);
    EXPECT_EQ(JSObject::GetProperty(thread, wsObj, aKey).GetValue()->GetInt(), 1);
    JSHandle<JSTaggedValue> wsB = JSObject::GetProperty(thread, wsObj, bKey).GetValue();
    ASSERT_TRUE(wsB->IsJSSharedArray());
    JSHandle<JSTaggedValue> wsBHandle(thread, wsB.GetTaggedValue());
    EXPECT_EQ(JSObject::GetProperty(thread, wsBHandle, idx0).GetValue()->GetInt(), 2);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_DictModeObject)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    const uint32_t keyCount = JSSharedObject::MAX_INLINE + 1;
    std::string jsonDict = "{";
    for (uint32_t i = 0; i < keyCount; i++) {
        if (i > 0) {
            jsonDict += ",";
        }
        jsonDict += "\"k" + std::to_string(i) + "\":" + std::to_string(i);
    }
    jsonDict += ",\"k5\":55,\"k7\":77,\"k7\":88,\"0\":7,\"0\":8}";
    JSHandle<JSTaggedValue> dictText(factory->NewFromStdString(jsonDict));
    std::vector<JSTaggedValue> dictArgs{dictText.GetTaggedValue()};
    JSTaggedValue dictResult = CallParseV2(thread, dictArgs);
    ASSERT_TRUE(dictResult.IsJSSharedObject());
    JSHandle<JSObject> dictObj(thread, dictResult);
    JSHandle<JSHClass> dictHclass(thread, JSObject::Cast(dictResult)->GetJSHClass());
    EXPECT_TRUE(dictHclass->IsDictionaryMode());

    JSHandle<JSTaggedValue> k5(factory->NewFromASCII("k5"));
    JSHandle<JSTaggedValue> val5 = JSObject::GetProperty(thread, dictObj, k5).GetValue();
    ASSERT_TRUE(val5->IsInt());
    EXPECT_EQ(val5->GetInt(), 55);
    JSHandle<JSTaggedValue> k7(factory->NewFromASCII("k7"));
    JSHandle<JSTaggedValue> val7 = JSObject::GetProperty(thread, dictObj, k7).GetValue();
    ASSERT_TRUE(val7->IsInt());
    EXPECT_EQ(val7->GetInt(), 88);
    JSHandle<JSTaggedValue> kLast(factory->NewFromStdString("k" + std::to_string(keyCount - 1)));
    JSHandle<JSTaggedValue> valLast = JSObject::GetProperty(thread, dictObj, kLast).GetValue();
    ASSERT_TRUE(valLast->IsInt());
    EXPECT_EQ(valLast->GetInt(), static_cast<int>(keyCount - 1));

    EXPECT_TRUE(dictHclass->IsDictionaryElement());
    JSHandle<JSTaggedValue> idx0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> elem0 = JSObject::GetProperty(thread, dictObj, idx0).GetValue();
    ASSERT_TRUE(elem0->IsInt());
    EXPECT_EQ(elem0->GetInt(), 8);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_ArrayConstruction)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> emptyArrText(factory->NewFromASCII("[]"));
    std::vector<JSTaggedValue> emptyArrArgs{emptyArrText.GetTaggedValue()};
    JSTaggedValue emptyArrResult = CallParseV2(thread, emptyArrArgs);
    ASSERT_TRUE(emptyArrResult.IsJSSharedArray());
    EXPECT_EQ(JSHandle<JSSharedArray>(thread, JSSharedArray::Cast(emptyArrResult))->GetArrayLength(), 0U);

    JSHandle<JSTaggedValue> arrText(factory->NewFromASCII(R"([1,"s",true,null])"));
    std::vector<JSTaggedValue> arrArgs{arrText.GetTaggedValue()};
    JSTaggedValue arrResult = CallParseV2(thread, arrArgs);
    ASSERT_TRUE(arrResult.IsJSSharedArray());
    JSHandle<JSSharedArray> sharedArr(thread, JSSharedArray::Cast(arrResult));
    EXPECT_EQ(sharedArr->GetArrayLength(), 4U);
    JSHandle<JSTaggedValue> arrHandle(sharedArr);
    JSHandle<JSTaggedValue> idx0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> idx1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> idx2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> idx3(thread, JSTaggedValue(3));
    EXPECT_TRUE(JSObject::GetProperty(thread, arrHandle, idx0).GetValue()->IsInt());
    JSHandle<JSTaggedValue> elem1 = JSObject::GetProperty(thread, arrHandle, idx1).GetValue();
    ASSERT_TRUE(elem1->IsString());
    EXPECT_STREQ("s", EcmaStringAccessor(JSHandle<EcmaString>(elem1)).ToCString(thread).c_str());
    EXPECT_TRUE(JSObject::GetProperty(thread, arrHandle, idx2).GetValue()->IsTrue());
    EXPECT_TRUE(JSObject::GetProperty(thread, arrHandle, idx3).GetValue()->IsNull());

    JSHandle<JSTaggedValue> nestedArrText(factory->NewFromASCII(R"([[1],[[2]]])"));
    std::vector<JSTaggedValue> nestedArrArgs{nestedArrText.GetTaggedValue()};
    JSTaggedValue nestedArrResult = CallParseV2(thread, nestedArrArgs);
    ASSERT_TRUE(nestedArrResult.IsJSSharedArray());
    JSHandle<JSSharedArray> outerArr(thread, JSSharedArray::Cast(nestedArrResult));
    EXPECT_EQ(outerArr->GetArrayLength(), 2U);
    JSHandle<JSTaggedValue> outerHandle(outerArr);
    JSHandle<JSTaggedValue> inner0 = JSObject::GetProperty(thread, outerHandle, idx0).GetValue();
    ASSERT_TRUE(inner0->IsJSSharedArray());
    EXPECT_EQ(JSHandle<JSSharedArray>(thread, JSSharedArray::Cast(inner0.GetTaggedValue()))
                  ->GetArrayLength(), 1U);
    JSHandle<JSTaggedValue> inner1 = JSObject::GetProperty(thread, outerHandle, idx1).GetValue();
    ASSERT_TRUE(inner1->IsJSSharedArray());
    EXPECT_EQ(JSHandle<JSSharedArray>(thread, JSSharedArray::Cast(inner1.GetTaggedValue()))
                  ->GetArrayLength(), 1U);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_NestedComposite)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> idx1(thread, JSTaggedValue(1));

    JSHandle<JSTaggedValue> compositeText(factory->NewFromASCII(R"({"a":[1,{"b":true}],"c":{"d":null}})"));
    std::vector<JSTaggedValue> compositeArgs{compositeText.GetTaggedValue()};
    JSTaggedValue compositeResult = CallParseV2(thread, compositeArgs);
    ASSERT_TRUE(compositeResult.IsJSSharedObject());
    JSHandle<JSTaggedValue> compositeObj(thread, compositeResult);
    JSHandle<JSTaggedValue> aKey(factory->NewFromASCII("a"));
    JSHandle<JSTaggedValue> cKey(factory->NewFromASCII("c"));
    JSHandle<JSTaggedValue> bKey(factory->NewFromASCII("b"));
    JSHandle<JSTaggedValue> dKey(factory->NewFromASCII("d"));
    JSHandle<JSTaggedValue> arrA = JSObject::GetProperty(thread, compositeObj, aKey).GetValue();
    ASSERT_TRUE(arrA->IsJSSharedArray());
    EXPECT_EQ(JSHandle<JSSharedArray>(thread, JSSharedArray::Cast(arrA.GetTaggedValue()))->GetArrayLength(),
              2U);
    JSHandle<JSTaggedValue> arrAHandle(thread, arrA.GetTaggedValue());
    JSHandle<JSTaggedValue> arrA1 = JSObject::GetProperty(thread, arrAHandle, idx1).GetValue();
    ASSERT_TRUE(arrA1->IsJSSharedObject());
    EXPECT_TRUE(JSObject::GetProperty(thread, arrA1, bKey).GetValue()->IsTrue());
    JSHandle<JSTaggedValue> objC = JSObject::GetProperty(thread, compositeObj, cKey).GetValue();
    ASSERT_TRUE(objC->IsJSSharedObject());
    JSHandle<JSTaggedValue> objCHandle(thread, objC.GetTaggedValue());
    EXPECT_TRUE(JSObject::GetProperty(thread, objCHandle, dKey).GetValue()->IsNull());
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_TopLevelLeafValues)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> idx0(thread, JSTaggedValue(0));

    JSHandle<JSTaggedValue> strText(factory->NewFromASCII(R"("str")"));
    std::vector<JSTaggedValue> strArgs{strText.GetTaggedValue()};
    JSTaggedValue strResult = CallParseV2(thread, strArgs);
    ASSERT_FALSE(strResult.IsException());
    EXPECT_TRUE(strResult.IsString());

    JSHandle<JSTaggedValue> numText(factory->NewFromASCII("123"));
    std::vector<JSTaggedValue> numArgs{numText.GetTaggedValue()};
    JSTaggedValue numResult = CallParseV2(thread, numArgs);
    ASSERT_FALSE(numResult.IsException());
    EXPECT_TRUE(numResult.IsNumber());

    JSHandle<JSTaggedValue> trueText(factory->NewFromASCII("true"));
    std::vector<JSTaggedValue> trueArgs{trueText.GetTaggedValue()};
    JSTaggedValue trueResult = CallParseV2(thread, trueArgs);
    ASSERT_FALSE(trueResult.IsException());
    EXPECT_TRUE(trueResult.IsTrue());

    JSHandle<JSTaggedValue> nullText(factory->NewFromASCII("null"));
    std::vector<JSTaggedValue> nullArgs{nullText.GetTaggedValue()};
    JSTaggedValue nullResult = CallParseV2(thread, nullArgs);
    ASSERT_FALSE(nullResult.IsException());
    EXPECT_TRUE(nullResult.IsNull());

    JSHandle<JSTaggedValue> falseText(factory->NewFromASCII("false"));
    std::vector<JSTaggedValue> falseArgs{falseText.GetTaggedValue()};
    JSTaggedValue falseResult = CallParseV2(thread, falseArgs);
    ASSERT_FALSE(falseResult.IsException());
    EXPECT_TRUE(falseResult.IsFalse());

    JSHandle<JSTaggedValue> escText(factory->NewFromASCII(R"(["a\"b"])"));
    std::vector<JSTaggedValue> escArgs{escText.GetTaggedValue()};
    JSTaggedValue escResult = CallParseV2(thread, escArgs);
    ASSERT_TRUE(escResult.IsJSSharedArray());
    JSHandle<JSTaggedValue> escArr(thread, escResult);
    JSHandle<JSTaggedValue> escElem = JSObject::GetProperty(thread, escArr, idx0).GetValue();
    ASSERT_TRUE(escElem->IsString());
    EXPECT_STREQ("a\"b", EcmaStringAccessor(JSHandle<EcmaString>(escElem)).ToCString(thread).c_str());
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_SyntaxErrors_Object)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> mapOptions = CreateMapModeOptions(thread);

    JSHandle<JSTaggedValue> openQuoteText(factory->NewFromASCII("{a:1}"));
    ExpectParseV2SyntaxError(thread, {openQuoteText.GetTaggedValue()},
                             "Unexpected Object Prop in JSON");
    JSHandle<JSTaggedValue> commaQuoteText(factory->NewFromASCII(R"({"a":1,b:2})"));
    ExpectParseV2SyntaxError(thread, {commaQuoteText.GetTaggedValue()},
                             "Unexpected Object Prop in JSON");
    JSHandle<JSTaggedValue> colonText(factory->NewFromASCII(R"({"a" 1})"));
    ExpectParseV2SyntaxError(thread, {colonText.GetTaggedValue()},
                             "Unexpected Object in JSON");
    JSHandle<JSTaggedValue> objOpenText(factory->NewFromASCII(R"({"a":1)"));
    ExpectParseV2SyntaxError(thread, {objOpenText.GetTaggedValue()},
                             "Unexpected Number in JSON Array Or Object");
    JSHandle<JSTaggedValue> objOpenLitText(factory->NewFromASCII(R"({"a":true)"));
    ExpectParseV2SyntaxError(thread, {objOpenLitText.GetTaggedValue()},
                             "Unexpected Object in JSON");

    JSHandle<JSTaggedValue> mapCommaText(factory->NewFromASCII(R"({"a":1,b:2})"));
    std::vector<JSTaggedValue> mapCommaArgs{mapCommaText.GetTaggedValue(), JSTaggedValue::Undefined(),
                                            mapOptions.GetTaggedValue()};
    ExpectParseV2SyntaxError(thread, mapCommaArgs, "Unexpected MAP Prop in JSON");
    JSHandle<JSTaggedValue> mapColonText(factory->NewFromASCII(R"({"a" 1})"));
    std::vector<JSTaggedValue> mapColonArgs{mapColonText.GetTaggedValue(), JSTaggedValue::Undefined(),
                                            mapOptions.GetTaggedValue()};
    ExpectParseV2SyntaxError(thread, mapColonArgs, "Unexpected MAP in JSON");
    JSHandle<JSTaggedValue> mapOpenText(factory->NewFromASCII(R"({"a":1)"));
    std::vector<JSTaggedValue> mapOpenArgs{mapOpenText.GetTaggedValue(), JSTaggedValue::Undefined(),
                                           mapOptions.GetTaggedValue()};
    ExpectParseV2SyntaxError(thread, mapOpenArgs, "Unexpected Number in JSON Array Or Object");
    JSHandle<JSTaggedValue> mapOpenLitText(factory->NewFromASCII(R"({"a":true)"));
    std::vector<JSTaggedValue> mapOpenLitArgs{mapOpenLitText.GetTaggedValue(), JSTaggedValue::Undefined(),
                                              mapOptions.GetTaggedValue()};
    ExpectParseV2SyntaxError(thread, mapOpenLitArgs, "Unexpected MAP in JSON");
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_SyntaxErrors_Array)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> arrOpenText(factory->NewFromASCII("[1,2"));
    ExpectParseV2SyntaxError(thread, {arrOpenText.GetTaggedValue()},
                             "Unexpected Number in JSON Array Or Object");
    JSHandle<JSTaggedValue> arrOpenLitText(factory->NewFromASCII("[true"));
    ExpectParseV2SyntaxError(thread, {arrOpenLitText.GetTaggedValue()},
                             "Unexpected Array in JSON");
    JSHandle<JSTaggedValue> arrSepText(factory->NewFromASCII("[1 2]"));
    ExpectParseV2SyntaxError(thread, {arrSepText.GetTaggedValue()},
                             "Unexpected Number in JSON Array Or Object");
    JSHandle<JSTaggedValue> illegalText(factory->NewFromASCII("[1,]"));
    ExpectParseV2SyntaxError(thread, {illegalText.GetTaggedValue()},
                             "Unexpected Text in JSON: Invalid Token");
    JSHandle<JSTaggedValue> trailingText(factory->NewFromASCII("{} x"));
    ExpectParseV2SyntaxError(thread, {trailingText.GetTaggedValue()},
                             "Unexpected Text in JSON: Remaining Text Before Return");
    JSHandle<JSTaggedValue> eoiText(factory->NewFromASCII("[1,"));
    ExpectParseV2SyntaxError(thread, {eoiText.GetTaggedValue()},
                             "Unexpected end in JSON");
    JSHandle<JSTaggedValue> literalText(factory->NewFromASCII("tru"));
    ExpectParseV2SyntaxError(thread, {literalText.GetTaggedValue()},
                             "Unexpected Text in JSON: ParseLiteralTrue Fail");
    JSHandle<JSTaggedValue> badKeyText(factory->NewFromASCII(R"({"abc)"));
    ExpectParseV2SyntaxError(thread, {badKeyText.GetTaggedValue()},
                             "Unexpected end Text in JSON");
    JSHandle<JSTaggedValue> badStrText(factory->NewFromASCII(R"("abc)"));
    ExpectParseV2SyntaxError(thread, {badStrText.GetTaggedValue()},
                             "Unexpected end Text in JSON");
    JSHandle<JSTaggedValue> emptyInputText(factory->NewFromASCII(""));
    ExpectParseV2SyntaxError(thread, {emptyInputText.GetTaggedValue()},
                             "Unexpected Text in JSON: Empty Text");
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_ArgumentArity)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> text(factory->NewFromASCII(R"({"a":1})"));

    JSTaggedValue none = CallParseV2WithArgc(thread, {}, 0);
    EXPECT_TRUE(none.IsException());
    bool hadException = false;
    ClearExceptionIfAny(thread, hadException);
    EXPECT_TRUE(hadException);

    JSTaggedValue single = CallParseV2WithArgc(thread, {text.GetTaggedValue()}, 1);
    ASSERT_FALSE(single.IsException());
    EXPECT_TRUE(single.IsJSSharedObject());

    JSTaggedValue two = CallParseV2WithArgc(thread, {text.GetTaggedValue(), JSTaggedValue::Undefined()},
                                            2);
    ASSERT_FALSE(two.IsException());
    EXPECT_TRUE(two.IsJSSharedObject());

    JSTaggedValue legacy = CallLegacySendableParse(thread, {text.GetTaggedValue()});
    ASSERT_FALSE(legacy.IsException());
    EXPECT_TRUE(legacy.IsJSSharedObject());

    JSTaggedValue four = CallParseV2WithArgc(thread, {text.GetTaggedValue(), JSTaggedValue::Undefined(),
                                                      JSTaggedValue::Undefined(), JSTaggedValue::Undefined()},
                                             4);
    EXPECT_TRUE(four.IsException());
    ClearExceptionIfAny(thread, hadException);
    EXPECT_TRUE(hadException);
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_TopLevelBigInt)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> bigText(factory->NewFromASCII("123456789012345678901234567890"));

    JSHandle<JSObject> optionsAlways = CreateBigIntModeOptions(thread, 2);  // 2: ALWAYS_PARSE_AS_BIGINT
    std::vector<JSTaggedValue> bigArgs{bigText.GetTaggedValue(), JSTaggedValue::Undefined(),
                                       optionsAlways.GetTaggedValue()};
    JSTaggedValue bigResult = CallParseV2(thread, bigArgs);
    ASSERT_FALSE(bigResult.IsException());
    EXPECT_TRUE(bigResult.IsBigInt());

    std::vector<JSTaggedValue> defaultArgs{bigText.GetTaggedValue(), JSTaggedValue::Undefined(),
                                           JSTaggedValue::Undefined()};
    JSTaggedValue defaultResult = CallParseV2(thread, defaultArgs);
    ASSERT_FALSE(defaultResult.IsException());
    EXPECT_TRUE(defaultResult.IsNumber());
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_NumberFormats)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    for (const char *numText : {"1e5", "1E+2", "1.5e-3", "0.25"}) {
        JSHandle<JSTaggedValue> okText(factory->NewFromASCII(numText));
        JSTaggedValue okResult = CallParseV2(thread, {okText.GetTaggedValue()});
        ASSERT_FALSE(okResult.IsException()) << "text: " << numText;
        EXPECT_TRUE(okResult.IsNumber()) << "text: " << numText;
    }

    JSHandle<JSTaggedValue> dotText(factory->NewFromASCII("1."));
    ExpectParseV2SyntaxError(thread, {dotText.GetTaggedValue()},
                             "Unexpected Number in JSON");
    JSHandle<JSTaggedValue> expText(factory->NewFromASCII("1e"));
    ExpectParseV2SyntaxError(thread, {expText.GetTaggedValue()},
                             "Unexpected Number in JSON");
    JSHandle<JSTaggedValue> dotExpText(factory->NewFromASCII("1.5e"));
    ExpectParseV2SyntaxError(thread, {dotExpText.GetTaggedValue()},
                             "Unexpected Number in JSON");
    JSHandle<JSTaggedValue> badExpText(factory->NewFromASCII("1.5ex"));
    ExpectParseV2SyntaxError(thread, {badExpText.GetTaggedValue()},
                             "Unexpected Number in JSON");
}

HWTEST_F_L0(BuiltinsJsonTest, ParseSendableV2_LegacyReviverIgnored)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> text(factory->NewFromASCII(R"({"a":1})"));
    JSHandle<JSTaggedValue> objectCtor(env->GetObjectFunction());

    std::vector<JSTaggedValue> args{text.GetTaggedValue(), objectCtor.GetTaggedValue()};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(
        thread, const_cast<std::vector<JSTaggedValue> &>(args), 8);  // 8: argc == 2
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsJson::ParseWithTransformType(ecmaRuntimeCallInfo,
                                        base::JsonHelper::TransformType::SENDABLE);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_FALSE(result.IsException());
    EXPECT_TRUE(result.IsJSSharedObject());
    bool hadPending = false;
    ClearExceptionIfAny(thread, hadPending);
    EXPECT_TRUE(hadPending);
}
}  // namespace panda::test
