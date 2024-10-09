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

#include "cg_irbuilder.h"
#include "isa.h"
#include "cg.h"
#include "cfi.h"
#include "gtest/gtest.h"
using namespace std;
using namespace maplebe;
namespace {
TEST(CreateMem_FUNC, t01)
{
    MemPoolCtrler memPoolCtrler1, memPoolCtrler2;
    MemPool memPool1(memPoolCtrler1, "poolName1"), memPool2(memPoolCtrler2, "poolName2");
    OperandBuilder opbuilder_obj(memPool1, 30);
    RegOperand regop_obj(0, 0, kRegTyInt, 0);
    ImmOperand immop_obj(20, 8, false);
    MIRSymbol mirsymbol_obj;
    // test :CreateMem(uint32 size, RegOperand &baseOpnd, ImmOperand &ofstOperand, MemPool *mp)
    MemOperand *ptr1 = &(opbuilder_obj.CreateMem(100, regop_obj, immop_obj, &memPool2));
    EXPECT_NE(ptr1, nullptr);
    // test :CreateMem(uint32, RegOperand &, ImmOperand &,const MIRSymbol &, MemPool *)
    MemOperand *ptr2 = &(opbuilder_obj.CreateMem(100, regop_obj, immop_obj, mirsymbol_obj, &memPool2));
    EXPECT_NE(ptr2, nullptr);
}

TEST(CreateBitShift_FUNC, t02)
{
    MemPoolCtrler memPoolCtrler1, memPoolCtrler2;
    MemPool memPool1(memPoolCtrler1, "poolName1"), memPool2(memPoolCtrler2, "poolName2");
    // test method:CreateBitShift(BitShiftOperand::ShiftOp op, uint32 amount, uint32 bitLen, MemPool *mp)
    OperandBuilder opbuilder_obj(memPool1, 30);
    BitShiftOperand *ptr = &(opbuilder_obj.CreateBitShift(BitShiftOperand::ShiftOp::kASR, 1, 2, &memPool2));
    EXPECT_NE(ptr, nullptr);
}
}  // namespace