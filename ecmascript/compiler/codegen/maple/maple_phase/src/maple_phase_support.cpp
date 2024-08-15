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

#include "maple_phase_support.h"

#include "cgfunc.h"
namespace maple {
#ifdef ARK_LITECG_DEBUG
void PhaseTimeHandler::RunBeforePhase(const MaplePhaseInfo &pi)
{
    (void)pi;
    if (isMultithread) {
        std::thread::id tid = std::this_thread::get_id();
        if (!multiTimers.count(tid)) {
            multiTimers.emplace(std::make_pair(tid, allocator.New<MPLTimer>()));
        }
        multiTimers[tid]->Start();
    } else {
        timer.Start();
    }
}

void PhaseTimeHandler::RunAfterPhase(const MaplePhaseInfo &pi)
{
    long usedTime = 0;
    if (isMultithread) {
        std::thread::id tid = std::this_thread::get_id();
        if (multiTimers.count(tid)) {
            multiTimers[tid]->Stop();
            usedTime += multiTimers[tid]->ElapsedMicroseconds();
        } else {
            DEBUG_ASSERT(false, " phase time handler create failed");
        }
    } else {
        timer.Stop();
        usedTime = timer.ElapsedMicroseconds();
    }
    std::string phaseName = pi.PhaseName();
    if (phaseTimeRecord.count(phaseName)) {
        phaseTimeRecord[phaseName] += usedTime;
    } else {
        auto ret = phaseTimeRecord.emplace(std::make_pair(phaseName, usedTime));
        if (ret.second) {
            originOrder.push_back(ret.first);
        }
    }
    phaseTotalTime += usedTime;
}

void PhaseTimeHandler::DumpPhasesTime()
{
    auto TimeLogger = [](const std::string &itemName, time_t itemTimeUs, time_t totalTimeUs) {
        LogInfo::MapleLogger() << std::left << std::setw(25) << std::setfill(' ') << itemName << std::setw(10)
                               << std::setfill(' ') << std::right << std::fixed << std::setprecision(2)
                               << (maplebe::kPercent * itemTimeUs / totalTimeUs) << "%" << std::setw(10)
                               << std::setfill(' ') << std::setprecision(4)
                               << (itemTimeUs / (maplebe::kMicroSecPerMilliSec * maplebe::kMicroSecPerMilliSec))
                               << "s\n";
    };

    LogInfo::MapleLogger() << "\n================== TIMEPHASES ==================\n";
    LogInfo::MapleLogger() << "================================================\n";
    for (auto phaseIt : originOrder) {
        /*
         * output information by specified format, setw function parameter specifies show width
         * setprecision function parameter specifies precision
         */
        TimeLogger(phaseIt->first, phaseIt->second, phaseTotalTime);
    }
    LogInfo::MapleLogger() << "================================================\n";
    TimeLogger("Total", phaseTotalTime, phaseTotalTime);
    LogInfo::MapleLogger() << "================================================\n\n";
    LogInfo::MapleLogger().unsetf(std::ios::fixed);
}
#endif

const MapleVector<MaplePhaseID> &AnalysisDep::GetRequiredPhase() const
{
    return required;
}
const MapleSet<MaplePhaseID> &AnalysisDep::GetPreservedPhase() const
{
    return preserved;
}
const MapleSet<MaplePhaseID> &AnalysisDep::GetPreservedExceptPhase() const
{
    return preservedExcept;
}
}  // namespace maple
