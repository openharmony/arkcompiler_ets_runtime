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

#include "cg.h"
#include "cg_critical_edge.h"
#include "cg_ssa.h"

namespace maplebe {
void CriticalEdge::SplitCriticalEdges()
{
    for (auto it = criticalEdges.begin(); it != criticalEdges.end(); ++it) {
        cgFunc->GetTheCFG()->BreakCriticalEdge(*((*it).first), *((*it).second));
    }
}

void CriticalEdge::CollectCriticalEdges()
{
    constexpr int multiPredsNum = 2;
    FOR_ALL_BB(bb, cgFunc) {
        const auto &preds = bb->GetPreds();
        if (preds.size() < multiPredsNum) {
            continue;
        }
        // current BB is a merge
        for (BB *pred : preds) {
            if (pred->GetKind() == BB::kBBGoto || pred->GetKind() == BB::kBBIgoto) {
                continue;
            }
            if (pred->GetSuccs().size() > 1) {
                // pred has more than one succ
                criticalEdges.push_back(std::make_pair(pred, bb));
            }
        }
    }
}

bool CgCriticalEdge::PhaseRun(maplebe::CGFunc &f)
{
    if (Globals::GetInstance()->GetOptimLevel() >= CGOptions::kLevel2 && f.NumBBs() < kBBLimit) {
        MemPool *memPool = GetPhaseMemPool();
        CriticalEdge *split = memPool->New<CriticalEdge>(f, *memPool);
        f.GetTheCFG()->InitInsnVisitor(f);
        split->CollectCriticalEdges();
        split->SplitCriticalEdges();
    }
    return false;
}

void CgCriticalEdge::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddPreserved<CgSSAConstruct>();
}
MAPLE_TRANSFORM_PHASE_REGISTER_CANSKIP(CgCriticalEdge, cgsplitcriticaledge)
} /* namespace maplebe */
