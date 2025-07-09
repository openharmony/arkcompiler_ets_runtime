/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "common_components/heap/collector/heuristic_gc_policy.h"

#include "common_components/heap/allocator/allocator.h"
#include "common_components/heap/heap.h"
#include "common_interfaces/base_runtime.h"

namespace common {
std::atomic<StartupStatus> StartupStatusManager::startupStatus_ = StartupStatus::BEFORE_STARTUP;

void StartupStatusManager::OnAppStartup()
{
    startupStatus_ = StartupStatus::COLD_STARTUP;
    Taskpool *threadPool = common::Taskpool::GetCurrentTaskpool();
    threadPool->PostDelayedTask(
        std::make_unique<StartupTask>(0, threadPool, STARTUP_DURATION_MS), STARTUP_DURATION_MS);
}

void HeuristicGCPolicy::Init()
{
    HeapParam &heapParam = BaseRuntime::GetInstance()->GetHeapParam();
    heapSize_ = heapParam.heapSize * KB;
}

bool HeuristicGCPolicy::ShouldRestrainGCOnStartup()
{
    StartupStatus currentStatus = StartupStatusManager::GetStartupStatus();
    if (UNLIKELY_CC(currentStatus == StartupStatus::COLD_STARTUP)) {
        return true;
    }
    size_t allocated = Heap::GetHeap().GetAllocator().GetAllocatedBytes();
    if (currentStatus == StartupStatus::COLD_STARTUP_PARTIALLY_FINISH &&
        allocated < heapSize_ * COLD_STARTUP_GC_THRESHOLD_RATIO) {
        return true;
    }
    return false;
}

void HeuristicGCPolicy::TryHeuristicGC()
{
    if (UNLIKELY_CC(ShouldRestrainGCOnStartup())) {
        return;
    }
    Collector& collector = Heap::GetHeap().GetCollector();
    size_t threshold = collector.GetGCStats().GetThreshold();
    size_t allocated = Heap::GetHeap().GetAllocator().GetAllocatedBytes();
    if (allocated >= threshold) {
        if (collector.GetGCStats().shouldRequestYoung) {
            DLOG(ALLOC, "request heu gc: young %zu, threshold %zu", allocated, threshold);
            collector.RequestGC(GC_REASON_YOUNG, true, GC_TYPE_YOUNG);
        } else {
            DLOG(ALLOC, "request heu gc: allocated %zu, threshold %zu", allocated, threshold);
            collector.RequestGC(GC_REASON_HEU, true, GC_TYPE_FULL);
        }
    }
}

void HeuristicGCPolicy::NotifyNativeAllocation(size_t bytes)
{
    notifiedNativeSize_.fetch_add(bytes, std::memory_order_relaxed);
    size_t currentObjects = nativeHeapObjects_.fetch_add(1, std::memory_order_relaxed);
    if (currentObjects % NOTIFY_NATIVE_INTERVAL == NOTIFY_NATIVE_INTERVAL - 1
        || bytes > NATIVE_IMMEDIATE_THRESHOLD) {
        CheckGCForNative();
    }
}
void HeuristicGCPolicy::CheckGCForNative()
{
    size_t currentNativeSize = notifiedNativeSize_.load(std::memory_order_relaxed);
    size_t currentThreshold = nativeHeapThreshold_.load(std::memory_order_relaxed);
    if (currentNativeSize > currentThreshold) {
        if (currentNativeSize > URGENCY_NATIVE_LIMIT) {
            // Native binding size is too large, should wait a sync finished.
            Heap::GetHeap().GetCollector().RequestGC(GC_REASON_NATIVE_SYNC, false, GC_TYPE_FULL);
            return;
        }
        Heap::GetHeap().GetCollector().RequestGC(GC_REASON_NATIVE, true, GC_TYPE_FULL);
    }
}
void HeuristicGCPolicy::NotifyNativeFree(size_t bytes)
{
    size_t allocated;
    size_t newFreedBytes;
    do {
        allocated = notifiedNativeSize_.load(std::memory_order_relaxed);
        newFreedBytes = std::min(allocated, bytes);
        // We should not be registering more free than allocated bytes.
        // But correctly keep going in non-debug builds.
        ASSERT(newFreedBytes == bytes);
    } while (!notifiedNativeSize_.compare_exchange_weak(allocated, allocated - newFreedBytes,
                                                        std::memory_order_relaxed));
}

void HeuristicGCPolicy::NotifyNativeReset(size_t oldBytes, size_t newBytes)
{
    NotifyNativeFree(oldBytes);
    NotifyNativeAllocation(newBytes);
}

size_t HeuristicGCPolicy::GetNotifiedNativeSize() const
{
    return notifiedNativeSize_.load(std::memory_order_relaxed);
}

void HeuristicGCPolicy::SetNativeHeapThreshold(size_t newThreshold)
{
    nativeHeapThreshold_.store(newThreshold, std::memory_order_relaxed);
}

size_t HeuristicGCPolicy::GetNativeHeapThreshold() const
{
    return nativeHeapThreshold_.load(std::memory_order_relaxed);
}

void HeuristicGCPolicy::RecordAliveSizeAfterLastGC(size_t aliveBytes)
{
    aliveSizeAfterGC_ = aliveBytes;
}

void HeuristicGCPolicy::ChangeGCParams(bool isBackground)
{
    if (isBackground) {
        size_t allocated = Heap::GetHeap().GetAllocator().GetAllocatedBytes();
        if (allocated > aliveSizeAfterGC_ && (allocated - aliveSizeAfterGC_) > BACKGROUND_LIMIT &&
            allocated > MIN_BACKGROUND_GC_SIZE) {
            Heap::GetHeap().GetCollector().RequestGC(GC_REASON_BACKGROUND, true, GC_TYPE_FULL);
        }
        common::Taskpool::GetCurrentTaskpool()->SetThreadPriority(common::PriorityMode::BACKGROUND);
    } else {
        common::Taskpool::GetCurrentTaskpool()->SetThreadPriority(common::PriorityMode::FOREGROUND);
    }
}

bool HeuristicGCPolicy::CheckAndTriggerHintGC(MemoryReduceDegree degree)
{
    if (UNLIKELY_CC(ShouldRestrainGCOnStartup())) {
        return false;
    }
    size_t allocated = Heap::GetHeap().GetAllocator().GetAllocatedBytes();

    size_t stepAfterLastGC = 0;
    if (degree == MemoryReduceDegree::LOW) {
        stepAfterLastGC = LOW_DEGREE_STEP_IN_IDLE;
    } else {
        stepAfterLastGC = HIGH_DEGREE_STEP_IN_IDLE;
    }
    if (aliveSizeAfterGC_ == 0) {
        return false;
    }
    size_t expectHeapSize = std::max(static_cast<size_t>(aliveSizeAfterGC_ * IDLE_MIN_INC_RATIO),
        aliveSizeAfterGC_ + stepAfterLastGC);
    if (expectHeapSize < allocated) {
        DLOG(ALLOC, "request heu gc by hint: allocated %zu, expectHeapSize %zu, aliveSizeAfterGC %zu",
             allocated, expectHeapSize, aliveSizeAfterGC_);
        Heap::GetHeap().GetCollector().RequestGC(GC_REASON_HINT, true, GC_TYPE_FULL);
        return true;
    }
    return false;
}
} // namespace common
