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

#include "base_schedule.h"
#ifdef TARGAARCH64
#include "aarch64_cg.h"
#endif

namespace maplebe {
// Set insnId to guarantee default priority,
// Set locInsn to maintain debug info
void BaseSchedule::InitInsnIdAndLocInsn()
{
    uint32 id = 0;
    FOR_ALL_BB(bb, &cgFunc)
    {
        bb->SetLastLoc(bb->GetPrev() ? bb->GetPrev()->GetLastLoc() : nullptr);
        FOR_BB_INSNS(insn, bb)
        {
            if (insn->IsMachineInstruction()) {
                insn->SetId(id++);
            }
#if defined(DEBUG) && DEBUG
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

void BaseSchedule::InitMachineInsnNum(CDGNode &cdgNode) const
{
    uint32 insnNum = 0;
    BB *curBB = cdgNode.GetBB();
    CHECK_FATAL(curBB != nullptr, "get bb from cdgNode failed");
    FOR_BB_INSNS_CONST(insn, curBB)
    {
        if (insn->IsMachineInstruction()) {
            insnNum++;
        }
    }
    cdgNode.SetInsnNum(insnNum);
}

void BaseSchedule::InitInRegion(CDGRegion &region) const
{
    // Init valid dependency size for scheduling
    for (auto *cdgNode : region.GetRegionNodes()) {
        for (auto *depNode : cdgNode->GetAllDataNodes()) {
            depNode->SetState(kNormal);
            depNode->SetValidPredsSize(static_cast<uint32>(depNode->GetPreds().size()));
            depNode->SetValidSuccsSize(static_cast<uint32>(depNode->GetSuccs().size()));
        }
    }
}

uint32 BaseSchedule::CaculateOriginalCyclesOfBB(CDGNode &cdgNode) const
{
    BB *bb = cdgNode.GetBB();
    DEBUG_ASSERT(bb != nullptr, "get bb from cdgNode failed");

    FOR_BB_INSNS(insn, bb)
    {
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        DepNode *depNode = insn->GetDepNode();
        DEBUG_ASSERT(depNode != nullptr, "get depNode from insn failed");
        // init
        depNode->SetSimulateState(kStateUndef);
        depNode->SetSimulateIssueCycle(0);
    }

    MAD *mad = Globals::GetInstance()->GetMAD();
    std::vector<DepNode *> runningList;
    uint32 curCycle = 0;
    FOR_BB_INSNS(insn, bb)
    {
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        DepNode *depNode = insn->GetDepNode();
        ASSERT_NOT_NULL(depNode);
        // Currently, do not consider the conflicts of resource
        if (depNode->GetPreds().empty()) {
            depNode->SetSimulateState(kRunning);
            depNode->SetSimulateIssueCycle(curCycle);
            (void)runningList.emplace_back(depNode);
            continue;
        }
        // Update depNode info on curCycle
        for (auto *runningNode : runningList) {
            if (runningNode->GetSimulateState() == kRunning &&
                (static_cast<int>(curCycle) - static_cast<int>(runningNode->GetSimulateIssueCycle()) >=
                 runningNode->GetReservation()->GetLatency())) {
                runningNode->SetSimulateState(kRetired);
            }
        }
        // Update curCycle by curDepNode
        uint32 maxWaitTime = 0;
        for (auto *predLink : depNode->GetPreds()) {
            ASSERT_NOT_NULL(predLink);
            DepNode &predNode = predLink->GetFrom();
            Insn *predInsn = predNode.GetInsn();
            ASSERT_NOT_NULL(predInsn);
            // Only calculate latency of true dependency in local BB
            if (predLink->GetDepType() == kDependenceTypeTrue && predInsn->GetBB() == bb &&
                predNode.GetSimulateState() == kRunning) {
                DEBUG_ASSERT(curCycle >= predNode.GetSimulateIssueCycle(), "the state of dependency node is wrong");
                if ((static_cast<int>(curCycle) - static_cast<int>(predNode.GetSimulateIssueCycle())) <
                    mad->GetLatency(*predInsn, *insn)) {
                    int actualLatency =
                        mad->GetLatency(*predInsn, *insn) -
                        (static_cast<int>(curCycle) - static_cast<int>(predNode.GetSimulateIssueCycle()));
                    maxWaitTime = std::max(maxWaitTime, static_cast<uint32>(actualLatency));
                }
            }
        }
        curCycle += maxWaitTime;
        depNode->SetSimulateState(kRunning);
        depNode->SetSimulateIssueCycle(curCycle);
    }
    return curCycle;
}

void BaseSchedule::DumpRegionInfoBeforeSchedule(CDGRegion &region) const
{
    LogInfo::MapleLogger() << "---------------- Schedule Region_" << region.GetRegionId() << " ----------------\n\n";
    LogInfo::MapleLogger() << "##  total number of blocks: " << region.GetRegionNodeSize() << "\n\n";
    LogInfo::MapleLogger() << "##  topological order of blocks in region: {";
    for (uint32 i = 0; i < region.GetRegionNodeSize(); ++i) {
        BB *bb = region.GetRegionNodes()[i]->GetBB();
        DEBUG_ASSERT(bb != nullptr, "get bb from cdgNode failed");
        LogInfo::MapleLogger() << "bb_" << bb->GetId();
        if (i != region.GetRegionNodeSize() - 1) {
            LogInfo::MapleLogger() << ", ";
        } else {
            LogInfo::MapleLogger() << "}\n\n";
        }
    }
}

void BaseSchedule::DumpCDGNodeInfoBeforeSchedule(CDGNode &cdgNode) const
{
    BB *curBB = cdgNode.GetBB();
    DEBUG_ASSERT(curBB != nullptr, "get bb from cdgNode failed");
    LogInfo::MapleLogger() << "= = = = = = = = = = = = = = = = = = = = = = = = = = = =\n\n";
    LogInfo::MapleLogger() << "##  -- bb_" << curBB->GetId() << " before schedule --\n\n";
    LogInfo::MapleLogger() << "    >> candidates info of bb_" << curBB->GetId() << " <<\n\n";
    curBB->Dump();
    LogInfo::MapleLogger() << "\n";
    DumpInsnInfoByScheduledOrder(cdgNode);
}

void BaseSchedule::DumpCDGNodeInfoAfterSchedule(CDGNode &cdgNode) const
{
    BB *curBB = cdgNode.GetBB();
    DEBUG_ASSERT(curBB != nullptr, "get bb from cdgNode failed");
    LogInfo::MapleLogger() << "\n";
    LogInfo::MapleLogger() << "##  -- bb_" << curBB->GetId() << " after schedule --\n";
    LogInfo::MapleLogger() << "    ideal total cycles: "
                           << (doDelayHeu ? listScheduler->GetMaxDelay() : listScheduler->GetMaxLStart()) << "\n";
    LogInfo::MapleLogger() << "    sched total cycles: " << listScheduler->GetCurrCycle() << "\n\n";
    curBB->Dump();
    LogInfo::MapleLogger() << "  = = = = = = = = = = = = = = = = = = = = = = = = = = =\n\n\n";
}

void BaseSchedule::DumpInsnInfoByScheduledOrder(CDGNode &cdgNode) const
{
    // For print table log with unequal widths
    int printWidth1 = 6;
    int printWidth2 = 8;
    int printWidth3 = 14;
    LogInfo::MapleLogger() << "    ------------------------------------------------\n";
    (void)LogInfo::MapleLogger().fill(' ');
    LogInfo::MapleLogger() << "      " << std::setiosflags(std::ios::left) << std::setw(printWidth1) << "insn"
                           << std::resetiosflags(std::ios::left) << std::setiosflags(std::ios::right)
                           << std::setw(printWidth2) << "mop" << std::resetiosflags(std::ios::right)
                           << std::setiosflags(std::ios::right) << std::setw(printWidth2) << "bb"
                           << std::resetiosflags(std::ios::right) << std::setiosflags(std::ios::right)
                           << std::setw(printWidth3) << "succs(latency)" << std::resetiosflags(std::ios::right) << "\n";
    LogInfo::MapleLogger() << "    ------------------------------------------------\n";
    BB *curBB = cdgNode.GetBB();
    DEBUG_ASSERT(curBB != nullptr, "get bb from cdgNode failed");
    FOR_BB_INSNS_CONST(insn, curBB)
    {
        if (!insn->IsMachineInstruction()) {
            continue;
        }
        LogInfo::MapleLogger() << "      " << std::setiosflags(std::ios::left) << std::setw(printWidth1)
                               << insn->GetId() << std::resetiosflags(std::ios::left)
                               << std::setiosflags(std::ios::right) << std::setw(printWidth2);
        const InsnDesc *md = nullptr;
        if (Globals::GetInstance()->GetTarget()->GetTargetMachine()->isAArch64()) {
            md = &AArch64CG::kMd[insn->GetMachineOpcode()];
        }
        CHECK_NULL_FATAL(md);
        LogInfo::MapleLogger() << md->name << std::resetiosflags(std::ios::right) << std::setiosflags(std::ios::right)
                               << std::setw(printWidth2) << curBB->GetId() << std::resetiosflags(std::ios::right)
                               << std::setiosflags(std::ios::right) << std::setw(printWidth3);
        const DepNode *depNode = insn->GetDepNode();
        DEBUG_ASSERT(depNode != nullptr, "get depNode from insn failed");
        for (auto succLink : depNode->GetSuccs()) {
            DepNode &succNode = succLink->GetTo();
            LogInfo::MapleLogger() << succNode.GetInsn()->GetId() << "(" << succLink->GetLatency() << "), ";
        }
        LogInfo::MapleLogger() << std::resetiosflags(std::ios::right) << "\n";
    }
    LogInfo::MapleLogger() << "    ------------------------------------------------\n";
    LogInfo::MapleLogger() << "\n";
}
}  // namespace maplebe
