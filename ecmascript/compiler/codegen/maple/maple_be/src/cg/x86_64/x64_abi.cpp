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

#include "x64_cgfunc.h"
#include "becommon.h"
#include "x64_isa.h"

namespace maplebe {
using namespace maple;
namespace x64 {
bool IsAvailableReg(X64reg reg)
{
    switch (reg) {
/* integer registers */
#define INT_REG(ID, PREF8, PREF8_16, PREF16, PREF32, PREF64, canBeAssigned, isCalleeSave, isParam, isSpill, \
                isExtraSpill)                                                                               \
    case R##ID:                                                                                             \
        return canBeAssigned;
#define INT_REG_ALIAS(ALIAS, ID)
#include "x64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) \
    case V##ID:                                                                                               \
        return canBeAssigned;
#include "x64_fp_simd_regs.def"
#undef FP_SIMD_REG
        default:
            return false;
    }
}

bool IsCalleeSavedReg(X64reg reg)
{
    switch (reg) {
/* integer registers */
#define INT_REG(ID, PREF8, PREF8_16, PREF16, PREF32, PREF64, canBeAssigned, isCalleeSave, isParam, isSpill, \
                isExtraSpill)                                                                               \
    case R##ID:                                                                                             \
        return isCalleeSave;
#define INT_REG_ALIAS(ALIAS, ID)
#include "x64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) \
    case V##ID:                                                                                               \
        return isCalleeSave;
#include "x64_fp_simd_regs.def"
#undef FP_SIMD_REG
        default:
            return false;
    }
}

bool IsSpillReg(X64reg reg)
{
    switch (reg) {
/* integer registers */
#define INT_REG(ID, PREF8, PREF8_16, PREF16, PREF32, PREF64, canBeAssigned, isCalleeSave, isParam, isSpill, \
                isExtraSpill)                                                                               \
    case R##ID:                                                                                             \
        return isSpill;
#define INT_REG_ALIAS(ALIAS, ID)
#include "x64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) \
    case V##ID:                                                                                               \
        return isSpill;
#include "x64_fp_simd_regs.def"
#undef FP_SIMD_REG
        default:
            return false;
    }
}

bool IsExtraSpillReg(X64reg reg)
{
    switch (reg) {
/* integer registers */
#define INT_REG(ID, PREF8, PREF8_16, PREF16, PREF32, PREF64, canBeAssigned, isCalleeSave, isParam, isSpill, \
                isExtraSpill)                                                                               \
    case R##ID:                                                                                             \
        return isExtraSpill;
#define INT_REG_ALIAS(ALIAS, ID)
#include "x64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) \
    case V##ID:                                                                                               \
        return isExtraSpill;
#include "x64_fp_simd_regs.def"
#undef FP_SIMD_REG
        default:
            return false;
    }
}

bool IsSpillRegInRA(X64reg regNO, bool has3RegOpnd)
{
    /* if has 3 RegOpnd, previous reg used to spill. */
    if (has3RegOpnd) {
        return IsSpillReg(regNO) || IsExtraSpillReg(regNO);
    }
    return IsSpillReg(regNO);
}
} /* namespace x64 */
} /* namespace maplebe */
