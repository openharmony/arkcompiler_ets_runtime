/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_GC_KEY_STATS_H
#define ECMASCRIPT_MEM_GC_KEY_STATS_H

#include <ctime>
#include <chrono>
#include <string>

#include "ecmascript/common_enum.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/log_wrapper.h"

#include "libpandabase/macros.h"

namespace panda::ecmascript {
using Clock = std::chrono::high_resolution_clock;
class Heap;
class GCStats;

enum class RecordKeyData : uint8_t {
    GC_TOTAL_MEM_USED = 0,
    GC_TOTAL_MEM_COMMITTED,
    GC_ACTIVE_MEM_USED,
    GC_ACTIVE_MEM_COMMITTED,
    GC_OLD_MEM_USED,
    GC_OLD_MEM_COMMITTED,
    GC_HUGE_MEM_USED,
    GC_HUGE_MEM_COMMITTED,
    NUM_OF_KEY_DATA,
    FIRST_DATA = GC_TOTAL_MEM_USED,
};

enum class RecordKeyDuration : uint8_t {
    GC_TOTAL_TIME = 0,
    GC_MARK_TIME,
    GC_EVACUATE_TIME,
    NUM_OF_KEY_DURATION,
    FIRST_DATA = GC_TOTAL_TIME,
};

class GCKeyStats {
static constexpr size_t KEY_PAUSE_TIME = 20; // ms
static constexpr int NEED_SEND_TIMES = 200;
static constexpr int MIN_SEND_INTERVAL = 60; // seconds
public:
    explicit GCKeyStats(const Heap *heap) : heap_(heap) {}
    GCKeyStats(const Heap *heap, GCStats *gcStats) : heap_(heap),
        gcStats_(gcStats) {}
    ~GCKeyStats() = default;

    static constexpr int GC_SENSITIVE_LONG_TIME = 33;
    static constexpr int GC_NOT_SENSITIVE_LONG_TIME = 33;
    static constexpr int GC_IDLE_LONG_TIME = 200;
    static constexpr int GC_BACKGROUD_LONG_TIME = 200;
    static constexpr int GC_BACKGROUD_IDLE_LONG_TIME = 500;
    void AddGCStatsToKey();
    bool CheckIfMainThread() const;
    bool CheckIfKeyPauseTime() const;
    void SendSysEventBeforeDump(std::string type, size_t limitSize, size_t activeMemory,
                                const std::string &eventConfig) const;
    int32_t SendSysEventDataSize(std::vector<std::string> filePaths,  std::vector<uint64_t> fileSizes) const;
    void ProcessLongGCEvent();
    static bool IsIdle(GCReason gcReason)
    {
        if (gcReason == GCReason::IDLE || gcReason == GCReason::IDLE_NATIVE) {
            return true;
        }
        return false;
    }

    void IncGCCount()
    {
        gcCount_++;
    }

    void AddRecordDataStats(RecordKeyData idx, int value)
    {
        recordDataStats_[GetRecordDataStatsIndex(idx)] += value;
    }

    int GetRecordDataStats(RecordKeyData idx) const
    {
        return recordDataStats_[GetRecordDataStatsIndex(idx)];
    }

    void AddRecordKeyDuration(RecordKeyDuration idx, float value)
    {
        recordDurationStats_[GetRecordKeyDurationIndex(idx)] += value;
    }

    float GetRecordKeyDuration(RecordKeyDuration idx) const
    {
        return recordDurationStats_[GetRecordKeyDurationIndex(idx)];
    }

private:
    void SendSysEvent() const;
    void InitializeRecordList();
    void PrintKeyStatisticResult() const;
    double GetCpuUsage();
    void SendLongGCEvent();
    bool CheckLastSendTimeIfSend() const
    {
        return lastSendTimestamp_ == Clock::time_point::min() ||
            std::chrono::duration_cast<std::chrono::seconds>(Clock::now() - lastSendTimestamp_).count() >=
            MIN_SEND_INTERVAL;
    }

    bool CheckCountIfSend() const
    {
        return recordCount_ >= NEED_SEND_TIMES;
    }

    int GetRecordDataStatsIndex(RecordKeyData dataIdx) const
    {
        return (int)dataIdx - (int)RecordKeyData::FIRST_DATA;
    }

    int GetRecordKeyDurationIndex(RecordKeyDuration dataIdx) const
    {
        return (int)dataIdx - (int)RecordKeyDuration::FIRST_DATA;
    }

    int SizeToIntKB(size_t size) const
    {
        return static_cast<int>(size / 1_KB);
    }

    const Heap *heap_;
    GCStats *gcStats_;

    int gcCount_{0};
    int recordCount_{0};
    Clock::time_point lastSendTimestamp_;
    float recordDurationStats_[(uint8_t)RecordKeyDuration::NUM_OF_KEY_DURATION]{0.0f};
    int recordDataStats_[(uint8_t)RecordKeyData::NUM_OF_KEY_DATA]{0};
};

} // namespace panda::ecmascript
#endif // ECMASCRIPT_MEM_GC_KEY_STATS_H