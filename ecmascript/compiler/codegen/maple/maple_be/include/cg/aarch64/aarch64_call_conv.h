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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CALL_CONV_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CALL_CONV_H

#include "types_def.h"
#include "becommon.h"
#include "call_conv.h"
#include "aarch64_abi.h"
#include "abi.h"

namespace maplebe {
using namespace maple;

/*
 * We use the names used in Procedure Call Standard for the Arm 64-bit
 * Architecture (AArch64) 2022Q3.  $6.8.2
 * nextGeneralRegNO (= _int_parm_num)  : Next General-purpose Register number
 * nextFloatRegNO (= _float_parm_num): Next SIMD and Floating-point Register Number
 * nextStackArgAdress (= _last_memOffset): Next Stacked Argument Address
 * for processing an incoming or outgoing parameter list
 */

class AArch64CallConvImpl : public CCImpl {
public:
    explicit AArch64CallConvImpl(BECommon &be) : CCImpl(), beCommon(be) {}

    ~AArch64CallConvImpl() = default;

    /* Return size of aggregate structure copy on stack. */
    uint64 LocateNextParm(const MIRType &mirType, CCLocInfo &pLoc, bool isFirst = false,
                          MIRFuncType *tFunc = nullptr) override;

    void LocateRetVal(const MIRType &retType, CCLocInfo &ploc) override;

    void SetupToReturnThroughMemory(CCLocInfo &pLoc) const
    {
        pLoc.regCount = 1;
        pLoc.reg0 = R8;
        pLoc.primTypeOfReg0 = GetExactPtrPrimType();
    }

    void Init() override
    {
        nextGeneralRegNO = 0;
        nextFloatRegNO = 0;
        nextStackArgAdress = 0;
    }

private:
    BECommon &beCommon;
    uint32 nextGeneralRegNO = 0; /* number of integer parameters processed so far */
    uint32 nextFloatRegNO = 0;   /* number of float parameters processed so far */

    AArch64reg AllocateGPRegister()
    {
        DEBUG_ASSERT(nextGeneralRegNO >= 0, "nextGeneralRegNO can not be neg");
        return (nextGeneralRegNO < AArch64Abi::kNumIntParmRegs) ? AArch64Abi::intParmRegs[nextGeneralRegNO++]
                                                                : kRinvalid;
    }

    void AllocateGPRegister(const MIRType &mirType, CCLocInfo &pLoc, uint64 size, uint64 align);

    AArch64reg AllocateSIMDFPRegister()
    {
        return (nextFloatRegNO < AArch64Abi::kNumFloatParmRegs) ? AArch64Abi::floatParmRegs[nextFloatRegNO++]
                                                                : kRinvalid;
    }
};

class AArch64WebKitJSCC : public CCImpl {
public:
    explicit AArch64WebKitJSCC(BECommon &be) : CCImpl(), beCommon(be) {}

    ~AArch64WebKitJSCC() = default;

    /* Return size of aggregate structure copy on stack. */
    uint64 LocateNextParm(const MIRType &mirType, CCLocInfo &pLoc, bool isFirst = false,
                          MIRFuncType *func = nullptr) override;

    void LocateRetVal(const MIRType &retType, CCLocInfo &ploc) override;

    /*  return value related  */
    void InitReturnInfo(MIRType &retTy, CCLocInfo &pLoc);

    void Init() override
    {
        nextGeneralRegNO = 0;
        nextFloatRegNO = 0;
        nextStackArgAdress = 0;
    }

private:
    BECommon &beCommon;
    int32 nextGeneralRegNO = 0; /* number of integer parameters processed so far */
    uint32 nextFloatRegNO = 0;  /* number of float parameters processed so far */
    static constexpr int32 kNumIntRetRegs = 8;
    static constexpr int32 kNumFloatRetRegs = 8;
    static constexpr int32 kNumIntParmRegs = 1;
    static constexpr int32 kNumFloatParmRegs = 0;
    static constexpr AArch64reg intReturnRegs[kNumIntRetRegs] = {R0, R1, R2, R3, R4, R5, R6, R7};
    static constexpr AArch64reg floatReturnRegs[kNumFloatRetRegs] = {V0, V1, V2, V3, V4, V5, V6, V7};
    static constexpr AArch64reg intParmRegs[kNumIntParmRegs] = {R0};
    static constexpr AArch64reg floatParmRegs[kNumFloatParmRegs] = {};

    int32 ClassificationArg(const BECommon &be, const MIRType &mirType, std::vector<ArgumentClass> &classes) const;

    int32 ClassificationRet(const BECommon &be, const MIRType &mirType, std::vector<ArgumentClass> &classes) const;

    AArch64reg AllocateGPParmRegister()
    {
        DEBUG_ASSERT(nextGeneralRegNO >= 0, "nextGeneralRegNO can not be neg");
        return (nextGeneralRegNO < AArch64WebKitJSCC::kNumIntParmRegs)
                   ? AArch64WebKitJSCC::intParmRegs[nextGeneralRegNO++]
                   : kRinvalid;
    }

    AArch64reg AllocateGPRetRegister()
    {
        DEBUG_ASSERT(nextGeneralRegNO >= 0, "nextGeneralRegNO can not be neg");
        return (nextGeneralRegNO < AArch64WebKitJSCC::kNumIntRetRegs)
                   ? AArch64WebKitJSCC::intReturnRegs[nextGeneralRegNO++]
                   : kRinvalid;
    }

    AArch64reg AllocateSIMDFPRetRegister()
    {
        return (nextFloatRegNO < AArch64WebKitJSCC::kNumFloatRetRegs)
                   ? AArch64WebKitJSCC::floatReturnRegs[nextFloatRegNO++]
                   : kRinvalid;
    }
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_CALL_CONV_H */
