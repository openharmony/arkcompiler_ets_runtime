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

#include "aarch64_optimize_common.h"
#include "aarch64_isa.h"
#include "aarch64_cgfunc.h"
#include "cgbb.h"

namespace maplebe {
void AArch64InsnVisitor::ModifyJumpTarget(Operand &targetOperand, BB &bb)
{
    if (bb.GetKind() == BB::kBBIgoto) {
        bool modified = false;
        for (Insn *insn = bb.GetLastInsn(); insn != nullptr; insn = insn->GetPrev()) {
            if (insn->GetMachineOpcode() == MOP_adrp_label) {
                LabelIdx labIdx = static_cast<LabelOperand &>(targetOperand).GetLabelIndex();
                ImmOperand &immOpnd =
                    static_cast<AArch64CGFunc *>(GetCGFunc())->CreateImmOperand(labIdx, k8BitSize, false);
                insn->SetOperand(1, immOpnd);
                modified = true;
            }
        }
        CHECK_FATAL(modified, "ModifyJumpTarget: Could not change jump target");
        return;
    } else if (bb.GetKind() == BB::kBBGoto) {
        for (Insn *insn = bb.GetLastInsn(); insn != nullptr; insn = insn->GetPrev()) {
            if (insn->GetMachineOpcode() == MOP_adrp_label) {
                maple::LabelIdx labidx = static_cast<LabelOperand &>(targetOperand).GetLabelIndex();
                LabelOperand &label = static_cast<AArch64CGFunc *>(GetCGFunc())->GetOrCreateLabelOperand(labidx);
                insn->SetOperand(1, label);
                break;
            }
        }
        // fallthru below to patch the branch insn
    }
    bb.GetLastInsn()->SetOperand(AArch64isa::GetJumpTargetIdx(*bb.GetLastInsn()), targetOperand);
}

void AArch64InsnVisitor::ModifyJumpTarget(maple::LabelIdx targetLabel, BB &bb)
{
    ModifyJumpTarget(static_cast<AArch64CGFunc *>(GetCGFunc())->GetOrCreateLabelOperand(targetLabel), bb);
}

void AArch64InsnVisitor::ModifyJumpTarget(BB &newTarget, BB &bb)
{
    ModifyJumpTarget(newTarget.GetLastInsn()->GetOperand(AArch64isa::GetJumpTargetIdx(*newTarget.GetLastInsn())), bb);
}

Insn *AArch64InsnVisitor::CloneInsn(Insn &originalInsn)
{
    MemPool *memPool = const_cast<MemPool *>(CG::GetCurCGFunc()->GetMemoryPool());
    if (originalInsn.IsTargetInsn()) {
        if (!originalInsn.IsVectorOp()) {
            return memPool->Clone<Insn>(originalInsn);
        } else {
            auto *insn = memPool->Clone<VectorInsn>(*static_cast<VectorInsn *>(&originalInsn));
            insn->SetRegSpecList(static_cast<VectorInsn &>(originalInsn).GetRegSpecList());
            return insn;
        }
    } else if (originalInsn.IsCfiInsn()) {
        return memPool->Clone<cfi::CfiInsn>(*static_cast<cfi::CfiInsn *>(&originalInsn));
    } else if (originalInsn.IsDbgInsn()) {
        return memPool->Clone<mpldbg::DbgInsn>(*static_cast<mpldbg::DbgInsn *>(&originalInsn));
    }
    if (originalInsn.IsComment()) {
        return memPool->Clone<Insn>(originalInsn);
    }
    CHECK_FATAL(false, "Cannot clone");
    return nullptr;
}

/*
 * Precondition: The given insn is a jump instruction.
 * Get the jump target label from the given instruction.
 * Note: MOP_xbr is a branching instruction, but the target is unknown at compile time,
 * because a register instead of label. So we don't take it as a branching instruction.
 */
LabelIdx AArch64InsnVisitor::GetJumpLabel(const Insn &insn) const
{
    uint32 operandIdx = AArch64isa::GetJumpTargetIdx(insn);
    if (insn.GetOperand(operandIdx).IsLabelOpnd()) {
        return static_cast<LabelOperand &>(insn.GetOperand(operandIdx)).GetLabelIndex();
    }
    DEBUG_ASSERT(false, "Operand is not label");
    return 0;
}

bool AArch64InsnVisitor::IsCompareInsn(const Insn &insn) const
{
    switch (insn.GetMachineOpcode()) {
        case MOP_wcmpri:
        case MOP_wcmprr:
        case MOP_xcmpri:
        case MOP_xcmprr:
        case MOP_hcmperi:
        case MOP_hcmperr:
        case MOP_scmperi:
        case MOP_scmperr:
        case MOP_dcmperi:
        case MOP_dcmperr:
        case MOP_hcmpqri:
        case MOP_hcmpqrr:
        case MOP_scmpqri:
        case MOP_scmpqrr:
        case MOP_dcmpqri:
        case MOP_dcmpqrr:
        case MOP_wcmnri:
        case MOP_wcmnrr:
        case MOP_xcmnri:
        case MOP_xcmnrr:
            return true;
        default:
            return false;
    }
}

bool AArch64InsnVisitor::IsCompareAndBranchInsn(const Insn &insn) const
{
    switch (insn.GetMachineOpcode()) {
        case MOP_wcbnz:
        case MOP_xcbnz:
        case MOP_wcbz:
        case MOP_xcbz:
            return true;
        default:
            return false;
    }
}

bool AArch64InsnVisitor::IsAddOrSubInsn(const Insn &insn) const
{
    switch (insn.GetMachineOpcode()) {
        case MOP_xaddrrr:
        case MOP_xaddrri12:
        case MOP_waddrrr:
        case MOP_waddrri12:
        case MOP_xsubrrr:
        case MOP_xsubrri12:
        case MOP_wsubrrr:
        case MOP_wsubrri12:
            return true;
        default:
            return false;
    }
}

RegOperand *AArch64InsnVisitor::CreateVregFromReg(const RegOperand &pReg)
{
    return &static_cast<AArch64CGFunc *>(GetCGFunc())
                ->CreateRegisterOperandOfType(pReg.GetRegisterType(), pReg.GetSize() / k8BitSize);
}
} /* namespace maplebe */
