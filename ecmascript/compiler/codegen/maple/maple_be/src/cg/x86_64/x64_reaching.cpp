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

#include "x64_reaching.h"
#include "x64_cg.h"
#include "insn.h"
#include "isa.h"
namespace maplebe {
/* find insn using register between startInsn and endInsn
 * startInsn and endInsn must be in the same BB.
 */
bool X64ReachingDefinition::FindRegUseBetweenInsn(uint32 regNO, Insn *startInsn, Insn *endInsn,
                                                  InsnSet &regUseInsnSet) const
{
    DEBUG_ASSERT(startInsn->GetBB() == endInsn->GetBB(), "two insns must be in a same BB");
    bool findFinish = false;
    if (startInsn == nullptr || endInsn == nullptr) {
        return findFinish;
    }
    for (Insn *insn = startInsn; insn != nullptr && insn != endInsn->GetNext(); insn = insn->GetNext()) {
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        /* if insn is call and regNO is caller-saved register, then regNO will not be used later */
        if (insn->IsCall() && IsRegKilledByCallInsn(*insn, regNO)) {
            findFinish = true;
        }

        if (IsDiv(*insn) && regNO == x64::RAX) {
            /* div insn use rax implicitly */
            findFinish = true;
        }

        const InsnDesc *md = insn->GetDesc();
        uint32 opndNum = insn->GetOperandSize();
        for (uint32 i = 0; i < opndNum; ++i) {
            Operand &opnd = insn->GetOperand(i);
            /* handle def or def use */
            auto *regProp = md->opndMD[i];
            if (regProp->IsDef() && opnd.IsRegister() &&
                (static_cast<RegOperand &>(opnd).GetRegisterNumber() == regNO)) {
                findFinish = true;
            }

            if (opnd.IsList()) {
                auto &listOpnd = static_cast<ListOperand &>(opnd);
                for (auto listElem : listOpnd.GetOperands()) {
                    RegOperand *regOpnd = static_cast<RegOperand *>(listElem);
                    DEBUG_ASSERT(regOpnd != nullptr, "parameter operand must be RegOperand");
                    if (regNO == regOpnd->GetRegisterNumber()) {
                        (void)regUseInsnSet.insert(insn);
                    }
                }
                continue;
            }
            if (!regProp->IsUse() && !opnd.IsMemoryAccessOperand()) {
                continue;
            }

            /* handle use */
            if (opnd.IsMemoryAccessOperand()) {
                auto &memOpnd = static_cast<MemOperand &>(opnd);
                RegOperand *base = memOpnd.GetBaseRegister();
                RegOperand *index = memOpnd.GetIndexRegister();
                if ((base != nullptr && base->GetRegisterNumber() == regNO) ||
                    (index != nullptr && index->GetRegisterNumber() == regNO)) {
                    (void)regUseInsnSet.insert(insn);
                }
            } else if (opnd.IsConditionCode()) {
                Operand &rflagOpnd = cgFunc->GetOrCreateRflag();
                RegOperand &rflagReg = static_cast<RegOperand &>(rflagOpnd);
                if (rflagReg.GetRegisterNumber() == regNO) {
                    (void)regUseInsnSet.insert(insn);
                }
            } else if (opnd.IsRegister() && (static_cast<RegOperand &>(opnd).GetRegisterNumber() == regNO)) {
                (void)regUseInsnSet.insert(insn);
            }
        }
        if (findFinish) {
            break;
        }
    }
    return findFinish;
}

std::vector<Insn *> X64ReachingDefinition::FindRegDefBetweenInsnGlobal(uint32 regNO, Insn *startInsn,
                                                                       Insn *endInsn) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return {};
}

std::vector<Insn *> X64ReachingDefinition::FindMemDefBetweenInsn(uint32 offset, const Insn *startInsn,
                                                                 Insn *endInsn) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return {};
}

bool X64ReachingDefinition::FindRegUseBetweenInsnGlobal(uint32 regNO, Insn *startInsn, Insn *endInsn, BB *movBB) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return false;
}

bool X64ReachingDefinition::FindMemUseBetweenInsn(uint32 offset, Insn *startInsn, const Insn *endInsn,
                                                  InsnSet &useInsnSet) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return false;
}

bool X64ReachingDefinition::HasRegDefBetweenInsnGlobal(uint32 regNO, Insn &startInsn, Insn &endInsn)
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return false;
}

bool X64ReachingDefinition::DFSFindRegDefBetweenBB(const BB &startBB, const BB &endBB, uint32 regNO,
                                                   std::vector<VisitStatus> &visitedBB) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return false;
}

InsnSet X64ReachingDefinition::FindDefForRegOpnd(Insn &insn, uint32 indexOrRegNO, bool isRegNO) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return {};
}

InsnSet X64ReachingDefinition::FindDefForMemOpnd(Insn &insn, uint32 indexOrOffset, bool isOffset) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return {};
}

InsnSet X64ReachingDefinition::FindUseForMemOpnd(Insn &insn, uint8 index, bool secondMem) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return {};
}

bool X64ReachingDefinition::FindRegUsingBetweenInsn(uint32 regNO, Insn *startInsn, const Insn *endInsn) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return false;
}

void X64ReachingDefinition::InitStartGen()
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return;
}

void X64ReachingDefinition::InitGenUse(BB &bb, bool firstTime)
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return;
}

void X64ReachingDefinition::GenAllAsmDefRegs(BB &bb, Insn &insn, uint32 index)
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return;
}

void X64ReachingDefinition::GenAllAsmUseRegs(BB &bb, Insn &insn, uint32 index)
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return;
}

void X64ReachingDefinition::GenAllCallerSavedRegs(BB &bb, Insn &insn)
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return;
}

bool X64ReachingDefinition::KilledByCallBetweenInsnInSameBB(const Insn &startInsn, const Insn &endInsn,
                                                            regno_t regNO) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return false;
}

void X64ReachingDefinition::AddRetPseudoInsn(BB &bb)
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return;
}

bool X64ReachingDefinition::IsCallerSavedReg(uint32 regNO) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return false;
}

void X64ReachingDefinition::FindRegDefInBB(uint32 regNO, BB &bb, InsnSet &defInsnSet) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return;
}

void X64ReachingDefinition::FindMemDefInBB(uint32 offset, BB &bb, InsnSet &defInsnSet) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return;
}

void X64ReachingDefinition::DFSFindDefForRegOpnd(const BB &startBB, uint32 regNO, std::vector<VisitStatus> &visitedBB,
                                                 InsnSet &defInsnSet) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return;
}

void X64ReachingDefinition::DFSFindDefForMemOpnd(const BB &startBB, uint32 offset, std::vector<VisitStatus> &visitedBB,
                                                 InsnSet &defInsnSet) const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return;
}

int32 X64ReachingDefinition::GetStackSize() const
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return 0;
};

void X64ReachingDefinition::AddRetPseudoInsns()
{
    CHECK_FATAL(false, "x64_reaching analysis not implemented yet!");
    return;
};

/* reg killed killed by call insn */
bool X64ReachingDefinition::IsRegKilledByCallInsn(const Insn &insn, regno_t regNO) const
{
    return !x64::IsCalleeSavedReg(static_cast<X64reg>(regNO));
}

bool X64ReachingDefinition::IsDiv(const Insn &insn) const
{
    MOperator mOp = insn.GetMachineOpcode();
    return (MOP_idivw_r <= mOp && mOp <= MOP_divq_m);
}

} /* namespace maplebe */
