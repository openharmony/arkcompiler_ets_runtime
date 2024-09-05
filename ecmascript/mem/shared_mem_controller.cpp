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

#include "ecmascript/mem/heap-inl.h"

namespace panda::ecmascript {

void SharedMemController::RecordAllocationForIdle()
{
    double currentTimeInMs = MemControllerUtils::GetSystemTimeInMs();
    size_t currentSize = sheap_->GetHeapObjectSize();
    if (sharedSpaceAllocSizeSinceIdle_ == 0 || currentSize < sharedSpaceAllocSizeSinceIdle_) {
        sharedSpaceAllocSizeSinceIdle_ = currentSize;
        allocTimeMsSharedIdle_ = currentTimeInMs;
        return;
    }
    recordedSharedSpaceAllocations_.Push(std::make_pair(currentSize - sharedSpaceAllocSizeSinceIdle_,
        currentTimeInMs - allocTimeMsSharedIdle_));
    sharedSpaceAllocSizeSinceIdle_ = currentSize;
    allocTimeMsSharedIdle_ = currentTimeInMs;
}

bool SharedMemController::CheckLowAllocationUsageState() const
{
    return GetIdleSharedSpaceAllocationThroughputPerMS() < LOW_ALLOCATION_RATE_PER_MS;
}

void SharedMemController::UpdateAllocationAfterGC()
{
    sharedSpaceAllocSizeSinceIdle_ = sheap_->GetHeapObjectSize();
    allocTimeMsSharedIdle_ = MemControllerUtils::GetSystemTimeInMs();
}

}  // namespace panda::ecmascript
