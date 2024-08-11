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

#ifndef MAPLEBE_INCLUDE_CG_X64_X64_ISA_H
#define MAPLEBE_INCLUDE_CG_X64_X64_ISA_H

#include "operand.h"
#include "isa.h"

namespace maplebe {
/*
 * X64 Architecture Reference Manual
 */
constexpr int kX64StackPtrAlignment = 16;

constexpr int32 kX64OffsetAlign = 8;
constexpr uint32 kX64IntregBytelen = 8; /* 64-bit */
constexpr uint32 kX64FpregBytelen = 8;  /* only lower 64 bits are used */
constexpr int kX64SizeOfFplr = 16;

class Insn;

namespace x64 {
/* machine instruction description */
#define DEFINE_MOP(op, ...) op,
enum X64MOP_t : maple::uint32 {
#include "abstract_mmir.def"
#include "x64_md.def"
    kMopLast
};
#undef DEFINE_MOP

/* Registers in x64 state */
enum X64reg : uint32 {
    kRinvalid = kInvalidRegNO,
/* integer registers */
#define INT_REG(ID, PREF8, PREF8_16, PREF16, PREF32, PREF64, canBeAssigned, isCalleeSave, isParam, isSpill, \
                isExtraSpill)                                                                               \
    R##ID,
#define INT_REG_ALIAS(ALIAS, ID)
#include "x64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
/* fp-simd registers */
#define FP_SIMD_REG(ID, P8, P16, P32, P64, P128, canBeAssigned, isCalleeSave, isParam, isSpill, isExtraSpill) V##ID,
#include "x64_fp_simd_regs.def"
#undef FP_SIMD_REG
    kMaxRegNum,
    kRFLAG,
    kAllRegNum,
/* integer registers alias */
#define INT_REG(ID, PREF8, PREF8_16, PREF16, PREF32, PREF64, canBeAssigned, isCalleeSave, isParam, isSpill, \
                isExtraSpill)
#define INT_REG_ALIAS(ALIAS, ID) R##ALIAS = R##ID,
#include "x64_int_regs.def"
#undef INT_REG
#undef INT_REG_ALIAS
};

static inline bool IsGPRegister(X64reg r)
{
    return R0 <= r && r <= RLAST_GP_REG;
}

static inline bool IsFPSIMDRegister(X64reg r)
{
    return V0 <= r && r <= V15;
}

static inline bool IsFPRegister(X64reg r)
{
    return V0 <= r && r <= V7;
}

static inline bool IsSIMDRegister(X64reg r)
{
    return V8 <= r && r <= V15;
}

static inline bool IsPhysicalRegister(regno_t r)
{
    return r < kMaxRegNum;
}

static inline RegType GetRegType(X64reg r)
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
/*
 * Precondition: The given insn is a jump instruction.
 * Get the jump target label operand index from the given instruction.
 * Note: MOP_jmp_m, MOP_jmp_r is a jump instruction, but the target is unknown at compile time.
 */
uint32 GetJumpTargetIdx(const Insn &insn);

MOperator FlipConditionOp(MOperator flippedOp);
} /* namespace x64 */

/*
 * We save callee-saved registers from lower stack area to upper stack area.
 * If possible, we store a pair of registers (int/int and fp/fp) in the stack.
 * The Stack Pointer has to be aligned at 16-byte boundary.
 * On X64, kX64IntregBytelen == 8 (see the above)
 */
inline void GetNextOffsetCalleeSaved(int &offset)
{
    offset += (kX64IntregBytelen << 1);
}
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_X64_X64_ISA_H */
