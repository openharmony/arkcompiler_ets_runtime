/*
 * Copyright (c) 2021-2024 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_MEM_CONTROLLER_H
#define ECMASCRIPT_MEM_MEM_CONTROLLER_H

#include <chrono>
#include <cmath>
#include <limits>

#include "ecmascript/base/gc_ring_buffer.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {

constexpr static int MILLISECONDS_PER_SECOND = 1000;

using BytesAndDuration = std::pair<uint64_t, double>;

class MemController {
public:
    explicit MemController(Heap* heap);
    MemController() = default;
    ~MemController() = default;
    NO_COPY_SEMANTIC(MemController);
    NO_MOVE_SEMANTIC(MemController);

    static double GetSystemTimeInMs()
    {
        double currentTime =
            std::chrono::duration<double>(std::chrono::high_resolution_clock::now().time_since_epoch()).count();
        return currentTime * MILLISECOND_PER_SECOND;
    }

    size_t CalculateAllocLimit(size_t currentSize, size_t minSize, size_t maxSize, size_t newSpaceCapacity,
                               double factor) const;

    double CalculateGrowingFactor(double gcSpeed, double mutatorSpeed);

    void StartCalculationBeforeGC();
    void StopCalculationAfterGC(TriggerGCType gcType);

    void RecordAllocationForIdle();
    double GetIdleNewSpaceAllocationThroughputPerMS() const;
    double GetIdleOldSpaceAllocationThroughputPerMS() const;
    bool CheckLowAllocationUsageState() const;

    void RecordAfterConcurrentMark(MarkType markType, const ConcurrentMarker *marker);

    double CalculateMarkCompactSpeedPerMS();
    double GetCurrentOldSpaceAllocationThroughputPerMS(double timeMs = THROUGHPUT_TIME_FRAME_MS) const;
    double GetEdenSpaceAllocationThroughputPerMS() const;
    double GetNewSpaceAllocationThroughputPerMS() const;
    double GetOldSpaceAllocationThroughputPerMS() const;
    double GetEdenSpaceConcurrentMarkSpeedPerMS() const;
    double GetNewSpaceConcurrentMarkSpeedPerMS() const;
    double GetFullSpaceConcurrentMarkSpeedPerMS() const;

    double GetAllocTimeMs() const
    {
        return allocTimeMs_;
    }

    size_t GetOldSpaceAllocAccumulatedSize() const
    {
        return oldSpaceAllocAccumulatedSize_;
    }

    size_t GetNonMovableSpaceAllocAccumulatedSize() const
    {
        return nonMovableSpaceAllocAccumulatedSize_;
    }

    size_t GetCodeSpaceAllocAccumulatedSize() const
    {
        return codeSpaceAllocAccumulatedSize_;
    }

    double GetAllocDurationSinceGc() const
    {
        return allocDurationSinceGc_;
    }

    size_t GetEdenSpaceAllocSizeSinceGC() const
    {
        return edenSpaceAllocSizeSinceGC_;
    }

    size_t GetNewSpaceAllocSizeSinceGC() const
    {
        return newSpaceAllocSizeSinceGC_;
    }

    size_t GetOldSpaceAllocSizeSinceGC() const
    {
        return oldSpaceAllocSizeSinceGC_;
    }

    size_t GetNonMovableSpaceAllocSizeSinceGC() const
    {
        return nonMovableSpaceAllocSizeSinceGC_;
    }

    size_t GetCodeSpaceAllocSizeSinceGC() const
    {
        return codeSpaceAllocSizeSinceGC_;
    }

    size_t GetHugeObjectAllocSizeSinceGC() const
    {
        return hugeObjectAllocSizeSinceGC_;
    }

    void AddSurvivalRate(double rate)
    {
        recordedSurvivalRates_.Push(rate);
        if (UNLIKELY(std::isnan(predictedSurvivalRate_))) {
            predictedSurvivalRate_ = rate;
        } else {
            predictedSurvivalRate_ = ALPHA * rate + (1 - ALPHA) * predictedSurvivalRate_;
        }
    }

    double GetAverageSurvivalRate() const
    {
        int count = recordedSurvivalRates_.Count();
        if (count == 0) {
            return 0;
        }
        double result = recordedSurvivalRates_.Sum([](double x, double y) { return x + y;}, 0.0);
        return result / count;
    }

    void AddEdenSurvivalRate(double rate)
    {
        recordedEdenSurvivalRates_.Push(rate);
    }

    double GetAverageEdenSurvivalRate() const
    {
        int count = recordedEdenSurvivalRates_.Count();
        if (count == 0) {
            return 0;
        }
        double result = recordedEdenSurvivalRates_.Sum([](double x, double y) { return x + y;}, 0.0);
        return result / count;
    }

    double GetPredictedSurvivalRate() const
    {
        if (UNLIKELY(std::isnan(predictedSurvivalRate_))) {
            return 0;
        }
        return predictedSurvivalRate_;
    }

    void ResetRecordedSurvivalRates()
    {
        recordedSurvivalRates_.Reset();
    }

private:
    static constexpr int LENGTH = 10;
    // Decayed weight for predicting survival rate.
    static constexpr double ALPHA = 0.8;
    static double CalculateAverageSpeed(const base::GCRingBuffer<BytesAndDuration, LENGTH> &buffer);
    static double CalculateAverageSpeed(const base::GCRingBuffer<BytesAndDuration, LENGTH> &buffer,
                                        const BytesAndDuration &initial, const double timeMs);

    Heap* heap_;
    size_t minAllocLimitGrowingStep_ {0};

    double gcStartTime_ {0.0};
    double gcEndTime_ {0.0};

    // Time and allocation accumulators.
    double allocTimeMs_ {0.0};
    size_t oldSpaceAllocAccumulatedSize_ {0};
    size_t nonMovableSpaceAllocAccumulatedSize_ {0};
    size_t codeSpaceAllocAccumulatedSize_ {0};

    // Duration and allocation size in last gc.
    double allocDurationSinceGc_ {0.0};
    size_t edenSpaceAllocSizeSinceGC_ {0};
    size_t newSpaceAllocSizeSinceGC_ {0};
    size_t oldSpaceAllocSizeSinceGC_ {0};
    size_t nonMovableSpaceAllocSizeSinceGC_ {0};
    size_t codeSpaceAllocSizeSinceGC_ {0};
    size_t hugeObjectAllocSizeSinceGC_{0};

    // Records data at idle time points.
    double allocTimeMsIdle_ {0.0};
    size_t newSpaceRecordLastTimeSizeIdle_ {0};
    size_t oldSpaceRecordLastTimeSizeIdle_ {0};
    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedIdleNewSpaceAllocations_;
    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedIdleOldSpaceAllocations_;

    int startCounter_ {0};
    double markCompactSpeedCache_ {0.0};

    double predictedSurvivalRate_ {std::numeric_limits<double>::quiet_NaN()};

    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedMarkCompacts_;
    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedEdenSpaceAllocations_;
    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedNewSpaceAllocations_;
    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedOldSpaceAllocations_;
    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedNonmovableSpaceAllocations_;
    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedCodeSpaceAllocations_;

    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedConcurrentMarks_;
    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedEdenConcurrentMarks_;
    base::GCRingBuffer<BytesAndDuration, LENGTH> recordedSemiConcurrentMarks_;
    base::GCRingBuffer<double, LENGTH> recordedSurvivalRates_;
    base::GCRingBuffer<double, LENGTH> recordedEdenSurvivalRates_;

    static constexpr double THROUGHPUT_TIME_FRAME_MS = 5000;
    static constexpr int MILLISECOND_PER_SECOND = 1000;
};

MemController *CreateMemController(Heap *heap, std::string_view gcTriggerType);
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_MEM_CONTROLLER_H
