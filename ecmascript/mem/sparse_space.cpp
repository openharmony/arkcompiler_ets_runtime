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

#include "ecmascript/base/config.h"
#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/heap-inl.h"

namespace panda::ecmascript {
SparseSpace::SparseSpace(Heap *heap, MemSpaceType type, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, heap->GetHeapRegionAllocator(), type, initialCapacity, maximumCapacity),
      sweepState_(SweepState::NO_SWEEP),
      localHeap_(heap),
      liveObjectSize_(0)
{
    allocator_ = new FreeListAllocator<FreeObject>(heap);
}

void SparseSpace::Initialize()
{
    JSThread *thread = localHeap_->GetJSThread();
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, thread, localHeap_);
    region->SetLocalHeap(reinterpret_cast<uintptr_t>(localHeap_));
    AddRegion(region);

    allocator_->Initialize(region);
}

void SparseSpace::Reset()
{
    allocator_->RebuildFreeList();
    ReclaimRegions();
    liveObjectSize_ = 0;
}

void SparseSpace::ResetTopPointer(uintptr_t top)
{
    allocator_->ResetTopPointer(top);
}

uintptr_t SparseSpace::Allocate(size_t size, bool allowGC)
{
    ASSERT(spaceType_ != MemSpaceType::OLD_SPACE);
#if ECMASCRIPT_ENABLE_THREAD_STATE_CHECK
    if (UNLIKELY(!localHeap_->GetJSThread()->IsInRunningStateOrProfiling())) { // LOCV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "Allocate must be in jsthread running state";
        UNREACHABLE();
    }
#endif
    auto object = allocator_->Allocate(size);
    CHECK_OBJECT_AND_INC_OBJ_SIZE(size);

    if (sweepState_ == SweepState::SWEEPING) {
        object = AllocateAfterSweepingCompleted(size);
        CHECK_OBJECT_AND_INC_OBJ_SIZE(size);
    }

    // Check whether it is necessary to trigger Old GC before expanding to avoid OOM risk.
    if (allowGC && localHeap_->CheckAndTriggerOldGC()) {
        object = allocator_->Allocate(size);
        CHECK_OBJECT_AND_INC_OBJ_SIZE(size);
    }

    if (Expand()) {
        object = allocator_->Allocate(size);
        CHECK_OBJECT_AND_INC_OBJ_SIZE(size);
    }

    if (allowGC) {
        localHeap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::ALLOCATION_FAILED);
        object = Allocate(size, false);
        // Size is already increment
    }
    return object;
}

bool SparseSpace::Expand()
{
    if (CommittedSizeExceed()) {
        LOG_ECMA_MEM(INFO) << "Expand::Committed size " << committedSize_ << " of Sparse Space is too big. ";
        return false;
    }
    JSThread *thread = localHeap_->GetJSThread();
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, thread, localHeap_);
    region->SetLocalHeap(reinterpret_cast<uintptr_t>(localHeap_));
    AddRegion(region);
    allocator_->AddFree(region);
    return true;
}

uintptr_t SparseSpace::AllocateAfterSweepingCompleted(size_t size)
{
    ASSERT(sweepState_ == SweepState::SWEEPING);
    if (TryFillSweptRegion()) {
        auto object = allocator_->Allocate(size);
        if (object != 0) {
            return object;
        }
    }
    // Parallel sweep and fill
    localHeap_->GetSweeper()->EnsureTaskFinished(spaceType_);
    return allocator_->Allocate(size);
}

void SparseSpace::PrepareSweeping()
{
    liveObjectSize_ = 0;
    ASSERT(GetSweepingRegionSafe() == nullptr);
    ASSERT(GetSweptRegionSafe() == nullptr);
    EnumerateRegions([this](Region *current) {
        if (!current->InCollectSet()) {
            ASSERT(!current->IsGCFlagSet(RegionGCFlags::HAS_BEEN_SWEPT));
            if (UNLIKELY(localHeap_->ShouldVerifyHeap() &&
                current->IsGCFlagSet(RegionGCFlags::HAS_BEEN_SWEPT))) { // LOCV_EXCL_BR_LINE
                LOG_ECMA(FATAL) << "Region should not be swept before PrepareSweeping: " << current;
            }
            IncreaseLiveObjectSize(current->AliveObject());
            current->ResetWasted();
            current->SwapOldToNewRSetForCS();
            current->SwapLocalToShareRSetForCS();
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
        } else {
            current->MergeOldToNewRSetForCS();
            current->MergeLocalToShareRSetForCS();
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
        region->MergeOldToNewRSetForCS();
        region->MergeLocalToShareRSetForCS();
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
        return first->AliveObject() > second->AliveObject();
    });
}

Region *SparseSpace::GetSweepingRegionSafe()
{
    LockHolder holder(lock_);
    Region *region = nullptr;
    if (!sweepingList_.empty()) {
        region = sweepingList_.back();
        sweepingList_.pop_back();
    }
    return region;
}

void SparseSpace::AddSweptRegionSafe(Region *region)
{
    LockHolder holder(lock_);
    sweptList_.emplace_back(region);
    region->SetSwept();
}

Region *SparseSpace::GetSweptRegionSafe()
{
    LockHolder holder(lock_);
    Region *region = nullptr;
    if (!sweptList_.empty()) {
        region = sweptList_.back();
        sweptList_.pop_back();
    }
    return region;
}

Region *SparseSpace::TryToGetSuitableSweptRegion(size_t size)
{
    if (sweepState_ != SweepState::SWEEPING) {
        return nullptr;
    }
    if (sweptList_.empty()) {
        return nullptr;
    }
    LockHolder holder(lock_);
    for (auto iter = sweptList_.begin(); iter != sweptList_.end(); iter++) {
        if (allocator_->MatchFreeObjectSet(*iter, size)) {
            Region *region = *iter;
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
        auto size = header->GetSize();

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
    localHeap_->GetSweeper()->ClearRSetInRange(current, freeStart, freeEnd);
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
                objSize = obj->GetSize();
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
    if (LIKELY(!allocationCounter_.IsActive())) { // LCOV_EXCL_BR_LINE
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
            return availableRegion;
        } else {
            AddSweptRegionSafe(availableRegion);
        }
    }
    return nullptr;
}

Region *OldSpace::TryToGetExclusiveRegion(size_t size)
{
    if (sweepState_ == SweepState::SWEEPING) {
        Region *availableRegion = nullptr;
        availableRegion = TryToGetSuitableSweptRegion(size);
        if (availableRegion == nullptr) {
            availableRegion = TrySweepToGetSuitableRegion(size);
        }
        if (availableRegion) {
            FreeRegionFromSpace(availableRegion);
        }
        return availableRegion;
    } else {
        LockHolder lock(lock_);
        uintptr_t result = allocator_->LookupSuitableFreeObject(size);
        if (result != 0) {
            // Remove region from global old space
            Region *region = Region::ObjectAddressToRange(result);
            RemoveRegion(region);
            allocator_->DetachFreeObjectSet(region);
            DecreaseLiveObjectSize(region->AliveObject());
            return region;
        }
    }
    return nullptr;
}

void OldSpace::FreeRegionFromSpace(Region *region)
{
    region->ResetSwept();
    region->MergeOldToNewRSetForCS();
    region->MergeLocalToShareRSetForCS();
    LockHolder holder(lock_);
    RemoveRegion(region);
    DecreaseLiveObjectSize(region->AliveObject());
}

uintptr_t OldSpace::AllocateFast(size_t size)
{
#if ECMASCRIPT_ENABLE_THREAD_STATE_CHECK
    if (UNLIKELY(!localHeap_->GetJSThread()->IsInRunningStateOrProfiling())) { // LOCV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "Allocate must be in jsthread running state";
        UNREACHABLE();
    }
#endif
    auto object = allocator_->Allocate(size);
    CHECK_OBJECT_AND_INC_OBJ_SIZE(size);

    if (sweepState_ == SweepState::SWEEPING) {
        object = AllocateAfterSweepingCompleted(size);
        CHECK_OBJECT_AND_INC_OBJ_SIZE(size);
    }
    return 0;
}

uintptr_t OldSpace::AllocateSlow(size_t size, bool tryFast)
{
    if (tryFast) {
        uintptr_t object = AllocateFast(size);
        CHECK_OBJECT_AND_INC_OBJ_SIZE(size);
    }
    if (Expand()) {
        uintptr_t object = allocator_->Allocate(size);
        CHECK_OBJECT_AND_INC_OBJ_SIZE(size);
    }
    return 0;
}

void OldSpace::Merge(LocalSpace *localSpace)
{
    localSpace->FreeBumpPoint();
    LockHolder lock(lock_);
    size_t oldCommittedSize = committedSize_;
    localSpace->EnumerateRegions([&](Region *region) {
        localSpace->DetachFreeObjectSet(region);
        localSpace->RemoveRegion(region);
        localSpace->DecreaseLiveObjectSize(region->AliveObject());
        AddRegion(region);
        IncreaseLiveObjectSize(region->AliveObject());
        allocator_->CollectFreeObjectSet(region);
    });
    size_t hugeSpaceCommitSize = localHeap_->GetHugeObjectSpace()->GetCommittedSize();
    if (committedSize_ + hugeSpaceCommitSize > GetOverShootMaximumCapacity()) {
        LOG_ECMA_MEM(ERROR) << "Merge::Committed size " << committedSize_ << " of old space is too big. ";
        if (localHeap_->CanThrowOOMError()) {
            localHeap_->ShouldThrowOOMError(true);
        }
        IncreaseMergeSize(committedSize_ - oldCommittedSize);
        size_t committedOverSizeLimit = committedSize_ + hugeSpaceCommitSize - GetOverShootMaximumCapacity();
        IncreaseCommittedOverSizeLimit(committedOverSizeLimit);
        // if throw OOM, temporarily increase space size to avoid vm crash
        IncreaseOutOfMemoryOvershootSize(committedOverSizeLimit);
    }

    localSpace->GetRegionList().Clear();
    allocator_->IncreaseAllocatedSize(localSpace->GetTotalAllocatedSize());
}

void OldSpace::SelectCSet()
{
    if (localHeap_->IsMarking()) {
        localHeap_->GetEcmaGCStats()->RecordStatisticBeforeGC(TriggerGCType::OLD_GC, GCReason::OTHER);
    }
    if (localHeap_->InSensitiveStatus()) {
        return;
    }
    CheckRegionSize();
    // 1、Select region which alive object larger than limit
    int64_t evacuateSizeLimit = 0;
    if (!localHeap_->IsInBackground()) {
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
    int64_t expectFreeSize =
        static_cast<int64_t>(localHeap_->GetCommittedSize() - localHeap_->GetHeapAliveSizeAfterGC());
    int64_t evacuateSize = std::min(evacuateSizeLimit, expectFreeSize);
    EnumerateCollectRegionSet([&](Region *current) {
        if (evacuateSize > 0) {
            selectedRegionNumber++;
            evacuateSize -= current->AliveObject();
        } else {
            return;
        }
    });
    LOG_ECMA_MEM(DEBUG) << "Max evacuation size is 6_MB. The CSet region number: "
        << selectedRegionNumber;
    selectedRegionNumber = std::max(selectedRegionNumber, GetSelectedRegionNumber());
    if (collectRegionSet_.size() > selectedRegionNumber) {
        collectRegionSet_.resize(selectedRegionNumber);
    }

    localHeap_->GetEcmaGCStats()->SetRecordData(
        RecordData::COLLECT_REGION_SET_SIZE, collectRegionSet_.size() * Region::AVERAGE_REGION_EVACUATE_SIZE);
    EnumerateCollectRegionSet([&](Region *current) {
        RemoveRegion(current);
        DecreaseLiveObjectSize(current->AliveObject());
        allocator_->DetachFreeObjectSet(current);
        current->SetGCFlag(RegionGCFlags::IN_COLLECT_SET);
    });
    sweepState_ = SweepState::NO_SWEEP;
    LOG_ECMA_MEM(DEBUG) << "Select CSet success: number is " << collectRegionSet_.size();
}

void OldSpace::CheckRegionSize()
{
#ifndef NDEBUG
    if (sweepState_ == SweepState::SWEEPING) {
        localHeap_->GetSweeper()->EnsureTaskFinished(spaceType_);
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
    size_t cachedSize = localHeap_->GetRegionCachedSize();
    EnumerateCollectRegionSet([this, &cachedSize](Region *region) {
        region->DeleteCrossRegionRSet();
        region->DeleteOldToNewRSet();
        region->DeleteLocalToShareRSet();
        region->DeleteSweepingOldToNewRSet();
        region->DeleteSweepingLocalToShareRSet();
        region->DestroyFreeObjectSets();
        heapRegionAllocator_->FreeRegion(region, cachedSize);
    });
    collectRegionSet_.clear();
}

bool OldSpace::SwapRegion(Region *region, SemiSpace *fromSpace)
{
    if (committedSize_ + region->GetCapacity() > maximumCapacity_) {
        return false;
    }
    fromSpace->RemoveRegion(region);
    region->InitializeFreeObjectSets();
    region->ResetRegionFlag(RegionSpaceFlag::IN_OLD_SPACE, RegionGCFlags::IN_NEW_TO_OLD_SET);

    regionList_.AddNodeToFront(region);
    IncreaseCommitted(region->GetCapacity());
    IncreaseObjectSize(region->GetSize());
    IncreaseLiveObjectSize(region->AliveObject());
    return true;
}

void OldSpace::PrepareSweepNewToOldRegions()
{
    EnumerateRegions([this](Region *current) {
        if (current->InNewToOldSet()) {
            ASSERT(!current->IsGCFlagSet(RegionGCFlags::HAS_BEEN_SWEPT));
            if (UNLIKELY(localHeap_->ShouldVerifyHeap() &&
                current->IsGCFlagSet(RegionGCFlags::HAS_BEEN_SWEPT))) { // LOCV_EXCL_BR_LINE
                LOG_ECMA(FATAL) << "Region should not be swept before PrepareSweeping: " << current;
            }
            current->ResetWasted();
            current->SwapOldToNewRSetForCS();
            current->SwapLocalToShareRSetForCS();
            current->ClearGCFlag(RegionGCFlags::IN_NEW_TO_OLD_SET);
            AddSweepingRegion(current);
        }
    });
    sweepState_ = SweepState::SWEEPING;
}

void OldSpace::SweepNewToOldRegions()
{
    EnumerateRegions([this](Region *current) {
        if (current->InNewToOldSet()) {
            current->ResetWasted();
            current->ClearGCFlag(RegionGCFlags::IN_NEW_TO_OLD_SET);
            FreeRegion(current);
        }
    });
}

LocalSpace::LocalSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : SparseSpace(heap, LOCAL_SPACE, initialCapacity, maximumCapacity) {}

bool LocalSpace::AddRegionToList(Region *region)
{
    if (committedSize_ >= maximumCapacity_) { // LOCV_EXCL_BR_LINE
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
    Region *currentRegion = GetCurrentRegion();
    if (GetCurrentRegion() != nullptr) {
        // Do not use allocator_->GetTop(), because it may point to freeObj from other regions.
        currentRegion->SetHighWaterMark(currentRegion->GetBegin() + currentRegion->AliveObject());
    }
}

uintptr_t NonMovableSpace::CheckAndAllocate(size_t size)
{
    if (maximumCapacity_ == committedSize_ && GetHeapObjectSize() > MAX_NONMOVABLE_LIVE_OBJ_SIZE &&
        !localHeap_->GetOldGCRequested() && !localHeap_->NeedStopCollection()) {
        localHeap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::ALLOCATION_LIMIT);
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

uintptr_t AppSpawnSpace::AllocateSync(size_t size)
{
    LockHolder holder(mutex_);
    return Allocate(size);
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

void LocalSpace::ForceExpandInGC()
{
    JSThread *thread = localHeap_->GetJSThread();
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, thread, localHeap_);
    region->SetLocalHeap(reinterpret_cast<uintptr_t>(localHeap_));
    AddRegion(region);
    allocator_->AddFree(region);
}

uintptr_t LocalSpace::Allocate(size_t size, bool isExpand)
{
    auto object = allocator_->Allocate(size);
    if (object == 0 && isExpand) {
        if (!Expand()) {
            ForceExpandInGC();
            localHeap_->ShouldThrowOOMError(true);
        }
        object = allocator_->Allocate(size);
        ASSERT(object != 0);
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

MachineCodeSpace::~MachineCodeSpace()
{
    if (localHeap_->GetEcmaVM()->GetJSOptions().GetEnableJitFort()) {
        if (jitFort_) {
            delete jitFort_;
            jitFort_ = nullptr;
        }
    }
}

void MachineCodeSpace::PrepareSweeping()
{
    ASSERT(!g_isEnableCMCGC);
    // fill free obj before sparse space prepare sweeping rebuild freelist, as may fail set free obj
    // when iterate machine code space in GetMachineCodeObject
    allocator_->FillBumpPointer();
    SparseSpace::PrepareSweeping();
    if (jitFort_) {
        jitFort_->PrepareSweeping();
    }
}

void MachineCodeSpace::Sweep()
{
    if (!g_isEnableCMCGC) {
        SparseSpace::Sweep();
    }
    if (jitFort_) {
        jitFort_->Sweep();
    }
}

void MachineCodeSpace::AsyncSweep(bool isMain)
{
    ASSERT(!g_isEnableCMCGC);
    LockHolder holder(asyncSweepMutex_);
    SparseSpace::AsyncSweep(isMain);
    if (jitFort_) {
        jitFort_->AsyncSweep();
    }
}

uintptr_t MachineCodeSpace::JitFortAllocate(MachineCodeDesc *desc)
{
    if (!jitFort_) {
        jitFort_ = new JitFort();
    }
    if (!g_isEnableCMCGC) {
        localHeap_->GetSweeper()->EnsureTaskFinishedNoCheck(spaceType_);
    }
    return jitFort_->Allocate(desc);
}

uintptr_t MachineCodeSpace::Allocate(size_t size, bool allowGC)
{
    ASSERT(!g_isEnableCMCGC);
    return SparseSpace::Allocate(size, allowGC);
}

uintptr_t MachineCodeSpace::Allocate(size_t size, MachineCodeDesc *desc, bool allowGC)
{
    ASSERT(!g_isEnableCMCGC);
#if ECMASCRIPT_ENABLE_THREAD_STATE_CHECK
    if (UNLIKELY(!localHeap_->GetJSThread()->IsInRunningStateOrProfiling())) { // LOCV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "Allocate must be in jsthread running state";
        UNREACHABLE();
    }
#endif
    // Include JitFort allocation size in Space LiveObjectSize and Region AliveObject size
    // in CHECK_AND_INC_OBJ_SIZE. Could be a problem with InvokeAllocationInspectr with
    // instruction separated from Machine Code object into Jit FortSpace.

    auto object = allocator_->Allocate(size);
    CHECK_OBJECT_AND_INC_OBJ_SIZE(size + desc->instructionsSize);

    if (sweepState_ == SweepState::SWEEPING) {
        object = AllocateAfterSweepingCompleted(size);
        CHECK_OBJECT_AND_INC_OBJ_SIZE(size + desc->instructionsSize);
    }

    // Check whether it is necessary to trigger Old GC before expanding to avoid OOM risk.
    if (allowGC && localHeap_->CheckAndTriggerOldGC()) {
        object = allocator_->Allocate(size);
        CHECK_OBJECT_AND_INC_OBJ_SIZE(size + desc->instructionsSize);
    }

    if (Expand()) {
        object = allocator_->Allocate(size);
        CHECK_OBJECT_AND_INC_OBJ_SIZE(size + desc->instructionsSize);
    }

    if (allowGC) {
        localHeap_->CollectGarbage(TriggerGCType::OLD_GC, GCReason::ALLOCATION_FAILED);
        object = Allocate(size, desc, false);
        // Size is already increment
    }
    return object;
}

size_t MachineCodeSpace::CheckMachineCodeObject(uintptr_t curPtr, uintptr_t &machineCode, uintptr_t pc)
{
    ASSERT(!g_isEnableCMCGC);
    auto freeObject = FreeObject::Cast(curPtr);
    size_t objSize = 0;
    if (!freeObject->IsFreeObject()) {
        auto obj = MachineCode::Cast(reinterpret_cast<TaggedObject*>(curPtr));
        if (obj->IsInText(pc)) {
            machineCode = curPtr;
        }
        objSize = obj->GetSize();
    } else {
        objSize = freeObject->Available();
    }
    return objSize;
}

void MachineCodeSpace::StoreMachineCodeObjectLocation(uintptr_t start, uintptr_t end, uintptr_t address)
{
    machineCodeObjectLocations.InsertMachineCodeObject(start, end, address);
}

uintptr_t MachineCodeSpace::GetMachineCodeObject(uintptr_t pc)
{
    if (g_isEnableCMCGC) {
        return machineCodeObjectLocations.GetMachineCodeObject(pc);
    }
    uintptr_t machineCode = 0;
    LockHolder holder(asyncSweepMutex_);
    allocator_->FillBumpPointer();

    EnumerateRegions([&](Region *region) {
        if (machineCode != 0) {
            return;
        }
        if (region->InCollectSet() || (!region->InRange(pc) && !InJitFortRange(pc))) {
            return;
        }
        uintptr_t curPtr = region->GetBegin();
        uintptr_t endPtr = region->GetEnd();
        while (curPtr < endPtr) {
            size_t objSize = CheckMachineCodeObject(curPtr, machineCode, pc);
            if (machineCode != 0) {
                return;
            }
            curPtr += objSize;
            CHECK_OBJECT_SIZE(objSize);
        }
        CHECK_REGION_END(curPtr, endPtr);
    });
    return machineCode;
}
}  // namespace panda::ecmascript
