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
#include "aarch64_call_conv.h"

namespace maplebe {
using namespace maple;

// instantiated with the type of the function return value, it describes how
// the return value is to be passed back to the caller
// Refer to Procedure Call Standard for the Arm 64-bit
// Architecture (AArch64) 2022Q3.  $6.9
//  "If the type, T, of the result of a function is such that
//     void func(T arg)
//   would require that arg be passed as a value in a register (or set of registers)
//   according to the rules in Parameter passing, then the result is returned in the
//   same registers as would be used for such an argument."
void AArch64CallConvImpl::LocateRetVal(const MIRType &retType, CCLocInfo &pLoc)
{
    InitCCLocInfo(pLoc);
    size_t retSize = retType.GetSize();
    if (retSize == 0) {
        return;  // size 0 ret val
    }

    PrimType primType = retType.GetPrimType();
    if (IsPrimitiveFloat(primType)) {
        // float or vector, return in v0
        pLoc.reg0 = AArch64Abi::floatReturnRegs[0];
        pLoc.primTypeOfReg0 = primType;
        pLoc.regCount = 1;
        return;
    }
    if (IsPrimitiveInteger(primType) && GetPrimTypeBitSize(primType) <= k64BitSize) {
        // interger and size <= 64-bit, return in x0
        pLoc.reg0 = AArch64Abi::intReturnRegs[0];
        pLoc.primTypeOfReg0 = primType;
        pLoc.regCount = 1;
        return;
    }
    CHECK_FATAL(false, "NIY");
}

// allocate general purpose register
void AArch64CallConvImpl::AllocateGPRegister(const MIRType &mirType, CCLocInfo &pLoc, uint64 size, uint64 align)
{
    if (IsPrimitiveInteger(mirType.GetPrimType()) && size <= k8ByteSize) {
        // C.9  If the argument is an Integral or Pointer Type, the size of the argument is less
        //      than or equal to 8 bytes and the NGRN is less than 8, the argument is copied to
        //      the least significant bits in x[NGRN]. The NGRN is incremented by one.
        //      The argument has now been allocated.
        pLoc.reg0 = AllocateGPRegister();
        pLoc.primTypeOfReg0 = mirType.GetPrimType();
        return;
    }
    CHECK_FATAL(false, "NIY");
}

static void SetupCCLocInfoRegCount(CCLocInfo &pLoc)
{
    if (pLoc.reg0 == kRinvalid) {
        return;
    }
    pLoc.regCount = kOneRegister;
    if (pLoc.reg1 == kRinvalid) {
        return;
    }
    pLoc.regCount++;
    if (pLoc.reg2 == kRinvalid) {
        return;
    }
    pLoc.regCount++;
    if (pLoc.reg3 == kRinvalid) {
        return;
    }
    pLoc.regCount++;
}

/*
 * Refer to ARM IHI 0055C_beta: Procedure Call Standard for
 * the ARM 64-bit Architecture. $5.4.2
 *
 * For internal only functions, we may want to implement
 * our own rules as Apple IOS has done. Maybe we want to
 * generate two versions for each of externally visible functions,
 * one conforming to the ARM standard ABI, and the other for
 * internal only use.
 *
 * LocateNextParm should be called with each parameter in the parameter list
 * starting from the beginning, one call per parameter in sequence; it returns
 * the information on how each parameter is passed in pLoc
 *
 * *** CAUTION OF USE: ***
 * If LocateNextParm is called for function formals, third argument isFirst is true.
 * LocateNextParm is then checked against a function parameter list.  All other calls
 * of LocateNextParm are against caller's argument list must not have isFirst set,
 * or it will be checking the caller's enclosing function.
 */

uint64 AArch64CallConvImpl::LocateNextParm(const MIRType &mirType, CCLocInfo &pLoc, bool isFirst, MIRFuncType *tFunc)
{
    InitCCLocInfo(pLoc);
    uint64 typeSize = mirType.GetSize();
    if (typeSize == 0) {
        return 0;
    }

    if (isFirst) {
        DEBUG_ASSERT(beCommon.GetMIRModule().CurFunction() != nullptr, "curFunction should not be nullptr");
        auto *func = (tFunc != nullptr) ? tFunc : beCommon.GetMIRModule().CurFunction()->GetMIRFuncType();
        if (func->FirstArgReturn()) {
            // For return struct in memory, the pointer returns in x8.
            SetupToReturnThroughMemory(pLoc);
            return GetPointerSize();
        }
    }

    uint64 typeAlign = mirType.GetAlign();

    pLoc.memSize = static_cast<int32>(typeSize);

    uint64 aggCopySize = 0;
    if (IsPrimitiveFloat(mirType.GetPrimType())) {
        // float or vector, passed by float or SIMD register
        pLoc.reg0 = AllocateSIMDFPRegister();
        pLoc.primTypeOfReg0 = mirType.GetPrimType();
    } else if (IsPrimitiveInteger(mirType.GetPrimType())) {
        // integer, passed by general purpose register
        AllocateGPRegister(mirType, pLoc, typeSize, typeAlign);
    } else {
        CHECK_FATAL(false, "NIY");
    }

    SetupCCLocInfoRegCount(pLoc);
    if (pLoc.reg0 == kRinvalid) {
        // being passed in memory
        typeAlign = (typeAlign <= k8ByteSize) ? k8ByteSize : typeAlign;
        nextStackArgAdress = static_cast<int32>(RoundUp(nextStackArgAdress, typeAlign));
        pLoc.memOffset = static_cast<int32>(nextStackArgAdress);
        // large struct, passed with pointer
        nextStackArgAdress += static_cast<int32>(aggCopySize != 0 ? k8ByteSize : typeSize);
    }
    return aggCopySize;
}

uint64 AArch64WebKitJSCC::LocateNextParm(const MIRType &mirType, CCLocInfo &pLoc, bool isFirst, MIRFuncType *tFunc)
{
    std::vector<ArgumentClass> classes {};
    int32 alignedTySize = ClassificationArg(beCommon, mirType, classes);
    pLoc.memSize = alignedTySize;
    if (classes[0] == kIntegerClass) {
        if (alignedTySize == k4ByteSize || alignedTySize == k8ByteSize) {
            pLoc.reg0 = AllocateGPParmRegister();
        } else {
            CHECK_FATAL(false, "no should not go here");
        }
    } else if (classes[0] == kFloatClass) {
        CHECK_FATAL(false, "float should passed on stack!");
    }
    if (pLoc.reg0 == kRinvalid || classes[0] == kMemoryClass) {
        /* being passed in memory */
        pLoc.memOffset = nextStackArgAdress;
        nextStackArgAdress = pLoc.memOffset + alignedTySize;
    }
    return 0;
}

void AArch64WebKitJSCC::LocateRetVal(const MIRType &retType, CCLocInfo &pLoc)
{
    InitCCLocInfo(pLoc);
    std::vector<ArgumentClass> classes {}; /* Max of four Regs. */
    int32 alignedTySize = ClassificationRet(beCommon, retType, classes);
    if (alignedTySize == 0) {
        return; /* size 0 ret val */
    }
    if (classes[0] == kIntegerClass) {
        if ((alignedTySize == k4ByteSize) || (alignedTySize == k8ByteSize)) {
            pLoc.reg0 = AllocateGPRetRegister();
            pLoc.regCount += 1;
            pLoc.primTypeOfReg0 = alignedTySize == k4ByteSize ? PTY_i32 : PTY_i64;
        } else {
            CHECK_FATAL(false, "should not go here");
        }
    } else if (classes[0] == kFloatClass) {
        if ((alignedTySize == k4ByteSize) || (alignedTySize == k8ByteSize)) {
            pLoc.reg0 = AllocateSIMDFPRetRegister();
            pLoc.regCount += 1;
            pLoc.primTypeOfReg0 = alignedTySize == k4ByteSize ? PTY_f32 : PTY_f64;
        } else {
            CHECK_FATAL(false, "should not go here");
        }
    }
    if (pLoc.reg0 == kRinvalid || classes[0] == kMemoryClass) {
        CHECK_FATAL(false, "should not happen");
    }
    return;
}

int32 AArch64WebKitJSCC::ClassificationRet(const BECommon &be, const MIRType &mirType,
                                           std::vector<ArgumentClass> &classes) const
{
    switch (mirType.GetPrimType()) {
        /*
         * Arguments of types void, (signed and unsigned) _Bool, char, short, int,
         * long, long long, and pointers are in the INTEGER class.
         */
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
}

int32 AArch64WebKitJSCC::ClassificationArg(const BECommon &be, const MIRType &mirType,
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
            classes.push_back(kMemoryClass);
            return k4ByteSize;
        case PTY_f64:
            classes.push_back(kMemoryClass);
            return k8ByteSize;
        default:
            CHECK_FATAL(false, "NYI");
    }
    return 0;
}

void AArch64WebKitJSCC::InitReturnInfo(MIRType &retTy, CCLocInfo &pLoc)
{
    // don't see why this function exisits?
    LocateRetVal(retTy, pLoc);
}
/*
 * From "ARM Procedure Call Standard for ARM 64-bit Architecture"
 *     ARM IHI 0055C_beta, 6th November 2013
 * $ 5.1 machine Registers
 * $ 5.1.1 General-Purpose Registers
 *  <Table 2>                Note
 *  SP       Stack Pointer
 *  R30/LR   Link register   Stores the return address.
 *                           We push it into stack along with FP on function
 *                           entry using STP and restore it on function exit
 *                           using LDP even if the function is a leaf (i.e.,
 *                           it does not call any other function) because it
 *                           is free (we have to store FP anyway).  So, if a
 *                           function is a leaf, we may use it as a temporary
 *                           register.
 *  R29/FP   Frame Pointer
 *  R19-R28  Callee-saved
 *           registers
 *  R18      Platform reg    Can we use it as a temporary register?
 *  R16,R17  IP0,IP1         Maybe used as temporary registers. Should be
 *                           given lower priorities. (i.e., we push them
 *                           into the free register stack before the others)
 *  R9-R15                   Temporary registers, caller-saved
 *  Note:
 *  R16 and R17 may be used by a linker as a scratch register between
 *  a routine and any subroutine it calls. They can also be used within a
 *  routine to hold intermediate values between subroutine calls.
 *
 *  The role of R18 is platform specific. If a platform ABI has need of
 *  a dedicated general purpose register to carry inter-procedural state
 *  (for example, the thread context) then it should use this register for
 *  that purpose. If the platform ABI has no such requirements, then it should
 *  use R18 as an additional temporary register. The platform ABI specification
 *  must document the usage for this register.
 *
 *  A subroutine invocation must preserve the contents of the registers R19-R29
 *  and SP. All 64 bits of each value stored in R19-R29 must be preserved, even
 *  when using the ILP32 data model.
 *
 *  $ 5.1.2 SIMD and Floating-Point Registers
 *
 *  The first eight registers, V0-V7, are used to pass argument values into
 *  a subroutine and to return result values from a function. They may also
 *  be used to hold intermediate values within a routine.
 *
 *  V8-V15 must be preserved by a callee across subroutine calls; the
 *  remaining registers do not need to be preserved( or caller-saved).
 *  Additionally, only the bottom 64 bits of each value stored in V8-
 *  V15 need to be preserved.
 */
} /* namespace maplebe */
