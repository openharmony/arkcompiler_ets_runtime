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

#ifndef ECMASCRIPT_MEM_CMS_MEM_SLOT_SPACE_INL_H
#define ECMASCRIPT_MEM_CMS_MEM_SLOT_SPACE_INL_H

#include "ecmascript/mem/cms_mem/slot_space.h"

#include "ecmascript/mem/cms_mem/cms_region_chain_manager-inl.h"
#include "ecmascript/mem/cms_mem/slot_allocator-inl.h"

namespace panda::ecmascript {

size_t SlotSpace::GetSlotIdxBySize(size_t size)
{
    ASSERT(size <= SlotSpaceConfig::MAX_REGULAR_HEAP_OBJECT_SLOT_SIZE);
    ASSERT(size % SlotSpaceConfig::SLOT_STEP_SIZE == 0);
    size_t idx = size / SlotSpaceConfig::SLOT_STEP_SIZE;
    return idx;
}

size_t SlotSpace::GetSlotSizeByIdx(size_t idx)
{
    ASSERT(idx < SlotSpaceConfig::NUM_SLOTS);
    return SLOT_SIZES[idx];
}

void SlotSpace::ReclaimRegions(size_t cachedSize)
{
    ASSERT(cachedSize >= 0);
    EnumerateRegions([this, &cachedSize](Region *region) {
        ClearAndFreeRegion(region, cachedSize);
    });
    committedSize_ = 0;
}

template <bool allowGC>
ARK_INLINE uintptr_t SlotSpace::Allocate(size_t size)
{
    SlotAllocator *allocator = &allocators_[GetSlotIdxBySize(size)];

    uintptr_t result = TryAllocateFromAllocator<allowGC>(allocator);
    if (LIKELY(result > 0)) {
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
        InvokeAllocationInspector(result, size, size);
#endif
        return result;
    }

    return TryExpandAndAllocate<allowGC>(allocator);
}

template <bool allowGC>
ARK_INLINE uintptr_t SlotSpace::TryAllocateFromAllocator(SlotAllocator *allocator)
{
    uintptr_t result = allocator->TryAllocate([this](size_t allocatedSize) {
        allocateAfterLastGC_ += allocatedSize;
        if constexpr (allowGC) {
            TryTriggerGCIfNeed();
        }
    });

    return result;
}

template <bool allowGC>
ARK_NOINLINE uintptr_t SlotSpace::TryExpandAndAllocate(SlotAllocator *allocator)
{
    ASSERT(TryAllocateFromAllocator<false>(allocator) == 0);

    // If is necessary to do GC for reduce memory, try triggerGC and try allocate without expand.
    if constexpr (allowGC) {
        TryTriggerGCIfNeed();
        uintptr_t result = TryAllocateFromAllocator<allowGC>(allocator);
        if (LIKELY(result > 0)) {
            return result;
        }
    }

    if constexpr (allowGC) {
        if (UNLIKELY(!TryExpandAllocator(allocator, MemoryCheckerKind::HARD))) {
            // Request the first GC.
            RequestGC();
        }
        uintptr_t result = TryAllocateFromAllocator<allowGC>(allocator);
        if (LIKELY(result > 0)) {
            return result;
        }

        if (UNLIKELY(!TryExpandAllocator(allocator, MemoryCheckerKind::HARD))) {
            // Request second GC and try allocate, because some floating garbage may not free
            // during the first GC due to the concurrent marking.
            RequestGC();
        }
        result = TryAllocateFromAllocator<allowGC>(allocator);
        if (LIKELY(result > 0)) {
            return result;
        }
    }

    if (UNLIKELY(!TryExpandAllocator(allocator, MemoryCheckerKind::SOFT))) {
        return 0;
    }

    uintptr_t result = TryAllocateFromAllocator<allowGC>(allocator);
    ASSERT(result > 0);
    return result;
}

ARK_INLINE void SlotSpace::InvokeAllocationInspector(Address object, size_t size, size_t alignedSize)
{
    ASSERT(size <= alignedSize);
    if (LIKELY(!allocationCounter_.IsActive())) {
        return;
    }
    if (alignedSize >= allocationCounter_.NextBytes()) {
        allocationCounter_.InvokeAllocationInspector(object, size, alignedSize);
    }
    allocationCounter_.AdvanceAllocationInspector(alignedSize);
}

std::pair<uintptr_t, size_t> SlotSpace::AllocateBufferSyncByIdx(size_t idx)
{
    size_t slotSize = GetSlotSizeByIdx(idx);
    ASSERT(slotSize > 0);
    size_t maxBufferSize = (SlotSpaceConfig::MAX_GC_TLAB_BUFFER_SIZE + slotSize - 1) / slotSize * slotSize;
    SlotAllocator *allocator = &allocators_[idx];

    // fixme: optimize?
    {
        // fixme: lock could be slot-wised
        LockHolder holder(lock_);
        {
            auto result = allocator->TryAllocateBuffer(maxBufferSize);
            if (LIKELY(result.first > 0)) {
                return result;
            }
        }

        ExpandAllocator(allocator);
    
        {
            auto result = allocator->TryAllocateBuffer(maxBufferSize);
            ASSERT(result.first > 0);
            return result;
        }
    }
}

uint32_t SlotSpace::GetRegionCount() const
{
    // fixme: optimize, do not iterate
    uint32_t cnt = 0;
    for (const CMSRegionChainManager &regionChainManager : regionChainManagers_) {
        cnt += regionChainManager.GetRegionCount();
    }
    return cnt;
}

template <typename Callback>
void SlotSpace::EnumerateRegions(Callback &&cb) const
{
    for (const CMSRegionChainManager &regionChainManager : regionChainManagers_) {
        regionChainManager.EnumerateRegions(cb);
    }
}

template <typename Callback>
void SlotSpace::EnumerateRegionsWithRecord(Callback &&cb) const
{
    for (const CMSRegionChainManager &regionChainManager : regionChainManagers_) {
        regionChainManager.EnumerateRegionsWithRecord(cb);
    }
}

void SlotSpace::SetRecordRegion()
{
    for (CMSRegionChainManager &regionChainManager : regionChainManagers_) {
        regionChainManager.SetRecordRegion();
    }
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_CMS_MEM_SLOT_SPACE_INL_H
