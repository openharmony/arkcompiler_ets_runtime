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

#include "reg_alloc.h"
#include "reg_alloc_lsra.h"
#include "cg.h"

namespace maplebe {
#define RA_DUMP (CG_DEBUG_FUNC(f))

void CgRegAlloc::GetAnalysisDependence(AnalysisDep &aDep) const
{
    if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
        return;
    }

    if (CGOptions::GetInstance().DoLinearScanRegisterAllocation()) {
        aDep.AddRequired<CgBBSort>();
    }
    aDep.AddRequired<CgLoopAnalysis>();
    aDep.AddRequired<CgLiveAnalysis>();
    aDep.PreservedAllExcept<CgLiveAnalysis>();
}

bool CgRegAlloc::PhaseRun(maplebe::CGFunc &f)
{
    bool success = false;
    while (!success) {
        MemPool *phaseMp = GetPhaseMemPool();
        LiveAnalysis *live = nullptr;

        /* create register allocator */
        RegAllocator *regAllocator = nullptr;
        MemPool *tempMP = memPoolCtrler.NewMemPool("regalloc", true);
        if (f.GetCG()->GetCGOptions().DoLinearScanRegisterAllocation()) {
            Bfs *bfs = GET_ANALYSIS(CgBBSort, f);
            CHECK_FATAL(bfs != nullptr, "null ptr check");
            live = GET_ANALYSIS(CgLiveAnalysis, f);
            CHECK_FATAL(live != nullptr, "null ptr check");
            LoopAnalysis *loop = GET_ANALYSIS(CgLoopAnalysis, f);
            CHECK_FATAL(loop != nullptr, "null ptr check");
            regAllocator = phaseMp->New<LSRALinearScanRegAllocator>(f, *phaseMp, bfs, *loop);
        } else {
            maple::LogInfo::MapleLogger(kLlErr)
                << "Warning: We only support Linear Scan and GraphColor register allocation\n";
        }
        RA_TIMER_REGISTER(ra, "RA Time");
        /* do register allocation */
        CHECK_FATAL(regAllocator != nullptr, "regAllocator is null in CgDoRegAlloc::Run");
        regAllocator->SetNeedDump(RA_DUMP);
        f.SetIsAfterRegAlloc();
        success = regAllocator->AllocateRegisters();

        if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0) {
            GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgLiveAnalysis::id);
        }
        memPoolCtrler.DeleteMemPool(tempMP);
    }
    RA_TIMER_PRINT(f.GetName());
    return false;
}
} /* namespace maplebe */
