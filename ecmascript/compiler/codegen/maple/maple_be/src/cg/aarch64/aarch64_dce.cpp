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

#include "aarch64_dce.h"
#include "aarch64_operand.h"
namespace maplebe {
bool AArch64Dce::RemoveUnuseDef(VRegVersion &defVersion)
{
    /* delete defs which have no uses */
    if (defVersion.GetAllUseInsns().empty()) {
        DUInsnInfo *defInsnInfo = defVersion.GetDefInsnInfo();
        if (defInsnInfo == nullptr) {
            return false;
        }
        CHECK_FATAL(defInsnInfo->GetInsn() != nullptr, "Get def insn failed");
        Insn *defInsn = defInsnInfo->GetInsn();
        /* have not support asm/neon opt yet */
        if (defInsn->GetMachineOpcode() == MOP_asm || defInsn->IsVectorOp() || defInsn->IsAtomic()) {
            return false;
        }
        std::set<uint32> defRegs = defInsn->GetDefRegs();
        if (defRegs.size() != 1) {
            return false;
        }
        uint32 bothDUIdx = defInsn->GetBothDefUseOpnd();
        if (!(bothDUIdx != kInsnMaxOpnd && defInsnInfo->GetOperands().count(bothDUIdx))) {
            defInsn->GetBB()->RemoveInsn(*defInsn);
            if (defInsn->IsPhi()) {
                defInsn->GetBB()->RemovePhiInsn(defVersion.GetOriginalRegNO());
            }
            defVersion.MarkDeleted();
            uint32 opndNum = defInsn->GetOperandSize();
            for (uint32 i = opndNum; i > 0; --i) {
                Operand &opnd = defInsn->GetOperand(i - 1);
                A64DeleteRegUseVisitor deleteUseRegVisitor(*GetSSAInfo(), defInsn->GetId());
                opnd.Accept(deleteUseRegVisitor);
            }
            return true;
        }
    }
    return false;
}

void A64DeleteRegUseVisitor::Visit(RegOperand *v)
{
    if (v->IsSSAForm()) {
        VRegVersion *regVersion = GetSSAInfo()->FindSSAVersion(v->GetRegisterNumber());
        MapleUnorderedMap<uint32, DUInsnInfo *> &useInfos = regVersion->GetAllUseInsns();
        auto it = useInfos.find(deleteInsnId);
        if (it != useInfos.end()) {
            useInfos.erase(it);
        }
    }
}
void A64DeleteRegUseVisitor::Visit(ListOperand *v)
{
    for (auto *regOpnd : v->GetOperands()) {
        Visit(regOpnd);
    }
}
void A64DeleteRegUseVisitor::Visit(MemOperand *a64MemOpnd)
{
    RegOperand *baseRegOpnd = a64MemOpnd->GetBaseRegister();
    RegOperand *indexRegOpnd = a64MemOpnd->GetIndexRegister();
    if (baseRegOpnd != nullptr && baseRegOpnd->IsSSAForm()) {
        Visit(baseRegOpnd);
    }
    if (indexRegOpnd != nullptr && indexRegOpnd->IsSSAForm()) {
        Visit(indexRegOpnd);
    }
}

void A64DeleteRegUseVisitor::Visit(PhiOperand *v)
{
    for (auto phiOpndIt : v->GetOperands()) {
        Visit(phiOpndIt.second);
    }
}
}  // namespace maplebe
