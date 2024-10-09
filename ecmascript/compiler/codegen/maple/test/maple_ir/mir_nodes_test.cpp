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

#include "mir_nodes.h"
#include <algorithm>
#include <stack>
#include "maple_string.h"
#include "mir_function.h"
#include "namemangler.h"
#include "opcode_info.h"
#include "printing.h"
#include "utils.h"

#include "gtest/gtest.h"
#include <iostream>
using namespace std;
using namespace maple;

namespace {

TEST(GetIntrinsicName_FUNC, t01)
{
    std::vector<MIRIntrinsicID> input_ls_intrinsic_js = {
        INTRN_ADD_WITH_OVERFLOW, INTRN_SUB_WITH_OVERFLOW, INTRN_MUL_WITH_OVERFLOW};
    std::vector<const char *> output_ls_intrinsic_js = {
        "ADD_WITH_OVERFLOW", "SUB_WITH_OVERFLOW", "MUL_WITH_OVERFLOW"};

    std::vector<MIRIntrinsicID> input_ls_intrinsic_c = {INTRN_C_clz32, INTRN_C_clz64};
    std::vector<const char *> output_ls_intrinsic_c = {"C_clz32", "C_clz64"};

    ASSERT_EQ(input_ls_intrinsic_js.size(), output_ls_intrinsic_js.size());
    ASSERT_EQ(input_ls_intrinsic_c.size(), output_ls_intrinsic_c.size());
    for (int i = 0; i < input_ls_intrinsic_js.size(); i++)
    {
        ASSERT_STREQ(output_ls_intrinsic_js[i], GetIntrinsicName(input_ls_intrinsic_js[i]));
    }
    for (int i = 0; i < input_ls_intrinsic_c.size(); i++)
    {
        ASSERT_STREQ(output_ls_intrinsic_c[i], GetIntrinsicName(input_ls_intrinsic_c[i]));
    }
}
}  // namespace