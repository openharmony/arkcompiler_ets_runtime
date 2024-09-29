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

namespace maple {
MIRType *IntrinDesc::jsValueType = nullptr;
MIRModule *IntrinDesc::mirModule = nullptr;
IntrinDesc IntrinDesc::intrinTable[INTRN_LAST + 1] = {
#define DEF_MIR_INTRINSIC(X, NAME, RETURN_TYPE, ...) \
    {(NAME), {(RETURN_TYPE), ##__VA_ARGS__}},
#include "intrinsics.def"
#undef DEF_MIR_INTRINSIC
};

void IntrinDesc::InitMIRModule(MIRModule *mod)
{
    mirModule = mod;
}

MIRType *IntrinDesc::GetTypeFromArgTy(IntrinArgType argType) const
{
    switch (argType) {
        case kArgTyVoid:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_void));
        case kArgTyI8:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_i8));
        case kArgTyI16:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_i16));
        case kArgTyI32:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_i32));
        case kArgTyI64:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_i64));
        case kArgTyU8:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_u8));
        case kArgTyU16:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_u16));
        case kArgTyU32:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_u32));
        case kArgTyU64:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_u64));
        case kArgTyU1:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_u1));
        case kArgTyPtr:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_ptr));
        case kArgTyRef:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_ref));
        case kArgTyA64:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_a64));
        case kArgTyF32:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_f32));
        case kArgTyF64:
            return GlobalTables::GetTypeTable().GetTypeFromTyIdx(static_cast<TyIdx>(PTY_f64));
        default:
            return nullptr;
    }
}

MIRType *IntrinDesc::GetArgType(uint32 index) const
{
    // 0 is the arg of return type
    CHECK_FATAL(index < kMaxArgsNum, "index out of range");
    return GetTypeFromArgTy(argTypes[index + 1]);
}

MIRType *IntrinDesc::GetReturnType() const
{
    return GetTypeFromArgTy(argTypes[0]);
}
}  // namespace maple
