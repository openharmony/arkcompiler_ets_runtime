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

#include "ecmascript/js_regexp_iterator.h"
#include "ecmascript/builtins/builtins_regexp.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_regexp.h"
#include "ecmascript/tests/ecma_test_common.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using BuiltinsRegExp = builtins::BuiltinsRegExp;
class JSRegexpIteratorTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(JSRegexpIteratorTest, CreateRegExpStringIterator)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> pattern = factory->NewFromASCII("\\w+");
    JSHandle<EcmaString> flags = factory->NewFromASCII("gim");

    JSHandle<JSTaggedValue> matchHandle(thread, TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern, flags));
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("g");
    JSHandle<JSTaggedValue> regExpIterator =
        JSRegExpIterator::CreateRegExpStringIterator(thread, matchHandle, inputStr, true, false);
    EXPECT_TRUE(regExpIterator->IsJSRegExpIterator());
    regExpIterator =
        JSRegExpIterator::CreateRegExpStringIterator(thread, matchHandle, inputStr, false, false);
    EXPECT_TRUE(regExpIterator->IsJSRegExpIterator());
}

HWTEST_F_L0(JSRegexpIteratorTest, Next)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<EcmaString> pattern = factory->NewFromASCII("-[0-9]+");
    JSHandle<EcmaString> flags = factory->NewFromASCII("g");
    JSHandle<EcmaString> inputStr = factory->NewFromASCII("2016-01-02|2019-03-04");
    JSHandle<JSTaggedValue> zero(factory->NewFromASCII("0"));
    JSHandle<JSTaggedValue> barZero(factory->NewFromASCII("-0"));

    JSTaggedValue jsRegExp = TestCommon::CreateJSRegexpByPatternAndFlags(thread, pattern, flags);
    JSHandle<JSRegExp> ObjValue(thread, reinterpret_cast<JSRegExp *>(jsRegExp.GetRawData()));
    // create regExp iterator
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(ObjValue.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetCallArg(0, inputStr.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue iteratorValue = BuiltinsRegExp::MatchAll(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);

    JSHandle<JSRegExpIterator> regExpIterator(thread, reinterpret_cast<JSRegExpIterator *>(iteratorValue.GetRawData()));
    uint32_t matchLength = 4; // 4 : 4 Number of matches
    // traversal regExp iterator
    for (uint32_t i = 0; i <= matchLength; i++) {
        auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
        ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
        ecmaRuntimeCallInfo2->SetThis(regExpIterator.GetTaggedValue());

        prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
        JSTaggedValue result = JSRegExpIterator::Next(ecmaRuntimeCallInfo2);
        TestHelper::TearDownFrame(thread, prev);

        JSHandle<JSTaggedValue> matchObj(thread, result);
        if (i <= matchLength - 1) {
            JSHandle<JSTaggedValue> resultValue(thread, JSTaggedValue(i + 1));
            JSHandle<EcmaString> compareVal =
                factory->ConcatFromString(JSHandle<EcmaString>(barZero), JSTaggedValue::ToString(thread, resultValue));
            JSHandle<JSTaggedValue> matchResult = JSIterator::IteratorValue(thread, matchObj);
            JSHandle<JSTaggedValue> zeroHandle(JSObject::GetProperty(thread, matchResult, zero).GetValue());
            JSHandle<EcmaString> outputZero = JSTaggedValue::ToString(thread, zeroHandle);
            EXPECT_EQ(EcmaStringAccessor::Compare(instance, outputZero, compareVal), 0);
            EXPECT_FALSE(regExpIterator->GetDone());
        }
        else {
            EXPECT_TRUE(regExpIterator->GetDone());
            EXPECT_EQ(JSIterator::IteratorValue(thread, matchObj).GetTaggedValue(), JSTaggedValue::Undefined());
        }
    }
}
} // namespace panda::test