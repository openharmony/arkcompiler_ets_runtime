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

#include "ecmascript/mem/sparse_space.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/mem/free_object_set.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
SparseSpace::SparseSpace(Heap *heap, MemSpaceType type, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap->GetHeapRegionAllocator(), type, initialCapacity, maximumCapacity),
      sweepState_(SweepState::NO_SWEEP),
      heap_(heap),
      liveObjectSize_(0)
{
    allocator_ = new FreeListAllocator(heap);
}

void SparseSpace::Initialize()
{
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE,
                                                                 heap_->GetJSThread());
    region->InitializeFreeObjectSets();
    AddRegion(region);

    allocator_->Initialize(region);
}

void SparseSpace::Reset()
{
    allocator_->RebuildFreeList();
    ReclaimRegions();
    liveObjectSize_ = 0;
}

uintptr_t SparseSpace::Allocate(size_t size, bool allowGC)
{
    auto object = allocator_->Allocate(size);
    CHECK_OBJECT_AND_INC_OBJ_SIZE(size);

    if (sweepState_ == SweepState::SWEEPING) {
        object = AllocateAfterSweepingCompleted(size);
        CHECK_OBJECT_AND_INC_OBJ_SIZE(size);
    }

    // Check whether it is necessary to trigger Old GC before expanding to avoid OOM risk.
    if (allowGC && heap_->CheckAndTriggerOldGC()) {
        object = allocator_->Allocate(size);
        CHECK_OBJECT_AND_INC_OBJ_SIZE(size);
    }

    if (Expand()) {
        object = allocator_->Allocate(size);
        CHECK_OBJECT_AND_INC_OBJ_SIZE(size);
    }

    if (allowGC) {
        heap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::ALLOCATION_FAILED);
        object = Allocate(size, false);
        // Size is already increment
    }
    return object;
}

bool SparseSpace::Expand()
{
    if (committedSize_ >= maximumCapacity_ + overshootSize_ + outOfMemoryOvershootSize_) {
        LOG_ECMA_MEM(INFO) << "Expand::Committed size " << committedSize_ << " of Sparse Space is too big. ";
        return false;
    }

    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, heap_->GetJSThread());
    region->InitializeFreeObjectSets();
    AddRegion(region);
    allocator_->AddFree(region);
    return true;
}

uintptr_t SparseSpace::AllocateAfterSweepingCompleted(size_t size)
{
    ASSERT(sweepState_ == SweepState::SWEEPING);
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ConcurrentSweepingWait);
    if (TryFillSweptRegion()) {
        auto object = allocator_->Allocate(size);
        if (object != 0) {
            return object;
        }
    }
    // Parallel sweep and fill
    heap_->GetSweeper()->EnsureTaskFinished(spaceType_);
    return allocator_->Allocate(size);
}

void SparseSpace::PrepareSweeping()
{
    liveObjectSize_ = 0;
    EnumerateRegions([this](Region *current) {
        if (!current->InCollectSet()) {
            IncreaseLiveObjectSize(current->AliveObject());
            current->ResetWasted();
            current->SwapRSetForConcurrentSweeping();
            AddSweepingRegion(current);
        }
    });
    SortSweepingRegion();
    sweepState_ = SweepState::SWEEPING;
    allocator_->RebuildFreeList();
}

void SparseSpace::AsyncSweep(bool isMain)
{
    Region *current = GetSweepingRegionSafe();
    while (current != nullptr) {
        FreeRegion(current, isMain);
        // Main thread sweeping region is added;
        if (!isMain) {
            AddSweptRegionSafe(current);
            current->SetSwept();
        } else {
            current->MergeRSetForConcurrentSweeping();
        }
        current = GetSweepingRegionSafe();
    }
}

void SparseSpace::Sweep()
{
    liveObjectSize_ = 0;
    allocator_->RebuildFreeList();
    EnumerateRegions([this](Region *current) {
        if (!current->InCollectSet()) {
            IncreaseLiveObjectSize(current->AliveObject());
            current->ResetWasted();
            FreeRegion(current);
        }
    });
}

bool SparseSpace::TryFillSweptRegion()
{
    if (sweptList_.empty()) {
        return false;
    }
    Region *region = nullptr;
    while ((region = GetSweptRegionSafe()) != nullptr) {
        allocator_->CollectFreeObjectSet(region);
        region->ResetSwept();
        region->MergeRSetForConcurrentSweeping();
    }
    return true;
}

bool SparseSpace::FinishFillSweptRegion()
{
    bool ret = TryFillSweptRegion();
    sweepState_ = SweepState::SWEPT;
    return ret;
}

void SparseSpace::AddSweepingRegion(Region *region)
{
    sweepingList_.emplace_back(region);
}

void SparseSpace::SortSweepingRegion()
{
    // Sweep low alive object size at first
    std::sort(sweepingList_.begin(), sweepingList_.end(), [](Region *first, Region *second) {
        return first->AliveObject() < second->AliveObject();
    });
}

Region *SparseSpace::GetSweepingRegionSafe()
{
    os::memory::LockHolder holder(lock_);
    Region *region = nullptr;
    if (!sweepingList_.empty()) {
        region = sweepingList_.back();
        sweepingList_.pop_back();
    }
    return region;
}

void SparseSpace::AddSweptRegionSafe(Region *region)
{
    os::memory::LockHolder holder(lock_);
    sweptList_.emplace_back(region);
}

Region *SparseSpace::GetSweptRegionSafe()
{
    os::memory::LockHolder holder(lock_);
    Region *region = nullptr;
    if (!sweptList_.empty()) {
        region = sweptList_.back();
        sweptList_.pop_back();
    }
    return region;
}

void SparseSpace::FreeRegionFromSpace(Region *region)
{
    region->ResetSwept();
    region->MergeRSetForConcurrentSweeping();
    RemoveRegion(region);
    DecreaseLiveObjectSize(region->AliveObject());
}

Region *SparseSpace::TryToGetSuitableSweptRegion(size_t size)
{
    if (sweepState_ != SweepState::SWEEPING) {
        return nullptr;
    }
    if (sweptList_.empty()) {
        return nullptr;
    }
    os::memory::LockHolder holder(lock_);
    for (auto iter = sweptList_.begin(); iter != sweptList_.end(); iter++) {
        if (allocator_->MatchFreeObjectSet(*iter, size)) {
            Region *region = *iter;
            FreeRegionFromSpace(region);
            sweptList_.erase(iter);
            return region;
        }
    }
    return nullptr;
}

void SparseSpace::FreeRegion(Region *current, bool isMain)
{
    uintptr_t freeStart = current->GetBegin();
    current->IterateAllMarkedBits([this, &current, &freeStart, isMain](void *mem) {
        ASSERT(current->InRange(ToUintPtr(mem)));
        auto header = reinterpret_cast<TaggedObject *>(mem);
        auto klass = header->GetClass();
        auto size = klass->SizeFromJSHClass(header);

        uintptr_t freeEnd = ToUintPtr(mem);
        if (freeStart != freeEnd) {
            FreeLiveRange(current, freeStart, freeEnd, isMain);
        }
        freeStart = freeEnd + size;
    });
    uintptr_t freeEnd = current->GetEnd();
    if (freeStart != freeEnd) {
        FreeLiveRange(current, freeStart, freeEnd, isMain);
    }
}

void SparseSpace::FreeLiveRange(Region *current, uintptr_t freeStart, uintptr_t freeEnd, bool isMain)
{
    heap_->GetSweeper()->ClearRSetInRange(current, freeStart, freeEnd);
    allocator_->Free(freeStart, freeEnd - freeStart, isMain);
}

void SparseSpace::IterateOverObjects(const std::function<void(TaggedObject *object)> &visitor) const
{
    allocator_->FillBumpPointer();
    EnumerateRegions([&](Region *region) {
        if (region->InCollectSet()) {
            return;
        }
        uintptr_t curPtr = region->GetBegin();
        uintptr_t endPtr = region->GetEnd();
        while (curPtr < endPtr) {
            auto freeObject = FreeObject::Cast(curPtr);
            size_t objSize;
            // If curPtr is freeObject, It must to mark unpoison first.
            ASAN_UNPOISON_MEMORY_REGION(freeObject, TaggedObject::TaggedObjectSize());
            if (!freeObject->IsFreeObject()) {
                auto obj = reinterpret_cast<TaggedObject *>(curPtr);
                visitor(obj);
                objSize = obj->GetClass()->SizeFromJSHClass(obj);
            } else {
                freeObject->AsanUnPoisonFreeObject();
                objSize = freeObject->Available();
                freeObject->AsanPoisonFreeObject();
            }
            curPtr += objSize;
            CHECK_OBJECT_SIZE(objSize);
        }
        CHECK_REGION_END(curPtr, endPtr);
    });
}

void SparseSpace::IterateOldToNewOverObjects(
    const std::function<void(TaggedObject *object, JSTaggedValue value)> &visitor) const
{
    auto cb = [visitor](void *mem) -> bool {
        ObjectSlot slot(ToUintPtr(mem));
        visitor(reinterpret_cast<TaggedObject *>(mem), JSTaggedValue(slot.GetTaggedType()));
        return true;
    };
    EnumerateRegions([cb] (Region *region) {
        region->IterateAllSweepingRSetBits(cb);
        region->IterateAllOldToNewBits(cb);
    });
}

size_t SparseSpace::GetHeapObjectSize() const
{
    return liveObjectSize_;
}

void SparseSpace::IncreaseAllocatedSize(size_t size)
{
    allocator_->IncreaseAllocatedSize(size);
}

size_t SparseSpace::GetTotalAllocatedSize() const
{
    return allocator_->GetAllocatedSize();
}

void SparseSpace::DetachFreeObjectSet(Region *region)
{
    allocator_->DetachFreeObjectSet(region);
}

void SparseSpace::InvokeAllocationInspector(Address object, size_t size, size_t alignedSize)
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

OldSpace::OldSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : SparseSpace(heap, OLD_SPACE, initialCapacity, maximumCapacity) {}

Region *OldSpace::TrySweepToGetSuitableRegion(size_t size)
{
    // Try Sweeping region to get space for allocation
    // since sweepingList_ is ordered, we just need to check once
    Region *availableRegion = GetSweepingRegionSafe();
    if (availableRegion != nullptr) {
        FreeRegion(availableRegion, false);
        // if region has free enough space for the size,
        // free the region from current space
        // and return for local space to use
        // otherwise, we add region to sweptList_.
        if (allocator_->MatchFreeObjectSet(availableRegion, size)) {
            FreeRegionFromSpace(availableRegion);
            return availableRegion;
        } else {
            AddSweptRegionSafe(availableRegion);
            availableRegion->SetSwept();
        }
    }
    return nullptr;
}

Region *OldSpace::TryToGetExclusiveRegion(size_t size)
{
    os::memory::LockHolder lock(lock_);
    uintptr_t result = allocator_->LookupSuitableFreeObject(size);
    if (result != 0) {
        // Remove region from global old space
        Region *region = Region::ObjectAddressToRange(result);
        RemoveRegion(region);
        allocator_->DetachFreeObjectSet(region);
        DecreaseLiveObjectSize(region->AliveObject());
        return region;
    }
    if (sweepState_ == SweepState::SWEEPING) {
        Region *availableRegion = nullptr;
        availableRegion = TryToGetSuitableSweptRegion(size);
        if (availableRegion != nullptr) {
            return availableRegion;
        }
        return TrySweepToGetSuitableRegion(size);
    }
    return nullptr;
}

void OldSpace::Merge(LocalSpace *localSpace)
{
    localSpace->FreeBumpPoint();
    os::memory::LockHolder lock(lock_);
    size_t oldCommittedSize = committedSize_;
    localSpace->EnumerateRegions([&](Region *region) {
        localSpace->DetachFreeObjectSet(region);
        localSpace->RemoveRegion(region);
        localSpace->DecreaseLiveObjectSize(region->AliveObject());
        AddRegion(region);
        IncreaseLiveObjectSize(region->AliveObject());
        allocator_->CollectFreeObjectSet(region);
    });
    size_t hugeSpaceCommitSize = heap_->GetHugeObjectSpace()->GetCommittedSize();
    if (committedSize_ + hugeSpaceCommitSize > GetOverShootMaximumCapacity()) {
        LOG_ECMA_MEM(ERROR) << "Merge::Committed size " << committedSize_ << " of old space is too big. ";
        heap_->ShouldThrowOOMError(true);
        IncreaseMergeSize(committedSize_ - oldCommittedSize);
        // if throw OOM, temporarily increase space size to avoid vm crash
        IncreaseOutOfMemoryOvershootSize(committedSize_ + hugeSpaceCommitSize - GetOverShootMaximumCapacity());
    }

    localSpace->GetRegionList().Clear();
    allocator_->IncreaseAllocatedSize(localSpace->GetTotalAllocatedSize());
}

void OldSpace::SelectCSet()
{
    if (heap_->GetJSThread()->IsMarking()) {
        heap_->GetEcmaVM()->GetEcmaGCStats()->RecordStatisticBeforeGC(TriggerGCType::OLD_GC, GCReason::OTHER);
    }
    CheckRegionSize();
    // 1、Select region which alive object larger than limit
    int64_t evacuateSizeLimit = 0;
    if (!heap_->IsInBackground()) {
        evacuateSizeLimit = PARTIAL_GC_MAX_EVACUATION_SIZE_FOREGROUND;
        EnumerateRegions([this](Region *region) {
            if (!region->MostObjectAlive()) {
                collectRegionSet_.emplace_back(region);
            }
        });
    } else {
        evacuateSizeLimit = PARTIAL_GC_MAX_EVACUATION_SIZE_BACKGROUND;
        EnumerateRegions([this](Region *region) {
            if (region->BelowCompressThreasholdAlive() || !region->MostObjectAlive()) {
                collectRegionSet_.emplace_back(region);
            }
        });
    }
    if (collectRegionSet_.size() < PARTIAL_GC_MIN_COLLECT_REGION_SIZE) {
        LOG_ECMA_MEM(DEBUG) << "Select CSet failure: number is too few";
        collectRegionSet_.clear();
        return;
    }
    // sort
    std::sort(collectRegionSet_.begin(), collectRegionSet_.end(), [](Region *first, Region *second) {
        return first->AliveObject() < second->AliveObject();
    });

    // Limit cset size
    unsigned long selectedRegionNumber = 0;
    int64_t expectFreeSize = static_cast<int64_t>(heap_->GetCommittedSize() - heap_->GetHeapAliveSizeAfterGC());
    int64_t evacuateSize = std::min(evacuateSizeLimit, expectFreeSize);
    EnumerateCollectRegionSet([&](Region *current) {
        if (evacuateSize > 0) {
            selectedRegionNumber++;
            evacuateSize -= current->AliveObject();
        } else {
            return;
        }
    });
    OPTIONAL_LOG(heap_->GetEcmaVM(), INFO) << "Max evacuation size is 6_MB. The CSet region number: "
        << selectedRegionNumber;
    selectedRegionNumber = std::max(selectedRegionNumber, GetSelectedRegionNumber());
    if (collectRegionSet_.size() > selectedRegionNumber) {
        collectRegionSet_.resize(selectedRegionNumber);
    }

    heap_->GetEcmaVM()->GetEcmaGCStats()->SetRecordData(
        RecordData::COLLECT_REGION_SET_SIZE, collectRegionSet_.size() * Region::AVERAGE_REGION_EVACUATE_SIZE);
    EnumerateCollectRegionSet([&](Region *current) {
        RemoveRegion(current);
        DecreaseLiveObjectSize(current->AliveObject());
        allocator_->DetachFreeObjectSet(current);
        current->SetGCFlag(RegionGCFlags::IN_COLLECT_SET);
    });
    sweepState_ = SweepState::NO_SWEEP;
    OPTIONAL_LOG(heap_->GetEcmaVM(), INFO) << "Select CSet success: number is " << collectRegionSet_.size();
}

void OldSpace::CheckRegionSize()
{
#ifndef NDEBUG
    if (sweepState_ == SweepState::SWEEPING) {
        heap_->GetSweeper()->EnsureTaskFinished(spaceType_);
    }
    size_t available = allocator_->GetAvailableSize();
    size_t wasted = allocator_->GetWastedSize();
    if (GetHeapObjectSize() + wasted + available != objectSize_) {
        LOG_GC(DEBUG) << "Actual live object size:" << GetHeapObjectSize()
                            << ", free object size:" << available
                            << ", wasted size:" << wasted
                            << ", but exception total size:" << objectSize_;
    }
#endif
}

void OldSpace::RevertCSet()
{
    EnumerateCollectRegionSet([&](Region *region) {
        region->ClearGCFlag(RegionGCFlags::IN_COLLECT_SET);
        AddRegion(region);
        allocator_->CollectFreeObjectSet(region);
        IncreaseLiveObjectSize(region->AliveObject());
    });
    collectRegionSet_.clear();
}

void OldSpace::ReclaimCSet()
{
    EnumerateCollectRegionSet([this](Region *region) {
        region->DeleteCrossRegionRSet();
        region->DeleteOldToNewRSet();
        region->DeleteSweepingRSet();
        region->DestroyFreeObjectSets();
        heapRegionAllocator_->FreeRegion(region);
    });
    collectRegionSet_.clear();
}

LocalSpace::LocalSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : SparseSpace(heap, LOCAL_SPACE, initialCapacity, maximumCapacity) {}

bool LocalSpace::AddRegionToList(Region *region)
{
    if (committedSize_ >= maximumCapacity_) {
        LOG_ECMA_MEM(FATAL) << "AddRegionTotList::Committed size " << committedSize_ << " of local space is too big.";
        return false;
    }
    AddRegion(region);
    allocator_->CollectFreeObjectSet(region);
    IncreaseLiveObjectSize(region->AliveObject());
    return true;
}

void LocalSpace::FreeBumpPoint()
{
    allocator_->FreeBumpPoint();
}

void LocalSpace::Stop()
{
    if (GetCurrentRegion() != nullptr) {
        GetCurrentRegion()->SetHighWaterMark(allocator_->GetTop());
    }
}

uintptr_t  NonMovableSpace::CheckAndAllocate(size_t size)
{
    if (maximumCapacity_ == committedSize_ && GetHeapObjectSize() > MAX_NONMOVABLE_LIVE_OBJ_SIZE &&
        !heap_->GetOldGCRequested())
    {
        heap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::ALLOCATION_LIMIT);
    }
    return Allocate(size);
}

NonMovableSpace::NonMovableSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : SparseSpace(heap, MemSpaceType::NON_MOVABLE, initialCapacity, maximumCapacity)
{
}

AppSpawnSpace::AppSpawnSpace(Heap *heap, size_t initialCapacity)
    : SparseSpace(heap, MemSpaceType::APPSPAWN_SPACE, initialCapacity, initialCapacity)
{
}

void AppSpawnSpace::IterateOverMarkedObjects(const std::function<void(TaggedObject *object)> &visitor) const
{
    EnumerateRegions([&](Region *current) {
        current->IterateAllMarkedBits([&](void *mem) {
            ASSERT(current->InRange(ToUintPtr(mem)));
            visitor(reinterpret_cast<TaggedObject *>(mem));
        });
    });
}

uintptr_t LocalSpace::Allocate(size_t size, bool isExpand)
{
    auto object = allocator_->Allocate(size);
    if (object == 0) {
        if (isExpand && Expand()) {
            object = allocator_->Allocate(size);
        }
    }
    if (object != 0) {
        Region::ObjectAddressToRange(object)->IncreaseAliveObject(size);
    }
    return object;
}

MachineCodeSpace::MachineCodeSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : SparseSpace(heap, MemSpaceType::MACHINE_CODE_SPACE, initialCapacity, maximumCapacity)
{
}
}  // namespace panda::ecmascript
