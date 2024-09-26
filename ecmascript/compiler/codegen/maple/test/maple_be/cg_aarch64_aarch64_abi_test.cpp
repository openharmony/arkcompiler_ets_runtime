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

#include "aarch64_cgfunc.h"
#include "becommon.h"

#include "gtest/gtest.h"
#include <iostream>
using namespace std;

using namespace maplebe;
using namespace AArch64Abi;

namespace {
TEST(FourSwitch_FUNC, t01)
{
    AArch64reg inputLs[] = {R0, V0, static_cast<AArch64reg>(900)};
    bool outputLs[][4] = {{true, false, false, false},
                          {true, false, false, false},
                          {false, false, false, false}};
    bool (*funcLs[])(AArch64reg) = {IsAvailableReg, IsCalleeSavedReg, IsSpillReg, IsExtraSpillReg};

    for (int i = 0; i < sizeof(inputLs) / sizeof(inputLs[0]); i++)
    { // 3 cases input
        for (int j = 0; j < 4; j++)
        { // 4 functions
            EXPECT_EQ(funcLs[j](inputLs[i]), outputLs[i][j]);
        }
    }
}
}  // namespace