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

#include "cgfunc.h"
#if TARGAARCH64
#include "aarch64_regsaves.h"
#elif TARGRISCV64
#include "riscv64_regsaves.h"
#endif

namespace maplebe {
using namespace maple;

bool CgRegSavesOpt::PhaseRun(maplebe::CGFunc &f)
{
    if (Globals::GetInstance()->GetOptimLevel() <= CGOptions::kLevel1) {
        return false;
    }

    /* Perform loop analysis, result to be obtained in CGFunc */
    (void)GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgLoopAnalysis::id, f);

    /* Perform live analysis, result to be obtained in CGFunc */
    LiveAnalysis *live = nullptr;
    MaplePhase *it =
        GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgLiveAnalysis::id, f);
    live = static_cast<CgLiveAnalysis *>(it)->GetResult();
    CHECK_FATAL(live != nullptr, "null ptr check");
    /* revert liveanalysis result container. */
    live->ResetLiveSet();

    /* Perform dom analysis, result to be inserted into AArch64RegSavesOpt object */
    DomAnalysis *dom = nullptr;
    PostDomAnalysis *pdom = nullptr;
    LoopAnalysis *loop = nullptr;
    if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel1 &&
        f.GetCG()->GetCGOptions().DoColoringBasedRegisterAllocation()) {
        MaplePhase *phase =
            GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgDomAnalysis::id, f);
        dom = static_cast<CgDomAnalysis *>(phase)->GetResult();
        CHECK_FATAL(dom != nullptr, "null ptr check");
        phase =
            GetAnalysisInfoHook()->ForceRunAnalysisPhase<MapleFunctionPhase<CGFunc>, CGFunc>(&CgPostDomAnalysis::id, f);
        pdom = static_cast<CgPostDomAnalysis *>(phase)->GetResult();
        CHECK_FATAL(pdom != nullptr, "null ptr check");
        loop = static_cast<CgLoopAnalysis*>(phase)->GetResult();
        CHECK_FATAL(loop != nullptr, "null ptr check");
    }

    MemPool *memPool = GetPhaseMemPool();
    RegSavesOpt *regSavesOpt = nullptr;
#if TARGAARCH64
    regSavesOpt = memPool->New<AArch64RegSavesOpt>(f, *memPool, *dom, *pdom, *loop);
#elif || TARGRISCV64
    regSavesOpt = memPool->New<Riscv64RegSavesOpt>(f, *memPool);
#endif

    if (regSavesOpt) {
        regSavesOpt->SetEnabledDebug(false); /* To turn on debug trace */
        if (regSavesOpt->GetEnabledDebug()) {
            dom->Dump();
        }
        regSavesOpt->Run();
    }
    return true;
}
MAPLE_TRANSFORM_PHASE_REGISTER(CgRegSavesOpt, regsaves)
} /* namespace maplebe */
