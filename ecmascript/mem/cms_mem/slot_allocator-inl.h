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

#ifndef ECMASCRIPT_MEM_CMS_MEM_SLOT_ALLOCATOR_INL_H
#define ECMASCRIPT_MEM_CMS_MEM_SLOT_ALLOCATOR_INL_H

#include "ecmascript/mem/cms_mem/slot_allocator.h"

#include "ecmascript/mem/cms_mem/cms_region_chain_manager-inl.h"
#include "ecmascript/mem/cms_mem/slot_free_list-inl.h"

namespace panda::ecmascript {

template <typename FreeListSoldOutCallback>
ARK_INLINE uintptr_t SlotAllocator::TryAllocate(FreeListSoldOutCallback &&freeListSoldOutCallback)
{
    uintptr_t result = freeList_.TryAllocate();
    if (LIKELY(result > 0)) {
        return result;
    }

    size_t allocatedSize = RecordFreeListSoldOut();
    freeListSoldOutCallback(allocatedSize);

    SlotFreeListMetaInfo slotFreeListMetaInfo = regionChainManager_->TryTakeSlotFreeList();
    if (LIKELY(slotFreeListMetaInfo.firstFreeObject_ != nullptr)) {
        ResetFreeList(slotFreeListMetaInfo);
        result = freeList_.TryAllocate();
        ASSERT(result > 0);
        return result;
    }

    return 0;
}

ARK_INLINE void SlotAllocator::ResetFreeList(SlotFreeListMetaInfo slotFreeListMetaInfo)
{
    freeList_.Reset(slotFreeListMetaInfo);
}

std::pair<uintptr_t, size_t> SlotAllocator::TryAllocateBuffer(size_t bufferSize)
{
    return freeList_.TryAllocateBuffer(bufferSize);
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_CMS_MEM_SLOT_ALLOCATOR_INL_H
