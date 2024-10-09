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

#include "aarch64_isa.h"
#include "insn.h"

using namespace maplebe;
using namespace std;
using namespace AArch64isa;
namespace {
TEST(GetMopPair_FUNC, t01)
{
    bool isIncludeStrbStrh = true;
    AArch64MopT input_ls1[] = {MOP_xldr, MOP_wldr, MOP_xstr, MOP_wstr, MOP_dldr, MOP_qldr, MOP_sldr,
                               MOP_dstr, MOP_sstr, MOP_qstr},
                input_ls2[] = {MOP_wstrb, MOP_wstrh};
    AArch64MopT output_ls1[] = {MOP_xldp, MOP_wldp, MOP_xstp, MOP_wstp, MOP_dldp, MOP_qldp, MOP_sldp,
                                MOP_dstp, MOP_sstp, MOP_qstp},
                output_ls2[] = {MOP_wstrh, MOP_wstr};
    for (int i = 0; i < sizeof(input_ls1) / sizeof(input_ls1[0]); i++)
    {
        EXPECT_EQ(GetMopPair(input_ls1[i], isIncludeStrbStrh), output_ls1[i]);
    }
    for (int i = 0; i < sizeof(input_ls2) / sizeof(input_ls2[0]); i++)
    {
        EXPECT_EQ(GetMopPair(input_ls2[i], isIncludeStrbStrh), output_ls2[i]);
        EXPECT_EQ(GetMopPair(input_ls2[i], !isIncludeStrbStrh), MOP_undef);
    }
}

TEST(FlipConditionOp_FUNC, t02)
{
    AArch64MopT input_ls[] = {
        MOP_beq, MOP_bge, MOP_bgt, MOP_bhi, MOP_bhs, MOP_ble, MOP_blo, MOP_bls,
        MOP_blt, MOP_bne, MOP_bpl, MOP_xcbnz, MOP_wcbnz, MOP_xcbz, MOP_wcbz, MOP_wtbnz,
        MOP_wtbz, MOP_xtbnz, MOP_xtbz};
    AArch64MopT output_ls[] = {
        MOP_bne, MOP_blt, MOP_ble, MOP_bls, MOP_blo, MOP_bgt, MOP_bhs, MOP_bhi,
        MOP_bge, MOP_beq, MOP_bmi, MOP_xcbz, MOP_wcbz, MOP_xcbnz, MOP_wcbnz, MOP_wtbz,
        MOP_wtbnz, MOP_xtbz, MOP_xtbnz};
    for (int i = 0; i < sizeof(input_ls) / sizeof(input_ls[0]); i++)
    {
        EXPECT_EQ(FlipConditionOp(input_ls[i]), output_ls[i]);
    }
    EXPECT_EQ(FlipConditionOp(9999), MOP_undef);
}

TEST(GetJumpTargetIdx_FUNC, t03)
{
    MemPoolCtrler memPoolCtrler;
    MemPool memPool(memPoolCtrler, "poolName");
    std::vector<maplebe::Insn> insnobj_ls;
    AArch64MopT input_ls[] = {MOP_xuncond, MOP_bmi, MOP_wtbz};
    int32 output_ls[] = {kInsnFirstOpnd, kInsnSecondOpnd, kInsnThirdOpnd};

    int i;
    for (i = 0; i < sizeof(input_ls) / sizeof(input_ls[0]); i++)
    {
        insnobj_ls.emplace_back(memPool, input_ls[i]);
    }
    i = 0;
    for (const auto &insn : insnobj_ls)
    {
        EXPECT_EQ(GetJumpTargetIdx(insn), output_ls[i++]);
    }
}

TEST(IsSub_FUNC, t04)
{
    MemPoolCtrler memPoolCtrler;
    MemPool memPool(memPoolCtrler, "poolName");
    maplebe::Insn insn_obj1(memPool, MOP_xsubrrr), insn_obj2(memPool, 999);
    EXPECT_EQ(IsSub(insn_obj1), true);
    EXPECT_EQ(IsSub(insn_obj2), false);
}

TEST(GetMopSub2Subs_FUNC, t05)
{
    AArch64MopT input_ls[] = {MOP_xsubrrr, MOP_xsubrrrs, MOP_xsubrri24, MOP_xsubrri12, MOP_wsubrrr,
                              MOP_wsubrrrs, MOP_wsubrri24, MOP_wsubrri12};
    AArch64MopT output_ls[] = {MOP_xsubsrrr, MOP_xsubsrrrs, MOP_xsubsrri24, MOP_xsubsrri12, MOP_wsubsrrr,
                               MOP_wsubsrrrs, MOP_wsubsrri24, MOP_wsubsrri12};

    MemPoolCtrler memPoolCtrler;
    MemPool memPool(memPoolCtrler, "poolName");
    std::vector<maplebe::Insn> insnobj_ls;
    int i;
    for (i = 0; i < sizeof(input_ls) / sizeof(input_ls[0]); i++)
    {
        insnobj_ls.emplace_back(memPool, input_ls[i]);
    }
    i = 0;
    for (const auto &insn : insnobj_ls)
    {
        EXPECT_EQ(GetMopSub2Subs(insn), output_ls[i++]);
    }
    maplebe::Insn insn_obj(memPool, 999);
    EXPECT_EQ(GetMopSub2Subs(insn_obj), insn_obj.GetMachineOpcode());
}
}  // namespace