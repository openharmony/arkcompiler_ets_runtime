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

#ifndef ECMASCRIPT_MEM_CMS_MEM_SLOT_GC_ALLOCATOR_INL_H
#define ECMASCRIPT_MEM_CMS_MEM_SLOT_GC_ALLOCATOR_INL_H

#include "ecmascript/mem/cms_mem/slot_gc_allocator.h"

#include "ecmascript/mem/cms_mem/slot_allocator-inl.h"
#include "ecmascript/mem/cms_mem/slot_space-inl.h"

namespace panda::ecmascript {

void SlotGCAllocator::Setup(SlotSpace *slotSpace)
{
    ASSERT(slotSpace_ == nullptr && slotSpace != nullptr);
    slotSpace_ = slotSpace;
    for (size_t i = 0; i < SlotSpaceConfig::NUM_SLOTS; ++i) {
        size_t slotSize = SlotSpace::GetSlotSizeByIdx(i);
        tlabAllocators_[i].Setup(slotSize);
    }
}

void SlotGCAllocator::Initialize()
{
    for (SlotGCAllocator::SlotBumpPointerAllocator &bpAllocator : tlabAllocators_) {
        ASSERT(bpAllocator.IsEmpty());
    }
}

uintptr_t SlotGCAllocator::Allocate(size_t size)
{
    size_t idx = SlotSpace::GetSlotIdxBySize(size);

    SlotGCAllocator::SlotBumpPointerAllocator &tlab = tlabAllocators_[idx];
    {
        uintptr_t result = tlab.Allocate();
        if (LIKELY(result > 0)) {
            return result;
        }
    }

    auto [bufferBegin, bufferSize] = slotSpace_->AllocateBufferSyncByIdx(idx);
    tlab.Reset(bufferBegin, bufferBegin + bufferSize);
    uintptr_t result = tlab.Allocate();
    ASSERT(result > 0);
    return result;
}

void SlotGCAllocator::Finalize(BaseHeap *heap)
{
    for (SlotGCAllocator::SlotBumpPointerAllocator &tlab : tlabAllocators_) {
        tlab.Finalize(heap);
        ASSERT(tlab.IsEmpty());
    }
}

void SlotGCAllocator::SlotBumpPointerAllocator::Setup(size_t slotSize)
{
    ASSERT(slotSize_ == 0);
    slotSize_ = slotSize;
}

void SlotGCAllocator::SlotBumpPointerAllocator::Reset(uintptr_t top, uintptr_t end)
{
    ASSERT(IsEmpty());
    ASSERT(end - top > 0 && (end - top) % slotSize_ == 0);
    top_ = top;
    end_ = end;
}

void SlotGCAllocator::SlotBumpPointerAllocator::Finalize(BaseHeap *heap)
{
    if (top_ < end_) {
        size_t left = end_ - top_;
        FreeObject::FillFreeObject(heap, top_, left);
    }
    top_ = 0;
    end_ = 0;
}

uintptr_t SlotGCAllocator::SlotBumpPointerAllocator::Allocate()
{
    if (LIKELY(top_ < end_)) {
        uintptr_t result = top_;
        top_ += slotSize_;
        return result;
    }
    return 0;
}

bool SlotGCAllocator::SlotBumpPointerAllocator::IsEmpty() const
{
    ASSERT(top_ <= end_);
    return top_ == end_;
}

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_CMS_MEM_TLAB_ALLOCATOR_INL_H
