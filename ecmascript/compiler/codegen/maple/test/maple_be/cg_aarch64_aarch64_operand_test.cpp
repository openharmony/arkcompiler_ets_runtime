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

#include "aarch64_operand.h"
#include <fstream>
#include <string>
#include "aarch64_abi.h"
#include "aarch64_cgfunc.h"
#include "aarch64_cg.h"

#include "gtest/gtest.h"
#include <iostream>
#include <unistd.h>

#include "operand.h"
#include "mempool.h"
using namespace maplebe;
using namespace maple;
using namespace std;
namespace {
static StImmOperand MyCreateStImmOperand(std::string poolname, int64 offset, int32 relocs)
{
    MIRSymbol mirsymbol_obj;
    mirsymbol_obj.SetNameStrIdx(poolname);
    MemPoolCtrler memPoolCtrler;
    MemPool memPool(memPoolCtrler, poolname);
    return *(memPool.New<StImmOperand>(mirsymbol_obj, offset, relocs));
}

static OfstOperand MyCreateOfstOpnd(std::string poolname, int64 offset, int32 relocs)
{
    MIRSymbol mirsymbol_obj;
    mirsymbol_obj.SetNameStrIdx(poolname);
    MemPoolCtrler memPoolCtrler;
    MemPool memPool(memPoolCtrler, poolname);
    return *(memPool.New<OfstOperand>(mirsymbol_obj, 0, offset, relocs));
}

static ExtendShiftOperand MyCreateExtendShiftOperand(std::string poolname, ExtendShiftOperand::ExtendOp op,
                                                     uint32 amount, int32 bitLen)
{
    MemPoolCtrler memPoolCtrler;
    MemPool memPool(memPoolCtrler, poolname);
    return *(memPool.New<ExtendShiftOperand>(op, amount, bitLen));
}

TEST(Less_FUNC, t01)
{ // StImmOperand::Less
    StImmOperand stimmoperand_obj1 = MyCreateStImmOperand("pool1", 1, 1),
                 stimmoperand_obj2 = MyCreateStImmOperand("pool1", 1, 2);
    StImmOperand stimmoperand_obj3 = MyCreateStImmOperand("pool2", 0, 2),
                 stimmoperand_obj4 = MyCreateStImmOperand("pool1", 0, 2);
    OfstOperand ofstoperand_obj = MyCreateOfstOpnd("pool", 1, 2);
    bool ans1, ans2, ans3, ans4, ans5;

    ans1 = stimmoperand_obj1.Less(stimmoperand_obj1);
    ans2 = stimmoperand_obj1.Less(ofstoperand_obj);
    ans3 = stimmoperand_obj1.Less(stimmoperand_obj3); // symbol diff
    ans4 = stimmoperand_obj1.Less(stimmoperand_obj4);
    ans5 = stimmoperand_obj1.Less(stimmoperand_obj2);

    EXPECT_EQ(ans1, false);
    EXPECT_EQ(ans2, true);
    EXPECT_EQ(ans3, false);
    EXPECT_EQ(ans4, false);
    EXPECT_EQ(ans5, true);
}

TEST(Less_FUNC, t02)
{ // ExtendShiftOperand::Less
    ExtendShiftOperand ext_shiftopnd_obj1 = MyCreateExtendShiftOperand("pool",
                                                                       ExtendShiftOperand::ExtendOp::kUXTB, 1, 1);
    ExtendShiftOperand ext_shiftopnd_obj2 = MyCreateExtendShiftOperand("pool",
                                                                       ExtendShiftOperand::ExtendOp::kUXTH, 1, 1);
    ExtendShiftOperand ext_shiftopnd_obj3 = MyCreateExtendShiftOperand("pool",
                                                                       ExtendShiftOperand::ExtendOp::kUXTH, 2, 1);
    StImmOperand stimmoperand_obj1 = MyCreateStImmOperand("pool1", 1, 1);

    bool ans1, ans2, ans3, ans4;
    ans1 = ext_shiftopnd_obj1.Less(ext_shiftopnd_obj1);
    ans2 = ext_shiftopnd_obj1.Less(stimmoperand_obj1);
    ans3 = ext_shiftopnd_obj1.Less(ext_shiftopnd_obj2);
    ans4 = ext_shiftopnd_obj2.Less(ext_shiftopnd_obj3);

    EXPECT_EQ(ans1, false);
    EXPECT_EQ(ans2, false);
    EXPECT_EQ(ans3, true);
    EXPECT_EQ(ans4, true);
}
}  // namespace