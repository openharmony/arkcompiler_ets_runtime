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

#include "standardize.h"
namespace maplebe {

void Standardize::DoStandardize()
{
    /* two address mapping first */
    FOR_ALL_BB(bb, cgFunc) {
        FOR_BB_INSNS(insn, bb) {
            if (insn->IsMachineInstruction()) {
                continue;
            }
            if (NeedAddressMapping(*insn)) {
                AddressMapping(*insn);
            }
        }
    }

    /* standardize for each op */
    FOR_ALL_BB(bb, cgFunc) {
        FOR_BB_INSNS(insn, bb) {
            if (insn->IsMachineInstruction()) {
                continue;
            }
            if (insn->IsMove()) {
                StdzMov(*insn);
            } else if (insn->IsStore() || insn->IsLoad()) {
                StdzStrLdr(*insn);
            } else if (insn->IsBasicOp()) {
                StdzBasicOp(*insn);
            } else if (insn->IsUnaryOp()) {
                StdzUnaryOp(*insn, *cgFunc);
            } else if (insn->IsConversion()) {
                StdzCvtOp(*insn, *cgFunc);
            } else if (insn->IsShift()) {
                StdzShiftOp(*insn, *cgFunc);
            } else {
                LogInfo::MapleLogger() << "Need STDZ function for " << insn->GetDesc()->GetName() << "\n";
                CHECK_FATAL(false, "NIY");
            }
        }
    }
}

void Standardize::AddressMapping(Insn &insn)
{
    Operand &dest = insn.GetOperand(kInsnFirstOpnd);
    Operand &src1 = insn.GetOperand(kInsnSecondOpnd);
    uint32 destSize = dest.GetSize();
    bool isInt = (static_cast<RegOperand&>(dest).GetRegisterType() == kRegTyInt);
    MOperator mOp = abstract::MOP_undef;
    switch (destSize) {
        case k8BitSize:
            mOp = isInt ? abstract::MOP_copy_rr_8 : abstract::MOP_undef;
            break;
        case k16BitSize:
            mOp = isInt ? abstract::MOP_copy_rr_16 : abstract::MOP_undef;
            break;
        case k32BitSize:
            mOp = isInt ? abstract::MOP_copy_rr_32 : abstract::MOP_copy_ff_32;
            break;
        case k64BitSize:
            mOp = isInt ? abstract::MOP_copy_rr_64 : abstract::MOP_copy_ff_64;
            break;
        default:
            break;
    }
    CHECK_FATAL(mOp != abstract::MOP_undef, "do two address mapping failed");
    Insn &newInsn = cgFunc->GetInsnBuilder()->BuildInsn(mOp, InsnDesc::GetAbstractId(mOp));
    (void)newInsn.AddOpndChain(dest).AddOpndChain(src1);
    (void)insn.GetBB()->InsertInsnBefore(insn, newInsn);
}
}  // namespace maplebe
