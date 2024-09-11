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
#include "aarch64_cg.h"
#include "aarch64_cgfunc.h"
#include "cgbb.h"

namespace maplebe {
void AArch64InsnVisitor::ModifyJumpTarget(Operand &targetOperand, BB &bb)
{
    if (bb.GetKind() == BB::kBBGoto) {
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
    CHECK_NULL_FATAL(bb.GetLastMachineInsn());
    bb.GetLastMachineInsn()->SetOperand(AArch64isa::GetJumpTargetIdx(*bb.GetLastMachineInsn()), targetOperand);
}

void AArch64InsnVisitor::ModifyJumpTarget(maple::LabelIdx targetLabel, BB &bb)
{
    ModifyJumpTarget(static_cast<AArch64CGFunc *>(GetCGFunc())->GetOrCreateLabelOperand(targetLabel), bb);
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
} /* namespace maplebe */
