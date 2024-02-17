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

#include "x64_local_opt.h"
#include "x64_reaching.h"
#include "operand.h"
#include "x64_cg.h"

namespace maplebe {
void X64LocalOpt::DoLocalCopyProp()
{
    LocalOptimizeManager optManager(*cgFunc, *GetRDInfo());
    optManager.Optimize<LocalCopyRegProp>();
    optManager.Optimize<X64RedundantDefRemove>();
}

bool LocalCopyRegProp::CheckCondition(Insn &insn)
{
    MOperator mOp = insn.GetMachineOpcode();
    if (mOp != MOP_movb_r_r && mOp != MOP_movw_r_r && mOp != MOP_movl_r_r && mOp != MOP_movq_r_r) {
        return false;
    }
    DEBUG_ASSERT(insn.GetOperand(kInsnFirstOpnd).IsRegister(), "expects registers");
    DEBUG_ASSERT(insn.GetOperand(kInsnSecondOpnd).IsRegister(), "expects registers");
    auto &regUse = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    auto &regDef = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    if (regUse.GetRegisterNumber() == regDef.GetRegisterNumber()) {
        return false;
    }
    auto &liveOutRegSet = insn.GetBB()->GetLiveOutRegNO();
    if (liveOutRegSet.find(regDef.GetRegisterNumber()) != liveOutRegSet.end()) {
        return false;
    }
    return true;
}

void LocalCopyRegProp::Optimize(BB &bb, Insn &insn)
{
    InsnSet useInsnSet;
    Insn *nextInsn = insn.GetNextMachineInsn();
    if (nextInsn == nullptr) {
        return;
    }
    auto &regDef = static_cast<RegOperand &>(insn.GetOperand(kInsnSecondOpnd));
    reachingDef->FindRegUseBetweenInsn(regDef.GetRegisterNumber(), nextInsn, bb.GetLastInsn(), useInsnSet);
    bool redefined = false;
    auto &replaceOpnd = static_cast<RegOperand &>(insn.GetOperand(kInsnFirstOpnd));
    for (Insn *tInsn : useInsnSet) {
        std::vector<Insn *> defInsnVec =
            reachingDef->FindRegDefBetweenInsn(replaceOpnd.GetRegisterNumber(), &insn, tInsn, false, false);
        if (defInsnVec.size() > 0) {
            redefined = true;
        }
        if (redefined) {
            break;
        }
        propagateOperand(*tInsn, regDef, replaceOpnd);
    }
    return;
}

bool LocalCopyRegProp::propagateOperand(Insn &insn, RegOperand &oldOpnd, RegOperand &replaceOpnd)
{
    bool propagateSuccess = false;
    uint32 opndNum = insn.GetOperandSize();
    const InsnDesc *md = insn.GetDesc();
    if (insn.IsShift() && oldOpnd.GetRegisterNumber() == x64::RCX) {
        return false;
    }
    if (insn.GetMachineOpcode() == MOP_pseudo_ret_int) {
        return false;
    }
    for (uint32 i = 0; i < opndNum; i++) {
        Operand &opnd = insn.GetOperand(i);
        if (opnd.IsList()) {
            /* list operands are used by call,
             * which can not be propagated
             */
            continue;
        }

        auto *regProp = md->opndMD[i];
        if (regProp->IsUse() && !regProp->IsDef() && opnd.IsRegister()) {
            RegOperand &regOpnd = static_cast<RegOperand &>(opnd);
            if (RegOperand::IsSameReg(regOpnd, oldOpnd)) {
                insn.SetOperand(i, replaceOpnd);
                propagateSuccess = true;
            }
        }
    }
    return propagateSuccess;
}

void X64RedundantDefRemove::Optimize(BB &bb, Insn &insn)
{
    const InsnDesc *md = insn.GetDesc();
    RegOperand *regDef = nullptr;
    uint32 opndNum = insn.GetOperandSize();
    for (uint32 i = 0; i < opndNum; ++i) {
        Operand &opnd = insn.GetOperand(i);
        auto *opndDesc = md->opndMD[i];
        if (opndDesc->IsRegDef()) {
            regDef = static_cast<RegOperand *>(&opnd);
        }
    }
    InsnSet useInsnSet;
    Insn *nextInsn = insn.GetNextMachineInsn();
    if (nextInsn == nullptr) {
        return;
    }
    reachingDef->FindRegUseBetweenInsn(regDef->GetRegisterNumber(), nextInsn, bb.GetLastInsn(), useInsnSet);
    if (useInsnSet.size() == 0) {
        bb.RemoveInsn(insn);
        return;
    }
    return;
}
}  // namespace maplebe
