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

#include "ecmascript/js_regexp.h"

#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/ecma_runtime_call_info.h"
#include "ecmascript/ecma_string.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/js_hclass.h"
#include "ecmascript/js_object-inl.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/regexp/regexp_parser_cache.h"
#include "ecmascript/object_factory.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
class BuiltinsRegExpTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(BuiltinsRegExpTest, RegExpConstructor1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern = thread->GetEcmaVM()->GetFactory()->NewFromASCII("\\w+");
    JSHandle<EcmaString> flags = thread->GetEcmaVM()->GetFactory()->NewFromASCII("i");
    JSTaggedValue result = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern, flags);

    // ASSERT IsRegExp()
    JSHandle<JSTaggedValue> regexpObject(thread, result);
    ASSERT_TRUE(JSObject::IsRegExp(thread, regexpObject));

    JSHandle<JSRegExp> jsRegexp(thread, JSRegExp::Cast(regexpObject->GetTaggedObject()));
    JSHandle<JSTaggedValue> originalSource(thread, jsRegexp->GetOriginalSource(thread));
    uint8_t flagsBits = static_cast<uint8_t>(jsRegexp->GetOriginalFlags(thread).GetInt());
    JSHandle<JSTaggedValue> originalFlags(thread, BuiltinsRegExp::FlagsBitsToString(thread, flagsBits));
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, JSHandle<EcmaString>(originalSource), pattern), 0);
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, JSHandle<EcmaString>(originalFlags), flags), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, RegExpConstructor2)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern = thread->GetEcmaVM()->GetFactory()->NewFromASCII("\\w+");
    JSHandle<EcmaString> flags = thread->GetEcmaVM()->GetFactory()->NewFromASCII("i");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern, flags);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> regexp(env->GetRegExpFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*regexp), 8);
    ecmaRuntimeCallInfo->SetFunction(regexp.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // invoke RegExpConstructor method
    JSTaggedValue result2 = BuiltinsRegExp::RegExpConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    // ASSERT IsRegExp()
    JSHandle<JSTaggedValue> regexpObject(thread, result2);
    ASSERT_TRUE(JSObject::IsRegExp(thread, regexpObject));

    JSHandle<JSRegExp> jsRegexp(thread, JSRegExp::Cast(regexpObject->GetTaggedObject()));
    JSHandle<JSTaggedValue> originalSource(thread, jsRegexp->GetOriginalSource(thread));
    uint8_t flagsBits = static_cast<uint8_t>(jsRegexp->GetOriginalFlags(thread).GetInt());
    JSHandle<JSTaggedValue> originalFlags(thread, BuiltinsRegExp::FlagsBitsToString(thread, flagsBits));
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, JSHandle<EcmaString>(originalSource), pattern), 0);
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, JSHandle<EcmaString>(originalFlags), flags), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, RegExpConstructor3)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("\\w+");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("i");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> regexp(env->GetRegExpFunction());
    JSHandle<JSObject> globalObject(thread, env->GetGlobalObject());
    JSHandle<EcmaString> flags2 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("gi");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*regexp), 8);
    ecmaRuntimeCallInfo->SetFunction(regexp.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(globalObject.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, flags2.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // invoke RegExpConstructor method
    JSTaggedValue result2 = BuiltinsRegExp::RegExpConstructor(ecmaRuntimeCallInfo);

    // ASSERT IsRegExp()
    JSHandle<JSTaggedValue> regexpObject(thread, result2);
    ASSERT_TRUE(JSObject::IsRegExp(thread, regexpObject));

    JSHandle<JSRegExp> jsRegexp(thread, JSRegExp::Cast(regexpObject->GetTaggedObject()));
    JSHandle<JSTaggedValue> originalSource(thread, jsRegexp->GetOriginalSource(thread));
    uint8_t flagsBits = static_cast<uint8_t>(jsRegexp->GetOriginalFlags(thread).GetInt());
    JSHandle<JSTaggedValue> originalFlags(thread, BuiltinsRegExp::FlagsBitsToString(thread, flagsBits));
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, JSHandle<EcmaString>(originalSource), pattern1), 0);
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, JSHandle<EcmaString>(originalFlags), flags2), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, GetSource1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("i");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSTaggedValue> result1Handle(thread, result1);

    // invoke GetSource method
    JSHandle<JSTaggedValue> source(
        thread, thread->GetEcmaVM()->GetFactory()->NewFromASCII("source").GetTaggedValue());
    JSHandle<JSTaggedValue> sourceResult(JSObject::GetProperty(thread, result1Handle, source).GetValue());

    JSHandle<EcmaString> expect = thread->GetEcmaVM()->GetFactory()->NewFromASCII("(?:)");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, JSHandle<EcmaString>(sourceResult), expect), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, GetSource2)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("/w+");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("i");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSTaggedValue> result1Handle(thread, result1);

    // invoke GetSource method
    JSHandle<JSTaggedValue> source(thread->GetEcmaVM()->GetFactory()->NewFromASCII("source"));
    JSHandle<JSTaggedValue> sourceResult(JSObject::GetProperty(thread, result1Handle, source).GetValue());

    JSHandle<EcmaString> expect = thread->GetEcmaVM()->GetFactory()->NewFromASCII("\\/w+");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, JSHandle<EcmaString>(sourceResult), expect), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, Get)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("\\w+");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("gimuy");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSTaggedValue> result1Handle(thread, result1);

    JSHandle<JSTaggedValue> global(thread->GetEcmaVM()->GetFactory()->NewFromASCII("global"));
    JSTaggedValue taggedGlobalResult =
        JSTaggedValue(JSObject::GetProperty(thread, result1Handle, global).GetValue().GetTaggedValue());
    ASSERT_EQ(taggedGlobalResult.GetRawData(), JSTaggedValue::True().GetRawData());

    JSHandle<JSTaggedValue> ignoreCase(thread->GetEcmaVM()->GetFactory()->NewFromASCII("ignoreCase"));
    JSTaggedValue taggedIgnoreCaseResult =
        JSTaggedValue(JSObject::GetProperty(thread, result1Handle, ignoreCase).GetValue().GetTaggedValue());
    ASSERT_EQ(taggedIgnoreCaseResult.GetRawData(), JSTaggedValue::True().GetRawData());

    JSHandle<JSTaggedValue> multiline(thread->GetEcmaVM()->GetFactory()->NewFromASCII("multiline"));
    JSTaggedValue taggedMultilineResult =
        JSTaggedValue(JSObject::GetProperty(thread, result1Handle, multiline).GetValue().GetTaggedValue());
    ASSERT_EQ(taggedMultilineResult.GetRawData(), JSTaggedValue::True().GetRawData());

    JSHandle<JSTaggedValue> sticky(thread->GetEcmaVM()->GetFactory()->NewFromASCII("sticky"));
    JSTaggedValue taggedStickyResult =
        JSTaggedValue(JSObject::GetProperty(thread, result1Handle, sticky).GetValue().GetTaggedValue());
    ASSERT_EQ(taggedStickyResult.GetRawData(), JSTaggedValue::True().GetRawData());

    JSHandle<JSTaggedValue> unicode(thread->GetEcmaVM()->GetFactory()->NewFromASCII("unicode"));
    JSTaggedValue taggedUnicodeResult =
        JSTaggedValue(JSObject::GetProperty(thread, result1Handle, unicode).GetValue().GetTaggedValue());
    ASSERT_EQ(taggedUnicodeResult.GetRawData(), JSTaggedValue::True().GetRawData());
}

HWTEST_F_L0(BuiltinsRegExpTest, GetFlags)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("\\w+");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("imuyg");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSTaggedValue> result1Handle(thread, result1);

    // invoke GetFlags method
    JSHandle<JSTaggedValue> flags(thread->GetEcmaVM()->GetFactory()->NewFromASCII("flags"));
    JSHandle<JSTaggedValue> flagsResult(JSObject::GetProperty(thread, result1Handle, flags).GetValue());

    JSHandle<EcmaString> expectResult = thread->GetEcmaVM()->GetFactory()->NewFromASCII("gimuy");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, JSHandle<EcmaString>(flagsResult), expectResult), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, toString)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("\\w+");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("imuyg");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // invoke ToString method
    JSTaggedValue toStringResult = BuiltinsRegExp::ToString(ecmaRuntimeCallInfo);
    ASSERT_TRUE(toStringResult.IsString());
    JSHandle<JSTaggedValue> toStringResultHandle(thread, toStringResult);
    JSHandle<EcmaString> expectResult = thread->GetEcmaVM()->GetFactory()->NewFromASCII("/\\w+/gimuy");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance,
        JSHandle<EcmaString>(toStringResultHandle), expectResult), 0);
}

void ExecCommon(JSThread* thread, EcmaVM* instance, JSHandle<JSTaggedValue>& execResult,
    JSHandle<EcmaString>& inputString, std::vector<JSHandle<EcmaString>>& result)
{
    JSHandle<JSTaggedValue> input(thread->GetEcmaVM()->GetFactory()->NewFromASCII("input"));
    JSHandle<JSTaggedValue> inputHandle(JSObject::GetProperty(thread, execResult, input).GetValue());
    JSHandle<EcmaString> outputInput = JSTaggedValue::ToString(thread, inputHandle);
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, outputInput, inputString), 0);

    JSHandle<JSTaggedValue> zero(thread->GetEcmaVM()->GetFactory()->NewFromASCII("0"));
    JSHandle<JSTaggedValue> zeroHandle(JSObject::GetProperty(thread, execResult, zero).GetValue());
    JSHandle<EcmaString> outputZero = JSTaggedValue::ToString(thread, zeroHandle);
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, outputZero, result[0]), 0);

    JSHandle<JSTaggedValue> first(thread->GetEcmaVM()->GetFactory()->NewFromASCII("1"));
    JSHandle<JSTaggedValue> oneHandle(JSObject::GetProperty(thread, execResult, first).GetValue());
    JSHandle<EcmaString> outputOne = JSTaggedValue::ToString(thread, oneHandle);
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, outputOne, result[1]), 0); // 1: second value

    JSHandle<JSTaggedValue> second(thread->GetEcmaVM()->GetFactory()->NewFromASCII("2"));
    JSHandle<JSTaggedValue> twoHandle(JSObject::GetProperty(thread, execResult, second).GetValue());
    JSHandle<EcmaString> outputTwo = JSTaggedValue::ToString(thread, twoHandle);
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, outputTwo, result[2]), 0); // 2: third value
}

HWTEST_F_L0(BuiltinsRegExpTest, Exec1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("quick\\s(brown).+?(jumps)");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("ig");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("The Quick Brown Fox Jumps Over The Lazy Dog");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // invoke Exec method
    JSTaggedValue results = BuiltinsRegExp::Exec(ecmaRuntimeCallInfo);

    JSHandle<JSTaggedValue> execResult(thread, results);
    JSHandle<EcmaString> resultZero =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("Quick Brown Fox Jumps");
    JSHandle<EcmaString> resultOne = thread->GetEcmaVM()->GetFactory()->NewFromASCII("Brown");
    JSHandle<EcmaString> resultTwo = thread->GetEcmaVM()->GetFactory()->NewFromASCII("Jumps");

    JSHandle<JSTaggedValue> index(thread->GetEcmaVM()->GetFactory()->NewFromASCII("index"));
    JSHandle<JSTaggedValue> indexHandle(JSObject::GetProperty(thread, execResult, index).GetValue());
    uint32_t resultIndex = JSTaggedValue::ToUint32(thread, indexHandle);
    ASSERT_TRUE(resultIndex == 4U);

    std::vector<JSHandle<EcmaString>> result{resultZero, resultOne, resultTwo};
    ExecCommon(thread, instance, execResult, inputString, result);
    JSHandle<JSTaggedValue> regexp = JSHandle<JSTaggedValue>::Cast(value);
    JSHandle<JSTaggedValue> lastIndexHandle(thread->GetEcmaVM()->GetFactory()->NewFromASCII("lastIndex"));
    JSHandle<JSTaggedValue> lastIndexObj(JSObject::GetProperty(thread, regexp, lastIndexHandle).GetValue());
    int lastIndex = lastIndexObj->GetInt();
    ASSERT_TRUE(lastIndex == 25);
}

HWTEST_F_L0(BuiltinsRegExpTest, Exec2)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("((1)|(12))((3)|(23))");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("ig");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString = thread->GetEcmaVM()->GetFactory()->NewFromASCII("123");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // invoke Exec method
    JSTaggedValue results = BuiltinsRegExp::Exec(ecmaRuntimeCallInfo);

    JSHandle<JSTaggedValue> execResult(thread, results);
    JSHandle<EcmaString> resultZero = thread->GetEcmaVM()->GetFactory()->NewFromASCII("123");
    JSHandle<EcmaString> resultOne = thread->GetEcmaVM()->GetFactory()->NewFromASCII("1");
    JSHandle<EcmaString> resultTwo = thread->GetEcmaVM()->GetFactory()->NewFromASCII("1");
    JSHandle<EcmaString> resultFour = thread->GetEcmaVM()->GetFactory()->NewFromASCII("23");
    JSHandle<EcmaString> resultSix = thread->GetEcmaVM()->GetFactory()->NewFromASCII("23");

    JSHandle<JSTaggedValue> index(thread->GetEcmaVM()->GetFactory()->NewFromASCII("index"));
    JSHandle<JSTaggedValue> indexHandle(JSObject::GetProperty(thread, execResult, index).GetValue());
    uint32_t resultIndex = JSTaggedValue::ToUint32(thread, indexHandle);
    ASSERT_TRUE(resultIndex == 0U);

    std::vector<JSHandle<EcmaString>> result{resultZero, resultOne, resultTwo};
    ExecCommon(thread, instance, execResult, inputString, result);
    JSHandle<JSTaggedValue> regexp = JSHandle<JSTaggedValue>::Cast(value);
    JSHandle<JSTaggedValue> lastIndexHandle(thread->GetEcmaVM()->GetFactory()->NewFromASCII("lastIndex"));
    JSHandle<JSTaggedValue> lastIndexObj(JSObject::GetProperty(thread, regexp, lastIndexHandle).GetValue());
    int lastIndex = lastIndexObj->GetInt();
    ASSERT_TRUE(lastIndex == 3);

    JSHandle<JSTaggedValue> third(thread->GetEcmaVM()->GetFactory()->NewFromASCII("3"));
    JSHandle<JSTaggedValue> thirdHandle(JSObject::GetProperty(thread, execResult, third).GetValue());
    ASSERT_TRUE(thirdHandle->IsUndefined());

    JSHandle<JSTaggedValue> four(thread->GetEcmaVM()->GetFactory()->NewFromASCII("4"));
    JSHandle<JSTaggedValue> fourHandle(JSObject::GetProperty(thread, execResult, four).GetValue());
    JSHandle<EcmaString> outputFour = JSTaggedValue::ToString(thread, fourHandle);
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, outputFour, resultFour), 0);

    JSHandle<JSTaggedValue> five(thread->GetEcmaVM()->GetFactory()->NewFromASCII("5"));
    JSHandle<JSTaggedValue> fiveHandle(JSObject::GetProperty(thread, execResult, five).GetValue());
    ASSERT_TRUE(fiveHandle->IsUndefined());

    JSHandle<JSTaggedValue> six(thread->GetEcmaVM()->GetFactory()->NewFromASCII("6"));
    JSHandle<JSTaggedValue> sixHandle(JSObject::GetProperty(thread, execResult, six).GetValue());
    JSHandle<EcmaString> outputSix = JSTaggedValue::ToString(thread, sixHandle);
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, outputSix, resultSix), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, Match1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("quick\\s(brown).+?(jumps)");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("iug");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("The Quick Brown Fox Jumps Over The Lazy Dog");
    std::vector<JSTaggedValue> args{inputString.GetTaggedValue()};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, args, 6, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // invoke Match method
    JSTaggedValue matchResults = BuiltinsRegExp::Match(ecmaRuntimeCallInfo);

    JSHandle<JSTaggedValue> matchResult(thread, matchResults);
    JSHandle<JSTaggedValue> zero(thread->GetEcmaVM()->GetFactory()->NewFromASCII("0"));
    JSHandle<EcmaString> resultZero =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("Quick Brown Fox Jumps");
    JSHandle<JSTaggedValue> zeroHandle(JSObject::GetProperty(thread, matchResult, zero).GetValue());
    JSHandle<EcmaString> outputZero = JSTaggedValue::ToString(thread, zeroHandle);
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, outputZero, resultZero), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, Test1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("quick\\s(brown).+?(jumps)");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("iug");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("The Quick Brown Fox Jumps Over The Lazy Dog");
    std::vector<JSTaggedValue> args{inputString.GetTaggedValue()};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, args, 6, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // invoke Test method
    JSTaggedValue testResult = BuiltinsRegExp::Test(ecmaRuntimeCallInfo);
    ASSERT_EQ(testResult.GetRawData(), JSTaggedValue::True().GetRawData());
}

HWTEST_F_L0(BuiltinsRegExpTest, Search1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("quick\\s(brown).+?(jumps)");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("iug");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("The Quick Brown Fox Jumps Over The Lazy Dog");
    std::vector<JSTaggedValue> args{inputString.GetTaggedValue()};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, args, 6, value.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // invoke Search method
    JSTaggedValue searchResult = BuiltinsRegExp::Search(ecmaRuntimeCallInfo);
    ASSERT_EQ(searchResult.GetRawData(), JSTaggedValue(4).GetRawData());
}

HWTEST_F_L0(BuiltinsRegExpTest, Split1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("-");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("iug");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString = thread->GetEcmaVM()->GetFactory()->NewFromASCII("");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // invoke Split method
    JSTaggedValue splitResults = BuiltinsRegExp::Split(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> splitResult(thread, splitResults);

    JSHandle<JSTaggedValue> zero(thread->GetEcmaVM()->GetFactory()->NewFromASCII("0"));
    JSHandle<JSTaggedValue> zeroHandle(JSObject::GetProperty(thread, splitResult, zero).GetValue());
    JSHandle<EcmaString> outputZero = JSTaggedValue::ToString(thread, zeroHandle);

    ASSERT_EQ(EcmaStringAccessor::Compare(instance, outputZero, inputString), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, Split2)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("-");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("iug");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString = thread->GetEcmaVM()->GetFactory()->NewFromASCII("a-b-c");

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // invoke Split method
    JSTaggedValue splitResults = BuiltinsRegExp::Split(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> splitResult(thread, splitResults);
    JSHandle<EcmaString> resultZero = thread->GetEcmaVM()->GetFactory()->NewFromASCII("a");
    JSHandle<EcmaString> resultOne = thread->GetEcmaVM()->GetFactory()->NewFromASCII("b");
    JSHandle<EcmaString> resultTwo = thread->GetEcmaVM()->GetFactory()->NewFromASCII("c");

    JSHandle<JSTaggedValue> zero(thread->GetEcmaVM()->GetFactory()->NewFromASCII("0"));
    JSHandle<JSTaggedValue> zeroHandle(JSObject::GetProperty(thread, splitResult, zero).GetValue());
    JSHandle<EcmaString> outputZero = JSTaggedValue::ToString(thread, zeroHandle);
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, outputZero, resultZero), 0);

    JSHandle<JSTaggedValue> first(thread->GetEcmaVM()->GetFactory()->NewFromASCII("1"));
    JSHandle<JSTaggedValue> oneHandle(JSObject::GetProperty(thread, splitResult, first).GetValue());
    JSHandle<EcmaString> outputOne = JSTaggedValue::ToString(thread, oneHandle);
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, outputOne, resultOne), 0);

    JSHandle<JSTaggedValue> second(thread->GetEcmaVM()->GetFactory()->NewFromASCII("2"));
    JSHandle<JSTaggedValue> twoHandle(JSObject::GetProperty(thread, splitResult, second).GetValue());
    JSHandle<EcmaString> outputTwo = JSTaggedValue::ToString(thread, twoHandle);
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, outputTwo, resultTwo), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, GetSpecies)
{
    // invoke RegExpConstructor method
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSTaggedValue> speciesSymbol = env->GetSpeciesSymbol();
    EXPECT_TRUE(!speciesSymbol.GetTaggedValue().IsUndefined());

    JSHandle<JSFunction> newTarget(env->GetRegExpFunction());

    JSTaggedValue value =
        JSObject::GetProperty(thread, JSHandle<JSTaggedValue>(newTarget), speciesSymbol).GetValue().GetTaggedValue();
    EXPECT_EQ(value, newTarget.GetTaggedValue());
}

HWTEST_F_L0(BuiltinsRegExpTest, Replace1)
{
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("quick\\s(brown).+?(jumps)");
    JSHandle<EcmaString> flags1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("iug");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("The Quick Brown Fox Jumps Over The Lazy Dog");
    JSHandle<EcmaString> replaceString =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("$&a $` $\' $2 $01 $$1 $21 $32 a");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, replaceString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // invoke replace method
    JSTaggedValue results = BuiltinsRegExp::Replace(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> replaceResult(thread, results);
    JSHandle<EcmaString> resultZero = thread->GetEcmaVM()->GetFactory()->NewFromASCII(
        "The Quick Brown Fox Jumpsa The   Over The Lazy Dog Jumps Brown $1 Jumps1 $32 a Over The Lazy Dog");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, JSHandle<EcmaString>(replaceResult), resultZero), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, Replace2)
{
    // invoke RegExpConstructor method
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> pattern1 = factory->NewFromASCII("b(c)(z)?(.)");
    JSHandle<EcmaString> flags1 = factory->NewFromASCII("");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString = factory->NewFromASCII("abcde");
    JSHandle<EcmaString> replaceString = factory->NewFromASCII("[$01$02$03$04$00]");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, replaceString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // invoke replace method
    JSTaggedValue results = BuiltinsRegExp::Replace(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> replaceResult(thread, results);
    JSHandle<EcmaString> resultZero = factory->NewFromASCII("a[cd$04$00]e");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, JSHandle<EcmaString>(replaceResult), resultZero), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, Replace3)
{
    // invoke RegExpConstructor method
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> pattern1 = factory->NewFromASCII("abc");
    JSHandle<EcmaString> flags1 = factory->NewFromASCII("g");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSRegExp> value(thread, reinterpret_cast<JSRegExp *>(result1.GetRawData()));

    JSHandle<EcmaString> inputString = factory->NewFromASCII("abcde");
    JSHandle<EcmaString> replaceString = factory->NewFromASCII("");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 8);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(value.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(1, replaceString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // invoke replace method
    JSTaggedValue results = BuiltinsRegExp::Replace(ecmaRuntimeCallInfo);
    JSHandle<JSTaggedValue> replaceResult(thread, results);
    JSHandle<EcmaString> resultZero = factory->NewFromASCII("de");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, JSHandle<EcmaString>(replaceResult), resultZero), 0);
}

HWTEST_F_L0(BuiltinsRegExpTest, RegExpParseCache)
{
    RegExpParserCache *regExpParserCache = thread->GetEcmaVM()->GetRegExpParserCache();
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> string1 = factory->NewFromASCII("abc");
    JSHandle<EcmaString> string2 = factory->NewFromASCII("abcd");
    CVector<CString> vec;
    regExpParserCache->SetCache(thread, *string1, 0, JSTaggedValue::True(), 2, vec);
    ASSERT_TRUE(regExpParserCache->GetCache(thread, *string1, 0, vec).first.IsTrue());
    ASSERT_TRUE(regExpParserCache->GetCache(thread, *string1, 0, vec).second == 2U);
    ASSERT_TRUE(regExpParserCache->GetCache(thread, *string1,
                                            RegExpParserCache::CACHE_SIZE, vec).first.IsHole());
    ASSERT_TRUE(regExpParserCache->GetCache(thread, *string2, 0, vec).first.IsHole());
    ASSERT_TRUE(regExpParserCache->GetCache(thread, *string2, UINT32_MAX, vec).first.IsHole());
}

HWTEST_F_L0(BuiltinsRegExpTest, FlagD)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    // invoke RegExpConstructor method
    JSHandle<EcmaString> pattern1 = factory->NewFromASCII("(?<groupname>a)");
    JSHandle<EcmaString> flags1 = factory->NewFromASCII("gd");
    JSTaggedValue result1 = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern1, flags1);
    JSHandle<JSTaggedValue> result1Handle(thread, result1);

    // invoke GetFlags method
    JSHandle<JSTaggedValue> flags(factory->NewFromASCII("flags"));
    JSHandle<JSTaggedValue> flagsResult(JSObject::GetProperty(thread, result1Handle, flags).GetValue());
    JSHandle<EcmaString> expectResult = factory->NewFromASCII("dg");
    ASSERT_EQ(EcmaStringAccessor::Compare(instance, JSHandle<EcmaString>(flagsResult), expectResult), 0);

    // invoke GetHasIndices method
    JSHandle<JSTaggedValue> hasIndices(factory->NewFromASCII("hasIndices"));
    JSTaggedValue taggedHasIndicesResult =
        JSObject::GetProperty(thread, result1Handle, hasIndices).GetValue().GetTaggedValue();
    ASSERT_EQ(taggedHasIndicesResult.GetRawData(), JSTaggedValue::True().GetRawData());

    JSHandle<EcmaString> inputString = factory->NewFromASCII("babcae");
    auto ecmaRuntimeCallInfo =
        TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6); // 6 means 1 call arg
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(result1Handle.GetTaggedValue());
    ecmaRuntimeCallInfo->SetCallArg(0, inputString.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    // invoke Exec method
    JSTaggedValue results = BuiltinsRegExp::Exec(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSTaggedValue> execResult(thread, results);
    JSHandle<JSTaggedValue> indices(factory->NewFromASCII("indices"));
    JSHandle<JSTaggedValue> indicesArr = JSObject::GetProperty(thread, execResult, indices).GetValue();
    EXPECT_TRUE(indicesArr->IsJSArray());

    JSHandle<JSTaggedValue> indices0 = JSObject::GetProperty(thread, indicesArr, 0).GetValue();
    EXPECT_TRUE(indices0->IsJSArray());
    // indices[0] [1, 2]
    EXPECT_EQ(JSObject::GetProperty(thread, indices0, 0).GetValue()->GetInt(), 1);
    EXPECT_EQ(JSObject::GetProperty(thread, indices0, 1).GetValue()->GetInt(), 2);
    JSHandle<JSTaggedValue> indices1 = JSObject::GetProperty(thread, indicesArr, 1).GetValue();
    EXPECT_TRUE(indices1->IsJSArray());
    // indices[1] [1, 2]
    EXPECT_EQ(JSObject::GetProperty(thread, indices1, 0).GetValue()->GetInt(), 1);
    EXPECT_EQ(JSObject::GetProperty(thread, indices1, 1).GetValue()->GetInt(), 2);

    JSHandle<JSTaggedValue> groups(factory->NewFromASCII("groups"));
    JSHandle<JSTaggedValue> groupsObj = JSObject::GetProperty(thread, indicesArr, groups).GetValue();
    EXPECT_TRUE(groupsObj->IsJSObject());
    JSHandle<JSTaggedValue> groupName(factory->NewFromASCII("groupname"));
    JSHandle<JSTaggedValue> groupNameArr = JSObject::GetProperty(thread, groupsObj, groupName).GetValue();
    EXPECT_TRUE(groupNameArr->IsJSArray());
    // {groupname: [1,2]]}
    EXPECT_EQ(JSObject::GetProperty(thread, groupNameArr, 0).GetValue()->GetInt(), 1);
    EXPECT_EQ(JSObject::GetProperty(thread, groupNameArr, 1).GetValue()->GetInt(), 2);
}
}  // namespace panda::test
