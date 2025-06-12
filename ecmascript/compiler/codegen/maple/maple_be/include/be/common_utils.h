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

#ifndef MAPLEBE_INCLUDE_BE_COMMON_UTILS_H
#define MAPLEBE_INCLUDE_BE_COMMON_UTILS_H
#include <cstdint>
#include "types_def.h"
#include "mpl_logging.h"

namespace maplebe {
using namespace maple;
constexpr uint32 kOffsetAlignmentOf8Bit = 0;
constexpr uint32 kOffsetAlignmentOf16Bit = 1;
constexpr uint32 kOffsetAlignmentOf32Bit = 2;
constexpr uint32 kOffsetAlignmentOf64Bit = 3;
constexpr uint32 kOffsetAlignmentOf128Bit = 4;
constexpr uint32 kBaseOffsetAlignment = 3;
/*
 * The constexpr implementations, without assertions.  Suitable for using in
 * constants.
 */
constexpr uint32 k1FConst = 31;
constexpr uint32 k0BitSize = 0;
constexpr uint32 k1BitSize = 1;
constexpr uint32 k2BitSize = 2;
constexpr uint32 k3BitSize = 3;
constexpr uint32 k4BitSize = 4;
constexpr uint32 k5BitSize = 5;
constexpr uint32 k6BitSize = 6;
constexpr uint32 k7BitSize = 7;
constexpr uint32 k8BitSize = 8;
constexpr uint32 k12BitSize = 12;
constexpr uint32 k15BitSize = 15;
constexpr uint32 k16BitSize = 16;
constexpr uint32 k24BitSize = 24;
constexpr uint32 k32BitSize = 32;
constexpr uint32 k40BitSize = 40;
constexpr uint32 k48BitSize = 48;
constexpr uint32 k56BitSize = 56;
constexpr uint32 k64BitSize = 64;
constexpr uint32 k128BitSize = 128;
constexpr uint32 k256BitSize = 256;
constexpr uint32 k512BitSize = 512;
constexpr uint32 k1024BitSize = 1024;
constexpr uint32 k2048BitSize = 2048;

constexpr int32 k1FConstInt = 31;
constexpr int32 k0BitSizeInt = 0;
constexpr int32 k1BitSizeInt = 1;
constexpr int32 k2BitSizeInt = 2;
constexpr int32 k3BitSizeInt = 3;
constexpr int32 k4BitSizeInt = 4;
constexpr int32 k5BitSizeInt = 5;
constexpr int32 k6BitSizeInt = 6;
constexpr int32 k7BitSizeInt = 7;
constexpr int32 k8BitSizeInt = 8;
constexpr int32 k16BitSizeInt = 16;
constexpr int32 k24BitSizeInt = 24;
constexpr int32 k32BitSizeInt = 32;
constexpr int32 k48BitSizeInt = 48;
constexpr int32 k56BitSizeInt = 56;
constexpr int32 k64BitSizeInt = 64;
constexpr int32 k128BitSizeInt = 128;
constexpr int32 k256BitSizeInt = 256;
constexpr int32 k512BitSizeInt = 512;
constexpr int32 k1024BitSizeInt = 1024;

constexpr int32 kNegative8BitSize = -8;
constexpr int32 kNegative256BitSize = -256;
constexpr int32 kNegative512BitSize = -512;
constexpr int32 kNegative1024BitSize = -1024;

constexpr uint32 k0ByteSize = 0;
constexpr uint32 k1ByteSize = 1;
constexpr uint32 k2ByteSize = 2;
constexpr uint32 k3ByteSize = 3;
constexpr uint32 k4ByteSize = 4;
constexpr uint32 k8ByteSize = 8;
constexpr uint32 k9ByteSize = 9;
constexpr uint32 k12ByteSize = 12;
constexpr uint32 k14ByteSize = 14;
constexpr uint32 k15ByteSize = 15;
constexpr uint32 k16ByteSize = 16;
constexpr uint32 k32ByteSize = 32;
constexpr uint32 k64ByteSize = 64;

constexpr int32 k1ByteSizeInt = 1;
constexpr int32 k2ByteSizeInt = 2;
constexpr int32 k3ByteSizeInt = 3;
constexpr int32 k4ByteSizeInt = 4;
constexpr int32 k8ByteSizeInt = 8;
constexpr int32 k9ByteSizeInt = 9;
constexpr int32 k12ByteSizeInt = 12;
constexpr int32 k14ByteSizeInt = 14;
constexpr int32 k15ByteSizeInt = 15;
constexpr int32 k16ByteSizeInt = 16;
constexpr int32 k32ByteSizeInt = 32;

constexpr uint32 k1EightBytesSize = 8;
constexpr uint32 k2EightBytesSize = 16;
constexpr uint32 k3EightBytesSize = 24;
constexpr uint32 k4EightBytesSize = 32;

constexpr uint32 k4BitShift = 2;  /* 4 is 1 << 2; */
constexpr uint32 k8BitShift = 3;  /* 8 is 1 << 3; */
constexpr uint32 k16BitShift = 4; /* 16 is 1 << 4 */

constexpr uint32 kDwordSizeTwo = 2;

constexpr uint32 k4ByteFloatSize = 4;
constexpr uint32 k8ByteDoubleSize = 8;

/* Storage location of operands in one insn */
constexpr int32 kInsnFirstOpnd = 0;
constexpr int32 kInsnSecondOpnd = 1;
constexpr int32 kInsnThirdOpnd = 2;
constexpr int32 kInsnFourthOpnd = 3;
constexpr int32 kInsnFifthOpnd = 4;
constexpr int32 kInsnSixthOpnd = 5;
constexpr int32 kInsnSeventhOpnd = 6;
constexpr int32 kInsnEighthOpnd = 7;
constexpr int32 kInsnNinthOpnd = 8;
constexpr int32 kInsnMaxOpnd = 9;

/* Reg of CCLocInfo */
constexpr uint32 kFirstReg = 0;
constexpr uint32 kSecondReg = 1;
constexpr uint32 kThirdReg = 2;
constexpr uint32 kFourthReg = 3;
constexpr uint32 kFifthReg = 4;
constexpr uint32 kSixthReg = 5;
constexpr uint32 kSeventhReg = 6;


/* inline asm operand designations */
constexpr uint32 kAsmStringOpnd = 0;
constexpr uint32 kAsmOutputListOpnd = 1;
constexpr uint32 kAsmClobberListOpnd = 2;
constexpr uint32 kAsmInputListOpnd = 3;
constexpr uint32 kAsmOutputConstraintOpnd = 4;
constexpr uint32 kAsmInputConstraintOpnd = 5;
constexpr uint32 kAsmOutputRegPrefixOpnd = 6;
constexpr uint32 kAsmInputRegPrefixOpnd = 7;

/* Number of registers */
constexpr uint32 kOneRegister = 1;
constexpr uint32 kTwoRegister = 2;
constexpr uint32 kThreeRegister = 3;
constexpr uint32 kFourRegister = 4;

/* position of an operand within an instruction */
constexpr uint32 kOperandPosition0 = 0;
constexpr uint32 kOperandPosition1 = 1;
constexpr uint32 kOperandPosition2 = 2;

/* Size of struct for memcpy */
constexpr uint32 kParmMemcpySize = 40;

/* Check whether the value is an even number. */
constexpr int32 kDivide2 = 2;
constexpr int32 kRegNum2 = 2;
constexpr int32 kStepNum2 = 2;
constexpr int32 kInsnNum2 = 2;
constexpr int32 kOpndNum2 = 2;

/* alignment in bytes of uint8 */
constexpr uint8 kAlignOfU8 = 3;

/*
 * if the number of local refvar is less than 12, use stp or str to init local refvar
 * else call function MCC_InitializeLocalStackRef to init.
 */
constexpr int32 kRefNum12 = 12;

/* mod function max argument size */
constexpr uint32 kMaxModFuncArgSize = 8;

/* string length of spacial name "__EARetTemp__" */
constexpr int32 kEARetTempNameSize = 10;

/*
 * Aarch64 data processing instructions have 12 bits of space for values in their instuction word
 * This is arranged as a four-bit rotate value and an eight-bit immediate value:
 */
constexpr uint32 kMaxImmVal5Bits = 5;
constexpr uint32 kMaxImmVal6Bits = 6;
constexpr uint32 kMaxImmVal8Bits = 8;
constexpr uint32 kMaxImmVal12Bits = 12;
constexpr uint32 kMaxImmVal13Bits = 13;
constexpr uint32 kMaxImmVal16Bits = 16;

constexpr int32 kMaxPimm8 = 4095;
constexpr int32 kMaxPimm16 = 8190;
constexpr int32 kMaxPimm32 = 16380;
constexpr int32 kMaxPimm64 = 32760;
constexpr int32 kMaxPimm128 = 65520;

constexpr int32 kMaxPimm[k5BitSize] = {kMaxPimm8, kMaxPimm16, kMaxPimm32, kMaxPimm64, kMaxPimm128};
constexpr int32 kMaxPairPimm[k3BitSize] = {k256BitSize, k512BitSize, k512BitSize};

constexpr int32 kMaxSimm32 = 255;
constexpr int32 kMaxSimm32Pair = 252;
constexpr int32 kMinSimm32 = kNegative256BitSize;
constexpr int32 kMaxSimm64Pair = 504;
constexpr int32 kMinSimm64 = kNegative512BitSize;

constexpr int32 kMax8UnsignedImm = 255;
constexpr int32 kMax12UnsignedImm = 4096;
constexpr int32 kMax13UnsignedImm = 8192;
constexpr int32 kMax16UnsignedImm = 65535;

/* Dedicated for Vector */
constexpr int32 kMinImmVal = -128;
constexpr int32 kMaxImmVal = 255;

/* aarch64 assembly takes up to 24-bits */
constexpr uint32 kMaxImmVal24Bits = 24;

constexpr uint32 kDecimalMax = 10;

constexpr double kMicroSecPerMilliSec = 1000.0;

constexpr double kPercent = 100.0;

enum ConditionCode : uint8 {
    CC_EQ, /* equal */
    CC_NE, /* not equal */
    CC_CS, /* carry set (== HS) */
    CC_HS, /* unsigned higher or same (== CS) */
    CC_CC, /* carry clear (== LO) */
    CC_LO, /* Unsigned lower (== CC) */
    CC_MI, /* Minus or negative result */
    CC_PL, /* positive or zero result */
    CC_VS, /* overflow */
    CC_VC, /* no overflow */
    CC_HI, /* unsigned higher */
    CC_LS, /* unsigned lower or same */
    CC_GE, /* signed greater than or equal */
    CC_LT, /* signed less than */
    CC_GT, /* signed greater than */
    CC_LE, /* signed less than or equal */
    CC_AL, /* always, this is the default. usually omitted. */
    kCcLast
};

inline ConditionCode GetReverseCC(ConditionCode cc)
{
    switch (cc) {
        case CC_NE:
            return CC_EQ;
        case CC_EQ:
            return CC_NE;
        case CC_HS:
            return CC_LO;
        case CC_LO:
            return CC_HS;
        case CC_MI:
            return CC_PL;
        case CC_PL:
            return CC_MI;
        case CC_VS:
            return CC_VC;
        case CC_VC:
            return CC_VS;
        case CC_HI:
            return CC_LS;
        case CC_LS:
            return CC_HI;
        case CC_LT:
            return CC_GE;
        case CC_GE:
            return CC_LT;
        case CC_GT:
            return CC_LE;
        case CC_LE:
            return CC_GT;
        default:
            CHECK_FATAL(0, "unknown condition code");
    }
    return kCcLast;
}
inline ConditionCode GetReverseBasicCC(ConditionCode cc)
{
    switch (cc) {
        case CC_NE:
            return CC_EQ;
        case CC_EQ:
            return CC_NE;
        case CC_LT:
            return CC_GE;
        case CC_GE:
            return CC_LT;
        case CC_GT:
            return CC_LE;
        case CC_LE:
            return CC_GT;
        default:
            CHECK_FATAL(false, "Not support yet.");
    }
    return kCcLast;
}

inline bool IsPowerOf2Const(uint64 i)
{
    return (i & (i - 1)) == 0;
}

inline uint64 RoundUpConst(uint64 offset, uint64 align)
{
    CHECK_FATAL(offset <= UINT64_MAX - align, "must not be zero");
    DEBUG_ASSERT(offset + align >= 1, "result must be uint");
    return (-align) & (offset + align - 1);
}

inline bool IsPowerOf2(uint64 i)
{
    return IsPowerOf2Const(i);
}

/* align must be a power of 2 */
inline uint64 RoundUp(uint64 offset, uint64 align)
{
    if (align == 0) {
        return offset;
    }
    DEBUG_ASSERT(IsPowerOf2(align), "align must be power of 2!");
    return RoundUpConst(offset, align);
}

inline int64 RoundDownConst(int64 offset, int64 align)
{
    return (-align) & offset;
}

// align must be a power of 2
inline int64 RoundDown(int64 offset, int64 align)
{
    if (align == 0) {
        return offset;
    }
    DEBUG_ASSERT(IsPowerOf2(align), "align must be power of 2!");
    return RoundDownConst(offset, align);
}

inline bool IsAlignedTo(uint64 offset, uint64 align)
{
    DEBUG_ASSERT(IsPowerOf2(align), "align must be power of 2!");
    CHECK_FATAL(align > 0, "must not be zero");
    return (offset & (align - 1)) == 0;
}
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_BE_COMMON_UTILS_H */
