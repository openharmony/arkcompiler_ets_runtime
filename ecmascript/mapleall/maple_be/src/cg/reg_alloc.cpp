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
void CgRegAlloc::GetAnalysisDependence(AnalysisDep &aDep) const
{
    if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevelLiteCG ||
        CGOptions::GetInstance().DoLinearScanRegisterAllocation()) {
        aDep.AddRequired<CgBBSort>();
    }
    if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0) {
        aDep.AddRequired<CgLoopAnalysis>();
        aDep.AddRequired<CgLiveAnalysis>();
        aDep.PreservedAllExcept<CgLiveAnalysis>();
    }
#if TARGAARCH64
    if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0 &&
        CGOptions::GetInstance().DoColoringBasedRegisterAllocation()) {
        aDep.AddRequired<CgDomAnalysis>();
    }
#endif
}

bool CgRegAlloc::PhaseRun(maplebe::CGFunc &f)
{
    bool success = false;
    while (success == false) {
        MemPool *phaseMp = GetPhaseMemPool();
        /* create register allocator */
        RegAllocator *regAllocator = nullptr;
        if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevel0) {
            regAllocator = phaseMp->New<DefaultO0RegAllocator>(f, *phaseMp);
        } else if (Globals::GetInstance()->GetOptimLevel() == CGOptions::kLevelLiteCG) {
            Bfs *bfs = GET_ANALYSIS(CgBBSort, f);
            CHECK_FATAL(bfs != nullptr, "null ptr check");
            regAllocator = phaseMp->New<LSRALinearScanRegAllocator>(f, *phaseMp, bfs);
        } else {
#if TARGAARCH64
            if (f.GetCG()->GetCGOptions().DoLinearScanRegisterAllocation()) {
                Bfs *bfs = GET_ANALYSIS(CgBBSort, f);
                CHECK_FATAL(bfs != nullptr, "null ptr check");
                regAllocator = phaseMp->New<LSRALinearScanRegAllocator>(f, *phaseMp, bfs);
            } else if (f.GetCG()->GetCGOptions().DoColoringBasedRegisterAllocation()) {
                MaplePhase *it = GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(
                    &CgLiveAnalysis::id, f);
                LiveAnalysis *live = static_cast<CgLiveAnalysis *>(it)->GetResult();
                CHECK_FATAL(live != nullptr, "null ptr check");
                /* revert liveanalysis result container. */
                live->ResetLiveSet();
                DomAnalysis *dom = GET_ANALYSIS(CgDomAnalysis, f);
                CHECK_FATAL(dom != nullptr, "null ptr check");
                regAllocator = phaseMp->New<GraphColorRegAllocator>(f, *phaseMp, *dom);
            } else {
                maple::LogInfo::MapleLogger(kLlErr)
                    << "Warning: We only support Linear Scan and GraphColor register allocation\n";
            }
#elif TARGX86_64
            LogInfo::MapleLogger(kLlErr) << "Error: We only support -O0, and -LiteCG for x64.\n";
#endif
        }
        /* do register allocation */
        CHECK_FATAL(regAllocator != nullptr, "regAllocator is null in CgDoRegAlloc::Run");
        f.SetIsAfterRegAlloc();
        success = regAllocator->AllocateRegisters();
        if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0) {
            GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgLiveAnalysis::id);
        }
    }
    if (Globals::GetInstance()->GetOptimLevel() > CGOptions::kLevel0) {
        GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgLoopAnalysis::id);
    }
    return false;
}
} /* namespace maplebe */