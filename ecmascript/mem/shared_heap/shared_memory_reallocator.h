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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_MEMORY_REALLOCATOR_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_MEMORY_REALLOCATOR_H

#include "ecmascript/mem/mem_common.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
class SharedOldSpace;
class SharedHugeObjectSpace;

/**
 * Memory reallocation helper for transferring capacity between SharedOldSpace and SharedHugeObjectSpace
 *
 * This is designed to be used as a member of SharedHeap.
 * It reuses SharedHugeObjectSpace's allocateLock_ to ensure thread safety with allocation.
 */
class SharedMemoryReallocator {
public:
    static constexpr size_t HUGE_SPACE_FREE_THRESHOLD = 100_MB;  // 100MB
    static constexpr double TRANSFER_RATIO = 0.5;  // Transfer half of excess

    SharedMemoryReallocator() = default;
    ~SharedMemoryReallocator() = default;

    NO_COPY_SEMANTIC(SharedMemoryReallocator);
    NO_MOVE_SEMANTIC(SharedMemoryReallocator);

    // Try to transfer memory from HugeObjectSpace to OldSpace
    bool TryTransferMemoryToOldSpace(SharedOldSpace *oldSpace, SharedOldSpace *compressSpace,
                                     SharedHugeObjectSpace *hugeSpace);

private:
    size_t CalculateTransferSize(SharedHugeObjectSpace *hugeSpace);
    void TransferCapacityLimits(SharedOldSpace *oldSpace, SharedOldSpace *compressSpace,
                                SharedHugeObjectSpace *hugeSpace, size_t transferSize);
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_MEMORY_REALLOCATOR_H
