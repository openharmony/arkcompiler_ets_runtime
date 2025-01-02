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
#include "ecmascript/mem/long_gc_stats.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
class Heap;
class SharedHeap;

enum class GCType : int {
    PARTIAL_EDEN_GC,
    PARTIAL_YOUNG_GC,
    PARTIAL_OLD_GC,
    COMPRESS_GC,
    SHARED_GC,
    SHARED_FULL_GC,
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
    explicit GCStats(const Heap *heap) : heap_(heap)
    {
        longGCStats_ = new LongGCStats();
    }
    GCStats(const Heap *heap, size_t longPuaseTime) : heap_(heap),
        longPauseTime_(longPuaseTime)
    {
        longGCStats_ = new LongGCStats();
    }
    virtual ~GCStats()
    {
        if (longGCStats_ != nullptr) {
            delete longGCStats_;
            longGCStats_ = nullptr;
        }
    };

    virtual void PrintStatisticResult();
    virtual void PrintGCMemoryStatistic();
    bool CheckIfLongTimePause();
    virtual void PrintGCStatistic();
    GCType GetGCType(TriggerGCType gcType);
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

    LongGCStats *GetLongGCStats()
    {
        return longGCStats_;
    }

    bool IsLongGC(GCReason gcReason, bool gcIsSensitive, bool gcIsInBackground, float gcTotalTime);

    const char *GetGCTypeName()
    {
        switch (gcType_) {
            case GCType::PARTIAL_EDEN_GC:
                return "HPP EdenGC";
            case GCType::PARTIAL_YOUNG_GC:
                return "HPP YoungGC";
            case GCType::PARTIAL_OLD_GC:
                return "HPP OldGC";
            case GCType::COMPRESS_GC:
                return "CompressGC";
            case GCType::SHARED_GC:
                return "SharedGC";
            case GCType::SHARED_FULL_GC:
                return "SharedCompressGC";
            default:
                return "UnknownType";
        }
    }

    static const char *GCReasonToString(GCReason reason);
    const char *GCReasonToString();

    double GetAvgSurvivalRate()
    {
        if (gcType_ == GCType::PARTIAL_EDEN_GC) {
            size_t commitSize = GetRecordData(RecordData::EDEN_TOTAL_COMMIT);
            if (commitSize == 0) {
                return 0;
            }
            double copiedRate = double(GetRecordData(RecordData::EDEN_TOTAL_ALIVE)) / commitSize;
            double promotedRate = double(GetRecordData(RecordData::EDEN_TOTAL_PROMOTE)) / commitSize;
            return std::min(copiedRate + promotedRate, 1.0);
        }
        double copiedRate = double(GetRecordData(RecordData::YOUNG_TOTAL_ALIVE)) /
                            GetRecordData(RecordData::YOUNG_TOTAL_COMMIT);
        double promotedRate = double(GetRecordData(RecordData::YOUNG_TOTAL_PROMOTE)) /
                              GetRecordData(RecordData::YOUNG_TOTAL_COMMIT);
        return std::min(copiedRate + promotedRate, 1.0);
    }

    virtual void RecordGCSpeed();
    virtual void RecordStatisticBeforeGC(TriggerGCType gcType, GCReason reason);
    virtual void RecordStatisticAfterGC();

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

    float GetScopeDuration(int pos) const
    {
        return scopeDuration_[pos];
    }

    void IncreaseTotalDuration(float duration)
    {
        gcDuration_ += duration;
    }

    size_t GetGCCount()
    {
        return GetRecordData(RecordData::SEMI_COUNT) + GetRecordData(RecordData::YOUNG_COUNT) +
            GetRecordData(RecordData::OLD_COUNT) + GetRecordData(RecordData::COMPRESS_COUNT) +
            GetRecordData(RecordData::SHARED_COUNT);
    }

    size_t GetGCDuration() const
    {
        return static_cast<size_t>(gcDuration_);
    }

    virtual size_t GetAccumulatedAllocateSize();
    size_t GetAccumulatedFreeSize() const
    {
        return accumulatedFreeSize_;
    }

    void IncreaseFullGCLongTimeCount()
    {
        fullGCLongTimeCount_ += 1;
    }

    size_t GetFullGCLongTimeCount() const
    {
        return fullGCLongTimeCount_;
    }

protected:
    bool CheckIfNeedPrint(GCType type);
    void PrintVerboseGCStatistic();
    void PrintGCDurationStatistic();
    void PrintGCSummaryStatistic(GCType type = GCType::START);
    void InitializeRecordList();
    float GetConcurrrentMarkDuration();
    void GCFinishTrace();
    virtual void ProcessBeforeLongGCStats();
    virtual void ProcessAfterLongGCStats();
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

    void IncreaseAccumulatedFreeSize(size_t size)
    {
        accumulatedFreeSize_ += size;
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
    float gcDuration_ = 0.0f;
    size_t longPauseTime_ = 0;
    size_t fullGCLongTimeCount_ = 0;
    size_t accumulatedFreeSize_ = 0;

    static constexpr size_t DEFAULT_UPDATE_REFERENCE_SPEED = 10_MB;
    static constexpr size_t DEFAULT_OLD_CLEAR_NATIVE_OBJ_SPEED = 1_KB;
    static constexpr size_t DEFAULT_OLD_EVACUATE_SPACE_SPEED = 600_KB;
    static constexpr size_t DEFAULT_YOUNG_CLEAR_NATIVE_OBJ_SPEED = 3_KB;

    GCType gcType_ {GCType::START};
    GCReason reason_ {GCReason::OTHER};
    float scopeDuration_[Scope::ScopeId::SCOPE_NUM] {0.0f};
    size_t recordData_[(uint8_t)RecordData::NUM_OF_DATA] {0};
    size_t gcSpeed_ [(uint8_t)SpeedData::NUM_OF_SPEED] = {
        DEFAULT_UPDATE_REFERENCE_SPEED, DEFAULT_OLD_CLEAR_NATIVE_OBJ_SPEED,
        DEFAULT_OLD_EVACUATE_SPACE_SPEED, DEFAULT_YOUNG_CLEAR_NATIVE_OBJ_SPEED};
    float recordDuration_[(uint8_t)RecordDuration::NUM_OF_DURATION] {0.0f};
    bool concurrentMark_ {false};

    static constexpr uint32_t THOUSAND = 1000;

    NO_COPY_SEMANTIC(GCStats);
    NO_MOVE_SEMANTIC(GCStats);
private:
    LongGCStats *longGCStats_;
};

class SharedGCStats : public GCStats {
public:
    SharedGCStats(const SharedHeap *sHeap, bool enableGCTracer)
        : GCStats(nullptr), sHeap_(sHeap), enableGCTracer_(enableGCTracer)
    {
        SetRecordData(RecordData::SHARED_COUNT, 0);
    }
    ~SharedGCStats() = default;

    void PrintStatisticResult() override;
    void PrintGCMemoryStatistic() override;
    void PrintGCStatistic() override;

    void RecordStatisticBeforeGC(TriggerGCType gcType, GCReason reason) override;
    void RecordStatisticAfterGC() override;
    size_t GetAccumulatedAllocateSize() override;
private:
    void PrintSharedGCDuration();
    void PrintSharedGCSummaryStatistic();
    void SharedGCFinishTrace();
    void ProcessAfterLongGCStats() override;
    void ProcessBeforeLongGCStats() override;

    const SharedHeap *sHeap_ {nullptr};
    bool enableGCTracer_ {false};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_GC_STATS_H
