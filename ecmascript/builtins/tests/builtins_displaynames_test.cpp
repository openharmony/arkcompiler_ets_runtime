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

#include "ecmascript/builtins/builtins_displaynames.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_displaynames.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
namespace panda::test {
class BuiltinsDisplayNamesTest : public BaseTestWithScope<true> {
};

static JSTaggedValue JSDisplayNamesCreateWithOptionTest(JSThread *thread, JSHandle<JSTaggedValue> &locale,
                                                        JSHandle<JSTaggedValue> &typeValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetDisplayNamesFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> typeKey = thread->GlobalConstants()->GetHandledTypeString();
    JSObject::SetProperty(thread, optionsObj, typeKey, typeValue);

    JSHandle<JSTaggedValue> localesString = locale;
    std::vector<JSTaggedValue> args{localesString.GetTaggedValue(), optionsObj.GetTaggedValue()};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, newTarget, args, 8);

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDisplayNames::DisplayNamesConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsJSDisplayNames());
    return result;
}

// new DisplayNames(locales, options)
HWTEST_F_L0(BuiltinsDisplayNamesTest, DisplayNamesConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetDisplayNamesFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> typeKey = thread->GlobalConstants()->GetHandledTypeString();
    JSHandle<JSTaggedValue> styleKey = thread->GlobalConstants()->GetHandledStyleString();
    JSHandle<JSTaggedValue> fallbackKey = thread->GlobalConstants()->GetHandledFallbackString();

    JSHandle<JSTaggedValue> localeString(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("language"));
    JSHandle<JSTaggedValue> styleValue(factory->NewFromASCII("narrow"));
    JSHandle<JSTaggedValue> fallbackValue(factory->NewFromASCII("code"));

    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, typeKey, typeValue);
    JSObject::SetProperty(thread, optionsObj, styleKey, styleValue);
    JSObject::SetProperty(thread, optionsObj, fallbackKey, fallbackValue);

    std::vector<JSTaggedValue> args{localeString.GetTaggedValue(), optionsObj.GetTaggedValue()};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, newTarget, args, 8);

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDisplayNames::DisplayNamesConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSDisplayNames());
}

// Of(fr, type(language))
HWTEST_F_L0(BuiltinsDisplayNamesTest, Of_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("language"));
    JSHandle<JSDisplayNames> jsDisplayNames =
        JSHandle<JSDisplayNames>(thread, JSDisplayNamesCreateWithOptionTest(thread, locale, typeValue));

    JSHandle<JSTaggedValue> stringValue(factory->NewFromASCII("fr"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDisplayNames.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, stringValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDisplayNames::Of(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("French", EcmaStringAccessor(handleEcmaStr).ToCString(thread).c_str());
}

// Of(419, type(region))
HWTEST_F_L0(BuiltinsDisplayNamesTest, Of_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("region"));
    JSHandle<JSDisplayNames> jsDisplayNames =
        JSHandle<JSDisplayNames>(thread, JSDisplayNamesCreateWithOptionTest(thread, locale, typeValue));

    JSHandle<JSTaggedValue> stringValue(factory->NewFromASCII("419"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDisplayNames.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, stringValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDisplayNames::Of(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("Latin America", EcmaStringAccessor(handleEcmaStr).ToCString(thread).c_str());
}

// Of(EUR, type(currency))
HWTEST_F_L0(BuiltinsDisplayNamesTest, Of_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("currency"));
    JSHandle<JSDisplayNames> jsDisplayNames =
        JSHandle<JSDisplayNames>(thread, JSDisplayNamesCreateWithOptionTest(thread, locale, typeValue));

    JSHandle<JSTaggedValue> stringValue(factory->NewFromASCII("EUR"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDisplayNames.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, stringValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDisplayNames::Of(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ("Euro", EcmaStringAccessor(handleEcmaStr).ToCString(thread).c_str());
}

// Of(Code Cover)
HWTEST_F_L0(BuiltinsDisplayNamesTest, Of_004)
{
    // IsNotJSDisplayNames
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDisplayNames::Of(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(thread->HasPendingException());
    EXPECT_EQ(result, JSTaggedValue::Exception());
    thread->ClearException();
}

// SupportedLocalesOf("best fit")
HWTEST_F_L0(BuiltinsDisplayNamesTest, SupportedLocalesOf_001)
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
    JSTaggedValue resultArr = BuiltinsDisplayNames::SupportedLocalesOf(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, resultArr);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements(thread));
    EXPECT_EQ(elements->GetLength(), 1U);
    JSHandle<EcmaString> handleEcmaStr(thread, elements->Get(thread, 0));
    EXPECT_STREQ("id-u-co-pinyin-de-id", EcmaStringAccessor(handleEcmaStr).ToCString(thread).c_str());
}

// SupportedLocalesOf("look up")
HWTEST_F_L0(BuiltinsDisplayNamesTest, SupportedLocalesOf_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> localeMatcherKey = thread->GlobalConstants()->GetHandledLocaleMatcherString();
    JSHandle<JSTaggedValue> localeMatcherValue(factory->NewFromASCII("lookup"));
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, localeMatcherKey, localeMatcherValue);
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de-DE-u-co-phonebk"));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue resultArr = BuiltinsDisplayNames::SupportedLocalesOf(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, resultArr);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements(thread));
    EXPECT_EQ(elements->GetLength(), 1U);
    JSHandle<EcmaString> handleEcmaStr(thread, elements->Get(thread, 0));
    EXPECT_STREQ("de-DE-u-co-phonebk", EcmaStringAccessor(handleEcmaStr).ToCString(thread).c_str());
}

HWTEST_F_L0(BuiltinsDisplayNamesTest, ResolvedOptions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de-DE"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("currency"));
    JSHandle<JSDisplayNames> jsDisplayNames =
        JSHandle<JSDisplayNames>(thread, JSDisplayNamesCreateWithOptionTest(
                                            thread, locale, typeValue));
    std::vector<JSTaggedValue> args{};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, args, 4, jsDisplayNames.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDisplayNames::ResolvedOptions(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSTaggedValue> resultObj =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result.GetRawData())));
    // judge whether the properties of the object are the same as those of jsdatetimeformat tag
    JSHandle<JSTaggedValue> localeKey = globalConst->GetHandledLocaleString();
    JSHandle<JSTaggedValue> localeValue(factory->NewFromASCII("de-DE"));
    EXPECT_EQ(JSTaggedValue::SameValue(thread,
        JSObject::GetProperty(thread, resultObj, localeKey).GetValue(), localeValue), true);
    JSHandle<JSTaggedValue> typeKey = globalConst->GetHandledTypeString();
    EXPECT_EQ(JSTaggedValue::SameValue(thread,
        JSObject::GetProperty(thread, resultObj, typeKey).GetValue(), typeValue), true);
}
}
