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
#include "live.h"
#include "loop.h"
#include "cg_dominance.h"
#include "mir_lower.h"
#include "securec.h"
#include "reg_alloc_basic.h"
#include "reg_alloc_lsra.h"
#include "cg.h"
#if TARGAARCH64
#include "aarch64_color_ra.h"
#endif

namespace maplebe {
#define RA_DUMP (CG_DEBUG_FUNC(f))

void CgRegAlloc::GetAnalysisDependence(AnalysisDep &aDep) const
{
    if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
        return;
    }

    if (CGOptions::GetInstance().DoLinearScanRegisterAllocation()) {
        aDep.AddRequired<CgBBSort>();
    } else if (CGOptions::GetInstance().DoColoringBasedRegisterAllocation()) {
        aDep.AddRequired<CgDomAnalysis>();
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
        if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
            regAllocator = phaseMp->New<DefaultO0RegAllocator>(f, *phaseMp);
        } else if (f.GetCG()->GetCGOptions().DoLinearScanRegisterAllocation()) {
            Bfs *bfs = GET_ANALYSIS(CgBBSort, f);
            CHECK_FATAL(bfs != nullptr, "null ptr check");
            live = GET_ANALYSIS(CgLiveAnalysis, f);
            CHECK_FATAL(live != nullptr, "null ptr check");
            // revert liveanalysis result container.
            live->ResetLiveSet();
            regAllocator = phaseMp->New<LSRALinearScanRegAllocator>(f, *phaseMp, bfs);
#if TARGAARCH64
        } else if (f.GetCG()->GetCGOptions().DoColoringBasedRegisterAllocation()) {
            MaplePhase *it = GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(
                &CgLiveAnalysis::id, f);
            live = static_cast<CgLiveAnalysis*>(it)->GetResult();
            CHECK_FATAL(live != nullptr, "null ptr check");
            /* revert liveanalysis result container. */
            live->ResetLiveSet();
            DomAnalysis *dom = GET_ANALYSIS(CgDomAnalysis, f);
            CHECK_FATAL(dom != nullptr, "null ptr check");
            regAllocator = phaseMp->New<GraphColorRegAllocator>(f, *tempMP, *dom);
#endif
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
