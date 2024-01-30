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

#include "global_schedule.h"
#include "optimize_common.h"
#include "aarch64_data_dep_base.h"
#include "cg.h"

namespace maplebe {
void GlobalSchedule::Run()
{
    FCDG *fcdg = cda.GetFCDG();
    CHECK_FATAL(fcdg != nullptr, "control dependence analysis failed");
    if (GLOBAL_SCHEDULE_DUMP) {
        cda.GenerateSimplifiedCFGDot();
        cda.GenerateCFGDot();
        cda.GenerateFCDGDot();
        DotGenerator::GenerateDot("globalsched", cgFunc, cgFunc.GetMirModule(), true, cgFunc.GetName());
    }
    InitInsnIdAndLocInsn();

    uint32 times = 0;
    for (auto region : fcdg->GetAllRegions()) {
        if (times++ >= CGOptions::GetGlobalScheduleLimit()) {
            break;
        }
        if (region == nullptr || !CheckCondition(*region)) {
            continue;
        }
        interDDA.Run(*region);
        if (GLOBAL_SCHEDULE_DUMP) {
            cda.GenerateCFGInRegionDot(*region);
            interDDA.GenerateDataDepGraphDotOfRegion(*region);
        }
        InitInRegion(*region);
        if (CGOptions::DoVerifySchedule()) {
            VerifyingSchedule(*region);
            continue;
        }
        DoGlobalSchedule(*region);
    }
}

bool GlobalSchedule::CheckCondition(CDGRegion &region)
{
    uint32 insnSum = 0;
    for (auto cdgNode : region.GetRegionNodes()) {
        BB *bb = cdgNode->GetBB();
        CHECK_FATAL(bb != nullptr, "get bb from cdgNode failed");
        FOR_BB_INSNS_CONST(insn, bb)
        {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            insnSum++;
        }
    }
    return insnSum <= kMaxInsnNum;
}

// The entry of global scheduling
void GlobalSchedule::DoGlobalSchedule(CDGRegion &region)
{
    if (GLOBAL_SCHEDULE_DUMP) {
        DumpRegionInfoBeforeSchedule(region);
    }
    listScheduler = schedMP.New<ListScheduler>(schedMP, cgFunc, true, "globalschedule");
    // Process nodes in a region by the topology sequence
    for (auto cdgNode : region.GetRegionNodes()) {
        BB *bb = cdgNode->GetBB();
        ASSERT(bb != nullptr, "get bb from cdgNode failed");
        if (bb->IsAtomicBuiltInBB()) {
            for (auto depNode : cdgNode->GetAllDataNodes()) {
                for (auto succLink : depNode->GetSuccs()) {
                    DepNode &succNode = succLink->GetTo();
                    succNode.DecreaseValidPredsSize();
                }
            }
            continue;
        }

        MemPool *cdgNodeMp = memPoolCtrler.NewMemPool("global-scheduler cdgNode memPool", true);
        // Collect candidate instructions of current cdgNode
        InitInCDGNode(region, *cdgNode, *cdgNodeMp);

        // Execute list scheduling
        listScheduler->SetCDGRegion(region);
        listScheduler->SetCDGNode(*cdgNode);
        listScheduler->DoListScheduling();

        // Reorder instructions in the current BB based on the scheduling result
        FinishScheduling(*cdgNode);

        if (GLOBAL_SCHEDULE_DUMP) {
            DumpCDGNodeInfoAfterSchedule(*cdgNode);
        }

        ClearCDGNodeInfo(region, *cdgNode, cdgNodeMp);
    }
}

void GlobalSchedule::InitInCDGNode(CDGRegion &region, CDGNode &cdgNode, MemPool &cdgNodeMp)
{
    PrepareCommonSchedInfo(region, cdgNode, cdgNodeMp);

    // Init insnNum of curCDGNode
    InitMachineInsnNum(cdgNode);

    if (GLOBAL_SCHEDULE_DUMP) {
        DumpCDGNodeInfoBeforeSchedule(cdgNode);
    }
}

void GlobalSchedule::PrepareCommonSchedInfo(CDGRegion &region, CDGNode &cdgNode, MemPool &cdgNodeMp)
{
    commonSchedInfo = cdgNodeMp.New<CommonScheduleInfo>(cdgNodeMp);
    // 1. The instructions of the current node
    MapleVector<DepNode *> &curDataNodes = cdgNode.GetAllDataNodes();
    // For verify, the node is stored in reverse order and for global, the node is stored in sequence
    for (auto *depNode : curDataNodes) {
        commonSchedInfo->AddCandidates(depNode);
        depNode->SetState(kCandidate);
    }
    // 2. The instructions of the equivalent candidate nodes of the current node
    std::vector<CDGNode *> equivalentNodes;
    cda.GetEquivalentNodesInRegion(region, cdgNode, equivalentNodes);
    if (GLOBAL_SCHEDULE_DUMP && !equivalentNodes.empty()) {
        LogInfo::MapleLogger() << "BB_" << cdgNode.GetBB()->GetId() << "'s "
                               << "Equivalent BBs: {";
    }
    for (auto *equivNode : equivalentNodes) {
        BB *equivBB = equivNode->GetBB();
        ASSERT(equivBB != nullptr, "get bb from cdgNode failed");
        if (GLOBAL_SCHEDULE_DUMP) {
            LogInfo::MapleLogger() << "BB_" << equivBB->GetId() << ", ";
        }
        if (equivBB->IsAtomicBuiltInBB()) {
            continue;
        }
        for (auto *depNode : equivNode->GetAllDataNodes()) {
            Insn *insn = depNode->GetInsn();
            CHECK_FATAL(insn != nullptr, "get insn from depNode failed");
            // call & branch insns cannot be moved across BB
            if (insn->IsBranch() || insn->IsCall()) {
                continue;
            }
            commonSchedInfo->AddCandidates(depNode);
            depNode->SetState(kCandidate);
        }
    }
    if (GLOBAL_SCHEDULE_DUMP && !equivalentNodes.empty()) {
        LogInfo::MapleLogger() << "}\n";
    }
    listScheduler->SetCommonSchedInfo(*commonSchedInfo);
}

void GlobalSchedule::ClearCDGNodeInfo(CDGRegion &region, CDGNode &cdgNode, MemPool *cdgNodeMp)
{
    std::vector<CDGNode *> equivalentNodes;
    cda.GetEquivalentNodesInRegion(region, cdgNode, equivalentNodes);
    for (auto *equivNode : equivalentNodes) {
        for (auto *depNode : equivNode->GetAllDataNodes()) {
            ASSERT(depNode->GetState() != kScheduled, "update state of depNode failed in finishScheduling");
            depNode->SetState(kNormal);
        }
    }

    memPoolCtrler.DeleteMemPool(cdgNodeMp);
    commonSchedInfo = nullptr;
}

void CgGlobalSchedule::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgControlDepAnalysis>();
}

bool CgGlobalSchedule::PhaseRun(maplebe::CGFunc &f)
{
    MemPool *memPool = GetPhaseMemPool();
    ControlDepAnalysis *cda = GET_ANALYSIS(CgControlDepAnalysis, f);
    MAD *mad = Globals::GetInstance()->GetMAD();
    auto *ddb = memPool->New<AArch64DataDepBase>(*memPool, f, *mad, false);
    auto *dda = memPool->New<DataDepAnalysis>(f, *memPool, *ddb);
    auto *globalScheduler = f.GetCG()->CreateGlobalSchedule(*memPool, f, *cda, *dda);
    globalScheduler->Run();
    return true;
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgGlobalSchedule, globalschedule)
}  // namespace maplebe
