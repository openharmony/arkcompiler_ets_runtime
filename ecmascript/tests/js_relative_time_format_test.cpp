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

#include "ecmascript/js_relative_time_format.h"
#include "ecmascript/intl/locale_helper.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/global_env.h"
#include "ecmascript/napi/jsnapi_helper.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class JSRelativeTimeFormatTest : public BaseTestWithScope<true> {
};

HWTEST_F_L0(JSRelativeTimeFormatTest, InitializeRelativeTimeFormat)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> ctor = env->GetRelativeTimeFormatFunction();
    JSHandle<JSRelativeTimeFormat> relativeTimeFormat =
        JSHandle<JSRelativeTimeFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    EXPECT_TRUE(*relativeTimeFormat != nullptr);

    JSHandle<JSTaggedValue> locales(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> undefinedOptions(thread, JSTaggedValue::Undefined());
    JSRelativeTimeFormat::InitializeRelativeTimeFormat(thread, relativeTimeFormat, locales, undefinedOptions);
    // Initialize attribute comparison
    JSHandle<EcmaString> numberingSystemStr(thread, relativeTimeFormat->GetNumberingSystem().GetTaggedObject());
    EXPECT_STREQ("latn", EcmaStringAccessor(numberingSystemStr).ToCString().c_str());
    JSHandle<EcmaString> localeStr(thread, relativeTimeFormat->GetLocale().GetTaggedObject());
    EXPECT_STREQ("en", EcmaStringAccessor(localeStr).ToCString().c_str());
    EXPECT_EQ(NumericOption::ALWAYS, relativeTimeFormat->GetNumeric());
    EXPECT_EQ(RelativeStyleOption::LONG, relativeTimeFormat->GetStyle());
}

HWTEST_F_L0(JSRelativeTimeFormatTest, GetIcuRTFFormatter)
{
    double value = base::NAN_VALUE;
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();

    JSHandle<JSTaggedValue> ctor = env->GetRelativeTimeFormatFunction();
    JSHandle<JSRelativeTimeFormat> relativeTimeFormat =
        JSHandle<JSRelativeTimeFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    // Create Icu RelativeDateTimeFormatter
    icu::Locale icuLocale("en", "US");
    UErrorCode status = U_ZERO_ERROR;
    icu::NumberFormat *icuNumberFormat = icu::NumberFormat::createInstance(icuLocale, UNUM_DECIMAL, status);
    icu::RelativeDateTimeFormatter rtfFormatter(icuLocale, icuNumberFormat, UDAT_STYLE_LONG,
                                                UDISPCTX_CAPITALIZATION_NONE, status);
    icu::UnicodeString result1 = rtfFormatter.formatNumericToValue(value, UDAT_REL_UNIT_YEAR, status).toString(status);
    JSHandle<EcmaString> stringValue1 = intl::LocaleHelper::UStringToString(thread, result1);
    // Set Icu RelativeDateTimeFormatter to Icu Field
    factory->NewJSIntlIcuData(relativeTimeFormat, rtfFormatter, JSRelativeTimeFormat::FreeIcuRTFFormatter);
    // Get Icu Field
    icu::RelativeDateTimeFormatter *resultRelativeDateTimeFormatter = relativeTimeFormat->GetIcuRTFFormatter();
    icu::UnicodeString result2 =
        resultRelativeDateTimeFormatter->formatNumericToValue(value, UDAT_REL_UNIT_YEAR, status).toString(status);
    JSHandle<EcmaString> stringValue2 = intl::LocaleHelper::UStringToString(thread, result2);
    EXPECT_EQ(EcmaStringAccessor::StringsAreEqual(*stringValue1, *stringValue2), true);
}

HWTEST_F_L0(JSRelativeTimeFormatTest, UnwrapRelativeTimeFormat)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    EcmaVM *vm = thread->GetEcmaVM();

    JSHandle<JSTaggedValue> relativeTimeFormatFunc = env->GetRelativeTimeFormatFunction();
    JSHandle<JSTaggedValue> relativeTimeFormat(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>(relativeTimeFormatFunc), relativeTimeFormatFunc));

    Local<FunctionRef> relativeTimeFormatLocal = JSNApiHelper::ToLocal<FunctionRef>(relativeTimeFormatFunc);
    JSHandle<JSTaggedValue> disPlayNamesFunc = env->GetDisplayNamesFunction();
    Local<FunctionRef> disPlayNamesLocal = JSNApiHelper::ToLocal<FunctionRef>(disPlayNamesFunc);
    // displaynames Inherit relativeTimeFormat
    disPlayNamesLocal->Inherit(vm, relativeTimeFormatLocal);
    JSHandle<JSTaggedValue> disPlayNamesHandle = JSNApiHelper::ToJSHandle(disPlayNamesLocal);
    JSHandle<JSTaggedValue> disPlayNamesObj(
        factory->NewJSObjectByConstructor(JSHandle<JSFunction>::Cast(disPlayNamesHandle), disPlayNamesHandle));
    // object has no Instance
    JSHandle<JSTaggedValue> unwrapNumberFormat1 =
        JSRelativeTimeFormat::UnwrapRelativeTimeFormat(thread, relativeTimeFormat);
    EXPECT_TRUE(JSTaggedValue::SameValue(relativeTimeFormat, unwrapNumberFormat1));
    // object has Instance
    JSHandle<JSTaggedValue> unwrapNumberFormat2 =
        JSRelativeTimeFormat::UnwrapRelativeTimeFormat(thread, disPlayNamesObj);
    EXPECT_TRUE(unwrapNumberFormat2->IsUndefined());
}
} // namespace panda::test