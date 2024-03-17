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

#include "yieldpoint.h"
#include "loop.h"
#if TARGAARCH64
#include "aarch64_yieldpoint.h"
#elif TARGRISCV64
#include "riscv64_yieldpoint.h"
#endif
#if TARGARM32
#include "arm32_yieldpoint.h"
#endif
#include "cgfunc.h"

namespace maplebe {
using namespace maple;

void CgYieldPointInsertion::GetAnalysisDependence(AnalysisDep &aDep) const
{
    aDep.AddRequired<CgLoopAnalysis>();
    aDep.SetPreservedAll();
}

bool CgYieldPointInsertion::PhaseRun(maplebe::CGFunc &f)
{
    YieldPointInsertion *yieldPoint = nullptr;
    auto *loopInfo = GET_ANALYSIS(CgLoopAnalysis, f);
    CHECK_FATAL(loopInfo != nullptr, "get result of LoopAnalysis failed");
#if TARGAARCH64 || TARGRISCV64
    yieldPoint = GetPhaseAllocator()->New<AArch64YieldPointInsertion>(f, *loopInfo);
#endif
#if TARGARM32
    yieldPoint = GetPhaseAllocator()->New<Arm32YieldPointInsertion>(f);
#endif
    yieldPoint->Run();
    return false;
}
} /* namespace maplebe */
