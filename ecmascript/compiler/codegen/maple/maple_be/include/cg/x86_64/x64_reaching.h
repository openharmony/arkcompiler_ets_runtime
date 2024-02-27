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

#ifndef MAPLEBE_INCLUDE_CG_X64_REACHING_H
#define MAPLEBE_INCLUDE_CG_X64_REACHING_H

#include "reaching.h"

namespace maplebe {
class X64ReachingDefinition : public ReachingDefinition {
public:
    X64ReachingDefinition(CGFunc &func, MemPool &memPool) : ReachingDefinition(func, memPool) {}
    ~X64ReachingDefinition() override = default;
    bool FindRegUseBetweenInsn(uint32 regNO, Insn *startInsn, Insn *endInsn, InsnSet &useInsnSet) const final;
    std::vector<Insn *> FindRegDefBetweenInsnGlobal(uint32 regNO, Insn *startInsn, Insn *endInsn) const final;
    std::vector<Insn *> FindMemDefBetweenInsn(uint32 offset, const Insn *startInsn, Insn *endInsn) const final;
    bool FindRegUseBetweenInsnGlobal(uint32 regNO, Insn *startInsn, Insn *endInsn, BB *movBB) const final;
    bool FindMemUseBetweenInsn(uint32 offset, Insn *startInsn, const Insn *endInsn, InsnSet &useInsnSet) const final;
    bool HasRegDefBetweenInsnGlobal(uint32 regNO, Insn &startInsn, Insn &endInsn);
    bool DFSFindRegDefBetweenBB(const BB &startBB, const BB &endBB, uint32 regNO,
                                std::vector<VisitStatus> &visitedBB) const;
    InsnSet FindDefForRegOpnd(Insn &insn, uint32 indexOrRegNO, bool isRegNO = false) const final;
    InsnSet FindDefForMemOpnd(Insn &insn, uint32 indexOrOffset, bool isOffset = false) const final;
    InsnSet FindUseForMemOpnd(Insn &insn, uint8 index, bool secondMem = false) const final;
    bool FindRegUsingBetweenInsn(uint32 regNO, Insn *startInsn, const Insn *endInsn) const;

protected:
    void InitStartGen() final;
    void InitGenUse(BB &bb, bool firstTime = true) final;
    void GenAllAsmDefRegs(BB &bb, Insn &insn, uint32 index) final;
    void GenAllAsmUseRegs(BB &bb, Insn &insn, uint32 index) final;
    void GenAllCallerSavedRegs(BB &bb, Insn &insn) final;
    bool IsRegKilledByCallInsn(const Insn &insn, regno_t regNO) const final;
    bool KilledByCallBetweenInsnInSameBB(const Insn &startInsn, const Insn &endInsn, regno_t regNO) const final;
    void AddRetPseudoInsn(BB &bb) final;

    void AddRetPseudoInsns() final;
    bool IsCallerSavedReg(uint32 regNO) const final;
    void FindRegDefInBB(uint32 regNO, BB &bb, InsnSet &defInsnSet) const final;
    void FindMemDefInBB(uint32 offset, BB &bb, InsnSet &defInsnSet) const final;
    void DFSFindDefForRegOpnd(const BB &startBB, uint32 regNO, std::vector<VisitStatus> &visitedBB,
                              InsnSet &defInsnSet) const final;
    void DFSFindDefForMemOpnd(const BB &startBB, uint32 offset, std::vector<VisitStatus> &visitedBB,
                              InsnSet &defInsnSet) const final;
    int32 GetStackSize() const final;

private:
    bool IsDiv(const Insn &insn) const;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REACHING_H */
