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

#ifndef COMMON_COMPONENTS_HEAP_ALLOC_BUFFER_H
#define COMMON_COMPONENTS_HEAP_ALLOC_BUFFER_H

#include <functional>

#include "common_components/mutator/thread_local.h"
#include "common_components/heap/allocator/region_list.h"
#include "common_components/common/mark_work_stack.h"

namespace common {

enum class AllocBufferType: uint8_t {
    YOUNG = 0, // for young space
    OLD,       // for old space
};

// thread-local data structure
class AllocationBuffer {
public:
    AllocationBuffer() : tlRawPointerRegions_("thread-local raw-pointer regions") {}
    ~AllocationBuffer();
    void Init();
    static AllocationBuffer* GetOrCreateAllocBuffer();
    static AllocationBuffer* GetAllocBuffer();
    HeapAddress ToSpaceAllocate(size_t size, AllocType allocType);
    HeapAddress Allocate(size_t size, AllocType allocType);

    template<AllocBufferType type = AllocBufferType::YOUNG>
    RegionDesc* GetRegion()
    {
        if constexpr (type == AllocBufferType::YOUNG) {
            return tlRegion_;
        } else if constexpr (type == AllocBufferType::OLD) {
            return tlOldRegion_;
        }
    }

    template <AllocBufferType type = AllocBufferType::YOUNG>
    void SetRegion(RegionDesc* newRegion)
    {
        if constexpr (type == AllocBufferType::YOUNG) {
            tlRegion_ = newRegion;
        } else if constexpr (type == AllocBufferType::OLD) {
            tlOldRegion_ = newRegion;
        }
    }

    RegionDesc* GetPreparedRegion() { return preparedRegion_.load(std::memory_order_acquire); }
    inline void ClearRegion()
    {
        if (tlRegion_ == RegionDesc::NullRegion()) {
            return;
        }
        DLOG(REGION, "AllocBuffer clear tlRegion %p@%#zx+%zu",
            tlRegion_, tlRegion_->GetRegionStart(), tlRegion_->GetRegionAllocatedSize());
        tlRegion_ = RegionDesc::NullRegion();
    }
    void ClearThreadLocalRegion();

    bool SetPreparedRegion(RegionDesc* newPreparedRegion)
    {
        RegionDesc* expect = nullptr;
        return preparedRegion_.compare_exchange_strong(expect, newPreparedRegion, std::memory_order_release,
            std::memory_order_relaxed);
    }

    // record stack roots in allocBuffer so that mutator can concurrently enumerate roots without lock.
    void PushRoot(BaseObject* root) { stackRoots_.emplace_back(root); }

    // record stack roots in allocBuffer so that mutator can concurrently enumerate roots without lock.
    void PushRoot(uint64_t* root) { taggedObjStackRoots_.emplace_back(root); }

    // move the stack roots to other container so that other threads can visit them.
    template <class Functor>
    inline void MarkStack(Functor consumer)
    {
        if (taggedObjStackRoots_.empty()) {
            return;
        }
        for (uint64_t* obj : taggedObjStackRoots_) {
            consumer(reinterpret_cast<BaseObject*>(obj));
        }
        stackRoots_.clear();
    }

    static constexpr size_t GetTLRegionOffset()
    {
        return offsetof(AllocationBuffer, tlRegion_);
    }

private:
    // slow path
    HeapAddress TryAllocateOnce(size_t totalSize, AllocType allocType);
    HeapAddress AllocateImpl(size_t totalSize, AllocType allocType);
    HeapAddress AllocateRawPointerObject(size_t totalSize);

    // tlRegion in AllocBuffer is a shortcut for fast allocation.
    // we should handle failure in RegionManager
    RegionDesc* tlRegion_ = RegionDesc::NullRegion();
    RegionDesc* tlOldRegion_ = RegionDesc::NullRegion();

    std::atomic<RegionDesc*> preparedRegion_ = { nullptr };
    // allocate objects which are exposed to runtime thus can not be moved.
    // allocation context is responsible to notify collector when these objects are safe to be collected.
    RegionList tlRawPointerRegions_;

    // Record stack roots in concurrent enum phase, waiting for GC to merge these roots
    std::list<BaseObject*> stackRoots_;

    std::list<uint64_t*> taggedObjStackRoots_;
};

static_assert(AllocationBuffer::GetTLRegionOffset() == 0);
} // namespace common

#endif // COMMON_COMPONENTS_HEAP_ALLOC_BUFFER_H
