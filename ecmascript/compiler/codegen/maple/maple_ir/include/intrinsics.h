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

#ifndef MAPLE_IR_INCLUDE_INTRINSICS_H
#define MAPLE_IR_INCLUDE_INTRINSICS_H
#include "prim_types.h"
#include "intrinsic_op.h"

namespace maple {
enum IntrinProperty {
    kIntrnUndef,
    kIntrnIsJs,
    kIntrnIsJsUnary,
    kIntrnIsJsBinary,
    kIntrnIsReturnStruct,
    kIntrnNoSideEffect,
    kIntrnIsLoadMem,
    kIntrnIsPure,
    kIntrnNeverReturn,
    kIntrnIsAtomic,
    kIntrnIsRC,
    kIntrnIsSpecial,
};

enum IntrinArgType {
    kArgTyUndef,
    kArgTyVoid,
    kArgTyI8,
    kArgTyI16,
    kArgTyI32,
    kArgTyI64,
    kArgTyU8,
    kArgTyU16,
    kArgTyU32,
    kArgTyU64,
    kArgTyU1,
    kArgTyPtr,
    kArgTyRef,
    kArgTyA32,
    kArgTyA64,
    kArgTyF32,
    kArgTyF64,
    kArgTyF128,
    kArgTyAgg,
#ifdef DYNAMICLANG
    kArgTyDynany,
    kArgTyDynu32,
    kArgTyDyni32,
    kArgTyDynundef,
    kArgTyDynnull,
    kArgTyDynhole,
    kArgTyDynbool,
    kArgTyDynf64,
    kArgTyDynf32,
    kArgTySimplestr,
    kArgTyDynstr,
    kArgTySimpleobj,
    kArgTyDynobj
#endif
};

constexpr uint32 INTRNISJS = 1U << kIntrnIsJs;
constexpr uint32 INTRNISJSUNARY = 1U << kIntrnIsJsUnary;
constexpr uint32 INTRNISJSBINARY = 1U << kIntrnIsJsBinary;
constexpr uint32 INTRNNOSIDEEFFECT = 1U << kIntrnNoSideEffect;
constexpr uint32 INTRNRETURNSTRUCT = 1U << kIntrnIsReturnStruct;
constexpr uint32 INTRNLOADMEM = 1U << kIntrnIsLoadMem;
constexpr uint32 INTRNISPURE = 1U << kIntrnIsPure;
constexpr uint32 INTRNNEVERRETURN = 1U << kIntrnNeverReturn;
constexpr uint32 INTRNATOMIC = 1U << kIntrnIsAtomic;
constexpr uint32 INTRNISRC = 1U << kIntrnIsRC;
constexpr uint32 INTRNISSPECIAL = 1U << kIntrnIsSpecial;
class MIRType;    // circular dependency exists, no other choice
class MIRModule;  // circular dependency exists, no other choice
struct IntrinDesc {
    static constexpr int kMaxArgsNum = 7;
    const char *name;
    uint32 properties;
    IntrinArgType argTypes[1 + kMaxArgsNum];  // argTypes[0] is the return type
    bool IsJS() const
    {
        return static_cast<bool>(properties & INTRNISJS);
    }

    bool IsJsUnary() const
    {
        return static_cast<bool>(properties & INTRNISJSUNARY);
    }

    bool IsJsBinary() const
    {
        return static_cast<bool>(properties & INTRNISJSBINARY);
    }

    bool IsJsOp() const
    {
        return static_cast<bool>(properties & INTRNISJSUNARY) || static_cast<bool>(properties & INTRNISJSBINARY);
    }

    bool IsLoadMem() const
    {
        return static_cast<bool>(properties & INTRNLOADMEM);
    }

    bool IsJsReturnStruct() const
    {
        return static_cast<bool>(properties & INTRNRETURNSTRUCT);
    }

    bool IsPure() const
    {
        return static_cast<bool>(properties & INTRNISPURE);
    }

    bool IsNeverReturn() const
    {
        return static_cast<bool>(properties & INTRNNEVERRETURN);
    }

    bool IsAtomic() const
    {
        return static_cast<bool>(properties & INTRNATOMIC);
    }

    bool IsRC() const
    {
        return static_cast<bool>(properties & INTRNISRC);
    }

    bool IsSpecial() const
    {
        return static_cast<bool>(properties & INTRNISSPECIAL);
    }

    bool HasNoSideEffect() const
    {
        return properties & INTRNNOSIDEEFFECT;
    }

    MIRType *GetReturnType() const;
    MIRType *GetArgType(uint32 index) const;
    MIRType *GetTypeFromArgTy(IntrinArgType argType) const;
    static MIRType *jsValueType;
    static MIRModule *mirModule;
    static void InitMIRModule(MIRModule *mirModule);
    static IntrinDesc intrinTable[INTRN_LAST + 1];
};
}  // namespace maple
#endif  // MAPLE_IR_INCLUDE_INTRINSICS_H
