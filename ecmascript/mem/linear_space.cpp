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

#include "ecmascript/mem/linear_space.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/allocator-inl.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/mem_controller.h"

namespace panda::ecmascript {
LinearSpace::LinearSpace(Heap *heap, MemSpaceType type, size_t initialCapacity, size_t maximumCapacity)
    : MonoSpace(heap, heap->GetHeapRegionAllocator(), type, initialCapacity, maximumCapacity),
      localHeap_(heap),
      thread_(heap->GetJSThread()),
      waterLine_(0)
{
}

bool LinearSpace::TryGetUsableFreeList(size_t size)
{
    if (!freeList_.empty()) {
        FreeMemory freeMemory = freeList_.back();
        if (freeMemory.length >= size) {
            freeList_.pop_back();
            allocator_.Reset(freeMemory.start, freeMemory.start + freeMemory.length);
            currentFreeListLength_ = freeMemory.length;
            return true;
        }
    }
    return false;
}

bool LinearSpace::TryUseFreeList(size_t size)
{
    if (TryGetUsableFreeList(size)) {
        return true;
    }
    switch (sweeping_.load(std::memory_order_relaxed)) {
        case SweepingState::NO_SWEEP:
            return false;
        case SweepingState::SWEEPING:
            break;
        case SweepingState::SWEPT:
            FinishFillSweptRegion();
            return TryGetUsableFreeList(size);
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable, "
                            << static_cast<int>(sweeping_.load(std::memory_order_relaxed));
            UNREACHABLE();
    }
    {
        LockHolder lock(mutex_);
        if (!sweptFreeList_.empty()) {
            FreeMemory freeMemory = sweptFreeList_.back();
            if (freeMemory.length >= size) {
                sweptFreeList_.pop_back();
                allocator_.Reset(freeMemory.start, freeMemory.start + freeMemory.length);
                currentFreeListLength_ = freeMemory.length;
                return true;
            }
        }
    }
    return false;
}

uintptr_t LinearSpace::Allocate(size_t size, bool isPromoted)
{
#if ECMASCRIPT_ENABLE_THREAD_STATE_CHECK
    if (UNLIKELY(!localHeap_->GetJSThread()->IsInRunningStateOrProfiling())) { // LOCV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "Allocate must be in jsthread running state";
        UNREACHABLE();
    }
#endif
    auto object = allocator_.Allocate(size);
    if (object != 0) {
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
        // can not heap sampling in gc.
        if (!isPromoted) {
            InvokeAllocationInspector(object, size, size);
        }
#endif
        return object;
    }
    DiscardCurrentAllocator(isPromoted);
    if (TryUseFreeList(size)) {
        object = allocator_.Allocate(size);
        return object;
    }
    if (Expand(isPromoted)) {
        if (!isPromoted) {
            if (!localHeap_->NeedStopCollection() || localHeap_->IsNearGCInSensitive() ||
                (localHeap_->IsJustFinishStartup() && localHeap_->ObjectExceedJustFinishStartupThresholdForCM())) {
                localHeap_->TryTriggerConcurrentMarking(MarkReason::ALLOCATION_LIMIT);
            }
        }
        object = allocator_.Allocate(size);
#if ENABLE_LATEST_OPTIMIZATION
    } else if (localHeap_->IsMarking() || localHeap_->IsProcessingLocalToSharedRSet()) {
#else
    } else if (localHeap_->IsMarking()) {
#endif
        // Temporary adjust semi space capacity
        if (localHeap_->IsConcurrentFullMark() || localHeap_->InSensitiveStatus()) {
            size_t prev = overShootSize_;
            size_t maxOverShootSize = localHeap_->CalculateLinearSpaceOverShoot();
            size_t stepSize = localHeap_->GetEcmaParamConfiguration().GetSemiSpaceStepOvershootSize();
            size_t currentCapacity = initialCapacity_ + overShootSize_ + outOfMemoryOvershootSize_;
            size_t increaseOverShoot = std::max(committedSize_, currentCapacity) - currentCapacity + stepSize;
            ASSERT(stepSize > 0);
            size_t alignedIncreaseOverShoot = (increaseOverShoot + stepSize - 1) / stepSize * stepSize;
            overShootSize_ = std::min(overShootSize_ + alignedIncreaseOverShoot, maxOverShootSize);
        } else {
            size_t stepOverShootSize = localHeap_->GetEcmaParamConfiguration().GetSemiSpaceStepOvershootSize();
            size_t maxOverShootSize = std::max(initialCapacity_ / 2, stepOverShootSize); // 2: half
            if (overShootSizeForConcurrentMark_ < maxOverShootSize) {
                overShootSize_ += stepOverShootSize;
                overShootSizeForConcurrentMark_ += stepOverShootSize;
            }
        }

        if (Expand(isPromoted)) {
            object = allocator_.Allocate(size);
        }
    }
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
    if (object != 0 && !isPromoted) {
        InvokeAllocationInspector(object, size, size);
    }
#endif
    return object;
}

bool LinearSpace::Expand(bool isPromoted)
{
    if (committedSize_ >= initialCapacity_ + overShootSize_ + outOfMemoryOvershootSize_ &&
        (isPromoted || !localHeap_->NeedStopCollection())) {
        return false;
    }

    JSThread *thread = localHeap_->GetJSThread();
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, thread, localHeap_);
    allocator_.Reset(region->GetBegin(), region->GetEnd());
    AddRegion(region);
    currentAllocatorRegion_ = region;
    return true;
}

void LinearSpace::DiscardCurrentAllocator(bool isPromoted)
{
    if (currentAllocatorRegion_ != nullptr) {
        ASSERT(currentFreeListLength_ == 0);
        uintptr_t top = allocator_.GetTop();
        Region *currentRegion = currentAllocatorRegion_;
        if (!isPromoted) {
            if (currentRegion->HasAgeMark()) {
                allocateAfterLastGC_ +=
                    currentRegion->GetAllocatedBytes(top) - currentRegion->GetAllocatedBytes(waterLine_);
            } else {
                allocateAfterLastGC_ += currentRegion->GetAllocatedBytes(top);
            }
        } else {
            // For GC
            survivalObjectSize_ += currentRegion->GetAllocatedBytes(top);
        }
        currentRegion->SetHighWaterMark(top);
        currentAllocatorRegion_ = nullptr;
    } else if (currentFreeListLength_ != 0) {
        ASSERT(currentAllocatorRegion_ == nullptr);
        ASSERT(!isPromoted);
        size_t left = allocator_.Available();
        if (left != 0) {
            FreeObject::FillFreeObject(localHeap_, allocator_.GetTop(), left);
        }
        allocateAfterLastGC_ += currentFreeListLength_ - left;
        currentFreeListLength_ = 0;
    }
    ASSERT(currentAllocatorRegion_ == nullptr);
    ASSERT(currentFreeListLength_ == 0);
    allocator_.Reset();
}

void LinearSpace::Stop()
{
    DiscardCurrentAllocator(false);
    for (Region *region : freeListRegions_) {
        region->ClearGCFlag(RegionGCFlags::FREE_LIST_IN_YOUNG);
    }
    freeListRegions_.clear();
    freeList_.clear();
}

void LinearSpace::ResetAllocator()
{
    auto currentRegion = GetCurrentAllocatorRegion();
    if (currentRegion != nullptr) {
        allocator_.Reset(currentRegion->GetBegin(), currentRegion->GetEnd(), currentRegion->GetHighWaterMark());
    }
}

void LinearSpace::IterateOverObjects(const std::function<void(TaggedObject *object)> &visitor) const
{
    if (currentFreeListLength_ != 0) {
        if (allocator_.Available() != 0) {
            FreeObject::FillFreeObject(localHeap_, allocator_.GetTop(), allocator_.Available());
        }
    }
    auto current = GetCurrentAllocatorRegion();
    EnumerateRegions([&](Region *region) {
        auto curPtr = region->GetBegin();
        uintptr_t endPtr = 0;
        if (region == current) {
            auto top = allocator_.GetTop();
            endPtr = curPtr + region->GetAllocatedBytes(top);
        } else {
            endPtr = curPtr + region->GetAllocatedBytes();
        }

        size_t objSize;
        while (curPtr < endPtr) {
            auto freeObject = FreeObject::Cast(curPtr);
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

void LinearSpace::InvokeAllocationInspector(Address object, size_t size, size_t alignedSize)
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

Region *LinearSpace::GetSweepingRegionSafe()
{
    LockHolder lock(mutex_);
    if (!pendingSweepingRegions_.empty()) {
        Region *region = pendingSweepingRegions_.back();
        pendingSweepingRegions_.pop_back();
        return region;
    }
    return nullptr;
}

void LinearSpace::AddSweptRegionSafe(Region *region)
{
    LockHolder lock(mutex_);
    sweptRegions_.emplace_back(region);
    region->SetSwept();
}

Region *LinearSpace::GetSweptRegionSafe()
{
    LockHolder lock(mutex_);
    if (!sweptRegions_.empty()) {
        Region *region = sweptRegions_.back();
        sweptRegions_.pop_back();
        return region;
    }
    return nullptr;
}

void LinearSpace::PrepareSweeping(LinearSpace *fromSpace)
{
    ASSERT(GetSweepingRegionSafe() == nullptr);
    ASSERT(GetSweptRegionSafe() == nullptr);
    fromSpace->EnumerateRegions([this, fromSpace](Region *region) {
        fromSpace->RemoveRegion(region);
        region->ClearGCFlag(RegionGCFlags::HAS_AGE_MARK);

        if (UNLIKELY(localHeap_->ShouldVerifyHeap())) {
            region->ResetInactiveSemiSpace();
        }

        regionList_.AddNodeToFront(region);
        IncreaseCommitted(region->GetCapacity());
        // fixme: if region is already freelist?
        IncreaseObjectSize(region->GetSize());
        survivalObjectSize_ += region->AliveObject();

        ASSERT(!region->IsToRegion());
        ASSERT(!region->IsGCFlagSet(RegionGCFlags::HAS_BEEN_SWEPT));
        if (UNLIKELY(localHeap_->ShouldVerifyHeap() &&
            region->IsGCFlagSet(RegionGCFlags::HAS_BEEN_SWEPT))) {
            LOG_ECMA(FATAL) << "Region should not be swept before PrepareSweeping: " << region;
        }

        region->SwapLocalToShareRSetForCS();
        pendingSweepingRegions_.emplace_back(region);
    });

    // Sweep low alive object size at first
    std::sort(pendingSweepingRegions_.begin(), pendingSweepingRegions_.end(), [](Region *first, Region *second) {
        return first->AliveObject() > second->AliveObject();
    });
    size_t num = pendingSweepingRegions_.size();
    if (num > 0) {
        sweeping_.store(SweepingState::SWEEPING, std::memory_order_relaxed);
        numPendingSweepingRegions_.store(num, std::memory_order_relaxed);
    }
}

void LinearSpace::Sweep(LinearSpace *fromSpace)
{
    ASSERT(GetSweepingRegionSafe() == nullptr);
    ASSERT(GetSweptRegionSafe() == nullptr);
    fromSpace->EnumerateRegions([this, fromSpace](Region *region) {
        fromSpace->RemoveRegion(region);
        region->ClearGCFlag(RegionGCFlags::HAS_AGE_MARK);

        if (UNLIKELY(localHeap_->ShouldVerifyHeap())) {
            region->ResetInactiveSemiSpace();
        }

        regionList_.AddNodeToFront(region);
        IncreaseCommitted(region->GetCapacity());
        // fixme: if region is already freelist?
        IncreaseObjectSize(region->GetSize());
        survivalObjectSize_ += region->AliveObject();

        ASSERT(!region->IsToRegion());
        ASSERT(!region->IsGCFlagSet(RegionGCFlags::HAS_BEEN_SWEPT));
        if (UNLIKELY(localHeap_->ShouldVerifyHeap() &&
            region->IsGCFlagSet(RegionGCFlags::HAS_BEEN_SWEPT))) {
            LOG_ECMA(FATAL) << "Region should not be swept before PrepareSweeping: " << region;
        }

        SweepRegion(region, freeList_);
    });
    BuildFreeList();
}

bool LinearSpace::TryFillSweptRegion()
{
    std::vector<Region *> regions;
    {
        LockHolder lock(mutex_);
        freeList_.reserve(freeList_.size() + sweptFreeList_.size());
        freeList_.insert(freeList_.end(), sweptFreeList_.begin(), sweptFreeList_.end());
        sweptFreeList_.clear();
        std::swap(regions, sweptRegions_);
    }
    for (Region *region : regions) {
        region->ResetSwept();
        region->MergeLocalToShareRSetForCS();
        freeListRegions_.emplace_back(region);
    }
    BuildFreeList();
    return true;
}

bool LinearSpace::FinishFillSweptRegion()
{
    bool ret = TryFillSweptRegion();
    sweeping_.store(SweepingState::NO_SWEEP, std::memory_order_relaxed);
    return ret;
}

void LinearSpace::AsyncSweep([[maybe_unused]] bool isMain, [[maybe_unused]] bool releaseMemory)
{
    std::vector<FreeMemory> list;
    size_t cnt = 0;
    while (true) {
        Region *region = GetSweepingRegionSafe();
        if (region == nullptr) {
            break;
        }

        ++cnt;
        SweepRegion(region, list);
        AddSweptRegionSafe(region);
        {
            LockHolder lock(mutex_);
            sweptFreeList_.insert(sweptFreeList_.end(), list.begin(), list.end());
        }
        list.clear();
    }
    if (cnt > 0 && numPendingSweepingRegions_.fetch_sub(cnt, std::memory_order_relaxed) == cnt) {
        sweeping_.store(SweepingState::SWEPT, std::memory_order_relaxed);
        std::atomic_thread_fence(std::memory_order_seq_cst);
    }
}

void LinearSpace::SweepRegion(Region *region, std::vector<FreeMemory> &list)
{
    uintptr_t freeStart = region->GetBegin();
    uintptr_t freeEnd = freeStart + region->GetAllocatedBytes();
    region->IterateAllMarkedBits([this, region, &freeStart, &list](void *mem) {
        ASSERT(region->InRange(ToUintPtr(mem)));
        TaggedObject *header = reinterpret_cast<TaggedObject *>(mem);
        size_t size = header->GetSize();

        uintptr_t freeEnd = ToUintPtr(mem);
        if (freeStart != freeEnd) {
            FreeLiveRange(region, freeStart, freeEnd, list);
        }
        freeStart = freeEnd + size;
    });
    if (freeStart != freeEnd) {
        FreeLiveRange(region, freeStart, freeEnd, list);
    }
    region->SetGCFlag(RegionGCFlags::FREE_LIST_IN_YOUNG);
}

void LinearSpace::FreeLiveRange(Region *region, uintptr_t freeStart, uintptr_t freeEnd, std::vector<FreeMemory> &list)
{
    localHeap_->GetSweeper()->ClearRSetInRange(region, freeStart, freeEnd);
    size_t size = freeEnd - freeStart;
    FreeObject::FillFreeObject(localHeap_, freeStart, size);
    constexpr size_t FREE_MEMORY_MIN_SIZE = 4 * 1024;
    if (size >= FREE_MEMORY_MIN_SIZE) {
        list.emplace_back(size, freeStart);
    }
    MEMORY_TRACE_FREEREGION(freeStart, size);
}

void LinearSpace::BuildFreeList()
{
    std::sort(freeList_.begin(), freeList_.end(),
        [](const FreeMemory &a, const FreeMemory &b) {
            if (a.length != b.length) {
                return a.length < b.length;
            }
            return a.start > b.start;
        });
}

SemiSpace::SemiSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : LinearSpace(heap, MemSpaceType::SEMI_SPACE, initialCapacity, maximumCapacity),
      minimumCapacity_(initialCapacity) {}

void SemiSpace::Initialize()
{
    JSThread *thread = localHeap_->GetJSThread();
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, thread, localHeap_);
    AddRegion(region);
    allocator_.Reset(region->GetBegin(), region->GetEnd());
    currentAllocatorRegion_ = region;
}

void SemiSpace::Restart(size_t overShootSize)
{
    overShootSize_ = overShootSize;
    overShootSizeForConcurrentMark_ = 0;
    survivalObjectSize_ = 0;
    allocateAfterLastGC_ = 0;
    Initialize();
}

size_t SemiSpace::CalculateNewOverShootSize()
{
    return committedSize_ <= maximumCapacity_ ?
           0 : AlignUp(static_cast<size_t>((committedSize_ - maximumCapacity_) * HPPGC_NEWSPACE_SIZE_RATIO),
                       DEFAULT_REGION_SIZE);
}

bool SemiSpace::CommittedSizeIsLarge()
{
    return committedSize_ >= maximumCapacity_ * 2; // 2 means double.
}

uintptr_t SemiSpace::AllocateSync(size_t size)
{
    LockHolder lock(lock_);
    return Allocate(size, true);
}

bool SemiSpace::SwapRegion(Region *region, SemiSpace *fromSpace)
{
    if (committedSize_ + region->GetCapacity() > maximumCapacity_ + overShootSize_) {
        return false;
    }
    fromSpace->RemoveRegion(region);

    region->SetGCFlag(RegionGCFlags::IN_NEW_TO_NEW_SET);

    if (UNLIKELY(heap_->ShouldVerifyHeap())) { // LCOV_EXCL_BR_LINE
        region->ResetInactiveSemiSpace();
    }

    regionList_.AddNodeToFront(region);
    IncreaseCommitted(region->GetCapacity());
    IncreaseObjectSize(region->GetSize());
    survivalObjectSize_ += region->GetAllocatedBytes();
    return true;
}

void SemiSpace::SetWaterLine(bool cmsGC)
{
    waterLine_ = 0;
    allocateAfterLastGC_ = 0;
    Region *last = GetCurrentAllocatorRegion();
    EnumerateRegions([&last, cmsGC](Region *current) {
        if (cmsGC) {
            current->ClearGCFlag(RegionGCFlags::BELOW_AGE_MARK);
            current->ClearGCFlag(RegionGCFlags::HAS_AGE_MARK);
        } else {
            if (current != last) {
                current->SetGCFlag(RegionGCFlags::BELOW_AGE_MARK);
            }
        }
    });
    if (last != nullptr) {
        last->SetGCFlag(RegionGCFlags::HAS_AGE_MARK);

        waterLine_ = allocator_.GetTop();
        survivalObjectSize_ += last->GetAllocatedBytes(waterLine_);
    } else {
        LOG_GC(INFO) << "SetWaterLine: No region survival in current gc, current region available size: "
                     << allocator_.Available();
    }
}

size_t SemiSpace::GetHeapObjectSize() const
{
    return survivalObjectSize_ + allocateAfterLastGC_;
}

size_t SemiSpace::GetSurvivalObjectSize() const
{
    return survivalObjectSize_;
}

void SemiSpace::SetOverShootSize(size_t size)
{
    overShootSize_ = size;
}

void SemiSpace::AddOverShootSize(size_t size)
{
    overShootSize_ += size;
}

bool SemiSpace::AdjustCapacity(size_t allocatedSizeSinceGC, JSThread *thread)
{
    if (allocatedSizeSinceGC <= initialCapacity_ * GROW_OBJECT_SURVIVAL_RATE / GROWING_FACTOR) {
        return false;
    }
    size_t committedSize = GetCommittedSize();
    double curObjectSurvivalRate = static_cast<double>(survivalObjectSize_) / allocatedSizeSinceGC;
    double committedSurvivalRate = static_cast<double>(committedSize) / initialCapacity_;
    SetOverShootSize(0);
    double allocSpeed = localHeap_->GetMemController()->GetNewSpaceAllocationThroughputPerMS();
    if (curObjectSurvivalRate > GROW_OBJECT_SURVIVAL_RATE || committedSurvivalRate > GROW_OBJECT_SURVIVAL_RATE) {
        size_t newCapacity = initialCapacity_ * GROWING_FACTOR;
        while (committedSize >= newCapacity && newCapacity < maximumCapacity_) {
            newCapacity = newCapacity * GROWING_FACTOR;
        }
        SetInitialCapacity(std::min(newCapacity, maximumCapacity_));
        if (committedSize >= initialCapacity_ * GROW_OBJECT_SURVIVAL_RATE) {
            // Overshoot size is too large. Avoid heapObjectSize is too close to committed size.
            SetOverShootSize(committedSize);
        }
        if (newCapacity == maximumCapacity_) {
            localHeap_->GetJSObjectResizingStrategy()->UpdateGrowStep(
                thread,
                JSObjectResizingStrategy::PROPERTIES_GROW_SIZE * 2);   // 2: double
        }
        return true;
    } else if (initialCapacity_ < (MIN_GC_INTERVAL_MS * allocSpeed) &&
        initialCapacity_ < maximumCapacity_) {
        size_t newCapacity = initialCapacity_ * GROWING_FACTOR;
        SetInitialCapacity(std::min(newCapacity, maximumCapacity_));
        if (newCapacity == maximumCapacity_) {
            localHeap_->GetJSObjectResizingStrategy()->UpdateGrowStep(
                thread,
                JSObjectResizingStrategy::PROPERTIES_GROW_SIZE * 2);   // 2: double
        }
        return true;
    } else if (curObjectSurvivalRate < SHRINK_OBJECT_SURVIVAL_RATE) {
        if (initialCapacity_ <= minimumCapacity_) {
            return false;
        }
        if (allocSpeed > LOW_ALLOCATION_SPEED_PER_MS) {
            return false;
        }
        size_t newCapacity = initialCapacity_ / GROWING_FACTOR;
        SetInitialCapacity(std::max(newCapacity, minimumCapacity_));
        localHeap_->GetJSObjectResizingStrategy()->UpdateGrowStep(thread);
        return true;
    }
    return false;
}

size_t SemiSpace::GetAllocatedSizeSinceGC(uintptr_t top) const
{
    size_t currentRegionSize = 0;
    auto currentRegion = GetCurrentAllocatorRegion();
    if (currentRegion != nullptr) {
        currentRegionSize = currentRegion->GetAllocatedBytes(top);
        if (currentRegion->HasAgeMark()) {
            currentRegionSize -= currentRegion->GetAllocatedBytes(waterLine_);
        }
    }
    return allocateAfterLastGC_ + currentRegionSize;
}

SnapshotSpace::SnapshotSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : LinearSpace(heap, MemSpaceType::SNAPSHOT_SPACE, initialCapacity, maximumCapacity) {}

ReadOnlySpace::ReadOnlySpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity, MemSpaceType type)
    : LinearSpace(heap, type, initialCapacity, maximumCapacity) {}
}  // namespace panda::ecmascript
