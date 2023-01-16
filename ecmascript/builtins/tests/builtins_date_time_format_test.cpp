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

#include "ecmascript/builtins/builtins_date_time_format.h"

#include <ctime>
#include <algorithm>
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using BuiltinsArray = ecmascript::builtins::BuiltinsArray;
class BuiltinsDateTimeFormatTest : public testing::Test {
public:
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
        JSRuntimeOptions options;
#if PANDA_TARGET_LINUX
        // for consistency requirement, use ohos_icu4j/data as icu-data-path
        options.SetIcuDataPath(ICU_PATH);
#endif
        options.SetEnableForceGC(true);
        instance = JSNApi::CreateEcmaVM(options);
        instance->SetEnableForceGC(true);
        ASSERT_TRUE(instance != nullptr) << "Cannot create EcmaVM";
        thread = instance->GetJSThread();
        scope = new EcmaHandleScope(thread);
    }

    void TearDown() override
    {
        TestHelper::DestroyEcmaVMWithScope(instance, scope);
    }

    EcmaVM *instance {nullptr};
    EcmaHandleScope *scope {nullptr};
    JSThread *thread {nullptr};
};

// new DateTimeFormat(locale)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, DateTimeFormatConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetDateTimeFormatFunction());

    JSHandle<JSTaggedValue> localesString(factory->NewFromASCII("en-US"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
    // option tag is default value
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDateTimeFormat::DateTimeFormatConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSDateTimeFormat());
}

static JSTaggedValue BuiltinsDateTimeOptionsSet(JSThread *thread)
{
    auto globalConst = thread->GlobalConstants();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> weekDay = globalConst->GetHandledWeekdayString();
    JSHandle<JSTaggedValue> dayPeriod = globalConst->GetHandledDayPeriodString();
    JSHandle<JSTaggedValue> hourCycle = globalConst->GetHandledHourCycleString();
    JSHandle<JSTaggedValue> timeZone = globalConst->GetHandledTimeZoneString();
    JSHandle<JSTaggedValue> numicValue(factory->NewFromASCII("numeric")); // test numeric
    JSHandle<JSTaggedValue> weekDayValue(factory->NewFromASCII("short")); // test short
    JSHandle<JSTaggedValue> dayPeriodValue(factory->NewFromASCII("long")); // test long
    JSHandle<JSTaggedValue> hourCycleValue(factory->NewFromASCII("h24")); // test h24
    JSHandle<JSTaggedValue> timeZoneValue(factory->NewFromASCII("UTC")); // test UTC

    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(6); // 6 : 6 length
    keyArray->Set(thread, 0, globalConst->GetHandledYearString()); // 0 : 0 first position
    keyArray->Set(thread, 1, globalConst->GetHandledMonthString()); // 1 : 1 second position
    keyArray->Set(thread, 2, globalConst->GetHandledDayString()); // 2 : 2 third position
    keyArray->Set(thread, 3, globalConst->GetHandledHourString()); // 3 : 3 fourth position
    keyArray->Set(thread, 4, globalConst->GetHandledMinuteString()); // 4 : 4 fifth position
    keyArray->Set(thread, 5, globalConst->GetHandledSecondString()); // 5 : 5 sixth position

    uint32_t arrayLen = keyArray->GetLength();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < arrayLen; i++) {
        key.Update(keyArray->Get(thread, i));
        JSObject::SetProperty(thread, optionsObj, key, numicValue);
    }
    JSObject::SetProperty(thread, optionsObj, weekDay, weekDayValue);
    JSObject::SetProperty(thread, optionsObj, dayPeriod, dayPeriodValue);
    JSObject::SetProperty(thread, optionsObj, hourCycle, hourCycleValue);
    JSObject::SetProperty(thread, optionsObj, timeZone, timeZoneValue);
    return optionsObj.GetTaggedValue();
}

static JSTaggedValue JSDateTimeFormatCreateWithLocaleTest(JSThread *thread, JSHandle<JSTaggedValue> &locale)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetDateTimeFormatFunction());
    JSHandle<JSObject> optionsObj(thread, BuiltinsDateTimeOptionsSet(thread));

    JSHandle<JSTaggedValue> localesString = locale;
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDateTimeFormat::DateTimeFormatConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsJSDateTimeFormat());
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

static double BuiltinsDateCreate(const double year, const double month, const double date)
{
    const double day = JSDate::MakeDay(year, month, date);
    const double time = JSDate::MakeTime(0, 0, 0, 0); // 24:00:00
    double days = JSDate::MakeDate(day, time);
    return days;
}

// Format.Tostring(en-US)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, Format_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
       JSHandle<JSDateTimeFormat>(thread, JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(jsDateTimeFormat.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = BuiltinsDateTimeFormat::Format(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    // jsDate supports zero to eleven, the month should be added with one
    JSHandle<JSFunction> jsFunction(thread, result1);
    JSArray *jsArray =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> jsObject(thread, jsArray);

    double days = BuiltinsDateCreate(2020, 10, 1);
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(static_cast<double>(days)));
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(jsFunction), true, true, true);
    JSHandle<JSTaggedValue> joinKey(factory->NewFromASCII("join"));
    JSArray::DefineOwnProperty(thread, jsObject, joinKey, desc);

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(jsObject.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, value.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result2 = BuiltinsArray::ToString(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> resultStr(thread, result2);
    EXPECT_STREQ("Sun, 11/1/2020, 24:00:00", EcmaStringAccessor(resultStr).ToCString().c_str());
}

// Format.Tostring(pt-BR)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, Format_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("pt-BR"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
       JSHandle<JSDateTimeFormat>(thread, JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(jsDateTimeFormat.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = BuiltinsDateTimeFormat::Format(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSFunction> jsFunction(thread, result1);
    JSArray *jsArray =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> jsObject(thread, jsArray);

    double days = BuiltinsDateCreate(2020, 5, 11);
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(static_cast<double>(days)));
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(jsFunction), true, true, true);
    JSHandle<JSTaggedValue> joinKey(factory->NewFromASCII("join"));
    JSArray::DefineOwnProperty(thread, jsObject, joinKey, desc);

    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(jsObject.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, value.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result2 = BuiltinsArray::ToString(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> resultStr(thread, result2);
    CString resStr = EcmaStringAccessor(resultStr).ToCString();
    // the index of string "qui" is zero.
    EXPECT_TRUE(resStr.find("qui") == 0);
    // the index of string "11/06/2020 24:00:00" is not zero.
    EXPECT_TRUE(resStr.find("11/06/2020 24:00:00") != 0);
}

HWTEST_F_L0(BuiltinsDateTimeFormatTest, FormatToParts)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
       JSHandle<JSDateTimeFormat>(thread, JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDateTimeFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDateTimeFormat::FormatToParts(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, result);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 16U); // sixteen formatters
}

// FormatRange(zh)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, FormatRange_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("zh"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
       JSHandle<JSDateTimeFormat>(thread, JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    double days1 = BuiltinsDateCreate(2020, 10, 1);
    double days2 = BuiltinsDateCreate(2021, 6, 1);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDateTimeFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(days1)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(days2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDateTimeFormat::FormatRange(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleStr(thread, result);
    JSHandle<EcmaString> resultStr = factory->NewFromUtf8("2020/11/1周日 24:00:00 – 2021/7/1周四 24:00:00");
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleStr, resultStr), 0);
}

// FormatRange(en)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, FormatRange_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
       JSHandle<JSDateTimeFormat>(thread, JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    double days1 = BuiltinsDateCreate(2020, 12, 1);
    double days2 = BuiltinsDateCreate(2021, 2, 1);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDateTimeFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(days1)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(days2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDateTimeFormat::FormatRange(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<EcmaString> handleStr(thread, result);
    JSHandle<EcmaString> resultStr = factory->NewFromUtf8("Fri, 1/1/2021, 24:00:00 – Mon, 3/1/2021, 24:00:00");
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleStr, resultStr), 0);
}

HWTEST_F_L0(BuiltinsDateTimeFormatTest, FormatRangeToParts)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
       JSHandle<JSDateTimeFormat>(thread, JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    double days1 = BuiltinsDateCreate(2020, 12, 1);
    double days2 = BuiltinsDateCreate(2021, 2, 1);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDateTimeFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue(static_cast<double>(days1)));
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue(static_cast<double>(days2)));

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDateTimeFormat::FormatRangeToParts(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, result);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 39U); // The number of characters of "Fri1/1/202124:00:00–Mon3/1/202124:00:00"
}

HWTEST_F_L0(BuiltinsDateTimeFormatTest, ResolvedOptions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de-ID"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
       JSHandle<JSDateTimeFormat>(thread, JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDateTimeFormat.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDateTimeFormat::ResolvedOptions(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSTaggedValue> resultObj =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result.GetRawData())));
    // judge whether the properties of the object are the same as those of jsdatetimeformat tag
    JSHandle<JSTaggedValue> localeKey = globalConst->GetHandledLocaleString();
    JSHandle<JSTaggedValue> localeValue(factory->NewFromASCII("de"));
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, localeKey).GetValue(), localeValue), true);
    JSHandle<JSTaggedValue> timeZone = globalConst->GetHandledTimeZoneString();
    JSHandle<JSTaggedValue> timeZoneValue(factory->NewFromASCII("UTC"));
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, timeZone).GetValue(), timeZoneValue), true);
}

// SupportedLocalesOf("best fit")
HWTEST_F_L0(BuiltinsDateTimeFormatTest, SupportedLocalesOf_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("id-u-co-pinyin-de-ID"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    // set the tag is default value
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue resultArr = BuiltinsDateTimeFormat::SupportedLocalesOf(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, resultArr);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 1U);

    JSHandle<EcmaString> resultStr(thread, elements->Get(0));
    EXPECT_STREQ("id-u-co-pinyin-de-id", EcmaStringAccessor(resultStr).ToCString().c_str());
}

// SupportedLocalesOf("look up")
HWTEST_F_L0(BuiltinsDateTimeFormatTest, SupportedLocalesOf_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> localeMatcherKey = thread->GlobalConstants()->GetHandledLocaleMatcherString();
    JSHandle<JSTaggedValue> localeMatcherValue(factory->NewFromASCII("lookup"));
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("id-u-co-pinyin-de-DE"));

    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, localeMatcherKey, localeMatcherValue);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue resultArr = BuiltinsDateTimeFormat::SupportedLocalesOf(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, resultArr);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 1U);

    JSHandle<EcmaString> resultStr(thread, elements->Get(0));
    EXPECT_STREQ("id-u-co-pinyin-de", EcmaStringAccessor(resultStr).ToCString().c_str());
}

static JSTaggedValue JSDateTime(JSThread *thread, JSTaggedValue &formatResult)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    double days = 1665187200000;
    // jsDate supports zero to eleven, the month should be added with one
    JSHandle<JSFunction> jsFunction(thread, formatResult);
    JSArray *jsArray =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> jsObject(thread, jsArray);
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(static_cast<double>(days)));
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(jsFunction), true, true, true);
    JSHandle<JSTaggedValue> joinKey(factory->NewFromASCII("join"));
    JSArray::DefineOwnProperty(thread, jsObject, joinKey, desc);
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(jsObject.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result2 = BuiltinsArray::ToString(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev2);
    return result2;
}

static JSTaggedValue JSDateTimeFormatConstructor(JSThread *thread, JSHandle<JSObject> &optionsObj,
    JSHandle<JSTaggedValue> &localesString)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetDateTimeFormatFunction());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDateTimeFormat::DateTimeFormatConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsJSDateTimeFormat());
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

static JSTaggedValue JSDateTimeFormatForObject(JSThread *thread, JSTaggedValue &constructorResult)
{
    JSHandle<JSDateTimeFormat> jsDateTimeFormat = JSHandle<JSDateTimeFormat>(thread, constructorResult);
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(jsDateTimeFormat.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, JSTaggedValue::Undefined());
    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = BuiltinsDateTimeFormat::Format(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev1);
    return result1;
}

static JSTaggedValue JSDateTimeFormatForObj_001(JSThread *thread)
{
    auto globalConst = thread->GlobalConstants();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> timeZoneName = globalConst->GetHandledTimeZoneNameString();
    JSHandle<JSTaggedValue> localesString(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> digitValue(factory->NewFromASCII("2-digit"));
    JSHandle<JSTaggedValue> timeZoneNameValue(factory->NewFromASCII("short"));

    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(6); // 6 : 6 length
    keyArray->Set(thread, 0, globalConst->GetHandledYearString()); // 0 : 0 first position
    keyArray->Set(thread, 1, globalConst->GetHandledMonthString()); // 1 : 1 second position
    keyArray->Set(thread, 2, globalConst->GetHandledDayString()); // 2 : 2 third position
    keyArray->Set(thread, 3, globalConst->GetHandledHourString()); // 3 : 3 fourth position
    keyArray->Set(thread, 4, globalConst->GetHandledMinuteString()); // 4 : 4 fifth position
    keyArray->Set(thread, 5, globalConst->GetHandledSecondString()); // 5 : 5 sixth position
    uint32_t arrayLen = keyArray->GetLength();
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < arrayLen; i++) {
        key.Update(keyArray->Get(thread, i));
        JSObject::SetProperty(thread, optionsObj, key, digitValue);
    }
    JSObject::SetProperty(thread, optionsObj, timeZoneName, timeZoneNameValue);
    return optionsObj.GetTaggedValue();
}

static int TimeOffset()
{
    // Get Sys time
    time_t rt = time(nullptr);
    // Convert Sys time to GMT time
    tm gtm = *gmtime(&rt);
    // Convert GMT time to Sys time
    time_t gt = mktime(&gtm);
    tm gtm2 = *localtime(&gt);
    // Calculate time difference
    int offset = ((rt - gt) + (gtm2.tm_isdst ? 3600 : 0)) / 60;
    return offset;
}

// DateTimeFormat_001
HWTEST_F_L0(BuiltinsDateTimeFormatTest, DateTimeFormat_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> localesString(factory->NewFromASCII("en-US"));
    auto jsObj = JSHandle<JSObject>(thread, JSDateTimeFormatForObj_001(thread));
    auto constructorResult = JSDateTimeFormatConstructor(thread, jsObj, localesString);
    auto formatResult = JSDateTimeFormatForObject(thread, constructorResult);
    auto dtResult = JSDateTime(thread, formatResult);
    JSHandle<EcmaString> resultStr(thread, dtResult);

    constexpr int shanghai = 480;
    constexpr int americaRegina = -360;
    constexpr int americaNewYork = -300;
    constexpr int sysDefaultTimezone = -180; // america_argentina_Buenos_Aires
    constexpr int utc = 0;
    auto cstr = EcmaStringAccessor(resultStr).ToCString();
    if (TimeOffset() == utc) {
        if (cstr.find("GMT") != std::string::npos) {
            EXPECT_STREQ("10/08/22, 12:00:00 AM GMT", cstr.c_str());
        }
        if (cstr.find("UTC") != std::string::npos) {
            EXPECT_STREQ("10/08/22, 12:00:00 AM UTC", cstr.c_str());
        }
    }
    if (TimeOffset() == shanghai) {
        if (cstr.find("CST") != std::string::npos) {
            EXPECT_STREQ("10/08/22, 08:00:00 AM CST", cstr.c_str());
        }
        if (cstr.find("GMT+8") != std::string::npos) {
            EXPECT_STREQ("10/08/22, 08:00:00 AM GMT+8", cstr.c_str());
        }
    }
    if (TimeOffset() == americaRegina) {
        if (cstr.find("CST") != std::string::npos) {
            EXPECT_STREQ("10/07/22, 06:00:00 PM CST", cstr.c_str());
        }
    }
    if (TimeOffset() == americaNewYork) {
        if (cstr.find("EST") != std::string::npos) {
            EXPECT_STREQ("10/07/22, 06:00:00 PM EST", cstr.c_str());
        }
    }
    if (TimeOffset() == sysDefaultTimezone) {
        if (cstr.find("GMT-3") != std::string::npos) {
            EXPECT_STREQ("10/07/22, 09:00:00 PM GMT-3", cstr.c_str());
        }
    }
}

static JSTaggedValue JSDateTimeFormatForObj_002(JSThread *thread)
{
    auto globalConst = thread->GlobalConstants();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> timeZoneName = globalConst->GetHandledTimeZoneNameString();
    JSHandle<JSTaggedValue> timeZone = globalConst->GetHandledTimeZoneString();
    JSHandle<JSTaggedValue> numicValue(factory->NewFromASCII("numeric"));
    JSHandle<JSTaggedValue> digitValue(factory->NewFromASCII("2-digit"));
    JSHandle<JSTaggedValue> longValue(factory->NewFromASCII("long"));
    JSHandle<JSTaggedValue> timeZoneNameValue(factory->NewFromASCII("short"));
    JSHandle<JSTaggedValue> timeZoneValue(factory->NewFromASCII("UTC"));

    JSHandle<TaggedArray> keyArray = factory->NewTaggedArray(6); // 6 : 6 length
    keyArray->Set(thread, 0, globalConst->GetHandledYearString()); // 0 : 0 first position
    keyArray->Set(thread, 1, globalConst->GetHandledMonthString()); // 1 : 1 second position
    keyArray->Set(thread, 2, globalConst->GetHandledDayString()); // 2 : 2 third position
    keyArray->Set(thread, 3, globalConst->GetHandledHourString()); // 3 : 3 fourth position
    keyArray->Set(thread, 4, globalConst->GetHandledMinuteString()); // 4 : 4 fifth position
    keyArray->Set(thread, 5, globalConst->GetHandledSecondString()); // 5 : 5 sixth position
    uint32_t arrayLen = keyArray->GetLength();
    uint32_t arrIndex[] = {0, 2};
    uint32_t arrIndex2 = 1;
    JSMutableHandle<JSTaggedValue> key(thread, JSTaggedValue::Undefined());
    for (uint32_t i = 0; i < arrayLen; i++) {
        key.Update(keyArray->Get(thread, i));
        bool exists = std::count(std::begin(arrIndex), std::end(arrIndex), i) > 0;
        if (exists) {
            JSObject::SetProperty(thread, optionsObj, key, numicValue);
        } else if (i == arrIndex2) {
            JSObject::SetProperty(thread, optionsObj, key, longValue);
        } else {
            JSObject::SetProperty(thread, optionsObj, key, digitValue);
        }
    }
    JSObject::SetProperty(thread, optionsObj, timeZoneName, timeZoneNameValue);
    JSObject::SetProperty(thread, optionsObj, timeZone, timeZoneValue);
    return optionsObj.GetTaggedValue();
}

// DateTimeFormat_002
HWTEST_F_L0(BuiltinsDateTimeFormatTest, DateTimeFormat_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> localesString(factory->NewFromASCII("zh-CN"));
    auto jsObj = JSHandle<JSObject>(thread, JSDateTimeFormatForObj_002(thread));
    auto constructorResult = JSDateTimeFormatConstructor(thread, jsObj, localesString);
    auto formatResult = JSDateTimeFormatForObject(thread, constructorResult);
    auto dtResult = JSDateTime(thread, formatResult);
    JSHandle<EcmaString> resultStr(thread, dtResult);
    EXPECT_STREQ("2022年10月8日 UTC 上午12:00:00", EcmaStringAccessor(resultStr).ToCString().c_str());
}

// DateTimeFormat_003
HWTEST_F_L0(BuiltinsDateTimeFormatTest, DateTimeFormat_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> localesString(factory->NewFromASCII("zh-CN"));

    auto jsObj = JSHandle<JSObject>(thread, JSDateTimeFormatForObj_002(thread));
    auto constructorResult = JSDateTimeFormatConstructor(thread, jsObj, localesString);
    JSHandle<JSDateTimeFormat> jsDateTimeFormat = JSHandle<JSDateTimeFormat>(thread, constructorResult);
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDateTimeFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDateTimeFormat::FormatToParts(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, result);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 16U);
}
}  // namespace panda::test
