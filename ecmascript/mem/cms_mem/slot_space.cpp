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

#include "ecmascript/mem/cms_mem/slot_space-inl.h"

#include "ecmascript/mem/heap-inl.h"

namespace panda::ecmascript {

SlotSpace::SlotSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, heap->GetHeapRegionAllocator(), MemSpaceType::SLOT_SPACE, initialCapacity, maximumCapacity),
      SweepableSpace(),
      localHeap_(heap),
      minimumCapacity_(initialCapacity)
{
    InitializeAllocators();
}

void SlotSpace::InitializeAllocators()
{
    for (size_t i = 0; i < SlotSpaceConfig::NUM_SLOTS; ++i) {
        size_t slotSize = GetSlotSizeByIdx(i);
        regionChainManagers_[i].Initialize(localHeap_, slotSize);
        allocators_[i].Initialize(slotSize, &regionChainManagers_[i]);
    }
}

void SlotSpace::TryTriggerGCIfNeed()
{
    localHeap_->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT);
}

void SlotSpace::RequestGC()
{
    localHeap_->CollectGarbage(TriggerGCType::CMS_GC, GCReason::ALLOCATION_FAILED);
}

bool SlotSpace::TryExpandAllocator(SlotAllocator *allocator)
{
    if (localHeap_->SlotSpaceExceedLimit()) {
        return false;
    }

    ExpandAllocator(allocator);
    return true;
}

void SlotSpace::ExpandAllocator(SlotAllocator *allocator)
{
    Region *region = AllocateRegion(allocator->GetSlotSize());
    allocator->Expand(region);
    IncreaseCommitted(region->GetCapacity());
}

Region *SlotSpace::AllocateRegion(size_t slotSize)
{
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, localHeap_->GetJSThread(),
        localHeap_, false, slotSize);
    ASSERT(region != nullptr);
    return region;
}

void SlotSpace::IterateOverObjects(const std::function<void(TaggedObject *object)> &visitor)
{
    for (SlotAllocator &allocator : allocators_) {
        allocator.Discard();
    }
    for (CMSRegionChainManager &regionChainManager : regionChainManagers_) {
        size_t slotSize = regionChainManager.GetSlotSize();
        regionChainManager.EnumerateRegions([&visitor, slotSize](Region *region) {
            ASSERT(slotSize > 0);
            uintptr_t curPtr = region->GetBegin();
            uintptr_t endPtr = CMSRegion::GetAllocatableEnd(CMSRegion::FromRegion(region), slotSize);
            while (curPtr < endPtr) {
                FreeObject *freeObject = FreeObject::Cast(curPtr);
                size_t objSize;
                ASAN_UNPOISON_MEMORY_REGION(freeObject, TaggedObject::TaggedObjectSize());
                if (!freeObject->IsFreeObject()) {
                    auto obj = reinterpret_cast<TaggedObject *>(curPtr);
                    visitor(obj);
                    objSize = slotSize;
                } else {
                    freeObject->AsanUnPoisonFreeObject();
                    objSize = freeObject->Available();
                    freeObject->AsanPoisonFreeObject();
                }
                curPtr += objSize;
                ASSERT(objSize % slotSize == 0);
                CHECK_OBJECT_SIZE(objSize);
            }
            CHECK_REGION_END(curPtr, endPtr);
        });
    }
}

void SlotSpace::PrepareSweeping()
{
    for (SlotAllocator &allocator : allocators_) {
        allocateAfterLastGC_ += allocator.Discard();
    }
    for (CMSRegionChainManager &regionChainManager : regionChainManagers_) {
        regionChainManager.PrepareSweeping();
    }
}

void SlotSpace::Sweep()
{
    for (SlotAllocator &allocator : allocators_) {
        allocateAfterLastGC_ += allocator.Discard();
    }
    for (CMSRegionChainManager &regionChainManager : regionChainManagers_) {
        regionChainManager.Sweep();
    }
}

void SlotSpace::AsyncSweep([[maybe_unused]] bool isMain)
{
    for (CMSRegionChainManager &regionChainManager : regionChainManagers_) {
        regionChainManager.ConcurrentSweep(CMSRegionChainManager::SweepMode::SWEEP_ALL);
    }
}

bool SlotSpace::TryFillSweptRegion()
{
    // Implement of SlotAllocator do not need to pre-fetch swept free list
    return false;
}

bool SlotSpace::FinishFillSweptRegion()
{
    for (CMSRegionChainManager &regionChainManager : regionChainManagers_) {
        regionChainManager.FinishSweeping();
    }
    return true;
}

void SlotSpace::AdjustCapacity(JSThread *thread)
{
    allocateAfterLastGC_ = 0;
    size_t objectSize = GetHeapObjectSize() + localHeap_->GetHugeObjectSpace()->GetHeapObjectSize();
    double objectSurvivalRate = static_cast<double>(objectSize) / initialCapacity_;
    double allocSpeed = localHeap_->GetMemController()->GetSlotAndHugeSpaceAllocationThroughputPerMS();
    size_t newCapacity = std::min(maximumCapacity_,
        objectSize + localHeap_->GetEcmaVM()->GetEcmaParamConfiguration().GetMinGrowingStep());
    if (objectSurvivalRate > GROW_OBJECT_SURVIVAL_RATE) {
        newCapacity = std::max(newCapacity, initialCapacity_ * GROWING_FACTOR);
        while (committedSize_ >= newCapacity && newCapacity < maximumCapacity_) {
            newCapacity = newCapacity * GROWING_FACTOR;
        }
        newCapacity = std::min(newCapacity, maximumCapacity_);
        if (newCapacity == maximumCapacity_) {
            localHeap_->GetJSObjectResizingStrategy()->UpdateGrowStep(
                thread,
                JSObjectResizingStrategy::PROPERTIES_GROW_SIZE * 2);   // 2: double
        }
        SetInitialCapacity(newCapacity);
        return;
    }
    if (initialCapacity_ < (MIN_GC_INTERVAL_MS * allocSpeed) && initialCapacity_ < maximumCapacity_) {
        newCapacity = std::max(newCapacity, initialCapacity_ * GROWING_FACTOR);
        newCapacity = std::min(newCapacity, maximumCapacity_);
        if (newCapacity == maximumCapacity_) {
            localHeap_->GetJSObjectResizingStrategy()->UpdateGrowStep(
                thread,
                JSObjectResizingStrategy::PROPERTIES_GROW_SIZE * 2);   // 2: double
        }
        SetInitialCapacity(newCapacity);
        return;
    }
    if (objectSurvivalRate < SHRINK_OBJECT_SURVIVAL_RATE) {
        if (initialCapacity_ <= minimumCapacity_) {
            SetInitialCapacity(newCapacity);
            return;
        }
        if (allocSpeed > LOW_ALLOCATION_SPEED_PER_MS) {
            SetInitialCapacity(newCapacity);
            return;
        }
        newCapacity = std::max(newCapacity, initialCapacity_ / GROWING_FACTOR);
        newCapacity = std::max(newCapacity, minimumCapacity_);
        localHeap_->GetJSObjectResizingStrategy()->UpdateGrowStep(thread);
        SetInitialCapacity(newCapacity);
        return;
    }
}

void SlotSpace::PrepareCompact()
{
    committedSize_ = 0;
    ASSERT(pendingReclaimFromRegions_.empty());
    pendingReclaimFromRegions_.reserve(GetRegionCount());
    for (CMSRegionChainManager &regionChainManager : regionChainManagers_) {
        regionChainManager.PrepareCompact(pendingReclaimFromRegions_);
    }
    for (SlotAllocator &allocator : allocators_) {
        allocateAfterLastGC_ += allocator.Discard();
    }
}

void SlotSpace::ReclaimFromRegions()
{
    for (Region *region : pendingReclaimFromRegions_) {
        region->DeleteLocalToShareRSet();
        heapRegionAllocator_->FreeRegion(region, 0);
    }
    pendingReclaimFromRegions_.clear();
}
}  // namespace panda::ecmascript