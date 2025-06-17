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
#include "ecmascript/global_env.h"
#include "ecmascript/js_array.h"
#include "ecmascript/js_list_format.h"
#include "ecmascript/js_iterator.h"
#include "ecmascript/object_factory-inl.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSListFormatTest : public BaseTestWithScope<true> {
};

HWTEST_F_L0(JSListFormatTest, Set_Get_IcuListFormatter_001)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> ctor = env->GetListFormatFunction();
    JSHandle<JSListFormat> jsFormatter =
        JSHandle<JSListFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    UErrorCode status = UErrorCode::U_ZERO_ERROR;
    icu::Locale icuLocale("en", "Latn", "US");
    icu::ListFormatter* icuFormatter = icu::ListFormatter::createInstance(icuLocale, status);
    JSListFormat::SetIcuListFormatter(thread, jsFormatter, icuFormatter, JSListFormat::FreeIcuListFormatter);
    icu::ListFormatter *resFormatter = jsFormatter->GetIcuListFormatter(thread);
    EXPECT_TRUE(resFormatter != nullptr);

    const int32_t itemNum = 3;
    const icu::UnicodeString items[itemNum] = { "One", "Two", "Three" };
    icu::UnicodeString resStr = "";
    resStr = resFormatter->format(items, itemNum, resStr, status);
    const icu::UnicodeString expectResStr("One, Two, and Three");
    EXPECT_TRUE(resStr.compare(expectResStr) == 0);
}

HWTEST_F_L0(JSListFormatTest, Set_Get_IcuListFormatter_002)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> ctor = env->GetListFormatFunction();
    JSHandle<JSListFormat> jsFormatter =
        JSHandle<JSListFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(ctor), ctor));
    UErrorCode status = UErrorCode::U_ZERO_ERROR;
    icu::Locale icuLocale("zh", "Hans", "Cn");
    icu::ListFormatter* icuFormatter = icu::ListFormatter::createInstance(icuLocale, status);
    JSListFormat::SetIcuListFormatter(thread, jsFormatter, icuFormatter, JSListFormat::FreeIcuListFormatter);
    icu::ListFormatter *resFormatter = jsFormatter->GetIcuListFormatter(thread);
    EXPECT_TRUE(resFormatter != nullptr);

    const int32_t itemNum = 3;
    const icu::UnicodeString items[itemNum] = { "一", "二", "三" };
    icu::UnicodeString resStr = "";
    resStr = resFormatter->format(items, itemNum, resStr, status);
    const icu::UnicodeString expectResStr("一、二和三");
    EXPECT_TRUE(resStr.compare(expectResStr) == 0);
}

JSHandle<JSListFormat> CreateJSListFormatterTest(JSThread *thread, icu::Locale icuLocale, JSHandle<JSObject> options)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();

    JSHandle<JSTaggedValue> localeCtor = env->GetLocaleFunction();
    JSHandle<JSTaggedValue> listCtor = env->GetListFormatFunction();
    JSHandle<JSLocale> locales =
        JSHandle<JSLocale>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(localeCtor), localeCtor));
    JSHandle<JSListFormat> listFormatter =
        JSHandle<JSListFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(listCtor), listCtor));

    JSHandle<JSTaggedValue> optionsVal = JSHandle<JSTaggedValue>::Cast(options);
    factory->NewJSIntlIcuData(locales, icuLocale, JSLocale::FreeIcuLocale);
    listFormatter = JSListFormat::InitializeListFormat(thread, listFormatter,
        JSHandle<JSTaggedValue>::Cast(locales), optionsVal);
    return listFormatter;
}

void SetFormatterOptionsTest(JSThread *thread, JSHandle<JSObject> &optionsObj,
    std::map<std::string, std::string> &options)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> localeMatcherKey = globalConst->GetHandledLocaleMatcherString();
    JSHandle<JSTaggedValue> typeKey = globalConst->GetHandledTypeString();
    JSHandle<JSTaggedValue> styleKey = globalConst->GetHandledStyleString();
    JSHandle<JSTaggedValue> localeMatcherValue(factory->NewFromASCII(options["localeMatcher"].c_str()));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII(options["type"].c_str()));
    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII(options["style"].c_str()));
    JSObject::SetProperty(thread, optionsObj, localeMatcherKey, localeMatcherValue);
    JSObject::SetProperty(thread, optionsObj, typeKey, typeValue);
    JSObject::SetProperty(thread, optionsObj, styleKey, styleValue);
}

HWTEST_F_L0(JSListFormatTest, InitializeListFormat)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    icu::Locale icuLocale("en", "Latn", "US");
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> object = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    std::map<std::string, std::string> options {
        { "localeMatcher", "best fit" },
        { "type", "conjunction" },
        { "style", "long" }
    };
    SetFormatterOptionsTest(thread, object, options);
    JSHandle<JSTaggedValue> localeCtor = env->GetLocaleFunction();
    JSHandle<JSTaggedValue> listCtor = env->GetListFormatFunction();
    JSHandle<JSLocale> locales =
        JSHandle<JSLocale>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(localeCtor), localeCtor));
    JSHandle<JSListFormat> listFormatter =
        JSHandle<JSListFormat>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(listCtor), listCtor));

    JSHandle<JSTaggedValue> optionsVal = JSHandle<JSTaggedValue>::Cast(object);
    factory->NewJSIntlIcuData(locales, icuLocale, JSLocale::FreeIcuLocale);
    listFormatter = JSListFormat::InitializeListFormat(thread, listFormatter,
        JSHandle<JSTaggedValue>::Cast(locales), optionsVal);
    icu::ListFormatter *resFormatter = listFormatter->GetIcuListFormatter(thread);
    EXPECT_TRUE(resFormatter != nullptr);

    const int32_t itemNum = 3;
    UErrorCode status = UErrorCode::U_ZERO_ERROR;
    const icu::UnicodeString items[itemNum] = { "Monday", "Tuesday", "Wednesday" };
    icu::UnicodeString resStr = "";
    resStr = resFormatter->format(items, itemNum, resStr, status);
    const icu::UnicodeString expectResStr("Monday, Tuesday, and Wednesday");
    EXPECT_TRUE(resStr.compare(expectResStr) == 0);
}

JSHandle<JSArray> FormatCommon(JSThread *thread, std::vector<std::string>& strs)
{
    auto factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSObject> valueObj = JSHandle<JSObject>::Cast(factory->NewJSArray());
    for (size_t i = 0; i < strs.size(); i++) {
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(static_cast<int32_t>(i)));
        JSHandle<JSTaggedValue> value(factory->NewFromStdString(strs[i]));
        JSObject::SetProperty(thread, valueObj, key, value);
    }
    JSHandle<JSArray> valueArr = JSHandle<JSArray>::Cast(valueObj);
    return valueArr;
}

JSHandle<JSListFormat> GetFormatter(JSThread *thread, std::map<std::string, std::string>& options,
    icu::Locale& icuLocale)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    auto env = vm->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> object = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    SetFormatterOptionsTest(thread, object, options);
    JSHandle<JSListFormat> jsFormatter = CreateJSListFormatterTest(thread, icuLocale, object);
    return jsFormatter;
}

HWTEST_F_L0(JSListFormatTest, FormatList_001)
{
    icu::Locale icuLocale("en", "Latn", "US");
    std::map<std::string, std::string> options {
        { "localeMatcher", "best fit" },
        { "type", "conjunction" },
        { "style", "long" }
    };
    JSHandle<JSListFormat> jsFormatter = GetFormatter(thread, options, icuLocale);
    std::vector<std::string> strs{"Zero", "One", "Two"};
    JSHandle<JSArray> valueArr = FormatCommon(thread, strs);
    JSHandle<EcmaString> valueStr = JSListFormat::FormatList(thread, jsFormatter, valueArr);
    EXPECT_STREQ(EcmaStringAccessor(valueStr).ToCString(thread).c_str(), "Zero, One, and Two");
}

HWTEST_F_L0(JSListFormatTest, FormatList_002)
{
    icu::Locale icuLocale("en", "Latn", "US");
    // when style is narrow, type can only be unit
    std::map<std::string, std::string> options {
        { "localeMatcher", "best fit" },
        { "type", "unit" },
        { "style", "narrow" }
    };
    JSHandle<JSListFormat> jsFormatter = GetFormatter(thread, options, icuLocale);
    std::vector<std::string> strs{"Zero", "One", "Two"};
    JSHandle<JSArray> valueArr = FormatCommon(thread, strs);
    JSHandle<EcmaString> valueStr = JSListFormat::FormatList(thread, jsFormatter, valueArr);
    EXPECT_STREQ(EcmaStringAccessor(valueStr).ToCString(thread).c_str(), "Zero One Two");
}

HWTEST_F_L0(JSListFormatTest, FormatList_003)
{
    icu::Locale icuLocale("zh", "Hans", "Cn");
    std::map<std::string, std::string> options {
        { "localeMatcher", "best fit" },
        { "type", "disjunction" },
        { "style", "long" }
    };
    JSHandle<JSListFormat> jsFormatter = GetFormatter(thread, options, icuLocale);
    std::vector<std::string> strs{"苹果", "梨子", "桃"};
    JSHandle<JSArray> valueArr = FormatCommon(thread, strs);
    JSHandle<EcmaString> valueStr = JSListFormat::FormatList(thread, jsFormatter, valueArr);
    EXPECT_STREQ(EcmaStringAccessor(valueStr).ToCString(thread).c_str(), "苹果、梨子或桃");
}

std::string GetListPartStringTest(JSThread *thread, JSHandle<JSTaggedValue> key, JSHandle<JSTaggedValue> part)
{
    JSHandle<JSObject> partObj = JSHandle<JSObject>::Cast(part);
    JSHandle<JSTaggedValue> partValue = JSObject::GetProperty(thread, partObj, key).GetValue();
    JSHandle<EcmaString> partEcmaStr = JSHandle<EcmaString>::Cast(partValue);
    std::string partStr = intl::LocaleHelper::ConvertToStdString(thread, partEcmaStr);
    return partStr;
}

HWTEST_F_L0(JSListFormatTest, FormatListToParts)
{
    icu::Locale icuLocale("zh", "Hans", "Cn");
    std::map<std::string, std::string> options {
        { "localeMatcher", "best fit" },
        { "type", "conjunction" },
        { "style", "long" }
    };
    JSHandle<JSListFormat> jsFormatter = GetFormatter(thread, options, icuLocale);
    std::vector<std::string> strs{"苹果", "梨子", "桃"};
    JSHandle<JSArray> valueArr = FormatCommon(thread, strs);
    JSHandle<EcmaString> valueStr = JSListFormat::FormatList(thread, jsFormatter, valueArr);
    EXPECT_STREQ(EcmaStringAccessor(valueStr).ToCString(thread).c_str(), "苹果、梨子和桃");

    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> typeKey = globalConst->GetHandledTypeString();
    JSHandle<JSTaggedValue> valueKey = globalConst->GetHandledValueString();
    JSHandle<JSArray> parts = JSListFormat::FormatListToParts(thread, jsFormatter, valueArr);
    auto element1 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(parts), 0).GetValue();
    auto literal1 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(parts), 1).GetValue();
    auto element2 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(parts), 2).GetValue();
    auto literal2 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(parts), 3).GetValue();
    auto element3 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(parts), 4).GetValue();
    EXPECT_STREQ(GetListPartStringTest(thread, typeKey, element1).c_str(), "element");
    EXPECT_STREQ(GetListPartStringTest(thread, valueKey, element1).c_str(), "苹果");
    EXPECT_STREQ(GetListPartStringTest(thread, typeKey, literal1).c_str(), "literal");
    EXPECT_STREQ(GetListPartStringTest(thread, valueKey, literal1).c_str(), "、");
    EXPECT_STREQ(GetListPartStringTest(thread, typeKey, element2).c_str(), "element");
    EXPECT_STREQ(GetListPartStringTest(thread, valueKey, element2).c_str(), "梨子");
    EXPECT_STREQ(GetListPartStringTest(thread, typeKey, literal2).c_str(), "literal");
    EXPECT_STREQ(GetListPartStringTest(thread, valueKey, literal2).c_str(), "和");
    EXPECT_STREQ(GetListPartStringTest(thread, typeKey, element3).c_str(), "element");
    EXPECT_STREQ(GetListPartStringTest(thread, valueKey, element3).c_str(), "桃");
}

HWTEST_F_L0(JSListFormatTest, StringListFromIterable)
{
    auto vm = thread->GetEcmaVM();
    auto factory = vm->GetFactory();
    JSHandle<TaggedArray> data = factory->NewTaggedArray(3);
    JSHandle<JSTaggedValue> value0(factory->NewFromStdString("one"));
    JSHandle<JSTaggedValue> value1(factory->NewFromStdString("two"));
    JSHandle<JSTaggedValue> value2(factory->NewFromStdString("three"));
    data->Set(thread, 0, value0.GetTaggedValue());
    data->Set(thread, 1, value1.GetTaggedValue());
    data->Set(thread, 2, value2.GetTaggedValue());
    JSHandle<JSTaggedValue> array(JSArray::CreateArrayFromList(thread, data));
    JSHandle<JSArrayIterator> iter(JSIterator::GetIterator(thread, array));
    JSHandle<JSTaggedValue> arrayString =
        JSListFormat::StringListFromIterable(thread, JSHandle<JSTaggedValue>::Cast(iter));
    JSHandle<JSArray> strValue = JSHandle<JSArray>::Cast(arrayString);
    EXPECT_EQ(strValue->GetArrayLength(), 3U);

    auto resValue0 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(strValue), 0).GetValue();
    auto resValue1 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(strValue), 1).GetValue();
    auto resValue2 = JSTaggedValue::GetProperty(thread, JSHandle<JSTaggedValue>::Cast(strValue), 2).GetValue();
    EXPECT_STREQ(EcmaStringAccessor(resValue0.GetTaggedValue()).ToCString(thread).c_str(), "one");
    EXPECT_STREQ(EcmaStringAccessor(resValue1.GetTaggedValue()).ToCString(thread).c_str(), "two");
    EXPECT_STREQ(EcmaStringAccessor(resValue2.GetTaggedValue()).ToCString(thread).c_str(), "three");
}
}  // namespace panda::test
