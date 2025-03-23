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

#include "maple_phase_manager.h"
#include "cgfunc.h"

namespace maple {
using cgFuncOptTy = MapleFunctionPhase<maplebe::CGFunc>;
MemPool *AnalysisDataManager::ApplyMemPoolForAnalysisPhase(uint32 phaseKey, const MaplePhaseInfo &pi)
{
    std::string mempoolName = pi.PhaseName() + " memPool";
    MemPool *phaseMempool = nullptr;
    if (UseGlobalMpCtrler()) {
        phaseMempool = memPoolCtrler.NewMemPool(mempoolName, true);
    } else {
        phaseMempool = innerCtrler->NewMemPool(mempoolName, true);
    }
    (void)analysisPhaseMemPool.emplace(
        std::pair<AnalysisMemKey, MemPool *>(AnalysisMemKey(phaseKey, pi.GetPhaseID()), phaseMempool));
    return phaseMempool;
}

void AnalysisDataManager::AddAnalysisPhase(uint32 phaseKey, MaplePhase *p)
{
    CHECK_FATAL(p != nullptr, "invalid phase when AddAnalysisPhase");  // change to assert after testing
    (void)availableAnalysisPhases.emplace(
        std::pair<AnalysisMemKey, MaplePhase *>(AnalysisMemKey(phaseKey, p->GetPhaseID()), p));
}

// Erase phase at O2
// This is for the actully phase
void AnalysisDataManager::EraseAnalysisPhase(uint32 phaseKey, MaplePhaseID pid)
{
    auto it = analysisPhaseMemPool.find(AnalysisMemKey(phaseKey, pid));
    auto itanother = availableAnalysisPhases.find(AnalysisMemKey(phaseKey, pid));
    if (it != analysisPhaseMemPool.end() && itanother != availableAnalysisPhases.end()) {
        auto resultanother = availableAnalysisPhases.erase(AnalysisMemKey(phaseKey, pid));
        CHECK_FATAL(resultanother, "Release Failed");
        delete it->second;
        it->second = nullptr;
        auto result = analysisPhaseMemPool.erase(AnalysisMemKey(phaseKey, pid));  // erase to release mempool ?
        CHECK_FATAL(result, "Release Failed");
    }
}

// Erase all safely
void AnalysisDataManager::EraseAllAnalysisPhase()
{
#ifdef ARK_LITECG_DEBUG
    for (auto it = availableAnalysisPhases.begin(); it != availableAnalysisPhases.end();) {
        EraseAnalysisPhase(it);
    }
#endif
}

// Erase safely
void AnalysisDataManager::EraseAnalysisPhase(MapleMap<AnalysisMemKey, MaplePhase *>::iterator &anaPhaseMapIt)
{
    auto it = analysisPhaseMemPool.find(anaPhaseMapIt->first);
    if (it != analysisPhaseMemPool.end()) {
        anaPhaseMapIt = availableAnalysisPhases.erase(anaPhaseMapIt);
        delete it->second;
        it->second = nullptr;
#ifdef DEBUG
        bool result = analysisPhaseMemPool.erase(it->first);  // erase to release mempool
#else
        (void)analysisPhaseMemPool.erase(it->first);
#endif
        DEBUG_ASSERT(result, "Release Failed");
#ifdef DEBUG
    } else {
        DEBUG_ASSERT(false, "cannot delete phase which is not exist  &&  mempool is not create TOO");
#endif
    }
}

void AnalysisDataManager::ClearInVaildAnalysisPhase(uint32 phaseKey, AnalysisDep &ADep)
{
    if (!ADep.GetPreservedAll()) {
        // delete phases which are not preserved
        if (ADep.GetPreservedPhase().empty()) {
            for (auto it = availableAnalysisPhases.begin(); it != availableAnalysisPhases.end();) {
                if (it->first.first == phaseKey) {
                    EraseAnalysisPhase(it);
                } else {
                    ++it;
                }
            }
        }
        for (auto it = availableAnalysisPhases.begin(); it != availableAnalysisPhases.end();) {
            if (!ADep.FindIsPreserved((it->first).second) && it->first.first == phaseKey) {
                EraseAnalysisPhase(it);
            } else {
                ++it;
            }
        }
    } else {
        if (!ADep.GetPreservedExceptPhase().empty()) {
            for (auto exceptPhaseID : ADep.GetPreservedExceptPhase()) {
                auto it = availableAnalysisPhases.find(AnalysisMemKey(phaseKey, exceptPhaseID));
                if (it != availableAnalysisPhases.end()) {
                    EraseAnalysisPhase(it);
                }
            }
        }
    }
}

MaplePhase *AnalysisDataManager::GetVaildAnalysisPhase(uint32 phaseKey, MaplePhaseID pid)
{
    auto it = availableAnalysisPhases.find(AnalysisMemKey(phaseKey, pid));
    if (it == availableAnalysisPhases.end()) {
        LogInfo::MapleLogger() << "Required "
                               << MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(pid)->PhaseName()
                               << " running before \n";
        CHECK_FATAL(false, "find analysis phase failed");
        return nullptr;
    } else {
        return it->second;
    }
}

bool AnalysisDataManager::IsAnalysisPhaseAvailable(uint32 phaseKey, MaplePhaseID pid)
{
    auto it = availableAnalysisPhases.find(AnalysisMemKey(phaseKey, pid));
    return it != availableAnalysisPhases.end();
}

void AnalysisDataManager::Dump()
{
#ifdef ARK_LITECG_DEBUG
    LogInfo::MapleLogger() << "availableAnalysisPhases: \n";
    for (auto &it : availableAnalysisPhases) {
        LogInfo::MapleLogger()
            << "<" << it.first.first << ", "
            << MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(it.first.second)->PhaseName()
            << "> : " << it.second << "\n";
    }
    LogInfo::MapleLogger() << "analysisPhaseMemPool: \n";
    for (auto &it : analysisPhaseMemPool) {
        LogInfo::MapleLogger()
            << "<" << it.first.first << ", "
            << MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(it.first.second)->PhaseName()
            << "> : " << it.second << "\n";
    }
#endif
}

void MaplePhaseManager::AddPhase(const std::string &phaseName, bool condition)
{
    if (!condition) {
        return;
    }
    bool found = false;
    for (auto &it : MaplePhaseRegister::GetMaplePhaseRegister()->GetAllPassInfo()) {
        if (it.second->PhaseName() == phaseName) {
            phasesSequence.emplace_back(it.first);
            found = true;
            break;
        }
    }
    if (!found) {
        CHECK_FATAL(false, "%s not found !", phaseName.c_str());
    }
}

void MaplePhaseManager::DumpPhaseTime()
{
#ifdef ARK_LITECG_DEBUG
    if (phaseTh != nullptr) {
        phaseTh->DumpPhasesTime();
    }
#endif
}

void MaplePhaseManager::SolveSkipFrom(const std::string &phaseName, size_t &i)
{
    const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
    if (!skipFromFlag && curPhase->PhaseName() == phaseName) {
        CHECK_FATAL(curPhase->CanSkip(), "%s cannot be skipped!", phaseName.c_str());
        skipFromFlag = true;
    }
    if (skipFromFlag) {
        while (curPhase->CanSkip() && (++i != phasesSequence.size())) {
            curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
            CHECK_FATAL(curPhase != nullptr, "null ptr check ");
        }
    }
    skipFromFlag = false;
}

void MaplePhaseManager::SolveSkipAfter(const std::string &phaseName, size_t &i)
{
    const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
    if (!skipAfterFlag && curPhase->PhaseName() == phaseName) {
        skipAfterFlag = true;
    }
    if (skipAfterFlag) {
        while (++i != phasesSequence.size()) {
            curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(phasesSequence[i]);
            CHECK_FATAL(curPhase != nullptr, "null ptr check ");
            if (!curPhase->CanSkip()) {
                break;
            }
        }
        --i; /* restore iterator */
    }
    skipAfterFlag = false;
}

AnalysisDep *MaplePhaseManager::FindAnalysisDep(const MaplePhase *phase)
{
    AnalysisDep *anDependence = nullptr;
    auto anDepIt = analysisDepMap.find(phase->GetPhaseID());
    if (anDepIt != analysisDepMap.end()) {
        anDependence = anDepIt->second;
    } else {
        anDependence = allocator.New<AnalysisDep>(*GetManagerMemPool());
        phase->AnalysisDepInit(*anDependence);
        (void)analysisDepMap.emplace(std::pair<MaplePhaseID, AnalysisDep *>(phase->GetPhaseID(), anDependence));
    }
    return anDependence;
}

std::unique_ptr<ThreadLocalMemPool> MaplePhaseManager::AllocateMemPoolInPhaseManager(const std::string &mempoolName)
{
    if (!UseGlobalMpCtrler()) {
        LogInfo::MapleLogger() << " Inner Ctrler has not been supported yet \n";
    }
    return std::make_unique<ThreadLocalMemPool>(memPoolCtrler, mempoolName);
}

template <typename phaseT, typename IRTemplate>
void MaplePhaseManager::RunDependentPhase(const MaplePhase &phase, AnalysisDataManager &adm, IRTemplate &irUnit,
                                          int lev)
{
    AnalysisDep *anaDependence = FindAnalysisDep(&phase);
    for (auto requiredPhase : anaDependence->GetRequiredPhase()) {
        const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(requiredPhase);
        if (adm.IsAnalysisPhaseAvailable(irUnit.GetUniqueID(), curPhase->GetPhaseID())) {
            continue;
        }
        LogDependence(curPhase, lev);
        if (curPhase->IsAnalysis()) {
            (void)RunAnalysisPhase<phaseT, IRTemplate>(*curPhase, adm, irUnit, lev);
        } else {
            (void)RunTransformPhase<phaseT, IRTemplate>(*curPhase, adm, irUnit, lev);
        }
    }
}

/* live range of a phase should be short than mempool */
template <typename phaseT, typename IRTemplate>
bool MaplePhaseManager::RunTransformPhase(const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, IRTemplate &irUnit,
                                          int lev)
{
    bool result = false;
    auto transformPhaseMempool = AllocateMemPoolInPhaseManager(phaseInfo.PhaseName() + "'s mempool");
    auto *phase = static_cast<phaseT *>(phaseInfo.GetConstructor()(transformPhaseMempool.get()));
    phase->SetAnalysisInfoHook(transformPhaseMempool->New<AnalysisInfoHook>(*(transformPhaseMempool.get()), adm, this));
    RunDependentPhase<phaseT, IRTemplate>(*phase, adm, irUnit, lev + 1);
    if (phaseTh != nullptr) {
#ifdef ARK_LITECG_DEBUG
        phaseTh->RunBeforePhase(phaseInfo);
        result = phase->PhaseRun(irUnit);
        phaseTh->RunAfterPhase(phaseInfo);
#endif
    } else {
        result = phase->PhaseRun(irUnit);
    }
    phase->ClearTempMemPool();
    adm.ClearInVaildAnalysisPhase(irUnit.GetUniqueID(), *FindAnalysisDep(phase));
    return result;
}

template <typename phaseT, typename IRTemplate>
bool MaplePhaseManager::RunAnalysisPhase(const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, IRTemplate &irUnit,
                                         int lev)
{
    bool result = false;
    phaseT *phase = nullptr;
    if (adm.IsAnalysisPhaseAvailable(irUnit.GetUniqueID(), phaseInfo.GetPhaseID())) {
        return result;
    }
    MemPool *anasPhaseMempool = adm.ApplyMemPoolForAnalysisPhase(irUnit.GetUniqueID(), phaseInfo);
    phase = static_cast<phaseT *>(phaseInfo.GetConstructor()(anasPhaseMempool));
    // change analysis info hook mempool from ADM Allocator to phase allocator?
    phase->SetAnalysisInfoHook(anasPhaseMempool->New<AnalysisInfoHook>(*anasPhaseMempool, adm, this));
    RunDependentPhase<phaseT, IRTemplate>(*phase, adm, irUnit, lev + 1);
    if (phaseTh != nullptr) {
#ifdef ARK_LITECG_DEBUG
        phaseTh->RunBeforePhase(phaseInfo);
        result = phase->PhaseRun(irUnit);
        phaseTh->RunAfterPhase(phaseInfo);
#endif
    } else {
        result = phase->PhaseRun(irUnit);
    }
    phase->ClearTempMemPool();
    adm.AddAnalysisPhase(irUnit.GetUniqueID(), phase);
    return result;
}

// declaration for functionPhase (template only)
template bool MaplePhaseManager::RunTransformPhase<MapleModulePhase, MIRModule>(const MaplePhaseInfo &phaseInfo,
                                                                                AnalysisDataManager &adm,
                                                                                MIRModule &irUnit, int lev);
template bool MaplePhaseManager::RunAnalysisPhase<MapleModulePhase, MIRModule>(const MaplePhaseInfo &phaseInfo,
                                                                               AnalysisDataManager &adm,
                                                                               MIRModule &irUnit, int lev);
template bool MaplePhaseManager::RunTransformPhase<cgFuncOptTy, maplebe::CGFunc>(const MaplePhaseInfo &phaseInfo,
                                                                                 AnalysisDataManager &adm,
                                                                                 maplebe::CGFunc &irUnit, int lev);
template bool MaplePhaseManager::RunAnalysisPhase<cgFuncOptTy, maplebe::CGFunc>(const MaplePhaseInfo &phaseInfo,
                                                                                AnalysisDataManager &adm,
                                                                                maplebe::CGFunc &irUnit, int lev);
}  // namespace maple
