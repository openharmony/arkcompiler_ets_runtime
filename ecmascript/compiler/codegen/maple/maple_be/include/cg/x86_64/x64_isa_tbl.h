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

#ifndef MAPLEBE_INCLUDE_CG_X64_X64_ISA_TBL_H
#define MAPLEBE_INCLUDE_CG_X64_X64_ISA_TBL_H

#include "x64_isa.h"
#include "operand.h"

namespace maplebe {

namespace x64 {
/* register,                  imm ,              memory,                   cond */
#define DEF_X64_CMP_MAPPING_INT(SIZE)                                                                        \
    static const X64MOP_t cmpIselMap##SIZE[Operand::OperandType::kOpdPhi][Operand::OperandType::kOpdPhi] = { \
        {MOP_cmp##SIZE##_r_r, MOP_begin, MOP_cmp##SIZE##_r_m, MOP_begin},                                    \
        {MOP_cmp##SIZE##_i_r, MOP_begin, MOP_cmp##SIZE##_i_m, MOP_begin},                                    \
        {MOP_cmp##SIZE##_m_r, MOP_begin, MOP_begin, MOP_begin},                                              \
        {MOP_begin, MOP_begin, MOP_begin, MOP_begin},                                                        \
    }
DEF_X64_CMP_MAPPING_INT(b);
DEF_X64_CMP_MAPPING_INT(w);
DEF_X64_CMP_MAPPING_INT(l);
DEF_X64_CMP_MAPPING_INT(q);

static inline X64MOP_t GetCmpMop(Operand::OperandType dTy, Operand::OperandType sTy, PrimType primType)
{
    X64MOP_t cmpOp = MOP_begin;
    switch (GetPrimTypeBitSize(primType)) {
        case k8BitSize:
            cmpOp = cmpIselMapb[sTy][dTy];
            break;
        case k16BitSize:
            cmpOp = cmpIselMapw[sTy][dTy];
            break;
        case k32BitSize:
            cmpOp = cmpIselMapl[sTy][dTy];
            break;
        case k64BitSize:
            cmpOp = cmpIselMapq[sTy][dTy];
            break;
        default:
            cmpOp = MOP_begin;
            break;
    }
    return cmpOp;
}

/* {OPCODE, {register,             imm ,              memory,                   cond}} */
#define DEF_X64_SET_MAPPING_INT(OPCODE, TYPE)                                        \
    {                                                                                \
        OPCODE,                                                                      \
        {                                                                            \
            x64::MOP_##TYPE##_r, x64::MOP_begin, x64::MOP_##TYPE##_m, x64::MOP_begin \
        }                                                                            \
    }

using SetIselMappingType = std::unordered_map<maple::Opcode, std::array<X64MOP_t, Operand::OperandType::kOpdPhi>>;
static const SetIselMappingType setUnsignedIselMapping = {
    DEF_X64_SET_MAPPING_INT(OP_le, setbe), DEF_X64_SET_MAPPING_INT(OP_ge, setae), DEF_X64_SET_MAPPING_INT(OP_gt, seta),
    DEF_X64_SET_MAPPING_INT(OP_lt, setb),  DEF_X64_SET_MAPPING_INT(OP_ne, setne), DEF_X64_SET_MAPPING_INT(OP_eq, sete),
};
static const SetIselMappingType setSignedIselMapping = {
    DEF_X64_SET_MAPPING_INT(OP_le, setle), DEF_X64_SET_MAPPING_INT(OP_ge, setge), DEF_X64_SET_MAPPING_INT(OP_gt, setg),
    DEF_X64_SET_MAPPING_INT(OP_lt, setl),  DEF_X64_SET_MAPPING_INT(OP_ne, setne), DEF_X64_SET_MAPPING_INT(OP_eq, sete),
};
static const SetIselMappingType setFloatIselMapping = {
    DEF_X64_SET_MAPPING_INT(OP_le, setae), DEF_X64_SET_MAPPING_INT(OP_ge, setae), DEF_X64_SET_MAPPING_INT(OP_gt, seta),
    DEF_X64_SET_MAPPING_INT(OP_lt, seta),  DEF_X64_SET_MAPPING_INT(OP_ne, setne), DEF_X64_SET_MAPPING_INT(OP_eq, sete),
};
#undef DEF_X64_SET_MAPPING_INT

static inline X64MOP_t GetSetCCMop(maple::Opcode opcode, Operand::OperandType dTy, bool isSigned, bool isFloat)
{
    DEBUG_ASSERT(dTy < Operand::OperandType::kOpdPhi, "illegal operand type");
    const SetIselMappingType &setIselMapping =
        isFloat ? setFloatIselMapping : (isSigned ? setSignedIselMapping : setUnsignedIselMapping);
    auto iter = setIselMapping.find(opcode);
    if (iter == setIselMapping.end()) {
        return x64::MOP_begin;
    }
    return iter->second[dTy];
}

#define DEF_X64_CMOV_MAPPING_INT(OPCODE, TYPE)                                                     \
    {                                                                                              \
        OPCODE,                                                                                    \
        {                                                                                          \
            x64::MOP_begin, x64::MOP_##TYPE##w_r_r, x64::MOP_##TYPE##l_r_r, x64::MOP_##TYPE##q_r_r \
        }                                                                                          \
    }
using CMovIselMappingType = std::unordered_map<maple::Opcode, std::array<X64MOP_t, kBitIndexEnd>>;
static const CMovIselMappingType cmovUnsignedIselMapping = {
    DEF_X64_CMOV_MAPPING_INT(OP_le, cmovbe), DEF_X64_CMOV_MAPPING_INT(OP_ge, cmovae),
    DEF_X64_CMOV_MAPPING_INT(OP_gt, cmova),  DEF_X64_CMOV_MAPPING_INT(OP_lt, cmovb),
    DEF_X64_CMOV_MAPPING_INT(OP_ne, cmovne), DEF_X64_CMOV_MAPPING_INT(OP_eq, cmove),
};
static const CMovIselMappingType cmovSignedIselMapping = {
    DEF_X64_CMOV_MAPPING_INT(OP_le, cmovle), DEF_X64_CMOV_MAPPING_INT(OP_ge, cmovge),
    DEF_X64_CMOV_MAPPING_INT(OP_gt, cmovg),  DEF_X64_CMOV_MAPPING_INT(OP_lt, cmovl),
    DEF_X64_CMOV_MAPPING_INT(OP_ne, cmovne), DEF_X64_CMOV_MAPPING_INT(OP_eq, cmove),
};
#undef DEF_X64_CMOV_MAPPING_INT

static inline X64MOP_t GetCMovCCMop(maple::Opcode opcode, int32 bitSize, bool isSigned)
{
    const auto &cmovIselMapping = isSigned ? cmovSignedIselMapping : cmovUnsignedIselMapping;
    auto iter = cmovIselMapping.find(opcode);
    if (iter == cmovIselMapping.end()) {
        return x64::MOP_begin;
    }
    return iter->second[GetBitIndex(bitSize)];
}
}  // namespace x64
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_X64_X64_ISA_TBL_H */
