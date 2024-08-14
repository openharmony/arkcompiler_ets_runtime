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

#include "maple_phase.h"
namespace maple {
MaplePhaseRegister *MaplePhaseRegister::GetMaplePhaseRegister()
{
    static MaplePhaseRegister *globalRegister = new MaplePhaseRegister();
    return globalRegister;
}

void MaplePhaseRegister::RegisterPhase(const MaplePhaseInfo &PI)
{
    bool checkSucc = passInfoMap.emplace(std::pair<MaplePhaseID, const MaplePhaseInfo *>(PI.GetPhaseID(), &PI)).second;
    CHECK_FATAL(checkSucc, "Register Phase failed");
}

const MaplePhaseInfo *MaplePhaseRegister::GetPhaseByID(MaplePhaseID id)
{
    if (passInfoMap.count(id)) {
        return passInfoMap.find(id)->second;
    } else {
        CHECK_FATAL(false, "This phase has not been registered");
        return passInfoMap.end()->second;
    }
}

MemPool *MaplePhase::ApplyTempMemPool()
{
    MemPool *res = memPoolCtrler.NewMemPool("temp Mempool", true);
    tempMemPools.emplace_back(res);
    return res;
}

void MaplePhase::ClearTempMemPool()
{
#ifdef DEBUG
    unsigned int maxMemPoolNum = 5;
    DEBUG_ASSERT(tempMemPools.size() <= maxMemPoolNum, " maple phase uses too many temp mempool");
#endif
    if (!tempMemPools.empty()) {
        for (auto mpIt : tempMemPools) {
            delete mpIt;
            mpIt = nullptr;
        }
        tempMemPools.clear();
    }
}

/* default : do not require any phases */
void MaplePhase::AnalysisDepInit(AnalysisDep &aDep) const
{
    GetAnalysisDependence(aDep);
}

void MaplePhase::GetAnalysisDependence(AnalysisDep &aDep) const
{
    (void)aDep;
    // do nothing if derived class not defined analysis dependence
}

void MaplePhase::SetAnalysisInfoHook(AnalysisInfoHook *hook)
{
    analysisInfo = hook;
}

AnalysisInfoHook *MaplePhase::GetAnalysisInfoHook()
{
    return analysisInfo;
}
}  // namespace maple
