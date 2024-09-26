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

#include "gtest/gtest.h"
#include <iostream>
#include <unistd.h>
#include <climits>

#include "x64_emitter.h"
#include "x64_cgfunc.h"
#include "x64_cg.h"
#include "insn.h"

using namespace maplebe;
using namespace x64;
using namespace std;
namespace {
TEST(FlipConditionOp_FUNC, t01)
{
    const maple::uint32 other_op = 500u;
    std::vector<X64MOP_t> input_ls = {MOP_je_l, MOP_jne_l, MOP_ja_l, MOP_jbe_l,
                           MOP_jae_l, MOP_jb_l, MOP_jg_l, MOP_jle_l,
                           MOP_jge_l, MOP_jl_l};
    std::vector<X64MOP_t> output_ls = {MOP_jne_l, MOP_je_l, MOP_jbe_l, MOP_ja_l,
                            MOP_jb_l, MOP_jae_l, MOP_jle_l, MOP_jg_l,
                            MOP_jl_l, MOP_jge_l};
    for (int i = 0; i < input_ls.size(); i++)
    {
        EXPECT_EQ(FlipConditionOp(input_ls[i]), output_ls[i]);
    }
    EXPECT_EQ(FlipConditionOp(other_op), MOP_begin);
}
}  // namespace