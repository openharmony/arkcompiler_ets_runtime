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

#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/parallel_evacuator.h"

namespace panda::ecmascript {
MemController::MemController(Heap *heap) : heap_(heap), allocTimeMs_(GetSystemTimeInMs())
{
    ASSERT(heap != nullptr);
    minAllocLimitGrowingStep_ = heap->GetEcmaVM()->GetEcmaParamConfiguration().GetMinAllocLimitGrowingStep();
}

size_t MemController::CalculateAllocLimit(size_t currentSize, size_t minSize, size_t maxSize, size_t newSpaceCapacity,
                                          double factor) const
{
    const uint64_t limit = std::max(static_cast<uint64_t>(currentSize * factor),
                                    static_cast<uint64_t>(currentSize) + minAllocLimitGrowingStep_) +
                           newSpaceCapacity;

    const uint64_t limitAboveMinSize = std::max<uint64_t>(limit, minSize);
    const uint64_t halfToMaxSize = (static_cast<uint64_t>(currentSize) + maxSize) / 2;
    size_t result = static_cast<size_t>(std::min(limitAboveMinSize, halfToMaxSize));
    result = static_cast<size_t>(std::min(result, maxSize));
    return result;
}

double MemController::CalculateGrowingFactor(double gcSpeed, double mutatorSpeed)
{
    double maxGrowingFactor = 4.0;
    double halfMaxGrowingFactor = 2.0;
    double minGrowingFactor = 1.3;
    double minimumFactor = 1.1;
    switch (heap_->GetMemGrowingType()) {
        case MemGrowingType::HIGH_THROUGHPUT:
            break;
        case MemGrowingType::CONSERVATIVE:
            minGrowingFactor = minimumFactor;
            maxGrowingFactor = halfMaxGrowingFactor;
            break;
        case MemGrowingType::PRESSURE:
            return minimumFactor;
        default: // LCOV_EXCL_BR_LINE
            break;
    }

    static constexpr double targetMutatorUtilization = 0.97;
    if (gcSpeed == 0 || mutatorSpeed == 0) {
        return maxGrowingFactor;
    }

    const double speedRatio = gcSpeed / mutatorSpeed;

    const double a = speedRatio * (1 - targetMutatorUtilization);
    const double b = speedRatio * (1 - targetMutatorUtilization) -  targetMutatorUtilization;

    double factor = (a < b * maxGrowingFactor) ? a / b : maxGrowingFactor;
    factor = std::min(maxGrowingFactor, factor);
    factor = std::max(factor, minGrowingFactor);
    OPTIONAL_LOG(heap_->GetEcmaVM(), INFO) << "CalculateGrowingFactor gcSpeed"
        << gcSpeed << " mutatorSpeed" << mutatorSpeed << " factor" << factor;
    return factor;
}

void MemController::StartCalculationBeforeGC()
{
    startCounter_++;
    if (startCounter_ != 1) {
        return;
    }

    auto edenSpace = heap_->GetEdenSpace();
    size_t edenSpaceAllocBytesSinceGC = edenSpace->GetAllocatedSizeSinceGC(edenSpace->GetTop());
    // It's unnecessary to calculate newSpaceAllocAccumulatedSize. newSpaceAllocBytesSinceGC can be calculated directly.
    auto newSpace = heap_->GetNewSpace();
    size_t newSpaceAllocBytesSinceGC = newSpace->GetAllocatedSizeSinceGC(newSpace->GetTop());
    size_t hugeObjectAllocSizeSinceGC = heap_->GetHugeObjectSpace()->GetHeapObjectSize() - hugeObjectAllocSizeSinceGC_;
    size_t oldSpaceAllocAccumulatedSize = heap_->GetOldSpace()->GetTotalAllocatedSize();
    size_t nonMovableSpaceAllocAccumulatedSize = heap_->GetNonMovableSpace()->GetTotalAllocatedSize();
    size_t codeSpaceAllocAccumulatedSize = heap_->GetMachineCodeSpace()->GetTotalAllocatedSize();
    double currentTimeInMs = GetSystemTimeInMs();
    gcStartTime_ = currentTimeInMs;
    size_t oldSpaceAllocSize = oldSpaceAllocAccumulatedSize - oldSpaceAllocAccumulatedSize_;
    size_t nonMovableSpaceAllocSize = nonMovableSpaceAllocAccumulatedSize - nonMovableSpaceAllocAccumulatedSize_;
    size_t codeSpaceAllocSize = codeSpaceAllocAccumulatedSize - codeSpaceAllocAccumulatedSize_;

    double duration = currentTimeInMs - allocTimeMs_;
    allocTimeMs_ = currentTimeInMs;
    oldSpaceAllocAccumulatedSize_ = oldSpaceAllocAccumulatedSize;
    nonMovableSpaceAllocAccumulatedSize_ = nonMovableSpaceAllocAccumulatedSize;
    codeSpaceAllocAccumulatedSize_ = codeSpaceAllocAccumulatedSize;

    allocDurationSinceGc_ += duration;

    edenSpaceAllocSizeSinceGC_ += edenSpaceAllocBytesSinceGC;
    newSpaceAllocSizeSinceGC_ += newSpaceAllocBytesSinceGC;
    oldSpaceAllocSizeSinceGC_ += oldSpaceAllocSize;
    oldSpaceAllocSizeSinceGC_ += hugeObjectAllocSizeSinceGC;
    nonMovableSpaceAllocSizeSinceGC_ += nonMovableSpaceAllocSize;
    codeSpaceAllocSizeSinceGC_ += codeSpaceAllocSize;

    if (heap_->GetEcmaGCStats()->GetGCReason() != GCReason::IDLE) {
        recordedIdleNewSpaceAllocations_.Push(MakeBytesAndDuration(
            heap_->GetNewSpace()->GetHeapObjectSize() - newSpaceRecordLastTimeSizeIdle_,
            currentTimeInMs - allocTimeMsIdle_));
        recordedIdleOldSpaceAllocations_.Push(MakeBytesAndDuration(
            heap_->GetOldSpace()->GetHeapObjectSize() - oldSpaceRecordLastTimeSizeIdle_,
            currentTimeInMs - allocTimeMsIdle_));
    }
}

void MemController::RecordAllocationForIdle()
{
    double currentTimeInMs = GetSystemTimeInMs();
    size_t currentNewSpaceObjectSize = heap_->GetNewSpace()->GetHeapObjectSize();
    size_t currentOldSpaceObjectSize = heap_->GetOldSpace()->GetHeapObjectSize();
    double duration = currentTimeInMs - allocTimeMsIdle_;
    allocTimeMsIdle_ = currentTimeInMs;
    if (currentNewSpaceObjectSize < newSpaceRecordLastTimeSizeIdle_ ||
        currentOldSpaceObjectSize < oldSpaceRecordLastTimeSizeIdle_) {
        newSpaceRecordLastTimeSizeIdle_ = currentNewSpaceObjectSize;
        oldSpaceRecordLastTimeSizeIdle_ = currentOldSpaceObjectSize;
        return;
    }

    size_t newSpaceAllocSizeSinceIdle = currentNewSpaceObjectSize - newSpaceRecordLastTimeSizeIdle_;
    newSpaceRecordLastTimeSizeIdle_ = currentNewSpaceObjectSize;
    size_t oldSpaceAllocSizeSinceIdle = currentOldSpaceObjectSize - oldSpaceRecordLastTimeSizeIdle_;
    oldSpaceRecordLastTimeSizeIdle_ = currentOldSpaceObjectSize;
    recordedIdleNewSpaceAllocations_.Push(MakeBytesAndDuration(newSpaceAllocSizeSinceIdle, duration));
    recordedIdleOldSpaceAllocations_.Push(MakeBytesAndDuration(oldSpaceAllocSizeSinceIdle, duration));
}

bool MemController::CheckLowAllocationUsageState() const
{
    LOG_GC(DEBUG) << "local CheckLowAllocationUsageState NewSpaceAllocBytesPerMS:" <<
        GetIdleNewSpaceAllocationThroughputPerMS() << ",OldSpaceAllocBytesPerMS:" <<
        GetIdleOldSpaceAllocationThroughputPerMS();
    return GetIdleNewSpaceAllocationThroughputPerMS() < LOW_ALLOCATION_RATE_PER_MS &&
                GetIdleOldSpaceAllocationThroughputPerMS() < LOW_ALLOCATION_RATE_PER_MS;
}

void MemController::StopCalculationAfterGC(TriggerGCType gcType)
{
    startCounter_--;
    if (startCounter_ != 0) {
        return;
    }

    gcEndTime_ = GetSystemTimeInMs();
    allocTimeMs_ = gcEndTime_;
    if (allocDurationSinceGc_ > 0) {
        oldSpaceAllocSizeSinceGC_ += heap_->GetEvacuator()->GetPromotedSize();
        recordedEdenSpaceAllocations_.Push(MakeBytesAndDuration(edenSpaceAllocSizeSinceGC_, allocDurationSinceGc_));
        recordedNewSpaceAllocations_.Push(MakeBytesAndDuration(newSpaceAllocSizeSinceGC_, allocDurationSinceGc_));
        recordedOldSpaceAllocations_.Push(MakeBytesAndDuration(oldSpaceAllocSizeSinceGC_, allocDurationSinceGc_));
        recordedNonmovableSpaceAllocations_.Push(
            MakeBytesAndDuration(nonMovableSpaceAllocSizeSinceGC_, allocDurationSinceGc_));
        recordedCodeSpaceAllocations_.Push(MakeBytesAndDuration(codeSpaceAllocSizeSinceGC_, allocDurationSinceGc_));
    }
    allocDurationSinceGc_ = 0.0;
    edenSpaceAllocSizeSinceGC_ = 0;
    newSpaceAllocSizeSinceGC_ = 0;
    oldSpaceAllocSizeSinceGC_ = 0;
    nonMovableSpaceAllocSizeSinceGC_ = 0;
    codeSpaceAllocSizeSinceGC_ = 0;

    hugeObjectAllocSizeSinceGC_ = heap_->GetHugeObjectSpace()->GetHeapObjectSize();

    double duration = gcEndTime_ - gcStartTime_;
    switch (gcType) {
        case TriggerGCType::YOUNG_GC:
        case TriggerGCType::OLD_GC: {
            if (heap_->IsConcurrentFullMark()) {
                if (heap_->GetConcurrentMarker()->IsEnabled()) {
                    duration += heap_->GetConcurrentMarker()->GetDuration();
                }
                recordedMarkCompacts_.Push(MakeBytesAndDuration(heap_->GetHeapObjectSize(), duration));
            }
            break;
        }
        case TriggerGCType::FULL_GC: {
            recordedMarkCompacts_.Push(MakeBytesAndDuration(heap_->GetHeapObjectSize(), duration));
            break;
        }
        default:
            break;
    }

    if (heap_->GetEcmaGCStats()->GetGCReason() != GCReason::IDLE) {
        newSpaceRecordLastTimeSizeIdle_ = heap_->GetNewSpace()->GetHeapObjectSize();
        oldSpaceRecordLastTimeSizeIdle_ = heap_->GetOldSpace()->GetHeapObjectSize();
        allocTimeMsIdle_ = gcEndTime_;
    }
}

void MemController::RecordAfterConcurrentMark(MarkType markType, const ConcurrentMarker *marker)
{
    ASSERT(marker != nullptr);
    double duration = marker->GetDuration();
    if (markType == MarkType::MARK_FULL) {
        recordedConcurrentMarks_.Push(MakeBytesAndDuration(marker->GetHeapObjectSize(), duration));
    } else if (markType == MarkType::MARK_YOUNG) {
        recordedSemiConcurrentMarks_.Push(MakeBytesAndDuration(marker->GetHeapObjectSize(), duration));
    } else if (markType == MarkType::MARK_EDEN) {
        recordedEdenConcurrentMarks_.Push(MakeBytesAndDuration(marker->GetHeapObjectSize(), duration));
    }
}

double MemController::CalculateMarkCompactSpeedPerMS()
{
    markCompactSpeedCache_ = CalculateAverageSpeed(recordedMarkCompacts_);
    if (markCompactSpeedCache_ > 0) {
        return markCompactSpeedCache_;
    }
    return 0;
}

double MemController::CalculateAverageSpeed(const base::GCRingBuffer<BytesAndDuration, LENGTH> &buffer,
                                            const BytesAndDuration &initial, const double timeMs)
{
    BytesAndDuration sum = buffer.Sum(
        [timeMs](BytesAndDuration a, BytesAndDuration b) {
            if (timeMs != 0 && a.second >= timeMs) {
                return a;
            }
            return std::make_pair(a.first + b.first, a.second + b.second);
        },
        initial);
    uint64_t bytes = sum.first;
    double durations = sum.second;
    if (fabs(durations) <= 1e-6) {
        return 0;
    }
    double speed = bytes / durations;
    const int maxSpeed = static_cast<int>(1_GB);
    const int minSpeed = 1;
    if (speed >= maxSpeed) {
        return maxSpeed;
    }
    if (speed <= minSpeed) {
        return minSpeed;
    }
    return speed;
}

double MemController::CalculateAverageSpeed(const base::GCRingBuffer<BytesAndDuration, LENGTH> &buffer)
{
    return CalculateAverageSpeed(buffer, MakeBytesAndDuration(0, 0), 0);
}

double MemController::GetCurrentOldSpaceAllocationThroughputPerMS(double timeMs) const
{
    size_t allocatedSize = oldSpaceAllocSizeSinceGC_;
    double duration = allocDurationSinceGc_;
    return CalculateAverageSpeed(recordedOldSpaceAllocations_,
                                 MakeBytesAndDuration(allocatedSize, duration), timeMs);
}

double MemController::GetEdenSpaceAllocationThroughputPerMS() const
{
    return CalculateAverageSpeed(recordedEdenSpaceAllocations_);
}

double MemController::GetNewSpaceAllocationThroughputPerMS() const
{
    return CalculateAverageSpeed(recordedNewSpaceAllocations_);
}

double MemController::GetEdenSpaceConcurrentMarkSpeedPerMS() const
{
    return CalculateAverageSpeed(recordedEdenConcurrentMarks_);
}

double MemController::GetNewSpaceConcurrentMarkSpeedPerMS() const
{
    return CalculateAverageSpeed(recordedSemiConcurrentMarks_);
}

double MemController::GetOldSpaceAllocationThroughputPerMS() const
{
    return CalculateAverageSpeed(recordedOldSpaceAllocations_);
}

double MemController::GetFullSpaceConcurrentMarkSpeedPerMS() const
{
    return CalculateAverageSpeed(recordedConcurrentMarks_);
}

double MemController::GetIdleNewSpaceAllocationThroughputPerMS() const
{
    return CalculateAverageSpeed(recordedIdleNewSpaceAllocations_);
}

double MemController::GetIdleOldSpaceAllocationThroughputPerMS() const
{
    return CalculateAverageSpeed(recordedIdleOldSpaceAllocations_);
}

}  // namespace panda::ecmascript
