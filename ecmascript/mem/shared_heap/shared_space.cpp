/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/shared_heap/shared_space.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/allocator-inl.h"
#include "ecmascript/mem/free_object_set.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"

namespace panda::ecmascript {
SharedSparseSpace::SharedSparseSpace(SharedHeap *heap, MemSpaceType type, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, heap->GetHeapRegionAllocator(), type, initialCapacity, maximumCapacity),
      sweepState_(SweepState::NO_SWEEP),
      sHeap_(heap),
      liveObjectSize_(0)
{
    allocator_ = new FreeListAllocator(heap);
}

void SharedSparseSpace::Initialize()
{
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, sHeap_);
    region->InitializeFreeObjectSets();
    AddRegion(region);

    allocator_->Initialize(region);
}

void SharedSparseSpace::Reset()
{
    allocator_->RebuildFreeList();
    ReclaimRegions();
    liveObjectSize_ = 0;
}

// only used in share heap initialize before first vmThread created.
uintptr_t SharedSparseSpace::AllocateWithoutGC(size_t size)
{
    uintptr_t object = TryAllocate(size);
    CHECK_SOBJECT_AND_INC_OBJ_SIZE(size);
    object = AllocateWithExpand(size);
    CHECK_SOBJECT_AND_INC_OBJ_SIZE(size);
    return object;
}

uintptr_t SharedSparseSpace::Allocate(JSThread *thread, size_t size, bool allowGC)
{
    uintptr_t object = TryAllocate(size);
    CHECK_SOBJECT_AND_INC_OBJ_SIZE(size);
    if (sweepState_ == SweepState::SWEEPING) {
        object = AllocateAfterSweepingCompleted(size);
        CHECK_SOBJECT_AND_INC_OBJ_SIZE(size);
    }
     // Check whether it is necessary to trigger Old GC before expanding to avoid OOM risk.
    if (allowGC && sHeap_->CheckAndTriggerOldGC(thread)) {
        object = TryAllocate(size);
        CHECK_SOBJECT_AND_INC_OBJ_SIZE(size);
    }
    object = AllocateWithExpand(size);
    CHECK_SOBJECT_AND_INC_OBJ_SIZE(size);
    if (allowGC) {
        sHeap_->CollectGarbage(thread, TriggerGCType::SHARED_GC, GCReason::ALLOCATION_FAILED);
        object = Allocate(thread, size, false);
    }
    return object;
}

uintptr_t SharedSparseSpace::TryAllocate(size_t size)
{
    LockHolder lock(allocateLock_);
    return allocator_->Allocate(size);
}

uintptr_t SharedSparseSpace::AllocateWithExpand(size_t size)
{
    LockHolder lock(allocateLock_);
    // In order to avoid expand twice by different threads, try allocate first.
    auto object = allocator_->Allocate(size);
    if (Expand()) {
        object = allocator_->Allocate(size);
    }
    return object;
}

bool SharedSparseSpace::Expand()
{
    if (committedSize_ >= maximumCapacity_ + outOfMemoryOvershootSize_) {
        LOG_ECMA_MEM(INFO) << "Expand::Committed size " << committedSize_ << " of Sparse Space is too big. ";
        return false;
    }
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, sHeap_);
    region->InitializeFreeObjectSets();
    AddRegion(region);
    allocator_->AddFree(region);
    return true;
}

uintptr_t SharedSparseSpace::AllocateAfterSweepingCompleted(size_t size)
{
    LockHolder lock(allocateLock_);
    if (sweepState_ != SweepState::SWEEPING) {
        return allocator_->Allocate(size);
    }
    if (TryFillSweptRegion()) {
        auto object = allocator_->Allocate(size);
        if (object != 0) {
            return object;
        }
    }
    // Parallel sweep and fill
    sHeap_->GetSweeper()->EnsureTaskFinished(spaceType_);
    return allocator_->Allocate(size);
}

void SharedSparseSpace::PrepareSweeping()
{
    liveObjectSize_ = 0;
    EnumerateRegions([this](Region *current) {
        IncreaseLiveObjectSize(current->AliveObject());
        current->ResetWasted();
        AddSweepingRegion(current);
    });
    SortSweepingRegion();
    sweepState_ = SweepState::SWEEPING;
    allocator_->RebuildFreeList();
}

void SharedSparseSpace::AsyncSweep(bool isMain)
{
    Region *current = GetSweepingRegionSafe();
    while (current != nullptr) {
        FreeRegion(current, isMain);
        // Main thread sweeping region is added;
        if (!isMain) {
            AddSweptRegionSafe(current);
        }
        current = GetSweepingRegionSafe();
    }
}

void SharedSparseSpace::Sweep()
{
    liveObjectSize_ = 0;
    allocator_->RebuildFreeList();
    EnumerateRegions([this](Region *current) {
        IncreaseLiveObjectSize(current->AliveObject());
        current->ResetWasted();
        FreeRegion(current);
    });
}

bool SharedSparseSpace::TryFillSweptRegion()
{
    if (sweptList_.empty()) {
        return false;
    }
    Region *region = nullptr;
    while ((region = GetSweptRegionSafe()) != nullptr) {
        allocator_->CollectFreeObjectSet(region);
        region->ResetSwept();
    }
    return true;
}

bool SharedSparseSpace::FinishFillSweptRegion()
{
    bool ret = TryFillSweptRegion();
    sweepState_ = SweepState::SWEPT;
    return ret;
}

void SharedSparseSpace::AddSweepingRegion(Region *region)
{
    sweepingList_.emplace_back(region);
}

void SharedSparseSpace::SortSweepingRegion()
{
    // Sweep low alive object size at first
    std::sort(sweepingList_.begin(), sweepingList_.end(), [](Region *first, Region *second) {
        return first->AliveObject() < second->AliveObject();
    });
}

Region *SharedSparseSpace::GetSweepingRegionSafe()
{
    LockHolder holder(lock_);
    Region *region = nullptr;
    if (!sweepingList_.empty()) {
        region = sweepingList_.back();
        sweepingList_.pop_back();
    }
    return region;
}

void SharedSparseSpace::AddSweptRegionSafe(Region *region)
{
    LockHolder holder(lock_);
    sweptList_.emplace_back(region);
}

Region *SharedSparseSpace::GetSweptRegionSafe()
{
    LockHolder holder(lock_);
    Region *region = nullptr;
    if (!sweptList_.empty()) {
        region = sweptList_.back();
        sweptList_.pop_back();
    }
    return region;
}

void SharedSparseSpace::FreeRegion(Region *current, bool isMain)
{
    uintptr_t freeStart = current->GetBegin();
    current->IterateAllMarkedBits([this, &freeStart, isMain](void *mem) {
        auto header = reinterpret_cast<TaggedObject *>(mem);
        auto klass = header->GetClass();
        auto size = klass->SizeFromJSHClass(header);

        uintptr_t freeEnd = ToUintPtr(mem);
        if (freeStart != freeEnd) {
            FreeLiveRange(freeStart, freeEnd, isMain);
        }
        freeStart = freeEnd + size;
    });
    uintptr_t freeEnd = current->GetEnd();
    if (freeStart != freeEnd) {
        FreeLiveRange(freeStart, freeEnd, isMain);
    }
}

void SharedSparseSpace::FreeLiveRange(uintptr_t freeStart, uintptr_t freeEnd, bool isMain)
{
    // No need to clear rememberset here, because shared region has no remember set now.
    allocator_->Free(freeStart, freeEnd - freeStart, isMain);
}

size_t SharedSparseSpace::GetHeapObjectSize() const
{
    return liveObjectSize_;
}

void SharedSparseSpace::IncreaseAllocatedSize(size_t size)
{
    allocator_->IncreaseAllocatedSize(size);
}

size_t SharedSparseSpace::GetTotalAllocatedSize() const
{
    return allocator_->GetAllocatedSize();
}

void SharedSparseSpace::InvokeAllocationInspector(Address object, size_t size, size_t alignedSize)
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


SharedNonMovableSpace::SharedNonMovableSpace(SharedHeap *heap, size_t initialCapacity, size_t maximumCapacity)
    : SharedSparseSpace(heap, MemSpaceType::SHARED_NON_MOVABLE, initialCapacity, maximumCapacity)
{
}

SharedOldSpace::SharedOldSpace(SharedHeap *heap, size_t initialCapacity, size_t maximumCapacity)
    : SharedSparseSpace(heap, MemSpaceType::SHARED_OLD_SPACE, initialCapacity, maximumCapacity)
{
}

SharedReadOnlySpace::SharedReadOnlySpace(SharedHeap *heap, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, heap->GetHeapRegionAllocator(), MemSpaceType::SHARED_READ_ONLY_SPACE, initialCapacity, maximumCapacity) {}

bool SharedReadOnlySpace::Expand()
{
    if (committedSize_ >= initialCapacity_ + outOfMemoryOvershootSize_ &&
        !heap_->NeedStopCollection()) {
        return false;
    }
    uintptr_t top = allocator_.GetTop();
    auto currentRegion = GetCurrentRegion();
    if (currentRegion != nullptr) {
        currentRegion->SetHighWaterMark(top);
    }
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, heap_);
    allocator_.Reset(region->GetBegin(), region->GetEnd());
    AddRegion(region);
    return true;
}

uintptr_t SharedReadOnlySpace::Allocate([[maybe_unused]]JSThread *thread, size_t size)
{
    LockHolder holder(allocateLock_);
    auto object = allocator_.Allocate(size);
    if (object != 0) {
        return object;
    }
    if (Expand()) {
        object = allocator_.Allocate(size);
    }
    return object;
}
}  // namespace panda::ecmascript