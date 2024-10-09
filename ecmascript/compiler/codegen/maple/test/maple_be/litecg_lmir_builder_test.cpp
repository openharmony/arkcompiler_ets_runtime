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

#include "lmir_builder.h"
#include "mir_builder.h"
#include "gtest/gtest.h"
#include <iostream>
using namespace std;
using namespace maple;
using namespace litecg;
namespace {
TEST(LiteCGGetTypeKind_FUNC, t01)
{
    MIRModule mirmodule("mirmodule");
    LMIRBuilder *lmirBuilder_ptr = new LMIRBuilder(mirmodule);
    MIRType mir_type(kTypeScalar, PTY_begin);

    MIRTypeKind input_ls[] = {kTypeScalar, kTypeArray, kTypePointer,
                              kTypeFunction, kTypeVoid, kTypeByName, static_cast<MIRTypeKind>(900)};
    LiteCGTypeKind output_ls[] = {kLiteCGTypeScalar, kLiteCGTypeArray,
                                  kLiteCGTypePointer, kLiteCGTypeFunction,
                                  kLiteCGTypeVoid, kLiteCGTypeByName, kLiteCGTypeUnknown};
    for (int i = 0; i < sizeof(input_ls) / sizeof(input_ls[0]); i++)
    {
        mir_type.SetMIRTypeKind(input_ls[i]);
        EXPECT_EQ(lmirBuilder_ptr->LiteCGGetTypeKind(&mir_type), output_ls[i]);
    }
}
}  // namespace
