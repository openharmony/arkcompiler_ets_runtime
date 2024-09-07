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

#include "x64_optimize_common.h"
#include "x64_cgfunc.h"
#include "cgbb.h"
#include "cg.h"

namespace maplebe {
void X64InsnVisitor::ModifyJumpTarget(Operand &targetOperand, BB &bb)
{
    Insn *jmpInsn = bb.GetLastInsn();
    jmpInsn->SetOperand(x64::GetJumpTargetIdx(*jmpInsn), targetOperand);
}

void X64InsnVisitor::ModifyJumpTarget(LabelIdx targetLabel, BB &bb)
{
    std::string lableName = ".L." + std::to_string(GetCGFunc()->GetUniqueID()) + "__" + std::to_string(targetLabel);
    ModifyJumpTarget(GetCGFunc()->GetOpndBuilder()->CreateLabel(lableName.c_str(), targetLabel), bb);
}

/*
 * Precondition: The given insn is a jump instruction.
 * Get the jump target label operand index from the given instruction.
 * Note: MOP_jmp_m, MOP_jmp_r is a jump instruction, but the target is unknown at compile time.
 */
LabelIdx X64InsnVisitor::GetJumpLabel(const Insn &insn) const
{
    uint32 operandIdx = x64::GetJumpTargetIdx(insn);
    if (insn.GetOperand(operandIdx).IsLabelOpnd()) {
        return static_cast<LabelOperand &>(insn.GetOperand(operandIdx)).GetLabelIndex();
    }
    DEBUG_ASSERT(false, "Operand is not label");
    return 0;
}
} /* namespace maplebe */
