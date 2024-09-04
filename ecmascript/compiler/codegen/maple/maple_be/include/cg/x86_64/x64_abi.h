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

#ifndef MAPLEBE_INCLUDE_CG_X64_X64_ABI_H
#define MAPLEBE_INCLUDE_CG_X64_X64_ABI_H

#include "x64_isa.h"
#include "types_def.h"
#include "becommon.h"

namespace maplebe {
using namespace maple;

namespace x64 {

constexpr int32 kNumFloatParmRegs = 8;
constexpr int32 kNumFloatReturnRegs = 2;

constexpr X64reg kFloatParmRegs[kNumFloatParmRegs] = { V0, V1, V2, V3, V4, V5, V6, V7 };
constexpr X64reg kFloatReturnRegs[kNumFloatReturnRegs] = { V0, V1 };

/*
 * Refer to：
 * x64-bit Architecture.
 */
bool IsAvailableReg(X64reg reg);
bool IsCalleeSavedReg(X64reg reg);
bool IsParamReg(X64reg reg);
bool IsSpillReg(X64reg reg);
bool IsExtraSpillReg(X64reg reg);
bool IsSpillRegInRA(X64reg regNO, bool has3RegOpnd);
} /* namespace x64 */

/*
 * X64-bit Architecture.
 * After the argument values have been computed, they are placed either in registers
 * or pushed on the stack. The way how values are passed is described in the
 * following sections.
 *   - INTEGER This class consists of integral types that fit into one of the general
       purpose registers.
     - SSE The class consists of types that fit into a vector register.
     - SSEUP The class consists of types that fit into a vector register and can be passed
       and returned in the upper bytes of it.
     - X87, X87UP These classes consists of types that will be returned via the x87 FPU.
     - COMPLEX_X87 This class consists of types that will be returned via the x87 FPU.
     - NO_CLASS This class is used as initializer in the algorithms. It will be used for
       padding and empty structures and unions.
     - MEMORY This class consists of types that will be passed and returned in memory via the stack.
 *
 */
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_X64_X64_ABI_H */
