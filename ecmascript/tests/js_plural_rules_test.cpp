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

#include "ecmascript/intl/locale_helper.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class JSPluralRulesTest : public BaseTestWithScope<true> {
};

HWTEST_F_L0(JSPluralRulesTest, GetIcuPluralRules)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> ctor = env->GetPluralRulesFunction();
    JSHandle<JSPluralRules> pluralRules =
        JSHandle<JSPluralRules>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    // Create PluralRules
    icu::Locale icuLocale("en", "US");
    UErrorCode status = U_ZERO_ERROR;
    UPluralType icuType = UPLURAL_TYPE_CARDINAL; // Plural rules for cardinal numbers
    icu::PluralRules *icuPluralRules(icu::PluralRules::forLocale(icuLocale, icuType, status));
    icu::UnicodeString result1 = icuPluralRules->select(0);
    JSHandle<EcmaString> stringValue1 = intl::LocaleHelper::UStringToString(thread, result1);

    JSPluralRules::SetIcuPluralRules(thread, pluralRules, *icuPluralRules, JSPluralRules::FreeIcuPluralRules);
    icu::PluralRules *getPluralRules = pluralRules->GetIcuPluralRules();
    icu::UnicodeString result2 = getPluralRules->select(0);
    JSHandle<EcmaString> stringValue2 = intl::LocaleHelper::UStringToString(thread, result2);
    EXPECT_EQ(EcmaStringAccessor::StringsAreEqual(*stringValue1, *stringValue2), true);
    delete icuPluralRules;
}

HWTEST_F_L0(JSPluralRulesTest, BuildLocaleSet)
{
    std::set<std::string> icuAvailableLocales {
        "en-Latn-US",
        "zh-Hans-CN",
        "pt-PT",
        "de-DE",
    };
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> resultStr;
    JSHandle<EcmaString> localeStr1 = factory->NewFromStdString("en-Latn-US");
    JSHandle<EcmaString> localeStr2 = factory->NewFromStdString("zh-Hans-CN");
    JSHandle<EcmaString> localeStr3 = factory->NewFromStdString("pt-PT");
    JSHandle<EcmaString> localeStr4 = factory->NewFromStdString("de-DE");
    JSHandle<TaggedArray> localeSet = JSPluralRules::BuildLocaleSet(thread, icuAvailableLocales);
    EXPECT_EQ(localeSet->GetLength(), 4U);

    resultStr = JSHandle<EcmaString>(thread, localeSet->Get(0).GetTaggedObject());
    EXPECT_EQ(EcmaStringAccessor::StringsAreEqual(*resultStr, *localeStr4), true);
    resultStr = JSHandle<EcmaString>(thread, localeSet->Get(1).GetTaggedObject());
    EXPECT_EQ(EcmaStringAccessor::StringsAreEqual(*resultStr, *localeStr1), true);
    resultStr = JSHandle<EcmaString>(thread, localeSet->Get(2).GetTaggedObject());
    EXPECT_EQ(EcmaStringAccessor::StringsAreEqual(*resultStr, *localeStr3), true);
    resultStr = JSHandle<EcmaString>(thread, localeSet->Get(3).GetTaggedObject());
    EXPECT_EQ(EcmaStringAccessor::StringsAreEqual(*resultStr, *localeStr2), true);
}

HWTEST_F_L0(JSPluralRulesTest, InitializePluralRules)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> optionHandle(thread, JSTaggedValue::Undefined());

    JSHandle<JSTaggedValue> ctor = env->GetPluralRulesFunction();
    JSHandle<JSPluralRules> pluralRules =
        JSHandle<JSPluralRules>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    JSHandle<JSTaggedValue> localeStr(factory->NewFromASCII("en-GB"));
    JSHandle<JSPluralRules> initPluralRules =
        JSPluralRules::InitializePluralRules(thread, pluralRules, localeStr, optionHandle);
    EXPECT_TRUE(*initPluralRules != nullptr);

    JSHandle<EcmaString> resultLocaleStr(thread, initPluralRules->GetLocale().GetTaggedObject());
    EXPECT_STREQ("en", EcmaStringAccessor(resultLocaleStr).ToCString().c_str());
    EXPECT_EQ(initPluralRules->GetType(), TypeOption::CARDINAL);
    EXPECT_EQ(initPluralRules->GetRoundingType(), RoundingType::FRACTIONDIGITS);
    EXPECT_EQ(initPluralRules->GetMinimumIntegerDigits().GetInt(), 1); // 1 : 1 default minimum integer
    EXPECT_EQ(initPluralRules->GetMinimumFractionDigits().GetInt(), 0); // 0 : 0 default minimum fraction
    EXPECT_EQ(initPluralRules->GetMaximumFractionDigits().GetInt(), 3); // 3 : 3 default maximum fraction
    EXPECT_EQ(initPluralRules->GetMinimumSignificantDigits().GetInt(), 0); // 0 : 0 default minimum sigfraction
    EXPECT_EQ(initPluralRules->GetMaximumSignificantDigits().GetInt(), 0); // 0 : 0 default maximum sigfraction
}

HWTEST_F_L0(JSPluralRulesTest, ResolvePlural)
{
    double n = base::NAN_VALUE;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> optionHandle(thread, JSTaggedValue::Undefined());
    JSHandle<JSTaggedValue> typeKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("ordinal"));
    JSHandle<JSTaggedValue> localeStr(factory->NewFromASCII("en-US"));

    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSTaggedValue> ctor = env->GetPluralRulesFunction();
    JSHandle<JSPluralRules> pluralRules =
        JSHandle<JSPluralRules>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, typeKey, typeValue);
    // initialize pluralRules
    JSHandle<JSPluralRules> initPluralRules =
        JSPluralRules::InitializePluralRules(thread, pluralRules, localeStr, optionHandle);
    EXPECT_TRUE(*initPluralRules != nullptr);
    // resolve plural nan
    JSHandle<EcmaString> resolveRules = JSPluralRules::ResolvePlural(thread, initPluralRules, n);
    EXPECT_STREQ("other", EcmaStringAccessor(resolveRules).ToCString().c_str());
}
} // namespace panda::test
