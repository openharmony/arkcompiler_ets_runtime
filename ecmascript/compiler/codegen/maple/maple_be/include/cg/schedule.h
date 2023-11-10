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

#ifndef MAPLEBE_INCLUDE_CG_SCHEDULE_H
#define MAPLEBE_INCLUDE_CG_SCHEDULE_H

#include "insn.h"
#include "mad.h"
#include "dependence.h"
#include "live.h"

namespace maplebe {
#define LIST_SCHED_DUMP_NEWPM CG_DEBUG_FUNC(f)
#define LIST_SCHED_DUMP_REF CG_DEBUG_FUNC(cgFunc)

class RegPressureSchedule {
public:
    RegPressureSchedule(CGFunc &func, MapleAllocator &alloc)
        : cgFunc(func),
          liveReg(alloc.Adapter()),
          scheduledNode(alloc.Adapter()),
          originalNodeSeries(alloc.Adapter()),
          readyList(alloc.Adapter()),
          partialList(alloc.Adapter()),
          partialSet(alloc.Adapter()),
          partialScheduledNode(alloc.Adapter()),
          optimisticScheduledNodes(alloc.Adapter()),
          splitterIndexes(alloc.Adapter()),
          liveInRegNO(alloc.Adapter()),
          liveOutRegNO(alloc.Adapter())
    {
    }
    virtual ~RegPressureSchedule() = default;

    void InitBBInfo(BB &b, MemPool &memPool, const MapleVector<DepNode *> &nodes);
    void BuildPhyRegInfo(const std::vector<int32> &regNumVec);
    void initPartialSplitters(const MapleVector<DepNode *> &nodes);
    void Init(const MapleVector<DepNode *> &nodes);
    void UpdateBBPressure(const DepNode &node);
    void CalculatePressure(DepNode &node, regno_t reg, bool def);
    void SortReadyList();
    static bool IsLastUse(const DepNode &node, regno_t regNO);
    void ReCalculateDepNodePressure(DepNode &node);
    void UpdateLiveReg(const DepNode &node, regno_t reg, bool def);
    bool CanSchedule(const DepNode &node) const;
    void UpdateReadyList(const DepNode &node);
    void BruteUpdateReadyList(const DepNode &node, std::vector<bool> &changedToReady);
    void RestoreReadyList(DepNode &node, std::vector<bool> &changedToReady);
    void UpdatePriority(DepNode &node);
    void CalculateMaxDepth(const MapleVector<DepNode *> &nodes);
    void CalculateNear(const DepNode &node);
    static bool DepNodePriorityCmp(const DepNode *node1, const DepNode *node2);
    DepNode *ChooseNode();
    void DoScheduling(MapleVector<DepNode *> &nodes);
    void HeuristicScheduling(MapleVector<DepNode *> &nodes);
    int CalculateRegisterPressure(MapleVector<DepNode *> &nodes);
    void PartialScheduling(MapleVector<DepNode *> &nodes);
    void BruteForceScheduling();
    void CalculatePredSize(DepNode &node);
    void InitBruteForceScheduling(MapleVector<DepNode *> &nodes);
    void EmitSchedulingSeries(MapleVector<DepNode *> &nodes);

private:
    void DumpBBPressureInfo() const;
    void DumpBBLiveInfo() const;
    void DumpReadyList() const;
    void DumpSelectInfo(const DepNode &node) const;
    static void DumpDependencyInfo(const MapleVector<DepNode *> &nodes);
    void ReportScheduleError() const;
    void ReportScheduleOutput() const;
    RegType GetRegisterType(regno_t reg) const;

    CGFunc &cgFunc;
    BB *bb = nullptr;
    int32 *maxPressure = nullptr;
    int32 *curPressure = nullptr;
    MapleUnorderedSet<regno_t> liveReg;
    /* save node that has been scheduled. */
    MapleVector<DepNode *> scheduledNode;
    MapleVector<DepNode *> originalNodeSeries;
    MapleVector<DepNode *> readyList;
    /* save partial nodes to be scheduled */
    MapleVector<DepNode *> partialList;
    MapleSet<DepNode *> partialSet;
    /* save partial nodes which have been scheduled. */
    MapleVector<DepNode *> partialScheduledNode;
    /* optimistic schedule series with minimum register pressure */
    MapleVector<DepNode *> optimisticScheduledNodes;
    /* save split points */
    MapleVector<int> splitterIndexes;
    /* save integer register pressure */
    std::vector<int> integerRegisterPressureList;
    /* save the amount of every type register. */
    int32 *physicalRegNum = nullptr;
    int32 maxPriority = 0;
    int32 scheduleSeriesCount = 0;
    /* live in register set */
    MapleSet<regno_t> liveInRegNO;
    /* live out register set */
    MapleSet<regno_t> liveOutRegNO;
    /* register pressure without pre-scheduling */
    int originalPressure = 0;
    /* register pressure after pre-scheduling */
    int scheduledPressure = 0;
    /* minimum pressure ever met */
    int minPressure = -1;
};

enum SimulateType : uint8 { kListSchedule, kBruteForce, kSimulateOnly };

class Schedule {
public:
    Schedule(CGFunc &func, MemPool &memPool, LiveAnalysis &liveAnalysis, const std::string &phase)
        : phaseName(phase),
          cgFunc(func),
          memPool(memPool),
          alloc(&memPool),
          live(liveAnalysis),
          considerRegPressure(false),
          nodes(alloc.Adapter()),
          readyList(alloc.Adapter()),
          liveInRegNo(alloc.Adapter()),
          liveOutRegNo(alloc.Adapter())
    {
    }

    virtual ~Schedule() = default;
    virtual void MemoryAccessPairOpt() = 0;
    virtual void ClinitPairOpt() = 0;
    virtual uint32 DoSchedule() = 0;
    virtual uint32 DoBruteForceSchedule() = 0;
    virtual uint32 SimulateOnly() = 0;
    virtual void UpdateBruteForceSchedCycle() = 0;
    virtual void IterateBruteForce(DepNode &targetNode, MapleVector<DepNode *> &readyList, uint32 currCycle,
                                   MapleVector<DepNode *> &scheduledNodes, uint32 &maxCycleCount,
                                   MapleVector<DepNode *> &optimizedScheduledNodes) = 0;
    virtual void UpdateReadyList(DepNode &targetNode, MapleVector<DepNode *> &readyList, bool updateEStart) = 0;
    virtual void ListScheduling(bool beforeRA) = 0;
    virtual void FinalizeScheduling(BB &bb, const DepAnalysis &depAnalysis) = 0;

protected:
    virtual void Init() = 0;
    virtual uint32 ComputeEstart(uint32 cycle) = 0;
    virtual void ComputeLstart(uint32 maxEstart) = 0;
    virtual void UpdateELStartsOnCycle(uint32 cycle) = 0;
    virtual void RandomTest() = 0;
    virtual void EraseNodeFromReadyList(const DepNode &target) = 0;
    virtual void EraseNodeFromNodeList(const DepNode &target, MapleVector<DepNode *> &nodeList) = 0;
    virtual uint32 GetNextSepIndex() const = 0;
    virtual void CountUnitKind(const DepNode &depNode, uint32 array[], const uint32 arraySize) const = 0;
    virtual void FindAndCombineMemoryAccessPair(const std::vector<DepNode *> &memList) = 0;
    virtual void RegPressureScheduling(BB &bb, MapleVector<DepNode *> &nodes) = 0;
    virtual bool CanCombine(const Insn &insn) const = 0;
    void SetConsiderRegPressure()
    {
        considerRegPressure = true;
    }
    bool GetConsiderRegPressure() const
    {
        return considerRegPressure;
    }
    void InitIDAndLoc();
    void RestoreFirstLoc();
    std::string PhaseName() const
    {
        return phaseName;
    }

    const std::string phaseName;
    CGFunc &cgFunc;
    MemPool &memPool;
    MapleAllocator alloc;
    LiveAnalysis &live;
    DepAnalysis *depAnalysis = nullptr;
    MAD *mad = nullptr;
    uint32 lastSeparatorIndex = 0;
    uint32 nodeSize = 0;
    bool considerRegPressure = false;
    MapleVector<DepNode *> nodes;     /* Dependence graph */
    MapleVector<DepNode *> readyList; /* Ready list. */
    MapleSet<regno_t> liveInRegNo;
    MapleSet<regno_t> liveOutRegNo;
};

MAPLE_FUNC_PHASE_DECLARE(CgPreScheduling, maplebe::CGFunc)
MAPLE_FUNC_PHASE_DECLARE(CgScheduling, maplebe::CGFunc)
} /* namespace maplebe */

#endif /* MAPLEBE_INCLUDE_CG_SCHEDULE_H */
