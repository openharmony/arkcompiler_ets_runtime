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

#ifndef MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_SCHEDULE_H
#define MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_SCHEDULE_H

#include "schedule.h"
#include "aarch64_operand.h"

namespace maplebe {
enum RegisterType : uint8 {
    kRegisterUndef,
    kRegisterInt,
    kRegisterFloat,
    kRegisterCc,
    kRegisterLast,
};

class AArch64ScheduleProcessInfo : public ScheduleProcessInfo {
public:
    explicit AArch64ScheduleProcessInfo(uint32 size) : ScheduleProcessInfo(size) {}
    ~AArch64ScheduleProcessInfo() override = default;

    /* recover register type which is not recorded in live analysis */
    static RegType GetRegisterType(const CGFunc &f, regno_t regNO);
    void VaryLiveRegSet(const CGFunc &f, regno_t regNO, bool isInc);
    void VaryFreeRegSet(const CGFunc &f, std::set<regno_t> regNOs, DepNode &node);

    uint32 GetFreeIntRegs(DepNode &node)
    {
        return (freeIntRegNodeSet.count(&node)) > 0 ? freeIntRegNodeSet.find(&node)->second : 0;
    }
    void IncFreeIntRegNode(DepNode &node)
    {
        if (freeIntRegNodeSet.count(&node) == 0) {
            freeIntRegNodeSet.emplace(std::pair<DepNode *, uint32>(&node, 1));
        } else {
            freeIntRegNodeSet.find(&node)->second++;
        }
    }
    const std::map<DepNode *, uint32> &GetFreeIntRegNodeSet() const
    {
        return freeIntRegNodeSet;
    }
    void IncFreeFpRegNode(DepNode &node)
    {
        if (freeFpRegNodeSet.count(&node) == 0) {
            freeFpRegNodeSet.emplace(std::pair<DepNode *, uint32>(&node, 1));
        } else {
            freeFpRegNodeSet.find(&node)->second++;
        }
    }
    uint32 GetFreeFpRegs(DepNode &node)
    {
        return (freeFpRegNodeSet.count(&node)) > 0 ? freeFpRegNodeSet.find(&node)->second : 0;
    }
    const std::map<DepNode *, uint32> &GetFreeFpRegNodeSet() const
    {
        return freeFpRegNodeSet;
    }

    void ClearALLFreeRegNodeSet()
    {
        freeIntRegNodeSet.clear();
        freeFpRegNodeSet.clear();
    }

    size_t FindIntLiveReg(regno_t reg) const
    {
        return intLiveRegSet.count(reg);
    }
    void IncIntLiveRegSet(regno_t reg)
    {
        intLiveRegSet.emplace(reg);
    }
    void DecIntLiveRegSet(regno_t reg)
    {
        intLiveRegSet.erase(reg);
    }
    size_t FindFpLiveReg(regno_t reg) const
    {
        return fpLiveRegSet.count(reg);
    }
    void IncFpLiveRegSet(regno_t reg)
    {
        fpLiveRegSet.emplace(reg);
    }
    void DecFpLiveRegSet(regno_t reg)
    {
        fpLiveRegSet.erase(reg);
    }

    size_t SizeOfIntLiveRegSet() const
    {
        return intLiveRegSet.size();
    }

    size_t SizeOfCalleeSaveLiveRegister(bool isInt)
    {
        size_t num = 0;
        if (isInt) {
            for (auto regNO : intLiveRegSet) {
                if (regNO > static_cast<regno_t>(R19)) {
                    num++;
                }
            }
        } else {
            for (auto regNO : fpLiveRegSet) {
                if (regNO > static_cast<regno_t>(V16)) {
                    num++;
                }
            }
        }
        return num;
    }

    size_t SizeOfFpLiveRegSet() const
    {
        return fpLiveRegSet.size();
    }

private:
    std::set<regno_t> intLiveRegSet;
    std::set<regno_t> fpLiveRegSet;
    std::map<DepNode *, uint32> freeIntRegNodeSet;
    std::map<DepNode *, uint32> freeFpRegNodeSet;
};

class AArch64Schedule : public Schedule {
public:
    AArch64Schedule(CGFunc &func, MemPool &memPool, LiveAnalysis &live, const std::string &phaseName)
        : Schedule(func, memPool, live, phaseName)
    {
        intCalleeSaveThreshold = func.UseFP() ? intCalleeSaveThresholdBase : intCalleeSaveThresholdEnhance;
    }
    ~AArch64Schedule() override = default;

protected:
    void DumpDepGraph(const MapleVector<DepNode *> &nodes) const;
    void DumpScheduleResult(const MapleVector<DepNode *> &nodes, SimulateType type) const;
    void GenerateDot(const BB &bb, const MapleVector<DepNode *> &nodes) const;
    void EraseNodeFromNodeList(const DepNode &target, MapleVector<DepNode *> &nodeList) override;
    void FindAndCombineMemoryAccessPair(const std::vector<DepNode *> &memList) override;
    void RegPressureScheduling(BB &bb, MapleVector<DepNode *> &nodes) override;

private:
    enum CSRResult : uint8 {
        kNode1,
        kNode2,
        kDoCSP /* can do csp further */
    };
    void Init() override;
    void MemoryAccessPairOpt() override;
    void ClinitPairOpt() override;
    uint32 DoSchedule() override;
    uint32 DoBruteForceSchedule() override;
    uint32 SimulateOnly() override;
    void UpdateBruteForceSchedCycle() override;
    void IterateBruteForce(DepNode &targetNode, MapleVector<DepNode *> &readyList, uint32 currCycle,
                           MapleVector<DepNode *> &scheduledNodes, uint32 &maxCycleCount,
                           MapleVector<DepNode *> &optimizedScheduledNodes) override;
    bool CanCombine(const Insn &insn) const override;
    void ListScheduling(bool beforeRA) override;
    void BruteForceScheduling(const BB &bb);
    void SimulateScheduling(const BB &bb);
    void FinalizeScheduling(BB &bb, const DepAnalysis &depAnalysis) override;
    uint32 ComputeEstart(uint32 cycle) override;
    void ComputeLstart(uint32 maxEstart) override;
    void UpdateELStartsOnCycle(uint32 cycle) override;
    void RandomTest() override;
    void EraseNodeFromReadyList(const DepNode &target) override;
    uint32 GetNextSepIndex() const override;
    void CountUnitKind(const DepNode &depNode, uint32 array[], const uint32 arraySize) const override;
    static bool IfUseUnitKind(const DepNode &depNode, uint32 index);
    void UpdateReadyList(DepNode &targetNode, MapleVector<DepNode *> &readyList, bool updateEStart) override;
    void UpdateScheduleProcessInfo(AArch64ScheduleProcessInfo &info) const;
    void UpdateAdvanceCycle(AArch64ScheduleProcessInfo &scheduleInfo, const DepNode &targetNode) const;
    bool CheckSchedulable(AArch64ScheduleProcessInfo &info) const;
    void SelectNode(AArch64ScheduleProcessInfo &scheduleInfo);
    static void DumpDebugInfo(const ScheduleProcessInfo &scheduleInfo);
    bool CompareDepNode(DepNode &node1, DepNode &node2, AArch64ScheduleProcessInfo &scheduleInfo) const;
    void CalculateMaxUnitKindCount(ScheduleProcessInfo &scheduleInfo) const;
    void UpdateReleaseRegInfo(AArch64ScheduleProcessInfo &scheduleInfo);
    std::set<regno_t> CanFreeRegister(const DepNode &node) const;
    void UpdateLiveRegSet(AArch64ScheduleProcessInfo &scheduleInfo, const DepNode &node);
    void InitLiveRegSet(AArch64ScheduleProcessInfo &scheduleInfo);
    int CalSeriesCycles(const MapleVector<DepNode *> &nodes) const;
    CSRResult DoCSR(DepNode &node1, DepNode &node2, AArch64ScheduleProcessInfo &scheduleInfo) const;
    AArch64Schedule::CSRResult ScheduleCrossCall(const DepNode &node1, const DepNode &node2) const;
    int intCalleeSaveThreshold = 0;

    static uint32 maxUnitIndex;
    static int intRegPressureThreshold;
    static int fpRegPressureThreshold;
    static int intCalleeSaveThresholdBase;
    static int intCalleeSaveThresholdEnhance;
    static int fpCalleeSaveThreshold;
};
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_AARCH64_AARCH64_SCHEDULE_H */
