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

#include "x64_peep.h"
#include "x64_cg.h"

namespace maplebe {
void X64CGPeepHole::Run()
{
    FOR_ALL_BB(bb, cgFunc) {
        FOR_BB_INSNS_SAFE(insn, bb, nextInsn) {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            DoNormalOptimize(*bb, *insn);
        }
    }
}

bool X64RemoveMovingtoSameRegPattern::CheckCondition(Insn &insn)
{
    DEBUG_ASSERT(insn.GetOperand(kInsnFirstOpnd).IsRegister(), "expects registers");
    DEBUG_ASSERT(insn.GetOperand(kInsnSecondOpnd).IsRegister(), "expects registers");
    auto &reg1 = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    auto &reg2 = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    /* remove mov x0,x0 when it cast i32 to i64 */
    if ((reg1.GetRegisterNumber() == reg2.GetRegisterNumber()) && (reg1.GetSize() >= reg2.GetSize())) {
        return true;
    }
    return false;
}

void X64RemoveMovingtoSameRegPattern::Run(BB &bb, Insn &insn)
{
    /* remove mov x0,x0 when it cast i32 to i64 */
    if (CheckCondition(insn)) {
        bb.RemoveInsn(insn);
    }
}

void X64CGPeepHole::DoNormalOptimize(BB &bb, Insn &insn)
{
    MOperator thisMop = insn.GetMachineOpcode();
    manager = peepMemPool->New<PeepOptimizeManager>(*cgFunc, bb, insn);
    switch (thisMop) {
        case MOP_movb_r_r:
        case MOP_movw_r_r:
        case MOP_movl_r_r:
        case MOP_movq_r_r: {
            manager->NormalPatternOpt<X64RemoveMovingtoSameRegPattern>(true);
            break;
        }
        default:
            break;
    }
}
} /* namespace maplebe */
