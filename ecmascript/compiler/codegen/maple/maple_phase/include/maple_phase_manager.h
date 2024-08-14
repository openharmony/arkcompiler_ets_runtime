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

#ifndef MAPLE_PHASE_INCLUDE_MAPLE_PHASE_MANAGER_H
#define MAPLE_PHASE_INCLUDE_MAPLE_PHASE_MANAGER_H
#include "maple_phase.h"
#include "me_option.h"

namespace maple {
class MaplePhase;
using AnalysisMemKey = std::pair<uint32, MaplePhaseID>;
class AnalysisDataManager {
public:
    explicit AnalysisDataManager(MemPool &mempool)
        : allocator(&mempool), analysisPhaseMemPool(allocator.Adapter()), availableAnalysisPhases(allocator.Adapter())
    {
        innerCtrler = &mempool.GetCtrler();
    }

    ~AnalysisDataManager() = default;

    bool UseGlobalMpCtrler() const
    {
        return useGlobalMpCtrler;
    }

    void CopyAnalysisResultFrom(const AnalysisDataManager &other)
    {
        analysisPhaseMemPool = other.analysisPhaseMemPool;
        availableAnalysisPhases = other.availableAnalysisPhases;
    }

    MemPool *ApplyMemPoolForAnalysisPhase(uint32 phaseKey, const MaplePhaseInfo &pi);
    void AddAnalysisPhase(uint32 phaseKey, MaplePhase *p);
    bool CheckAnalysisInfoEmpty() const
    {
        return analysisPhaseMemPool.empty() && availableAnalysisPhases.empty();
    }
    void EraseAnalysisPhase(uint32 phaseKey, MaplePhaseID pid);
    void EraseAllAnalysisPhase();
    void EraseAnalysisPhase(MapleMap<AnalysisMemKey, MaplePhase *>::iterator &anaPhaseMapIt);
    void ClearInVaildAnalysisPhase(uint32 phaseKey, AnalysisDep &ADep);  // do after transform phase;
    MaplePhase *GetVaildAnalysisPhase(uint32 phaseKey, MaplePhaseID pid);
    bool IsAnalysisPhaseAvailable(uint32 phaseKey, MaplePhaseID pid);
    void Dump();

private:
    MapleAllocator allocator;  // thread local
    MemPoolCtrler *innerCtrler = nullptr;
    MapleMap<AnalysisMemKey, MemPool *> analysisPhaseMemPool;
    MapleMap<AnalysisMemKey, MaplePhase *> availableAnalysisPhases;
    bool useGlobalMpCtrler = false;
};

/* top level manager [manages phase managers/ immutable pass(not implement yet)] */
class MaplePhaseManager {
public:
    explicit MaplePhaseManager(MemPool &memPool)
        : allocator(&memPool),
          phasesSequence(allocator.Adapter()),
          analysisDepMap(allocator.Adapter()),
          threadMemPools(allocator.Adapter()),
          analysisDataManagers(allocator.Adapter())
    {
    }

    virtual ~MaplePhaseManager()
    {
        phaseTh = nullptr;
    }

    MemPool *GetManagerMemPool()
    {
        return allocator.GetMemPool();
    }

    void ClearAllPhases()
    {
        phasesSequence.clear();
    }

#define ADDMAPLEPHASE(PhaseName, condition) AddPhase(PhaseName, condition)
#define ADDMAPLEMEPHASE(PhaseName, condition) \
    if (!MeOption::IsSkipPhase(PhaseName)) {  \
        AddPhase(PhaseName, condition);       \
    }
#define ADDMODULEPHASE(PhaseName, condition)                  \
    if (!Options::IsSkipPhase(PhaseName) && IsRunMpl2Mpl()) { \
        AddPhase(PhaseName, condition);                       \
    }
    void AddPhase(const std::string &phaseName, bool condition);
    AnalysisDep *FindAnalysisDep(const MaplePhase *phase);

    void SetQuiet(bool value)
    {
        quiet = value;
    }

    bool IsQuiet() const
    {
        return quiet;
    }

    void LogDependence(const MaplePhaseInfo *curPhase, int depLev)
    {
        std::string prefix = "";
        while (depLev > 0) {
            prefix += "  ";
            depLev--;
        }
        if (!IsQuiet()) {
            LogInfo::MapleLogger() << prefix << "  ++ trigger phase [ " << curPhase->PhaseName() << " ]\n";
        }
    }

    void SolveSkipFrom(const std::string &phaseName, size_t &i);
    void SolveSkipAfter(const std::string &phaseName, size_t &i);

    /* phase time record */
    void InitTimeHandler(uint32 threadNum = 1)
    {
        phaseTh = GetManagerMemPool()->New<PhaseTimeHandler>(*GetManagerMemPool(), threadNum);
    }
    void DumpPhaseTime();

    /* mempool */
    std::unique_ptr<ThreadLocalMemPool> AllocateMemPoolInPhaseManager(const std::string &mempoolName);
    bool UseGlobalMpCtrler() const
    {
        return useGlobalMpCtrler;
    }

    template <typename phaseT, typename IRTemplate>
    void RunDependentPhase(const MaplePhase &phase, AnalysisDataManager &adm, IRTemplate &irUnit, int lev = 0);
    template <typename phaseT, typename IRTemplate>
    bool RunTransformPhase(const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, IRTemplate &irUnit, int lev = 0);
    template <typename phaseT, typename IRTemplate>
    bool RunAnalysisPhase(const MaplePhaseInfo &phaseInfo, AnalysisDataManager &adm, IRTemplate &irUnit, int lev = 0);

protected:
    MapleAllocator allocator;
    MapleVector<MaplePhaseID> phasesSequence;
    // write in multithread once. read any time
    MapleMap<MaplePhaseID, AnalysisDep *> analysisDepMap;
    // thread mempool for analysisDataManger and analysis info hook
    MapleUnorderedMap<std::thread::id, MemPool *> threadMemPools;

private:
    // in serial model. there is no analysisataManager.
    MapleUnorderedMap<std::thread::id, AnalysisDataManager *> analysisDataManagers;
    PhaseTimeHandler *phaseTh = nullptr;
    bool skipFromFlag = false;
    bool skipAfterFlag = false;
    bool quiet = false;

    /*
     * use global/local mempool controller to allocate mempool
     * Use global mempool : memPoolCtrler  [reduce memory occupancy]
     * Use local mempool : innerCtrler     [reduce compiling time]
     * Can be deleted after testing
     */
    bool useGlobalMpCtrler = true;
};

class AnalysisInfoHook {
public:
    AnalysisInfoHook(MemPool &memPool, AnalysisDataManager &adm, MaplePhaseManager *bpm)
        : allocator(&memPool), adManager(adm), bindingPM(bpm), analysisPhasesData(allocator.Adapter())
    {
    }
    virtual ~AnalysisInfoHook() = default;
    void AddAnalysisData(uint32 phaseKey, MaplePhaseID id, MaplePhase *phaseImpl)
    {
        (void)analysisPhasesData.emplace(
            std::pair<AnalysisMemKey, MaplePhase *>(AnalysisMemKey(phaseKey, id), phaseImpl));
    }

    MaplePhase *FindAnalysisData(uint32 phaseKey, const MaplePhase *p, MaplePhaseID id)
    {
        auto anaPhaseInfoIt = analysisPhasesData.find(AnalysisMemKey(phaseKey, id));
        if (anaPhaseInfoIt != analysisPhasesData.end()) {
            return anaPhaseInfoIt->second;
        } else {
            /* fill all required analysis phase at first time */
            AnalysisDep *anaDependence = bindingPM->FindAnalysisDep(p);
            for (auto requiredAnaPhase : anaDependence->GetRequiredPhase()) {
                const MaplePhaseInfo *requiredPhase =
                    MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(requiredAnaPhase);
                if (!requiredPhase->IsAnalysis()) {
                    continue;
                }
                AddAnalysisData(phaseKey, requiredAnaPhase,
                                adManager.GetVaildAnalysisPhase(phaseKey, requiredAnaPhase));
            }
            DEBUG_ASSERT(analysisPhasesData.find(AnalysisMemKey(phaseKey, id)) != analysisPhasesData.end(),
                         "Need Analysis Dependence info");
            return analysisPhasesData[AnalysisMemKey(phaseKey, id)];
        }
    }

    /* Find analysis Data which is at higher IR level */
    template <typename AIMPHASE, typename IRUnit>
    MaplePhase *GetOverIRAnalyisData(const IRUnit &u)
    {
        MaplePhase *it = dynamic_cast<MaplePhase *>(bindingPM);
        DEBUG_ASSERT(it != nullptr, "find Over IR info failed");
        return it->GetAnalysisInfoHook()->FindAnalysisData(u.GetUniqueID(), it, &AIMPHASE::id);
    }

    /* Find analysis Data which is at highest IR level */
    template <typename AIMPHASE, typename IRUnit>
    MaplePhase *GetTopLevelAnalyisData(const IRUnit &u)
    {
        MaplePhase *curPhase = nullptr;
        MaplePhase *upperPhase = dynamic_cast<MaplePhase *>(bindingPM);
        DEBUG_ASSERT(upperPhase != nullptr, "find Over IR info failed");
        AnalysisInfoHook *curHook = this;
        AnalysisInfoHook *upperHook = upperPhase->GetAnalysisInfoHook();
        while (upperHook != nullptr) {
            curPhase = upperPhase;
            curHook = upperHook;
            upperPhase = dynamic_cast<MaplePhase *>(upperHook->bindingPM);
            upperHook = upperPhase->GetAnalysisInfoHook();
        }
        return curHook->FindAnalysisData(u.GetUniqueID(), curPhase, &AIMPHASE::id);
    }

    MemPool *GetOverIRMempool()
    {
        return bindingPM->GetManagerMemPool();
    }

    /* Use In O2 carefully */
    template <typename PHASEType, typename IRTemplate>
    MaplePhase *ForceRunAnalysisPhase(MaplePhaseID anaPid, IRTemplate &irUnit, int depLev = 1)
    {
        const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(anaPid);
        if (adManager.IsAnalysisPhaseAvailable(irUnit.GetUniqueID(), curPhase->GetPhaseID())) {
            return adManager.GetVaildAnalysisPhase(irUnit.GetUniqueID(), anaPid);
        }
        bindingPM->LogDependence(curPhase, depLev);
        (void)bindingPM->RunAnalysisPhase<PHASEType, IRTemplate>(*curPhase, adManager, irUnit, depLev);
        return adManager.GetVaildAnalysisPhase(irUnit.GetUniqueID(), anaPid);
    }

    template <typename PHASEType, typename IRTemplate>
    void ForceRunTransFormPhase(MaplePhaseID anaPid, IRTemplate &irUnit, int depLev = 1)
    {
        const MaplePhaseInfo *curPhase = MaplePhaseRegister::GetMaplePhaseRegister()->GetPhaseByID(anaPid);
        bindingPM->LogDependence(curPhase, depLev);
        (void)bindingPM->RunTransformPhase<PHASEType, IRTemplate>(*curPhase, adManager, irUnit, depLev);
    }

    /* Use In O2 carefully */
    void ForceEraseAnalysisPhase(uint32 phaseKey, MaplePhaseID anaPid)
    {
        adManager.EraseAnalysisPhase(phaseKey, anaPid);
    }

    /* Use In O2 carefully */
    void ForceEraseAllAnalysisPhase()
    {
        adManager.EraseAllAnalysisPhase();
    }

private:
    MapleAllocator allocator;
    AnalysisDataManager &adManager;
    MaplePhaseManager *bindingPM;
    MapleMap<AnalysisMemKey, MaplePhase *> analysisPhasesData;
};

/* manages (module phases) & (funtion phase managers) */
class ModulePM : public MaplePhase, public MaplePhaseManager {
public:
    ModulePM(MemPool *mp, MaplePhaseID id) : MaplePhase(kModulePM, id, *mp), MaplePhaseManager(*mp) {}
    virtual ~ModulePM() = default;
};

/* manages (function phases) & (loop/region phase managers) */
class FunctionPM : public MapleModulePhase, public MaplePhaseManager {
public:
    FunctionPM(MemPool *mp, MaplePhaseID id) : MapleModulePhase(id, mp), MaplePhaseManager(*mp) {}
    virtual ~FunctionPM() = default;
};

/* manages (scc phases) */
class SccPM : public MapleModulePhase, public MaplePhaseManager {
public:
    SccPM(MemPool *mp, MaplePhaseID id) : MapleModulePhase(id, mp), MaplePhaseManager(*mp) {}
    virtual ~SccPM() = default;
};

/* manages (function phases in function phase) */
template <typename IRType>
class FunctionPhaseGroup : public MapleFunctionPhase<IRType>, public MaplePhaseManager {
public:
    FunctionPhaseGroup(MemPool *mp, MaplePhaseID id) : MapleFunctionPhase<IRType>(&id, mp), MaplePhaseManager(*mp) {}
    virtual ~FunctionPhaseGroup() = default;
};
}  // namespace maple
#endif  // MAPLE_PHASE_MANAGER_H
