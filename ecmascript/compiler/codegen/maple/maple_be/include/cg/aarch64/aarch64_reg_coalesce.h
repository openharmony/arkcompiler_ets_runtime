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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REGCOALESCE_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REGCOALESCE_H
#include "reg_coalesce.h"
#include "aarch64_isa.h"
#include "live.h"

namespace maplebe {
class AArch64LiveIntervalAnalysis : public LiveIntervalAnalysis {
public:
    AArch64LiveIntervalAnalysis(CGFunc &func, MemPool &memPool)
        : LiveIntervalAnalysis(func, memPool), vregLive(alloc.Adapter()), candidates(alloc.Adapter())
    {
    }

    ~AArch64LiveIntervalAnalysis() override = default;

    void ComputeLiveIntervals() override;
    bool IsUnconcernedReg(const RegOperand &regOpnd) const;
    LiveInterval *GetOrCreateLiveInterval(regno_t regNO);
    void UpdateCallInfo();
    void SetupLiveIntervalByOp(Operand &op, Insn &insn, bool isDef);
    void ComputeLiveIntervalsForEachDefOperand(Insn &insn);
    void ComputeLiveIntervalsForEachUseOperand(Insn &insn);
    void SetupLiveIntervalInLiveOut(regno_t liveOut, const BB &bb, uint32 currPoint);
    void CoalesceRegPair(RegOperand &regDest, RegOperand &regSrc);
    void CoalesceRegisters() override;
    void CollectMoveForEachBB(BB &bb, std::vector<Insn *> &movInsns) const;
    void CoalesceMoves(std::vector<Insn *> &movInsns, bool phiOnly);
    void CheckInterference(LiveInterval &li1, LiveInterval &li2) const;
    void CollectCandidate();
    std::string PhaseName() const
    {
        return "regcoalesce";
    }

private:
    static bool IsRegistersCopy(Insn &insn);
    MapleUnorderedSet<regno_t> vregLive;
    MapleSet<regno_t> candidates;
};

} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_REGCOALESCE_H */
