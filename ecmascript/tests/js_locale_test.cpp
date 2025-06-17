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

#include "ecmascript/js_locale.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_number_format.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/global_env.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSLocaleTest : public BaseTestWithScope<true> {
};

void CreateLanguageIterator(std::vector<std::string>& languageTemp)
{
    languageTemp.push_back("zh-Hans-Cn");
    languageTemp.push_back("ko-kore-kr");
    languageTemp.push_back("fr-FR");
    languageTemp.push_back("en-Latn-US");
    languageTemp.push_back("ja-JP-u-ca-japanese");
    languageTemp.push_back("ar-EG");
}

/**
 * @tc.name: JSIntlIteratorTest
 * @tc.desc: Construct an iterator of JSIntl and then traverse the iterator to compare whether the variable
 *           at each position is equal to the setting.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, JSIntlIteratorTest)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    std::vector<std::string> languageVector;
    CreateLanguageIterator(languageVector);
    uint32_t arrayDataLength = languageVector.size();
    JSHandle<TaggedArray> arrayData = factory->NewTaggedArray(arrayDataLength);

    for (uint32_t i = 0; i < arrayDataLength; i++) {
        JSHandle<JSTaggedValue> languageStr(factory->NewFromASCII(languageVector[i].c_str()));
        arrayData->Set(thread, i, languageStr);
    }
    // construct a JSIntlIterator object
    JSIntlIterator jsIntlIterator(thread, arrayData, arrayDataLength);
    EXPECT_TRUE(jsIntlIterator.hasNext());
    // call "next" function to traverse the container
    for (uint32_t i = 0; i < arrayDataLength; i++) {
        EXPECT_TRUE(jsIntlIterator.next() != nullptr);
        EXPECT_STREQ(jsIntlIterator[i].c_str(), languageVector[i].c_str());
    }
    EXPECT_FALSE(jsIntlIterator.hasNext());
}

/**
 * @tc.name: IsPrivateSubTag
 * @tc.desc: Check whether the string is private subtag through "IsPrivateSubTag" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, IsPrivateSubTag)
{
    std::string result = "en-GB-oed";
    EXPECT_FALSE(JSLocale::IsPrivateSubTag(result, result.length()));

    result = "i-ami";
    EXPECT_TRUE(JSLocale::IsPrivateSubTag(result, result.length()));

    result = "x-default";
    EXPECT_TRUE(JSLocale::IsPrivateSubTag(result, result.length()));
}

/**
 * @tc.name: GetIcuField
 * @tc.desc: Call "NewJSIntlIcuData" function Set locale IcuField,check whether the locale IcuField through
 *           "getBaseName" function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, GetIcuField)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> ctor = env->GetLocaleFunction();
    JSHandle<JSLocale> locale =
        JSHandle<JSLocale>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    // call "NewJSIntlIcuData" function Set IcuField
    icu::Locale icuLocale("zh", "Hans", "Cn");
    factory->NewJSIntlIcuData(locale, icuLocale, JSLocale::FreeIcuLocale);

    // call "GetIcuLocale" function Get IcuField
    icu::Locale *result = locale->GetIcuLocale(thread);
    EXPECT_STREQ(result->getBaseName(), "zh_Hans_CN");
}

/**
 * @tc.name: IsValidTimeZoneName
 * @tc.desc: Call "IsValidTimeZoneName" function check whether the TimeZone is valid.if TimeZone include "GMT-Time"
 *           return true otherwise, return false.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, IsValidTimeZoneName)
{
    icu::UnicodeString stringID1("GMT-8:00");
    icu::TimeZone *timeZone = icu::TimeZone::createTimeZone(stringID1);
    EXPECT_TRUE(JSLocale::IsValidTimeZoneName(*timeZone));
    delete timeZone;

    icu::UnicodeString stringID2("Etc/Unknown");
    timeZone = icu::TimeZone::createTimeZone(stringID2);
    EXPECT_FALSE(JSLocale::IsValidTimeZoneName(*timeZone));
    delete timeZone;
}

/**
 * @tc.name: PutElement
 * @tc.desc: Put elements in empty JSArray and return the JSArray.call "GetProperty" function to get the value and
 *           check whether the value is consistent with the value of the put.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, PutElement)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSArray> jsArray = factory->NewJSArray();
    JSHandle<JSTaggedValue> typeString = globalConst->GetHandledTypeString();
    JSHandle<JSTaggedValue> valueString = globalConst->GetHandledValueString();
    JSHandle<JSTaggedValue> fieldTypeString = globalConst->GetHandledUnitString();
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(static_cast<int>(11)));

    int index = 1;
    JSHandle<JSObject> recordObj = JSLocale::PutElement(thread, index, jsArray, fieldTypeString, value);
    EXPECT_EQ(JSTaggedValue::SameValue(thread,
              JSObject::GetProperty(thread, recordObj, typeString).GetValue(), fieldTypeString), true);
    EXPECT_EQ(JSObject::GetProperty(thread, recordObj, valueString).GetValue()->GetInt(), 11);

    JSHandle<JSTaggedValue> indexKey(factory->NewFromASCII("1"));
    EXPECT_TRUE(JSObject::GetProperty(thread, JSHandle<JSObject>(jsArray), indexKey).GetValue()->IsECMAObject());
}

/**
 * @tc.name: GetNumberingSystem
 * @tc.desc: Call "GetNumberingSystem" function get the script from the ICU Locale.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, GetNumberingSystem)
{
    icu::Locale icuLocale1("en", "US");
    std::string numberingSystem = JSLocale::GetNumberingSystem(icuLocale1);
    EXPECT_STREQ("latn", numberingSystem.c_str());

    icu::Locale icuLocale2("zh", "Hans", "CN", "collation=phonebk;numbers=hans");
    numberingSystem = JSLocale::GetNumberingSystem(icuLocale2);
    EXPECT_STREQ("hans", numberingSystem.c_str());
}

/**
 * @tc.name: GetNumberFieldType
 * @tc.desc: Call "GetNumberFieldType" function get Number Field type.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, GetNumberFieldType)
{
    auto globalConst = thread->GlobalConstants();
    int32_t fieldId = 0; // UNUM_INTEGER_FIELD

    JSTaggedValue x(0.0f / 0.0f); // Nan
    JSHandle<JSTaggedValue> nanString = globalConst->GetHandledNanString();
    JSHandle<JSTaggedValue> fieldTypeString = JSLocale::GetNumberFieldType(thread, x, fieldId);
    EXPECT_EQ(JSTaggedValue::SameValue(thread, fieldTypeString, nanString), true);

    JSTaggedValue y(-10); // integer(sign bit)
    JSHandle<JSTaggedValue> integerString = globalConst->GetHandledIntegerString();
    fieldTypeString = JSLocale::GetNumberFieldType(thread, y, fieldId);
    EXPECT_EQ(JSTaggedValue::SameValue(thread, fieldTypeString, integerString), true);

    fieldId = 10; // UNUM_SIGN_FIELD
    JSHandle<JSTaggedValue> minusSignString = globalConst->GetHandledMinusSignString();
    fieldTypeString = JSLocale::GetNumberFieldType(thread, y, fieldId);
    EXPECT_EQ(JSTaggedValue::SameValue(thread, fieldTypeString, minusSignString), true);

    JSTaggedValue z(10); // no sign bit
    JSHandle<JSTaggedValue> plusSignString = globalConst->GetHandledPlusSignString();
    fieldTypeString = JSLocale::GetNumberFieldType(thread, z, fieldId);
    EXPECT_EQ(JSTaggedValue::SameValue(thread, fieldTypeString, plusSignString), true);
}

/**
 * @tc.name: ApplyOptionsToTag
 * @tc.desc: Call "ApplyOptionsToTag" function parse information in option into tag string.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, ApplyOptionsToTag)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    TagElements tagElements;
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<EcmaString> languageTag = intl::LocaleHelper::DefaultLocale(thread);

    JSHandle<JSTaggedValue> languageKey = thread->GlobalConstants()->GetHandledLanguageString();
    JSHandle<JSTaggedValue> regionKey = thread->GlobalConstants()->GetHandledRegionString();
    JSHandle<JSTaggedValue> scriptKey = thread->GlobalConstants()->GetHandledScriptString();
    JSHandle<JSTaggedValue> languageValue(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> regionValue(factory->NewFromASCII("US"));
    JSHandle<JSTaggedValue> scriptValue(factory->NewFromASCII("Latn"));

    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, languageKey, languageValue);
    JSObject::SetProperty(thread, optionsObj, regionKey, regionValue);
    JSObject::SetProperty(thread, optionsObj, scriptKey, scriptValue);
    bool result = JSLocale::ApplyOptionsToTag(thread, languageTag, optionsObj, tagElements);
    EXPECT_TRUE(result);
    EXPECT_EQ(tagElements.language, languageValue);
    EXPECT_EQ(tagElements.script, scriptValue);
    EXPECT_EQ(tagElements.region, regionValue);
    // fault script
    JSHandle<JSTaggedValue> scriptValue1(factory->NewFromASCII(""));
    JSObject::SetProperty(thread, optionsObj, scriptKey, scriptValue1);
    result = JSLocale::ApplyOptionsToTag(thread, languageTag, optionsObj, tagElements);
    EXPECT_FALSE(result);
}

/**
 * @tc.name: ConstructLocaleList
 * @tc.desc: Get LocaleList numbers through "ConstructLocaleList" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, ConstructLocaleList)
{
    std::vector<std::string> availableLocales = {"zh-Hans-CN", "de-ID", "en-US", "en-GB"};
    JSHandle<TaggedArray> localeArr = JSLocale::ConstructLocaleList(thread, availableLocales);
    EXPECT_EQ(localeArr->GetLength(), 4U); // 4 : 4 Number of locales
}

/**
 * @tc.name: SetNumberFormatDigitOptions
 * @tc.desc: Call "SetNumberFormatDigitOptions" function parse information in option into attributes
 *           of the JSNumberFormat.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, SetNumberFormatDigitOptions_Significant)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> numberFormatObj = env->GetNumberFormatFunction();

    JSHandle<JSTaggedValue> mnidKey = thread->GlobalConstants()->GetHandledMinimumIntegerDigitsString();
    JSHandle<JSTaggedValue> mnsdKey = thread->GlobalConstants()->GetHandledMinimumSignificantDigitsString();
    JSHandle<JSTaggedValue> mxsdKey = thread->GlobalConstants()->GetHandledMaximumSignificantDigitsString();
    JSHandle<JSTaggedValue> mnidValue(thread, JSTaggedValue(10));
    JSHandle<JSTaggedValue> maxFraValue(thread, JSTaggedValue(11));
    JSHandle<JSTaggedValue> minSignValue(thread, JSTaggedValue(12));

    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, mnidKey, mnidValue);
    JSObject::SetProperty(thread, optionsObj, mnsdKey, maxFraValue);
    JSObject::SetProperty(thread, optionsObj, mxsdKey, minSignValue);
    JSHandle<JSNumberFormat> jsNumberFormat = JSHandle<JSNumberFormat>::Cast(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(numberFormatObj), numberFormatObj));

    JSLocale::SetNumberFormatDigitOptions(thread, jsNumberFormat, JSHandle<JSTaggedValue>(optionsObj),
                                                   1, 1, NotationOption::COMPACT);
    EXPECT_EQ(jsNumberFormat->GetMinimumSignificantDigits(thread).GetInt(), 11);
    EXPECT_EQ(jsNumberFormat->GetMaximumSignificantDigits(thread).GetInt(), 12);
    EXPECT_EQ(jsNumberFormat->GetMinimumIntegerDigits(thread).GetInt(), 10);
    EXPECT_EQ(jsNumberFormat->GetRoundingType(), RoundingType::SIGNIFICANTDIGITS);
}

HWTEST_F_L0(JSLocaleTest, SetNumberFormatDigitOptions_Fraction)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> pluralRulesObj = env->GetPluralRulesFunction();

    JSHandle<JSTaggedValue> mnidKey = thread->GlobalConstants()->GetHandledMinimumIntegerDigitsString();
    JSHandle<JSTaggedValue> mnidValue(thread, JSTaggedValue(10));

    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, mnidKey, mnidValue);
    JSHandle<JSPluralRules> jsPluralRules = JSHandle<JSPluralRules>::Cast(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(pluralRulesObj), pluralRulesObj));

    JSLocale::SetNumberFormatDigitOptions(thread, jsPluralRules, JSHandle<JSTaggedValue>(optionsObj),
                                                  10, 13, NotationOption::EXCEPTION);
    EXPECT_EQ(jsPluralRules->GetMinimumFractionDigits(thread).GetInt(), 10);
    EXPECT_EQ(jsPluralRules->GetMaximumFractionDigits(thread).GetInt(), 13);
    EXPECT_EQ(jsPluralRules->GetMinimumIntegerDigits(thread).GetInt(), 10);
    EXPECT_EQ(jsPluralRules->GetRoundingType(), RoundingType::FRACTIONDIGITS);
}

/**
 * @tc.name: CheckLocales
 * @tc.desc: Call "CheckLocales" function check wether language is correct from locale libraries obtained
 *           from different ways.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, CheckLocales)
{
    bool res = false;
    const char *path = JSCollator::uIcuDataColl.c_str();
    // default language
    bool result = JSLocale::CheckLocales("en", nullptr, path, res);
    EXPECT_TRUE(result);
    // fault language
    result = JSLocale::CheckLocales("e", nullptr, path, res);
    EXPECT_FALSE(result);
    // find language in calendar
    result = JSLocale::CheckLocales("en-US", "calendar", nullptr, res);
    EXPECT_TRUE(result);
    // find language in NumberElements
    result = JSLocale::CheckLocales("en-US", "NumberElements", nullptr, res);
    EXPECT_TRUE(result);
}

/**
 * @tc.name: UnicodeExtensionValue
 * @tc.desc: Call "UnicodeExtensionValue" function get subtag after key in Extension.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, UnicodeExtensionValue)
{
    // extension has one "-"
    std::string result = JSLocale::UnicodeExtensionValue("-ca=chinese", "ca");
    EXPECT_STREQ(result.c_str(), "undefined");
    // extension has one "-" and key value is full
    result = JSLocale::UnicodeExtensionValue("-ca", "ca");
    EXPECT_STREQ(result.c_str(), "");
    // extension has two "-"
    result = JSLocale::UnicodeExtensionValue("-ca-chinese", "ca");
    EXPECT_STREQ(result.c_str(), "chinese");

    result = JSLocale::UnicodeExtensionValue("-ca-chinese-co-compat", "co");
    EXPECT_STREQ(result.c_str(), "compat");

    result = JSLocale::UnicodeExtensionValue("-ca-kn-true", "kn");
    EXPECT_STREQ(result.c_str(), "true");
}

/**
 * @tc.name: IsWellCalendar
 * @tc.desc: Call "IsWellCalendar" function judge whether the calendar is well from locale.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, IsWellCalendar)
{
    EXPECT_TRUE(JSLocale::IsWellCalendar("ar-EG", "islamic"));
    EXPECT_TRUE(JSLocale::IsWellCalendar("ar-EG", "coptic"));
    EXPECT_TRUE(JSLocale::IsWellCalendar("zh-CN", "chinese"));
    EXPECT_TRUE(JSLocale::IsWellCalendar("en-US", "gregory"));

    EXPECT_FALSE(JSLocale::IsWellCalendar("zh-CN", "English"));
}

/**
 * @tc.name: IsWellCollation
 * @tc.desc: Call "IsWellCollation" function judge whether the collation is well from locale.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, IsWellCollation)
{
    EXPECT_TRUE(JSLocale::IsWellCollation("ar-EG", "compat"));

    EXPECT_FALSE(JSLocale::IsWellCollation("ar-EG", "stroke"));
    EXPECT_FALSE(JSLocale::IsWellCollation("ar-EG", "pinyin"));
    EXPECT_FALSE(JSLocale::IsWellCollation("ar-EG", "phonebk"));
    EXPECT_FALSE(JSLocale::IsWellCollation("ar-EG", "search"));
    EXPECT_FALSE(JSLocale::IsWellCollation("ar-EG", "standard"));
}

/**
 * @tc.name: IsWellNumberingSystem
 * @tc.desc: Call "IsWellNumberingSystem" function judge whether the script is well.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, IsWellNumberingSystem)
{
    EXPECT_FALSE(JSLocale::IsWellNumberingSystem("finance"));
    EXPECT_FALSE(JSLocale::IsWellNumberingSystem("native"));
    EXPECT_FALSE(JSLocale::IsWellNumberingSystem("traditio"));

    EXPECT_TRUE(JSLocale::IsWellNumberingSystem("hans"));
    EXPECT_TRUE(JSLocale::IsWellNumberingSystem("deva"));
    EXPECT_TRUE(JSLocale::IsWellNumberingSystem("greklow"));
}

/**
 * @tc.name: DefaultNumberOption
 * @tc.desc: Call "DefaultNumberOption" function get default number from value.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, DefaultNumberOption)
{
    JSHandle<JSTaggedValue> value1(thread, JSTaggedValue(static_cast<double>(4.99)));
    int result = JSLocale::DefaultNumberOption(thread, value1, 1, 5, 1);
    EXPECT_EQ(result, 4);
    JSHandle<JSTaggedValue> value2(thread, JSTaggedValue::Undefined());
    result = JSLocale::DefaultNumberOption(thread, value2, 1, 5, 1);
    EXPECT_EQ(result, 1);
}

JSHandle<JSObject> GetOptionCommon(JSThread *thread, JSHandle<JSTaggedValue>& languageProperty,
    JSHandle<JSTaggedValue>& languageValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    // Set key value
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, languageProperty, languageValue);
    return optionsObj;
}
/**
 * @tc.name: GetOptionOfString
 * @tc.desc: Call "GetOptionOfString" function get the string from Option value.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, GetOptionOfString)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> languageProperty = thread->GlobalConstants()->GetHandledLanguageString();
    JSHandle<JSTaggedValue> regionProperty = thread->GlobalConstants()->GetHandledRegionString();
    JSHandle<JSTaggedValue> regionValue(factory->NewFromASCII("CN"));
    JSHandle<JSTaggedValue> languageValue(factory->NewFromASCII("zh"));
    // Set key value
    JSHandle<JSObject> optionsObj = GetOptionCommon(thread, languageProperty, languageValue);
    JSObject::SetProperty(thread, optionsObj, regionProperty, regionValue);
    std::vector<std::string> stringValues = {"zh", "Hans", "CN"};
    std::string optionValue;
    // Get language
    bool result = JSLocale::GetOptionOfString(thread, optionsObj, languageProperty, stringValues, &optionValue);
    EXPECT_TRUE(result);
    EXPECT_STREQ(optionValue.c_str(), "zh");
    // Get region
    result = JSLocale::GetOptionOfString(thread, optionsObj, regionProperty, stringValues, &optionValue);
    EXPECT_TRUE(result);
    EXPECT_STREQ(optionValue.c_str(), "CN");
}

/**
 * @tc.name: GetOption
 * @tc.desc: Call "GetOption" function get value of the key from Option.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, GetOption)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> languageProperty = thread->GlobalConstants()->GetHandledLanguageString();
    JSHandle<JSTaggedValue> regionProperty = thread->GlobalConstants()->GetHandledRegionString();
    JSHandle<JSTaggedValue> regionValue(factory->NewFromASCII("CN"));
    JSHandle<JSTaggedValue> languageValue(factory->NewFromASCII("zh"));
    // Set key value
    JSHandle<JSObject> optionsObj = GetOptionCommon(thread, languageProperty, languageValue);
    JSHandle<TaggedArray> stringValues = factory->NewTaggedArray(3);
    stringValues->Set(thread, 0, languageValue);
    stringValues->Set(thread, 1, regionValue);
    JSHandle<JSTaggedValue> arrayValue(stringValues);
    JSHandle<JSTaggedValue> fallback(thread, JSTaggedValue::Undefined());

    JSHandle<JSTaggedValue> optionValue =
        JSLocale::GetOption(thread, optionsObj, languageProperty, OptionType::STRING, arrayValue, fallback);
    EXPECT_EQ(JSTaggedValue::SameValue(thread, optionValue, languageValue), true);

    optionValue = JSLocale::GetOption(thread, optionsObj, regionProperty, OptionType::STRING, arrayValue, fallback);
    EXPECT_EQ(JSTaggedValue::SameValue(thread, optionValue, fallback), true);
}

/**
 * @tc.name: GetOptionOfBool
 * @tc.desc: Call "GetOptionOfBool" function get the bool value from Option.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, GetOptionOfBool)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> numericProperty = thread->GlobalConstants()->GetHandledNumericString();
    JSHandle<JSTaggedValue> numericValue(thread, JSTaggedValue::True());
    // Set key value
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, numericProperty, numericValue);
    bool res;
    // Test correct keyValue
    EXPECT_TRUE(JSLocale::GetOptionOfBool(thread, optionsObj, numericProperty, false, &res));
    EXPECT_TRUE(res);

    JSHandle<JSTaggedValue> numericValue1(thread, JSTaggedValue(0));
    JSObject::SetProperty(thread, optionsObj, numericProperty, numericValue1);
    // Test fault keyValue
    EXPECT_TRUE(JSLocale::GetOptionOfBool(thread, optionsObj, numericProperty, false, &res));
    EXPECT_FALSE(res);
}

ResolvedLocale ResolveLocaleCommon(JSThread *thread, JSHandle<TaggedArray> &availableLocales,
                                   JSHandle<TaggedArray> &requestedLocales,
                                   std::set<std::string> &relevantExtensionKeys, JSHandle<JSTaggedValue> &testLocale1)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    ResolvedLocale result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                                    LocaleMatcherOption::BEST_FIT, relevantExtensionKeys);
    EXPECT_STREQ("en-US", result.locale.c_str());  // default locale
    // availableLocales and requestLocales is not empty
    std::vector<std::string> availableStringLocales =
        intl::LocaleHelper::GetAvailableLocales(thread, "calendar", nullptr);
    availableLocales = JSLocale::ConstructLocaleList(thread, availableStringLocales);
    requestedLocales = factory->NewTaggedArray(1);
    // test locale1
    requestedLocales->Set(thread, 0, testLocale1);
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales, LocaleMatcherOption::BEST_FIT,
                                     relevantExtensionKeys);
    return result;
}

/**
 * @tc.name: ResolveLocale
 * @tc.desc: Resolve Locale and return from available locale through "ResolveLocale" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(JSLocaleTest, ResolveLocale_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> availableLocales = factory->EmptyArray();
    JSHandle<TaggedArray> requestedLocales = factory->EmptyArray();
    std::set<std::string> relevantExtensionKeys = {"co", "ca"};
    JSHandle<JSTaggedValue> testLocale1(factory->NewFromASCII("id-u-co-pinyin-ca-gregory-de-ID"));
    JSHandle<JSTaggedValue> testLocale2(factory->NewFromASCII("en-Latn-US-u-co-phonebk-ca-ethioaa"));
    // availableLocales and requestLocales is empty
    auto result = ResolveLocaleCommon(thread, availableLocales, requestedLocales, relevantExtensionKeys, testLocale1);
    EXPECT_STREQ("id-u-ca-gregory-co-pinyin-de-id", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                                    LocaleMatcherOption::LOOKUP, relevantExtensionKeys);
    EXPECT_STREQ("id-u-ca-gregory-co-pinyin-de-id", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                                    LocaleMatcherOption::EXCEPTION, relevantExtensionKeys);
    EXPECT_STREQ("id-u-ca-gregory-co-pinyin-de-id", result.locale.c_str());
    // test locale2
    requestedLocales->Set(thread, 0, testLocale2);
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                     LocaleMatcherOption::BEST_FIT, relevantExtensionKeys);
    EXPECT_STREQ("en-u-ca-ethioaa-co-phonebk", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                     LocaleMatcherOption::LOOKUP, relevantExtensionKeys);
    EXPECT_STREQ("en-u-ca-ethioaa-co-phonebk", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                     LocaleMatcherOption::EXCEPTION, relevantExtensionKeys);
    EXPECT_STREQ("en-u-ca-ethioaa-co-phonebk", result.locale.c_str());
}

HWTEST_F_L0(JSLocaleTest, ResolveLocale_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<TaggedArray> availableLocales = factory->EmptyArray();
    JSHandle<TaggedArray> requestedLocales = factory->EmptyArray();
    std::set<std::string> relevantExtensionKeys = {"hc", "lb", "kn", "kf"};
    JSHandle<JSTaggedValue> testLocale1(factory->NewFromASCII("id-u-kn-false-kf-yes-de-ID"));
    JSHandle<JSTaggedValue> testLocale2(factory->NewFromASCII("en-US-u-hc-h24-lb-strict"));
    // availableLocales and requestLocales is empty
    auto result = ResolveLocaleCommon(thread, availableLocales, requestedLocales, relevantExtensionKeys, testLocale1);
    EXPECT_STREQ("id-u-de-id-kf-kn-false", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                                    LocaleMatcherOption::LOOKUP, relevantExtensionKeys);
    EXPECT_STREQ("id-u-de-id-kf-kn-false", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                                    LocaleMatcherOption::EXCEPTION, relevantExtensionKeys);
    EXPECT_STREQ("id-u-de-id-kf-kn-false", result.locale.c_str());
    // test locale2
    requestedLocales->Set(thread, 0, testLocale2);
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                     LocaleMatcherOption::BEST_FIT, relevantExtensionKeys);
    EXPECT_STREQ("en-US-u-hc-h24-lb-strict", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                     LocaleMatcherOption::LOOKUP, relevantExtensionKeys);
    EXPECT_STREQ("en-US-u-hc-h24-lb-strict", result.locale.c_str());
    result = JSLocale::ResolveLocale(thread, availableLocales, requestedLocales,
                                     LocaleMatcherOption::EXCEPTION, relevantExtensionKeys);
    EXPECT_STREQ("en-US-u-hc-h24-lb-strict", result.locale.c_str());
}
}  // namespace panda::test
