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

#include "constantfold.h"
#include <cmath>
#include <cfloat>
#include <climits>
#include <type_traits>
#include "mpl_logging.h"
#include "mir_function.h"
#include "mir_builder.h"
#include "global_tables.h"
#include "me_option.h"
#include "maple_phase_manager.h"
#include "mir_type.h"

#include "gtest/gtest.h"

using namespace maple;
using namespace std;
namespace {
TEST(FoldIntConstBinaryMIRConst_FUNC, t01)
{
    MIRIntConst *mc_int_ptr1, *mc_int_ptr2;
    mc_int_ptr1 = GlobalTables::GetIntConstTable().GetOrCreateIntConst(2, *GlobalTables::GetTypeTable().GetInt64());
    mc_int_ptr2 = GlobalTables::GetIntConstTable().GetOrCreateIntConst(1, *GlobalTables::GetTypeTable().GetInt64());
    std::vector<Opcode> input_op_ls = {OP_add, OP_sub, OP_mul, OP_div, OP_rem, OP_ashr,
                                       OP_lshr, OP_shl, OP_max, OP_min, OP_band, OP_bior, OP_bxor};
    std::vector<uint64> output_ls = {3, 1, 2, 2, 0, 1, 1, 4, 2, 1, 0, 3, 3};
    PrimType resultType = PTY_i64;

    MIRModule mirmodule("mirmodule");
    ConstantFold cf_obj(mirmodule, false);
    MIRConst *ans;
    ASSERT_EQ(input_op_ls.size(), output_ls.size());
    for (int i = 0; i < input_op_ls.size(); i++)
    {
        ans = cf_obj.FoldIntConstBinaryMIRConst(input_op_ls[i], resultType, *mc_int_ptr1, *mc_int_ptr2);
        EXPECT_EQ(((MIRIntConst *)ans)->GetExtValue(), output_ls[i]);
    }
}

TEST(FoldFPConstBinary_FUNC, t02)
{
    MIRModule mirmodule("mirmodule"), mirmodule2("mirmodule2");
    // NOTE:After creating a MIRModule class object, mirFunc must be set on the object
    StIdx sdidx_obj;
    MemPoolCtrler memPoolCtrler;
    MemPool memPool(memPoolCtrler, "poolName");
    MIRFunction mir_func(&mirmodule2, sdidx_obj);
    mir_func.SetMemPool(&memPool);
    mirmodule.SetCurFunction(&mir_func);

    ConstvalNode constvalnode0, constvalnode1;
    std::vector<PrimType> primtype_ls = {PTY_f32, PTY_f64};
    std::vector<Opcode> input_op_ls = {OP_add, OP_sub, OP_mul, OP_div, OP_max, OP_min};
    MIRFloatConst *mirConst_float_ptr1 = GlobalTables::GetFpConstTable().GetOrCreateFloatConst(1.0);
    MIRFloatConst *mirConst_float_ptr2 = GlobalTables::GetFpConstTable().GetOrCreateFloatConst(2.0);

    constvalnode0.SetConstVal(mirConst_float_ptr1);
    constvalnode1.SetConstVal(mirConst_float_ptr2);

    ConstantFold cf_obj(mirmodule, false);
    for (int i = 0; i < input_op_ls.size(); i++)
    {
        for (int j = 0; j < primtype_ls.size(); j++)
        {
            BinaryNode root(input_op_ls[i], primtype_ls[j], &constvalnode0, &constvalnode1);
            BaseNode *ans = cf_obj.Fold(&root);
            EXPECT_EQ(ans->IsConstval(), true);
        }
    }
}

TEST(FoldCompare_FUNC, t03)
{
    MIRModule mirmodule("mirmodule"), mirmodule2("mirmodule2");
    StIdx sdidx_obj;
    MemPoolCtrler memPoolCtrler;
    MemPool memPool(memPoolCtrler, "poolName");
    MIRFunction mir_func(&mirmodule2, sdidx_obj); // mir_func Can be designed arbitrarily
    mir_func.SetMemPool(&memPool);                // The SetMemPool function must be called
    mirmodule.SetCurFunction(&mir_func);

    PrimType primtyp = PTY_i64;
    PrimType opndType = PTY_i64; // PTY_64 -> ... -> FoldIntConstComparisonMIRConst
    std::vector<Opcode> opcode_ls = {OP_eq, OP_ge, OP_gt, OP_le, OP_lt, OP_ne, OP_cmp};

    MIRIntConst *mc_int_ptr0, *mc_int_ptr1;
    mc_int_ptr0 = GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *GlobalTables::GetTypeTable().GetInt64());
    mc_int_ptr1 = GlobalTables::GetIntConstTable().GetOrCreateIntConst(1, *GlobalTables::GetTypeTable().GetInt64());

    ConstantFold cf_obj(mirmodule, false);
    ConstvalNode constvalnode0, constvalnode1;

    for (int opc_idx = 0; opc_idx < opcode_ls.size(); opc_idx++)
    {
        constvalnode0.SetConstVal(mc_int_ptr0);
        constvalnode1.SetConstVal(mc_int_ptr1);
        CompareNode root(opcode_ls[opc_idx], primtyp, opndType, &constvalnode0, &constvalnode1);
        BaseNode *ans = cf_obj.Fold(&root);
        EXPECT_EQ(ans->IsConstval(), true);
    }
}

TEST(FoldCompare_FUNC, t04)
{
    MIRModule mirmodule("mirmodule"), mirmodule2("mirmodule2");
    StIdx sdidx_obj;
    MemPoolCtrler memPoolCtrler;
    MemPool memPool(memPoolCtrler, "poolName");
    MIRFunction mir_func(&mirmodule2, sdidx_obj);
    mir_func.SetMemPool(&memPool);
    mirmodule.SetCurFunction(&mir_func);

    PrimType primtype = PTY_i64;
    std::vector<PrimType> opndType_ls = {PTY_f32, PTY_f64}; // PTY_f32 or PTY_f64 -> ... -> ComparisonResult
    std::vector<Opcode> opcode_ls = {OP_eq, OP_ge, OP_gt, OP_le, OP_lt, OP_ne, OP_cmp};

    MIRConst *ptr_ls[2][2] = {
        {GlobalTables::GetFpConstTable().GetOrCreateFloatConst(1.0),
         GlobalTables::GetFpConstTable().GetOrCreateFloatConst(2.0)},
        {GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(1.0),
         GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(2.0)}};

    ConstantFold cf_obj(mirmodule, false);
    ConstvalNode constvalnode0, constvalnode1;

    for (int opc_idx = 0; opc_idx < opcode_ls.size(); opc_idx++)
    {
        for (int i = 0; i < opndType_ls.size(); i++)
        {
            // NOTE: if opndType==PTY_f32,constvalnode1.SetConstVal() actual paremeter must be PTY_f32
            constvalnode0.SetConstVal(ptr_ls[i][0]);
            constvalnode1.SetConstVal(ptr_ls[i][1]);
            CompareNode root(opcode_ls[opc_idx], primtype, opndType_ls[i], &constvalnode0, &constvalnode1);
            BaseNode *ans = cf_obj.Fold(&root);
            EXPECT_EQ(ans->IsConstval(), true);
        }
    }
}

TEST(FoldUnary_FUNC, t05)
{
    MIRModule mirmodule("mirmodule"), mirmodule2("mirmodule2");
    StIdx sdidx_obj;
    MemPoolCtrler memPoolCtrler;
    MemPool memPool(memPoolCtrler, "poolName");
    MIRFunction mir_func(&mirmodule2, sdidx_obj);
    mir_func.SetMemPool(&memPool);
    mirmodule.SetCurFunction(&mir_func);

    std::vector<PrimType> primtype_ls = {PTY_i64, PTY_f32, PTY_f64};
    std::vector<std::vector<Opcode>> opcode_ls = {
        {OP_abs, OP_bnot, OP_lnot, OP_neg},
        {OP_abs, OP_neg, OP_sqrt}};
    std::vector<MIRConst *> const_ptr_ls = {
        GlobalTables::GetIntConstTable().GetOrCreateIntConst(0, *GlobalTables::GetTypeTable().GetInt64()),
        GlobalTables::GetFpConstTable().GetOrCreateFloatConst(1.0),
        GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(1.0)};
    ConstantFold cf_obj(mirmodule, false);
    ConstvalNode constvalnode;

    for (int i = 0; i < primtype_ls.size(); i++)
    {
        if (!i)
        { // i=0:integer
            for (int opc_idx = 0; opc_idx < opcode_ls[0].size(); opc_idx++)
            {
                constvalnode.SetConstVal(const_ptr_ls[i]);
                UnaryNode root(opcode_ls[0][opc_idx], primtype_ls[i], &constvalnode);
                BaseNode *ans = cf_obj.Fold(&root);
                EXPECT_EQ(ans->IsConstval(), true);
            }
        }
        else
        { // i=1:float ,i=2:double
            for (int opc_idx = 0; opc_idx < opcode_ls[1].size(); opc_idx++)
            {
                constvalnode.SetConstVal(const_ptr_ls[i]);
                UnaryNode root(opcode_ls[1][opc_idx], primtype_ls[i], &constvalnode);
                BaseNode *ans = cf_obj.Fold(&root);
                EXPECT_EQ(ans->IsConstval(), true);
            }
        }
    }
}
// The from and to types required to enter the Ceil branch {from,to}.Pay attention to modifying with 'static'
static std::vector<std::vector<PrimType>> primtype_pairs_ceil = {
    {PTY_f32, PTY_f32}, {PTY_f32, PTY_u64}, {PTY_f64, PTY_f64}, {PTY_f64, PTY_u64}};
static std::vector<std::vector<PrimType>> primtype_pairs_floor = {
    {PTY_f32, PTY_i64}, {PTY_f64, PTY_i64}, {PTY_i64, PTY_f32},
    {PTY_u64, PTY_f32}, {PTY_i64, PTY_f64}, {PTY_u64, PTY_f64}};
static std::vector<std::vector<PrimType>> primtype_pairs_cvt = {
    {PTY_u1, PTY_u1}, {PTY_f32, PTY_u8}};
static std::vector<std::vector<PrimType>> primtype_pairs_trunc = {
    {PTY_f32, PTY_f32}, {PTY_f32, PTY_u64}, {PTY_f64, PTY_f64}, {PTY_f64, PTY_u64}};
static std::unordered_map<Opcode, std::vector<std::vector<PrimType>>> opcode_to_primtype_pairs = {
    {OP_ceil, primtype_pairs_ceil}, {OP_floor, primtype_pairs_floor},
    {OP_cvt, primtype_pairs_cvt}, {OP_trunc, primtype_pairs_trunc}};

TEST(FoldTypeCvt_FUNC, t06)
{
    MIRModule mirmodule("mirmodule"), mirmodule2("mirmodule2");
    StIdx sdidx_obj;
    MemPoolCtrler memPoolCtrler;
    MemPool memPool(memPoolCtrler, "poolName");
    MIRFunction mir_func(&mirmodule2, sdidx_obj);
    mir_func.SetMemPool(&memPool);
    mirmodule.SetCurFunction(&mir_func);
    std::vector<Opcode> opcode_ls = {OP_ceil, OP_floor, OP_trunc, OP_cvt};
    std::unordered_map<PrimType, MIRConst *> const_ptr_map = {
        {PTY_f32, GlobalTables::GetFpConstTable().GetOrCreateFloatConst(1.5)},
        {PTY_u64, GlobalTables::GetIntConstTable().GetOrCreateIntConst(1,
                                                                       *GlobalTables::GetTypeTable().GetUInt64())},
        {PTY_f64, GlobalTables::GetFpConstTable().GetOrCreateDoubleConst(1.5)},
        {PTY_i64, GlobalTables::GetIntConstTable().GetOrCreateIntConst(1,
                                                                       *GlobalTables::GetTypeTable().GetInt64())},
        {PTY_u1, GlobalTables::GetIntConstTable().GetOrCreateIntConst(1,
                                                                      *GlobalTables::GetTypeTable().GetUInt1())},
        {PTY_u8, GlobalTables::GetIntConstTable().GetOrCreateIntConst(1,
                                                                      *GlobalTables::GetTypeTable().GetUInt8())}};
    ConstantFold cf_obj(mirmodule, false);
    ConstvalNode constvalnode;
    for (int opc_idx = 0; opc_idx < opcode_ls.size(); opc_idx++)
    {
        Opcode opcode = opcode_ls[opc_idx];
        for (int pairs_idx = 0; pairs_idx < opcode_to_primtype_pairs[opcode].size(); pairs_idx++)
        {
            PrimType from = opcode_to_primtype_pairs[opcode][pairs_idx][0],
                     to = opcode_to_primtype_pairs[opcode][pairs_idx][1];
            constvalnode.SetConstVal(const_ptr_map[from]);
            TypeCvtNode type_cvt_node(opcode, to, from, &constvalnode);
            BaseNode *ans = cf_obj.Fold(&type_cvt_node);
            EXPECT_EQ(ans->IsConstval(), true);
        }
    }
}
}  // namespace
