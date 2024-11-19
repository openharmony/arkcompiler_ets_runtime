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

#include "intrinsics.h"
#include "mir_module.h"
#include "mir_type.h"
#include "mir_builder.h"

#include "gtest/gtest.h"
#include <iostream>
using namespace maple;
using namespace std;
namespace {
TEST(GetTypeFromArgTy_FUNC, t01)
{
    struct IntrinDesc IntrinDesc_struc;
    IntrinArgType input_ls[] = {kArgTyVoid, kArgTyI8, kArgTyI16, kArgTyI32, kArgTyI64, kArgTyU8,
                                kArgTyU16, kArgTyU32, kArgTyU64, kArgTyU1,
                                kArgTyA64, kArgTyF32, kArgTyF64};
    PrimType res_ls[] = {
            PTY_void, PTY_i8, PTY_i16, PTY_i32, PTY_i64, PTY_u8,
            PTY_u16, PTY_u32, PTY_u64, PTY_u1,
            PTY_a64, PTY_f32, PTY_f64
    };
    MIRType *mirtype_ptr;
    int n = sizeof(input_ls) / sizeof(input_ls[0]);
    for (int i = 0; i < n; i++)
    {
        mirtype_ptr = IntrinDesc_struc.GetTypeFromArgTy(input_ls[i]);
        ASSERT_TRUE(mirtype_ptr != nullptr);
        EXPECT_EQ(mirtype_ptr->GetPrimType(), res_ls[i]);
    }
}
}  // namespace
