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
#include "ecmascript/builtins/builtins_date_time_format.h"

#include <ctime>
#include <algorithm>
#include "ecmascript/builtins/builtins_array.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_date_time_format.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;

namespace panda::test {
using BuiltinsArray = ecmascript::builtins::BuiltinsArray;
class BuiltinsDateTimeFormatTest : public BaseTestWithScope<true> {
};

// new DateTimeFormat(locale)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, DateTimeFormatConstructor)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<GlobalEnv> env = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> newTarget(env->GetDateTimeFormatFunction());

    JSHandle<JSTaggedValue> localesString(factory->NewFromASCII("en-US"));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue(*newTarget), 8);
    ecmaRuntimeCallInfo->SetFunction(newTarget.GetTaggedValue());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, localesString.GetTaggedValue());
    // option tag is default value
    ecmaRuntimeCallInfo->SetCallArg(1, JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDateTimeFormat::DateTimeFormatConstructor(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(result.IsJSDateTimeFormat());
}

static double BuiltinsDateCreate(const double year, const double month, const double date)
{
    const double day = JSDate::MakeDay(year, month, date);
    const double time = JSDate::MakeTime(0, 0, 0, 0); // 24:00:00
    double days = JSDate::MakeDate(day, time);
    return days;
}

enum class AlgorithmType {
    ALGORITHM_FORMAT,
    ALGORITHM_FORMAT_TO_PARTS,
    ALGORITHM_FORMAT_RANGE,
    ALGORITHM_FORMAT_RANGE_TO_PARTS,
};

static JSTaggedValue AtomicsAlgorithm(JSThread *thread, JSHandle<JSDateTimeFormat>& jsDateTimeFormat,
    std::vector<JSTaggedValue> vals, uint32_t argLen, AlgorithmType type = AlgorithmType::ALGORITHM_FORMAT)
{
    auto ecmaRuntimeCallInfos = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), argLen);
    ecmaRuntimeCallInfos->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetThis(jsDateTimeFormat.GetTaggedValue());
    for (size_t i = 0; i < vals.size(); i++) {
        ecmaRuntimeCallInfos->SetCallArg(i, vals[i]);
    }
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfos);
    JSTaggedValue result;
    switch (type) {
        case AlgorithmType::ALGORITHM_FORMAT:
            result = BuiltinsDateTimeFormat::Format(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_FORMAT_TO_PARTS:
            result = BuiltinsDateTimeFormat::FormatToParts(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_FORMAT_RANGE:
            result = BuiltinsDateTimeFormat::FormatRange(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_FORMAT_RANGE_TO_PARTS:
            result = BuiltinsDateTimeFormat::FormatRangeToParts(ecmaRuntimeCallInfos);
            break;
        default:
            break;
    }
    TestHelper::TearDownFrame(thread, prev);
    return result;
}

JSHandle<EcmaString> FormatCommon(JSThread *thread, std::string_view localeStr, double days)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII(localeStr));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
        JSHandle<JSDateTimeFormat>(thread, BuiltTestUtil::JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    std::vector<JSTaggedValue> vals{JSTaggedValue::Undefined()};
    auto result1 =
        AtomicsAlgorithm(thread, jsDateTimeFormat, vals, 6, AlgorithmType::ALGORITHM_FORMAT);  // 6: args length
    JSHandle<JSFunction> jsFunction(thread, result1);
    JSHandle<JSTaggedValue> value(thread, JSTaggedValue(static_cast<double>(days)));
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(jsFunction.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetThis(jsFunction.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetCallArg(0, value.GetTaggedValue());

    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result2 = JSFunction::Call(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);
    JSHandle<EcmaString> resultStr(thread, result2);
    return resultStr;
}

// Format.Tostring(en-US)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, Format_001)
{
    double days = BuiltinsDateCreate(2020, 10, 1);
    auto resultStr = FormatCommon(thread, "en-US", days);
    EXPECT_STREQ("Sun, 11/1/2020, 24:00:00", EcmaStringAccessor(resultStr).ToCString().c_str());
}

// Format.Tostring(pt-BR)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, Format_002)
{
    double days = BuiltinsDateCreate(2020, 5, 11);
    auto resultStr = FormatCommon(thread, "pt-BR", days);
    CString resStr = EcmaStringAccessor(resultStr).ToCString();
    // the index of string "qui" is zero.
    EXPECT_TRUE(resStr.find("qui") == 0);
    // the index of string "11/06/2020 24:00:00" is not zero.
    EXPECT_TRUE(resStr.find("11/06/2020 24:00:00") != 0);
}

HWTEST_F_L0(BuiltinsDateTimeFormatTest, FormatToParts)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
        JSHandle<JSDateTimeFormat>(thread, BuiltTestUtil::JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    std::vector<JSTaggedValue> vals{JSTaggedValue::Undefined()};
    auto result = AtomicsAlgorithm(thread, jsDateTimeFormat, vals, 6,
                                   AlgorithmType::ALGORITHM_FORMAT_TO_PARTS);  // 6: args length
    JSHandle<JSArray> resultHandle(thread, result);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 16U);  // sixteen formatters
}

// FormatRange(zh)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, FormatRange_001)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("zh"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
        JSHandle<JSDateTimeFormat>(thread, BuiltTestUtil::JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    double days1 = BuiltinsDateCreate(2020, 10, 1);
    double days2 = BuiltinsDateCreate(2021, 6, 1);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(days1)),
                                    JSTaggedValue(static_cast<double>(days2))};
    auto result =
        AtomicsAlgorithm(thread, jsDateTimeFormat, vals, 8, AlgorithmType::ALGORITHM_FORMAT_RANGE);  // 8: args length

    JSHandle<EcmaString> handleStr(thread, result);
    JSHandle<EcmaString> resultStr = factory->NewFromUtf8("2020/11/1周日 24:00:00 – 2021/7/1周四 24:00:00");
    EXPECT_EQ(EcmaStringAccessor::Compare(instance, handleStr, resultStr), 0);
}

// FormatRange(en)
HWTEST_F_L0(BuiltinsDateTimeFormatTest, FormatRange_002)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
        JSHandle<JSDateTimeFormat>(thread, BuiltTestUtil::JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    double days1 = BuiltinsDateCreate(2020, 12, 1);
    double days2 = BuiltinsDateCreate(2021, 2, 1);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(days1)),
                                    JSTaggedValue(static_cast<double>(days2))};
    auto result =
        AtomicsAlgorithm(thread, jsDateTimeFormat, vals, 8, AlgorithmType::ALGORITHM_FORMAT_RANGE);  // 8: args length

    JSHandle<EcmaString> handleStr(thread, result);
    JSHandle<EcmaString> resultStr = factory->NewFromUtf8("Fri, 1/1/2021, 24:00:00 – Mon, 3/1/2021, 24:00:00");
    JSHandle<EcmaString> oldStr = factory->NewFromUtf8("Fri, 1/1/2021, 24:00:00 – Mon, 3/1/2021, 24:00:00");
    EXPECT_TRUE(EcmaStringAccessor::Compare(instance, handleStr, resultStr) == 0 ||
                EcmaStringAccessor::Compare(instance, handleStr, oldStr) == 0);
}

HWTEST_F_L0(BuiltinsDateTimeFormatTest, FormatRangeToParts)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    JSHandle<JSTaggedValue> locale(factory->NewFromASCII("en-US"));
    JSHandle<JSDateTimeFormat> jsDateTimeFormat =
        JSHandle<JSDateTimeFormat>(thread, BuiltTestUtil::JSDateTimeFormatCreateWithLocaleTest(thread, locale));

    double days1 = BuiltinsDateCreate(2020, 12, 1);
    double days2 = BuiltinsDateCreate(2021, 2, 1);
    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(days1)),
                                    JSTaggedValue(static_cast<double>(days2))};
    auto result = AtomicsAlgorithm(thread, jsDateTimeFormat, vals, 8,
                                   AlgorithmType::ALGORITHM_FORMAT_RANGE_TO_PARTS);  // 8: args length

    JSHandle<JSArray> resultHandle(thread, result);
    JSHandle<TaggedArray> elements(thread, resultHandle->GetElements());
    EXPECT_EQ(elements->GetLength(), 39U);  // The number of characters of "Fri1/1/202124:00:00–Mon3/1/202124:00:00"
}
}  // namespace panda::test

