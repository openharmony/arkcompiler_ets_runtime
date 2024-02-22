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

#include "ra_opt.h"
#include "cgfunc.h"
#include "loop.h"
#include "cg.h"
#include "optimize_common.h"
#include "aarch64_ra_opt.h"

namespace maplebe {
using namespace maple;

bool CgRaOpt::PhaseRun(maplebe::CGFunc &f)
{
    MemPool *memPool = GetPhaseMemPool();
    RaOpt *raOpt = nullptr;
    auto *dom = GET_ANALYSIS(CgDomAnalysis, f);
    CHECK_FATAL(dom != nullptr, "null ptr check");
    auto *loop = GET_ANALYSIS(CgLoopAnalysis, f);
    CHECK_FATAL(loop != nullptr, "null ptr check");
    raOpt = memPool->New<AArch64RaOpt>(f, *memPool, *dom, *loop);
    if (raOpt) {
        raOpt->Run();
    }
    return false;
}

void CgRaOpt::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgDomAnalysis>();
    aDep.AddRequired<CgPostDomAnalysis>();
    aDep.AddRequired<CgLoopAnalysis>();
    aDep.SetPreservedAll();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgRaOpt, raopt)
} /* namespace maplebe */
