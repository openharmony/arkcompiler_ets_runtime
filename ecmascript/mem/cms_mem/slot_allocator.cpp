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

#include "ecmascript/mem/cms_mem/slot_allocator-inl.h"

#include "ecmascript/mem/heap-inl.h"

namespace panda::ecmascript {

void SlotAllocator::Initialize(size_t slotSize, CMSRegionChainManager *regionChainManager)
{
    ASSERT(regionChainManager_ == nullptr && regionChainManager != nullptr);
    freeList_.Initialize(slotSize);
    regionChainManager_ = regionChainManager;
}

size_t SlotAllocator::RecordFreeListSoldOut()
{
    ASSERT(freeList_.IsSoldOut());
    return Discard();
}

size_t SlotAllocator::Discard()
{
    size_t allocatedSize = freeList_.Discard([this](uintptr_t start, size_t size) {
        FreeObject::FillFreeObject(regionChainManager_->GetLocalHeap(), start, size);
    });
    ASSERT(freeList_.IsSoldOut());
    return allocatedSize;
}

void SlotAllocator::Expand(Region *region)
{
    ASSERT(freeList_.IsSoldOut());
    regionChainManager_->AddNewRegion(region);

    uintptr_t start = region->GetBegin();
    uintptr_t end = region->GetEnd();
    size_t totalUsableSize = (end - start) / freeList_.GetSlotSize() * freeList_.GetSlotSize();
    ASSERT(totalUsableSize > 0 && totalUsableSize % freeList_.GetSlotSize() == 0);
    FreeObject::FillFreeObject(regionChainManager_->GetLocalHeap(), start, totalUsableSize);
    ResetFreeList({FreeObject::Cast(start), totalUsableSize});
}
}  // namespace panda::ecmascript