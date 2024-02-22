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
#include "me_dominance.h"
#include <iostream>
#include "me_option.h"
#include "me_cfg.h"
#include "base_graph_node.h"
// This phase analyses the CFG of the given MeFunction, generates the dominator tree,
// and the dominance frontiers of each basic block using Keith Cooper's algorithm.
// For some backward data-flow problems, such as LiveOut,
// the reverse CFG(The CFG with its edges reversed) is always useful,
// so we also generates the above two structures on the reverse CFG.
namespace maple {
void MEDominance::GetAnalysisDependence(maple::AnalysisDep &aDep) const
{
    aDep.AddRequired<MEMeCfg>();
    aDep.SetPreservedAll();
}

bool MEDominance::PhaseRun(maple::MeFunction &f)
{
    MeCFG *cfg = f.GetCfg();
    ASSERT_NOT_NULL(cfg);
    MemPool *memPool = GetPhaseMemPool();
    auto alloc = MapleAllocator(memPool);
    auto nodeVec = memPool->New<MapleVector<BaseGraphNode *>>(alloc.Adapter());
    CHECK_NULL_FATAL(nodeVec);
    ConvertToVectorOfBasePtr<BB>(cfg->GetAllBBs(), *nodeVec);
    dom = memPool->New<Dominance>(*memPool, *nodeVec, *cfg->GetCommonEntryBB(), *cfg->GetCommonExitBB(), false);
    CHECK_NULL_FATAL(dom);
    pdom = memPool->New<Dominance>(*memPool, *nodeVec, *cfg->GetCommonExitBB(), *cfg->GetCommonEntryBB(), true);
    CHECK_NULL_FATAL(pdom);
    dom->Init();
    pdom->Init();
    if (DEBUGFUNC_NEWPM(f)) {
        LogInfo::MapleLogger() << "-----------------Dump dominance info and postdominance info---------\n";
        dom->DumpDoms();
        pdom->DumpDoms();
    }
    return false;
}
}  // namespace maple
