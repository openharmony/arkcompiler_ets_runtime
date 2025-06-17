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

#include "ecmascript/builtins/builtins_date.h"

#include "ecmascript/base/string_helper.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/platform/time.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;
using namespace panda::ecmascript::builtins;
namespace panda::test {
const char NEG = '-';
const char PLUS = '+';
const int STR_LENGTH_OTHERS = 2;
const int MINUTE_PER_HOUR = 60;
class BuiltinsDateTest : public BaseTestWithScope<false> {
};

JSHandle<JSDate> JSDateCreateTest(JSThread *thread)
{
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVM->GetFactory();
    JSHandle<GlobalEnv> globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> dateFunction = globalEnv->GetDateFunction();
    JSHandle<JSDate> dateObject =
        JSHandle<JSDate>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dateFunction), dateFunction));
    return dateObject;
}

enum class AlgorithmType {
    ALGORITHM_TO_STRING,
    ALGORITHM_TO_TIME_STRING,
    ALGORITHM_TO_ISO_STRING,
};

static JSTaggedValue DateAlgorithm(JSThread *thread, const JSTaggedValue& thisTag, std::vector<JSTaggedValue>& vals,
    uint32_t argLen = 8, AlgorithmType type = AlgorithmType::ALGORITHM_TO_STRING)
{
    auto ecmaRuntimeCallInfos = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), argLen);
    ecmaRuntimeCallInfos->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfos->SetThis(thisTag);
    for (size_t i = 0; i < vals.size(); i++) {
        ecmaRuntimeCallInfos->SetCallArg(i, JSTaggedValue(vals[i]));
    }
    auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfos);
    JSTaggedValue result;
    switch (type) {
        case AlgorithmType::ALGORITHM_TO_STRING:
            result = BuiltinsDate::ToString(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_TO_TIME_STRING:
            result = BuiltinsDate::ToTimeString(ecmaRuntimeCallInfos);
            break;
        case AlgorithmType::ALGORITHM_TO_ISO_STRING:
            result = BuiltinsDate::ToISOString(ecmaRuntimeCallInfos);
            break;
        default:
            break;
    }
    TestHelper::TearDownFrame(thread, prev);
    return result;
}


HWTEST_F_L0(BuiltinsDateTest, SetGetDate)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(2))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 6, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    [[maybe_unused]] JSTaggedValue result1 = BuiltinsDate::SetDate(ecmaRuntimeCallInfo);
    JSTaggedValue result2 = BuiltinsDate::GetDate(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetUTCDate)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(2))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 6, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    [[maybe_unused]] JSTaggedValue result3 = BuiltinsDate::SetUTCDate(ecmaRuntimeCallInfo);
    JSTaggedValue result4 = BuiltinsDate::GetUTCDate(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMinusUTCDate)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(-2))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 6, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    [[maybe_unused]] JSTaggedValue result3 = BuiltinsDate::SetUTCDate(ecmaRuntimeCallInfo);
    JSTaggedValue result4 = BuiltinsDate::GetUTCDate(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(29)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetFullYear)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(2018)), JSTaggedValue(static_cast<double>(10)),
                                    JSTaggedValue(static_cast<double>(6))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 10, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::GetFullYear(ecmaRuntimeCallInfo);
    // 2018 : test case
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(2018)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetMonth(ecmaRuntimeCallInfo);
    // 10 : test case
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());

    JSTaggedValue result3 = BuiltinsDate::GetDate(ecmaRuntimeCallInfo);
    // 6 : test case
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(6)).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsDateTest, SetGetUTCFullYear)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(2018)), JSTaggedValue(static_cast<double>(10)),
                                    JSTaggedValue(static_cast<double>(6))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 10, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetUTCFullYear(ecmaRuntimeCallInfo);
    JSTaggedValue result4 = BuiltinsDate::GetUTCFullYear(ecmaRuntimeCallInfo);
    // 2018 : test case
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(2018)).GetRawData());

    JSTaggedValue result5 = BuiltinsDate::GetUTCMonth(ecmaRuntimeCallInfo);
    // 10 : test case
    ASSERT_EQ(result5.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());

    JSTaggedValue result6 = BuiltinsDate::GetUTCDate(ecmaRuntimeCallInfo);
    // 6 : test case
    ASSERT_EQ(result6.GetRawData(), JSTaggedValue(static_cast<double>(6)).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMinusFullYear)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(-2018)), JSTaggedValue(static_cast<double>(-10)),
                                    JSTaggedValue(static_cast<double>(-6))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 10, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::GetFullYear(ecmaRuntimeCallInfo);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(-2019)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetMonth(ecmaRuntimeCallInfo);
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(1)).GetRawData());

    JSTaggedValue result3 = BuiltinsDate::GetDate(ecmaRuntimeCallInfo);
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(22)).GetRawData());
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMinusUTCFullYear)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(-2018)), JSTaggedValue(static_cast<double>(-10)),
                                    JSTaggedValue(static_cast<double>(-6))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 10, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetUTCFullYear(ecmaRuntimeCallInfo);
    JSTaggedValue result4 = BuiltinsDate::GetUTCFullYear(ecmaRuntimeCallInfo);
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(-2019)).GetRawData());

    JSTaggedValue result5 = BuiltinsDate::GetUTCMonth(ecmaRuntimeCallInfo);
    ASSERT_EQ(result5.GetRawData(), JSTaggedValue(static_cast<double>(1)).GetRawData());

    JSTaggedValue result6 = BuiltinsDate::GetUTCDate(ecmaRuntimeCallInfo);
    ASSERT_EQ(result6.GetRawData(), JSTaggedValue(static_cast<double>(22)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetHours)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(18)), JSTaggedValue(static_cast<double>(10)),
                                    JSTaggedValue(static_cast<double>(6)), JSTaggedValue(static_cast<double>(111))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 12, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetHours(ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::GetHours(ecmaRuntimeCallInfo);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(18)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetMinutes(ecmaRuntimeCallInfo);
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());

    JSTaggedValue result3 = BuiltinsDate::GetSeconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(6)).GetRawData());

    JSTaggedValue result4 = BuiltinsDate::GetMilliseconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(111)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetUTCHours)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(18)), JSTaggedValue(static_cast<double>(10)),
                                    JSTaggedValue(static_cast<double>(6)), JSTaggedValue(static_cast<double>(111))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 12, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetUTCHours(ecmaRuntimeCallInfo);
    JSTaggedValue result5 = BuiltinsDate::GetUTCHours(ecmaRuntimeCallInfo);
    ASSERT_EQ(result5.GetRawData(), JSTaggedValue(static_cast<double>(18)).GetRawData());

    JSTaggedValue result6 = BuiltinsDate::GetUTCMinutes(ecmaRuntimeCallInfo);
    ASSERT_EQ(result6.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());

    JSTaggedValue result7 = BuiltinsDate::GetUTCSeconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result7.GetRawData(), JSTaggedValue(static_cast<double>(6)).GetRawData());

    JSTaggedValue result8 = BuiltinsDate::GetUTCMilliseconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result8.GetRawData(), JSTaggedValue(static_cast<double>(111)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMinusHours)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(-18)), JSTaggedValue(static_cast<double>(-10)),
                                    JSTaggedValue(static_cast<double>(-6)), JSTaggedValue(static_cast<double>(-111))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 12, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetHours(ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::GetHours(ecmaRuntimeCallInfo);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(5)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetMinutes(ecmaRuntimeCallInfo);
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(49)).GetRawData());

    JSTaggedValue result3 = BuiltinsDate::GetSeconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(53)).GetRawData());

    JSTaggedValue result4 = BuiltinsDate::GetMilliseconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(889)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMinusUTCHours)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(-18)), JSTaggedValue(static_cast<double>(-10)),
                                    JSTaggedValue(static_cast<double>(-6)), JSTaggedValue(static_cast<double>(-111))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 12, jsDate.GetTaggedValue());
    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetUTCHours(ecmaRuntimeCallInfo);
    JSTaggedValue result5 = BuiltinsDate::GetUTCHours(ecmaRuntimeCallInfo);
    ASSERT_EQ(result5.GetRawData(), JSTaggedValue(static_cast<double>(5)).GetRawData());

    JSTaggedValue result6 = BuiltinsDate::GetUTCMinutes(ecmaRuntimeCallInfo);
    ASSERT_EQ(result6.GetRawData(), JSTaggedValue(static_cast<double>(49)).GetRawData());

    JSTaggedValue result7 = BuiltinsDate::GetUTCSeconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result7.GetRawData(), JSTaggedValue(static_cast<double>(53)).GetRawData());

    JSTaggedValue result8 = BuiltinsDate::GetUTCMilliseconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result8.GetRawData(), JSTaggedValue(static_cast<double>(889)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMilliseconds)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(100))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 6, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::SetMilliseconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(100)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetMilliseconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(100)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetUTCMilliseconds)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(100))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 6, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result3 = BuiltinsDate::SetUTCMilliseconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(100)).GetRawData());

    JSTaggedValue result4 = BuiltinsDate::GetUTCMilliseconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(100)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMinutes)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(10)), JSTaggedValue(static_cast<double>(6)),
                                                  JSTaggedValue(static_cast<double>(111))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 10, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetMinutes(ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::GetMinutes(ecmaRuntimeCallInfo);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetSeconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(6)).GetRawData());

    JSTaggedValue result3 = BuiltinsDate::GetMilliseconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(111)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetUTCMinutes)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(10)), JSTaggedValue(static_cast<double>(6)),
                                                  JSTaggedValue(static_cast<double>(111))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 10, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetUTCMinutes(ecmaRuntimeCallInfo);
    JSTaggedValue result4 = BuiltinsDate::GetUTCMinutes(ecmaRuntimeCallInfo);
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());

    JSTaggedValue result5 = BuiltinsDate::GetUTCSeconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result5.GetRawData(), JSTaggedValue(static_cast<double>(6)).GetRawData());

    JSTaggedValue result6 = BuiltinsDate::GetUTCMilliseconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result6.GetRawData(), JSTaggedValue(static_cast<double>(111)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetMonth)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(8)), JSTaggedValue(static_cast<double>(3))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 8, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetMonth(ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::GetMonth(ecmaRuntimeCallInfo);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(8)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetDate(ecmaRuntimeCallInfo);
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(3)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetUTCMonth)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(8)), JSTaggedValue(static_cast<double>(3))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 8, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetUTCMonth(ecmaRuntimeCallInfo);
    JSTaggedValue result3 = BuiltinsDate::GetUTCMonth(ecmaRuntimeCallInfo);
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(8)).GetRawData());

    JSTaggedValue result4 = BuiltinsDate::GetUTCDate(ecmaRuntimeCallInfo);
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(3)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetSeconds)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(59)), JSTaggedValue(static_cast<double>(123))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 8, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetSeconds(ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::GetSeconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(59)).GetRawData());

    JSTaggedValue result2 = BuiltinsDate::GetMilliseconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result2.GetRawData(), JSTaggedValue(static_cast<double>(123)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetUTCSeconds)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(59)), JSTaggedValue(static_cast<double>(123))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 8, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetUTCSeconds(ecmaRuntimeCallInfo);
    JSTaggedValue result3 = BuiltinsDate::GetUTCSeconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result3.GetRawData(), JSTaggedValue(static_cast<double>(59)).GetRawData());

    JSTaggedValue result4 = BuiltinsDate::GetUTCMilliseconds(ecmaRuntimeCallInfo);
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(123)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, SetGetTime)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);

    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(2))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 6, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDate::SetTime(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());

    result = BuiltinsDate::GetTime(ecmaRuntimeCallInfo);
    ASSERT_EQ(result.GetRawData(), JSTaggedValue(static_cast<double>(2)).GetRawData());

    // msg != IsDate
    auto info1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 6);
    prev = TestHelper::SetupFrame(thread, info1);
    result = BuiltinsDate::SetTime(info1);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(thread->HasPendingException());
    EXPECT_EQ(result, JSTaggedValue::Exception());
    thread->ClearException();

    auto info2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 6);
    result = BuiltinsDate::GetTime(info2);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(thread->HasPendingException());
    EXPECT_EQ(result, JSTaggedValue::Exception());
    thread->ClearException();
}

HWTEST_F_L0(BuiltinsDateTest, UTC)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    std::vector<JSTaggedValue> vals{JSTaggedValue(2020.982), JSTaggedValue(10.23), JSTaggedValue(4.32),
                                    JSTaggedValue(11.32)};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 12, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::UTC(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1604487600000)).GetRawData());

    vals.push_back(JSTaggedValue(45.1));
    vals.push_back(JSTaggedValue(34.321));
    vals.push_back(JSTaggedValue(static_cast<int32_t>(231)));
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 18, jsDate.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    result1 = BuiltinsDate::UTC(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1604490334231)).GetRawData());

    std::vector<JSTaggedValue> vals2{JSTaggedValue(10.23), JSTaggedValue(4.32), JSTaggedValue(11.32)};
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals2, 10, jsDate.GetTaggedValue());
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    result1 = BuiltinsDate::UTC(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(-1882224000000)).GetRawData());

    std::vector<JSTaggedValue> vals3{JSTaggedValue(1994.982)};
    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals3, 6, jsDate.GetTaggedValue());
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3);
    result1 = BuiltinsDate::UTC(ecmaRuntimeCallInfo3);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(757382400000)).GetRawData());

    std::vector<JSTaggedValue> vals4{JSTaggedValue(19999944.982)};
    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals4, 6, jsDate.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4);
    result1 = BuiltinsDate::UTC(ecmaRuntimeCallInfo4);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(base::NAN_VALUE)).GetRawData());
}

void SetAllYearAndHours(JSThread *thread, const JSHandle<JSDate> &jsDate)
{
    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(2018)), JSTaggedValue(static_cast<double>(10)),
                                     JSTaggedValue(static_cast<double>(6))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 10, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    std::vector<JSTaggedValue> vals1{JSTaggedValue(static_cast<double>(18)), JSTaggedValue(static_cast<double>(10)),
                                    JSTaggedValue(static_cast<double>(6)), JSTaggedValue(static_cast<double>(111))};
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals1, 12, jsDate.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    BuiltinsDate::SetHours(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
}

void SetAll1(JSThread *thread, const JSHandle<JSDate> &jsDate)
{
    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(1900)), JSTaggedValue(static_cast<double>(11)),
                                    JSTaggedValue(static_cast<double>(31))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 10, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    std::vector<JSTaggedValue> vals1{JSTaggedValue(static_cast<double>(23)), JSTaggedValue(static_cast<double>(54)),
                                     JSTaggedValue(static_cast<double>(16)), JSTaggedValue(static_cast<double>(888))};
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals1, 12, jsDate.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    BuiltinsDate::SetHours(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
}

void SetAll2(JSThread *thread, const JSHandle<JSDate> &jsDate)
{
    std::vector<JSTaggedValue> vals{JSTaggedValue(static_cast<double>(1901)), JSTaggedValue(static_cast<double>(0)),
                                     JSTaggedValue(static_cast<double>(1))};
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals, 10, jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    std::vector<JSTaggedValue> vals1{JSTaggedValue(static_cast<double>(0)), JSTaggedValue(static_cast<double>(3)),
                                     JSTaggedValue(static_cast<double>(21)), JSTaggedValue(static_cast<double>(129))};
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, vals1, 12, jsDate.GetTaggedValue());
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    BuiltinsDate::SetHours(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
}

HWTEST_F_L0(BuiltinsDateTest, parse)
{
    JSHandle<EcmaString> str =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("2020-11-19T12:18:18.132Z");
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetCallArg(0, str.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::Parse(ecmaRuntimeCallInfo);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1605788298132)).GetRawData());

    JSHandle<EcmaString> str1 = thread->GetEcmaVM()->GetFactory()->NewFromASCII("2020-11-19Z");
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo1->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo1->SetCallArg(0, str1.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    result1 = BuiltinsDate::Parse(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1605744000000)).GetRawData());

    JSHandle<EcmaString> str2 =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("2020-11T12:18:17.231+08:00");
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo2->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo2->SetCallArg(0, str2.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    result1 = BuiltinsDate::Parse(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1604204297231)).GetRawData());

    JSHandle<EcmaString> str3 =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("Thu Nov 19 2020 20:18:18 GMT+0800");
    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo3->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo3->SetCallArg(0, str3.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3);
    result1 = BuiltinsDate::Parse(ecmaRuntimeCallInfo3);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1605788298000)).GetRawData());

    JSHandle<EcmaString> str4 =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("Thu 03 Jun 2093 04:18 GMT");
    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo4->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo4->SetCallArg(0, str4.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4);
    result1 = BuiltinsDate::Parse(ecmaRuntimeCallInfo4);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(3894841080000)).GetRawData());

    auto ecmaRuntimeCallInfo5 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 6);
    ecmaRuntimeCallInfo5->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo5->SetThis(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo5->SetCallArg(0, JSTaggedValue::Null());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo5);
    result1 = BuiltinsDate::Parse(ecmaRuntimeCallInfo5);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(base::NAN_VALUE)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, ToDateString)
{
    JSHandle<EcmaString> expect_value =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("Tue Nov 06 2018");
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    SetAllYearAndHours(thread, jsDate);

    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result = BuiltinsDate::ToDateString(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);

    ASSERT_TRUE(result.IsString());
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread,
        reinterpret_cast<EcmaString *>(result.GetRawData()), *expect_value));
}

HWTEST_F_L0(BuiltinsDateTest, ToISOString)
{
    JSHandle<EcmaString> expect_value =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("2020-11-19T12:18:18.132Z");
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->SetTimeValue(thread, JSTaggedValue(1605788298132.0));
    std::vector<JSTaggedValue> args{};
    auto result1 = DateAlgorithm(thread, jsDate.GetTaggedValue(), args, 4, AlgorithmType::ALGORITHM_TO_ISO_STRING);
    ASSERT_TRUE(result1.IsString());
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread,
        reinterpret_cast<EcmaString *>(result1.GetRawData()), *expect_value));
}

HWTEST_F_L0(BuiltinsDateTest, ToISOStringMinus)
{
    JSHandle<EcmaString> expect_value =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("1831-12-02T21:47:18.382Z");
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->SetTimeValue(thread, JSTaggedValue(-4357419161618.0));

    std::vector<JSTaggedValue> args{};
    auto result1 = DateAlgorithm(thread, jsDate.GetTaggedValue(), args, 4, AlgorithmType::ALGORITHM_TO_ISO_STRING);
    ASSERT_TRUE(result1.IsString());
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread,
        reinterpret_cast<EcmaString *>(result1.GetRawData()), *expect_value));
}

// test toJSON and toPrimitive
HWTEST_F_L0(BuiltinsDateTest, ToJSON)
{
    JSHandle<EcmaString> expect_value =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("2020-11-19T12:18:18.132Z");
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    // number double finite
    jsDate->SetTimeValue(thread, JSTaggedValue(1605788298132.0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::ToJSON(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result1.IsString());
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread,
        reinterpret_cast<EcmaString *>(result1.GetRawData()), *expect_value));

    // number double infinite
    jsDate->SetTimeValue(thread, JSTaggedValue(base::POSITIVE_INFINITY));
    ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    result1 = BuiltinsDate::ToJSON(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1, JSTaggedValue::Null());
}

HWTEST_F_L0(BuiltinsDateTest, ToJSONMinus)
{
    JSHandle<EcmaString> expect_value =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("1831-12-02T21:47:18.382Z");
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    jsDate->SetTimeValue(thread, JSTaggedValue(-4357419161618.0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::ToJSON(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result1.IsString());
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread,
        reinterpret_cast<EcmaString *>(result1.GetRawData()), *expect_value));
}

CString GetLocalTime(JSThread *thread, JSHandle<JSDate>& jsDate)
{
    CString localTime;
    int localMin = 0;
    localMin = GetLocalOffsetFromOS(static_cast<int64_t>((*jsDate)->GetTimeValue(thread).GetDouble()), true);
    if (localMin >= 0) {
        localTime += PLUS;
    } else if (localMin < 0) {
        localTime += NEG;
        localMin = -localMin;
    }
    localTime = localTime + JSDate::StrToTargetLength(ToCString(localMin / MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    localTime = localTime + JSDate::StrToTargetLength(ToCString(localMin % MINUTE_PER_HOUR), STR_LENGTH_OTHERS);
    return localTime;
}

void ToStringCommon(JSThread* thread, CString& str1, CString& str2, CString& str3, AlgorithmType type)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    std::vector<JSTaggedValue> argTags{};
    SetAllYearAndHours(thread, jsDate);
    JSTaggedValue result1 = DateAlgorithm(thread, jsDate.GetTaggedValue(), argTags, 4, type);
    
    ASSERT_TRUE(result1.IsString());
    JSHandle<EcmaString> result1_val(thread, reinterpret_cast<EcmaString *>(result1.GetRawData()));
    CString str = str1 + GetLocalTime(thread, jsDate);
    JSHandle<EcmaString> str_handle = thread->GetEcmaVM()->GetFactory()->NewFromASCII(str);
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread, *result1_val, *str_handle));

    JSHandle<JSDate> js_date1 = JSDateCreateTest(thread);
    SetAll1(thread, js_date1);

    JSTaggedValue result2 = DateAlgorithm(thread, js_date1.GetTaggedValue(), argTags, 4, type);
    ASSERT_TRUE(result2.IsString());
    JSHandle<EcmaString> result2_val(thread, reinterpret_cast<EcmaString *>(result2.GetRawData()));
    str = str2 + GetLocalTime(thread, js_date1);
    str_handle = thread->GetEcmaVM()->GetFactory()->NewFromASCII(str);
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread, *result2_val, *str_handle));

    JSHandle<JSDate> js_date2 = JSDateCreateTest(thread);
    SetAll2(thread, js_date2);
    JSTaggedValue result3 = DateAlgorithm(thread, js_date2.GetTaggedValue(), argTags, 4, type);
    ASSERT_TRUE(result3.IsString());
    JSHandle<EcmaString> result3_val(thread, reinterpret_cast<EcmaString *>(result3.GetRawData()));
    str = str3 + GetLocalTime(thread, js_date2);
    str_handle = thread->GetEcmaVM()->GetFactory()->NewFromASCII(str);
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread, *result3_val, *str_handle));
}

HWTEST_F_L0(BuiltinsDateTest, ToString)
{
    CString str1 = "Tue Nov 06 2018 18:10:06 GMT";
    CString str2 = "Mon Dec 31 1900 23:54:16 GMT";
    CString str3 = "Tue Jan 01 1901 00:03:21 GMT";
    ToStringCommon(thread, str1, str2, str3, AlgorithmType::ALGORITHM_TO_STRING);
}

HWTEST_F_L0(BuiltinsDateTest, ToTimeString)
{
    CString str1 = "18:10:06 GMT";
    CString str2 = "23:54:16 GMT";
    CString str3 = "00:03:21 GMT";
    ToStringCommon(thread, str1, str2, str3, AlgorithmType::ALGORITHM_TO_TIME_STRING);
}

HWTEST_F_L0(BuiltinsDateTest, ToUTCString)
{
    JSHandle<EcmaString> expect_value =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("Thu, 19 Nov 2020 12:18:18 GMT");
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->SetTimeValue(thread, JSTaggedValue(1605788298132.0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::ToUTCString(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result1.IsString());
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread,
        reinterpret_cast<EcmaString *>(result1.GetRawData()), *expect_value));
}

HWTEST_F_L0(BuiltinsDateTest, ToUTCStringMinus)
{
    JSHandle<EcmaString> expect_value =
        thread->GetEcmaVM()->GetFactory()->NewFromASCII("Fri, 02 Dec 1831 21:47:18 GMT");
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->SetTimeValue(thread, JSTaggedValue(-4357419161618.0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::ToUTCString(ecmaRuntimeCallInfo);
    ASSERT_TRUE(result1.IsString());
    ASSERT_TRUE(EcmaStringAccessor::StringsAreEqual(thread,
        reinterpret_cast<EcmaString *>(result1.GetRawData()), *expect_value));
}

HWTEST_F_L0(BuiltinsDateTest, ValueOf)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->SetTimeValue(thread, JSTaggedValue(1605788298132.0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::ValueOf(ecmaRuntimeCallInfo);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(1605788298132)).GetRawData());

    // case :msg is not Date
    auto info1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    info1->SetFunction(JSTaggedValue::Undefined());
    info1->SetThis(JSTaggedValue::Undefined());

    prev = TestHelper::SetupFrame(thread, info1);
    result1 = BuiltinsDate::ValueOf(info1);
    TestHelper::TearDownFrame(thread, prev);
    EXPECT_TRUE(thread->HasPendingException());
    EXPECT_EQ(result1, JSTaggedValue::Exception());
    thread->ClearException();
}

HWTEST_F_L0(BuiltinsDateTest, ValueOfMinus)
{
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    JSDate::Cast(jsDate.GetTaggedValue().GetTaggedObject())->SetTimeValue(thread, JSTaggedValue(-4357419161618.0));
    auto ecmaRuntimeCallInfo = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo->SetFunction(JSTaggedValue::Undefined());
    ecmaRuntimeCallInfo->SetThis(jsDate.GetTaggedValue());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo);
    JSTaggedValue result1 = BuiltinsDate::ValueOf(ecmaRuntimeCallInfo);
    ASSERT_EQ(result1.GetRawData(), JSTaggedValue(static_cast<double>(-4357419161618)).GetRawData());
}

HWTEST_F_L0(BuiltinsDateTest, DateConstructor)
{
    // case1: test new target is undefined.
    JSHandle<JSDate> jsDate = JSDateCreateTest(thread);
    JSHandle<GlobalEnv> globalEnv = thread->GetEcmaVM()->GetGlobalEnv();
    JSHandle<JSFunction> date_func(globalEnv->GetDateFunction());
    auto ecmaRuntimeCallInfo1 = TestHelper::CreateEcmaRuntimeCallInfo(thread, JSTaggedValue::Undefined(), 4);
    ecmaRuntimeCallInfo1->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo1->SetThis(JSTaggedValue::Undefined());

    [[maybe_unused]] auto prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo1);
    JSTaggedValue result1 = BuiltinsDate::DateConstructor(ecmaRuntimeCallInfo1);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result1.IsString());

    // case2: length == 0
    auto ecmaRuntimeCallInfo2 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 4);
    ecmaRuntimeCallInfo2->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo2->SetThis(jsDate.GetTaggedValue());

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo2);
    JSTaggedValue result2 = BuiltinsDate::DateConstructor(ecmaRuntimeCallInfo2);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result2.IsObject());

    // case3: length == 1
    auto ecmaRuntimeCallInfo3 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 6);
    ecmaRuntimeCallInfo3->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo3->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo3);
    JSTaggedValue result3 = BuiltinsDate::DateConstructor(ecmaRuntimeCallInfo3);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result3.IsObject());

    auto ecmaRuntimeCallInfo4 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 6);
    ecmaRuntimeCallInfo4->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo4->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo4);
    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo4);
    TestHelper::TearDownFrame(thread, prev);

    auto ecmaRuntimeCallInfo5 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 6);
    ecmaRuntimeCallInfo5->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo5->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo5->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo5);
    JSTaggedValue result4 = BuiltinsDate::GetFullYear(ecmaRuntimeCallInfo5);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result4.GetRawData(), JSTaggedValue(static_cast<double>(2018)).GetRawData());

    // case4: length > 1
    auto ecmaRuntimeCallInfo6 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo6->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo6->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo6->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));
    ecmaRuntimeCallInfo6->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo6);
    JSTaggedValue result5 = BuiltinsDate::DateConstructor(ecmaRuntimeCallInfo6);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result5.IsObject());

    SetAllYearAndHours(thread, jsDate);

    auto ecmaRuntimeCallInfo7 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo7->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo7->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo7->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));
    ecmaRuntimeCallInfo7->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo7);
    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo7);
    TestHelper::TearDownFrame(thread, prev);

    auto ecmaRuntimeCallInfo8 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo8->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo8->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo8->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));
    ecmaRuntimeCallInfo8->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo8);
    JSTaggedValue result6 = BuiltinsDate::GetFullYear(ecmaRuntimeCallInfo8);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result6.GetRawData(), JSTaggedValue(static_cast<double>(2018)).GetRawData());

    auto ecmaRuntimeCallInfo9 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo9->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo9->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo9->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));
    ecmaRuntimeCallInfo9->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo9);
    JSTaggedValue result7 = BuiltinsDate::GetMonth(ecmaRuntimeCallInfo9);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result7.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());

    // case5: length > 7
    auto ecmaRuntimeCallInfo10 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 20);
    ecmaRuntimeCallInfo10->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo10->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo10->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));
    ecmaRuntimeCallInfo10->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));
    ecmaRuntimeCallInfo10->SetCallArg(2, JSTaggedValue(static_cast<double>(10)));
    ecmaRuntimeCallInfo10->SetCallArg(3, JSTaggedValue(static_cast<double>(10)));
    ecmaRuntimeCallInfo10->SetCallArg(4, JSTaggedValue(static_cast<double>(10)));
    ecmaRuntimeCallInfo10->SetCallArg(5, JSTaggedValue(static_cast<double>(10)));
    ecmaRuntimeCallInfo10->SetCallArg(6, JSTaggedValue(static_cast<double>(10)));
    ecmaRuntimeCallInfo10->SetCallArg(7, JSTaggedValue(static_cast<double>(10)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo10);
    JSTaggedValue result8 = BuiltinsDate::DateConstructor(ecmaRuntimeCallInfo10);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result8.IsObject());

    // case6: length > 1, NAN number
    auto ecmaRuntimeCallInfo11 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo11->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo11->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo11->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));
    ecmaRuntimeCallInfo11->SetCallArg(1, JSTaggedValue(static_cast<double>(base::NAN_VALUE)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo11);
    JSTaggedValue result9 = BuiltinsDate::DateConstructor(ecmaRuntimeCallInfo11);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result9.IsObject());
    JSHandle<JSTaggedValue> result10(thread, result9);
    JSHandle<JSDate> dateResult1 = JSHandle<JSDate>::Cast(result10);
    ASSERT_EQ(dateResult1->GetTimeValue(thread), JSTaggedValue(static_cast<double>(base::NAN_VALUE)));

    // case7: length > 1, infinite number
    auto ecmaRuntimeCallInfo12 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo12->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo12->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo12->SetCallArg(0, JSTaggedValue(static_cast<double>(2018)));
    ecmaRuntimeCallInfo12->SetCallArg(1, JSTaggedValue(static_cast<double>(base::POSITIVE_INFINITY)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo12);
    JSTaggedValue result11 = BuiltinsDate::DateConstructor(ecmaRuntimeCallInfo12);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result11.IsObject());
    JSHandle<JSTaggedValue> result12(thread, result11);
    JSHandle<JSDate> dateResult2 = JSHandle<JSDate>::Cast(result12);
    ASSERT_EQ(dateResult2->GetTimeValue(thread), JSTaggedValue(static_cast<double>(base::NAN_VALUE)));

    // case8: length > 1
    auto ecmaRuntimeCallInfo13 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo13->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo13->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo13->SetCallArg(0, JSTaggedValue(static_cast<double>(10)));
    ecmaRuntimeCallInfo13->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));

    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo13);
    JSTaggedValue result13 = BuiltinsDate::DateConstructor(ecmaRuntimeCallInfo13);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_TRUE(result13.IsObject());

    SetAllYearAndHours(thread, jsDate);

    auto ecmaRuntimeCallInfo14 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo14->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo14->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo14->SetCallArg(0, JSTaggedValue(static_cast<double>(10)));
    ecmaRuntimeCallInfo14->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo14);
    BuiltinsDate::SetFullYear(ecmaRuntimeCallInfo14);
    TestHelper::TearDownFrame(thread, prev);

    auto ecmaRuntimeCallInfo15 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo15->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo15->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo15->SetCallArg(0, JSTaggedValue(static_cast<double>(10)));
    ecmaRuntimeCallInfo15->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo15);
    JSTaggedValue result14 = BuiltinsDate::GetFullYear(ecmaRuntimeCallInfo15);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result14.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());

    auto ecmaRuntimeCallInfo16 = TestHelper::CreateEcmaRuntimeCallInfo(thread, jsDate.GetTaggedValue(), 8);
    ecmaRuntimeCallInfo16->SetFunction(date_func.GetTaggedValue());
    ecmaRuntimeCallInfo16->SetThis(jsDate.GetTaggedValue());
    ecmaRuntimeCallInfo16->SetCallArg(0, JSTaggedValue(static_cast<double>(10)));
    ecmaRuntimeCallInfo16->SetCallArg(1, JSTaggedValue(static_cast<double>(10)));
    prev = TestHelper::SetupFrame(thread, ecmaRuntimeCallInfo16);
    JSTaggedValue result15 = BuiltinsDate::GetMonth(ecmaRuntimeCallInfo16);
    TestHelper::TearDownFrame(thread, prev);
    ASSERT_EQ(result15.GetRawData(), JSTaggedValue(static_cast<double>(10)).GetRawData());
}
}  // namespace panda::test
