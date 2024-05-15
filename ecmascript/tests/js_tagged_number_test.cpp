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

#include "ecmascript/js_tagged_number.h"
#include "ecmascript/base/number_helper.h"
#include "ecmascript/tests/test_helper.h"

using namespace panda;
using namespace panda::ecmascript;
using namespace panda::ecmascript::base;

namespace panda::test {
class JSTaggedNumberTest : public BaseTestWithScope<false> {
};

HWTEST_F_L0(JSTaggedNumberTest, TaggedNumber_Minus)
{
    // number is int
    JSTaggedNumber number1(123);
    JSTaggedNumber number2(-123);
    JSTaggedNumber result = number1 - number2;
    EXPECT_EQ(result.GetInt(), 246);
    // number is INT32_MAX
    JSTaggedNumber number7(INT32_MAX);
    JSTaggedNumber number8(-INT32_MAX);
    result = number7 - number8;
    EXPECT_EQ(result, JSTaggedNumber(static_cast<double>(4294967294)));
    // number is double
    JSTaggedNumber number3(static_cast<double>(123.456));
    JSTaggedNumber number4(static_cast<double>(-123.456));
    result = number3 - number4;
    EXPECT_EQ(result, JSTaggedNumber(static_cast<double>(246.912)));
    // number is NAN
    JSTaggedNumber number5(NAN_VALUE);
    EXPECT_FALSE(number5.IsImpureNaN(number5.GetNumber()));
}

HWTEST_F_L0(JSTaggedNumberTest, TaggedNumber_Multiply)
{
    // both numbers are int
    JSTaggedNumber number1(12);
    JSTaggedNumber number2(-12);
    JSTaggedNumber result = number1 * number2;
    EXPECT_EQ(result.GetInt(), -144);
    // one numer is int,one number is double
    JSTaggedNumber number3(12);
    JSTaggedNumber number4(static_cast<double>(12.1));
    result = number3 * number4;
    EXPECT_EQ(result, JSTaggedNumber(145.2));
    // number is double
    JSTaggedNumber number5(static_cast<double>(12.12));
    JSTaggedNumber number6(static_cast<double>(12.12));
    result = number5 * number6;
    EXPECT_EQ(result, JSTaggedNumber(static_cast<double>(146.8944)));
}

HWTEST_F_L0(JSTaggedNumberTest, TaggedNumber_AddAndSubself)
{
    // number is int
    JSTaggedNumber number(12);
    JSTaggedNumber result;
    result = ++number;
    EXPECT_EQ(result.GetNumber(), 13);
    result = --number;
    EXPECT_EQ(result.GetNumber(), 11);
    // number is double
    JSTaggedNumber number1(static_cast<double>(12.1));
    result = ++number1;
    EXPECT_EQ(result.GetNumber(), 13.1);
    result = --number1;
    EXPECT_EQ(result.GetNumber(), 11.1);
}

HWTEST_F_L0(JSTaggedNumberTest, TaggedNumber_NotEqual)
{
    // number is nan
    JSTaggedNumber number1(NAN_VALUE);
    JSTaggedNumber number2(NAN_VALUE);
    EXPECT_TRUE(JSTaggedNumber::SameValue(number1, number2));
    EXPECT_TRUE(number1 != number2);
    // number is zero
    JSTaggedNumber number3(static_cast<double>(0.0));
    JSTaggedNumber number4(static_cast<double>(-0.0));
    EXPECT_FALSE(JSTaggedNumber::SameValue(number3, number4));
    EXPECT_FALSE(number3 != number4);
    // number is int
    JSTaggedNumber number5(1000);
    JSTaggedNumber number6(1000);
    EXPECT_TRUE(JSTaggedNumber::SameValue(number5, number6));
    EXPECT_FALSE(number5 != number6);
    // number is double
    JSTaggedNumber number7(12.12345);
    JSTaggedNumber number8(12.12345);
    EXPECT_TRUE(JSTaggedNumber::SameValue(number7, number8));
    EXPECT_FALSE(number7 != number8);
}
}  // namespace panda::test