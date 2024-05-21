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

#include "ecmascript/builtins/builtins_number.h"

#include <cmath>
#include <iostream>

#include "ecmascript/base/number_helper.h"
#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_global_object.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_primitive_ref.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsNumberTest : public BaseTestWithScope<false> {
};

// new Number(10)
HWTEST_F_L0(BuiltinsNumberTest, NumberConstructor)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSFunction> number(env->GetNumberFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*number), 6);
    ecmaRuntimeCallInfo->SetFunction(number.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(5)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::NumberConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    JSTaggedValue value(static_cast<JSTaggedType>(result.GetRawData()));
    ASSERT_TRUE(value.IsECMAObject());
    JSPrimitiveRef *ref = JSPrimitiveRef::Cast(value.GetTaggedObject());
    ASSERT_EQ(ref->GetValue().GetDouble(), 5.0);
}

// Number.isFinite(-10)
HWTEST_F_L0(BuiltinsNumberTest, IsFinite)
{
    const double value = -10;
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(value)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::IsFinite(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// Number.isFinite(Number.MAX_VALUE)
HWTEST_F_L0(BuiltinsNumberTest, IsFinite1)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::MAX_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::IsFinite(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// Number.isFinite("helloworld")
HWTEST_F_L0(BuiltinsNumberTest, IsFinite2)
{
    JSHandle<EcmaString> test = thread->GetEcmaVM()->GetFactory()->NewFromASCII("helloworld");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, test.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::IsFinite(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// Number.isFinite(NaN)
HWTEST_F_L0(BuiltinsNumberTest, IsFinite3)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::NAN_VALUE));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::IsFinite(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// Number.isFinite(Infinity)
HWTEST_F_L0(BuiltinsNumberTest, IsFinite4)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(base::POSITIVE_INFINITY));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::IsFinite(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// Number.isFinite(undefined)
HWTEST_F_L0(BuiltinsNumberTest, IsFinite5)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::IsFinite(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// Number.isFinite(null)
HWTEST_F_L0(BuiltinsNumberTest, IsFinite6)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Null());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::IsFinite(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// Number.isInteger(0.1)
HWTEST_F_L0(BuiltinsNumberTest, IsInteger)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(0.1));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::IsInteger(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// Number.isNaN(0.1)
HWTEST_F_L0(BuiltinsNumberTest, IsNaN)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(0.1));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::IsNaN(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue::False().GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsNumberTest, ToString1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    // new Number(123.456).toString(7)
    JSHandle<JSFunction> numberObject(env->GetNumberFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(123.456));
    JSHandle<JSPrimitiveRef> number = thread->GetEcmaVM()->GetFactory()->NewJSPrimitiveRef(numberObject, value);

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(number.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(7.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = BuiltinsNumber::ToString(ecmaRuntimeCallInfo1);
    ASSERT_TRUE(result1.IsString());
    JSHandle<EcmaString> res1(thread, reinterpret_cast<EcmaString *>(result1.GetRawData()));
    JSHandle<EcmaString> correctResult1 = factory->NewFromASCII("234.312256641535441");
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*res1, *correctResult1));
    TestHelper::TearDownFrame(thread, prev);

    // (15).toString(4)
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(JSTaggedValue(15));
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue(4));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result2 = BuiltinsNumber::ToString(ecmaRuntimeCallInfo2);
    ASSERT_TRUE(result2.IsString());
    JSHandle<EcmaString> res2(thread, reinterpret_cast<EcmaString *>(result2.GetRawData()));
    JSHandle<EcmaString> correctResult2 = factory->NewFromASCII("33");
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*res2, *correctResult2));
    TestHelper::TearDownFrame(thread, prev);

    // (5).toString(8)
    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(JSTaggedValue(5));
    ecmaRuntimeCallInfo3->SetCallArg(0, JSTaggedValue(8));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3);
    JSTaggedValue result3 = BuiltinsNumber::ToString(ecmaRuntimeCallInfo3);
    ASSERT_TRUE(result3.IsString());
    JSHandle<EcmaString> res3(thread, reinterpret_cast<EcmaString *>(result3.GetRawData()));
    JSHandle<EcmaString> correctResult3 = factory->NewFromASCII("5");
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*res3, *correctResult3));
    TestHelper::TearDownFrame(thread, prev);

    // (0).toString(8)
    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo4->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetThis(JSTaggedValue(0));
    ecmaRuntimeCallInfo4->SetCallArg(0, JSTaggedValue(8));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4);
    JSTaggedValue result4 = BuiltinsNumber::ToString(ecmaRuntimeCallInfo4);
    ASSERT_TRUE(result4.IsString());
    JSHandle<EcmaString> res4(thread, reinterpret_cast<EcmaString *>(result4.GetRawData()));
    JSHandle<EcmaString> correctResult4 = factory->NewFromASCII("0");
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*res4, *correctResult4));
    TestHelper::TearDownFrame(thread, prev);
}
HWTEST_F_L0(BuiltinsNumberTest, ToString2)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // (-50).toString(35)
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue(-50));
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue(35));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = BuiltinsNumber::ToString(ecmaRuntimeCallInfo1);
    ASSERT_TRUE(result1.IsString());
    JSHandle<EcmaString> res1(thread, reinterpret_cast<EcmaString *>(result1.GetRawData()));
    JSHandle<EcmaString> correctResult1 = factory->NewFromASCII("-1f");
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*res1, *correctResult1));
    TestHelper::TearDownFrame(thread, prev);

    // (2).toString(2.5)
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(JSTaggedValue(2));
    ecmaRuntimeCallInfo2->SetCallArg(0, JSTaggedValue(2.5));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result2 = BuiltinsNumber::ToString(ecmaRuntimeCallInfo2);
    ASSERT_TRUE(result2.IsString());
    JSHandle<EcmaString> res2(thread, reinterpret_cast<EcmaString *>(result2.GetRawData()));
    JSHandle<EcmaString> correctResult2 = factory->NewFromASCII("10");
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*res2, *correctResult2));
    TestHelper::TearDownFrame(thread, prev);
}

// new Number(123.456).toExponential(5)
HWTEST_F_L0(BuiltinsNumberTest, IsExponential)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSFunction> numberObject(env->GetNumberFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(123.456));
    JSHandle<JSPrimitiveRef> number = factory->NewJSPrimitiveRef(numberObject, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(number.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(5.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::ToExponential(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> res(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSHandle<EcmaString> correctResult = factory->NewFromASCII("1.23456e+2");
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*res, *correctResult));
    TestHelper::TearDownFrame(thread, prev);
}

// new Number(123.456).toFixed(10)
HWTEST_F_L0(BuiltinsNumberTest, ToFixed)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSFunction> numberObject(env->GetNumberFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(123.456));
    JSHandle<JSPrimitiveRef> number = factory->NewJSPrimitiveRef(numberObject, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(number.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(10.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::ToFixed(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> res(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSHandle<EcmaString> correctResult = factory->NewFromASCII("123.4560000000");
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*res, *correctResult));
    TestHelper::TearDownFrame(thread, prev);
}

// new Number(123.456).toFixed(30)
HWTEST_F_L0(BuiltinsNumberTest, ToFixed1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSFunction> numberObject(env->GetNumberFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(123.456));
    JSHandle<JSPrimitiveRef> number = factory->NewJSPrimitiveRef(numberObject, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(number.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(30.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::ToFixed(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> res(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSHandle<EcmaString> correctResult = factory->NewFromASCII("123.456000000000003069544618483633");
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*res, *correctResult));
    TestHelper::TearDownFrame(thread, prev);
}

// new Number(1e21).toFixed(20)
HWTEST_F_L0(BuiltinsNumberTest, ToFixed2) {
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto ecmaVM = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVM->GetGlobalEnv();

    JSHandle<JSFunction> numberObject(env->GetNumberFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(1e21));
    JSHandle<JSPrimitiveRef> number = factory->NewJSPrimitiveRef(numberObject, value);

    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(number.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(20.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::ToFixed(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> res(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSHandle<EcmaString> correctResult = factory->NewFromASCII("1e+21");
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*res, *correctResult));
    TestHelper::TearDownFrame(thread, prev);
}

// new Number(123.456).toPrecision(8)
HWTEST_F_L0(BuiltinsNumberTest, ToPrecision)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSFunction> numberObject(env->GetNumberFunction());
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(123.456));
    JSHandle<JSPrimitiveRef> number = factory->NewJSPrimitiveRef(numberObject, value);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(number.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(8.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::ToPrecision(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result.IsString());
    JSHandle<EcmaString> res(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    JSHandle<EcmaString> correctResult = factory->NewFromASCII("123.45600");
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(*res, *correctResult));
    TestHelper::TearDownFrame(thread, prev);
}

// Number.parseFloat(0x123)
HWTEST_F_L0(BuiltinsNumberTest, parseFloat)
{
    JSHandle<EcmaString> param = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0x123");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, param.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::ParseFloat(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(0)).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// Number.parseFloat(0x123xx)
HWTEST_F_L0(BuiltinsNumberTest, parseFloat1)
{
    JSHandle<EcmaString> param = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0x123xx");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, param.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::ParseFloat(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(0)).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// Number.parseInt(0x123)
HWTEST_F_L0(BuiltinsNumberTest, parseInt)
{
    const char *number = "0x123";

    JSHandle<EcmaString> param = thread->GetEcmaVM()->GetFactory()->NewFromASCII(number);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, param.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(16.0));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsNumber::ParseInt(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<int>(291)).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

// testcases of StringToDouble flags
HWTEST_F_L0(BuiltinsNumberTest, StringToDoubleFlags)
{
    JSHandle<EcmaString> str;
    Span<const uint8_t> sp;
    CVector<uint8_t> buf;

    // flags of IGNORE_TRAILING

    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0a");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 0);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0b");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 0);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0o");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 0);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII(" 00x");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 0);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII(" 000.4_");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 0.4);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII(" 0010.s ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 10);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII(" 0010e2");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 1000);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII(" 0010e+3_0");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::IGNORE_TRAILING), 10000);

    // flags of ALLOW_HEX
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0x");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_TRUE(std::isnan(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_HEX)));
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  0x10 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_HEX), 16);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0x1g");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_HEX + base::IGNORE_TRAILING), 1);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0xh");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_TRUE(std::isnan(
        base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_HEX + base::IGNORE_TRAILING)));

    // flags of ALLOW_OCTAL
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0O");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_TRUE(std::isnan(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_OCTAL)));
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  0o10 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_OCTAL), 8);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0o1d");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_OCTAL | base::IGNORE_TRAILING),
              1);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0o8");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_TRUE(std::isnan(
        base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_OCTAL | base::IGNORE_TRAILING)));

    // flags of ALLOW_BINARY
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0b");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_TRUE(std::isnan(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_BINARY)));
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  0b10 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_BINARY), 2);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0b1d");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_BINARY | base::IGNORE_TRAILING),
              1);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("0b2");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_TRUE(std::isnan(
        base::NumberHelper::StringToDouble(sp.begin(), sp.end(), 0, base::ALLOW_BINARY | base::IGNORE_TRAILING)));
}

// testcases of StringToDouble radix
HWTEST_F_L0(BuiltinsNumberTest, StringToDoubleRadix)
{
    JSHandle<EcmaString> str;
    Span<const uint8_t> sp;
    CVector<uint8_t> buf;
    int radix;

    radix = 0;  // default 10
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII(" 100 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 100);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII(" 100.3e2 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 10030);
    radix = 1;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  0000 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 0);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  0001 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_TRUE(std::isnan(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS)));
    radix = 2;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 4);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  11 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 3);
    radix = 3;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 9);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  21 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 7);
    radix = 4;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 16);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  31 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 13);
    radix = 8;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 64);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  71 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 57);
    radix = 10;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 100);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  0020 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 20);
    radix = 16;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 256);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  1e ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 30);
    radix = 18;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 324);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  1g ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 34);
    radix = 25;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 625);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  1g ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 41);
    radix = 36;
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  100 ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 1296);
    str = thread->GetEcmaVM()->GetFactory()->NewFromASCII("  1z ");
    sp = EcmaStringAccessor(str).ToUtf8Span(buf);
    ASSERT_EQ(base::NumberHelper::StringToDouble(sp.begin(), sp.end(), radix, base::NO_FLAGS), 71);
}

HWTEST_F_L0(BuiltinsNumberTest, NumberToString)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> res = factory->NewFromASCII("100");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        base::NumberHelper::NumberToString(thread, JSTaggedValue(100)), res), 0);
    res = factory->NewFromASCII("11223344");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        base::NumberHelper::NumberToString(thread, JSTaggedValue(11223344)), res), 0);
    res = factory->NewFromASCII("1234567890");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        base::NumberHelper::NumberToString(thread, JSTaggedValue(1234567890)), res), 0);
    res = factory->NewFromASCII("100");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        base::NumberHelper::NumberToString(thread, JSTaggedValue(double(100.0))), res), 0);
    res = factory->NewFromASCII("100.5");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        base::NumberHelper::NumberToString(thread, JSTaggedValue(double(100.5))), res), 0);
    res = factory->NewFromASCII("100.25");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        base::NumberHelper::NumberToString(thread, JSTaggedValue(double(100.25))), res), 0);
    res = factory->NewFromASCII("100.125");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        base::NumberHelper::NumberToString(thread, JSTaggedValue(double(100.125))), res), 0);
    res = factory->NewFromASCII("100.6125");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        base::NumberHelper::NumberToString(thread, JSTaggedValue(double(100.6125))), res), 0);
    res = factory->NewFromASCII("0.0006125");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        base::NumberHelper::NumberToString(thread, JSTaggedValue(double(0.0006125))), res), 0);
    res = factory->NewFromASCII("-0.0006125");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        base::NumberHelper::NumberToString(thread, JSTaggedValue(double(-0.0006125))), res), 0);
    res = factory->NewFromASCII("-1234567890.0006125");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        base::NumberHelper::NumberToString(thread, JSTaggedValue(double(-1234567890.0006125))), res), 0);
    res = factory->NewFromASCII("1234567890.0006125");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        base::NumberHelper::NumberToString(thread, JSTaggedValue(double(1234567890.0006125))), res), 0);
    res = factory->NewFromASCII("11234567890.000612");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        base::NumberHelper::NumberToString(thread, JSTaggedValue(double(11234567890.0006125))), res), 0);
}
}  // namespace panda::test
