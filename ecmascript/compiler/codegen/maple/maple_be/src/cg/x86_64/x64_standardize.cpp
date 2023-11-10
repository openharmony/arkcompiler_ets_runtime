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

#include "x64_standardize.h"
#include "x64_isa.h"
#include "x64_cg.h"
#include "insn.h"

namespace maplebe {
#define DEFINE_MAPPING(ABSTRACT_IR, X64_MOP, ...) {ABSTRACT_IR, X64_MOP},
std::unordered_map<MOperator, X64MOP_t> x64AbstractMapping = {
#include "x64_abstract_mapping.def"
};

static inline X64MOP_t GetMopFromAbstraceIRMop(MOperator mOp)
{
    auto iter = x64AbstractMapping.find(mOp);
    if (iter == x64AbstractMapping.end()) {
        CHECK_FATAL(false, "NIY mapping");
    }
    CHECK_FATAL(iter->second != x64::MOP_begin, "NIY mapping");
    return iter->second;
}

void X64Standardize::StdzMov(maplebe::Insn &insn)
{
    X64MOP_t directlyMappingMop = GetMopFromAbstraceIRMop(insn.GetMachineOpcode());
    insn.SetMOP(X64CG::kMd[directlyMappingMop]);
    insn.CommuteOperands(kInsnFirstOpnd, kInsnSecondOpnd);
}

void X64Standardize::StdzStrLdr(Insn &insn)
{
    StdzMov(insn);
}

void X64Standardize::StdzBasicOp(Insn &insn)
{
    X64MOP_t directlyMappingMop = GetMopFromAbstraceIRMop(insn.GetMachineOpcode());
    insn.SetMOP(X64CG::kMd[directlyMappingMop]);
    Operand &dest = insn.GetOperand(kInsnFirstOpnd);
    Operand &src2 = insn.GetOperand(kInsnThirdOpnd);
    insn.CleanAllOperand();
    insn.AddOpndChain(src2).AddOpndChain(dest);
}

void X64Standardize::StdzUnaryOp(Insn &insn)
{
    X64MOP_t directlyMappingMop = GetMopFromAbstraceIRMop(insn.GetMachineOpcode());
    insn.SetMOP(X64CG::kMd[directlyMappingMop]);
    Operand &dest = insn.GetOperand(kInsnFirstOpnd);
    insn.CleanAllOperand();
    insn.AddOpndChain(dest);
}

void X64Standardize::StdzCvtOp(Insn &insn, CGFunc &cgFunc)
{
    uint32 OpndDesSize = insn.GetDesc()->GetOpndDes(kInsnFirstOpnd)->GetSize();
    uint32 destSize = OpndDesSize;
    uint32 OpndSrcSize = insn.GetDesc()->GetOpndDes(kInsnSecondOpnd)->GetSize();
    uint32 srcSize = OpndSrcSize;
    switch (insn.GetMachineOpcode()) {
        case abstract::MOP_zext_rr_64_8:
            destSize = k32BitSize;
            break;
        case abstract::MOP_zext_rr_64_16:
            destSize = k32BitSize;
            break;
        case abstract::MOP_zext_rr_64_32:
            destSize = k32BitSize;
            break;
        default:
            break;
    }
    MOperator directlyMappingMop = GetMopFromAbstraceIRMop(insn.GetMachineOpcode());
    if (directlyMappingMop != abstract::MOP_undef) {
        insn.SetMOP(X64CG::kMd[directlyMappingMop]);
        Operand *opnd0 = &insn.GetOperand(kInsnSecondOpnd);
        RegOperand *src = static_cast<RegOperand *>(opnd0);
        if (srcSize != OpndSrcSize) {
            src = &cgFunc.GetOpndBuilder()->CreateVReg(src->GetRegisterNumber(), srcSize, src->GetRegisterType());
        }
        Operand *opnd1 = &insn.GetOperand(kInsnFirstOpnd);
        RegOperand *dest = static_cast<RegOperand *>(opnd1);
        if (destSize != OpndDesSize) {
            dest = &cgFunc.GetOpndBuilder()->CreateVReg(dest->GetRegisterNumber(), destSize, dest->GetRegisterType());
        }
        insn.CleanAllOperand();
        insn.AddOpndChain(*src).AddOpndChain(*dest);
    } else {
        CHECK_FATAL(false, "NIY mapping");
    }
}

void X64Standardize::StdzShiftOp(Insn &insn, CGFunc &cgFunc)
{
    RegOperand *countOpnd = static_cast<RegOperand *>(&insn.GetOperand(kInsnThirdOpnd));
    /* count operand cvt -> PTY_u8 */
    if (countOpnd->GetSize() != GetPrimTypeBitSize(PTY_u8)) {
        countOpnd = &cgFunc.GetOpndBuilder()->CreateVReg(countOpnd->GetRegisterNumber(), GetPrimTypeBitSize(PTY_u8),
                                                         countOpnd->GetRegisterType());
    }
    /* copy count operand to cl(rcx) register */
    RegOperand &clOpnd = cgFunc.GetOpndBuilder()->CreatePReg(x64::RCX, GetPrimTypeBitSize(PTY_u8), kRegTyInt);
    X64MOP_t copyMop = x64::MOP_movb_r_r;
    Insn &copyInsn = cgFunc.GetInsnBuilder()->BuildInsn(copyMop, X64CG::kMd[copyMop]);
    copyInsn.AddOpndChain(*countOpnd).AddOpndChain(clOpnd);
    insn.GetBB()->InsertInsnBefore(insn, copyInsn);
    /* shift OP */
    X64MOP_t directlyMappingMop = GetMopFromAbstraceIRMop(insn.GetMachineOpcode());
    insn.SetMOP(X64CG::kMd[directlyMappingMop]);
    RegOperand &destOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    insn.CleanAllOperand();
    insn.AddOpndChain(clOpnd).AddOpndChain(destOpnd);
}

}  // namespace maplebe
