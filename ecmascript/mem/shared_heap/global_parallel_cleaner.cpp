/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/shared_heap/global_parallel_cleaner.h"

#include "common_components/taskpool/taskpool.h"
#include "ecmascript/free_object.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/runtime.h"

namespace panda::ecmascript {

void GlobalParallelCleaner::Clean()
{
    CollectRegions();
    if (oldRegions_.empty() && youngRegions_.empty()) {
        return;
    }
    if (sHeap_->IsParallelGCEnabled()) {
        ParallelCleanRegions();
    } else {
        SequentialCleanRegions();
    }
}

void GlobalParallelCleaner::CollectRegions()
{
    oldRegions_.clear();
    youngRegions_.clear();
    regionIdx_ = 0;
    Runtime::GetInstance()->GCIterateThreadList([&](JSThread *iteratedThread) {
        Heap *heap = iteratedThread->GetEcmaVM()->GetHeap();
        heap->EnumerateNonNewSpaceRegions([&](Region *region) {
            oldRegions_.push_back(region);
        });
        SemiSpace *newSpace = heap->GetNewSpace();
        Region *current = newSpace->GetCurrentRegion();
        uintptr_t top = newSpace->GetTop();
        newSpace->EnumerateRegions([&](Region *region) {
            uintptr_t endAddr = (region == current)
                ? region->GetBegin() + region->GetAllocatedBytes(top)
                : region->GetBegin() + region->GetAllocatedBytes();
            youngRegions_.push_back({region, endAddr});
        });
    });
}

void GlobalParallelCleaner::ParallelCleanRegions()
{
    uint32_t taskCount = std::min(static_cast<uint32_t>(oldRegions_.size() + youngRegions_.size()),
                                  common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum());
    common::TaskPackMonitor monitor(taskCount, taskCount);
    for (uint32_t i = 0; i < taskCount; i++) {
        common::Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<CleanerTask>(common::GLOBAL_TASK_ID, this, &monitor));
    }
    monitor.WaitAllFinished();
}

void GlobalParallelCleaner::ProcessRegions(common::TaskPackMonitor *monitor)
{
    size_t oldSize = oldRegions_.size();
    size_t youngSize = youngRegions_.size();
    size_t idx = regionIdx_.fetch_add(1, std::memory_order_relaxed);
    while (idx < oldSize) {
        ClearDeadRSetForRegion(oldRegions_[idx], oldRegions_[idx]->GetEnd());
        idx = regionIdx_.fetch_add(1, std::memory_order_relaxed);
    }
    while (idx < oldSize + youngSize) {
        auto [region, endAddr] = youngRegions_[idx - oldSize];
        ClearDeadRSetForRegion(region, endAddr);
        idx = regionIdx_.fetch_add(1, std::memory_order_relaxed);
    }
    monitor->NotifyFinishOne();
}

void GlobalParallelCleaner::SequentialCleanRegions()
{
    for (auto *region : oldRegions_) {
        ClearDeadRSetForRegion(region, region->GetEnd());
    }
    for (auto [region, endAddr] : youngRegions_) {
        ClearDeadRSetForRegion(region, endAddr);
    }
}

void GlobalParallelCleaner::ClearDeadRSetForRegion(Region *region, uintptr_t endAddr)
{
    uintptr_t freeStart = region->GetBegin();
    region->IterateAllMarkedBits([&freeStart, region, this](void *mem) {
        auto header = reinterpret_cast<TaggedObject *>(mem);
        size_t size = header->GetSize();
        uintptr_t freeEnd = ToUintPtr(mem);
        if (freeStart != freeEnd) {
            region->ClearLocalToShareRSetInRange(freeStart, freeEnd);
            region->ClearOldToNewRSetInRange(freeStart, freeEnd);
            FreeObject::FillFreeObject(sHeap_, freeStart, freeEnd - freeStart);
        }
        freeStart = freeEnd + size;
    });
    if (freeStart < endAddr) {
        region->ClearLocalToShareRSetInRange(freeStart, endAddr);
        region->ClearOldToNewRSetInRange(freeStart, endAddr);
        FreeObject::FillFreeObject(sHeap_, freeStart, endAddr - freeStart);
    }
}

bool GlobalParallelCleaner::CleanerTask::Run(uint32_t threadIndex)
{
    cleaner_->ProcessRegions(monitor_);
    return true;
}

}  // namespace panda::ecmascript
