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

#include "ecmascript/mem/shared_heap/shared_memory_reallocator.h"

#include "ecmascript/mem/mem.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/mem/shared_heap/shared_space.h"

namespace panda::ecmascript {

bool SharedMemoryReallocator::TryTransferMemoryToOldSpace(SharedOldSpace *oldSpace, SharedOldSpace *compressSpace,
                                                          SharedHugeObjectSpace *hugeSpace)
{
    ASSERT(oldSpace != nullptr);
    ASSERT(hugeSpace != nullptr);
    ASSERT(compressSpace != nullptr);
    // Hold HugeObjectSpace's allocateLock_ to ensure thread safety with concurrent allocation
    LockHolder lock(hugeSpace->allocateLock_);

    if (oldSpace->GetMaximumCapacity() > oldSpace->GetCommittedSize() &&
        oldSpace->GetMaximumCapacity() - oldSpace->GetCommittedSize() >= DEFAULT_REGION_SIZE) {
        LOG_ECMA_MEM(INFO) << "Shared old space has enough memory to expand;";
        return false;
    }

    // Calculate how much to transfer
    size_t transferSize = CalculateTransferSize(hugeSpace);
    if (transferSize == 0) {
        LOG_ECMA_MEM(INFO) << "SharedMemoryReallocator: No memory to transfer";
        return false;
    }

    // Transfer capacity limits
    TransferCapacityLimits(oldSpace, compressSpace, hugeSpace, transferSize);
    return true;
}

size_t SharedMemoryReallocator::CalculateTransferSize(SharedHugeObjectSpace *hugeSpace)
{
    size_t committedSize = hugeSpace->GetCommittedSize();
    size_t maximumCapacity = hugeSpace->GetMaximumCapacity();

    size_t freeSize = (committedSize <= maximumCapacity) ? maximumCapacity - committedSize : 0;
    if (freeSize > HUGE_SPACE_FREE_THRESHOLD) {
        // Huge space remain 100MB at least
        size_t transferSize =
            std::min(static_cast<size_t>(freeSize * TRANSFER_RATIO), freeSize - HUGE_SPACE_FREE_THRESHOLD);
        return AlignUp(transferSize, DEFAULT_REGION_SIZE);
    }
    return 0;
}

void SharedMemoryReallocator::TransferCapacityLimits(SharedOldSpace *oldSpace,
                                                     SharedOldSpace *compressSpace,
                                                     SharedHugeObjectSpace *hugeSpace,
                                                     size_t transferSize)
{
    // Increase OldSpace capacity
    size_t oldMaxCapacity = oldSpace->GetMaximumCapacity();
    size_t compressMaxCapacity = compressSpace->GetMaximumCapacity();
    oldSpace->SetMaximumCapacity(oldMaxCapacity + transferSize);
    oldSpace->SetInitialCapacity(oldMaxCapacity + transferSize);
    compressSpace->SetMaximumCapacity(compressMaxCapacity + transferSize);
    compressSpace->SetInitialCapacity(compressMaxCapacity + transferSize);

    // Decrease HugeObjectSpace capacity
    size_t hugeMaxCapacity = hugeSpace->GetMaximumCapacity();
    ASSERT(hugeMaxCapacity >= transferSize);
    hugeSpace->SetMaximumCapacity(hugeMaxCapacity - transferSize);
    hugeSpace->SetInitialCapacity(hugeMaxCapacity - transferSize);

    LOG_ECMA_MEM(INFO) << "SharedMemoryReallocator: Capacity transfer completed"
                       << " OldSpace: " << oldMaxCapacity << " -> " << oldSpace->GetMaximumCapacity()
                       << " HugeObjectSpace: " << hugeMaxCapacity << " -> " << hugeSpace->GetMaximumCapacity();
}
}  // namespace panda::ecmascript
