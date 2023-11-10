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

#include "cg_dce.h"
#include "cg.h"
namespace maplebe {
void CGDce::DoDce()
{
    bool tryDceAgain = false;
    do {
        tryDceAgain = false;
        for (auto &ssaIt : GetSSAInfo()->GetAllSSAOperands()) {
            if (ssaIt.second != nullptr && !ssaIt.second->IsDeleted()) {
                if (RemoveUnuseDef(*ssaIt.second)) {
                    tryDceAgain = true;
                }
            }
        }
    } while (tryDceAgain);
}

bool CgDce::PhaseRun(maplebe::CGFunc &f)
{
    CGSSAInfo *ssaInfo = GET_ANALYSIS(CgSSAConstruct, f);
    CGDce *cgDce = f.GetCG()->CreateCGDce(*GetPhaseMemPool(), f, *ssaInfo);
    cgDce->DoDce();
    return false;
}

void CgDce::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgSSAConstruct>();
    aDep.AddPreserved<CgSSAConstruct>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgDce, cgdeadcodeelimination)
}  // namespace maplebe
