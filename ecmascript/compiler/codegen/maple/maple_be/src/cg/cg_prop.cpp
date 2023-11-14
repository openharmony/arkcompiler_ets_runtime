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

#include "loop.h"
#include "cg_prop.h"

namespace maplebe {
void CGProp::DoCopyProp()
{
    CopyProp();
    cgDce->DoDce();
}

void CGProp::DoTargetProp()
{
    DoCopyProp();
    /* instruction level opt */
    FOR_ALL_BB(bb, cgFunc) {
        FOR_BB_INSNS(insn, bb) {
            if (!insn->IsMachineInstruction()) {
                continue;
            }
            TargetProp(*insn);
        }
    }
    /* pattern  level opt */
    if (CGOptions::GetInstance().GetOptimizeLevel() == CGOptions::kLevel2) {
        PropPatternOpt();
    }
}

Insn *PropOptimizePattern::FindDefInsn(const VRegVersion *useVersion)
{
    if (!useVersion) {
        return nullptr;
    }
    DUInsnInfo *defInfo = useVersion->GetDefInsnInfo();
    if (!defInfo) {
        return nullptr;
    }
    return defInfo->GetInsn();
}

bool CgCopyProp::PhaseRun(maplebe::CGFunc &f)
{
    CGSSAInfo *ssaInfo = GET_ANALYSIS(CgSSAConstruct, f);
    LiveIntervalAnalysis *ll = GET_ANALYSIS(CGliveIntervalAnalysis, f);
    CGProp *cgProp = f.GetCG()->CreateCGProp(*GetPhaseMemPool(), f, *ssaInfo, *ll);
    cgProp->DoCopyProp();
    ll->ClearBFS();
    return false;
}
void CgCopyProp::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgSSAConstruct>();
    aDep.AddRequired<CGliveIntervalAnalysis>();
    aDep.AddPreserved<CgSSAConstruct>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgCopyProp, cgcopyprop)

bool CgTargetProp::PhaseRun(maplebe::CGFunc &f)
{
    CGSSAInfo *ssaInfo = GET_ANALYSIS(CgSSAConstruct, f);
    LiveIntervalAnalysis *ll = GET_ANALYSIS(CGliveIntervalAnalysis, f);
    CGProp *cgProp = f.GetCG()->CreateCGProp(*GetPhaseMemPool(), f, *ssaInfo, *ll);
    cgProp->DoTargetProp();
    ll->ClearBFS();
    return false;
}
void CgTargetProp::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<CgSSAConstruct>();
    aDep.AddRequired<CGliveIntervalAnalysis>();
    aDep.AddPreserved<CgSSAConstruct>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgTargetProp, cgtargetprop)
}  // namespace maplebe
