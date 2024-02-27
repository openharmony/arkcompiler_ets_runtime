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

#if TARGAARCH64
#include "aarch64_schedule.h"
#elif defined(TARGRISCV64) && TARGRISCV64
#include "riscv64_schedule.h"
#endif
#if defined(TARGARM32) && TARGARM32
#include "arm32_schedule.h"
#endif
#include "cg.h"
#include "optimize_common.h"

#undef PRESCHED_DEBUG

namespace maplebe {
/* pressure standard value; pressure under this value will not lead to spill operation */
static constexpr int PRESSURE_STANDARD = 27;
/* optimistic scheduling option */
static constexpr bool OPTIMISTIC_SCHEDULING = false;
/* brute maximum count limit option */
static constexpr bool BRUTE_MAXIMUM_LIMIT = true;
/* brute maximum count */
static constexpr int SCHEDULING_MAXIMUM_COUNT = 20000;

/* ---- RegPressureSchedule function ---- */
void RegPressureSchedule::InitBBInfo(BB &b, MemPool &memPool, const MapleVector<DepNode *> &nodes)
{
    bb = &b;
    liveReg.clear();
    scheduledNode.clear();
    readyList.clear();
    maxPriority = 0;
    maxPressure = memPool.NewArray<int32>(RegPressure::GetMaxRegClassNum());
    curPressure = memPool.NewArray<int32>(RegPressure::GetMaxRegClassNum());
    physicalRegNum = memPool.NewArray<int32>(RegPressure::GetMaxRegClassNum());
    for (auto node : nodes) {
        node->SetState(kNormal);
    }
}

/* return register type according to register number */
RegType RegPressureSchedule::GetRegisterType(regno_t reg) const
{
    return cgFunc.GetRegisterType(reg);
}

/* Get amount of every physical register */
void RegPressureSchedule::BuildPhyRegInfo(const std::vector<int32> &regNumVec) const
{
    FOR_ALL_REGCLASS(i)
    {
        physicalRegNum[i] = regNumVec[i];
    }
}

/* Initialize pre-scheduling split point in BB */
void RegPressureSchedule::InitPartialSplitters(const MapleVector<DepNode *> &nodes)
{
    bool addFirstAndLastNodeIndex = false;
    constexpr uint32 kSecondLastNodeIndexFromBack = 2;
    constexpr uint32 kLastNodeIndexFromBack = 1;
    constexpr uint32 kFirstNodeIndex = 0;
    constexpr uint32 kMinimumBBSize = 2;
    /* Add split point for the last instruction in return BB */
    if (bb->GetKind() == BB::kBBReturn && nodes.size() > kMinimumBBSize) {
        splitterIndexes.emplace_back(nodes.size() - kSecondLastNodeIndexFromBack);
        addFirstAndLastNodeIndex = true;
    }
    /* Add first and last node as split point if needed */
    if (addFirstAndLastNodeIndex) {
        splitterIndexes.emplace_back(nodes.size() - kLastNodeIndexFromBack);
        splitterIndexes.emplace_back(kFirstNodeIndex);
    }
    std::sort(splitterIndexes.begin(), splitterIndexes.end(), std::less<int> {});
}

/* initialize register pressure information according to bb's live-in data.
 * initialize node's valid preds size.
 */
void RegPressureSchedule::Init(const MapleVector<DepNode *> &nodes)
{
    readyList.clear();
    scheduledNode.clear();
    liveReg.clear();
    liveInRegNO.clear();
    liveOutRegNO.clear();
    liveInRegNO = bb->GetLiveInRegNO();
    liveOutRegNO = bb->GetLiveOutRegNO();

    FOR_ALL_REGCLASS(i)
    {
        curPressure[i] = 0;
        maxPressure[i] = 0;
    }

    for (auto *node : nodes) {
        /* calculate the node uses'register pressure */
        for (auto &useReg : node->GetUseRegnos()) {
            CalculatePressure(*node, useReg, false);
        }

        /* calculate the node defs'register pressure */
        size_t i = 0;
        for (auto &defReg : node->GetDefRegnos()) {
            CalculatePressure(*node, defReg, true);
            RegType regType = GetRegisterType(defReg);
            /* if no use list, a register is only defined, not be used */
            if (node->GetRegDefs(i) == nullptr && liveOutRegNO.find(defReg) == liveOutRegNO.end()) {
                node->IncDeadDefByIndex(regType);
            }
            ++i;
        }
        /* Calculate pred size of the node */
        CalculatePredSize(*node);
    }

    DepNode *firstNode = nodes.front();
    readyList.emplace_back(firstNode);
    firstNode->SetState(kReady);
    scheduledNode.reserve(nodes.size());
    constexpr size_t readyListSize = 10;
    readyList.reserve(readyListSize);
}

void RegPressureSchedule::SortReadyList()
{
    std::sort(readyList.begin(), readyList.end(), DepNodePriorityCmp);
}

/* return true if nodes1 first. */
bool RegPressureSchedule::DepNodePriorityCmp(const DepNode *node1, const DepNode *node2)
{
    CHECK_NULL_FATAL(node1);
    CHECK_NULL_FATAL(node2);
    int32 priority1 = node1->GetPriority();
    int32 priority2 = node2->GetPriority();
    if (priority1 != priority2) {
        return priority1 > priority2;
    }

    int32 numCall1 = node1->GetNumCall();
    int32 numCall2 = node2->GetNumCall();
    if (node1->GetIncPressure() && node2->GetIncPressure()) {
        if (numCall1 != numCall2) {
            return numCall1 > numCall2;
        }
    }

    int32 near1 = node1->GetNear();
    int32 near2 = node1->GetNear();
    int32 depthS1 = node1->GetMaxDepth() + near1;
    int32 depthS2 = node2->GetMaxDepth() + near2;
    if (depthS1 != depthS2) {
        return depthS1 > depthS2;
    }

    if (near1 != near2) {
        return near1 > near2;
    }

    if (numCall1 != numCall2) {
        return numCall1 > numCall2;
    }

    size_t succsSize1 = node1->GetSuccs().size();
    size_t succsSize2 = node1->GetSuccs().size();
    if (succsSize1 != succsSize2) {
        return succsSize1 < succsSize2;
    }

    if (node1->GetHasPreg() != node2->GetHasPreg()) {
        return node1->GetHasPreg();
    }

    return node1->GetInsn()->GetId() < node2->GetInsn()->GetId();
}

/* set a node's incPressure is true, when a class register inscrease */
void RegPressureSchedule::ReCalculateDepNodePressure(const DepNode &node) const
{
    /* if there is a type of register pressure increases, set incPressure as true. */
    auto &pressures = node.GetPressure();
    node.SetIncPressure(pressures[kRegisterInt] > 0);
}

/* calculate the maxDepth of every node in nodes. */
void RegPressureSchedule::CalculateMaxDepth(const MapleVector<DepNode *> &nodes) const
{
    /* from the last node to first node. */
    for (auto it = nodes.rbegin(); it != nodes.rend(); ++it) {
        /* init call count */
        if ((*it)->GetInsn()->IsCall()) {
            (*it)->SetNumCall(1);
        }
        /* traversing each successor of it. */
        for (auto succ : (*it)->GetSuccs()) {
            DepNode &to = succ->GetTo();
            if ((*it)->GetMaxDepth() < (to.GetMaxDepth() + 1)) {
                (*it)->SetMaxDepth(to.GetMaxDepth() + 1);
            }

            if (to.GetInsn()->IsCall() && ((*it)->GetNumCall() < to.GetNumCall() + 1)) {
                (*it)->SetNumCall(to.GetNumCall() + 1);
            } else if ((*it)->GetNumCall() < to.GetNumCall()) {
                (*it)->SetNumCall(to.GetNumCall());
            }
        }
    }
}

/* calculate the near of every successor of the node. */
void RegPressureSchedule::CalculateNear(const DepNode &node) const
{
    for (auto succ : node.GetSuccs()) {
        DepNode &to = succ->GetTo();
        if (succ->GetDepType() == kDependenceTypeTrue && to.GetNear() < node.GetNear() + 1) {
            to.SetNear(node.GetNear() + 1);
        }
    }
}

/* return true if it is last time using the regNO. */
bool RegPressureSchedule::IsLastUse(const DepNode &node, regno_t regNO)
{
    size_t i = 0;
    for (auto reg : node.GetUseRegnos()) {
        if (reg == regNO) {
            break;
        }
        ++i;
    }
    RegList *regList = node.GetRegUses(i);

    /*
     * except the node, if there are insn that has no scheduled in regNO's regList,
     * then it is not the last time using the regNO, return false.
     */
    while (regList != nullptr) {
        CHECK_NULL_FATAL(regList->insn);
        DepNode *useNode = regList->insn->GetDepNode();
        DEBUG_ASSERT(useNode != nullptr, "get depend node failed in RegPressureSchedule::IsLastUse");
        if ((regList->insn != node.GetInsn()) && (useNode->GetState() != kScheduled)) {
            return false;
        }
        regList = regList->next;
    }
    return true;
}

void RegPressureSchedule::CalculatePressure(const DepNode &node, regno_t reg, bool def) const
{
    RegType regType = GetRegisterType(reg);
    /* if def a register, register pressure increase. */
    if (def) {
        node.IncPressureByIndex(regType);
    } else {
        /* if it is the last time using the reg, register pressure decrease. */
        if (IsLastUse(node, reg)) {
            node.DecPressureByIndex(regType);
        }
    }
}

/* update live reg information. */
void RegPressureSchedule::UpdateLiveReg(const DepNode &node, regno_t reg, bool def)
{
    if (def) {
        if (liveReg.find(reg) == liveReg.end()) {
            (void)liveReg.insert(reg);
#ifdef PRESCHED_DEBUG
            LogInfo::MapleLogger() << "Add new def R" << reg << " to live reg list \n";
#endif
        }
        /* if no use list, a register is only defined, not be used */
        size_t i = 1;
        for (auto defReg : node.GetDefRegnos()) {
            if (defReg == reg) {
                break;
            }
            ++i;
        }
        if (node.GetRegDefs(i) == nullptr && liveOutRegNO.find(reg) == liveOutRegNO.end()) {
#ifdef PRESCHED_DEBUG
            LogInfo::MapleLogger() << "Remove dead def " << reg << " from live reg list \n";
#endif
            liveReg.erase(reg);
        } else if (node.GetRegDefs(i) != nullptr) {
#ifdef PRESCHED_DEBUG
            auto regList = node.GetRegDefs(i);
            LogInfo::MapleLogger() << i << " Live def, dump use insn here \n";
            while (regList != nullptr) {
                node.GetRegDefs(i)->insn->Dump();
                regList = regList->next;
            }
#endif
        }
    } else {
        if (IsLastUse(node, reg)) {
            if (liveReg.find(reg) != liveReg.end() && liveOutRegNO.find(reg) == liveOutRegNO.end()) {
#ifdef PRESCHED_DEBUG
                LogInfo::MapleLogger() << "Remove last use R" << reg << " from live reg list\n";
#endif
                liveReg.erase(reg);
            }
        }
    }
}

/* update register pressure information. */
void RegPressureSchedule::UpdateBBPressure(const DepNode &node)
{
    size_t idx = 0;
    for (auto &reg : node.GetUseRegnos()) {
#ifdef PRESCHED_DEBUG
        LogInfo::MapleLogger() << "Use Reg : R" << reg << "\n";
        UpdateLiveReg(node, reg, false);
        if (liveReg.find(reg) == liveReg.end()) {
            ++idx;
            continue;
        }
#endif

        /* find all insn that use the reg, if a insn use the reg lastly, insn'pressure - 1 */
        RegList *regList = node.GetRegUses(idx);

        while (regList != nullptr) {
            CHECK_NULL_FATAL(regList->insn);
            DepNode *useNode = regList->insn->GetDepNode();
            if (useNode->GetState() == kScheduled) {
                regList = regList->next;
                continue;
            }

            if (IsLastUse(*useNode, reg)) {
                RegType regType = GetRegisterType(reg);
                useNode->DecPressureByIndex(regType);
            }
            break;
        }
        ++idx;
    }

#ifdef PRESCHED_DEBUG
    for (auto &defReg : node.GetDefRegnos()) {
        UpdateLiveReg(node, defReg, true);
    }
#endif

    const auto &pressures = node.GetPressure();
    const auto &deadDefNum = node.GetDeadDefNum();
#ifdef PRESCHED_DEBUG
    LogInfo::MapleLogger() << "\nnode's pressure: ";
    for (auto pressure : pressures) {
        LogInfo::MapleLogger() << pressure << " ";
    }
    LogInfo::MapleLogger() << "\n";
#endif

    FOR_ALL_REGCLASS(i)
    {
        curPressure[i] += pressures[i];
        curPressure[i] -= deadDefNum[i];
        if (curPressure[i] > maxPressure[i]) {
            maxPressure[i] = curPressure[i];
        }
    }
}

/* update node priority and try to update the priority of all node's ancestor. */
void RegPressureSchedule::UpdatePriority(DepNode &node)
{
    std::vector<DepNode *> workQueue;
    workQueue.emplace_back(&node);
    node.SetPriority(maxPriority++);
    do {
        DepNode *nowNode = workQueue.front();
        (void)workQueue.erase(workQueue.begin());
        for (auto pred : nowNode->GetPreds()) {
            DepNode &from = pred->GetFrom();
            if (from.GetState() != kScheduled && from.GetPriority() < maxPriority) {
                from.SetPriority(maxPriority);
                workQueue.emplace_back(&from);
            }
        }
    } while (!workQueue.empty());
}

/* return true if all node's pred has been scheduled. */
bool RegPressureSchedule::CanSchedule(const DepNode &node) const
{
    return node.GetValidPredsSize() == 0;
}

/*
 * delete node from readylist and
 * add the successor of node to readyList when
 *  1. successor has no been scheduled;
 *  2. successor's has been scheduled or the dependence between node and successor is true-dependence.
 */
void RegPressureSchedule::UpdateReadyList(const DepNode &node)
{
    /* delete node from readylist */
    for (auto it = readyList.begin(); it != readyList.end(); ++it) {
        if (*it == &node) {
            readyList.erase(it);
            break;
        }
    }
    /* update dependency information of the successors and add nodes into readyList */
    for (auto *succ : node.GetSuccs()) {
        DepNode &succNode = succ->GetTo();
        if (!partialSet.empty() && (partialSet.find(&succNode) == partialSet.end())) {
            continue;
        }
        succNode.DecreaseValidPredsSize();
        if (((succ->GetDepType() == kDependenceTypeTrue) || CanSchedule(succNode)) &&
            (succNode.GetState() == kNormal)) {
            readyList.emplace_back(&succNode);
            succNode.SetState(kReady);
        }
    }
}
/*
 * Another version of UpdateReadyList for brute force ready list update
 * The difference is to store the state change status for the successors for later restoring
 */
void RegPressureSchedule::BruteUpdateReadyList(const DepNode &node, std::vector<bool> &changedToReady)
{
    /* delete node from readylist */
    for (auto it = readyList.begin(); it != readyList.end(); ++it) {
        if (*it == &node) {
            readyList.erase(it);
            break;
        }
    }
    /* update dependency information of the successors and add nodes into readyList */
    for (auto *succ : node.GetSuccs()) {
        DepNode &succNode = succ->GetTo();
        if (!partialSet.empty() && (partialSet.find(&succNode) == partialSet.end())) {
            continue;
        }
        succNode.DecreaseValidPredsSize();
        if (((succ->GetDepType() == kDependenceTypeTrue) || CanSchedule(succNode)) &&
            (succNode.GetState() == kNormal)) {
            readyList.emplace_back(&succNode);
            succNode.SetState(kReady);
            changedToReady.emplace_back(true);
        } else {
            changedToReady.emplace_back(false);
        }
    }
}

/*
 * Restore the ready list status when finishing one brute scheduling series generation
 */
void RegPressureSchedule::RestoreReadyList(DepNode &node, std::vector<bool> &changedToReady)
{
    uint32 i = 0;
    /* restore state information of the successors and delete them from readyList */
    for (auto *succ : node.GetSuccs()) {
        DepNode &succNode = succ->GetTo();
        succNode.IncreaseValidPredsSize();
        if (changedToReady.at(i)) {
            succNode.SetState(kNormal);
            for (auto it = readyList.begin(); it != readyList.end(); ++it) {
                if (*it == &succNode) {
                    readyList.erase(it);
                    break;
                }
            }
        }
        ++i;
    }
    /* add the node back into the readyList */
    readyList.emplace_back(&node);
}
/* choose a node to schedule */
DepNode *RegPressureSchedule::ChooseNode()
{
    DepNode *node = nullptr;
    for (auto *it : readyList) {
        if (!it->GetIncPressure() && !it->GetHasNativeCallRegister()) {
            if (CanSchedule(*it)) {
                return it;
            } else if (node == nullptr) {
                node = it;
            }
        }
    }
    if (node == nullptr) {
        node = readyList.front();
    }
    return node;
}

void RegPressureSchedule::DumpBBLiveInfo() const
{
    LogInfo::MapleLogger() << "Live In: ";
    for (auto reg : bb->GetLiveInRegNO()) {
        LogInfo::MapleLogger() << "R" << reg << " ";
    }
    LogInfo::MapleLogger() << "\n";

    LogInfo::MapleLogger() << "Live Out: ";
    for (auto reg : bb->GetLiveOutRegNO()) {
        LogInfo::MapleLogger() << "R" << reg << " ";
    }
    LogInfo::MapleLogger() << "\n";
}

void RegPressureSchedule::DumpReadyList() const
{
    LogInfo::MapleLogger() << "readyList: "
                           << "\n";
    for (DepNode *it : readyList) {
        if (CanSchedule(*it)) {
            LogInfo::MapleLogger() << it->GetInsn()->GetId() << "CS ";
        } else {
            LogInfo::MapleLogger() << it->GetInsn()->GetId() << "NO ";
        }
    }
    LogInfo::MapleLogger() << "\n";
}

void RegPressureSchedule::DumpSelectInfo(const DepNode &node) const
{
    LogInfo::MapleLogger() << "select a node: "
                           << "\n";
    node.DumpSchedInfo();
    node.DumpRegPressure();
    node.GetInsn()->Dump();

    LogInfo::MapleLogger() << "liveReg: ";
    for (auto reg : liveReg) {
        LogInfo::MapleLogger() << "R" << reg << " ";
    }
    LogInfo::MapleLogger() << "\n";

    LogInfo::MapleLogger() << "\n";
}

void RegPressureSchedule::DumpDependencyInfo(const MapleVector<DepNode *> &nodes)
{
    LogInfo::MapleLogger() << "Dump Dependency Begin \n";
    for (auto node : nodes) {
        LogInfo::MapleLogger() << "Insn \n";
        node->GetInsn()->Dump();
        LogInfo::MapleLogger() << "Successors \n";
        /* update dependency information of the successors and add nodes into readyList */
        for (auto *succ : node->GetSuccs()) {
            DepNode &succNode = succ->GetTo();
            succNode.GetInsn()->Dump();
        }
    }
    LogInfo::MapleLogger() << "Dump Dependency End \n";
}

void RegPressureSchedule::ReportScheduleError() const
{
    LogInfo::MapleLogger() << "Error No Equal Length for Series"
                           << "\n";
    DumpDependencyInfo(originalNodeSeries);
    for (auto node : scheduledNode) {
        node->GetInsn()->Dump();
    }
    LogInfo::MapleLogger() << "Original One"
                           << "\n";
    for (auto node : originalNodeSeries) {
        node->GetInsn()->Dump();
    }
    LogInfo::MapleLogger() << "Error No Equal Length for End"
                           << "\n";
}

void RegPressureSchedule::ReportScheduleOutput() const
{
    LogInfo::MapleLogger() << "Original Pressure : " << originalPressure << " \n";
    LogInfo::MapleLogger() << "Scheduled Pressure : " << scheduledPressure << " \n";
    if (originalPressure > scheduledPressure) {
        LogInfo::MapleLogger() << "Pressure Reduced by : " << (originalPressure - scheduledPressure) << " \n";
        return;
    } else if (originalPressure == scheduledPressure) {
        LogInfo::MapleLogger() << "Pressure Not Changed \n";
    } else {
        LogInfo::MapleLogger() << "Pressure Increased by : " << (scheduledPressure - originalPressure) << " \n";
    }
    LogInfo::MapleLogger() << "Pressure Not Reduced, Restore Node Series \n";
}

void RegPressureSchedule::DumpBBPressureInfo() const
{
    LogInfo::MapleLogger() << "curPressure: ";
    FOR_ALL_REGCLASS(i)
    {
        LogInfo::MapleLogger() << curPressure[i] << " ";
    }
    LogInfo::MapleLogger() << "\n";

    LogInfo::MapleLogger() << "maxPressure: ";
    FOR_ALL_REGCLASS(i)
    {
        LogInfo::MapleLogger() << maxPressure[i] << " ";
    }
    LogInfo::MapleLogger() << "\n";
}

void RegPressureSchedule::DoScheduling(MapleVector<DepNode *> &nodes)
{
    /* Store the original series */
    originalNodeSeries.clear();
    for (auto node : nodes) {
        originalNodeSeries.emplace_back(node);
    }
    InitPartialSplitters(nodes);
#if PRESCHED_DEBUG
    LogInfo::MapleLogger() << "\n Calculate Pressure Info for Schedule Input Series \n";
#endif
    originalPressure = CalculateRegisterPressure(nodes);
#if PRESCHED_DEBUG
    LogInfo::MapleLogger() << "Original pressure : " << originalPressure << "\n";
#endif
    /* Original pressure is small enough, skip pre-scheduling */
    if (originalPressure < PRESSURE_STANDARD) {
#if PRESCHED_DEBUG
        LogInfo::MapleLogger() << "Original pressure is small enough, skip pre-scheduling \n";
#endif
        return;
    }
    if (splitterIndexes.empty()) {
        LogInfo::MapleLogger() << "No splitter, normal scheduling \n";
        if (!OPTIMISTIC_SCHEDULING) {
            HeuristicScheduling(nodes);
        } else {
            InitBruteForceScheduling(nodes);
            BruteForceScheduling();
            if (optimisticScheduledNodes.size() == nodes.size() && minPressure < originalPressure) {
                nodes.clear();
                for (auto node : optimisticScheduledNodes) {
                    nodes.emplace_back(node);
                }
            }
        }
    } else {
        /* Split the node list into multiple parts based on split point and conduct scheduling */
        PartialScheduling(nodes);
    }
    scheduledPressure = CalculateRegisterPressure(nodes);
    EmitSchedulingSeries(nodes);
}

void RegPressureSchedule::HeuristicScheduling(MapleVector<DepNode *> &nodes)
{
#ifdef PRESCHED_DEBUG
    LogInfo::MapleLogger() << "--------------- bb " << bb->GetId() << " begin scheduling -------------"
                           << "\n";
    DumpBBLiveInfo();
#endif

    /* initialize register pressure information and readylist. */
    Init(nodes);
    CalculateMaxDepth(nodes);
    while (!readyList.empty()) {
        /* calculate register pressure */
        for (DepNode *it : readyList) {
            ReCalculateDepNodePressure(*it);
        }
        if (readyList.size() > 1) {
            SortReadyList();
        }

        /* choose a node can be scheduled currently. */
        DepNode *node = ChooseNode();
#ifdef PRESCHED_DEBUG
        DumpBBPressureInfo();
        DumpReadyList();
        LogInfo::MapleLogger() << "first tmp select node: " << node->GetInsn()->GetId() << "\n";
#endif

        while (!CanSchedule(*node)) {
            UpdatePriority(*node);
            SortReadyList();
            node = readyList.front();
#ifdef PRESCHED_DEBUG
            LogInfo::MapleLogger() << "update ready list: "
                                   << "\n";
            DumpReadyList();
#endif
        }

        scheduledNode.emplace_back(node);
        /* mark node has scheduled */
        node->SetState(kScheduled);
        UpdateBBPressure(*node);
        CalculateNear(*node);
        UpdateReadyList(*node);
#ifdef PRESCHED_DEBUG
        DumpSelectInfo(*node);
#endif
    }

#ifdef PRESCHED_DEBUG
    LogInfo::MapleLogger() << "---------------------------------- end --------------------------------"
                           << "\n";
#endif
    /* update nodes according to scheduledNode. */
    nodes.clear();
    for (auto node : scheduledNode) {
        nodes.emplace_back(node);
    }
}
/*
 * Calculate the register pressure for current BB based on an instruction series
 */
int RegPressureSchedule::CalculateRegisterPressure(MapleVector<DepNode *> &nodes)
{
    /* Initialize the live, live in, live out register max pressure information */
    liveReg.clear();
    liveInRegNO = bb->GetLiveInRegNO();
    liveOutRegNO = bb->GetLiveOutRegNO();
    std::vector<ScheduleState> restoreStateSeries;
    int maximumPressure = 0;
    /* Mock all the nodes to kScheduled status for pressure calculation */
    for (auto node : nodes) {
        restoreStateSeries.emplace_back(node->GetState());
        node->SetState(kScheduled);
    }
    /* Update live register set according to the instruction series */
    for (auto node : nodes) {
        for (auto &reg : node->GetUseRegnos()) {
            UpdateLiveReg(*node, reg, false);
        }
        for (auto &defReg : node->GetDefRegnos()) {
            UpdateLiveReg(*node, defReg, true);
        }
        int currentPressure = static_cast<int>(liveReg.size());
        if (currentPressure > maximumPressure) {
            maximumPressure = currentPressure;
        }
#ifdef PRESCHED_DEBUG
        node->GetInsn()->Dump();
        LogInfo::MapleLogger() << "Dump Live Reg : "
                               << "\n";
        for (auto reg : liveReg) {
            LogInfo::MapleLogger() << "R" << reg << " ";
        }
        LogInfo::MapleLogger() << "\n";
#endif
    }
    /* Restore the Schedule State */
    uint32 i = 0;
    for (auto node : nodes) {
        node->SetState(restoreStateSeries.at(i));
        ++i;
    }
    return maximumPressure;
}

/*
 * Split the series into multiple parts and conduct pre-scheduling in every part
 */
void RegPressureSchedule::PartialScheduling(MapleVector<DepNode *> &nodes)
{
    for (size_t i = 0; i < splitterIndexes.size() - 1; ++i) {
        constexpr uint32 lastTwoNodeIndex = 2;
        auto begin = static_cast<uint32>(splitterIndexes.at(i));
        auto end = static_cast<uint32>(splitterIndexes.at(i + 1));
        for (uint32 j = begin; j < end; ++j) {
            partialList.emplace_back(nodes.at(j));
        }
        if (i == splitterIndexes.size() - lastTwoNodeIndex) {
            partialList.emplace_back(nodes.at(end));
        }
        for (auto node : partialList) {
            partialSet.insert(node);
        }
        HeuristicScheduling(partialList);
        for (auto node : partialList) {
            partialScheduledNode.emplace_back(node);
        }
        partialList.clear();
        partialSet.clear();
    }
    nodes.clear();
    /* Construct overall scheduling output */
    for (auto node : partialScheduledNode) {
        nodes.emplace_back(node);
    }
}

/*
 * Brute-force scheduling algorithm
 * It enumerates all the possible schedule series and pick a best one
 */
void RegPressureSchedule::BruteForceScheduling()
{
    /* stop brute force scheduling when exceeding the count limit */
    if (BRUTE_MAXIMUM_LIMIT && (scheduleSeriesCount > SCHEDULING_MAXIMUM_COUNT)) {
        return;
    }
    int defaultPressureValue = -1;
    /* ReadyList is empty, scheduling is over */
    if (readyList.empty()) {
        if (originalNodeSeries.size() != scheduledNode.size()) {
#ifdef PRESCHED_DEBUG
            ReportScheduleError();
#endif
            return;
        }
        ++scheduleSeriesCount;
        int currentPressure = CalculateRegisterPressure(scheduledNode);
        if (minPressure == defaultPressureValue || currentPressure < minPressure) {
            minPressure = currentPressure;
            /* update better scheduled series */
            optimisticScheduledNodes.clear();
            for (auto node : scheduledNode) {
                optimisticScheduledNodes.emplace_back(node);
            }
            return;
        }
        return;
    }
    /* store the current status of the ready list */
    std::vector<DepNode *> innerList;
    for (auto tempNode : readyList) {
        innerList.emplace_back(tempNode);
    }
    for (auto *node : innerList) {
        if (CanSchedule(*node)) {
            /* update readyList and node dependency info */
            std::vector<bool> changedToReady;
            BruteUpdateReadyList(*node, changedToReady);
            scheduledNode.emplace_back(node);
            node->SetState(kScheduled);
            BruteForceScheduling();
            node->SetState(kReady);
            /* restore readyList and node dependency info */
            RestoreReadyList(*node, changedToReady);
            scheduledNode.pop_back();
        }
    }
}

/*
 * Calculate the pred size based on the dependency information
 */
void RegPressureSchedule::CalculatePredSize(DepNode &node)
{
    constexpr uint32 emptyPredsSize = 0;
    node.SetValidPredsSize(emptyPredsSize);
    for (auto pred : node.GetPreds()) {
        DepNode &from = pred->GetFrom();
        if (!partialSet.empty() && (partialSet.find(&from) == partialSet.end())) {
            continue;
        } else {
            node.IncreaseValidPredsSize();
        }
    }
}

void RegPressureSchedule::InitBruteForceScheduling(MapleVector<DepNode *> &nodes)
{
    /* Calculate pred size of the node */
    for (auto node : nodes) {
        CalculatePredSize(*node);
    }
    readyList.clear();
    optimisticScheduledNodes.clear();
    scheduledNode.clear();
    DepNode *firstNode = nodes.front();
    firstNode->SetState(kReady);
    readyList.emplace_back(firstNode);
}

/*
 * Give out the pre-scheduling output based on new register pressure
 */
void RegPressureSchedule::EmitSchedulingSeries(MapleVector<DepNode *> &nodes)
{
#ifdef PRESCHED_DEBUG
    ReportScheduleOutput();
#endif
    if (originalPressure <= scheduledPressure) {
        /* Restore the original series */
        nodes.clear();
        for (auto node : originalNodeSeries) {
            nodes.emplace_back(node);
        }
    }
}

/*
 * ------------- Schedule function ----------
 * calculate and mark each insn id, each BB's firstLoc and lastLoc.
 */
void Schedule::InitIDAndLoc()
{
    uint32 id = 0;
    FOR_ALL_BB(bb, &cgFunc)
    {
        bb->SetLastLoc(bb->GetPrev() ? bb->GetPrev()->GetLastLoc() : nullptr);
        FOR_BB_INSNS(insn, bb)
        {
            insn->SetId(id++);
#if DEBUG
            insn->AppendComment(" Insn id: " + std::to_string(insn->GetId()));
#endif
            if (insn->IsImmaterialInsn() && !insn->IsComment()) {
                bb->SetLastLoc(insn);
            } else if (!bb->GetFirstLoc() && insn->IsMachineInstruction()) {
                bb->SetFirstLoc(*bb->GetLastLoc());
            }
        }
    }
}

/* === new pm === */
bool CgPreScheduling::PhaseRun(maplebe::CGFunc &f)
{
    if (f.HasAsm()) {
        return true;
    }
    if (LIST_SCHED_DUMP_NEWPM) {
        LogInfo::MapleLogger() << "Before CgDoPreScheduling : " << f.GetName() << "\n";
        DotGenerator::GenerateDot("preschedule", f, f.GetMirModule());
    }
    auto *live = GET_ANALYSIS(CgLiveAnalysis, f);
    /* revert liveanalysis result container. */
    DEBUG_ASSERT(live != nullptr, "nullptr check");
    live->ResetLiveSet();

    Schedule *schedule = nullptr;
#if (defined(TARGAARCH64) && TARGAARCH64) || (defined(TARGRISCV64) && TARGRISCV64)
    schedule = GetPhaseAllocator()->New<AArch64Schedule>(f, *GetPhaseMemPool(), *live, PhaseName());
#endif
#if defined(TARGARM32) && TARGARM32
    schedule = GetPhaseAllocator()->New<Arm32Schedule>(f, *GetPhaseMemPool(), *live, PhaseName());
#endif
    schedule->ListScheduling(true);
    live->ClearInOutDataInfo();

    return true;
}

void CgPreScheduling::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgLiveAnalysis>();
    aDep.PreservedAllExcept<CgLiveAnalysis>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgPreScheduling, prescheduling)

bool CgScheduling::PhaseRun(maplebe::CGFunc &f)
{
    if (f.HasAsm()) {
        return true;
    }
    if (LIST_SCHED_DUMP_NEWPM) {
        LogInfo::MapleLogger() << "Before CgDoScheduling : " << f.GetName() << "\n";
        DotGenerator::GenerateDot("scheduling", f, f.GetMirModule());
    }
    auto *live = GET_ANALYSIS(CgLiveAnalysis, f);
    /* revert liveanalysis result container. */
    DEBUG_ASSERT(live != nullptr, "nullptr check");
    live->ResetLiveSet();

    Schedule *schedule = nullptr;
#if (defined(TARGAARCH64) && TARGAARCH64) || (defined(TARGRISCV64) && TARGRISCV64)
    schedule = GetPhaseAllocator()->New<AArch64Schedule>(f, *GetPhaseMemPool(), *live, PhaseName());
#endif
#if defined(TARGARM32) && TARGARM32
    schedule = GetPhaseAllocator()->New<Arm32Schedule>(f, *GetPhaseMemPool(), *live, PhaseName());
#endif
    schedule->ListScheduling(false);
    live->ClearInOutDataInfo();

    return true;
}

void CgScheduling::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgLiveAnalysis>();
    aDep.PreservedAllExcept<CgLiveAnalysis>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgScheduling, scheduling)
} /* namespace maplebe */
