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

#include "ecmascript/builtins/builtins_collator.h"

#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_collator.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using BuiltinsArray = ecmascript::builtins::BuiltinsArray;
class BuiltinsCollatorTest : public BaseTestWithScope<true> {
};

static JSTaggedValue CollatorConstructor(JSThread *thread, std::vector<JSTaggedValue>& args,
    JSHandle<JSFunction>& target)
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*target), 8);
    ecmaRuntimeCallInfo->SetFunction(target.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    for (size_t i = 0; i < args.size(); i++) {
        ecmaRuntimeCallInfo->SetCallArg(i, args[i]);
    }

    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsCollator::CollatorConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

enum class AlgorithmType {
    COLLATOR_SUPPORTED_LOCALES_OF,
    COLLATOR_RESOLVED_OPTIONS,
    COLLATOR_COMPARE,
    ARRAY_SORT,
    ARRAY_JOIN,
    ARRAY_TOSTR,
};

static JSTaggedValue CollatorAlgorithm(JSThread *thread, std::vector<JSTaggedValue>& args, int32_t maxArgLen,
    AlgorithmType type, JSTaggedValue thisValue = JSTaggedValue::Undefined())
{
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), maxArgLen);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(thisValue);
    for (size_t i = 0; i < args.size(); i++) {
        ecmaRuntimeCallInfo->SetCallArg(i, args[i]);
    }
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result;
    switch (type) {
        case AlgorithmType::COLLATOR_SUPPORTED_LOCALES_OF:
            result = BuiltinsCollator::SupportedLocalesOf(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::COLLATOR_RESOLVED_OPTIONS:
            result = BuiltinsCollator::ResolvedOptions(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::COLLATOR_COMPARE:
            result = BuiltinsCollator::Compare(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::ARRAY_SORT:
            result = BuiltinsArray::Sort(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::ARRAY_JOIN:
            result = BuiltinsArray::Join(ecmaRuntimeCallInfo);
            break;
        case AlgorithmType::ARRAY_TOSTR:
            result = BuiltinsArray::ToString(ecmaRuntimeCallInfo);
            break;
        default:
            break;
    }
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

HWTEST_F_L0(BuiltinsCollatorTest, CollatorConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetCollatorFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> usageKey = thread->GlobalConstants()->GetHandledUsageString();
    JSHandle<JSTaggedValue> localeMatcherKey = thread->GlobalConstants()->GetHandledLocaleMatcherString();
    JSHandle<JSTaggedValue> numericKey = thread->GlobalConstants()->GetHandledNumericString();
    JSHandle<JSTaggedValue> caseFirstKey = thread->GlobalConstants()->GetHandledCaseFirstString();
    JSHandle<JSTaggedValue> sensitivityKey = thread->GlobalConstants()->GetHandledSensitivityString();
    JSHandle<JSTaggedValue> ignorePunctuationKey = thread->GlobalConstants()->GetHandledIgnorePunctuationString();

    JSHandle<JSTaggedValue> usageValue(factory->NewFromASCII("search"));
    JSHandle<JSTaggedValue> localeMatcherValue(factory->NewFromASCII("lookup"));
    JSHandle<JSTaggedValue> numericValue(factory->NewFromASCII("true"));
    JSHandle<JSTaggedValue> caseFirstValue(factory->NewFromASCII("lower"));
    JSHandle<JSTaggedValue> sensitivityValue(factory->NewFromASCII("variant"));
    JSHandle<JSTaggedValue> ignorePunctuationValue(factory->NewFromASCII("true"));
    JSHandle<JSTaggedValue> localesString(factory->NewFromASCII("en-Latn-US"));

    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, usageKey, usageValue);
    JSObject::SetProperty(thread, optionsObj, localeMatcherKey, localeMatcherValue);
    JSObject::SetProperty(thread, optionsObj, caseFirstKey, caseFirstValue);
    JSObject::SetProperty(thread, optionsObj, sensitivityKey, sensitivityValue);
    JSObject::SetProperty(thread, optionsObj, ignorePunctuationKey, ignorePunctuationValue);
    JSObject::SetProperty(thread, optionsObj, numericKey, numericValue);

    std::vector<JSTaggedValue> vals{localesString.GetTaggedValue(), optionsObj.GetTaggedValue()};
    auto result = CollatorConstructor(thread, vals, newTarget);

    EXPECT_TRUE(result.IsJSCollator());
}

static JSTaggedValue JSCollatorCreateWithLocaleTest(JSThread *thread, JSHandle<JSTaggedValue> &locale)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetCollatorFunction());

    JSHandle<JSTaggedValue> localesString = locale;
    std::vector<JSTaggedValue> vals{localesString.GetTaggedValue(), JSTaggedValue::Undefined()};
    auto result = CollatorConstructor(thread, vals, newTarget);

    EXPECT_TRUE(result.IsJSCollator());
    return result;
}

static JSTaggedValue JSCollatorCreateWithLocaleAndOptionsTest(JSThread *thread, JSHandle<JSTaggedValue> &locale)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetCollatorFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> localesString = locale;
    JSHandle<JSTaggedValue> usageKey = thread->GlobalConstants()->GetHandledUsageString();
    JSHandle<JSTaggedValue> sensitivityKey = thread->GlobalConstants()->GetHandledSensitivityString();
    JSHandle<JSTaggedValue> usageValue(factory->NewFromASCII("search"));
    JSHandle<JSTaggedValue> sensitivityValue(factory->NewFromASCII("base"));

    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, usageKey, usageValue);
    JSObject::SetProperty(thread, optionsObj, sensitivityKey, sensitivityValue);

    std::vector<JSTaggedValue> vals{localesString.GetTaggedValue(), optionsObj.GetTaggedValue()};
    auto result = CollatorConstructor(thread, vals, newTarget);

    EXPECT_TRUE(result.IsJSCollator());
    return result;
}

// compare with sort(de)
HWTEST_F_L0(BuiltinsCollatorTest, Compare_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de"));
    JSHandle<JSCollator> jsCollator = JSHandle<JSCollator>(thread, JSCollatorCreateWithLocaleTest(thread, locale));

    std::vector<JSTaggedValue> vals{};
    auto result1 = CollatorAlgorithm(thread, vals, 4, AlgorithmType::COLLATOR_COMPARE, jsCollator.GetTaggedValue());
    JSHandle<JSFunction> jsFunction(thread, result1);

    JSArray *jsArray =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> jsObject(thread, jsArray);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> value0(factory->NewFromASCII("Z"));
    PropertyDescriptor desc0(thread, value0, true, true, true);
    JSArray::DefineOwnProperty(thread, jsObject, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("a"));
    PropertyDescriptor desc1(thread, value1, true, true, true);
    JSArray::DefineOwnProperty(thread, jsObject, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value2(factory->NewFromUtf8("ä"));
    PropertyDescriptor desc2(thread, value2, true, true, true);
    JSArray::DefineOwnProperty(thread, jsObject, key2, desc2);

    std::vector<JSTaggedValue> arrayVals{jsFunction.GetTaggedValue()};
    auto result2 = CollatorAlgorithm(thread, arrayVals, 6, AlgorithmType::ARRAY_SORT, jsObject.GetTaggedValue());

    JSHandle<JSTaggedValue> resultArr =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result2.GetRawData())));
    EXPECT_EQ(JSTaggedValue::SameValue(thread, JSArray::GetProperty(thread, resultArr, key0).GetValue(), value1), true);
    EXPECT_EQ(JSTaggedValue::SameValue(thread, JSArray::GetProperty(thread, resultArr, key1).GetValue(), value2), true);
    EXPECT_EQ(JSTaggedValue::SameValue(thread, JSArray::GetProperty(thread, resultArr, key2).GetValue(), value0), true);
}

// // compare with sort(sv)
HWTEST_F_L0(BuiltinsCollatorTest, Compare_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("sv"));
    JSHandle<JSCollator> jsCollator = JSHandle<JSCollator>(thread, JSCollatorCreateWithLocaleTest(thread, locale));

    std::vector<JSTaggedValue> vals{};
    auto result1 = CollatorAlgorithm(thread, vals, 4, AlgorithmType::COLLATOR_COMPARE, jsCollator.GetTaggedValue());

    JSHandle<JSFunction> jsFunction(thread, result1);
    JSArray *jsArray =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> jsObject(thread, jsArray);

    JSHandle<JSTaggedValue> key0(thread, JSTaggedValue(0));
    JSHandle<JSTaggedValue> value0(factory->NewFromASCII("Z"));
    PropertyDescriptor desc0(thread, value0, true, true, true);
    JSArray::DefineOwnProperty(thread, jsObject, key0, desc0);
    JSHandle<JSTaggedValue> key1(thread, JSTaggedValue(1));
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("a"));
    PropertyDescriptor desc1(thread, value1, true, true, true);
    JSArray::DefineOwnProperty(thread, jsObject, key1, desc1);
    JSHandle<JSTaggedValue> key2(thread, JSTaggedValue(2));
    JSHandle<JSTaggedValue> value2(factory->NewFromUtf8("ä"));
    PropertyDescriptor desc2(thread, value2, true, true, true);
    JSArray::DefineOwnProperty(thread, jsObject, key2, desc2);

    std::vector<JSTaggedValue> arrayVals{jsFunction.GetTaggedValue()};
    auto result2 = CollatorAlgorithm(thread, arrayVals, 6, AlgorithmType::ARRAY_SORT, jsObject.GetTaggedValue());
    JSHandle<JSObject> resultObj(thread, result2);

    JSHandle<EcmaString> str = thread->GetEcmaVM()->GetFactory()->NewFromUtf8("a,Z,ä");
    std::vector<JSTaggedValue> arrayVals2{};
    auto result = CollatorAlgorithm(thread, arrayVals2, 4, AlgorithmType::ARRAY_JOIN, resultObj.GetTaggedValue());

    JSHandle<EcmaString> resultHandle(thread, reinterpret_cast<EcmaString *>(result.GetRawData()));
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, resultHandle, str), 0);
}

// compare with options("search")
HWTEST_F_L0(BuiltinsCollatorTest, Compare_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("sv"));
    JSHandle<JSCollator> jsCollator =
        JSHandle<JSCollator>(thread, JSCollatorCreateWithLocaleAndOptionsTest(thread, locale));

    std::vector<JSTaggedValue> vals{};
    auto result1 = CollatorAlgorithm(thread, vals, 4, AlgorithmType::COLLATOR_COMPARE, jsCollator.GetTaggedValue());

    JSHandle<JSFunction> jsFunction(thread, result1);
    JSArray *jsArray =
        JSArray::Cast(JSArray::ArrayCreate(thread, JSTaggedNumber(0)).GetTaggedValue().GetTaggedObject());
    JSHandle<JSObject> jsObject(thread, jsArray);

    JSHandle<JSTaggedValue> value0(factory->NewFromUtf8("Congrès"));
    JSHandle<JSTaggedValue> value1(factory->NewFromASCII("congres"));
    PropertyDescriptor desc(thread, JSHandle<JSTaggedValue>(jsFunction), true, true, true);
    JSHandle<JSTaggedValue> joinKey(factory->NewFromASCII("join"));
    JSArray::DefineOwnProperty(thread, jsObject, joinKey, desc);

    std::vector<JSTaggedValue> arrayVals{value0.GetTaggedValue(), value1.GetTaggedValue()};
    auto result2 = CollatorAlgorithm(thread, arrayVals, 8, AlgorithmType::ARRAY_TOSTR, jsObject.GetTaggedValue());

    JSHandle<JSTaggedValue> resultHandle(thread, result2);
    EXPECT_EQ(resultHandle->GetInt(), 0); // Congrès and congres is matching
}

HWTEST_F_L0(BuiltinsCollatorTest, ResolvedOptions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de"));
    JSHandle<JSCollator> jsCollator = JSHandle<JSCollator>(thread, JSCollatorCreateWithLocaleTest(thread, locale));

    std::vector<JSTaggedValue> vals{};
    auto result =
        CollatorAlgorithm(thread, vals, 4, AlgorithmType::COLLATOR_RESOLVED_OPTIONS, jsCollator.GetTaggedValue());

    JSHandle<JSTaggedValue> resultObj =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result.GetRawData())));
    // judge whether the properties of the object are the same as those of jscollator tag
    JSHandle<JSTaggedValue> localeKey = globalConst->GetHandledLocaleString();
    EXPECT_EQ(JSTaggedValue::SameValue(thread, JSObject::GetProperty(thread, resultObj, localeKey).GetValue(), locale),
        true);
    JSHandle<JSTaggedValue> usageKey = globalConst->GetHandledUsageString();
    JSHandle<JSTaggedValue> defaultUsageValue(factory->NewFromASCII("sort"));
    EXPECT_EQ(JSTaggedValue::SameValue(thread, JSObject::GetProperty(thread, resultObj, usageKey).GetValue(),
                                       defaultUsageValue),
              true);
    JSHandle<JSTaggedValue> handledCaseFirstKey = globalConst->GetHandledCaseFirstString();
    JSHandle<JSTaggedValue> handledCaseFirstValue(factory->NewFromASCII("upper"));
    EXPECT_EQ(JSTaggedValue::SameValue(thread, JSObject::GetProperty(thread, resultObj, handledCaseFirstKey).GetValue(),
                                       handledCaseFirstValue),
              true);
}

HWTEST_F_L0(BuiltinsCollatorTest, SupportedLocalesOf)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> localeMatcherKey = thread->GlobalConstants()->GetHandledLocaleMatcherString();
    JSHandle<JSTaggedValue> localeMatcherValue(factory->NewFromASCII("lookup"));
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, localeMatcherKey, localeMatcherValue);
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("id-u-co-pinyin-de-ID"));

    std::vector<JSTaggedValue> vals{locale.GetTaggedValue(), optionsObj.GetTaggedValue()};
    auto resultArr = CollatorAlgorithm(thread, vals, 8, AlgorithmType::COLLATOR_SUPPORTED_LOCALES_OF);

    JSHandle<JSArray> resultHandle(thread, resultArr);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements(thread));
    EXPECT_EQ(elements->GetLength(), 1U);
    JSHandle<EcmaString> handleEcmaStr(thread, elements->Get(thread, 0));
    EXPECT_STREQ("id-u-co-pinyin-de-id", EcmaStringAccessor(handleEcmaStr).ToCString(thread).c_str());
}
}  // namespace panda::test
