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

#include "ecmascript/builtins/builtins_plural_rules.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_plural_rules.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsPluralRulesTest : public BaseTestWithScope<true> {
};

// new DateTimeFormat(newTarget is defined)
HWTEST_F_L0(BuiltinsPluralRulesTest, PluralRulesConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetPluralRulesFunction());

    JSHandle<JSTaggedValue> localesString(factory->NewFromASCII("en-US"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
    // option tag is default value
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsPluralRules::PluralRulesConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsJSPluralRules());
}

static JSTaggedValue JSPluralRulesCreateWithLocaleTest(JSThread *thread, JSHandle<JSTaggedValue> &locale,
                                                       JSHandle<JSTaggedValue> &typeValue)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetPluralRulesFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);

    JSHandle<JSTaggedValue> typeKey = thread->GlobalConstants()->GetHandledTypeString();
    JSObject::SetProperty(thread, optionsObj, typeKey, typeValue);

    JSHandle<JSTaggedValue> localesString = locale;
    std::vector<JSTaggedValue> args{ localesString.GetTaggedValue(), optionsObj.GetTaggedValue()};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, newTarget, args, 8);

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsPluralRules::PluralRulesConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsJSPluralRules());
    return result;
}

static void SelectTest(JSThread *thread, std::string_view data, std::string_view expectStr, int v)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII(data));
    JSHandle<JSPluralRules> jsPluralRules =
        JSHandle<JSPluralRules>(thread, JSPluralRulesCreateWithLocaleTest(thread, locale, typeValue));

    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(v));
    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);  // 6 : argv length
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsPluralRules::Select(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsString());
    JSHandle<EcmaString> handleEcmaStr(thread, result);
    EXPECT_STREQ(expectStr.data(), EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

// select(0, type(cardinal))
HWTEST_F_L0(BuiltinsPluralRulesTest, Select_001)
{
    SelectTest(thread, "cardinal", "other", 0);
}

// select(1, type(cardinal))
HWTEST_F_L0(BuiltinsPluralRulesTest, Select_002)
{
    SelectTest(thread, "cardinal", "one", 1);
}

// select(2, type(cardinal))
HWTEST_F_L0(BuiltinsPluralRulesTest, Select_003)
{
    SelectTest(thread, "cardinal", "other", 2);
}

// select(3, type(ordinal))
HWTEST_F_L0(BuiltinsPluralRulesTest, Select_004)
{
    SelectTest(thread, "ordinal", "few", 3);
}

// select(2, type(ordinal))
HWTEST_F_L0(BuiltinsPluralRulesTest, Select_005)
{
    SelectTest(thread, "ordinal", "two", 2);
}

// select(0, type(ordinal))
HWTEST_F_L0(BuiltinsPluralRulesTest, Select_006)
{
    SelectTest(thread, "ordinal", "other", 0);
}

// select(1, type(ordinal))
HWTEST_F_L0(BuiltinsPluralRulesTest, Select_007)
{
    SelectTest(thread, "ordinal", "one", 1);
}

HWTEST_F_L0(BuiltinsPluralRulesTest, SupportedLocalesOf)
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
    JSTaggedValue resultArr = BuiltinsPluralRules::SupportedLocalesOf(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, resultArr);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 1U);
    JSHandle<EcmaString> handleEcmaStr(thread, elements->Get(0));
    EXPECT_STREQ("id-u-co-pinyin-de-id", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

HWTEST_F_L0(BuiltinsPluralRulesTest, ResolvedOptions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSTaggedValue> typeValue(factory->NewFromASCII("ordinal"));
    JSHandle<JSPluralRules> jsPluralRules =
        JSHandle<JSPluralRules>(thread, JSPluralRulesCreateWithLocaleTest(thread, locale, typeValue));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsPluralRules.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsPluralRules::ResolvedOptions(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSTaggedValue> resultObj =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result.GetRawData())));
    // judge whether the properties of the object are the same as those of jspluralrulesformat tag
    JSHandle<JSTaggedValue> localeKey = globalConst->GetHandledLocaleString();
    JSHandle<JSTaggedValue> localeValue(factory->NewFromASCII("en"));
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, localeKey).GetValue(), localeValue), true);
    JSHandle<JSTaggedValue> typeKey = globalConst->GetHandledTypeString();
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, typeKey).GetValue(), typeValue), true);
}
} // namespace panda::test
