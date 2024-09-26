/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "mir_const.h"
#include "mir_function.h"
#include "global_tables.h"
#include "printing.h"
#include <iomanip>

#include "gtest/gtest.h"
#include <iostream>
#include "mir_builder.h"

using namespace maple;
namespace {
TEST(operator_EE_FUNC, t01)
{ // MIRIntConst::operator==
    MIRIntConst *mirConst_int_ptr1 = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
        1, *GlobalTables::GetTypeTable().GetInt64());
    MIRIntConst *mirConst_int_ptr2 = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
        1, *GlobalTables::GetTypeTable().GetInt64());
    MIRIntConst *mirConst_int_ptr3 = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
        2, *GlobalTables::GetTypeTable().GetInt64());
    MIRDoubleConst *mirConst_double_ptr1 = GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(1);
    // GetKind() =  kConstDoubleConst (8)

    bool ans1, ans2, ans3, ans4;
    ans1 = (*mirConst_int_ptr1 == *mirConst_int_ptr2);
    ans2 = (*mirConst_int_ptr1 == *mirConst_int_ptr3);
    ans3 = (*mirConst_int_ptr1 == *mirConst_int_ptr1);
    ans4 = (*mirConst_int_ptr1 == *mirConst_double_ptr1);

    EXPECT_EQ(ans1, true);
    EXPECT_EQ(ans2, false);
    EXPECT_EQ(ans3, true);
    EXPECT_EQ(ans4, false);
}

TEST(operator_EE_FUNC, t03)
{ // MIRAddrofConst::operator==  Incomplete!
    MIRIntConst *mirConst_int_ptr1 = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
        0, *GlobalTables::GetTypeTable().GetInt64());
    MIRDoubleConst *mirConst_double_ptr1 = GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(1);
    MIRAddrofConst *addrSymbol_1 = static_cast<MIRAddrofConst *>((MIRConst *)mirConst_int_ptr1);
    MIRAddrofConst *addrSymbol_2 = static_cast<MIRAddrofConst *>((MIRConst *)mirConst_double_ptr1);
    bool ans1 = (*addrSymbol_1 == *addrSymbol_1), ans2 = (*addrSymbol_1 == *addrSymbol_2);
    EXPECT_EQ(ans1, true);
    EXPECT_EQ(ans2, false);
}

TEST(operator_EE_FUNC, t04)
{ // MIRFloatConst::operator==
    MIRFloatConst *mirConst_float_ptr1 = GlobalTables::GetFpConstTable().GetOrCreateFloatConst(0.0);
    MIRFloatConst *mirConst_float_ptr2 = GlobalTables::GetFpConstTable().GetOrCreateFloatConst(0.0);
    MIRFloatConst *mirConst_float_ptr3 = GlobalTables::GetFpConstTable().GetOrCreateFloatConst(
        std::numeric_limits<float>::quiet_NaN());
    MIRFloatConst *mirConst_float_ptr4 = GlobalTables::GetFpConstTable().GetOrCreateFloatConst(0.1);
    MIRIntConst *mirConst_int_ptr1 = GlobalTables::GetIntConstTable().GetOrCreateIntConst(
        1, *GlobalTables::GetTypeTable().GetInt64());

    bool ans1, ans2, ans3, ans4, ans5, ans6;
    ans1 = (*mirConst_float_ptr1 == *mirConst_float_ptr1);
    ans2 = (*mirConst_int_ptr1 == *mirConst_float_ptr1);
    ans3 = (*mirConst_float_ptr1 == *mirConst_float_ptr3);
    ans4 = (*mirConst_float_ptr3 == *mirConst_float_ptr1);
    ans5 = (*mirConst_float_ptr1 == *mirConst_float_ptr2);
    ans6 = (*mirConst_float_ptr1 == *mirConst_float_ptr4);

    EXPECT_EQ(ans1, true);
    EXPECT_EQ(ans2, false);
    EXPECT_EQ(ans3, false);
    EXPECT_EQ(ans4, false);
    EXPECT_EQ(ans5, true);
    EXPECT_EQ(ans6, false);
}

TEST(operator_EE_FUNC, t05)
{ // MIRDoubleConst::operator==
    MIRDoubleConst *mirConst_double_ptr1 = GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(0.0);
    MIRDoubleConst *mirConst_double_ptr2 = GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(0.0);
    MIRDoubleConst *mirConst_double_ptr3 = GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(
        std::numeric_limits<float>::quiet_NaN());
    MIRDoubleConst *mirConst_double_ptr4 = GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(0.1);

    MIRFloatConst *mirConst_float_ptr1 = GlobalTables::GetFpConstTable().GetOrCreateFloatConst(0.0);

    bool ans1, ans2, ans3, ans4, ans5, ans6;
    ans1 = (*mirConst_double_ptr1 == *mirConst_double_ptr1);
    ans2 = (*mirConst_double_ptr1 == *mirConst_float_ptr1);
    ans3 = (*mirConst_double_ptr1 == *mirConst_double_ptr3);
    ans4 = (*mirConst_double_ptr3 == *mirConst_double_ptr1);
    ans5 = (*mirConst_double_ptr1 == *mirConst_double_ptr2);
    ans6 = (*mirConst_double_ptr1 == *mirConst_double_ptr4);
    EXPECT_EQ(ans1, true);
    EXPECT_EQ(ans2, false);
    EXPECT_EQ(ans3, false);
    EXPECT_EQ(ans4, false);
    EXPECT_EQ(ans5, true);
    EXPECT_EQ(ans6, false);
}
}  // namespace