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

#ifndef ECMASCRIPT_MEM_CMS_MEM_SLOT_FREE_LIST_INL_H
#define ECMASCRIPT_MEM_CMS_MEM_SLOT_FREE_LIST_INL_H

#include "ecmascript/mem/cms_mem/slot_free_list.h"

namespace panda::ecmascript {

void SlotFreeList::Initialize(size_t slotSize)
{
    ASSERT(slotSize_ == 0);
    ASSERT(slotSize % SlotSpaceConfig::SLOT_STEP_SIZE == 0);
    slotSize_ = slotSize;
}

bool SlotFreeList::IsSoldOut() const
{
    return top_ >= end_ && !HasNextSegment();
}

void SlotFreeList::Reset(SlotFreeListMetaInfo slotFreeListMetaInfo)
{
    ASSERT(slotFreeListMetaInfo.totalUsableSize_ > 0);
    ASSERT(slotFreeListMetaInfo.totalUsableSize_ % slotSize_ == 0);
    totalUsableSize_ = slotFreeListMetaInfo.totalUsableSize_;
    FetchAndUpdateSegment(slotFreeListMetaInfo.firstFreeSegment_);
}

size_t SlotFreeList::Discard()
{
    size_t allocatedSize = totalUsableSize_;
    if (top_ < end_) {
        size_t size = end_ - top_;
        allocatedSize -= size;
        SlotFreeSegment::FillFreeSegment(top_, size);
    }
    {
        SlotFreeSegment *unused = SlotFreeSegment::Cast(nextTop_);
        while (unused != nullptr) {
            allocatedSize -= unused->GetSize();
            unused = unused->GetNextFreeSegment();
        }
    }
    top_ = 0;
    end_ = 0;
    nextTop_ = 0;
    totalUsableSize_ = 0;
    return allocatedSize;
}

uintptr_t SlotFreeList::TryAllocate()
{
    if (LIKELY(top_ < end_)) {
        uintptr_t result = top_;
        top_ += slotSize_;
        return result;
    }

    if (UNLIKELY(!HasNextSegment())) {
        return 0;
    }

    FetchAndUpdateSegment(reinterpret_cast<SlotFreeSegment *>(nextTop_));
    uintptr_t result = top_;
    top_ += slotSize_;
    return result;
}

bool SlotFreeList::HasNextSegment() const
{
    return nextTop_ != 0;
}

void SlotFreeList::FetchAndUpdateSegment(SlotFreeSegment *freeSegment)
{
    size_t size = freeSegment->GetSize();
    ASSERT(size >= slotSize_);
    ASSERT(size % slotSize_ == 0);

    top_ = reinterpret_cast<uintptr_t>(freeSegment);
    end_ = top_ + size;
    ASSERT(top_ < end_);

    SlotFreeSegment *nextFreeSegment = freeSegment->GetNextFreeSegment();
    nextTop_ = reinterpret_cast<uintptr_t>(nextFreeSegment);
    if (LIKELY(nextFreeSegment != nullptr)) {
        [[maybe_unused]] size_t nextSize = nextFreeSegment->GetSize();
        ASSERT(nextSize >= slotSize_);
        ASSERT(nextSize % slotSize_ == 0);
    }
}

std::pair<uintptr_t, size_t> SlotFreeList::TryAllocateBuffer(size_t bufferSize)
{
    size_t usable = end_ - top_;
    if (UNLIKELY(usable == 0)) {
        return {0, 0};
    }

    size_t allocSize = std::min(bufferSize, usable);
    uintptr_t result = top_;
    top_ += allocSize;
    return {result, allocSize};
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_CMS_MEM_SLOT_FREE_LIST_INL_H
