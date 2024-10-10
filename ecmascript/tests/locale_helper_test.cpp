/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/intl/locale_helper.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;
using LocaleHelper = panda::ecmascript::intl::LocaleHelper;

namespace panda::test {
class LocaleHelperTest : public BaseTestWithScope<true> {
};

/**
 * @tc.name: UStringToString
 * @tc.desc: Call "UStringToString" function Convert UnicodeString to string(Utf16).
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(LocaleHelperTest, UStringToString)
{
    icu::UnicodeString unicodeString1("GMT-12:00"); // times
    JSHandle<EcmaString> ecmaString = LocaleHelper::UStringToString(thread, unicodeString1);
    EXPECT_STREQ("GMT-12:00", EcmaStringAccessor(ecmaString).ToCString().c_str());

    icu::UnicodeString unicodeString2("周日16:00:00–周五23:00:00"); // date
    ecmaString = LocaleHelper::UStringToString(thread, unicodeString2);
    EXPECT_STREQ("周日16:00:00–周五23:00:00", EcmaStringAccessor(ecmaString).ToCString().c_str());

    icu::UnicodeString unicodeString3("$654K"); // money
    ecmaString = LocaleHelper::UStringToString(thread, unicodeString3);
    EXPECT_STREQ("$654K", EcmaStringAccessor(ecmaString).ToCString().c_str());

    icu::UnicodeString unicodeString4("1 minute ago"); // moment
    ecmaString = LocaleHelper::UStringToString(thread, unicodeString4, 0, 2);
    EXPECT_STREQ("1 ", EcmaStringAccessor(ecmaString).ToCString().c_str());
}

/**
 * @tc.name: CanonicalizeLocaleList
 * @tc.desc: Create a list of locales and canonicalize the locales in the list through "CanonicalizeUnicodeLocaleId"
 *           function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(LocaleHelperTest, CanonicalizeLocaleList)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> ctor = env->GetLocaleFunction();
    JSHandle<JSLocale> jsLocale =
        JSHandle<JSLocale>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    // Set IcuLocale
    icu::Locale icuLocale("fr", "Latn", "Fr");
    factory->NewJSIntlIcuData(jsLocale, icuLocale, JSLocale::FreeIcuLocale);
    // test locale is jslocale
    JSHandle<TaggedArray> localeArr = LocaleHelper::CanonicalizeLocaleList(thread, JSHandle<JSTaggedValue>(jsLocale));
    EXPECT_EQ(localeArr->GetLength(), 1U);
    JSHandle<EcmaString> handleEcmaStr(thread, localeArr->Get(0));
    EXPECT_STREQ("fr-Latn-FR", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
    // test locale is object
    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    JSHandle<JSTaggedValue> localeObj(thread, arr);

    JSHandle<JSTaggedValue> localeStr1(factory->NewFromASCII("EN-us"));
    PropertyDescriptor desc1(thread, localeStr1, true, true, true);
    JSHandle<JSTaggedValue> key1(factory->NewFromASCII("1"));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(localeObj), key1, desc1);

    JSHandle<JSTaggedValue> localeStr2(factory->NewFromASCII("en-GB"));
    PropertyDescriptor desc2(thread, localeStr2, true, true, true);
    JSHandle<JSTaggedValue> key2(factory->NewFromASCII("2"));
    JSArray::DefineOwnProperty(thread, JSHandle<JSObject>(localeObj), key2, desc2);
    // check canonicalized string
    localeArr = LocaleHelper::CanonicalizeLocaleList(thread, localeObj);
    EXPECT_EQ(localeArr->GetLength(), 2U);
    JSHandle<EcmaString> resultEcmaStr1(thread, localeArr->Get(0));
    EXPECT_STREQ("en-US", EcmaStringAccessor(resultEcmaStr1).ToCString().c_str());
    JSHandle<EcmaString> resultEcmaStr2(thread, localeArr->Get(1));
    EXPECT_STREQ("en-GB", EcmaStringAccessor(resultEcmaStr2).ToCString().c_str());
}

/**
 * @tc.name: CanonicalizeUnicodeLocaleId
 * @tc.desc: Call "CanonicalizeUnicodeLocaleId" function canonicalize locale(Language-Tag),The English case of language,
 *           region and script is fixed.the language is lowercase.the beginning of region is uppercase, and the script
 *           is lowercase.if locale string is IsUtf16,return empty string.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(LocaleHelperTest, CanonicalizeUnicodeLocaleId)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();

    JSHandle<EcmaString> locale = factory-> NewFromStdString("en-Us");
    JSHandle<EcmaString> canonicalizeLocaleId = LocaleHelper::CanonicalizeUnicodeLocaleId(thread, locale);
    EXPECT_STREQ("en-US", EcmaStringAccessor(canonicalizeLocaleId).ToCString().c_str());

    locale = factory-> NewFromStdString("kO-kore-kr");
    canonicalizeLocaleId = LocaleHelper::CanonicalizeUnicodeLocaleId(thread, locale);
    EXPECT_STREQ("ko-Kore-KR", EcmaStringAccessor(canonicalizeLocaleId).ToCString().c_str());

    locale = factory-> NewFromStdString("id-u-co-pinyin-de-ID");
    canonicalizeLocaleId = LocaleHelper::CanonicalizeUnicodeLocaleId(thread, locale);
    EXPECT_STREQ("id-u-co-pinyin-de-id", EcmaStringAccessor(canonicalizeLocaleId).ToCString().c_str());
    // invalid locale
    uint16_t localeArr[] = {0x122, 0x104, 0x45, 0x72, 0x97, 0x110, 0x115, 0x45, 0x67, 0x78}; // zh-Hans-CN
    uint32_t localeArrLength = sizeof(localeArr) / sizeof(localeArr[0]);
    locale = factory->NewFromUtf16(localeArr, localeArrLength);

    canonicalizeLocaleId = LocaleHelper::CanonicalizeUnicodeLocaleId(thread, locale);
    JSHandle<EcmaString> emptyString = factory->GetEmptyString();
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, canonicalizeLocaleId, emptyString), 0);
}

/**
 * @tc.name: ToLanguageTag
 * @tc.desc: call "ToLanguageTag" function Convert ICU Locale into language tag.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(LocaleHelperTest, ToLanguageTag)
{
    icu::Locale icuLocale1("en", "Latn", "US", "collation=phonebk;currency=euro");
    JSHandle<EcmaString> languageTag = LocaleHelper::ToLanguageTag(thread, icuLocale1);
    EXPECT_STREQ("en-Latn-US-u-co-phonebk-cu-euro", EcmaStringAccessor(languageTag).ToCString().c_str());

    icu::Locale icuLocale2("zh", "Hans", "CN", "collation=phonebk;kn=true");
    languageTag = LocaleHelper::ToLanguageTag(thread, icuLocale2);
    EXPECT_STREQ("zh-Hans-CN-u-co-phonebk-kn", EcmaStringAccessor(languageTag).ToCString().c_str());

    icu::Locale icuLocale3("ja", "Jpan", "JP", "collation=phonebk;co=yes");
    languageTag = LocaleHelper::ToLanguageTag(thread, icuLocale3);
    EXPECT_STREQ("ja-Jpan-JP-u-co", EcmaStringAccessor(languageTag).ToCString().c_str());

    icu::Locale icuLocale4("z", "CN"); // language is fault
    languageTag = LocaleHelper::ToLanguageTag(thread, icuLocale4);
    EXPECT_STREQ("und-CN", EcmaStringAccessor(languageTag).ToCString().c_str());

    icu::Locale icuLocale5("zh", "c"); // script is fault
    languageTag = LocaleHelper::ToLanguageTag(thread, icuLocale5);
    EXPECT_STREQ("zh-x-lvariant-c", EcmaStringAccessor(languageTag).ToCString().c_str());

    icu::Locale icuLocale6("en", "Latn", "E"); // region is fault
    languageTag = LocaleHelper::ToLanguageTag(thread, icuLocale6);
    EXPECT_STREQ("en-Latn-x-lvariant-e", EcmaStringAccessor(languageTag).ToCString().c_str());

    icu::Locale icuLocale7("en", "Latn", "EG", "kf=yes"); // key value is fault
    languageTag = LocaleHelper::ToLanguageTag(thread, icuLocale7);
    EXPECT_STREQ("en-Latn-EG-u-kf", EcmaStringAccessor(languageTag).ToCString().c_str());
}

/**
 * @tc.name: IsStructurallyValidLanguageTag
 * @tc.desc: Call "IsStructurallyValidLanguageTag" function check Language-Tag is valid structurally.If the tag contains
 *           the correct language, region, script and extension, return true otherwise, return false.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(LocaleHelperTest, IsStructurallyValidLanguageTag)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // number-language
    JSHandle<EcmaString> handleEcmaStr = factory->NewFromStdString("123-de");
    EXPECT_FALSE(LocaleHelper::IsStructurallyValidLanguageTag(handleEcmaStr));
    // only language(zh)
    handleEcmaStr = factory-> NewFromStdString("zh");
    EXPECT_TRUE(LocaleHelper::IsStructurallyValidLanguageTag(handleEcmaStr));
    // only language and script, region
    handleEcmaStr = factory-> NewFromStdString("zh-Hans-Cn");
    EXPECT_TRUE(LocaleHelper::IsStructurallyValidLanguageTag(handleEcmaStr));

    handleEcmaStr = factory-> NewFromStdString("ja-JP-u-ca-japanese");
    EXPECT_TRUE(LocaleHelper::IsStructurallyValidLanguageTag(handleEcmaStr));

    handleEcmaStr = factory-> NewFromStdString("语言脚本地区");
    EXPECT_FALSE(LocaleHelper::IsStructurallyValidLanguageTag(handleEcmaStr));

    handleEcmaStr = factory-> NewFromStdString("e-US");
    EXPECT_FALSE(LocaleHelper::IsStructurallyValidLanguageTag(handleEcmaStr));
}

/**
 * @tc.name: ConvertToStdString
 * @tc.desc: Convert char* type to std string,check whether the returned string through "ConvertToStdString"
 *           function is within expectations.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(LocaleHelperTest, ConvertToStdString)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> handleEcmaStr = factory-> NewFromStdString("一二三四");
    std::string stdString = LocaleHelper::ConvertToStdString(handleEcmaStr);
    EXPECT_STREQ(stdString.c_str(), "一二三四");

    handleEcmaStr = factory-> NewFromStdString("#%!\0@$");
    stdString = LocaleHelper::ConvertToStdString(handleEcmaStr);
    EXPECT_STREQ(stdString.c_str(), "#%!\0@$");

    handleEcmaStr = factory-> NewFromStdString("123456");
    stdString = LocaleHelper::ConvertToStdString(handleEcmaStr);
    EXPECT_STREQ(stdString.c_str(), "123456");

    handleEcmaStr = factory-> NewFromStdString("zhde");
    stdString = LocaleHelper::ConvertToStdString(handleEcmaStr);
    EXPECT_STREQ(stdString.c_str(), "zhde");
}

/**
 * @tc.name: HandleLocaleExtension
 * @tc.desc: Find position of subtag "x" or "u" in Locale through "HandleLocaleExtension" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(LocaleHelperTest, HandleLocaleExtension)
{
    std::string result = "en-Latn-US-u-ca-gregory-co-compat";
    size_t start = 0;
    size_t extensionEnd = 0;
    LocaleHelper::HandleLocaleExtension(start, extensionEnd, result, result.size());
    EXPECT_EQ(extensionEnd, 10U); // the position of "u"
    // private extension("x")
    result = "de-zh-x-co-phonebk-nu-kali";
    start = 0;
    extensionEnd = 0;
    LocaleHelper::HandleLocaleExtension(start, extensionEnd, result, result.size());
    EXPECT_EQ(extensionEnd, 5U); // the position of "x"
}

/**
 * @tc.name: HandleLocale
 * @tc.desc: Call "HandleLocale" function handle locale,if Locale has subtag "u" ignore it.If Locale has
 *           both subtag "x" and "u","x" is in front of "u","u" does not ignore,"x" is after "u","u" ignores.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(LocaleHelperTest, HandleLocale)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // no "u" or "x"
    JSHandle<EcmaString> localeString = factory->NewFromASCII("en-Latn-US");
    LocaleHelper::ParsedLocale parsedResult = LocaleHelper::HandleLocale(localeString);
    EXPECT_STREQ(parsedResult.base.c_str(), "en-Latn-US");
    // only "x"
    localeString = factory->NewFromASCII("zh-CN-x-ca-pinyin");
    parsedResult = LocaleHelper::HandleLocale(localeString);
    EXPECT_STREQ(parsedResult.base.c_str(), "zh-CN-x-ca-pinyin");
    // only "u"
    localeString = factory->NewFromASCII("ko-Kore-KR-u-co-phonebk");
    parsedResult = LocaleHelper::HandleLocale(localeString);
    EXPECT_STREQ(parsedResult.base.c_str(), "ko-Kore-KR");
    // both "x" and "u"
    localeString = factory->NewFromASCII("en-Latn-US-u-x-co-phonebk-kn-true");
    parsedResult = LocaleHelper::HandleLocale(localeString);
    EXPECT_STREQ(parsedResult.base.c_str(), "en-Latn-US-x-co-phonebk-kn-true");

    localeString = factory->NewFromASCII("en-Latn-US-x-u-ca-pinyin-co-compat");
    parsedResult = LocaleHelper::HandleLocale(localeString);
    EXPECT_STREQ(parsedResult.base.c_str(), "en-Latn-US-x-u-ca-pinyin-co-compat");
}

/**
 * @tc.name: BestAvailableLocale
 * @tc.desc: Match the best Locale and return from available locale through "BestAvailableLocale" function.
 * @tc.type: FUNC
 * @tc.require:
 */
HWTEST_F_L0(LocaleHelperTest, BestAvailableLocale)
{
    const char *path = JSCollator::uIcuDataColl.c_str();
    // available locales in uIcuDataColl
    std::vector<std::string> icuDataAvailableLocales =
        LocaleHelper::GetAvailableLocales(thread, nullptr, path);
    // available locales(calendar)
    std::vector<std::string> calendarAvailableLocales =
        LocaleHelper::GetAvailableLocales(thread, "calendar", nullptr);
    // available locales(NumberElements)
    std::vector<std::string> numberAvailableLocales =
        LocaleHelper::GetAvailableLocales(thread, "NumberElements", nullptr);
    // "ar-001" is found
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(icuDataAvailableLocales, "ar-001").c_str(), "ar-001");
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(calendarAvailableLocales, "ar-001").c_str(), "ar-001");
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(numberAvailableLocales, "ar-001").c_str(), "ar-001");
    // "agq-CM" is not found in uIcuDataColl
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(icuDataAvailableLocales, "agq-CM").c_str(), "");
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(calendarAvailableLocales, "agq-CM").c_str(), "agq-CM");
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(numberAvailableLocales, "agq-CM").c_str(), "agq-CM");
    // language(und)-region(CN)
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(icuDataAvailableLocales, "und-CN").c_str(), "");
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(calendarAvailableLocales, "und-CN").c_str(), "");
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(numberAvailableLocales, "und-CN").c_str(), "");
    // language(en)-region(001)
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(icuDataAvailableLocales, "en-001").c_str(), "en-001");
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(calendarAvailableLocales, "en-001").c_str(), "en-001");
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(numberAvailableLocales, "en-001").c_str(), "en-001");
    // language(en)-script(Hans)-region(US)
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(icuDataAvailableLocales, "en-Hans-US").c_str(), "en");
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(calendarAvailableLocales, "en-Hans-US").c_str(), "en");
    EXPECT_STREQ(LocaleHelper::BestAvailableLocale(numberAvailableLocales, "en-Hans-US").c_str(), "en");
}
} // namespace panda::test