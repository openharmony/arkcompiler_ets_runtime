/*
 * Copyright (c) 2023 Shenzhen Kaihong Digital Industry Development Co., Ltd.
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

#include "ecmascript/builtins/builtins_segmenter.h"
#include "ecmascript/builtins/builtins_segments.h"
#include "ecmascript/builtins/builtins_segment_iterator.h"

#include "ecmascript/global_env.h"
#include "ecmascript/js_segmenter.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
namespace panda::test {
class BuiltinsSegmenterTest : public BaseTestWithScope<true> {
};

static JSTaggedValue JSSegmenterCreateWithLocaleTest(JSThread *thread, JSHandle<JSTaggedValue> &locale)
{
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetSegmenterFunction());

    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 6); // 6 means 1 call args
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSegmenter::SegmenterConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsJSSegmenter());
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

static JSTaggedValue JSSegmenterCreateWithLocaleAndOptionsTest(JSThread *thread, JSHandle<JSTaggedValue> &locale,
                                                               JSHandle<JSTaggedValue> &granularity)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetSegmenterFunction());
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> granularityKey = thread->GlobalConstants()->GetHandledGranularityString();
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, granularityKey, granularity);

    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8); // 8 means 2 call args
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSegmenter::SegmenterConstructor(ecmaRuntimeCallInfo);
    EXPECT_TRUE(result.IsJSSegmenter());
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

static JSTaggedValue JSSegmentsCreateTest(JSThread *thread, JSHandle<JSTaggedValue> &locale,
                                          JSHandle<JSTaggedValue> &granularity, JSHandle<JSTaggedValue> &stringValue)
{
    JSHandle<JSSegmenter> jsSegmenter =
        JSHandle<JSSegmenter>(thread, JSSegmenterCreateWithLocaleAndOptionsTest(thread, locale, granularity));
    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);  // 6 means 1 call args
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsSegmenter.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, stringValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue segments = BuiltinsSegmenter::Segment(ecmaRuntimeCallInfo);
    EXPECT_TRUE(segments.IsJSSegments());
    TestHelper::TearDownFrame(thread, prev);
    return segments;
}

// new  Intl.Segmenter ( [ locales [ , options ] ] )
HWTEST_F_L0(BuiltinsSegmenterTest, SegmenterConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetSegmenterFunction());

    JSHandle<JSTaggedValue> localesString(factory->NewFromASCII("en-US"));
    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 6); // 6 means 1 call args
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSegmenter::SegmenterConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    EXPECT_TRUE(result.IsJSSegmenter());
}

// Intl.Segmenter.prototype.segment ( string )
HWTEST_F_L0(BuiltinsSegmenterTest, segment)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("zh-cn"));
    JSHandle<JSTaggedValue> granularity(factory->NewFromASCII("word"));
    JSHandle<JSTaggedValue> stringValue(factory->NewFromUtf8("这句话是中文"));
    JSHandle<JSSegmenter> jsSegmenter =
        JSHandle<JSSegmenter>(thread, JSSegmenterCreateWithLocaleAndOptionsTest(thread, locale, granularity));
    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);  // 6 means 1 call args
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsSegmenter.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, stringValue.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSegmenter::Segment(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSSegments());
}

// SupportedLocalesOf("lookup")
HWTEST_F_L0(BuiltinsSegmenterTest, SupportedLocalesOf)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> objFun = env->GetObjectFunction();

    JSHandle<JSTaggedValue> localeMatcherKey = thread->GlobalConstants()->GetHandledLocaleMatcherString();
    JSHandle<JSTaggedValue> localeMatcherValue(factory->NewFromASCII("lookup"));
    JSHandle<JSObject> optionsObj = factory->NewJSObjectByConstructor(JSHandle<JSFunction>(objFun), objFun);
    JSObject::SetProperty(thread, optionsObj, localeMatcherKey, localeMatcherValue);
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("id-u-co-pinyin-de-ID"));

    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8); // 8 means 2 call args
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, locale.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, optionsObj.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue resultArr = BuiltinsSegmenter::SupportedLocalesOf(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSArray> resultHandle(thread, resultArr);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 1U);
    JSHandle<EcmaString> handleEcmaStr(thread, elements->Get(0));
    EXPECT_STREQ("id-u-co-pinyin-de-id", EcmaStringAccessor(handleEcmaStr).ToCString().c_str());
}

// Intl.Segmenter.prototype.resolvedOptions
HWTEST_F_L0(BuiltinsSegmenterTest, ResolvedOptions)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("de-DE"));
    JSHandle<JSSegmenter> jsSegmenter =
        JSHandle<JSSegmenter>(thread, JSSegmenterCreateWithLocaleTest(thread, locale));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsSegmenter.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSegmenter::ResolvedOptions(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSTaggedValue> resultObj =
        JSHandle<JSTaggedValue>(thread, JSTaggedValue(static_cast<JSTaggedType>(result.GetRawData())));
    // judge whether the properties of the object are the same as those of jsdatetimeformat tag
    JSHandle<JSTaggedValue> localeKey = globalConst->GetHandledLocaleString();
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, localeKey).GetValue(), locale), true);
    JSHandle<JSTaggedValue> granularityKey = globalConst->GetHandledGranularityString();
    JSHandle<JSTaggedValue> defaultGranularityValue(factory->NewFromASCII("grapheme"));
    EXPECT_EQ(JSTaggedValue::SameValue(
        JSObject::GetProperty(thread, resultObj, granularityKey).GetValue(), defaultGranularityValue), true);
}

void SegmentsPrototypeCommon(JSThread *thread, JSHandle<JSTaggedValue> &result,
                             std::vector<JSHandle<JSTaggedValue>> &values)
{
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> segmentKey = globalConst->GetHandledSegmentString();
    JSHandle<JSTaggedValue> indexKey = globalConst->GetHandledIndexString();
    JSHandle<JSTaggedValue> inputKey = globalConst->GetHandledInputString();
    JSHandle<JSTaggedValue> isWordLikeKey = globalConst->GetHandledIsWordLikeString();
    JSHandle<JSTaggedValue> segmentValue(JSObject::GetProperty(thread, result, segmentKey).GetValue());
    JSHandle<JSTaggedValue> indexValue(JSObject::GetProperty(thread, result, indexKey).GetValue());
    JSHandle<JSTaggedValue> inputValue(JSObject::GetProperty(thread, result, inputKey).GetValue());
    JSHandle<JSTaggedValue> isWordLikeValue(JSObject::GetProperty(thread, result, isWordLikeKey).GetValue());
    values.push_back(segmentValue);
    values.push_back(indexValue);
    values.push_back(inputValue);
    values.push_back(isWordLikeValue);
}

// %SegmentsPrototype%.containing ( index )
HWTEST_F_L0(BuiltinsSegmenterTest, SegmentsPrototypeContaining_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("zh-cn"));
    JSHandle<JSTaggedValue> granularity(factory->NewFromASCII("sentence"));
    JSHandle<JSTaggedValue> stringValue(factory->NewFromUtf8("这句话是中文。这句还是中文！"));
    JSHandle<JSTaggedValue> segments(thread, JSSegmentsCreateTest(thread, locale, granularity, stringValue));

    std::vector<JSTaggedValue> args{ JSTaggedValue(static_cast<double>(3))};
    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, args, 6, segments.GetTaggedValue());  // 6 means 1 call args

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> result(thread, BuiltinsSegments::Containing(ecmaRuntimeCallInfo));
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result->IsJSObject());
    std::vector<JSHandle<JSTaggedValue>> outValues;
    SegmentsPrototypeCommon(thread, result, outValues);

    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(outValues[0])).ToCString().c_str(),
                 "这句话是中文。");
    EXPECT_EQ(outValues[1]->GetRawData(), JSTaggedValue(0).GetRawData());   // 1:index value
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(outValues[2])).ToCString().c_str(), // 2: input value
                 "这句话是中文。这句还是中文！");
    EXPECT_TRUE(outValues[3]->IsUndefined()); // 2: word link value
}

HWTEST_F_L0(BuiltinsSegmenterTest, SegmentsPrototypeContaining_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("fr"));
    JSHandle<JSTaggedValue> granularity(factory->NewFromASCII("word"));
    JSHandle<JSTaggedValue> stringValue(factory->NewFromUtf8("Que ma joie demeure"));
    JSHandle<JSTaggedValue> segments(thread, JSSegmentsCreateTest(thread, locale, granularity, stringValue));

    std::vector<JSTaggedValue> args{ JSTaggedValue(static_cast<double>(10))};
    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, args, 6, segments.GetTaggedValue());  // 6 means 1 call args

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> result(thread, BuiltinsSegments::Containing(ecmaRuntimeCallInfo));
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result->IsJSObject());

    std::vector<JSHandle<JSTaggedValue>> outValues;
    SegmentsPrototypeCommon(thread, result, outValues);
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(outValues[0])).ToCString().c_str(),
                 "joie");
    EXPECT_EQ(outValues[1]->GetRawData(), JSTaggedValue(7).GetRawData());
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(outValues[2])).ToCString().c_str(),
                 "Que ma joie demeure");
    EXPECT_EQ(outValues[3]->GetRawData(), JSTaggedValue::True().GetRawData());
}

HWTEST_F_L0(BuiltinsSegmenterTest, SegmentsPrototypeContaining_003)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("fr"));
    JSHandle<JSTaggedValue> granularity(factory->NewFromASCII("word"));
    JSHandle<JSTaggedValue> stringValue(factory->NewFromUtf8("Que ma joie demeure"));
    JSHandle<JSTaggedValue> segments(thread, JSSegmentsCreateTest(thread, locale, granularity, stringValue));

    std::vector<JSTaggedValue> args{ JSTaggedValue(static_cast<double>(-10))};
    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, args, 6, segments.GetTaggedValue());  // 6 means 1 call args

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> result(thread, BuiltinsSegments::Containing(ecmaRuntimeCallInfo));
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result->IsUndefined());
}

// %SegmentsPrototype% [ @@iterator ] ( )
HWTEST_F_L0(BuiltinsSegmenterTest, GetSegmentIterator)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("fr"));
    JSHandle<JSTaggedValue> granularity(factory->NewFromASCII("word"));
    JSHandle<JSTaggedValue> stringValue(factory->NewFromUtf8("Que ma joie demeure"));
    JSHandle<JSTaggedValue> segments(thread, JSSegmentsCreateTest(thread, locale, granularity, stringValue));

    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);  // 4 means 0 call args
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(segments.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsSegments::GetSegmentIterator(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSSegmentIterator());
}

// %SegmentIteratorPrototype%.next ( )
HWTEST_F_L0(BuiltinsSegmenterTest, SegmentIteratorNext)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("fr"));
    JSHandle<JSTaggedValue> granularity(factory->NewFromASCII("sentence"));
    JSHandle<JSTaggedValue> stringValue(factory->NewFromUtf8("Que ma joie demeure."));
    JSHandle<JSTaggedValue> segments(thread, JSSegmentsCreateTest(thread, locale, granularity, stringValue));

    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);  // 4 means 0 call args
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(segments.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> iterator(thread, BuiltinsSegments::GetSegmentIterator(ecmaRuntimeCallInfo));
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(iterator->IsJSSegmentIterator());

    auto ecmaRuntimeCallInfo1 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);  // 4 means 0 call args
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(iterator.GetTaggedValue());

    [[maybe_unused]] auto prev1 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSHandle<JSTaggedValue> result1(thread, BuiltinsSegmentIterator::Next(ecmaRuntimeCallInfo1));
    TestHelper::TearDownFrame(thread, prev1);
    EXPECT_TRUE(result1->IsJSObject());
    auto globalConst = thread->GlobalConstants();
    JSHandle<JSTaggedValue> valueKey = globalConst->GetHandledValueString();
    JSHandle<JSTaggedValue> doneKey = globalConst->GetHandledDoneString();
    JSHandle<JSTaggedValue> value1(JSObject::GetProperty(thread, result1, valueKey).GetValue());
    JSHandle<JSTaggedValue> done1(JSObject::GetProperty(thread, result1, doneKey).GetValue());
    EXPECT_TRUE(value1->IsJSObject());
    JSHandle<JSTaggedValue> segmentKey = globalConst->GetHandledSegmentString();
    JSHandle<JSTaggedValue> segmentValue(JSObject::GetProperty(thread, value1, segmentKey).GetValue());
    EXPECT_STREQ(EcmaStringAccessor(JSHandle<EcmaString>::Cast(segmentValue)).ToCString().c_str(),
                 "Que ma joie demeure.");
    EXPECT_FALSE(done1->ToBoolean());

    auto ecmaRuntimeCallInfo2 =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);  // 4 means 0 call args
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(iterator.GetTaggedValue());

    [[maybe_unused]] auto prev2 = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSHandle<JSTaggedValue> result2(thread, BuiltinsSegmentIterator::Next(ecmaRuntimeCallInfo2));
    TestHelper::TearDownFrame(thread, prev2);
    EXPECT_TRUE(result2->IsJSObject());
    JSHandle<JSTaggedValue> value2(JSObject::GetProperty(thread, result2, valueKey).GetValue());
    JSHandle<JSTaggedValue> done2(JSObject::GetProperty(thread, result2, doneKey).GetValue());
    EXPECT_TRUE(value2->IsUndefined());
    EXPECT_TRUE(done2->ToBoolean());
}
}
