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

#include "aarch64_schedule.h"
#include <ctime>
#include "aarch64_cg.h"
#include "aarch64_operand.h"
#include "aarch64_dependence.h"
#include "file_utils.h"
#include "pressure.h"

/*
 * This phase is Instruction Scheduling.
 * There is a local list scheduling, it is scheduling in basic block.
 * The entry is AArch64Schedule::ListScheduling, will traversal all basic block,
 * for a basic block:
 *  1. build a dependence graph;
 *  2. combine clinit pairs and str&ldr pairs;
 *  3. reorder instructions.
 */
namespace maplebe {
namespace {
constexpr uint32 kSecondToLastNode = 2;
}  // namespace

uint32 AArch64Schedule::maxUnitIndex = 0;
/* reserve two register for special purpose */
int AArch64Schedule::intRegPressureThreshold = static_cast<int>(R27 - R0);
int AArch64Schedule::fpRegPressureThreshold = static_cast<int>(V30 - V0);
int AArch64Schedule::intCalleeSaveThresholdBase = static_cast<int>(R29 - R19);
int AArch64Schedule::intCalleeSaveThresholdEnhance = static_cast<int>(R30 - R19);
int AArch64Schedule::fpCalleeSaveThreshold = static_cast<int>(R16 - R8);
/* Init schedule's data struction. */
void AArch64Schedule::Init()
{
    readyList.clear();
    nodeSize = nodes.size();
    lastSeparatorIndex = 0;
    mad->ReleaseAllUnits();
    DepNode *node = nodes[0];

    DEBUG_ASSERT(node->GetType() == kNodeTypeSeparator,
                 "CG internal error, the first node should be a separator node.");

    if (CGOptions::IsDruteForceSched() || CGOptions::IsSimulateSched()) {
        for (auto nodeTemp : nodes) {
            nodeTemp->SetVisit(0);
            nodeTemp->SetState(kNormal);
            nodeTemp->SetSchedCycle(0);
            nodeTemp->SetEStart(0);
            nodeTemp->SetLStart(0);
        }
    }

    readyList.emplace_back(node);
    node->SetState(kReady);

    /* Init validPredsSize and validSuccsSize. */
    for (auto nodeTemp : nodes) {
        nodeTemp->SetValidPredsSize(nodeTemp->GetPreds().size());
        nodeTemp->SetValidSuccsSize(nodeTemp->GetSuccs().size());
    }
}

/*
 *  A insn which can be combine should meet this conditions:
 *  1. it is str/ldr insn;
 *  2. address mode is kAddrModeBOi, [baseReg, offset];
 *  3. the register operand size equal memory operand size;
 *  4. if define USE_32BIT_REF, register operand size should be 4 byte;
 *  5. for stp/ldp, the imm should be within -512 and 504(64bit), or -256 and 252(32bit);
 *  6. pair instr for 8/4 byte registers must have multiple of 8/4 for imm.
 *  If insn can be combine, return true.
 */
bool AArch64Schedule::CanCombine(const Insn &insn) const
{
    MOperator opCode = insn.GetMachineOpcode();
    if ((opCode != MOP_xldr) && (opCode != MOP_wldr) && (opCode != MOP_dldr) && (opCode != MOP_sldr) &&
        (opCode != MOP_xstr) && (opCode != MOP_wstr) && (opCode != MOP_dstr) && (opCode != MOP_sstr)) {
        return false;
    }

    DEBUG_ASSERT(insn.GetOperand(1).IsMemoryAccessOperand(), "expects mem operands");
    auto &memOpnd = static_cast<MemOperand &>(insn.GetOperand(1));
    MemOperand::AArch64AddressingMode addrMode = memOpnd.GetAddrMode();
    if (addrMode != MemOperand::kAddrModeBOi) {
        return false;
    }

    auto &regOpnd = static_cast<RegOperand &>(insn.GetOperand(0));
    if (regOpnd.GetSize() != memOpnd.GetSize()) {
        return false;
    }

    uint32 size = regOpnd.GetSize() >> kLog2BitsPerByte;
#ifdef USE_32BIT_REF
    if (insn.IsAccessRefField() && (size > (kAarch64IntregBytelen >> 1))) {
        return false;
    }
#endif /* USE_32BIT_REF */

    OfstOperand *offset = memOpnd.GetOffsetImmediate();
    if (offset == nullptr) {
        return false;
    }
    int32 offsetValue = static_cast<int32>(offset->GetOffsetValue());
    if (size == kAarch64IntregBytelen) { /* 64 bit */
        if ((offsetValue <= kStpLdpImm64LowerBound) || (offsetValue >= kStpLdpImm64UpperBound)) {
            return false;
        }
    } else if (size == (kAarch64IntregBytelen >> 1)) { /* 32 bit */
        if ((offsetValue <= kStpLdpImm32LowerBound) || (offsetValue >= kStpLdpImm32UpperBound)) {
            return false;
        }
    }

    /* pair instr for 8/4 byte registers must have multiple of 8/4 for imm */
    if ((static_cast<uint32>(offsetValue) % size) != 0) {
        return false;
    }
    return true;
}

/* After building dependence graph, combine str&ldr pairs. */
void AArch64Schedule::MemoryAccessPairOpt()
{
    Init();
    std::vector<DepNode *> memList;

    while ((!readyList.empty()) || !memList.empty()) {
        DepNode *readNode = nullptr;
        if (!readyList.empty()) {
            readNode = readyList[0];
            readyList.erase(readyList.begin());
        } else {
            if (memList[0]->GetType() != kNodeTypeEmpty) {
                FindAndCombineMemoryAccessPair(memList);
            }
            readNode = memList[0];
            memList.erase(memList.cbegin());
        }

        /* schedule readNode */
        CHECK_FATAL(readNode != nullptr, "readNode is null in MemoryAccessPairOpt");
        readNode->SetState(kScheduled);

        /* add readNode's succs to readyList or memList. */
        for (auto succLink : readNode->GetSuccs()) {
            DepNode &succNode = succLink->GetTo();
            succNode.DecreaseValidPredsSize();
            if (succNode.GetValidPredsSize() == 0) {
                DEBUG_ASSERT(succNode.GetState() == kNormal, "schedule state should be kNormal");
                succNode.SetState(kReady);
                DEBUG_ASSERT(succNode.GetInsn() != nullptr, "insn can't be nullptr!");
                if (CanCombine(*succNode.GetInsn())) {
                    memList.emplace_back(&succNode);
                } else {
                    readyList.emplace_back(&succNode);
                }
            }
        }
    }

    for (auto node : nodes) {
        node->SetVisit(0);
        node->SetState(kNormal);
    }
}

/* Find and combine correct MemoryAccessPair for memList[0]. */
void AArch64Schedule::FindAndCombineMemoryAccessPair(const std::vector<DepNode *> &memList)
{
    DEBUG_ASSERT(!memList.empty(), "memList should not be empty");
    CHECK_FATAL(memList[0]->GetInsn() != nullptr, "memList[0]'s insn should not be nullptr");
    MemOperand *currMemOpnd = static_cast<MemOperand *>(memList[0]->GetInsn()->GetMemOpnd());
    DEBUG_ASSERT(currMemOpnd != nullptr, "opnd should not be nullptr");
    DEBUG_ASSERT(currMemOpnd->IsMemoryAccessOperand(), "opnd should be memOpnd");
    int32 currOffsetVal = static_cast<int32>(currMemOpnd->GetOffsetImmediate()->GetOffsetValue());
    MOperator currMop = memList[0]->GetInsn()->GetMachineOpcode();
    /* find a depNode to combine with memList[0], and break; */
    for (auto it = std::next(memList.begin(), 1); it != memList.end(); ++it) {
        DEBUG_ASSERT((*it)->GetInsn() != nullptr, "null ptr check");

        if (currMop == (*it)->GetInsn()->GetMachineOpcode()) {
            MemOperand *nextMemOpnd = static_cast<MemOperand *>((*it)->GetInsn()->GetMemOpnd());
            CHECK_FATAL(nextMemOpnd != nullptr, "opnd should not be nullptr");
            CHECK_FATAL(nextMemOpnd->IsMemoryAccessOperand(), "opnd should be MemOperand");
            int32 nextOffsetVal = static_cast<int32>(nextMemOpnd->GetOffsetImmediate()->GetOffsetValue());
            uint32 size = currMemOpnd->GetSize() >> kLog2BitsPerByte;
            if ((nextMemOpnd->GetBaseRegister() == currMemOpnd->GetBaseRegister()) &&
                (nextMemOpnd->GetSize() == currMemOpnd->GetSize()) &&
                (static_cast<uint32>(abs(nextOffsetVal - currOffsetVal)) == size)) {
                /*
                 * In ARM Architecture Reference Manual ARMv8, for ARMv8-A architecture profile
                 * LDP on page K1-6125 declare that ldp can't use same reg
                 */
                if (((currMop == MOP_xldr) || (currMop == MOP_sldr) || (currMop == MOP_dldr) ||
                     (currMop == MOP_wldr)) &&
                    &(memList[0]->GetInsn()->GetOperand(0)) == &((*it)->GetInsn()->GetOperand(0))) {
                    continue;
                }
                if (static_cast<RegOperand &>((*it)->GetInsn()->GetOperand(0)).GetRegisterType() !=
                    static_cast<RegOperand &>(memList[0]->GetInsn()->GetOperand(0)).GetRegisterType()) {
                    continue;
                }

                if (LIST_SCHED_DUMP_REF) {
                    LogInfo::MapleLogger() << "Combine insn: "
                                           << "\n";
                    memList[0]->GetInsn()->Dump();
                    (*it)->GetInsn()->Dump();
                }
                depAnalysis->CombineMemoryAccessPair(*memList[0], **it, nextOffsetVal > currOffsetVal);
                if (LIST_SCHED_DUMP_REF) {
                    LogInfo::MapleLogger() << "To: "
                                           << "\n";
                    memList[0]->GetInsn()->Dump();
                }
                break;
            }
        }
    }
}

/* combine clinit pairs. */
void AArch64Schedule::ClinitPairOpt()
{
    for (auto it = nodes.begin(); it != nodes.end(); ++it) {
        auto nextIt = std::next(it, 1);
        if (nextIt == nodes.end()) {
            return;
        }

        if ((*it)->GetInsn()->GetMachineOpcode() == MOP_adrp_ldr) {
            if ((*nextIt)->GetInsn()->GetMachineOpcode() == MOP_clinit_tail) {
                depAnalysis->CombineClinit(**it, **(nextIt), false);
            } else if ((*nextIt)->GetType() == kNodeTypeSeparator) {
                nextIt = std::next(nextIt, 1);
                if (nextIt == nodes.end()) {
                    return;
                }
                if ((*nextIt)->GetInsn()->GetMachineOpcode() == MOP_clinit_tail) {
                    /* Do something. */
                    depAnalysis->CombineClinit(**it, **(nextIt), true);
                }
            }
        }
    }
}

/* Return the next node's index who is kNodeTypeSeparator. */
uint32 AArch64Schedule::GetNextSepIndex() const
{
    CHECK_FATAL(nodes.size() >= 1, "value overflow");
    return ((lastSeparatorIndex + kMaxDependenceNum) < nodeSize) ? (lastSeparatorIndex + kMaxDependenceNum)
                                                                 : (nodes.size() - 1);
}

/* Do register pressure schduling. */
void AArch64Schedule::RegPressureScheduling(BB &bb, MapleVector<DepNode *> &nodes)
{
    auto *regSchedule = memPool.New<RegPressureSchedule>(cgFunc, alloc);
    /*
     * Get physical register amount currently
     * undef, Int Reg, Float Reg, Flag Reg
     */
    const std::vector<int32> kRegNumVec = {0, V0, (kMaxRegNum - V0) + 1, 1};
    regSchedule->InitBBInfo(bb, memPool, nodes);
    regSchedule->BuildPhyRegInfo(kRegNumVec);
    regSchedule->DoScheduling(nodes);
}

/*
 * Compute earliest start of the node,
 * return value : the maximum estart.
 */
uint32 AArch64Schedule::ComputeEstart(uint32 cycle)
{
    std::vector<DepNode *> readyNodes;
    uint32 maxIndex = GetNextSepIndex();

    if (CGOptions::IsDebugSched()) {
        /* Check validPredsSize. */
        for (uint32 i = lastSeparatorIndex; i <= maxIndex; ++i) {
            DepNode *node = nodes[i];
            int32 schedNum = 0;
            for (const auto *predLink : node->GetPreds()) {
                if (predLink->GetFrom().GetState() == kScheduled) {
                    ++schedNum;
                }
            }
            CHECK_FATAL((static_cast<uint32>(node->GetPreds().size()) - static_cast<uint32>(schedNum)) ==
                            node->GetValidPredsSize(),
                        "validPredsSize error.");
        }
    }

    DEBUG_ASSERT(nodes[maxIndex]->GetType() == kNodeTypeSeparator,
                 "CG internal error, nodes[maxIndex] should be a separator node.");

    (void)readyNodes.insert(readyNodes.cbegin(), readyList.cbegin(), readyList.cend());

    uint32 maxEstart = cycle;
    for (uint32 i = lastSeparatorIndex; i <= maxIndex; ++i) {
        DepNode *node = nodes[i];
        node->SetVisit(0);
    }

    for (auto *node : readyNodes) {
        DEBUG_ASSERT(node->GetState() == kReady, "CG internal error, all nodes in ready list should be ready.");
        if (node->GetEStart() < cycle) {
            node->SetEStart(cycle);
        }
    }

    while (!readyNodes.empty()) {
        DepNode *node = readyNodes.front();
        readyNodes.erase(readyNodes.cbegin());

        for (const auto *succLink : node->GetSuccs()) {
            DepNode &succNode = succLink->GetTo();
            if (succNode.GetType() == kNodeTypeSeparator) {
                continue;
            }

            if (succNode.GetEStart() < (node->GetEStart() + succLink->GetLatency())) {
                succNode.SetEStart(node->GetEStart() + succLink->GetLatency());
            }
            maxEstart = (maxEstart < succNode.GetEStart() ? succNode.GetEStart() : maxEstart);
            succNode.IncreaseVisit();
            if ((succNode.GetVisit() >= succNode.GetValidPredsSize()) && (succNode.GetType() != kNodeTypeSeparator)) {
                readyNodes.emplace_back(&succNode);
            }
            DEBUG_ASSERT(succNode.GetVisit() <= succNode.GetValidPredsSize(), "CG internal error.");
        }
    }

    return maxEstart;
}

/* Compute latest start of the node. */
void AArch64Schedule::ComputeLstart(uint32 maxEstart)
{
    /* std::vector is better than std::queue in run time */
    std::vector<DepNode *> readyNodes;
    uint32 maxIndex = GetNextSepIndex();

    DEBUG_ASSERT(nodes[maxIndex]->GetType() == kNodeTypeSeparator,
                 "CG internal error, nodes[maxIndex] should be a separator node.");

    for (uint32 i = lastSeparatorIndex; i <= maxIndex; ++i) {
        DepNode *node = nodes[i];
        node->SetLStart(maxEstart);
        node->SetVisit(0);
    }

    readyNodes.emplace_back(nodes[maxIndex]);
    while (!readyNodes.empty()) {
        DepNode *node = readyNodes.front();
        readyNodes.erase(readyNodes.cbegin());
        for (const auto *predLink : node->GetPreds()) {
            DepNode &predNode = predLink->GetFrom();
            if (predNode.GetState() == kScheduled) {
                continue;
            }

            if (predNode.GetLStart() > (node->GetLStart() - predLink->GetLatency())) {
                predNode.SetLStart(node->GetLStart() - predLink->GetLatency());
            }
            predNode.IncreaseVisit();
            if ((predNode.GetVisit() >= predNode.GetValidSuccsSize()) && (predNode.GetType() != kNodeTypeSeparator)) {
                readyNodes.emplace_back(&predNode);
            }

            DEBUG_ASSERT(predNode.GetVisit() <= predNode.GetValidSuccsSize(), "CG internal error.");
        }
    }
}

/* Compute earliest start and latest start of the node that is in readyList and not be scheduled. */
void AArch64Schedule::UpdateELStartsOnCycle(uint32 cycle)
{
    ComputeLstart(ComputeEstart(cycle));
}

/* Count unit kinds to an array. Each element of the array indicates the unit kind number of a node set. */
void AArch64Schedule::CountUnitKind(const DepNode &depNode, uint32 array[], const uint32 arraySize) const
{
    (void)arraySize;
    DEBUG_ASSERT(arraySize >= kUnitKindLast, "CG internal error. unit kind number is not correct.");
    uint32 unitKind = depNode.GetUnitKind();
    uint32 index = static_cast<uint32>(__builtin_ffs(static_cast<int>(unitKind)));
    while (index != 0) {
        DEBUG_ASSERT(index < kUnitKindLast, "CG internal error. index error.");
        ++array[index];
        unitKind &= ~(1u << (index - 1u));
        index = static_cast<uint32>(__builtin_ffs(static_cast<int>(unitKind)));
    }
}

/* Check if a node use a specific unit kind. */
bool AArch64Schedule::IfUseUnitKind(const DepNode &depNode, uint32 index)
{
    uint32 unitKind = depNode.GetUnitKind();
    uint32 idx = static_cast<uint32>(__builtin_ffs(static_cast<int>(unitKind)));
    while (idx != 0) {
        DEBUG_ASSERT(index < kUnitKindLast, "CG internal error. index error.");
        if (idx == index) {
            return true;
        }
        unitKind &= ~(1u << (idx - 1u));
        idx = static_cast<uint32>(__builtin_ffs(static_cast<int>(unitKind)));
    }

    return false;
}

/* A sample schedule according dependence graph only, to verify correctness of dependence graph. */
void AArch64Schedule::RandomTest()
{
    Init();
    nodes.clear();

    while (!readyList.empty()) {
        DepNode *currNode = readyList.back();
        currNode->SetState(kScheduled);
        readyList.pop_back();
        nodes.emplace_back(currNode);

        for (auto succLink : currNode->GetSuccs()) {
            DepNode &succNode = succLink->GetTo();
            bool ready = true;
            for (auto predLink : succNode.GetPreds()) {
                DepNode &predNode = predLink->GetFrom();
                if (predNode.GetState() != kScheduled) {
                    ready = false;
                    break;
                }
            }

            if (ready) {
                DEBUG_ASSERT(succNode.GetState() == kNormal, "succNode must be kNormal");
                readyList.emplace_back(&succNode);
                succNode.SetState(kReady);
            }
        }
    }
}

/* Remove target from readyList. */
void AArch64Schedule::EraseNodeFromReadyList(const DepNode &target)
{
    EraseNodeFromNodeList(target, readyList);
}

/* Remove target from nodeList. */
void AArch64Schedule::EraseNodeFromNodeList(const DepNode &target, MapleVector<DepNode *> &nodeList)
{
    for (auto it = nodeList.begin(); it != nodeList.end(); ++it) {
        if ((*it) == &target) {
            nodeList.erase(it);
            return;
        }
    }

    DEBUG_ASSERT(false, "CG internal error, erase node fail.");
}

/* Dump all node of availableReadyList schedule information in current cycle. */
void AArch64Schedule::DumpDebugInfo(const ScheduleProcessInfo &scheduleInfo)
{
    LogInfo::MapleLogger() << "Current cycle[ " << scheduleInfo.GetCurrCycle() << " ], Available in readyList is : \n";
    for (auto node : scheduleInfo.GetAvailableReadyList()) {
        LogInfo::MapleLogger() << "NodeIndex[ " << node->GetIndex() << " ], Estart[ " << node->GetEStart()
                               << " ], Lstart[ ";
        LogInfo::MapleLogger() << node->GetLStart() << " ], slot[ ";
        LogInfo::MapleLogger() << (node->GetReservation() == nullptr ? "SlotNone"
                                                                     : node->GetReservation()->GetSlotName())
                               << " ], ";
        LogInfo::MapleLogger() << "succNodeNum[ " << node->GetSuccs().size() << " ], ";
        node->GetInsn()->Dump();
        LogInfo::MapleLogger() << '\n';
    }
}

/*
 * Select a node from availableReadyList according to some heuristic rules, then:
 *   1. change targetNode's schedule information;
 *   2. try to add successors of targetNode to readyList;
 *   3. update unscheduled node set, when targetNode is last kNodeTypeSeparator;
 *   4. update AdvanceCycle.
 */
void AArch64Schedule::SelectNode(AArch64ScheduleProcessInfo &scheduleInfo)
{
    auto &availableReadyList = scheduleInfo.GetAvailableReadyList();
    auto it = availableReadyList.begin();
    DepNode *targetNode = *it;
    if (availableReadyList.size() > 1) {
        CalculateMaxUnitKindCount(scheduleInfo);
        if (GetConsiderRegPressure()) {
            UpdateReleaseRegInfo(scheduleInfo);
        }
        ++it;
        for (; it != availableReadyList.end(); ++it) {
            if (CompareDepNode(**it, *targetNode, scheduleInfo)) {
                targetNode = *it;
            }
        }
    }
    /* The priority of free-reg node is higher than pipeline */
    while (!targetNode->IsResourceIdle()) {
        scheduleInfo.IncCurrCycle();
        mad->AdvanceOneCycleForAll();
    }
    if (GetConsiderRegPressure() && !scheduleInfo.IsFirstSeparator()) {
        UpdateLiveRegSet(scheduleInfo, *targetNode);
    }
    /* push target node into scheduled nodes and turn it into kScheduled state */
    scheduleInfo.PushElemIntoScheduledNodes(targetNode);

    EraseNodeFromReadyList(*targetNode);

    if (CGOptions::IsDebugSched()) {
        LogInfo::MapleLogger() << "TargetNode : ";
        targetNode->GetInsn()->Dump();
        LogInfo::MapleLogger() << "\n";
    }

    /* Update readyList. */
    UpdateReadyList(*targetNode, readyList, true);

    if (targetNode->GetType() == kNodeTypeSeparator) {
        /* If target node is separator node, update lastSeparatorIndex and calculate those depNodes's estart and lstart
         * between current separator node  and new Separator node.
         */
        if (!scheduleInfo.IsFirstSeparator()) {
            lastSeparatorIndex += kMaxDependenceNum;
            UpdateELStartsOnCycle(scheduleInfo.GetCurrCycle());
        } else {
            scheduleInfo.ResetIsFirstSeparator();
        }
    }

    UpdateAdvanceCycle(scheduleInfo, *targetNode);
}

void AArch64Schedule::UpdateAdvanceCycle(AArch64ScheduleProcessInfo &scheduleInfo, const DepNode &targetNode) const
{
    switch (targetNode.GetInsn()->GetLatencyType()) {
        case kLtClinit:
            scheduleInfo.SetAdvanceCycle(kClinitAdvanceCycle);
            break;
        case kLtAdrpLdr:
            scheduleInfo.SetAdvanceCycle(kAdrpLdrAdvanceCycle);
            break;
        case kLtClinitTail:
            scheduleInfo.SetAdvanceCycle(kClinitTailAdvanceCycle);
            break;
        default:
            break;
    }

    if ((scheduleInfo.GetAdvanceCycle() == 0) && mad->IsFullIssued()) {
        if (targetNode.GetEStart() > scheduleInfo.GetCurrCycle()) {
            scheduleInfo.SetAdvanceCycle(1 + targetNode.GetEStart() - scheduleInfo.GetCurrCycle());
        } else {
            scheduleInfo.SetAdvanceCycle(1);
        }
    }
}

/*
 * Advance mad's cycle until info's advanceCycle equal zero,
 * and then clear info's availableReadyList.
 */
void AArch64Schedule::UpdateScheduleProcessInfo(AArch64ScheduleProcessInfo &info) const
{
    while (info.GetAdvanceCycle() > 0) {
        info.IncCurrCycle();
        mad->AdvanceOneCycleForAll();
        info.DecAdvanceCycle();
    }
    info.ClearAvailableReadyList();
}

/*
 * Forward traversal readyList, if a node in readyList can be Schedule, add it to availableReadyList.
 * Return true, if availableReadyList is not empty.
 */
bool AArch64Schedule::CheckSchedulable(AArch64ScheduleProcessInfo &info) const
{
    for (auto node : readyList) {
        if (GetConsiderRegPressure()) {
            info.PushElemIntoAvailableReadyList(node);
        } else {
            if (node->IsResourceIdle() && node->GetEStart() <= info.GetCurrCycle()) {
                info.PushElemIntoAvailableReadyList(node);
            }
        }
    }
    return info.AvailableReadyListIsEmpty() ? false : true;
}

/*
 * Calculate estimated machine cycle count for an input node series
 */
int AArch64Schedule::CalSeriesCycles(const MapleVector<DepNode *> &nodes) const
{
    int currentCycle = 0;
    /* after an instruction is issued, the minimum cycle count for the next instruction is 1 */
    int instructionBaseCycleCount = 1;
    std::map<DepNode *, int> scheduledCycleMap;
    for (auto node : nodes) {
        int latencyCycle = 0;
        /* calculate the latest begin time of this node based on its predecessor's issue time and latency */
        for (auto pred : node->GetPreds()) {
            DepNode &from = pred->GetFrom();
            int latency = static_cast<int>(pred->GetLatency());
            int fromCycle = scheduledCycleMap[&from];
            if (fromCycle + latency > latencyCycle) {
                latencyCycle = fromCycle + latency;
            }
        }
        /* the issue time of this node is the max value between the next cycle and latest begin time */
        if (currentCycle + instructionBaseCycleCount >= latencyCycle) {
            currentCycle = currentCycle + instructionBaseCycleCount;
        } else {
            currentCycle = latencyCycle;
        }
        /* record this node's issue cycle */
        scheduledCycleMap[node] = currentCycle;
    }
    return currentCycle;
}

/* After building dependence graph, schedule insns. */
uint32 AArch64Schedule::DoSchedule()
{
    AArch64ScheduleProcessInfo scheduleInfo(nodeSize);
    Init();
    UpdateELStartsOnCycle(scheduleInfo.GetCurrCycle());
    InitLiveRegSet(scheduleInfo);
    while (!readyList.empty()) {
        UpdateScheduleProcessInfo(scheduleInfo);
        /* Check if schedulable */
        if (!CheckSchedulable(scheduleInfo)) {
            /* Advance cycle. */
            scheduleInfo.SetAdvanceCycle(1);
            continue;
        }

        if (scheduleInfo.GetLastUpdateCycle() < scheduleInfo.GetCurrCycle()) {
            scheduleInfo.SetLastUpdateCycle(scheduleInfo.GetCurrCycle());
        }

        if (CGOptions::IsDebugSched()) {
            DumpDebugInfo(scheduleInfo);
        }

        /* Select a node to scheduling */
        SelectNode(scheduleInfo);
    }

    DEBUG_ASSERT(scheduleInfo.SizeOfScheduledNodes() == nodes.size(), "CG internal error, Not all nodes scheduled.");

    nodes.clear();
    (void)nodes.insert(nodes.cbegin(), scheduleInfo.GetScheduledNodes().cbegin(),
                       scheduleInfo.GetScheduledNodes().cend());
    /* the second to last node is the true last node, because the last is kNodeTypeSeparator node */
    DEBUG_ASSERT(nodes.size() - 2 >= 0, "size of nodes should be greater than or equal 2");  // size not less than 2
    return (nodes[nodes.size() - 2]->GetSchedCycle());  // size-2 get the second to last node
}

struct RegisterInfoUnit {
    RegisterInfoUnit() : intRegNum(0), fpRegNum(0), ccRegNum(0) {}
    uint32 intRegNum = 0;
    uint32 fpRegNum = 0;
    uint32 ccRegNum = 0;
};

RegisterInfoUnit GetDepNodeDefType(const DepNode &depNode, const CGFunc &f)
{
    RegisterInfoUnit rIU;
    for (auto defRegNO : depNode.GetDefRegnos()) {
        RegType defRegTy = AArch64ScheduleProcessInfo::GetRegisterType(f, defRegNO);
        if (defRegTy == kRegTyInt) {
            rIU.intRegNum++;
        } else if (defRegTy == kRegTyFloat) {
            rIU.fpRegNum++;
        } else if (defRegTy == kRegTyCc) {
            rIU.ccRegNum++;
            DEBUG_ASSERT(rIU.ccRegNum <= 1, "spill cc reg?");
        } else {
            CHECK_FATAL(false, "NIY aarch64 register type");
        }
    }
    /* call node will not increase reg def pressure */
    if (depNode.GetInsn() != nullptr && depNode.GetInsn()->IsCall()) {
        rIU.intRegNum = 0;
        rIU.fpRegNum = 0;
    }
    return rIU;
}

AArch64Schedule::CSRResult AArch64Schedule::DoCSR(DepNode &node1, DepNode &node2,
                                                  AArch64ScheduleProcessInfo &scheduleInfo) const
{
    RegisterInfoUnit defRIU1 = GetDepNodeDefType(node1, cgFunc);
    RegisterInfoUnit defRIU2 = GetDepNodeDefType(node2, cgFunc);
    /* do not increase callee save pressure before call */
    if (static_cast<int>(scheduleInfo.SizeOfCalleeSaveLiveRegister(true)) >= intCalleeSaveThreshold) {
        if (defRIU1.intRegNum > 0 && defRIU2.intRegNum > 0) {
            CSRResult csrInfo = ScheduleCrossCall(node1, node2);
            if ((csrInfo == kNode1 && defRIU1.intRegNum >= scheduleInfo.GetFreeIntRegs(node1)) ||
                (csrInfo == kNode2 && defRIU2.intRegNum >= scheduleInfo.GetFreeIntRegs(node2))) {
                return csrInfo;
            }
        }
    }
    if (static_cast<int>(scheduleInfo.SizeOfCalleeSaveLiveRegister(false)) >= fpCalleeSaveThreshold) {
        if (defRIU1.fpRegNum > 0 && defRIU2.fpRegNum > 0) {
            CSRResult csrInfo = ScheduleCrossCall(node1, node2);
            if ((csrInfo == kNode1 && defRIU1.fpRegNum >= scheduleInfo.GetFreeFpRegs(node1)) ||
                (csrInfo == kNode2 && defRIU2.fpRegNum >= scheduleInfo.GetFreeFpRegs(node2))) {
                return csrInfo;
            }
        }
    }
    auto findFreeRegNode = [&scheduleInfo, &node1, &node2](bool isInt) -> CSRResult {
        auto freeRegNodes = isInt ? scheduleInfo.GetFreeIntRegNodeSet() : scheduleInfo.GetFreeFpRegNodeSet();
        if (freeRegNodes.find(&node1) != freeRegNodes.end() && freeRegNodes.find(&node2) == freeRegNodes.end()) {
            return kNode1;
        }
        if (freeRegNodes.find(&node1) == freeRegNodes.end() && freeRegNodes.find(&node2) != freeRegNodes.end()) {
            return kNode2;
        }
        return kDoCSP;
    };
    if (static_cast<int>(scheduleInfo.SizeOfIntLiveRegSet()) >= intRegPressureThreshold) {
        if (findFreeRegNode(true) != kDoCSP) {
            return findFreeRegNode(true);
        }
    }
    if (static_cast<int>(scheduleInfo.SizeOfFpLiveRegSet()) >= fpRegPressureThreshold) {
        if (findFreeRegNode(false) != kDoCSP) {
            return findFreeRegNode(false);
        }
    }

    bool canDoCSPFurther = false;
    if (static_cast<int>(scheduleInfo.SizeOfIntLiveRegSet()) >= intRegPressureThreshold) {
        if (defRIU1.intRegNum != defRIU2.intRegNum) {
            return defRIU1.intRegNum < defRIU2.intRegNum ? kNode1 : kNode2;
        } else {
            canDoCSPFurther = defRIU1.intRegNum == 0;
        }
    }
    if (static_cast<int>(scheduleInfo.SizeOfFpLiveRegSet()) >= fpRegPressureThreshold) {
        if (defRIU1.fpRegNum != defRIU2.fpRegNum) {
            return defRIU1.fpRegNum < defRIU2.fpRegNum ? kNode1 : kNode2;
        } else {
            canDoCSPFurther = (defRIU1.fpRegNum == 0 && canDoCSPFurther);
        }
    }
    /* if both nodes are going to increase reg pressure, do not do CSP further */
    return canDoCSPFurther ? kDoCSP : (node1.GetInsn()->GetId() < node2.GetInsn()->GetId() ? kNode1 : kNode2);
}

AArch64Schedule::CSRResult AArch64Schedule::ScheduleCrossCall(const DepNode &node1, const DepNode &node2) const
{
    uint32 node1ID = node1.GetInsn()->GetId();
    uint32 node2ID = node2.GetInsn()->GetId();
    bool order = node1ID < node2ID; /* true -- node1 before node2  false -- node1 after node2 */
    Insn *beginInsn = order ? node1.GetInsn() : node2.GetInsn();
    uint32 finialId = order ? node2ID : node1ID;
    for (Insn *checkInsn = beginInsn; (checkInsn != nullptr && checkInsn->GetId() <= finialId);
         checkInsn = checkInsn->GetNextMachineInsn()) {
        if (checkInsn->IsCall()) {
            return order ? kNode1 : kNode2;
        }
    }
    return kDoCSP;
};

/*
 * Comparing priorities of node1 and node2 according to some heuristic rules
 * return true if node1's priority is higher
 * crp -- consider reg pressure
 */
bool AArch64Schedule::CompareDepNode(DepNode &node1, DepNode &node2, AArch64ScheduleProcessInfo &scheduleInfo) const
{
    /*
     * strategy CSR -- code schedule for register pressure
     * if pressure is above the threshold, select the node which can reduce register pressure
     */
    if (GetConsiderRegPressure()) {
        switch (DoCSR(node1, node2, scheduleInfo)) {
            case kNode1:
                return true;
            case kNode2:
                return false;
            default:
                break;
        }
    }
    /* strategy CSP -- code schedule for CPU pipeline */
    /* less LStart first */
    if (node1.GetLStart() != node2.GetLStart()) {
        return node1.GetLStart() < node2.GetLStart();
    }

    /* max unit kind use */
    bool use1 = IfUseUnitKind(node1, maxUnitIndex);
    bool use2 = IfUseUnitKind(node2, maxUnitIndex);
    if (use1 != use2) {
        return use1;
    }

    /* slot0 first */
    SlotType slotType1 = node1.GetReservation()->GetSlot();
    SlotType slotType2 = node2.GetReservation()->GetSlot();
    if (slotType1 == kSlots) {
        slotType1 = kSlot0;
    }
    if (slotType2 == kSlots) {
        slotType2 = kSlot0;
    }
    if (slotType1 != slotType2) {
        return slotType1 < slotType2;
    }

    /* more succNodes fisrt */
    if (node1.GetSuccs().size() != node2.GetSuccs().size()) {
        return node1.GetSuccs().size() > node2.GetSuccs().size();
    }

    /* default order */
    return node1.GetInsn()->GetId() < node2.GetInsn()->GetId();
}

/*
 * Calculate number of every unit that used by avaliableReadyList's nodes and save the max in maxUnitIndex
 */
void AArch64Schedule::CalculateMaxUnitKindCount(ScheduleProcessInfo &scheduleInfo) const
{
    uint32 unitKindCount[kUnitKindLast] = {0};
    for (auto node : scheduleInfo.GetAvailableReadyList()) {
        CountUnitKind(*node, unitKindCount, kUnitKindLast);
    }

    uint32 maxCount = 0;
    maxUnitIndex = 0;
    for (size_t i = 1; i < kUnitKindLast; ++i) {
        if (maxCount < unitKindCount[i]) {
            maxCount = unitKindCount[i];
            maxUnitIndex = i;
        }
    }
}

/*
 * Update the release reg node set
 * When node in this set is scheduled, register pressure can be reduced
 */
void AArch64Schedule::UpdateReleaseRegInfo(AArch64ScheduleProcessInfo &scheduleInfo)
{
    auto &availableReadyList = scheduleInfo.GetAvailableReadyList();
    scheduleInfo.ClearALLFreeRegNodeSet();
    /* Traverse availableReadyList and add those can reduce register pressure to release reg node set */
    for (auto node : availableReadyList) {
        std::set<regno_t> freeRegNO = CanFreeRegister(*node);
        if (!freeRegNO.empty()) {
            scheduleInfo.VaryFreeRegSet(cgFunc, freeRegNO, *node);
        }
    }
}

/*
 * return registers which an instruction can release after being scheduled
 */
std::set<regno_t> AArch64Schedule::CanFreeRegister(const DepNode &node) const
{
    std::set<regno_t> freeRegSet;
    for (auto reg : node.GetUseRegnos()) {
        if (RegPressureSchedule::IsLastUse(node, reg)) {
            freeRegSet.emplace(reg);
        }
    }
    return freeRegSet;
}

/*
 * After an instruction is scheduled, update live reg set
 */
void AArch64Schedule::UpdateLiveRegSet(AArch64ScheduleProcessInfo &scheduleInfo, const DepNode &node)
{
    /* dealing with def reg, add def reg into the live reg set */
    size_t i = 1;
    for (auto &defReg : node.GetDefRegnos()) {
        if (scheduleInfo.FindIntLiveReg(defReg) == 0 && scheduleInfo.FindFpLiveReg(defReg) == 0) {
            scheduleInfo.VaryLiveRegSet(cgFunc, defReg, true);
        }
        /* delete dead def reg from live reg set because its live range is only 1 cycle */
        if (node.GetRegDefs(i) == nullptr && liveOutRegNo.find(defReg) == liveOutRegNo.end()) {
            scheduleInfo.VaryLiveRegSet(cgFunc, defReg, false);
        }
        ++i;
    }
    /* dealing with use reg, delete use reg from live reg set if this instruction is last use of it */
    for (auto &useReg : node.GetUseRegnos()) {
        if (RegPressureSchedule::IsLastUse(node, useReg)) {
            if ((scheduleInfo.FindIntLiveReg(useReg) != 0 || scheduleInfo.FindFpLiveReg(useReg) != 0) &&
                liveOutRegNo.find(useReg) == liveOutRegNo.end()) {
                scheduleInfo.VaryLiveRegSet(cgFunc, useReg, false);
            }
        }
    }
}

/*
 * Initialize the live reg set based on the live in reg information
 */
void AArch64Schedule::InitLiveRegSet(AArch64ScheduleProcessInfo &scheduleInfo)
{
    if (GetConsiderRegPressure()) {
        for (auto reg : liveInRegNo) {
            scheduleInfo.VaryLiveRegSet(cgFunc, reg, true);
        }
    }
}

/*
 * A simulated schedule:
 * scheduling instruction in original order to calculate original execute cycles.
 */
uint32 AArch64Schedule::SimulateOnly()
{
    uint32 currCycle = 0;
    uint32 advanceCycle = 0;
    Init();

    for (uint32 i = 0; i < nodes.size();) {
        while (advanceCycle > 0) {
            ++currCycle;
            mad->AdvanceOneCycleForAll();
            --advanceCycle;
        }

        DepNode *targetNode = nodes[i];
        if ((currCycle >= targetNode->GetEStart()) && targetNode->IsResourceIdle()) {
            targetNode->SetSimulateCycle(currCycle);
            targetNode->OccupyRequiredUnits();

            /* Update estart. */
            for (auto succLink : targetNode->GetSuccs()) {
                DepNode &succNode = succLink->GetTo();
                uint32 eStart = currCycle + succLink->GetLatency();
                if (succNode.GetEStart() < eStart) {
                    succNode.SetEStart(eStart);
                }
            }

            if (CGOptions::IsDebugSched()) {
                LogInfo::MapleLogger() << "[Simulate] TargetNode : ";
                targetNode->GetInsn()->Dump();
                LogInfo::MapleLogger() << "\n";
            }

            switch (targetNode->GetInsn()->GetLatencyType()) {
                case kLtClinit:
                    advanceCycle = kClinitAdvanceCycle;
                    break;
                case kLtAdrpLdr:
                    advanceCycle = kAdrpLdrAdvanceCycle;
                    break;
                case kLtClinitTail:
                    advanceCycle = kClinitTailAdvanceCycle;
                    break;
                default:
                    break;
            }

            ++i;
        } else {
            advanceCycle = 1;
        }
    }
    /* the second to last node is the true last node, because the last is kNodeTypeSeparator nod */
    DEBUG_ASSERT(nodes.size() - kSecondToLastNode >= 0, "size of nodes should be greater than or equal 2");
    return (nodes[nodes.size() - kSecondToLastNode]->GetSimulateCycle());
}

/* Restore dependence graph to normal CGIR. */
void AArch64Schedule::FinalizeScheduling(BB &bb, const DepAnalysis &depAnalysis)
{
    bb.ClearInsns();

    const Insn *prevLocInsn = (bb.GetPrev() != nullptr ? bb.GetPrev()->GetLastLoc() : nullptr);
    for (auto node : nodes) {
        /* Append comments first. */
        for (auto comment : node->GetComments()) {
            if (comment->GetPrev() != nullptr && comment->GetPrev()->IsDbgInsn()) {
                bb.AppendInsn(*comment->GetPrev());
            }
            bb.AppendInsn(*comment);
        }
        /* Append cfi instructions. */
        for (auto cfi : node->GetCfiInsns()) {
            bb.AppendInsn(*cfi);
        }
        /* Append insn. */
        if (!node->GetClinitInsns().empty()) {
            for (auto clinit : node->GetClinitInsns()) {
                bb.AppendInsn(*clinit);
            }
        } else if (node->GetType() == kNodeTypeNormal) {
            if (node->GetInsn()->GetPrev() != nullptr && node->GetInsn()->GetPrev()->IsDbgInsn()) {
                bb.AppendInsn(*node->GetInsn()->GetPrev());
            }
            bb.AppendInsn(*node->GetInsn());
        }
    }
    bb.SetLastLoc(prevLocInsn);

    for (auto lastComment : depAnalysis.GetLastComments()) {
        bb.AppendInsn(*lastComment);
    }
}

/* For every node of nodes, update it's bruteForceSchedCycle. */
void AArch64Schedule::UpdateBruteForceSchedCycle()
{
    for (auto node : nodes) {
        node->SetBruteForceSchedCycle(node->GetSchedCycle());
    }
}

/* Recursively schedule all of the possible node. */
void AArch64Schedule::IterateBruteForce(DepNode &targetNode, MapleVector<DepNode *> &readyList, uint32 currCycle,
                                        MapleVector<DepNode *> &scheduledNodes, uint32 &maxCycleCount,
                                        MapleVector<DepNode *> &optimizedScheduledNodes)
{
    /* Save states. */
    constexpr int32 unitSize = 31;
    DEBUG_ASSERT(unitSize == mad->GetAllUnitsSize(), "CG internal error.");
    constexpr int32 bitSetLength = 32;
    std::vector<std::bitset<bitSetLength>> occupyTable;
    occupyTable.resize(unitSize, 0);
    mad->SaveStates(occupyTable, unitSize);

    /* Schedule targetNode first. */
    targetNode.SetState(kScheduled);
    targetNode.SetSchedCycle(currCycle);
    scheduledNodes.emplace_back(&targetNode);

    MapleVector<DepNode *> tempList = readyList;
    EraseNodeFromNodeList(targetNode, tempList);
    targetNode.OccupyRequiredUnits();

    /* Update readyList. */
    UpdateReadyList(targetNode, tempList, true);

    if (targetNode.GetType() == kNodeTypeSeparator) {
        /* If target node is separator node, update lastSeparatorIndex. */
        lastSeparatorIndex += kMaxDependenceNum;
    }

    if (tempList.empty()) {
        DEBUG_ASSERT(scheduledNodes.size() == nodes.size(), "CG internal error, Not all nodes scheduled.");
        if (currCycle < maxCycleCount) {
            maxCycleCount = currCycle;
            UpdateBruteForceSchedCycle();
            optimizedScheduledNodes = scheduledNodes;
        }
    } else {
        uint32 advanceCycle = 0;
        switch (targetNode.GetInsn()->GetLatencyType()) {
            case kLtClinit:
                advanceCycle = kClinitAdvanceCycle;
                break;
            case kLtAdrpLdr:
                advanceCycle = kAdrpLdrAdvanceCycle;
                break;
            case kLtClinitTail:
                advanceCycle = kClinitTailAdvanceCycle;
                break;
            default:
                break;
        }

        do {
            std::vector<DepNode *> availableReadyList;
            std::vector<DepNode *> tempAvailableList;
            while (advanceCycle > 0) {
                ++currCycle;
                mad->AdvanceOneCycleForAll();
                --advanceCycle;
            }
            /* Check EStart. */
            for (auto node : tempList) {
                if (node->GetEStart() <= currCycle) {
                    tempAvailableList.emplace_back(node);
                }
            }

            if (tempAvailableList.empty()) {
                /* Advance cycle. */
                advanceCycle = 1;
                continue;
            }

            /* Check if schedulable */
            for (auto node : tempAvailableList) {
                if (node->IsResourceIdle()) {
                    availableReadyList.emplace_back(node);
                }
            }

            if (availableReadyList.empty()) {
                /* Advance cycle. */
                advanceCycle = 1;
                continue;
            }

            for (auto node : availableReadyList) {
                IterateBruteForce(*node, tempList, currCycle, scheduledNodes, maxCycleCount, optimizedScheduledNodes);
            }

            break;
        } while (true);
    }

    /*
     * Recover states.
     * Restore targetNode first.
     */
    targetNode.SetState(kReady);
    targetNode.SetSchedCycle(0);
    scheduledNodes.pop_back();
    mad->RestoreStates(occupyTable, unitSize);

    /* Update readyList. */
    for (auto succLink : targetNode.GetSuccs()) {
        DepNode &succNode = succLink->GetTo();
        succNode.IncreaseValidPredsSize();
        succNode.SetState(kNormal);
    }

    if (targetNode.GetType() == kNodeTypeSeparator) {
        /* If target node is separator node, update lastSeparatorIndex. */
        lastSeparatorIndex -= kMaxDependenceNum;
    }
}

/*
 * Brute force schedule:
 * Finding all possibile schedule list of current bb, and calculate every list's execute cycles,
 * return the optimal schedule list and it's cycles.
 */
uint32 AArch64Schedule::DoBruteForceSchedule()
{
    MapleVector<DepNode *> scheduledNodes(alloc.Adapter());
    MapleVector<DepNode *> optimizedScheduledNodes(alloc.Adapter());

    uint32 currCycle = 0;
    uint32 maxCycleCount = 0xFFFFFFFF;
    Init();

    /* Schedule First separator. */
    DepNode *targetNode = readyList.front();
    targetNode->SetState(kScheduled);
    targetNode->SetSchedCycle(currCycle);
    scheduledNodes.emplace_back(targetNode);
    readyList.clear();

    /* Update readyList. */
    UpdateReadyList(*targetNode, readyList, false);

    DEBUG_ASSERT(targetNode->GetType() == kNodeTypeSeparator, "The first node should be separator node.");
    DEBUG_ASSERT(!readyList.empty(), "readyList should not be empty.");

    for (auto targetNodeTemp : readyList) {
        IterateBruteForce(*targetNodeTemp, readyList, currCycle, scheduledNodes, maxCycleCount,
                          optimizedScheduledNodes);
    }

    nodes = optimizedScheduledNodes;
    return maxCycleCount;
}

/*
 * Update ready list after the targetNode has been scheduled.
 * For every targetNode's successor, if it's all predecessors have been scheduled,
 * add it to ready list and update it's information (like state, estart).
 */
void AArch64Schedule::UpdateReadyList(DepNode &targetNode, MapleVector<DepNode *> &readyList, bool updateEStart)
{
    for (auto succLink : targetNode.GetSuccs()) {
        DepNode &succNode = succLink->GetTo();
        succNode.DecreaseValidPredsSize();
        if (succNode.GetValidPredsSize() == 0) {
            readyList.emplace_back(&succNode);
            succNode.SetState(kReady);

            /* Set eStart. */
            if (updateEStart) {
                uint32 maxEstart = 0;
                for (auto predLink : succNode.GetPreds()) {
                    DepNode &predNode = predLink->GetFrom();
                    uint32 eStart = predNode.GetSchedCycle() + predLink->GetLatency();
                    maxEstart = (maxEstart < eStart ? eStart : maxEstart);
                }
                succNode.SetEStart(maxEstart);
            }
        }
    }
}

/* For every node of nodes, dump it's Depdence information. */
void AArch64Schedule::DumpDepGraph(const MapleVector<DepNode *> &nodes) const
{
    for (auto node : nodes) {
        depAnalysis->DumpDepNode(*node);
        LogInfo::MapleLogger() << "---------- preds ----------"
                               << "\n";
        for (auto pred : node->GetPreds()) {
            depAnalysis->DumpDepLink(*pred, &(pred->GetFrom()));
        }
        LogInfo::MapleLogger() << "---------- succs ----------"
                               << "\n";
        for (auto succ : node->GetSuccs()) {
            depAnalysis->DumpDepLink(*succ, &(succ->GetTo()));
        }
        LogInfo::MapleLogger() << "---------------------------"
                               << "\n";
    }
}

/* For every node of nodes, dump it's schedule time according simulate type and instruction information. */
void AArch64Schedule::DumpScheduleResult(const MapleVector<DepNode *> &nodes, SimulateType type) const
{
    for (auto node : nodes) {
        LogInfo::MapleLogger() << "cycle[ ";
        switch (type) {
            case kListSchedule:
                LogInfo::MapleLogger() << node->GetSchedCycle();
                break;
            case kBruteForce:
                LogInfo::MapleLogger() << node->GetBruteForceSchedCycle();
                break;
            case kSimulateOnly:
                LogInfo::MapleLogger() << node->GetSimulateCycle();
                break;
        }
        LogInfo::MapleLogger() << " ]  ";
        node->GetInsn()->Dump();
        LogInfo::MapleLogger() << "\n";
    }
}

/* Print bb's dependence dot graph information to a file. */
void AArch64Schedule::GenerateDot(const BB &bb, const MapleVector<DepNode *> &nodes) const
{
    std::streambuf *coutBuf = std::cout.rdbuf(); /* keep original cout buffer */
    std::ofstream dgFile;
    std::streambuf *buf = dgFile.rdbuf();
    std::cout.rdbuf(buf);

    /* construct the file name */
    std::string fileName;
    fileName.append(phaseName);
    fileName.append("_");
    fileName.append(cgFunc.GetName());
    fileName.append("_BB");
    auto str = std::to_string(bb.GetId());
    fileName.append(str);
    fileName.append("_dep_graph.dot");

    dgFile.open(FileUtils::GetRealPath(fileName), std::ios::trunc);
    if (!dgFile.is_open()) {
        LogInfo::MapleLogger(kLlWarn) << "fileName:" << fileName << " open failure.\n";
        return;
    }
    dgFile << "digraph {\n";
    for (auto node : nodes) {
        for (auto succ : node->GetSuccs()) {
            dgFile << "insn" << node->GetInsn() << " -> "
                   << "insn" << succ->GetTo().GetInsn();
            dgFile << " [";
            if (succ->GetDepType() == kDependenceTypeTrue) {
                dgFile << "color=red,";
            }
            dgFile << "label= \"" << succ->GetLatency() << "\"";
            dgFile << "];\n";
        }
    }

    for (auto node : nodes) {
        MOperator mOp = node->GetInsn()->GetMachineOpcode();
        const InsnDesc *md = &AArch64CG::kMd[mOp];
        dgFile << "insn" << node->GetInsn() << "[";
        dgFile << "shape=box,label= \" " << node->GetInsn()->GetId() << ":\n";
        dgFile << "{ ";
        dgFile << md->name << "\n";
        dgFile << "}\"];\n";
    }
    dgFile << "}\n";
    dgFile.flush();
    dgFile.close();
    std::cout.rdbuf(coutBuf);
}

RegType AArch64ScheduleProcessInfo::GetRegisterType(const CGFunc &f, regno_t regNO)
{
    if (AArch64isa::IsPhysicalRegister(regNO)) {
        if (AArch64isa::IsGPRegister(static_cast<AArch64reg>(regNO))) {
            return kRegTyInt;
        } else if (AArch64isa::IsFPSIMDRegister(static_cast<AArch64reg>(regNO))) {
            return kRegTyFloat;
        } else {
            CHECK_FATAL(false, "unknown physical reg");
        }
    } else {
        RegOperand *curRegOpnd = f.GetVirtualRegisterOperand(regNO);
        DEBUG_ASSERT(curRegOpnd != nullptr, "register which is not physical and virtual");
        return curRegOpnd->GetRegisterType();
    }
}

void AArch64ScheduleProcessInfo::VaryLiveRegSet(const CGFunc &f, regno_t regNO, bool isInc)
{
    RegType registerTy = GetRegisterType(f, regNO);
    if (registerTy == kRegTyInt || registerTy == kRegTyVary) {
        isInc ? IncIntLiveRegSet(regNO) : DecIntLiveRegSet(regNO);
    } else if (registerTy == kRegTyFloat) {
        isInc ? IncFpLiveRegSet(regNO) : DecFpLiveRegSet(regNO);
    }
    /* consider other type register */
}

void AArch64ScheduleProcessInfo::VaryFreeRegSet(const CGFunc &f, std::set<regno_t> regNOs, DepNode &node)
{
    for (auto regNO : regNOs) {
        RegType registerTy = GetRegisterType(f, regNO);
        if (registerTy == kRegTyInt || registerTy == kRegTyVary /* memory base register must be int */) {
            IncFreeIntRegNode(node);
        } else if (registerTy == kRegTyFloat) {
            IncFreeFpRegNode(node);
        } else if (registerTy == kRegTyCc) {
            /* do not count CC reg */
            return;
        } else {
            /* consider other type register */
            CHECK_FATAL(false, "do not support this type of register");
        }
    }
}

/* Do brute force scheduling and dump scheduling information */
void AArch64Schedule::BruteForceScheduling(const BB &bb)
{
    LogInfo::MapleLogger() << "\n\n$$ Function: " << cgFunc.GetName();
    LogInfo::MapleLogger() << "\n    BB id = " << bb.GetId() << "; nodes.size = " << nodes.size() << "\n";

    constexpr uint32 maxBruteForceNum = 50;
    if (nodes.size() < maxBruteForceNum) {
        GenerateDot(bb, nodes);
        uint32 maxBruteForceCycle = DoBruteForceSchedule();
        MapleVector<DepNode *> bruteNodes = nodes;
        uint32 maxSchedCycle = DoSchedule();
        if (maxBruteForceCycle < maxSchedCycle) {
            LogInfo::MapleLogger() << "maxBruteForceCycle = " << maxBruteForceCycle << "; maxSchedCycle = ";
            LogInfo::MapleLogger() << maxSchedCycle << "\n";
            LogInfo::MapleLogger() << "\n    ## Dump dependence graph ##    "
                                   << "\n";
            DumpDepGraph(nodes);
            LogInfo::MapleLogger() << "\n    ** Dump bruteForce scheduling result."
                                   << "\n";
            DumpScheduleResult(bruteNodes, kBruteForce);
            LogInfo::MapleLogger() << "\n    ^^ Dump list scheduling result."
                                   << "\n";
            DumpScheduleResult(nodes, kListSchedule);
        }
    } else {
        LogInfo::MapleLogger() << "Skip BruteForce scheduling."
                               << "\n";
        DoSchedule();
    }
}

/* Do simulate scheduling and dump scheduling information */
void AArch64Schedule::SimulateScheduling(const BB &bb)
{
    uint32 originCycle = SimulateOnly();
    MapleVector<DepNode *> oldNodes = nodes;
    uint32 schedCycle = DoSchedule();
    if (originCycle < schedCycle) {
        LogInfo::MapleLogger() << "Worse cycle [ " << (schedCycle - originCycle) << " ]; ";
        LogInfo::MapleLogger() << "originCycle = " << originCycle << "; schedCycle = ";
        LogInfo::MapleLogger() << schedCycle << "; nodes.size = " << nodes.size();
        LogInfo::MapleLogger() << ";    $$ Function: " << cgFunc.GetName();
        LogInfo::MapleLogger() << ";    BB id = " << bb.GetId() << "\n";
        LogInfo::MapleLogger() << "\n    ** Dump original result."
                               << "\n";
        DumpScheduleResult(oldNodes, kSimulateOnly);
        LogInfo::MapleLogger() << "\n    ^^ Dump list scheduling result."
                               << "\n";
        DumpScheduleResult(nodes, kListSchedule);
    } else if (originCycle > schedCycle) {
        LogInfo::MapleLogger() << "Advance cycle [ " << (originCycle - schedCycle) << " ]; ";
        LogInfo::MapleLogger() << "originCycle = " << originCycle << "; schedCycle = ";
        LogInfo::MapleLogger() << schedCycle << "; nodes.size = " << nodes.size();
        LogInfo::MapleLogger() << ";    $$ Function: " << cgFunc.GetName();
        LogInfo::MapleLogger() << ";    BB id = " << bb.GetId() << "\n";
    } else {
        LogInfo::MapleLogger() << "Equal cycle [ 0 ]; originCycle = " << originCycle;
        LogInfo::MapleLogger() << " ], ignore. nodes.size = " << nodes.size() << "\n";
    }
}

/*
 * A local list scheduling.
 * Schedule insns in basic blocks.
 */
void AArch64Schedule::ListScheduling(bool beforeRA)
{
    InitIDAndLoc();

    mad = Globals::GetInstance()->GetMAD();
    if (beforeRA) {
        RegPressure::SetMaxRegClassNum(kRegisterLast);
    }
    depAnalysis = memPool.New<AArch64DepAnalysis>(cgFunc, memPool, *mad, beforeRA);
    FOR_ALL_BB(bb, &cgFunc)
    {
        if (bb->IsUnreachable()) {
            continue;
        }
        if (bb->IsAtomicBuiltInBB()) {
            continue;
        }
        depAnalysis->Run(*bb, nodes);

        if (LIST_SCHED_DUMP_REF) {
            GenerateDot(*bb, nodes);
            DumpDepGraph(nodes);
        }
        if (beforeRA) {
            liveInRegNo = bb->GetLiveInRegNO();
            liveOutRegNo = bb->GetLiveOutRegNO();
            if (bb->GetKind() != BB::kBBReturn) {
                SetConsiderRegPressure();
                DoSchedule();
            } else {
                RegPressureScheduling(*bb, nodes);
            }
        } else {
            ClinitPairOpt();
            MemoryAccessPairOpt();
            if (CGOptions::IsDruteForceSched()) {
                BruteForceScheduling(*bb);
            } else if (CGOptions::IsSimulateSched()) {
                SimulateScheduling(*bb);
            } else {
                DoSchedule();
            }
        }

        FinalizeScheduling(*bb, *depAnalysis);
    }
}
} /* namespace maplebe */
