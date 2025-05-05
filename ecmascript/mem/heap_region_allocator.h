/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_HEAP_REGION_ALLOCATOR_H
#define ECMASCRIPT_MEM_HEAP_REGION_ALLOCATOR_H

#include <atomic>

#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
class BaseHeap;
class Region;
class Space;

class HeapRegionAllocator {
public:
    // For `Runtime::heapRegionAllocator_`, since it's for SharedHeap, so do not need enable PageTag threadId.
    HeapRegionAllocator() = default;
    HeapRegionAllocator(JSRuntimeOptions &option);
    virtual ~HeapRegionAllocator() = default;

    Region *AllocateAlignedRegion(Space *space, size_t capacity, JSThread* thread, BaseHeap *heap,
                                  bool isFresh = false);
    void FreeRegion(Region *region, size_t cachedSize, bool skipCache = false);

    void IncreaseAnnoMemoryUsage(size_t bytes)
    {
        size_t current = annoMemoryUsage_.fetch_add(bytes, std::memory_order_relaxed) + bytes;
        size_t max = maxAnnoMemoryUsage_.load(std::memory_order_relaxed);
        while (current > max && !maxAnnoMemoryUsage_.compare_exchange_weak(max, current, std::memory_order_relaxed)) {
        }
    }

    void DecreaseAnnoMemoryUsage(size_t bytes)
    {
        annoMemoryUsage_.fetch_sub(bytes, std::memory_order_relaxed);
    }

    size_t GetAnnoMemoryUsage() const
    {
        return annoMemoryUsage_.load(std::memory_order_relaxed);
    }

    size_t GetMaxAnnoMemoryUsage() const
    {
        return maxAnnoMemoryUsage_.load(std::memory_order_relaxed);
    }

private:
    NO_COPY_SEMANTIC(HeapRegionAllocator);
    NO_MOVE_SEMANTIC(HeapRegionAllocator);

    // Can not throw OOM during GC, so just make MemMapAllocator infinite to make allocating region always
    // success to complete this GC, and then do HeapDump and Fatal.
    // This will temporarily lead that all JSThread could always AllcationRegion success,
    // breaking the global region limit, but thread calling this will soon complete GC and then fatal.
    void TemporarilyEnsureAllocateionAlwaysSuccess(BaseHeap *heap);

    bool AllocateRegionShouldPageTag(Space *space) const;
    bool FreeRegionShouldPageTag(Region *region) const;

    std::atomic<size_t> annoMemoryUsage_ {0};
    std::atomic<size_t> maxAnnoMemoryUsage_ {0};
    bool enablePageTagThreadId_ {false};
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_HEAP_REGION_ALLOCATOR_H
