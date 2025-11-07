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
    ASSERT(slotFreeListMetaInfo.firstFreeObject_->IsFreeObject());
    ASSERT(slotFreeListMetaInfo.totalUsableSize_ > 0 && slotFreeListMetaInfo.totalUsableSize_ % slotSize_ == 0);
    totalUsableSize_ = slotFreeListMetaInfo.totalUsableSize_;
    FetchAndUpdateSegment(slotFreeListMetaInfo.firstFreeObject_);
}

template <typename DiscardCurrentSegmentRemainMemory>
size_t SlotFreeList::Discard(DiscardCurrentSegmentRemainMemory &&discardCurrentSegmentRemainMemory)
{
    size_t allocatedSize = totalUsableSize_;
    if (top_ < end_) {
        size_t size = end_ - top_;
        allocatedSize -= size;
        discardCurrentSegmentRemainMemory(top_, size);
    }
    {
        FreeObject *unused = FreeObject::Cast(nextTop_);
        while (unused != INVALID_OBJECT) {
            allocatedSize -= unused->Available();
            unused = unused->GetNext();
        }
    }
    top_ = 0;
    end_ = 0;
    nextTop_ = static_cast<uintptr_t>(JSTaggedValue::NULL_POINTER);
    nextEnd_ = static_cast<uintptr_t>(JSTaggedValue::NULL_POINTER);
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

    FetchAndUpdateSegment(FreeObject::Cast(nextTop_));
    uintptr_t result = top_;
    top_ += slotSize_;
    return result;
}

bool SlotFreeList::HasNextSegment() const
{
    return (nextTop_ & INVALID_OBJECT_FAST_CHECK_MASK) == 0;
}

void SlotFreeList::FetchAndUpdateSegment(FreeObject *freeObject)
{
    size_t size = freeObject->Available();
    ASSERT(size >= slotSize_ && size % slotSize_ == 0);

    top_ = reinterpret_cast<uintptr_t>(freeObject);
    end_ = top_ + size;
    ASSERT(FreeObject::Cast(top_)->IsFreeObject() && top_ < end_);

    FreeObject *nextFreeObject = freeObject->GetNext();
    nextTop_ = reinterpret_cast<uintptr_t>(nextFreeObject);
    // fixme: store next freeobject size in current freeobject?
    if (LIKELY(nextFreeObject != INVALID_OBJECT)) {
        size_t nextSize = nextFreeObject->Available();
        ASSERT(nextSize >= slotSize_ && nextSize % slotSize_ == 0);
        nextEnd_ = nextTop_ + nextSize;
    } else {
        nextEnd_ = nextTop_;
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
