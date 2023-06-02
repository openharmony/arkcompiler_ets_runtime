/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_GC_STATS_H
#define ECMASCRIPT_MEM_GC_STATS_H

#include <chrono>
#include <cstring>
#include <ctime>

#include "ecmascript/common.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/mem_common.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
class Heap;

enum class GCType : int {
    STW_YOUNG_GC = 0,
    PARTIAL_YOUNG_GC,
    PARTIAL_OLD_GC,
    COMPRESS_GC,
    OTHER,
    START,
};

enum class RecordData : uint8_t {
#define DEFINE_SCOPE(scope) scope,
    RECORD_DATA(DEFINE_SCOPE)
#undef DEFINE_SCOPE
    NUM_OF_DATA,
    FIRST_DATA = START_OBJ_SIZE,
    END_RECORD_OVERWRITE = COLLECT_REGION_SET_SIZE,
};

enum class SpeedData : uint8_t {
#define DEFINE_SCOPE(scope) scope,
    TRACE_GC_SPEED(DEFINE_SCOPE)
#undef DEFINE_SCOPE
    NUM_OF_SPEED,
};

enum class RecordDuration : uint8_t {
#define DEFINE_SCOPE(scope) scope,
    RECORD_DURATION(DEFINE_SCOPE)
#undef DEFINE_SCOPE
    NUM_OF_DURATION,
    FIRST_DATA = SEMI_MIN_PAUSE,
};

#define TRACE_GC(scope_id, gc_stats)    \
    [[maybe_unused]] GCStats::Scope sp(scope_id, gc_stats)

class GCStats {
    using Duration = std::chrono::duration<uint64_t, std::nano>;

public:
    explicit GCStats(const Heap *heap) : heap_(heap) {}
    GCStats(const Heap *heap, size_t longPuaseTime) : heap_(heap),
        longPauseTime_(longPuaseTime) {}
    ~GCStats() = default;

    void PrintStatisticResult();
    void PrintGCMemoryStatistic();
    bool CheckIfLongTimePause();
    void PrintGCStatistic();

    float GetGCSpeed(SpeedData data)
    {
        return gcSpeed_[(uint8_t)data];
    }

    void SetRecordData(RecordData dataIdx, size_t value)
    {
        recordData_[GetRecordDataIndex(dataIdx)] = value;
    }

    size_t GetRecordData(RecordData dataIdx)
    {
        return recordData_[GetRecordDataIndex(dataIdx)];
    }

    void SetGCReason(GCReason reason)
    {
        reason_ = reason;
    }

    GCReason GetGCReason() const
    {
        return reason_;
    }

    double GetAvgSurvivalRate()
    {
        double copiedRate = double(GetRecordData(RecordData::YOUNG_TOTAL_ALIVE)) /
                            GetRecordData(RecordData::YOUNG_TOTAL_COMMIT);
        double promotedRate = double(GetRecordData(RecordData::YOUNG_TOTAL_PROMOTE)) /
                              GetRecordData(RecordData::YOUNG_TOTAL_COMMIT);
        return std::min(copiedRate + promotedRate, 1.0);
    }

    void RecordGCSpeed();
    void RecordStatisticBeforeGC(TriggerGCType gcType, GCReason reason);
    void RecordStatisticAfterGC();

    class Scope : public ClockScope {
    public:
        enum ScopeId : uint8_t {
#define DEFINE_SCOPE(scope) scope,
            SCOPE_LIST(DEFINE_SCOPE)
#undef DEFINE_SCOPE
            SCOPE_NUM,
        };

        Scope(ScopeId id, GCStats* stats) : id_(id), stats_(stats)
        {
            if (id_ == ScopeId::ConcurrentMark) {
                stats_->NotifyConcurrentMark();
            }
        }
        ~Scope()
        {
            float duration = stats_->PrintTimeMilliseconds(stats_->TimeToMicroseconds(GetPauseTime()));
            stats_->SetScopeId(id_, duration);
        }

    private:
        ScopeId id_;
        GCStats* stats_;
    };

private:
    bool CheckIfNeedPrint(GCType type);
    void PrintVerboseGCStatistic();
    void PrintGCDurationStatistic();
    void PrintGCSummaryStatistic(GCType type = GCType::START);
    GCType GetGCType(TriggerGCType gcType);
    const char *GCReasonToString();
    void InitializeRecordList();
    const char *GetGCTypeName();
    float GetConcurrrentMarkDuration();

    int GetRecordDurationIndex(RecordDuration durationIdx)
    {
        return (int)durationIdx - (int)RecordDuration::FIRST_DATA;
    }

    float GetRecordDuration(RecordDuration durationIdx)
    {
        return recordDuration_[GetRecordDurationIndex(durationIdx)];
    }

    void SetRecordDuration(RecordDuration durationIdx, float value)
    {
        recordDuration_[GetRecordDurationIndex(durationIdx)] = value;
    }

    void IncreaseRecordDuration(RecordDuration durationIdx, float value)
    {
        recordDuration_[GetRecordDurationIndex(durationIdx)] += value;
    }

    int GetRecordDataIndex(RecordData dataIdx)
    {
        return (int)dataIdx - (int)RecordData::FIRST_DATA;
    }

    void IncreaseRecordData(RecordData dataIdx, size_t value = 1)
    {
        recordData_[GetRecordDataIndex(dataIdx)] += value;
    }

    void SetScopeId(int pos, float duration)
    {
        scopeDuration_[pos] = duration;
    }

    void NotifyConcurrentMark()
    {
        concurrentMark_ = true;
    }

    size_t TimeToMicroseconds(Duration time)
    {
        return std::chrono::duration_cast<std::chrono::microseconds>(time).count();
    }

    float PrintTimeMilliseconds(uint64_t ms)
    {
        return (float)ms / THOUSAND;
    }

    float sizeToMB(size_t size)
    {
        return (float)size / 1_MB;
    }

    float sizeToKB(size_t size)
    {
        return (float)size / 1_KB;
    }

    const Heap *heap_ {nullptr};
    size_t longPauseTime_ = 0;

    static constexpr size_t DEFAULT_UPDATE_REFERENCE_SPEED = 10_MB;
    static constexpr size_t DEFAULT_OLD_CLEAR_NATIVE_OBJ_SPEED = 1_KB;
    static constexpr size_t DEFAULT_OLD_EVACUATE_SPACE_SPEED = 600_KB;
    static constexpr size_t DEFAULT_YOUNG_CLEAR_NATIVE_OBJ_SPEED = 3_KB;

    GCType gcType_ {GCType::START};
    GCReason reason_ {GCReason::OTHER};
    float scopeDuration_[Scope::ScopeId::SCOPE_NUM] {0.0f};
    size_t recordData_[(uint8_t)RecordData::NUM_OF_DATA];
    size_t gcSpeed_ [(uint8_t)SpeedData::NUM_OF_SPEED] = {
        DEFAULT_UPDATE_REFERENCE_SPEED, DEFAULT_OLD_CLEAR_NATIVE_OBJ_SPEED,
        DEFAULT_OLD_EVACUATE_SPACE_SPEED, DEFAULT_YOUNG_CLEAR_NATIVE_OBJ_SPEED};
    float recordDuration_[(uint8_t)RecordDuration::NUM_OF_DURATION] {0.0f};
    bool concurrentMark_ {false};

    static constexpr uint32_t THOUSAND = 1000;

    NO_COPY_SEMANTIC(GCStats);
    NO_MOVE_SEMANTIC(GCStats);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_GC_STATS_H
