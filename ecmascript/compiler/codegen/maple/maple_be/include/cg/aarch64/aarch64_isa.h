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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ISA_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ISA_H

#include "isa.h"

#define DEFINE_MOP(op, ...) op,
enum AArch64MopT : maple::uint32 {
#include "abstract_mmir.def"
#include "aarch64_md.def"
    kMopLast
};
#undef DEFINE_MOP

namespace maplebe {
/*
 * ARM Architecture Reference Manual (for ARMv8)
 * D1.8.2
 */
constexpr int kAarch64StackPtrAlignment = 16;

constexpr int32 kAarch64OffsetAlign = 8;
constexpr uint32 kAarch64IntregBytelen = 8; /* 64-bit */
constexpr uint32 kAarch64FpregBytelen = 8;  /* only lower 64 bits are used */
constexpr int kAarch64SizeOfFplr = 16;

enum StpLdpImmBound : int {
    kStpLdpImm64LowerBound = -512,
    kStpLdpImm64UpperBound = 504,
    kStpLdpImm32LowerBound = -256,
    kStpLdpImm32UpperBound = 252
};

enum StrLdrPerPostBound : int64 { kStrLdrPerPostLowerBound = -256, kStrLdrPerPostUpperBound = 255 };

constexpr int64 kStrAllLdrAllImmLowerBound = 0;
enum StrLdrImmUpperBound : int64 {
    kStrLdrImm32UpperBound = 16380, /* must be a multiple of 4 */
    kStrLdrImm64UpperBound = 32760, /* must be a multiple of 8 */
    kStrbLdrbImmUpperBound = 4095,
    kStrhLdrhImmUpperBound = 8190
};

/*
 * ARM Compiler armasm User Guide version 6.6.
 * http://infocenter.arm.com/help/index.jsp?topic=/com.arm.doc.dui0473j/deb1353594352617.html
 * (retrieved on 3/24/2017)
 *
 * $ 4.1 Registers in AArch64 state
 * ...When you use the 32-bit form of an instruction, the upper
 * 32 bits of the source registers are ignored and
 * the upper 32 bits of the destination register are set to zero.
 * ^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^
 *
 * There is no register named W31 or X31.
 * Depending on the instruction, register 31 is either the stack
 * pointer or the zero register. When used as the stack pointer,
 * you refer to it as "SP". When used as the zero register, you refer
 * to it as WZR in a 32-bit context or XZR in a 64-bit context.
 * The zero register returns 0 when read and discards data when
 * written (e.g., when setting the status register for testing).
 */
enum AArch64reg : uint32 {
    kRinvalid = kInvalidRegNO,
/* integer registers */
#define INT_REG(ID, PREF32, PREF64, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) R##ID,
#define INT_REG_ALIAS(ALIAS, ID, PREF32, PREF64)
#include "aarch64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, PV, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) V##ID,
#define FP_SIMD_REG_ALIAS(ID)
#include "aarch64_fp_simd_regs.def"
#undef FP_SIMD_REG
#undef FP_SIMD_REG_ALIAS
    kMaxRegNum,
    kRFLAG,
    kAllRegNum,
/* alias */
#define INT_REG(ID, PREF32, PREF64, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill)
#define INT_REG_ALIAS(ALIAS, ID, PREF32, PREF64) R##ALIAS = R##ID,
#include "aarch64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
#define FP_SIMD_REG(ID, PV, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill)
#define FP_SIMD_REG_ALIAS(ID) S##ID = V##ID,
#include "aarch64_fp_simd_regs.def"
#undef FP_SIMD_REG
#undef FP_SIMD_REG_ALIAS
#define FP_SIMD_REG(ID, PV, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill)
#define FP_SIMD_REG_ALIAS(ID) D##ID = V##ID,
#include "aarch64_fp_simd_regs.def"
#undef FP_SIMD_REG
#undef FP_SIMD_REG_ALIAS
};

class Insn;

namespace AArch64isa {
static inline bool IsGPRegister(AArch64reg r)
{
    return R0 <= r && r <= RZR;
}

static inline bool IsFPSIMDRegister(AArch64reg r)
{
    return V0 <= r && r <= V31;
}

static inline bool IsPhysicalRegister(regno_t r)
{
    return r < kMaxRegNum;
}

static inline RegType GetRegType(AArch64reg r)
{
    if (IsGPRegister(r)) {
        return kRegTyInt;
    }
    if (IsFPSIMDRegister(r)) {
        return kRegTyFloat;
    }
    DEBUG_ASSERT(false, "No suitable register type to return?");
    return kRegTyUndef;
}

static inline bool IsPseudoInstruction(MOperator mOp)
{
    return (mOp >= MOP_pseudo_param_def_x && mOp <= MOP_pseudo_eh_def_x);
}

/*
 * Precondition: The given insn is a jump instruction.
 * Get the jump target label operand index from the given instruction.
 * Note: MOP_xbr is a jump instruction, but the target is unknown at compile time,
 * because a register instead of label. So we don't take it as a branching instruction.
 * However for special long range branch patch, the label is installed in this case.
 */
uint32 GetJumpTargetIdx(const Insn &insn);

bool IsSub(const Insn &insn);

MOperator GetMopSub2Subs(const Insn &insn);

MOperator FlipConditionOp(MOperator flippedOp);

// Function: for immediate verification, memopnd ofstvalue is returned from opnd input.
// It's worth noting that 0 will be returned when kBOR memopnd is input.
int64 GetMemOpndOffsetValue(Operand *o);

int32 GetTail0BitNum(int64 val);

int32 GetHead0BitNum(int64 val);

inline void GetNextOffsetCalleeSaved(int &offset)
{
    offset += (kAarch64IntregBytelen  << 1);
}
} /* namespace AArch64isa */

/*
 * We save callee-saved registers from lower stack area to upper stack area.
 * If possible, we store a pair of registers (int/int and fp/fp) in the stack.
 * The Stack Pointer has to be aligned at 16-byte boundary.
 * On AArch64, kIntregBytelen == 8 (see the above)
 */

MOperator GetMopPair(MOperator mop, bool isIncludeStrbStrh);
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_ISA_H */
