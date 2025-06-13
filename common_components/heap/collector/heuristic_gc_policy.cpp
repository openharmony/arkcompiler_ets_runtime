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
            collector.RequestGC(GC_REASON_YOUNG, true);
        } else {
            DLOG(ALLOC, "request heu gc: allocated %zu, threshold %zu", allocated, threshold);
            collector.RequestGC(GC_REASON_HEU, true);
        }
    }
}
} // namespace common
