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
#include "aarch64_call_conv.h"

namespace maplebe {
using namespace maple;

/* external interface to look for pure float struct */
uint32 AArch64CallConvImpl::FloatParamRegRequired(const MIRStructType &structType, uint32 &fpSize)
{
    PrimType baseType = PTY_begin;
    size_t elemNum = 0;
    if (!IsHomogeneousAggregates(structType, baseType, elemNum)) {
        return 0;
    }
    fpSize = GetPrimTypeSize(baseType);
    return static_cast<uint32>(elemNum);
}

static void AllocateHomogeneousAggregatesRegister(CCLocInfo &pLoc, std::vector<AArch64reg> &regList, uint32 maxRegNum,
                                                  PrimType baseType, uint32 allocNum, [[maybe_unused]] uint32 begin = 0)
{
    CHECK_FATAL(allocNum + begin - 1 < maxRegNum, "NIY, out of range.");
    if (allocNum >= kOneRegister) {
        pLoc.reg0 = regList[begin++];
        pLoc.primTypeOfReg0 = baseType;
    }
    if (allocNum >= kTwoRegister) {
        pLoc.reg1 = regList[begin++];
        pLoc.primTypeOfReg1 = baseType;
    }
    if (allocNum >= kThreeRegister) {
        pLoc.reg2 = regList[begin++];
        pLoc.primTypeOfReg2 = baseType;
    }
    if (allocNum >= kFourRegister) {
        pLoc.reg3 = regList[begin];
        pLoc.primTypeOfReg3 = baseType;
    }
    pLoc.regCount = static_cast<uint8>(allocNum);
}

// instantiated with the type of the function return value, it describes how
// the return value is to be passed back to the caller
//
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
    if (IsPrimitiveFloat(primType) || IsPrimitiveVector(primType)) {
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
    PrimType baseType = PTY_begin;
    size_t elemNum = 0;
    if (IsHomogeneousAggregates(retType, baseType, elemNum)) {
        // homogeneous aggregates, return in v0-v3
        AllocateHomogeneousAggregatesRegister(pLoc, AArch64Abi::floatReturnRegs, AArch64Abi::kNumFloatParmRegs,
                                              baseType, static_cast<uint32>(elemNum));
        return;
    }
    if (retSize <= k16ByteSize) {
        // agg size <= 16-byte or int128, return in x0-x1
        pLoc.reg0 = AArch64Abi::intReturnRegs[0];
        pLoc.primTypeOfReg0 = (retSize <= k4ByteSize && !CGOptions::IsBigEndian()) ? PTY_u32 : PTY_u64;
        if (retSize > k8ByteSize) {
            pLoc.reg1 = AArch64Abi::intReturnRegs[1];
            pLoc.primTypeOfReg1 = (retSize <= k12ByteSize && !CGOptions::IsBigEndian()) ? PTY_u32 : PTY_u64;
        }
        pLoc.regCount = retSize <= k8ByteSize ? kOneRegister : kTwoRegister;
        return;
    }
}

uint64 AArch64CallConvImpl::AllocateRegisterForAgg(const MIRType &mirType, CCLocInfo &pLoc, uint64 size, uint64 &align)
{
    uint64 aggCopySize = 0;
    PrimType baseType = PTY_begin;
    size_t elemNum = 0;
    if (IsHomogeneousAggregates(mirType, baseType, elemNum)) {
        align = GetPrimTypeSize(baseType);
        if ((nextFloatRegNO + elemNum - 1) < AArch64Abi::kNumFloatParmRegs) {
            // C.2  If the argument is an HFA or an HVA and there are sufficient unallocated SIMD and
            //      Floating-point registers (NSRN + number of members <= 8), then the argument is
            //      allocated to SIMD and Floating-point registers (with one register per member of
            //      the HFA or HVA). The NSRN is incremented by the number of registers used.
            //      The argument has now been allocated
            AllocateHomogeneousAggregatesRegister(pLoc, AArch64Abi::floatReturnRegs, AArch64Abi::kNumFloatParmRegs,
                                                  baseType, static_cast<uint32>(elemNum), nextFloatRegNO);
            nextFloatRegNO = static_cast<uint32>(nextFloatRegNO + elemNum);
        } else {
            // C.3  If the argument is an HFA or an HVA then the NSRN is set to 8 and the size of the
            //      argument is rounded up to the nearest multiple of 8 bytes.
            nextFloatRegNO = AArch64Abi::kNumFloatParmRegs;
            pLoc.reg0 = kRinvalid;
        }
    } else if (size <= k16ByteSize) {
        // small struct, passed by general purpose register
        // B.6 If the argument is an alignment adjusted type its value is passed as a copy of the actual
        //     value. The copy will have an alignment defined as follows:
        //     (1) For a Fundamental Data Type, the alignment is the natural alignment of that type,
        //         after any promotions.
        //     (2) For a Composite Type, the alignment of the copy will have 8-byte alignment if its
        //         natural alignment is <= 8 and 16-byte alignment if its natural alignment is >= 16.
        //     The alignment of the copy is used for applying marshaling rules.
        if (mirType.GetUnadjustedAlign() <= k8ByteSize) {
            align = k8ByteSize;
        } else {
            align = k16ByteSize;
        }
        AllocateGPRegister(mirType, pLoc, size, align);
    } else {
        // large struct, a pointer to the copy is used
        pLoc.reg0 = AllocateGPRegister();
        pLoc.primTypeOfReg0 = PTY_a64;
        pLoc.memSize = k8ByteSizeInt;
        aggCopySize = RoundUp(size, k8ByteSize);
    }
    return aggCopySize;
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
    if (align == k16ByteSize) {
        // C.10  If the argument has an alignment of 16 then the NGRN is rounded up to the next
        //       even number.
        nextGeneralRegNO = (nextGeneralRegNO + 1U) & ~1U;
    }
    if (mirType.GetPrimType() == PTY_i128 || mirType.GetPrimType() == PTY_u128) {
        // C.11  If the argument is an Integral Type, the size of the argument is equal to 16
        //       and the NGRN is less than 7, the argument is copied to x[NGRN] and x[NGRN+1].
        //       x[NGRN] shall contain the lower addressed double-word of the memory
        //       representation of the argument. The NGRN is incremented by two.
        //       The argument has now been allocated.
        if (nextGeneralRegNO < AArch64Abi::kNumIntParmRegs - 1) {
            DEBUG_ASSERT(size == k16ByteSize, "NIY, size must be 16-byte.");
            pLoc.reg0 = AllocateGPRegister();
            pLoc.primTypeOfReg0 = PTY_u64;
            pLoc.reg1 = AllocateGPRegister();
            pLoc.primTypeOfReg1 = PTY_u64;
            return;
        }
    } else if (size <= k16ByteSize) {
        // C.12  If the argument is a Composite Type and the size in double-words of the argument
        //       is not more than 8 minus NGRN, then the argument is copied into consecutive
        //       general-purpose registers, starting at x[NGRN]. The argument is passed as though
        //       it had been loaded into the registers from a double-word-aligned address with
        //       an appropriate sequence of LDR instructions loading consecutive registers from
        //       memory (the contents of any unused parts of the registers are unspecified by this
        //       standard). The NGRN is incremented by the number of registers used.
        //       The argument has now been allocated.
        DEBUG_ASSERT(mirType.GetPrimType() == PTY_agg, "NIY, primType must be PTY_agg.");
        auto regNum = (size <= k8ByteSize) ? kOneRegister : kTwoRegister;
        if (nextGeneralRegNO + regNum - 1 < AArch64Abi::kNumIntParmRegs) {
            pLoc.reg0 = AllocateGPRegister();
            pLoc.primTypeOfReg0 = (size <= k4ByteSize && !CGOptions::IsBigEndian()) ? PTY_u32 : PTY_u64;
            if (regNum == kTwoRegister) {
                pLoc.reg1 = AllocateGPRegister();
                pLoc.primTypeOfReg1 = (size <= k12ByteSize && !CGOptions::IsBigEndian()) ? PTY_u32 : PTY_u64;
            }
            return;
        }
    }

    // C.13  The NGRN is set to 8.
    pLoc.reg0 = kRinvalid;
    nextGeneralRegNO = AArch64Abi::kNumIntParmRegs;
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
    if (IsPrimitiveFloat(mirType.GetPrimType()) || IsPrimitiveVector(mirType.GetPrimType())) {
        // float or vector, passed by float or SIMD register
        pLoc.reg0 = AllocateSIMDFPRegister();
        pLoc.primTypeOfReg0 = mirType.GetPrimType();
    } else if (IsPrimitiveInteger(mirType.GetPrimType())) {
        // integer, passed by general purpose register
        AllocateGPRegister(mirType, pLoc, typeSize, typeAlign);
    } else {
        CHECK_FATAL(mirType.GetPrimType() == PTY_agg, "NIY");
        aggCopySize = AllocateRegisterForAgg(mirType, pLoc, typeSize, typeAlign);
    }

    SetupCCLocInfoRegCount(pLoc);
    if (pLoc.reg0 == kRinvalid) {
        // being passed in memory
        typeAlign = (typeAlign <= k8ByteSize) ? k8ByteSize : typeAlign;
        nextStackArgAdress = RoundUp(nextStackArgAdress, typeAlign);
        pLoc.memOffset = static_cast<int32>(nextStackArgAdress);
        // large struct, passed with pointer
        nextStackArgAdress += (aggCopySize != 0 ? k8ByteSize : typeSize);
    }
    return aggCopySize;
}

void AArch64CallConvImpl::SetupSecondRetReg(const MIRType &retTy2, CCLocInfo &pLoc) const
{
    DEBUG_ASSERT(pLoc.reg1 == kRinvalid, "make sure reg1 equal kRinvalid");
    PrimType pType = retTy2.GetPrimType();
    switch (pType) {
        case PTY_void:
            break;
        case PTY_u1:
        case PTY_u8:
        case PTY_i8:
        case PTY_u16:
        case PTY_i16:
        case PTY_a32:
        case PTY_u32:
        case PTY_i32:
        case PTY_ptr:
        case PTY_ref:
        case PTY_a64:
        case PTY_u64:
        case PTY_i64:
            pLoc.reg1 = AArch64Abi::intReturnRegs[1];
            pLoc.primTypeOfReg1 = IsSignedInteger(pType) ? PTY_i64 : PTY_u64; /* promote the type */
            break;
        default:
            CHECK_FATAL(false, "NYI");
    }
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
        case PTY_a32:
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

void AArch64WebKitJSCC::SetupSecondRetReg(const MIRType &retTy2, CCLocInfo &pLoc) const
{
    // already done in locate retval;
    return;
}

uint64 GHCCC::LocateNextParm(const MIRType &mirType, CCLocInfo &pLoc, bool isFirst, MIRFuncType *tFunc)
{
    std::vector<ArgumentClass> classes {};
    int32 alignedTySize = ClassificationArg(beCommon, mirType, classes);
    pLoc.memSize = alignedTySize;
    if (classes[0] == kIntegerClass) {
        if ((alignedTySize == k4ByteSize) || (alignedTySize == k8ByteSize)) {
            pLoc.reg0 = AllocateGPParmRegister();
        } else {
            CHECK_FATAL(false, "no should not go here");
        }
    } else if (classes[0] == kFloatClass) {
        if (alignedTySize == k4ByteSize) {
            pLoc.reg0 = AllocateSIMDFPParmRegisterF32();
        } else if (alignedTySize == k8ByteSize) {
            pLoc.reg0 = AllocateSIMDFPParmRegisterF64();
        } else if (alignedTySize == k16ByteSize) {
            pLoc.reg0 = AllocateSIMDFPParmRegisterF128();
        } else {
            CHECK_FATAL(false, "no should not go here");
        }
    }
    if (pLoc.reg0 == kRinvalid || classes[0] == kMemoryClass) {
        /* being passed in memory */
        CHECK_FATAL(false, "GHC does not support stack pass");
    }
    return 0;
}

void GHCCC::LocateRetVal(const MIRType &retType, CCLocInfo &pLoc)
{
    CHECK_FATAL(false, "GHC does not return");
}

int32 GHCCC::ClassificationArg(const BECommon &be, const MIRType &mirType, std::vector<ArgumentClass> &classes) const
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
        case PTY_a32:
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
            classes.push_back(kFloatClass);
            return k4ByteSize;
        case PTY_f64:
        case PTY_v2i32:
        case PTY_v4i16:
        case PTY_v8i8:
        case PTY_v2f32:
            classes.push_back(kFloatClass);
            return k8ByteSize;
        case PTY_v2i64:
        case PTY_v4i32:
        case PTY_v8i16:
        case PTY_v16i8:
        case PTY_v4f32:
        case PTY_f128:
            classes.push_back(kFloatClass);
            return k16ByteSize;
        default:
            CHECK_FATAL(false, "NYI");
    }
    return 0;
}

void GHCCC::InitReturnInfo(MIRType &retTy, CCLocInfo &pLoc)
{
    // don't see why this function exisits?
    LocateRetVal(retTy, pLoc);
}

void GHCCC::SetupSecondRetReg(const MIRType &retTy2, CCLocInfo &pLoc) const
{
    // already done in locate retval;
    CHECK_FATAL(false, "GHC does not return");
    return;
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
