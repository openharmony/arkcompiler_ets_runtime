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

namespace maplebe {
using namespace maple;

namespace AArch64Abi {
bool IsAvailableReg(AArch64reg reg)
{
    switch (reg) {
/* integer registers */
#define INT_REG(ID, PREF32, PREF64, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) \
    case R##ID: {                                                                                \
        return canBeAssigned;                                                                    \
    }
#define INT_REG_ALIAS(ALIAS, ID, PREF32, PREF64)
#include "aarch64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, PV, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) \
    case V##ID: {                                                                                                 \
        return canBeAssigned;                                                                                     \
    }
#define FP_SIMD_REG_ALIAS(ID)
#include "aarch64_fp_simd_regs.def"
#undef FP_SIMD_REG
#undef FP_SIMD_REG_ALIAS
        default:
            return false;
    }
}

bool IsCallerSaveReg(AArch64reg regNO)
{
    return (R0 <= regNO && regNO <= R18) || (V0 <= regNO && regNO <= V7) || (V16 <= regNO && regNO <= V31) ||
           (regNO == kRFLAG);
}

bool IsCalleeSavedReg(AArch64reg reg)
{
    switch (reg) {
/* integer registers */
#define INT_REG(ID, PREF32, PREF64, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) \
    case R##ID: {                                                                                \
        return isCalleeSave;                                                                     \
    }
#define INT_REG_ALIAS(ALIAS, ID, PREF32, PREF64)
#include "aarch64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, PV, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) \
    case V##ID: {                                                                                                 \
        return isCalleeSave;                                                                                      \
    }
#define FP_SIMD_REG_ALIAS(ID)
#include "aarch64_fp_simd_regs.def"
#undef FP_SIMD_REG
#undef FP_SIMD_REG_ALIAS
        default:
            return false;
    }
}

bool IsSpillReg(AArch64reg reg)
{
    switch (reg) {
/* integer registers */
#define INT_REG(ID, PREF32, PREF64, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) \
    case R##ID: {                                                                                \
        return isSpill;                                                                          \
    }
#define INT_REG_ALIAS(ALIAS, ID, PREF32, PREF64)
#include "aarch64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, PV, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) \
    case V##ID: {                                                                                                 \
        return isSpill;                                                                                           \
    }
#define FP_SIMD_REG_ALIAS(ID)
#include "aarch64_fp_simd_regs.def"
#undef FP_SIMD_REG
#undef FP_SIMD_REG_ALIAS
        default:
            return false;
    }
}

bool IsExtraSpillReg(AArch64reg reg)
{
    switch (reg) {
/* integer registers */
#define INT_REG(ID, PREF32, PREF64, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) \
    case R##ID: {                                                                                \
        return isExtraSpill;                                                                     \
    }
#define INT_REG_ALIAS(ALIAS, ID, PREF32, PREF64)
#include "aarch64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, PV, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) \
    case V##ID: {                                                                                                 \
        return isExtraSpill;                                                                                      \
    }
#define FP_SIMD_REG_ALIAS(ID)
#include "aarch64_fp_simd_regs.def"
#undef FP_SIMD_REG
#undef FP_SIMD_REG_ALIAS
        default:
            return false;
    }
}

bool IsSpillRegInRA(AArch64reg regNO, bool has3RegOpnd)
{
    /* if has 3 RegOpnd, previous reg used to spill. */
    if (has3RegOpnd) {
        return AArch64Abi::IsSpillReg(regNO) || AArch64Abi::IsExtraSpillReg(regNO);
    }
    return AArch64Abi::IsSpillReg(regNO);
}

PrimType IsVectorArrayType(MIRType *ty, uint32 &arraySize)
{
    if (ty->GetKind() == kTypeStruct) {
        MIRStructType *structTy = static_cast<MIRStructType *>(ty);
        if (structTy->GetFields().size() == 1) {
            auto fieldPair = structTy->GetFields()[0];
            MIRType *fieldTy = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldPair.second.first);
            if (fieldTy->GetKind() == kTypeArray) {
                MIRArrayType *arrayTy = static_cast<MIRArrayType *>(fieldTy);
                MIRType *arrayElemTy = arrayTy->GetElemType();
                arraySize = arrayTy->GetSizeArrayItem(0);
                if (arrayTy->GetDim() == k1BitSize && arraySize <= static_cast<int32>(k4BitSize) &&
                    IsPrimitiveVector(arrayElemTy->GetPrimType())) {
                    return arrayElemTy->GetPrimType();
                }
            }
        }
    }
    return PTY_void;
}
} /* namespace AArch64Abi */
} /* namespace maplebe */
