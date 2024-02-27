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
#include "aarch64_strldr.h"
#elif TARGRISCV64
#include "riscv64_strldr.h"
#endif
#if TARGARM32
#include "arm32_strldr.h"
#endif
#include "reaching.h"
#include "cg.h"
#include "optimize_common.h"

namespace maplebe {
using namespace maple;
#define SCHD_DUMP_NEWPM CG_DEBUG_FUNC(f)
bool CgStoreLoadOpt::PhaseRun(maplebe::CGFunc &f)
{
    if (SCHD_DUMP_NEWPM) {
        DotGenerator::GenerateDot("storeloadopt", f, f.GetMirModule());
    }
    ReachingDefinition *reachingDef = nullptr;
    if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel2) {
        reachingDef = GET_ANALYSIS(CgReachingDefinition, f);
    }
    if (reachingDef == nullptr || !f.GetRDStatus()) {
        GetAnalysisInfoHook()->ForceEraseAnalysisPhase(f.GetUniqueID(), &CgReachingDefinition::id);
        return false;
    }
    auto *loopInfo = GET_ANALYSIS(CgLoopAnalysis, f);
    StoreLoadOpt *storeLoadOpt = nullptr;
#if TARGAARCH64 || TARGRISCV64
    storeLoadOpt = GetPhaseMemPool()->New<AArch64StoreLoadOpt>(f, *GetPhaseMemPool(), *loopInfo);
#endif
#if TARGARM32
    storeLoadOpt = GetPhaseMemPool()->New<Arm32StoreLoadOpt>(f, *GetPhaseMemPool());
#endif
    storeLoadOpt->Run();
    return true;
}
void CgStoreLoadOpt::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgReachingDefinition>();
    aDep.AddRequired<CgLoopAnalysis>();
    aDep.SetPreservedAll();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgStoreLoadOpt, storeloadopt)
} /* namespace maplebe */
