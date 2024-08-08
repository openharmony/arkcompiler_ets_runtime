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

#ifndef MAPLE_PHASE_INCLUDE_MAPLE_PHASE_SUPPORT_H
#define MAPLE_PHASE_INCLUDE_MAPLE_PHASE_SUPPORT_H
#include <map>
#include <thread>
#include <string>
#include <iostream>
#include "mempool.h"
#include "maple_string.h"
#include "mpl_timer.h"
#include "mempool_allocator.h"

namespace maple {
using MaplePhaseID = const void *;
class MaplePhase;
typedef MaplePhase *(*MaplePhase_t)(MemPool *);

// base class of analysisPhase's result
class AnalysisResult {
public:
    explicit AnalysisResult(MemPool *memPoolParam)
    {
        DEBUG_ASSERT(memPoolParam != nullptr, "memPoolParam is null in AnalysisResult::AnalysisResult");
        memPool = memPoolParam;
    }

    virtual ~AnalysisResult() = default;

    MemPool *GetMempool() const
    {
        return memPool;
    }

    void EraseMemPool()
    {
        delete memPool;
        memPool = nullptr;
    }

protected:
    MemPool *memPool;
};

/* record every phase known by the system */
class MaplePhaseInfo {
public:
    MaplePhaseInfo(const std::string &pName, MaplePhaseID pID, bool isAS, bool isCFGonly, bool canSkip)
        : phaseName(pName), phaseID(pID), isAnalysis(isAS), isCFGOnlyPass(isCFGonly), canSkip(canSkip)
    {
    }

    ~MaplePhaseInfo()
    {
        constructor = nullptr;
        phaseID = nullptr;
    };
    MaplePhaseID GetPhaseID() const
    {
        return phaseID;
    }
    void SetConstructor(MaplePhase_t newConstructor)
    {
        constructor = newConstructor;
    }
    MaplePhase_t GetConstructor() const
    {
        return constructor;
    }
    bool IsAnalysis() const
    {
        return isAnalysis;
    };
    bool IsCFGonly() const
    {
        return isCFGOnlyPass;
    };
    std::string PhaseName() const
    {
        return phaseName;
    }
    bool CanSkip() const
    {
        return canSkip;
    }
    MaplePhase_t constructor = nullptr;

private:
    std::string phaseName;
    MaplePhaseID phaseID;
    const bool isAnalysis;
    const bool isCFGOnlyPass;
    const bool canSkip;
};

class PhaseTimeHandler {
public:
    PhaseTimeHandler(MemPool &memPool, uint32_t threadNum = 1)
        : allocator(&memPool),
          phaseTimeRecord(allocator.Adapter()),
          originOrder(allocator.Adapter()),
          multiTimers(allocator.Adapter())
    {
        if (threadNum > 1) {
            isMultithread = true;
        }
    }
    virtual ~PhaseTimeHandler() = default;
    void RunBeforePhase(const MaplePhaseInfo &pi);
    void RunAfterPhase(const MaplePhaseInfo &pi);
    void DumpPhasesTime();

private:
    MapleAllocator allocator;
    MapleMap<std::string, long> phaseTimeRecord;
    MapleVector<MapleMap<std::string, long>::iterator> originOrder;
    long phaseTotalTime = 0;
    MPLTimer timer;
    bool isMultithread = false;
    MapleMap<std::thread::id, MPLTimer *> multiTimers;
};

// usasge :: analysis dependency
class AnalysisDep {
public:
    explicit AnalysisDep(MemPool &mp)
        : allocator(&mp),
          required(allocator.Adapter()),
          preserved(allocator.Adapter()),
          preservedExcept(allocator.Adapter()) {};
    virtual ~AnalysisDep() = default;
    template <class PhaseT>
    void AddRequired()
    {
        required.emplace_back(&PhaseT::id);
    }
    template <class PhaseT>
    void AddPreserved()
    {
        (void)preserved.emplace(&PhaseT::id);
    }
    template <class PhaseT>
    void PreservedAllExcept()
    {
        SetPreservedAll();
        (void)preservedExcept.emplace(&PhaseT::id);
    }
    void SetPreservedAll()
    {
        preservedAll = true;
    }
    bool GetPreservedAll() const
    {
        return preservedAll;
    }
    bool FindIsPreserved(const MaplePhaseID pid)
    {
        return preserved.find(pid) != preserved.end();
    }
    const MapleVector<MaplePhaseID> &GetRequiredPhase() const;
    const MapleSet<MaplePhaseID> &GetPreservedPhase() const;
    const MapleSet<MaplePhaseID> &GetPreservedExceptPhase() const;

private:
    MapleAllocator allocator;
    MapleVector<MaplePhaseID> required;
    MapleSet<MaplePhaseID> preserved;        // keep analysis result as it is
    MapleSet<MaplePhaseID> preservedExcept;  // keep analysis result except
    bool preservedAll = false;
};
}  // namespace maple
#endif  // MAPLE_PHASE_SUPPORT_H
