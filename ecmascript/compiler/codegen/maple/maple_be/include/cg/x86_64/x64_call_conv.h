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

#ifndef MAPLEBE_INCLUDE_CG_X64_X64_CALL_CONV_H
#define MAPLEBE_INCLUDE_CG_X64_X64_CALL_CONV_H

#include "types_def.h"
#include "becommon.h"
#include "call_conv.h"
#include "abi.h"
#include "x64_abi.h"
#include "x64_isa.h"
#include <vector>

namespace maplebe {
using namespace maple;
using namespace x64;
constexpr const uint32 kMaxStructParamByReg = 4;

class CallConventionInfo {
public:
    virtual const std::vector<X64reg> &GetIntParamRegs() const = 0;
    virtual size_t GetIntParamRegsNum() const = 0;
    virtual const std::vector<X64reg> &GetIntReturnRegs() const = 0;
    virtual size_t GetIntReturnRegsNum() const = 0;
    virtual const std::vector<X64reg> &GetFloatParamRegs() const = 0;
    virtual size_t GetFloatParamRegsNum() const = 0;
    virtual const std::vector<X64reg> &GetFloatReturnRegs() const = 0;
    virtual size_t GetFloatReturnRegsNum() const = 0;
    virtual int32 Classification(const BECommon &be, MIRType &mirType, std::vector<ArgumentClass> &classes) const = 0;
};

#define CALL_CONVENTION_INFO_SUBCLASS_DECLARE_BEGIN(CLASSNAME)                    \
    class CLASSNAME : public CallConventionInfo {                                 \
    public:                                                                       \
        const std::vector<X64reg> &GetIntParamRegs() const override               \
        {                                                                         \
            return intParmRegs;                                                   \
        }                                                                         \
        size_t GetIntParamRegsNum() const override                                \
        {                                                                         \
            return intParmRegs.size();                                            \
        }                                                                         \
        const std::vector<X64reg> &GetIntReturnRegs() const override              \
        {                                                                         \
            return intReturnRegs;                                                 \
        }                                                                         \
        size_t GetIntReturnRegsNum() const override                               \
        {                                                                         \
            return intReturnRegs.size();                                          \
        }                                                                         \
        const std::vector<X64reg> &GetFloatParamRegs() const override             \
        {                                                                         \
            return floatParmRegs;                                                 \
        }                                                                         \
        size_t GetFloatParamRegsNum() const override                              \
        {                                                                         \
            return floatParmRegs.size();                                          \
        }                                                                         \
        const std::vector<X64reg> &GetFloatReturnRegs() const override            \
        {                                                                         \
            return floatReturnRegs;                                               \
        }                                                                         \
        size_t GetFloatReturnRegsNum() const override                             \
        {                                                                         \
            return floatReturnRegs.size();                                        \
        }                                                                         \
        const static CLASSNAME &GetCallConvInfo()                                 \
        {                                                                         \
            static CLASSNAME callConvInfo;                                        \
            return callConvInfo;                                                  \
        }                                                                         \
        int32 Classification(const BECommon &be, MIRType &mirType,                \
                             std::vector<ArgumentClass> &classes) const override; \
                                                                                  \
    private:                                                                      \
        CLASSNAME() {}                                                            \
        ~CLASSNAME() {}                                                           \
        CLASSNAME &operator=(const CLASSNAME &);                                  \
        CLASSNAME(const CLASSNAME &);

#define CALL_CONVENTION_INFO_SUBCLASS_DECLARE_END \
    }                                             \
    ;

CALL_CONVENTION_INFO_SUBCLASS_DECLARE_BEGIN(WebKitJSCallConventionInfo)

const std::vector<X64reg> intParmRegs {
    RAX};
const std::vector<X64reg> intReturnRegs {
    RAX};
const std::vector<X64reg> floatParmRegs{};
const std::vector<X64reg> floatReturnRegs{};
CALL_CONVENTION_INFO_SUBCLASS_DECLARE_END

CALL_CONVENTION_INFO_SUBCLASS_DECLARE_BEGIN(CCallConventionInfo)
const std::vector<X64reg> intParmRegs {
    RDI, RSI, RDX, RCX, X64reg::R8, X64reg::R9};
const std::vector<X64reg> intReturnRegs {
    RAX, RDX};
const std::vector<X64reg> floatParmRegs = {
    X64reg::V0, X64reg::V1, X64reg::V2, X64reg::V3, X64reg::V4, X64reg::V5, X64reg::V6, X64reg::V7};
const std::vector<X64reg> floatReturnRegs = {
    X64reg::V0, X64reg::V1 };
CALL_CONVENTION_INFO_SUBCLASS_DECLARE_END

CALL_CONVENTION_INFO_SUBCLASS_DECLARE_BEGIN(GHCCallConventionInfo)
const std::vector<X64reg> intParmRegs {
    X64reg::R13, RBP, X64reg::R12, RBX, X64reg::R14, RSI, RDI, X64reg::R8, X64reg::R9, X64reg::R15};
const std::vector<X64reg> intReturnRegs{};
const std::vector<X64reg> floatParmRegs{};
const std::vector<X64reg> floatReturnRegs{};
CALL_CONVENTION_INFO_SUBCLASS_DECLARE_END

class X64CallConvImpl {
public:
    X64CallConvImpl(BECommon &be) : beCommon(be)
    {
        convKind = GetCallConvKind(*(be.GetMIRModule().CurFunction()));
    }
    X64CallConvImpl(BECommon &be, CallConvKind convKind) : beCommon(be), convKind(convKind) {}

    ~X64CallConvImpl() = default;

    const CallConventionInfo &GetCallConvInfo() const
    {
        return GetCallConvInfo(convKind);
    }

    static const CallConventionInfo &GetCallConvInfo(CallConvKind convKind_)
    {
        switch (convKind_) {
            case kCCall:
                return CCallConventionInfo::GetCallConvInfo();
            case kWebKitJS:
                return WebKitJSCallConventionInfo::GetCallConvInfo();
            case kGHC:
                return GHCCallConventionInfo::GetCallConvInfo();
            default:
                return CCallConventionInfo::GetCallConvInfo();
        }
    }

    static CallConvKind GetCallConvKind(MIRFunction &mirFunction)
    {
        if (mirFunction.GetAttr(FUNCATTR_ccall)) {
            return kCCall;
        } else if (mirFunction.GetAttr(FUNCATTR_webkitjscall)) {
            return kWebKitJS;
        } else if (mirFunction.GetAttr(FUNCATTR_ghcall)) {
            return kGHC;
        } else {
            return kCCall;
        }
    }

    static CallConvKind GetCallConvKind(StmtNode &node)
    {
        if (node.GetAttr(STMTATTR_ccall)) {
            return kCCall;
        } else if (node.GetAttr(STMTATTR_webkitjscall)) {
            return kWebKitJS;
        } else {
            return kCCall;
        }
    }

    void InitCCLocInfo(CCLocInfo &pLoc) const;

    /* Passing  value related */
    int32 LocateNextParm(MIRType &mirType, CCLocInfo &pLoc, bool isFirst = false, MIRFunction *func = nullptr);

    /*  return value related  */
    int32 LocateRetVal(MIRType &retType, CCLocInfo &ploc);

private:
    X64reg AllocateGPParmRegister()
    {
        const std::vector<X64reg> &intParamRegs = GetCallConvInfo().GetIntParamRegs();
        return (nextGeneralParmRegNO < intParamRegs.size()) ? intParamRegs[nextGeneralParmRegNO++] : X64reg::kRinvalid;
    }

    X64reg AllocateSIMDFPRegister()
    {
        return (nextFloatRegNO < kNumFloatParmRegs) ? kFloatParmRegs[nextFloatRegNO++] : X64reg::kRinvalid;
    }

    X64reg AllocateGPReturnRegister()
    {
        const std::vector<X64reg> &intReturnRegs = GetCallConvInfo().GetIntReturnRegs();
        return (nextGeneralReturnRegNO < intReturnRegs.size()) ?
            intReturnRegs[nextGeneralReturnRegNO++] : X64reg::kRinvalid;
    }

    void AllocateTwoGPReturnRegisters(CCLocInfo &pLoc)
    {
        const std::vector<X64reg> &intReturnRegs = GetCallConvInfo().GetIntReturnRegs();
        if ((nextGeneralReturnRegNO + 1) < intReturnRegs.size()) {
            pLoc.reg0 = intReturnRegs[nextGeneralReturnRegNO++];
            pLoc.reg1 = intReturnRegs[nextGeneralReturnRegNO++];
        } else {
            pLoc.reg0 = X64reg::kRinvalid;
        }
    }

    X64reg AllocateSIMDFPReturnRegister()
    {
        return (nextFloatRetRegNO < kNumFloatReturnRegs) ? kFloatReturnRegs[nextFloatRetRegNO++] : X64reg::kRinvalid;
    }

    BECommon &beCommon;
    CallConvKind convKind = kCCall;
    uint64 paramNum = 0;               /* number of all types of parameters processed so far */
    uint32 nextGeneralParmRegNO = 0;   /* number of integer parameters processed so far */
    uint32 nextGeneralReturnRegNO = 0; /* number of integer return processed so far */
    uint32 nextStackArgAdress = 0;
    uint32 nextFloatRegNO = 0;
    uint32 nextFloatRetRegNO = 0;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_X64_X64_CALL_CONV_H */
