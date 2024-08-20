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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ABI_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ABI_H

#include "aarch64_isa.h"
#include "types_def.h"
#include "becommon.h"

namespace maplebe {
using namespace maple;

namespace AArch64Abi {
constexpr int32 kNumIntParmRegs = 8;
constexpr int32 kNumFloatParmRegs = 8;
constexpr int32 kYieldPointReservedReg = 19;
constexpr uint32 kNormalUseOperandNum = 3;
constexpr uint32 kMaxInstrForTbnz = 8000;      // approximately less than (2^13);
constexpr uint32 kMaxInstrForCondBr = 260000;  // approximately less than (2^18);
constexpr uint32 kMaxInstrForLdr = 260000;     // approximately less than (2^18);

extern std::vector<AArch64reg> intReturnRegs;
extern std::vector<AArch64reg> floatReturnRegs;
extern std::vector<AArch64reg> intParmRegs;
extern std::vector<AArch64reg> floatParmRegs;

/*
 * Refer to ARM IHI 0055C_beta: Procedure Call Standard  for
 * ARM 64-bit Architecture. Section 5.5
 */
bool IsAvailableReg(AArch64reg reg);
bool IsCalleeSavedReg(AArch64reg reg);
bool IsCallerSaveReg(AArch64reg reg);
bool IsSpillReg(AArch64reg reg);
bool IsExtraSpillReg(AArch64reg reg);
bool IsSpillRegInRA(AArch64reg regNO, bool has3RegOpnd);
} /* namespace AArch64Abi */

} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ABI_H */
