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

#include "local_schedule.h"
#include "data_dep_base.h"
#include "aarch64_data_dep_base.h"
#include "cg.h"
#include "optimize_common.h"

namespace maplebe {
void LocalSchedule::Run()
{
    FCDG *fcdg = cda.GetFCDG();
    CHECK_FATAL(fcdg != nullptr, "control dependence analysis failed");
    if (LOCAL_SCHEDULE_DUMP) {
        DotGenerator::GenerateDot("localsched", cgFunc, cgFunc.GetMirModule(), true, cgFunc.GetName());
    }
    InitInsnIdAndLocInsn();
    for (auto region : fcdg->GetAllRegions()) {
        if (region == nullptr || !CheckCondition(*region)) {
            continue;
        }
        DoLocalScheduleForRegion(*region);
    }
}

bool LocalSchedule::CheckCondition(CDGRegion &region) const
{
    CHECK_FATAL(region.GetRegionNodeSize() == 1 && region.GetRegionRoot() != nullptr,
                "invalid region in local scheduling");
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
    if (insnSum > kMaxInsnNum) {
        return false;
    }
    return true;
}

void LocalSchedule::DoLocalScheduleForRegion(CDGRegion &region)
{
    CDGNode *cdgNode = region.GetRegionRoot();
    BB *bb = cdgNode->GetBB();
    DEBUG_ASSERT(bb != nullptr, "get bb from cdgNode failed");
    if (bb->IsAtomicBuiltInBB()) {
        return;
    }
    intraDDA.Run(region);
    if (LOCAL_SCHEDULE_DUMP) {
        intraDDA.GenerateDataDepGraphDotOfRegion(region);
    }
    InitInRegion(region);
    if (LOCAL_SCHEDULE_DUMP || isUnitTest) {
        DumpRegionInfoBeforeSchedule(region);
    }
    DoLocalSchedule(*cdgNode);
}

void LocalSchedule::DoLocalSchedule(CDGNode &cdgNode)
{
    listScheduler = schedMP.New<ListScheduler>(schedMP, cgFunc, true, "localschedule");
    InitInCDGNode(cdgNode);
    listScheduler->SetCDGRegion(*cdgNode.GetRegion());
    listScheduler->SetCDGNode(cdgNode);
    listScheduler->SetUnitTest(isUnitTest);
    listScheduler->DoListScheduling();
    FinishScheduling(cdgNode);
    if (LOCAL_SCHEDULE_DUMP || isUnitTest) {
        DumpCDGNodeInfoAfterSchedule(cdgNode);
    }
}

void LocalSchedule::InitInCDGNode(CDGNode &cdgNode)
{
    commonSchedInfo = schedMP.New<CommonScheduleInfo>(schedMP);
    for (auto *depNode : cdgNode.GetAllDataNodes()) {
        commonSchedInfo->AddCandidates(depNode);
        depNode->SetState(kCandidate);
    }
    listScheduler->SetCommonSchedInfo(*commonSchedInfo);

    InitMachineInsnNum(cdgNode);

    if (LOCAL_SCHEDULE_DUMP || isUnitTest) {
        DumpCDGNodeInfoBeforeSchedule(cdgNode);
    }
}

bool CgLocalSchedule::PhaseRun(maplebe::CGFunc &f)
{
    MemPool *memPool = GetPhaseMemPool();
    auto *cda = memPool->New<ControlDepAnalysis>(f, *memPool, "localschedule", true);
    cda->Run();
    MAD *mad = Globals::GetInstance()->GetMAD();
    auto *ddb = memPool->New<AArch64DataDepBase>(*memPool, f, *mad, true);
    auto *dda = memPool->New<DataDepAnalysis>(f, *memPool, *ddb);
    auto *localScheduler = f.GetCG()->CreateLocalSchedule(*memPool, f, *cda, *dda);
    localScheduler->Run();
    return true;
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgLocalSchedule, localschedule)
}  // namespace maplebe
