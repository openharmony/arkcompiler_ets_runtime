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

#include "ecmascript/builtins/builtins_locale.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_locale.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsLocaleTest : public BaseTestWithScope<true> {
};

// new locale( [ options ] )
HWTEST_F_L0(BuiltinsLocaleTest, LocaleConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetLocaleFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> languageKey = thread->GlobalConstants()->GetHandledLanguageString();
    JSHandle<JSTaggedValue> regionKey = thread->GlobalConstants()->GetHandledRegionString();
    JSHandle<JSTaggedValue> numericKey = thread->GlobalConstants()->GetHandledNumericString();
    JSHandle<JSTaggedValue> scriptKey = thread->GlobalConstants()->GetHandledScriptString();
    JSHandle<JSTaggedValue> languageValue(factory->NewFromASCII("cn"));
    JSHandle<JSTaggedValue> regionValue(factory->NewFromASCII("CN"));
    JSHandle<JSTaggedValue> scriptValue(factory->NewFromASCII("Chin"));
    JSHandle<JSTaggedValue> numericValue(thread, JSTaggedValue::True());
    JSHandle<JSTaggedValue> localeString(factory->NewFromASCII("zh-Hans-CN"));

    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, numericKey, numericValue);
    JSObject::SetProperty(thread, optionsObj, regionKey, regionValue);
    JSObject::SetProperty(thread, optionsObj, languageKey, languageValue);
    JSObject::SetProperty(thread, optionsObj, scriptKey, scriptValue);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localeString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsLocale::LocaleConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsJSLocale());
}

static JSTaggedValue JSLocaleCreateWithOptionTest(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetLocaleFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> languageKey = thread->GlobalConstants()->GetHandledLanguageString();
    JSHandle<JSTaggedValue> regionKey = thread->GlobalConstants()->GetHandledRegionString();
    JSHandle<JSTaggedValue> scriptKey = thread->GlobalConstants()->GetHandledScriptString();
    JSHandle<JSTaggedValue> languageValue(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> regionValue(factory->NewFromASCII("US"));
    JSHandle<JSTaggedValue> scriptValue(factory->NewFromASCII("Latn"));
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-Latn-US"));
    // set option(language, region, script)
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, languageKey, languageValue);
    JSObject::SetProperty(thread, optionsObj, regionKey, regionValue);
    JSObject::SetProperty(thread, optionsObj, scriptKey, scriptValue);

    JSHandle<JSTaggedValue> calendarValue(factory->NewFromASCII("chinese"));
    JSHandle<JSTaggedValue> calendarKey = thread->GlobalConstants()->GetHandledCalendarString();
    JSObject::SetProperty(thread, optionsObj, calendarKey, calendarValue); // test chinese calendar

    JSHandle<JSTaggedValue> hourCycleKey = thread->GlobalConstants()->GetHandledHourCycleString();
    JSHandle<JSTaggedValue> hourCycleValue(factory->NewFromASCII("h24"));
    JSObject::SetProperty(thread, optionsObj, hourCycleKey, hourCycleValue); // test h24

    JSHandle<JSTaggedValue> numericKey = thread->GlobalConstants()->GetHandledNumericString();
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("true"));
    JSObject::SetProperty(thread, optionsObj, numericKey, numericValue); // test true

    JSHandle<JSTaggedValue> numberingSystemKey = thread->GlobalConstants()->GetHandledNumberingSystemString();
    JSHandle<JSTaggedValue> numberingSystemValue(factory->NewFromASCII("mong"));
    JSObject::SetProperty(thread, optionsObj, numberingSystemKey, numberingSystemValue); // test mong

    JSHandle<JSTaggedValue> collationKey = thread->GlobalConstants()->GetHandledCollationString();
    JSHandle<JSTaggedValue> collationValue(factory->NewFromASCII("compat"));
    JSObject::SetProperty(thread, optionsObj, collationKey, collationValue); // test compat

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsLocale::LocaleConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

enum class AlgorithmType {
    TO_STRING,
    GET_BASE_NAME,
    GET_HOUR_CYCLE,
    GET_CALENDAR,
    GET_CASE_FIRST,
    GET_COLLATION,
    GET_NUMERIC,
    GET_NUMBERING_SYSTEM,
    GET_LANGUAGE,
    GET_SCRIPT,
    GET_REGION,
    MAXI_MIZE,
    MINI_MIZE,
};

JSTaggedValue AlgorithmOther(EcmaRuntimeCallInfo *ecmaRuntimeCallInfo, AlgorithmType type)
{
    switch (type) {
        case AlgorithmType::TO_STRING:
            return BuiltinsLocale::ToString(ecmaRuntimeCallInfo);
        case AlgorithmType::MAXI_MIZE:
            return BuiltinsLocale::Maximize(ecmaRuntimeCallInfo);
        case AlgorithmType::MINI_MIZE:
            return BuiltinsLocale::Minimize(ecmaRuntimeCallInfo);
        default:
            return JSTaggedValue();
    }
}

JSTaggedValue LocaleAlgorithm(JSThread *thread, JSTaggedValue thisArg, std::vector<JSTaggedValue>& args,
    int32_t argLen, AlgorithmType type)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), argLen);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisArg);
    for (size_t i = 0; i < args.size(); i++) {
        ecmaRuntimeCallInfo->SetCallArg(i, args[i]);
    }
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result;
    switch (type) {
        case AlgorithmType::GET_BASE_NAME:
            result = BuiltinsLocale::GetBaseName(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_HOUR_CYCLE:
            result = BuiltinsLocale::GetHourCycle(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_CALENDAR:
            result = BuiltinsLocale::GetCalendar(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_CASE_FIRST:
            result = BuiltinsLocale::GetCaseFirst(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_COLLATION:
            result = BuiltinsLocale::GetCollation(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_NUMERIC:
            result = BuiltinsLocale::GetNumeric(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_NUMBERING_SYSTEM:
            result = BuiltinsLocale::GetNumberingSystem(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_LANGUAGE:
            result = BuiltinsLocale::GetLanguage(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_SCRIPT:
            result = BuiltinsLocale::GetScript(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::GET_REGION:
            result = BuiltinsLocale::GetRegion(ecmaRuntimeCallInfo);
            break;
        default:
            result = AlgorithmOther(ecmaRuntimeCallInfo, type);
            break;
    }
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

HWTEST_F_L0(BuiltinsLocaleTest, ToString)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionTest(thread));
    std::vector<JSTaggedValue> args{};
    auto result = LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4, AlgorithmType::TO_STRING); // 4 : arg len

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    JSHandle<EcmaString> resultStr(
        factory->NewFromASCII("en-Latn-US-u-ca-chinese-co-compat-hc-h24-kn-nu-mong"));
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleEcmaStr, resultStr), 0);
}

HWTEST_F_L0(BuiltinsLocaleTest, GetBaseName)
{
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionTest(thread));

    std::vector<JSTaggedValue> args{};
    auto result =
        LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4, AlgorithmType::GET_BASE_NAME);  // 4 : arg len

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("en-Latn-US", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

HWTEST_F_L0(BuiltinsLocaleTest, GetHourCycle)
{
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionTest(thread));

    std::vector<JSTaggedValue> args{};
    auto result =
        LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4, AlgorithmType::GET_HOUR_CYCLE);  // 4 : arg len

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("h24", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

HWTEST_F_L0(BuiltinsLocaleTest, GetCalendar)
{
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionTest(thread));
    std::vector<JSTaggedValue> args{};
    auto result =
        LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4, AlgorithmType::GET_CALENDAR);  // 4 : arg len

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("chinese", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

HWTEST_F_L0(BuiltinsLocaleTest, GetCaseFirst)
{
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionTest(thread));

    std::vector<JSTaggedValue> args{};
    auto result =
        LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4, AlgorithmType::GET_CASE_FIRST);  // 4 : arg len

    EXPECT_TRUE(result.IsUndefined());
}

HWTEST_F_L0(BuiltinsLocaleTest, GetCollation)
{
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionTest(thread));

    std::vector<JSTaggedValue> args{};
    auto result =
        LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4, AlgorithmType::GET_COLLATION);  // 4 : arg len

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("compat", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

HWTEST_F_L0(BuiltinsLocaleTest, GetNumeric)
{
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionTest(thread));

    std::vector<JSTaggedValue> args{};
    auto result =
        LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4, AlgorithmType::GET_NUMERIC);  // 4 : arg len

    EXPECT_EQ(result.GetRawData(), JSTaggedValue::True().GetRawData());
}

HWTEST_F_L0(BuiltinsLocaleTest, GetNumberingSystem)
{
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionTest(thread));

    std::vector<JSTaggedValue> args{};
    auto result = LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4,
                                  AlgorithmType::GET_NUMBERING_SYSTEM);  // 4 : arg len

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("mong", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

HWTEST_F_L0(BuiltinsLocaleTest, GetLanguage)
{
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionTest(thread));

    std::vector<JSTaggedValue> args{};
    auto result =
        LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4, AlgorithmType::GET_LANGUAGE);  // 4 : arg len

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("en", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

HWTEST_F_L0(BuiltinsLocaleTest, GetScript)
{
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionTest(thread));

    std::vector<JSTaggedValue> args{};
    auto result = LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4, AlgorithmType::GET_SCRIPT); // 4 : arg len

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("Latn", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

HWTEST_F_L0(BuiltinsLocaleTest, GetRegion)
{
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionTest(thread));

    std::vector<JSTaggedValue> args{};
    auto result = LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4, AlgorithmType::GET_REGION); // 4 : arg len

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("US", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

static JSTaggedValue JSLocaleCreateWithOptionsTagsTest(JSThread *thread, JSHandle<JSTaggedValue> &value)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetLocaleFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> languageKey = thread->GlobalConstants()->GetHandledLanguageString();
    JSHandle<JSTaggedValue> languageValue = JSHandle<JSTaggedValue>(value);
    JSHandle<EcmaString> locale = factory->NewFromASCII("zh");
    // set option(language)
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, languageKey, languageValue);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsLocale::LocaleConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsJSLocale());
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

HWTEST_F_L0(BuiltinsLocaleTest, Maximize_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // set language,then call Maximize function get language,script and region
    JSHandle<JSTaggedValue> languageValue(factory->NewFromASCII("zh"));
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionsTagsTest(thread, languageValue));

    std::vector<JSTaggedValue> args{};
    auto resultObj =
        LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4, AlgorithmType::MAXI_MIZE);  // 4 : arg len

    JSHandle<JSLocale> resultLocale(thread, resultObj);
    auto result =
        LocaleAlgorithm(thread, resultLocale.GetTaggedValue(), args, 4, AlgorithmType::GET_BASE_NAME);  // 4 : arg len

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("zh-Hans-CN", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

HWTEST_F_L0(BuiltinsLocaleTest, Maximize_002)
{
    // set language,script,region and numeric and it's maximized
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionTest(thread));

    std::vector<JSTaggedValue> args{};
    auto resultObj =
        LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4, AlgorithmType::MAXI_MIZE);  // 4 : arg len
    JSHandle<JSLocale> resultLocale(thread, resultObj);
    auto result =
        LocaleAlgorithm(thread, resultLocale.GetTaggedValue(), args, 4, AlgorithmType::GET_BASE_NAME);  // 4 : arg len

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("en-Latn-US", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

HWTEST_F_L0(BuiltinsLocaleTest, Minimize_001)
{
    // set language, script, region, and it's maximized,then call Minimize function get language
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionTest(thread));

    std::vector<JSTaggedValue> args{};
    auto resultObj =
        LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4, AlgorithmType::MINI_MIZE);  // 4 : arg len

    JSHandle<JSLocale> resultLocale(thread, resultObj);
    auto result =
        LocaleAlgorithm(thread, resultLocale.GetTaggedValue(), args, 4, AlgorithmType::GET_BASE_NAME);  // 4 : arg len

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("en", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

HWTEST_F_L0(BuiltinsLocaleTest, Minimize_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // set language and it's minimized
    JSHandle<JSTaggedValue> languageValue(factory->NewFromASCII("zh"));
    JSHandle<JSLocale> jsLocale = JSHandle<JSLocale>(thread, JSLocaleCreateWithOptionsTagsTest(thread, languageValue));

    std::vector<JSTaggedValue> args{};
    auto resultObj =
        LocaleAlgorithm(thread, jsLocale.GetTaggedValue(), args, 4, AlgorithmType::MINI_MIZE);  // 4 : arg len

    JSHandle<JSLocale> resultLocale(thread, resultObj);
    auto result =
        LocaleAlgorithm(thread, resultLocale.GetTaggedValue(), args, 4, AlgorithmType::TO_STRING);  // 4 : arg len

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("zh", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

// test NormalizeKeywordValue(kf is not setted "yes")
HWTEST_F_L0(BuiltinsLocaleTest, NormalizeKeywordValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetLocaleFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> caseFirstKey = thread->GlobalConstants()->GetHandledCaseFirstString();
    JSHandle<JSTaggedValue> caseFirstValue(factory->NewFromASCII("false"));
    JSHandle<EcmaString> locale = factory->NewFromASCII("en-US");

    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, caseFirstKey, caseFirstValue);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsLocale::LocaleConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSLocale> jsInitLocale(thread, result);
    JSTaggedValue keyWords = JSLocale::NormalizeKeywordValue(thread, jsInitLocale, "kf");
    EXPECT_STREQ("false", EcmaStringAccessor(keyWords).ToCString().c_str());
}
}  // namespace panda::test
