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
#include "abi.h"
#include "x64_call_conv.h"
namespace maplebe {
using namespace maple;
using namespace x64;

int32 CCallConventionInfo::Classification(const BECommon &be, MIRType &mirType,
                                          std::vector<ArgumentClass> &classes) const
{
    switch (mirType.GetPrimType()) {
        /*
         * Arguments of types void, (signed and unsigned) _Bool, char, short, int,
         * long, long long, and pointers are in the INTEGER class.
         */
        case PTY_void:
        case PTY_u1:
        case PTY_u8:
        case PTY_i8:
        case PTY_u16:
        case PTY_i16:
        case PTY_u32:
        case PTY_i32:
        case PTY_a64:
        case PTY_ptr:
        case PTY_ref:
        case PTY_u64:
        case PTY_i64:
            classes.push_back(kIntegerClass);
            return k8ByteSize;
        case PTY_f32:
        case PTY_f64:
            classes.push_back(kFloatClass);
            return k8ByteSize;
        default:
            CHECK_FATAL(false, "NYI");
    }
    return 0;
}

int32 WebKitJSCallConventionInfo::Classification(const BECommon &be, MIRType &mirType,
                                                 std::vector<ArgumentClass> &classes) const
{
    switch (mirType.GetPrimType()) {
        /*
         * Arguments of types void, (signed and unsigned) _Bool, char, short, int,
         * long, long long, and pointers are in the INTEGER class.
         */
        case PTY_void:
        case PTY_u1:
        case PTY_u8:
        case PTY_i8:
        case PTY_u16:
        case PTY_i16:
        case PTY_u32:
        case PTY_i32:
            classes.push_back(kIntegerClass);
            return k4ByteSize;
        case PTY_a64:
        case PTY_ptr:
        case PTY_ref:
        case PTY_u64:
        case PTY_i64:
            classes.push_back(kIntegerClass);
            return k8ByteSize;
        case PTY_f32:
            classes.push_back(kFloatClass);
            return k4ByteSize;
        case PTY_f64:
            classes.push_back(kFloatClass);
            return k8ByteSize;
        default:
            CHECK_FATAL(false, "NYI");
    }
    return 0;
}

int32 GHCCallConventionInfo::Classification(const BECommon &be, MIRType &mirType,
                                            std::vector<ArgumentClass> &classes) const
{
    switch (mirType.GetPrimType()) {
        case PTY_u1:
        case PTY_u8:
        case PTY_i8:
        case PTY_u16:
        case PTY_i16:
        case PTY_u32:
        case PTY_i32:
        case PTY_a64:
        case PTY_ptr:
        case PTY_ref:
        case PTY_u64:
        case PTY_i64:
            classes.push_back(kIntegerClass);
            return k8ByteSize;
        default:
            CHECK_FATAL(false, "NYI");
    }
    return 0;
}

void X64CallConvImpl::InitCCLocInfo(CCLocInfo &pLoc) const
{
    pLoc.reg0 = kRinvalid;
    pLoc.reg1 = kRinvalid;
    pLoc.reg2 = kRinvalid;
    pLoc.reg3 = kRinvalid;
    pLoc.memOffset = nextStackArgAdress;
    pLoc.fpSize = 0;
    pLoc.numFpPureRegs = 0;
}

int32 X64CallConvImpl::LocateNextParm(MIRType &mirType, CCLocInfo &pLoc, bool isFirst, MIRFunction *tFunc)
{
    InitCCLocInfo(pLoc);
    std::vector<ArgumentClass> classes {};
    int32 alignedTySize = GetCallConvInfo().Classification(beCommon, mirType, classes);
    if (alignedTySize == 0) {
        return 0;
    }
    pLoc.memSize = alignedTySize;
    ++paramNum;
    if (classes[0] == kIntegerClass) {
        CHECK_FATAL((alignedTySize == k4ByteSize) || (alignedTySize == k8ByteSize), "NIY");
        pLoc.reg0 = AllocateGPParmRegister();
        DEBUG_ASSERT(nextGeneralParmRegNO <= GetCallConvInfo().GetIntParamRegsNum(), "RegNo should be pramRegNO");
    } else if (classes[0] == kFloatClass) {
        CHECK_FATAL(alignedTySize == k8ByteSize, "NIY");
        pLoc.reg0 = AllocateSIMDFPRegister();
        DEBUG_ASSERT(nextGeneralParmRegNO <= kNumFloatParmRegs, "RegNo should be pramRegNO");
    }
    if (pLoc.reg0 == kRinvalid || classes[0] == kMemoryClass) {
        /* being passed in memory */
        nextStackArgAdress = pLoc.memOffset + alignedTySize;
    }
    return 0;
}

int32 X64CallConvImpl::LocateRetVal(MIRType &retType, CCLocInfo &pLoc)
{
    InitCCLocInfo(pLoc);
    std::vector<ArgumentClass> classes {}; /* Max of four Regs. */
    uint32 alignedTySize = static_cast<uint32>(GetCallConvInfo().Classification(beCommon, retType, classes));
    if (alignedTySize == 0) {
        return 0; /* size 0 ret val */
    }
    if (classes[0] == kIntegerClass) {
        /* If the class is INTEGER, the next available register of the sequence %rax, */
        /* %rdx is used. */
        CHECK_FATAL((alignedTySize == k4ByteSize) || (alignedTySize == k8ByteSize),
                    "LocateRetVal: illegal number of regs");
        pLoc.regCount = kOneRegister;
        pLoc.reg0 = AllocateGPReturnRegister();
        DEBUG_ASSERT(nextGeneralReturnRegNO <= GetCallConvInfo().GetIntReturnRegsNum(), "RegNo should be pramRegNO");
        CHECK_FATAL(nextGeneralReturnRegNO == kOneRegister, "RegNo should be pramRegNO");
        pLoc.primTypeOfReg0 = retType.GetPrimType();
        return 0;
    } else if (classes[0] == kFloatClass) {
        /* If the class is SSE, the next available vector register of the sequence %xmm0, */
        /* %xmm1 is used. */
        CHECK_FATAL(alignedTySize == k8ByteSize, "LocateRetVal: illegal number of regs");
        pLoc.regCount = 1;
        pLoc.reg0 = AllocateSIMDFPReturnRegister();
        CHECK_FATAL(nextFloatRetRegNO == kOneRegister, "RegNo should be pramRegNO");
        pLoc.primTypeOfReg0 = retType.GetPrimType();
        return 0;
    }
    CHECK_FATAL(false, "NYI");
    return 0;
}
} /* namespace maplebe */
