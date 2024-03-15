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

void SharedSparseSpace::Reset()
{
    allocator_->RebuildFreeList();
    ReclaimRegions();
    liveObjectSize_ = 0;
}

void SharedSparseSpace::ResetTopPointer(uintptr_t top)
{
    allocator_->ResetTopPointer(top);
}

// only used in share heap initialize before first vmThread created.
uintptr_t SharedSparseSpace::AllocateWithoutGC(size_t size)
{
    uintptr_t object = TryAllocate(size);
    CHECK_SOBJECT_AND_INC_OBJ_SIZE(size);
    object = AllocateWithExpand(nullptr, size);
    CHECK_SOBJECT_AND_INC_OBJ_SIZE(size);
    return object;
}

uintptr_t SharedSparseSpace::Allocate(JSThread *thread, size_t size, bool allowGC)
{
    ASSERT(thread->IsInRunningState());
    uintptr_t object = TryAllocate(size);
    CHECK_SOBJECT_AND_INC_OBJ_SIZE(size);
    if (sweepState_ == SweepState::SWEEPING) {
        object = AllocateAfterSweepingCompleted(size);
        CHECK_SOBJECT_AND_INC_OBJ_SIZE(size);
    }
    // Check whether it is necessary to trigger Shared GC before expanding to avoid OOM risk.
    if (allowGC && sHeap_->CheckAndTriggerGC(thread)) {
        object = TryAllocate(size);
        CHECK_SOBJECT_AND_INC_OBJ_SIZE(size);
    }
    object = AllocateWithExpand(thread, size);
    CHECK_SOBJECT_AND_INC_OBJ_SIZE(size);
    if (allowGC) {
        sHeap_->CollectGarbage(thread, TriggerGCType::SHARED_GC, GCReason::ALLOCATION_FAILED);
        object = Allocate(thread, size, false);
    }
    return object;
}

uintptr_t SharedSparseSpace::AllocateNoGCAndExpand([[maybe_unused]] JSThread *thread, size_t size)
{
    ASSERT(thread->IsInRunningState());
    uintptr_t object = TryAllocate(size);
    CHECK_SOBJECT_AND_INC_OBJ_SIZE(size);
    if (sweepState_ == SweepState::SWEEPING) {
        object = AllocateAfterSweepingCompleted(size);
        CHECK_SOBJECT_AND_INC_OBJ_SIZE(size);
    }
    return object;
}

uintptr_t SharedSparseSpace::TryAllocate(size_t size)
{
    LockHolder lock(allocateLock_);
    return allocator_->Allocate(size);
}

uintptr_t SharedSparseSpace::AllocateWithExpand(JSThread *thread, size_t size)
{
    LockHolder lock(allocateLock_);
    // In order to avoid expand twice by different threads, try allocate first.
    auto object = allocator_->Allocate(size);
    if (object == 0 && Expand(thread)) {
        object = allocator_->Allocate(size);
    }
    return object;
}

bool SharedSparseSpace::Expand(JSThread *thread)
{
    if (CommittedSizeExceed()) {
        LOG_ECMA_MEM(INFO) << "Expand::Committed size " << committedSize_ << " of Sparse Space is too big. ";
        return false;
    }
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, thread, sHeap_);
    region->InitializeFreeObjectSets();
    AddRegion(region);
    allocator_->AddFree(region);
    return true;
}

Region *SharedSparseSpace::AllocateDeserializeRegion(JSThread *thread)
{
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, thread, sHeap_);
    region->InitializeFreeObjectSets();
    return region;
}

void SharedSparseSpace::MergeDeserializeAllocateRegions(const std::vector<Region *> &allocateRegions)
{
    LockHolder lock(allocateLock_);
    for (auto region : allocateRegions) {
        AddRegion(region);
        allocator_->AddFree(region);
        allocator_->ResetTopPointer(region->GetHighWaterMark());
        region->SetHighWaterMark(region->GetEnd());
    }
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

void SharedSparseSpace::IterateOverObjects(const std::function<void(TaggedObject *object)> &visitor) const
{
    allocator_->FillBumpPointer();
    EnumerateRegions([&](Region *region) {
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

bool SharedReadOnlySpace::Expand(JSThread *thread)
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
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, thread, heap_);
    allocator_.Reset(region->GetBegin(), region->GetEnd());
    AddRegion(region);
    return true;
}

uintptr_t SharedReadOnlySpace::Allocate(JSThread *thread, size_t size)
{
    ASSERT(thread->IsInRunningState());
    LockHolder holder(allocateLock_);
    auto object = allocator_.Allocate(size);
    if (object != 0) {
        return object;
    }
    if (Expand(thread)) {
        object = allocator_.Allocate(size);
    }
    return object;
}

SharedHugeObjectSpace::SharedHugeObjectSpace(BaseHeap *heap, HeapRegionAllocator *heapRegionAllocator,
                                             size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, heapRegionAllocator, MemSpaceType::SHARED_HUGE_OBJECT_SPACE, initialCapacity, maximumCapacity)
{
}


uintptr_t SharedHugeObjectSpace::Allocate(JSThread *thread, size_t objectSize)
{
    ASSERT(thread->IsInRunningState());
    LockHolder lock(allocateLock_);
    // In HugeObject allocation, we have a revervation of 8 bytes for markBitSet in objectSize.
    // In case Region is not aligned by 16 bytes, HUGE_OBJECT_BITSET_SIZE is 8 bytes more.
    size_t alignedSize = AlignUp(objectSize + sizeof(Region) + HUGE_OBJECT_BITSET_SIZE, PANDA_POOL_ALIGNMENT_IN_BYTES);
    if (CommittedSizeExceed(alignedSize)) {
        LOG_ECMA_MEM(INFO) << "Committed size " << committedSize_ << " of huge object space is too big.";
        return 0;
    }
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, alignedSize, thread, heap_);
    AddRegion(region);
    // It need to mark unpoison when huge object being allocated.
    ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<void *>(region->GetBegin()), objectSize);
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
    InvokeAllocationInspector(region->GetBegin(), objectSize);
#endif
    return region->GetBegin();
}

void SharedHugeObjectSpace::Sweep()
{
    Region *currentRegion = GetRegionList().GetFirst();
    while (currentRegion != nullptr) {
        Region *next = currentRegion->GetNext();
        bool isMarked = false;
        currentRegion->IterateAllMarkedBits([&isMarked]([[maybe_unused]] void *mem) { isMarked = true; });
        if (!isMarked) {
            GetRegionList().RemoveNode(currentRegion);
            hugeNeedFreeList_.AddNode(currentRegion);
        }
        currentRegion = next;
    }
}

size_t SharedHugeObjectSpace::GetHeapObjectSize() const
{
    return committedSize_;
}

void SharedHugeObjectSpace::IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const
{
    EnumerateRegions([&](Region *region) {
        uintptr_t curPtr = region->GetBegin();
        objectVisitor(reinterpret_cast<TaggedObject *>(curPtr));
    });
}

void SharedHugeObjectSpace::ReclaimHugeRegion()
{
    if (hugeNeedFreeList_.IsEmpty()) {
        return;
    }
    do {
        Region *last = hugeNeedFreeList_.PopBack();
        ClearAndFreeRegion(last);
    } while (!hugeNeedFreeList_.IsEmpty());
}

void SharedHugeObjectSpace::InvokeAllocationInspector(Address object, size_t objectSize)
{
    if (LIKELY(!allocationCounter_.IsActive())) {
        return;
    }
    if (objectSize >= allocationCounter_.NextBytes()) {
        allocationCounter_.InvokeAllocationInspector(object, objectSize, objectSize);
    }
    allocationCounter_.AdvanceAllocationInspector(objectSize);
}
}  // namespace panda::ecmascript