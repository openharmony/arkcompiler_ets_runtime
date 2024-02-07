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

namespace {
constexpr int kMaxRegCount = 4;

/*
 * Refer to ARM IHI 0055C_beta: Procedure Call Standard for
 * ARM 64-bit Architecture. Table 1.
 */
enum AArch64ArgumentClass : uint8 { kAArch64NoClass, kAArch64IntegerClass, kAArch64FloatClass, kAArch64MemoryClass };

int32 ProcessNonStructAndNonArrayWhenClassifyAggregate(const MIRType &mirType,
                                                       AArch64ArgumentClass classes[kMaxRegCount], size_t classesLength)
{
    CHECK_FATAL(classesLength > 0, "classLength must > 0");
    /* scalar type */
    switch (mirType.GetPrimType()) {
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
            classes[0] = kAArch64IntegerClass;
            return 1;
        case PTY_f32:
        case PTY_f64:
        case PTY_c64:
        case PTY_c128:
            classes[0] = kAArch64FloatClass;
            return 1;
        default:
            CHECK_FATAL(false, "NYI");
    }

    /* should not reach to this point */
    return 0;
}

PrimType TraverseStructFieldsForFp(MIRType *ty, uint32 &numRegs)
{
    if (ty->GetKind() == kTypeArray) {
        MIRArrayType *arrtype = static_cast<MIRArrayType *>(ty);
        MIRType *pty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(arrtype->GetElemTyIdx());
        if (pty->GetKind() == kTypeArray || pty->GetKind() == kTypeStruct) {
            return TraverseStructFieldsForFp(pty, numRegs);
        }
        for (uint32 i = 0; i < arrtype->GetDim(); ++i) {
            numRegs += arrtype->GetSizeArrayItem(i);
        }
        return pty->GetPrimType();
    } else if (ty->GetKind() == kTypeStruct) {
        MIRStructType *sttype = static_cast<MIRStructType *>(ty);
        FieldVector fields = sttype->GetFields();
        PrimType oldtype = PTY_void;
        for (uint32 fcnt = 0; fcnt < fields.size(); ++fcnt) {
            TyIdx fieldtyidx = fields[fcnt].second.first;
            MIRType *fieldty = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldtyidx);
            PrimType ptype = TraverseStructFieldsForFp(fieldty, numRegs);
            if (oldtype != PTY_void && oldtype != ptype) {
                return PTY_void;
            } else {
                oldtype = ptype;
            }
        }
        return oldtype;
    } else {
        numRegs++;
        return ty->GetPrimType();
    }
}

int32 ClassifyAggregate(const BECommon &be, MIRType &mirType, AArch64ArgumentClass classes[kMaxRegCount],
                        size_t classesLength, uint32 &fpSize);

uint32 ProcessStructWhenClassifyAggregate(const BECommon &be, MIRStructType &structType,
                                          AArch64ArgumentClass classes[kMaxRegCount], size_t classesLength,
                                          uint32 &fpSize)
{
    CHECK_FATAL(classesLength > 0, "classLength must > 0");
    uint32 sizeOfTyInDwords =
        static_cast<uint32>(RoundUp(be.GetTypeSize(structType.GetTypeIndex()), k8ByteSize) >> k8BitShift);
    bool isF32 = false;
    bool isF64 = false;
    uint32 numRegs = 0;
    for (uint32 f = 0; f < structType.GetFieldsSize(); ++f) {
        TyIdx fieldTyIdx = structType.GetFieldsElemt(f).second.first;
        MIRType *fieldType = GlobalTables::GetTypeTable().GetTypeFromTyIdx(fieldTyIdx);
        PrimType pType = TraverseStructFieldsForFp(fieldType, numRegs);
        if (pType == PTY_f32) {
            if (isF64) {
                isF64 = false;
                break;
            }
            isF32 = true;
        } else if (pType == PTY_f64) {
            if (isF32) {
                isF32 = false;
                break;
            }
            isF64 = true;
        } else if (IsPrimitiveVector(pType)) {
            isF64 = true;
            break;
        } else {
            isF32 = isF64 = false;
            break;
        }
    }
    if (isF32 || isF64) {
        CHECK_FATAL(numRegs <= classesLength, "ClassifyAggregate: num regs exceed limit");
        for (uint32 i = 0; i < numRegs; ++i) {
            classes[i] = kAArch64FloatClass;
        }
        fpSize = isF32 ? k4ByteSize : k8ByteSize;
        return numRegs;
    }

    classes[0] = kAArch64IntegerClass;
    if (sizeOfTyInDwords == kDwordSizeTwo) {
        classes[1] = kAArch64IntegerClass;
    }
    DEBUG_ASSERT(sizeOfTyInDwords <= classesLength, "sizeOfTyInDwords exceed limit");
    return sizeOfTyInDwords;
}

/*
 * Analyze the given aggregate using the rules given by the ARM 64-bit ABI and
 * return the number of doublewords to be passed in registers; the classes of
 * the doublewords are returned in parameter "classes"; if 0 is returned, it
 * means the whole aggregate is passed in memory.
 */
int32 ClassifyAggregate(const BECommon &be, MIRType &mirType, AArch64ArgumentClass classes[kMaxRegCount],
                        size_t classesLength, uint32 &fpSize)
{
    CHECK_FATAL(classesLength > 0, "invalid index");
    uint64 sizeOfTy = be.GetTypeSize(mirType.GetTypeIndex());
    /* Rule B.3.
     * If the argument type is a Composite Type that is larger than 16 bytes
     * then the argument is copied to memory allocated by the caller and
     * the argument is replaced by a pointer to the copy.
     */
    if ((sizeOfTy > k16ByteSize) || (sizeOfTy == 0)) {
        return 0;
    }

    /*
     * An argument of any Integer class takes up an integer register
     * which is a single double-word.
     * Rule B.4. The size of an argument of composite type is rounded up to the nearest
     * multiple of 8 bytes.
     */
    int64 sizeOfTyInDwords = static_cast<int64>(RoundUp(sizeOfTy, k8ByteSize) >> k8BitShift);
    DEBUG_ASSERT(sizeOfTyInDwords > 0, "sizeOfTyInDwords should be sizeOfTyInDwords > 0");
    DEBUG_ASSERT(sizeOfTyInDwords <= kTwoRegister, "sizeOfTyInDwords should be <= 2");
    int64 i;
    for (i = 0; i < sizeOfTyInDwords; ++i) {
        classes[i] = kAArch64NoClass;
    }
    if ((mirType.GetKind() != kTypeStruct) && (mirType.GetKind() != kTypeArray) && (mirType.GetKind() != kTypeUnion)) {
        return ProcessNonStructAndNonArrayWhenClassifyAggregate(mirType, classes, classesLength);
    }
    if (mirType.GetKind() == kTypeStruct) {
        MIRStructType &structType = static_cast<MIRStructType &>(mirType);
        return static_cast<int32>(ProcessStructWhenClassifyAggregate(be, structType, classes, classesLength, fpSize));
    }
    /* post merger clean-up */
    for (i = 0; i < sizeOfTyInDwords; ++i) {
        if (classes[i] == kAArch64MemoryClass) {
            return 0;
        }
    }
    return static_cast<int32>(sizeOfTyInDwords);
}
}  // namespace

/* external interface to look for pure float struct */
uint32 AArch64CallConvImpl::FloatParamRegRequired(MIRStructType &structType, uint32 &fpSize)
{
    if (structType.GetSize() > k32ByteSize) {
        return 0;
    }
    AArch64ArgumentClass classes[kMaxRegCount];
    uint32 numRegs = ProcessStructWhenClassifyAggregate(beCommon, structType, classes, kMaxRegCount, fpSize);
    if (numRegs == 0) {
        return 0;
    }

    bool isPure = true;
    for (uint32 i = 0; i < numRegs; ++i) {
        DEBUG_ASSERT(i < kMaxRegCount, "i should be lower than kMaxRegCount");
        if (classes[i] != kAArch64FloatClass) {
            isPure = false;
            break;
        }
    }
    if (isPure) {
        return numRegs;
    }
    return 0;
}

int32 AArch64CallConvImpl::LocateRetVal(MIRType &retType, CCLocInfo &pLoc)
{
    InitCCLocInfo(pLoc);
    uint32 retSize = beCommon.GetTypeSize(retType.GetTypeIndex().GetIdx());
    if (retSize == 0) {
        return 0; /* size 0 ret val */
    }
    if (retSize <= k16ByteSize) {
        /* For return struct size less or equal to 16 bytes, the values */
        /* are returned in register pairs. */
        AArch64ArgumentClass classes[kMaxRegCount] = {kAArch64NoClass}; /* Max of four floats. */
        uint32 fpSize;
        uint32 numRegs = static_cast<uint32>(ClassifyAggregate(beCommon, retType, classes, sizeof(classes), fpSize));
        if (classes[0] == kAArch64FloatClass) {
            CHECK_FATAL(numRegs <= kMaxRegCount, "LocateNextParm: illegal number of regs");
            AllocateNSIMDFPRegisters(pLoc, numRegs);
            pLoc.numFpPureRegs = numRegs;
            pLoc.fpSize = fpSize;
            return 0;
        } else {
            CHECK_FATAL(numRegs <= kTwoRegister, "LocateNextParm: illegal number of regs");
            if (numRegs == kOneRegister) {
                pLoc.reg0 = AllocateGPRegister();
            } else {
                AllocateTwoGPRegisters(pLoc);
            }
            return 0;
        }
    } else {
        /* For return struct size > 16 bytes the pointer returns in x8. */
        pLoc.reg0 = R8;
        return GetPointerSize();
    }
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

int32 AArch64CallConvImpl::LocateNextParm(MIRType &mirType, CCLocInfo &pLoc, bool isFirst, MIRFunction *tFunc)
{
    InitCCLocInfo(pLoc);
    // C calling convention.
    bool is64x1vec = false;

    if (isFirst) {
        MIRFunction *func = tFunc != nullptr ? tFunc : const_cast<MIRFunction *>(beCommon.GetMIRModule().CurFunction());
        if (func->IsFirstArgReturn()) {
            TyIdx tyIdx = func->GetFuncRetStructTyIdx();
            size_t size = beCommon.GetTypeSize(tyIdx);
            if (size == 0) {
                /* For return struct size 0 there is no return value. */
                return 0;
            }
            /* For return struct size > 16 bytes the pointer returns in x8. */
            pLoc.reg0 = R8;
            return GetPointerSize();
        }
    }
    uint64 typeSize = beCommon.GetTypeSize(mirType.GetTypeIndex());
    if (typeSize == 0) {
        return 0;
    }
    int32 typeAlign = beCommon.GetTypeAlign(mirType.GetTypeIndex());
    /*
     * Rule C.12 states that we do round nextStackArgAdress up before we use its value
     * according to the alignment requirement of the argument being processed.
     * We do the rounding up at the end of LocateNextParm(),
     * so we want to make sure our rounding up is correct.
     */
    DEBUG_ASSERT((nextStackArgAdress & (std::max(typeAlign, static_cast<int32>(k8ByteSize)) - 1)) == 0,
                 "C.12 alignment requirement is violated");
    pLoc.memSize = static_cast<int32>(typeSize);
    ++paramNum;

    int32 aggCopySize = 0;
    switch (mirType.GetPrimType()) {
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
        case PTY_i128:
        case PTY_u128:
            /* Rule C.7 */
            typeSize = k8ByteSize;
            pLoc.reg0 = is64x1vec ? AllocateSIMDFPRegister() : AllocateGPRegister();
            DEBUG_ASSERT(nextGeneralRegNO <= AArch64Abi::kNumIntParmRegs, "RegNo should be pramRegNO");
            break;
        /*
         * for c64 complex numbers, we assume
         * - callers marshall the two f32 numbers into one f64 register
         * - callees de-marshall one f64 value into the real and the imaginery part
         */
        case PTY_f32:
        case PTY_f64:
        case PTY_c64:
        case PTY_v2i32:
        case PTY_v4i16:
        case PTY_v8i8:
        case PTY_v2u32:
        case PTY_v4u16:
        case PTY_v8u8:
        case PTY_v2f32:
            /* Rule C.1 */
            DEBUG_ASSERT(GetPrimTypeSize(PTY_f64) == k8ByteSize, "unexpected type size");
            typeSize = k8ByteSize;
            pLoc.reg0 = AllocateSIMDFPRegister();
            break;
        /*
         * for c128 complex numbers, we assume
         * - callers marshall the two f64 numbers into one f128 register
         * - callees de-marshall one f128 value into the real and the imaginery part
         */
        case PTY_c128:
        case PTY_v2i64:
        case PTY_v4i32:
        case PTY_v8i16:
        case PTY_v16i8:
        case PTY_v2u64:
        case PTY_v4u32:
        case PTY_v8u16:
        case PTY_v16u8:
        case PTY_v2f64:
        case PTY_v4f32:
            /* SIMD-FP registers have 128-bits. */
            pLoc.reg0 = AllocateSIMDFPRegister();
            DEBUG_ASSERT(nextFloatRegNO <= AArch64Abi::kNumFloatParmRegs,
                         "regNO should not be greater than kNumFloatParmRegs");
            DEBUG_ASSERT(typeSize == k16ByteSize, "unexpected type size");
            break;
        /*
         * case of quad-word integer:
         * we don't support java yet.
         * if (has-16-byte-alignment-requirement)
         * nextGeneralRegNO = (nextGeneralRegNO+1) & ~1; // C.8 round it up to the next even number
         * try allocate two consecutive registers at once.
         */
        /* case PTY_agg */
        case PTY_agg: {
            aggCopySize = ProcessPtyAggWhenLocateNextParm(mirType, pLoc, typeSize, typeAlign);
            break;
        }
        default:
            CHECK_FATAL(false, "NYI");
    }

    /* Rule C.12 */
    if (pLoc.reg0 == kRinvalid) {
        /* being passed in memory */
        nextStackArgAdress = pLoc.memOffset + static_cast<int32>(static_cast<int64>(typeSize));
    }
    return aggCopySize;
}

int32 AArch64CallConvImpl::ProcessPtyAggWhenLocateNextParm(MIRType &mirType, CCLocInfo &pLoc, uint64 &typeSize,
                                                           int32 typeAlign)
{
    /*
     * In AArch64, integer-float or float-integer
     * argument passing is not allowed. All should go through
     * integer-integer.
     * In the case where a struct is homogeneous composed of one of the fp types,
     * either all single fp or all double fp, then it can be passed by float-float.
     */
    AArch64ArgumentClass classes[kMaxRegCount] = {kAArch64NoClass};
    typeSize = beCommon.GetTypeSize(mirType.GetTypeIndex().GetIdx());
    int32 aggCopySize = 0;
    if (typeSize > k16ByteSize) {
        aggCopySize = static_cast<int32>(RoundUp(typeSize, GetPointerSize()));
    }
    /*
     * alignment requirement
     * Note. This is one of a few things iOS diverges from
     * the ARM 64-bit standard. They don't observe the round-up requirement.
     */
    if (typeAlign == k16ByteSize) {
        RoundNGRNUpToNextEven();
    }

    uint32 fpSize;
    uint32 numRegs = static_cast<uint32>(
        ClassifyAggregate(beCommon, mirType, classes, sizeof(classes) / sizeof(AArch64ArgumentClass), fpSize));
    if (classes[0] == kAArch64FloatClass) {
        CHECK_FATAL(numRegs <= kMaxRegCount, "LocateNextParm: illegal number of regs");
        typeSize = k8ByteSize;
        AllocateNSIMDFPRegisters(pLoc, numRegs);
        pLoc.numFpPureRegs = numRegs;
        pLoc.fpSize = fpSize;
    } else if (numRegs == 1) {
        /* passing in registers */
        typeSize = k8ByteSize;
        if (classes[0] == kAArch64FloatClass) {
            CHECK_FATAL(false, "param passing in FP reg not allowed here");
        } else {
            pLoc.reg0 = AllocateGPRegister();
            /* Rule C.11 */
            DEBUG_ASSERT((pLoc.reg0 != kRinvalid) || (nextGeneralRegNO == AArch64Abi::kNumIntParmRegs),
                         "reg0 should not be kRinvalid or nextGeneralRegNO should equal kNumIntParmRegs");
        }
    } else if (numRegs == kTwoRegister) {
        /* Other aggregates with 8 < size <= 16 bytes can be allocated in reg pair */
        DEBUG_ASSERT(classes[0] == kAArch64IntegerClass || classes[0] == kAArch64NoClass,
                     "classes[0] must be either integer class or no class");
        DEBUG_ASSERT(classes[1] == kAArch64IntegerClass || classes[1] == kAArch64NoClass,
                     "classes[1] must be either integer class or no class");
        AllocateTwoGPRegisters(pLoc);
        /* Rule C.11 */
        if (pLoc.reg0 == kRinvalid) {
            nextGeneralRegNO = AArch64Abi::kNumIntParmRegs;
        }
    } else {
        /*
         * 0 returned from ClassifyAggregate(). This means the whole data
         * is passed thru memory.
         * Rule B.3.
         *  If the argument type is a Composite Type that is larger than 16
         *  bytes then the argument is copied to memory allocated by the
         *  caller and the argument is replaced by a pointer to the copy.
         *
         * Try to allocate an integer register
         */
        typeSize = k8ByteSize;
        pLoc.reg0 = AllocateGPRegister();
        pLoc.memSize = k8ByteSizeInt; /* byte size of a pointer in AArch64 */
        if (pLoc.reg0 != kRinvalid) {
            numRegs = 1;
        }
    }
    /* compute rightpad */
    if ((numRegs == 0) || (pLoc.reg0 == kRinvalid)) {
        /* passed in memory */
        typeSize = RoundUp(static_cast<uint64>(static_cast<int64>(pLoc.memSize)), k8ByteSize);
    }
    return aggCopySize;
}

/*
 * instantiated with the type of the function return value, it describes how
 * the return value is to be passed back to the caller
 *
 *  Refer to ARM IHI 0055C_beta: Procedure Call Standard for
 *  the ARM 64-bit Architecture. $5.5
 *  "If the type, T, of the result of a function is such that
 *     void func(T arg)
 *   would require that 'arg' be passed as a value in a register
 *   (or set of registers) according to the rules in $5.4 Parameter
 *   Passing, then the result is returned in the same registers
 *   as would be used for such an argument.
 */
void AArch64CallConvImpl::InitReturnInfo(MIRType &retTy, CCLocInfo &ccLocInfo)
{
    PrimType pType = retTy.GetPrimType();
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
            ccLocInfo.regCount = 1;
            ccLocInfo.reg0 = AArch64Abi::intReturnRegs[0];
            ccLocInfo.primTypeOfReg0 = IsSignedInteger(pType) ? PTY_i32 : PTY_u32; /* promote the type */
            return;

        case PTY_ptr:
        case PTY_ref:
            CHECK_FATAL(false, "PTY_ptr should have been lowered");
            return;

        case PTY_a64:
        case PTY_u64:
        case PTY_i64:
        case PTY_i128:
        case PTY_u128:
            ccLocInfo.regCount = 1;
            ccLocInfo.reg0 = AArch64Abi::intReturnRegs[0];
            ccLocInfo.primTypeOfReg0 = IsSignedInteger(pType) ? PTY_i64 : PTY_u64; /* promote the type */
            return;

        /*
         * for c64 complex numbers, we assume
         * - callers marshall the two f32 numbers into one f64 register
         * - callees de-marshall one f64 value into the real and the imaginery part
         */
        case PTY_f32:
        case PTY_f64:
        case PTY_c64:
        case PTY_v2i32:
        case PTY_v4i16:
        case PTY_v8i8:
        case PTY_v2u32:
        case PTY_v4u16:
        case PTY_v8u8:
        case PTY_v2f32:

        /*
         * for c128 complex numbers, we assume
         * - callers marshall the two f64 numbers into one f128 register
         * - callees de-marshall one f128 value into the real and the imaginery part
         */
        case PTY_c128:
        case PTY_v2i64:
        case PTY_v4i32:
        case PTY_v8i16:
        case PTY_v16i8:
        case PTY_v2u64:
        case PTY_v4u32:
        case PTY_v8u16:
        case PTY_v16u8:
        case PTY_v2f64:
        case PTY_v4f32:
            ccLocInfo.regCount = 1;
            ccLocInfo.reg0 = AArch64Abi::floatReturnRegs[0];
            ccLocInfo.primTypeOfReg0 = pType;
            return;

        /*
         * Refer to ARM IHI 0055C_beta: Procedure Call Standard for
         * the ARM 64-bit Architecture. $5.5
         * "Otherwise, the caller shall reserve a block of memory of
         * sufficient size and alignment to hold the result. The
         * address of the memory block shall be passed as an additional
         * argument to the function in x8. The callee may modify the
         * result memory block at any point during the execution of the
         * subroutine (there is no requirement for the callee to preserve
         * the value stored in x8)."
         */
        case PTY_agg: {
            uint64 size = beCommon.GetTypeSize(retTy.GetTypeIndex());
            if ((size > k16ByteSize) || (size == 0)) {
                /*
                 * The return value is returned via memory.
                 * The address is in X8 and passed by the caller.
                 */
                SetupToReturnThroughMemory(ccLocInfo);
                return;
            }
            uint32 fpSize;
            AArch64ArgumentClass classes[kMaxRegCount] = {kAArch64NoClass};
            ccLocInfo.regCount = static_cast<uint8>(
                ClassifyAggregate(beCommon, retTy, classes, sizeof(classes) / sizeof(AArch64ArgumentClass), fpSize));
            if (classes[0] == kAArch64FloatClass) {
                switch (ccLocInfo.regCount) {
                    case kFourRegister:
                        ccLocInfo.reg3 = AArch64Abi::floatReturnRegs[kFourthReg];
                        break;
                    case kThreeRegister:
                        ccLocInfo.reg2 = AArch64Abi::floatReturnRegs[kThirdReg];
                        break;
                    case kTwoRegister:
                        ccLocInfo.reg1 = AArch64Abi::floatReturnRegs[kSecondReg];
                        break;
                    case kOneRegister:
                        ccLocInfo.reg0 = AArch64Abi::floatReturnRegs[kFirstReg];
                        break;
                    default:
                        CHECK_FATAL(0, "AArch64CallConvImpl: unsupported");
                }
                if (fpSize == k4ByteSize) {
                    ccLocInfo.primTypeOfReg0 = ccLocInfo.primTypeOfReg1 = PTY_f32;
                } else {
                    ccLocInfo.primTypeOfReg0 = ccLocInfo.primTypeOfReg1 = PTY_f64;
                }
                return;
            } else if (ccLocInfo.regCount == 0) {
                SetupToReturnThroughMemory(ccLocInfo);
                return;
            } else {
                if (ccLocInfo.regCount == 1) {
                    /* passing in registers */
                    if (classes[0] == kAArch64FloatClass) {
                        ccLocInfo.reg0 = AArch64Abi::floatReturnRegs[0];
                        ccLocInfo.primTypeOfReg0 = PTY_f64;
                    } else {
                        ccLocInfo.reg0 = AArch64Abi::intReturnRegs[0];
                        ccLocInfo.primTypeOfReg0 = PTY_i64;
                    }
                } else {
                    DEBUG_ASSERT(ccLocInfo.regCount <= k2ByteSize,
                                 "reg count from ClassifyAggregate() should be 0, 1, or 2");
                    DEBUG_ASSERT(classes[0] == kAArch64IntegerClass, "error val :classes[0]");
                    DEBUG_ASSERT(classes[1] == kAArch64IntegerClass, "error val :classes[1]");
                    ccLocInfo.reg0 = AArch64Abi::intReturnRegs[0];
                    ccLocInfo.primTypeOfReg0 = PTY_i64;
                    ccLocInfo.reg1 = AArch64Abi::intReturnRegs[1];
                    ccLocInfo.primTypeOfReg1 = PTY_i64;
                }
                return;
            }
        }
        default:
            CHECK_FATAL(false, "NYI");
    }
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

int32 AArch64WebKitJSCC::LocateNextParm(MIRType &mirType, CCLocInfo &pLoc, bool isFirst, MIRFunction *tFunc)
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

int32 AArch64WebKitJSCC::LocateRetVal(MIRType &retType, CCLocInfo &pLoc)
{
    InitCCLocInfo(pLoc);
    std::vector<ArgumentClass> classes {}; /* Max of four Regs. */
    int32 alignedTySize = ClassificationRet(beCommon, retType, classes);
    if (alignedTySize == 0) {
        return 0; /* size 0 ret val */
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
    return 0;
}

int32 AArch64WebKitJSCC::ClassificationRet(const BECommon &be, MIRType &mirType,
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

int32 AArch64WebKitJSCC::ClassificationArg(const BECommon &be, MIRType &mirType,
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

int32 GHCCC::LocateNextParm(MIRType &mirType, CCLocInfo &pLoc, bool isFirst, MIRFunction *tFunc)
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

int32 GHCCC::LocateRetVal(MIRType &retType, CCLocInfo &pLoc)
{
    CHECK_FATAL(false, "GHC does not return");
    return 0;
}

int32 GHCCC::ClassificationArg(const BECommon &be, MIRType &mirType, std::vector<ArgumentClass> &classes) const
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
