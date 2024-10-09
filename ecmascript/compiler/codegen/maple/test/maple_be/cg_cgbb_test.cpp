/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "cgbb.h"
#include "mempool.h"
#include "cgfunc.h"

using namespace maplebe;
using namespace std;
namespace {
static maplebe::Insn CreateInsnObj(std::string poolName, maple::uint32 opc)
{
    MemPoolCtrler memPoolCtrler;
    MemPool memPool(memPoolCtrler, poolName);
    maplebe::Insn Insn_obj(memPool, opc);
    return Insn_obj;
}
static maplebe::BB CreateBBObj(uint32 bbID, std::string poolName)
{
    // BBID bbID, MapleAllocator &mallocator
    MemPoolCtrler memPoolCtrler;
    MemPool memPool(memPoolCtrler, poolName);
    MapleAllocator mallocator(&memPool);
    maplebe::BB BB_obj(bbID, mallocator);
    return BB_obj;
}

TEST(InsertInsnBefore_FUNC, t01)
{
    maplebe::BB test_bb = CreateBBObj(10, "BB_pool");
    maplebe::Insn existing = CreateInsnObj("existing", 32);
    maplebe::Insn newInsn = CreateInsnObj("newInsn", 3);

    test_bb.SetFirstInsn(&existing);
    EXPECT_EQ(existing.GetPrev(), nullptr);
    maplebe::Insn *ans = test_bb.InsertInsnBefore(existing, newInsn);
    EXPECT_EQ(ans, &newInsn);

    maplebe::Insn newInsn1 = CreateInsnObj("newInsn1", 3);
    EXPECT_NE(existing.GetPrev(), nullptr);
    ans = test_bb.InsertInsnBefore(existing, newInsn1);
    EXPECT_EQ(ans, &newInsn1);
    EXPECT_EQ(newInsn.GetNext(), &newInsn1);
}

TEST(InsertInsnAfter_FUNC, t02)
{
    maplebe::BB test_bb = CreateBBObj(2, "BB_pool");
    maplebe::Insn existing = CreateInsnObj("existing", 32);
    maplebe::Insn newInsn1 = CreateInsnObj("newInsn1", 1), newInsn2 = CreateInsnObj("newInsn2", 2);

    maplebe::Insn *ans;
    test_bb.SetFirstInsn(&existing);
    ans = test_bb.InsertInsnAfter(existing, newInsn1);
    EXPECT_EQ(ans, &newInsn1);
    EXPECT_EQ(newInsn1.GetNext(), nullptr);

    ans = test_bb.InsertInsnAfter(existing, newInsn2);
    EXPECT_EQ(ans, &newInsn2);
    EXPECT_EQ(newInsn2.GetNext(), &newInsn1);
}

TEST(RemoveInsn_FUNC, t03)
{
    /*void BB::RemoveInsn(Insn &insn)*/
    maplebe::BB test_bb = CreateBBObj(10, "BB_pool");
    maplebe::Insn insn = CreateInsnObj("insn", 0);
    maplebe::Insn insn1 = CreateInsnObj("insn1", 1);
    maplebe::Insn insn2 = CreateInsnObj("insn2", 2);

    test_bb.SetFirstInsn(&insn);
    test_bb.RemoveInsn(insn);
    EXPECT_EQ(test_bb.GetFirstInsn(), nullptr);

    test_bb.SetFirstInsn(&insn);
    test_bb.InsertInsnAfter(insn, insn1);
    test_bb.InsertInsnAfter(insn1, insn2);
    test_bb.RemoveInsn(insn);
    EXPECT_EQ(test_bb.GetFirstInsn(), &insn1);
    EXPECT_EQ((*(insn.GetNext())).GetPrev(), nullptr);
}
}  // namespace