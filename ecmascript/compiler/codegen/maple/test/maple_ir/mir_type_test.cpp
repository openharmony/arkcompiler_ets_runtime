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

#include "mir_type.h"
#include "mir_symbol.h"
#include "printing.h"
#include "namemangler.h"
#include "global_tables.h"
#include "mir_builder.h"
#include "cfg_primitive_types.h"
#include "string_utils.h"
#include "triple.h"

#include "gtest/gtest.h"

using namespace maple;
namespace {
TEST(GetRegPrimType_FUNC, t01)
{
    PrimType input_ls[] = {PTY_i8, PTY_u1, static_cast<PrimType>(9000)};
    PrimType output_ls[] = {PTY_i32, PTY_u32, static_cast<PrimType>(9000)};
    for (int i = 0; i < sizeof(input_ls) / sizeof(input_ls[0]); i++)
    {
        EXPECT_EQ(GetRegPrimType(input_ls[i]), output_ls[i]);
    }
}

TEST(GetIntegerPrimTypeBySizeAndSign_FUNC, t02)
{
    size_t input_ls[] = {k1BitSize, k8BitSize, k16BitSize, k32BitSize, k64BitSize, static_cast<size_t>(2048U)};
    PrimType output_ls[][2] = {
        {PTY_u1, PTY_begin},
        {PTY_u8, PTY_i8},
        {PTY_u16, PTY_i16},
        {PTY_u32, PTY_i32},
        {PTY_u64, PTY_i64},
        {PTY_begin, PTY_begin}
    };
    for (int i = 0; i < sizeof(input_ls) / sizeof(input_ls[0]); i++)
    {
        for (int j = 0; j < 2; j++)
        {
            EXPECT_EQ(GetIntegerPrimTypeBySizeAndSign(input_ls[i], (bool)(j)), output_ls[i][j]);
        }
    }
}

TEST(IsNoCvtNeeded_FUNC, t03)
{
    PrimType input_ls[][2] = {
        {PTY_i32, PTY_i32},
        {PTY_i32, PTY_i16},
        {PTY_u32, PTY_u8},
        {PTY_u1, PTY_u32},
        {PTY_i8, PTY_i32},
        {PTY_i64, PTY_a64}
    };
    for (int i = 0; i < sizeof(input_ls) / sizeof(input_ls[0]); i++)
    {
        EXPECT_EQ(IsNoCvtNeeded(input_ls[i][0], input_ls[i][1]), true);
    }
    EXPECT_EQ(IsNoCvtNeeded(static_cast<PrimType>(900), static_cast<PrimType>(800)), false);
}

TEST(GetPrimTypeSize_FUNC, t04)
{
    PrimType input_ls[] = {PTY_void, PTY_u1, PTY_i16, PTY_f32, PTY_a64, static_cast<PrimType>(9000)};
    uint32 output_ls[] = {k0BitSize, k1BitSize, k2BitSize, k4BitSize, k8BitSize, k0BitSize};
    /*
    NOTE:GetPointerSize() reach #error "Unsupported target"
    */
    for (int i = 0; i < sizeof(input_ls) / sizeof(input_ls[0]); i++)
    {
        EXPECT_EQ(GetPrimTypeSize(input_ls[i]), output_ls[i]);
    }
}

TEST(GetUnsignedPrimType_FUNC, t06)
{
    PrimType input_ls[] = {PTY_i64, PTY_i8, PTY_i16, PTY_i32, static_cast<PrimType>(9000)};
    PrimType output_ls[] = {PTY_u64, PTY_u8, PTY_u16, PTY_u32, static_cast<PrimType>(9000)};
    for (int i = 0; i < sizeof(input_ls) / sizeof(input_ls[0]); i++)
    {
        EXPECT_EQ(GetUnsignedPrimType(input_ls[i]), output_ls[i]);
    }
}

}  // namespace