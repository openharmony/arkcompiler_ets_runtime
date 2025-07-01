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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/global_env.h"
#include "ecmascript/js_date.h"
#include "ecmascript/js_function.h"
#include "ecmascript/js_handle.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda::ecmascript;

namespace panda::test {
class JSDateTest : public BaseTestWithScope<false> {
};

JSDate *JSDateCreate(JSThread *thread)
{
    ObjectFactory *factory = thread->GetEcmaVM()->GetFactory();
    EcmaVM *ecmaVM = thread->GetEcmaVM();
    auto globalEnv = ecmaVM->GetGlobalEnv();
    JSHandle<JSTaggedValue> dateFunction = globalEnv->GetDateFunction();
    JSHandle<JSDate> dateObject =
        JSHandle<JSDate>::Cast(factory->NewJSObjectByConstructor(JSHandle<JSFunction>(dateFunction), dateFunction));
    return *dateObject;
}

HWTEST_F_L0(JSDateTest, Create)
{
    double tm = 0.0;
    JSHandle<JSDate> jsDate(thread, JSDateCreate(thread));
    EXPECT_EQ(jsDate->GetTimeValue(thread), JSTaggedValue(tm));
    EXPECT_EQ(jsDate->GetLocalOffset(thread), JSTaggedValue(JSDate::MAX_DOUBLE));
    tm = 28 * 60 * 60 * 1000;
    jsDate->SetTimeValue(thread, JSTaggedValue(tm));

    [[maybe_unused]] double temp = jsDate->GetTimeValue(thread).GetDouble();
    EXPECT_EQ(jsDate->GetTimeValue(thread), JSTaggedValue(tm));
}

HWTEST_F_L0(JSDateTest, MakeTime)
{
    double const day1 = ecmascript::JSDate::MakeDay(0, 11, 31);
    double const time1 = ecmascript::JSDate::MakeTime(0, 0, 0, 0);
    double ms1 = ecmascript::JSDate::TimeClip(ecmascript::JSDate::MakeDate(day1, time1));
    EXPECT_EQ(ms1, -62135683200000.0);

    double const day = ecmascript::JSDate::MakeDay(-1, 11, 31);
    double const time = ecmascript::JSDate::MakeTime(0, 0, 0, 0);
    double ms = ecmascript::JSDate::TimeClip(ecmascript::JSDate::MakeDate(day, time));
    EXPECT_EQ(ms, -62167305600000.0);
}

HWTEST_F_L0(JSDateTest, GetTimeFromString)
{
    // test ISO 8601
    CString str = "2020-11-19T12:18:18.132Z";
    JSTaggedValue ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1605788298132.0);

    str = "1880-12-30T23:59:59";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), -2808633901000.0);

    str = "2020-11-19Z";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1605744000000.0);

    str = "2020-11";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1604188800000.0);

    str = "2023-1-11";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1673366400000.0);

    str = "+275760-09-13T00:00:00.000Z";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 8640000000000000.0);

    str = "-271821-04-20T00:00:00.000Z";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), -8640000000000000.0);

    str = "2020T12:18Z";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1577881080000.0);

    str = "2020T12:18:17.231Z";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1577881097231.0);

    str = "2020-11T12:18:17.231Z";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1604233097231.0);

    str = "1645-11T12:18:17.231+08:00";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), -10229658102769.0);

    str = "2020-11-19T12:18-08:12";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1605817800000.0);

    // test Local time
    str = "Thu Nov 19 2020 20:18:18 GMT+0800";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1605788298000.0);

    str = "Thu Nov 19 2020 20:18 GMT-0800";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1605845880000.0);

    str = "Thu Nov 03 2093 04:18 GMT";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 3908060280000.0);

    str = "Thu Nov 19 1820 GMT+1232";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), -4705734720000.0);

    // test UTC time
    str = "Thu, 19 Nov 2020 20:18:18 GMT+0800";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1605788298000.0);

    str = "Thu, 19 Nov 2020 20:18 GMT-0800";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1605845880000.0);

    str = "Thu 03 Jun 2093 04:18 GMT";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 3894841080000.0);

    str = "Thu 19 Nov 1820 GMT+1232";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), -4705734720000.0);

    // test YYYY-MM-DD HH:MM:ss
    str = "2020-11-19 12:18:18.132Z";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1605788298132.0);

    str = "2020-11-19 12:18:18.132+02:21";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1605779838132.0);

    str = "2020-11-19 12:18:18.132-02:21";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1605796758132.0);

    str = "+175760-09-13 03:16:02.003Z";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 5484304811762003.0);

    str = "-171821-04-20 13:08:23.037Z";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), -5484304752696963.0);

    // test Month DD,YYYY HH:MM:SS
    str = "January 12,2006 22:19:35";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1137075575000.0);

    str = "January 12,2006";
    ms = ecmascript::JSDate::GetTimeFromString(str.c_str(), str.length());
    EXPECT_EQ(ms.GetDouble(), 1136995200000.0);
}
}  // namespace panda::test
