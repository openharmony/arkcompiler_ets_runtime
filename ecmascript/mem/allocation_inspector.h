/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_ALLOCATION_INSPECTOR_H
#define ECMASCRIPT_MEM_ALLOCATION_INSPECTOR_H

#include "ecmascript/common.h"
#include "libpandabase/macros.h"

namespace panda::ecmascript {
class Heap;
class HeapSampling;

class AllocationInspector {
public:
    explicit AllocationInspector(Heap *heap, uint64_t rate, HeapSampling *profiler)
        : rate_(rate),
          profiler_(profiler),
          heap_(heap) {}
    ~AllocationInspector() = default;
    NO_MOVE_SEMANTIC(AllocationInspector);
    NO_COPY_SEMANTIC(AllocationInspector);

    // When total at least *rate_* bytes have been allocated, this func can be called.
    // *object* is an address pointing to allocated and uninitialized memory.
    // *size* is the dimension of allocation corresponding to `object`.
    void Step([[maybe_unused]] Address object, [[maybe_unused]] size_t size);
    size_t GetNextStepSize();

private:
    const uint64_t rate_;
    [[maybe_unused]] HeapSampling *const profiler_;
    [[maybe_unused]] Heap *const heap_;
};

// each space has an allocationCounter which inspector can be added to.
class AllocationCounter final {
public:
    AllocationCounter() = default;
    ~AllocationCounter() = default;

    void AddAllocationInspector(AllocationInspector *inspector);
    void ClearAllocationInspector();
    void AdvanceAllocationInspector(size_t allocated);
    void InvokeAllocationInspector(Address object, size_t objectSize, size_t alignedObjectSize);

    bool IsActive() const
    {
        return inspector_ != nullptr;
    }

    size_t NextBytes() const
    {
        return nextCounter_ - currentCounter_;
    }

private:
    AllocationInspector *inspector_ = nullptr;
    size_t currentCounter_ = 0;
    size_t nextCounter_ = 0;
};
} // namespace panda::ecmascript

#endif