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

#include "builtin_test_util.h"
#include "ecmascript/builtins/builtins_list_format.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_list_format.h"
#include "ecmascript/js_array.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsListFormatTest : public BaseTestWithScope<true> {
};

JSTaggedValue CommonTest(JSThread* thread, JSHandle<JSTaggedValue>& locale, JSHandle<JSTaggedValue>& typeValue,
    std::vector<std::string>& strValues)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSListFormat> jsListFormat =
        JSHandle<JSListFormat>(thread, BuiltTestUtil::JSListFormatCreateWithOptionTest(thread, locale, typeValue));
    JSArray *arr = JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetObject<JSArray>();
    EXPECT_TRUE(arr != nullptr);
    JSHandle<JSObject> value(thread, arr);
    std::vector<JSHandle<JSTaggedValue>> vals;
    for (size_t i = 0; i < strValues.size(); i++) {
        JSHandle<JSTaggedValue> listValue(factory->NewFromStdString(strValues[i]));
        JSHandle<JSTaggedValue> key(thread, JSTaggedValue(static_cast<int>(i)));
        PropertyDescriptor desc(thread, listValue, true, true, true);
        JSArray::DefineOwnProperty(thread, value, key, desc);
    }

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsListFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsListFormat::FormatToParts(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

HWTEST_F_L0(BuiltinsListFormatTest, FormatToParts_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de-DE"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("conjunction"));
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, BuiltTestUtil::JSListFormatCreateWithOptionTest(thread, locale, typeValue));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jSListFormat.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsListFormat::FormatToParts(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, result);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements(thread));
    EXPECT_EQ(elements->GetLength(), 0U); // zero formatters
}

// FormatToParts(["Apple", "Orange", "Pineapple" ], type(conjunction))
HWTEST_F_L0(BuiltinsListFormatTest, FormatToParts_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-GB"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("conjunction")); // the default value
    
    std::vector<std::string> vals{"Apple", "Orange", "Pineapple"};
    JSTaggedValue result = CommonTest(thread, locale, typeValue, vals);

    JSHandle<JSArray> resultHandle(thread, result);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements(thread));
    EXPECT_EQ(elements->GetLength(), 6U);
}

HWTEST_F_L0(BuiltinsListFormatTest, FormatToParts_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-GB"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("conjunction")); // the default value

    std::vector<std::string> vals{"Apple", "Orange", "Pineapple"};
    JSTaggedValue result = CommonTest(thread, locale, typeValue, vals);

    JSTaggedValue valueList(static_cast<JSTaggedType>(result.GetRawData()));

    ASSERT_TRUE(valueList.IsECMAObject());
    JSHandle<JSObject> valueHandle(thread, valueList);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKey).GetValue()->GetInt(), 5);
}

// FormatToParts(["中文", "英文"], type(conjunction))
HWTEST_F_L0(BuiltinsListFormatTest, FormatToParts_004)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("zh-cn"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("conjunction"));

    std::vector<std::string> vals{"中文", "英文"};
    JSTaggedValue result = CommonTest(thread, locale, typeValue, vals);

    JSHandle<JSArray> resultHandle(thread, result);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements(thread));
    EXPECT_EQ(elements->GetLength(), 3U);
}

// FormatToParts(["中文", "英文","韩文","葡萄牙语"], type(disjunction))
HWTEST_F_L0(BuiltinsListFormatTest, FormatToParts_005)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> lengthKey = thread->GlobalConstants()->GetHandledLengthString();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("zh-cn"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("disjunction"));

    std::vector<std::string> vals{"中文", "英文", "韩文", "葡萄牙语"};
    JSTaggedValue result = CommonTest(thread, locale, typeValue, vals);
    JSTaggedValue valueList(static_cast<JSTaggedType>(result.GetRawData()));

    ASSERT_TRUE(valueList.IsECMAObject());
    JSHandle<JSObject> valueHandle(thread, valueList);
    EXPECT_EQ(JSArray::GetProperty(thread, JSHandle<JSTaggedValue>(valueHandle), lengthKey).GetValue()->GetInt(), 7);
}

// SupportedLocalesOf("best fit")
HWTEST_F_L0(BuiltinsListFormatTest, SupportedLocalesOf_001)
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
    JSTaggedValue resultArr = BuiltinsListFormat::SupportedLocalesOf(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, resultArr);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements(thread));
    EXPECT_EQ(elements->GetLength(), 1U);
    JSHandle<EcmaString> handleEcmaStr(thread, elements->Get(thread, 0));
    EXPECT_STREQ("id-u-co-pinyin-de-id", EcmaStringAccessor(handleEcmaStr).ToCString(thread).c_str());
}

// SupportedLocalesOf("look up")
HWTEST_F_L0(BuiltinsListFormatTest, SupportedLocalesOf_002)
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
    JSTaggedValue resultArr = BuiltinsListFormat::SupportedLocalesOf(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, resultArr);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements(thread));
    EXPECT_EQ(elements->GetLength(), 1U);

    JSHandle<EcmaString> resultStr(thread, elements->Get(thread, 0));
    EXPECT_STREQ("id-u-co-pinyin-de", EcmaStringAccessor(resultStr).ToCString(thread).c_str());
}

HWTEST_F_L0(BuiltinsListFormatTest, ResolvedOptions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de-DE"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("disjunction"));
    JSHandle<JSListFormat> jSListFormat =
        JSHandle<JSListFormat>(thread, BuiltTestUtil::JSListFormatCreateWithOptionTest(thread, locale, typeValue));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jSListFormat.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsListFormat::ResolvedOptions(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSTaggedValue> resultObj =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result.GetRawData())));
    // judge whether the properties of the object are the same as those of jslistformat tag
    JSHandle<JSTaggedValue> localeKey = globalConst->GetHandledLocaleString();
    JSHandle<JSTaggedValue> localeValue(factory->NewFromASCII("de-DE"));
    EXPECT_EQ(JSTaggedValue::SameValue(thread,
        JSObject::GetProperty(thread, resultObj, localeKey).GetValue(), localeValue), true);
    JSHandle<JSTaggedValue> typeKey = globalConst->GetHandledTypeString();
    EXPECT_EQ(JSTaggedValue::SameValue(thread,
        JSObject::GetProperty(thread, resultObj, typeKey).GetValue(), typeValue), true);
}
} // namespace panda::test