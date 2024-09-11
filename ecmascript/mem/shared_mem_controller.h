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

#ifndef ECMASCRIPT_MEM_SHARED_MEM_CONTROLLER_H
#define ECMASCRIPT_MEM_SHARED_MEM_CONTROLLER_H

#include <chrono>
#include <cmath>
#include <limits>

#include "ecmascript/base/gc_ring_buffer.h"
#include "ecmascript/mem/mem_controller_utils.h"
#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
class SharedHeap;

using Clock = std::chrono::high_resolution_clock;
using BytesAndDuration = std::pair<uint64_t, double>;

class SharedMemController {
public:
    explicit SharedMemController(const SharedHeap* sheap):sheap_(sheap){};
    SharedMemController() = default;
    ~SharedMemController() = default;
    NO_COPY_SEMANTIC(SharedMemController);
    NO_MOVE_SEMANTIC(SharedMemController);

    void RecordAllocationForIdle();
    bool CheckLowAllocationUsageState() const;
    void UpdateAllocationAfterGC();

    double GetIdleSharedSpaceAllocationThroughputPerMS() const
    {
        return MemControllerUtils::CalculateAverageSpeed(recordedSharedSpaceAllocations_);
    }

    double GetLastGCOldSpaceObjectUsageRate() const
    {
        return lastGCOldSpaceObjectUsageRate_;
    }

    double GetLastGCObjectUsageRate() const
    {
        return lastGCObjectUsageRate_;
    }

private:
    const SharedHeap *sheap_ {nullptr};

    double allocTimeMsSharedIdle_ {0.0};
    size_t sharedSpaceAllocSizeSinceIdle_ {0};
    double lastGCObjectUsageRate_ {1.0f};
    double lastGCOldSpaceObjectUsageRate_ {1.0f};
    base::GCRingBuffer<BytesAndDuration, MemControllerUtils::LENGTH> recordedSharedSpaceAllocations_;
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_MEM_CONTROLLER_H
