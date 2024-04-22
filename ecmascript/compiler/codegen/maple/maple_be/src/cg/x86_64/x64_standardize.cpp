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

void X64Standardize::StdzUnaryOp(Insn &insn, CGFunc &cgFunc)
{
    MOperator mOp = insn.GetMachineOpcode();
    if (mOp == abstract::MOP_neg_f_32 || mOp == abstract::MOP_neg_f_64) {
        StdzFloatingNeg(insn, cgFunc);
        return;
    }
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
        case abstract::MOP_zext_rr_64_16:
        case abstract::MOP_zext_rr_64_32:
            destSize = k32BitSize;
            break;
        case abstract::MOP_cvt_f32_u32:
            srcSize = k64BitSize;
            break;
        case abstract::MOP_cvt_u32_f32:
            destSize = k64BitSize;
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

/* x86 does not have floating point neg instruction
 * neg_f   operand0  operand1
 * ==>
 * movd    xmm0 R1
 * 64: movabsq 0x8000000000000000 R2
 *     xorq R2 R1
 * 32: xorl 0x80000000 R1
 * movd R1 xmm0
*/
void X64Standardize::StdzFloatingNeg(Insn &insn, CGFunc &cgFunc)
{
    MOperator mOp = insn.GetMachineOpcode();
    uint32 bitSize = mOp == abstract::MOP_neg_f_32 ? k32BitSize : k64BitSize;

    // mov dest -> tmpOperand0
    MOperator movOp = mOp == abstract::MOP_neg_f_32 ? x64::MOP_movd_fr_r : x64::MOP_movq_fr_r;
    RegOperand *tmpOperand0 = &cgFunc.GetOpndBuilder()->CreateVReg(bitSize, kRegTyInt);
    Insn &movInsn0 = cgFunc.GetInsnBuilder()->BuildInsn(movOp, X64CG::kMd[movOp]);
    Operand &dest = insn.GetOperand(kInsnFirstOpnd);
    movInsn0.AddOpndChain(dest).AddOpndChain(*tmpOperand0);
    insn.GetBB()->InsertInsnBefore(insn, movInsn0);

    // 32 : xorl   0x80000000         tmpOperand0
    // 64 : movabs 0x8000000000000000 tmpOperand1
    //      xorq   tmpOperand1        tmpOperand0
    ImmOperand &imm = cgFunc.GetOpndBuilder()->CreateImm(bitSize, (static_cast<int64>(1) << (bitSize - 1)));
    if (mOp == abstract::MOP_neg_f_64) {
        Operand *tmpOperand1 = &cgFunc.GetOpndBuilder()->CreateVReg(k64BitSize, kRegTyInt);
        Insn &movabs = cgFunc.GetInsnBuilder()->BuildInsn(x64::MOP_movabs_i_r, X64CG::kMd[x64::MOP_movabs_i_r]);
        movabs.AddOpndChain(imm).AddOpndChain(*tmpOperand1);
        insn.GetBB()->InsertInsnBefore(insn, movabs);

        MOperator xorOp = x64::MOP_xorq_r_r;
        Insn &xorq = cgFunc.GetInsnBuilder()->BuildInsn(xorOp, X64CG::kMd[xorOp]);
        xorq.AddOpndChain(*tmpOperand1).AddOpndChain(*tmpOperand0);
        insn.GetBB()->InsertInsnBefore(insn, xorq);
    } else {
        MOperator xorOp = x64::MOP_xorl_i_r;
        Insn &xorq = cgFunc.GetInsnBuilder()->BuildInsn(xorOp, X64CG::kMd[xorOp]);
        xorq.AddOpndChain(imm).AddOpndChain(*tmpOperand0);
        insn.GetBB()->InsertInsnBefore(insn, xorq);
    }

    // mov tmpOperand0 -> dest
    Insn &movq = cgFunc.GetInsnBuilder()->BuildInsn(movOp, X64CG::kMd[movOp]);
    movq.AddOpndChain(*tmpOperand0).AddOpndChain(dest);
    insn.GetBB()->InsertInsnBefore(insn, movq);

    insn.GetBB()->RemoveInsn(insn);
    return;
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
