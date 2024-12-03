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
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/checkpoint/thread_state_transition.h"

namespace panda::ecmascript {
SharedSparseSpace::SharedSparseSpace(SharedHeap *heap,
                                     MemSpaceType type,
                                     size_t initialCapacity,
                                     size_t maximumCapacity)
    : Space(heap, heap->GetHeapRegionAllocator(), type, initialCapacity, maximumCapacity),
      sweepState_(SweepState::NO_SWEEP),
      sHeap_(heap),
      liveObjectSize_(0)
{
    triggerLocalFullMarkLimit_ = maximumCapacity * LIVE_OBJECT_SIZE_RATIO;
    allocator_ = new FreeListAllocator<FreeObject>(heap);
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
uintptr_t SharedSparseSpace::AllocateWithoutGC(JSThread *thread, size_t size)
{
    uintptr_t object = TryAllocate(thread, size);
    CHECK_SOBJECT_NOT_NULL();
    object = AllocateWithExpand(thread, size);
    return object;
}

uintptr_t SharedSparseSpace::Allocate(JSThread *thread, size_t size, bool allowGC)
{
#if ECMASCRIPT_ENABLE_THREAD_STATE_CHECK
    if (UNLIKELY(!thread->IsInRunningStateOrProfiling())) { // LOCV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "Allocate must be in jsthread running state";
        UNREACHABLE();
    }
#endif
    // Shared old space cannot use this allocate func. Shared full gc may happen in trigger and thread state update.
    // Shared old space pointer might change by shared full gc.
    // jit thread no heap
    allowGC = allowGC && (!thread->IsJitThread());
    if (allowGC) {
        auto localHeap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
        localHeap->TryTriggerFullMarkBySharedSize(size);
    }
    uintptr_t object = TryAllocate(thread, size);
    CHECK_SOBJECT_NOT_NULL();
    if (sweepState_ == SweepState::SWEEPING) {
        object = AllocateAfterSweepingCompleted(thread, size);
        CHECK_SOBJECT_NOT_NULL();
    }
    // Check whether it is necessary to trigger Shared GC before expanding to avoid OOM risk.
    if (allowGC && sHeap_->CheckAndTriggerSharedGC(thread)) {
        object = TryAllocate(thread, size);
        CHECK_SOBJECT_NOT_NULL();
    }
    object = AllocateWithExpand(thread, size);
    CHECK_SOBJECT_NOT_NULL();
    if (allowGC) {
        sHeap_->CollectGarbage<TriggerGCType::SHARED_GC, GCReason::ALLOCATION_FAILED>(thread);
        object = Allocate(thread, size, false);
    }
    return object;
}

uintptr_t SharedSparseSpace::TryAllocateAndExpand(JSThread *thread, size_t size, bool expand)
{
    uintptr_t object = TryAllocate(thread, size);
    CHECK_SOBJECT_NOT_NULL();
    if (sweepState_ == SweepState::SWEEPING) {
        object = AllocateAfterSweepingCompleted(thread, size);
        CHECK_SOBJECT_NOT_NULL();
    }
    if (expand) {
        object = AllocateWithExpand(thread, size);
    }
    return object;
}

uintptr_t SharedSparseSpace::AllocateNoGCAndExpand(JSThread *thread, size_t size)
{
#if ECMASCRIPT_ENABLE_THREAD_STATE_CHECK
    if (UNLIKELY(!thread->IsInRunningStateOrProfiling())) { // LOCV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "Allocate must be in jsthread running state";
        UNREACHABLE();
    }
#endif
    uintptr_t object = TryAllocate(thread, size);
    CHECK_SOBJECT_NOT_NULL();
    if (sweepState_ == SweepState::SWEEPING) {
        object = AllocateAfterSweepingCompleted(thread, size);
    }
    return object;
}

uintptr_t SharedSparseSpace::TryAllocate([[maybe_unused]] JSThread *thread, size_t size)
{
    LockHolder lock(allocateLock_);
    uintptr_t object = allocator_->Allocate(size);
    IncAllocSObjectSize(object, size);
    return object;
}

uintptr_t SharedSparseSpace::AllocateWithExpand(JSThread *thread, size_t size)
{
    LockHolder lock(allocateLock_);
    // In order to avoid expand twice by different threads, try allocate first.
    CheckAndTriggerLocalFullMark();
    auto object = allocator_->Allocate(size);
    if (object == 0 && Expand(thread)) {
        object = allocator_->Allocate(size);
    }
    IncAllocSObjectSize(object, size);
    return object;
}

bool SharedSparseSpace::Expand(JSThread *thread)
{
    if (CommittedSizeExceed()) {
        LOG_ECMA_MEM(INFO) << "Expand::Committed size " << committedSize_ << " of Sparse Space is too big. ";
        return false;
    }
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, thread, sHeap_);
    if (region == nullptr) { // LOCV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "SharedSparseSpace::Expand:region is nullptr";
    }
    AddRegion(region);
    allocator_->AddFree(region);
    return true;
}

Region *SharedSparseSpace::AllocateDeserializeRegion(JSThread *thread)
{
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, thread, sHeap_);
    if (region == nullptr) { // LOCV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "SharedSparseSpace::AllocateDeserializeRegion:region is nullptr";
    }
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

uintptr_t SharedSparseSpace::AllocateAfterSweepingCompleted([[maybe_unused]] JSThread *thread, size_t size)
{
    LockHolder lock(allocateLock_);
    uintptr_t object = 0U;
    if (sweepState_ != SweepState::SWEEPING) {
        object = allocator_->Allocate(size);
        IncAllocSObjectSize(object, size);
        return object;
    }
    if (TryFillSweptRegion()) {
        object = allocator_->Allocate(size);
        IncAllocSObjectSize(object, size);
        if (object != 0) {
            return object;
        }
    }
    // Parallel sweep and fill
    sHeap_->GetSweeper()->EnsureTaskFinished(spaceType_);
    object = allocator_->Allocate(size);
    IncAllocSObjectSize(object, size);
    return object;
}

void SharedSparseSpace::PrepareSweeping()
{
    liveObjectSize_ = 0;
    ASSERT(GetSweepingRegionSafe() == nullptr);
    ASSERT(GetSweptRegionSafe() == nullptr);
    EnumerateRegions([this](Region *current) {
        ASSERT(!current->IsGCFlagSet(RegionGCFlags::HAS_BEEN_SWEPT));
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

void SharedSparseSpace::DetachFreeObjectSet(Region *region)
{
    allocator_->DetachFreeObjectSet(region);
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

void SharedSparseSpace::CheckAndTriggerLocalFullMark()
{
    if (liveObjectSize_ >= triggerLocalFullMarkLimit_) {
        sHeap_->TryTriggerLocalConcurrentMarking();
    }
}

void SharedSparseSpace::IncAllocSObjectSize(uintptr_t object, size_t size)
{
    if (object != 0) {
        IncreaseLiveObjectSize(size);
        if (sHeap_->IsReadyToConcurrentMark()) {
            Region::ObjectAddressToRange(object)->IncreaseAliveObject(size);
        }
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
        InvokeAllocationInspector(object, size, size);
#endif
    }
}

SharedAppSpawnSpace::SharedAppSpawnSpace(SharedHeap *heap, size_t initialCapacity)
    : SharedSparseSpace(heap, MemSpaceType::SHARED_APPSPAWN_SPACE, initialCapacity, initialCapacity)
{
}

void SharedAppSpawnSpace::IterateOverMarkedObjects(const std::function<void(TaggedObject *object)> &visitor) const
{
    EnumerateRegions([&](Region *current) {
        current->IterateAllMarkedBits([&](void *mem) {
            ASSERT(current->InRange(ToUintPtr(mem)));
            visitor(reinterpret_cast<TaggedObject *>(mem));
        });
    });
}

SharedNonMovableSpace::SharedNonMovableSpace(SharedHeap *heap, size_t initialCapacity, size_t maximumCapacity)
    : SharedSparseSpace(heap, MemSpaceType::SHARED_NON_MOVABLE, initialCapacity, maximumCapacity)
{
}

SharedOldSpace::SharedOldSpace(SharedHeap *heap, size_t initialCapacity, size_t maximumCapacity)
    : SharedSparseSpace(heap, MemSpaceType::SHARED_OLD_SPACE, initialCapacity, maximumCapacity)
{
}

void SharedOldSpace::Merge(SharedLocalSpace *localSpace)
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
    size_t hugeSpaceCommitSize = sHeap_->GetHugeObjectSpace()->GetCommittedSize();
    if (committedSize_ + hugeSpaceCommitSize > GetOverShootMaximumCapacity()) {
        LOG_ECMA_MEM(ERROR) << "Merge::Committed size " << committedSize_ << " of old space is too big. ";
        if (sHeap_->CanThrowOOMError()) {
            sHeap_->ShouldThrowOOMError(true);
        }
        IncreaseMergeSize(committedSize_ - oldCommittedSize);
        // if throw OOM, temporarily increase space size to avoid vm crash
        IncreaseOutOfMemoryOvershootSize(committedSize_ + hugeSpaceCommitSize - GetOverShootMaximumCapacity());
    }

    localSpace->GetRegionList().Clear();
    allocator_->IncreaseAllocatedSize(localSpace->GetTotalAllocatedSize());
}

SharedLocalSpace::SharedLocalSpace(SharedHeap *heap, size_t initialCapacity, size_t maximumCapacity)
    : SharedSparseSpace(heap, MemSpaceType::SHARED_LOCAL_SPACE, initialCapacity, maximumCapacity) {}

bool SharedLocalSpace::AddRegionToList(Region *region)
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

void SharedLocalSpace::FreeBumpPoint()
{
    allocator_->FreeBumpPoint();
}

void SharedLocalSpace::Stop()
{
    Region *currentRegion = GetCurrentRegion();
    if (currentRegion != nullptr) {
        currentRegion->SetHighWaterMark(currentRegion->GetBegin() + currentRegion->AliveObject());
    }
}

void SharedLocalSpace::ForceExpandInSharedGC(JSThread *thread)
{
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, thread, sHeap_);
    AddRegion(region);
    allocator_->AddFree(region);
}

uintptr_t SharedLocalSpace::Allocate(size_t size, bool isExpand)
{
    auto object = allocator_->Allocate(size);
    if (object == 0 && isExpand) {
        // Shared Full GC will compress all regions and cannot recognize all threads' region.
        if (!Expand(Runtime::GetInstance()->GetMainThread())) {
            ForceExpandInSharedGC(Runtime::GetInstance()->GetMainThread());
            sHeap_->ShouldThrowOOMError(true);
        }
        object = allocator_->Allocate(size);
        ASSERT(object != 0);
    }
    if (object != 0) {
        Region::ObjectAddressToRange(object)->IncreaseAliveObject(size);
    }
    return object;
}

SharedReadOnlySpace::SharedReadOnlySpace(SharedHeap *heap, size_t initialCapacity, size_t maximumCapacity)
    : Space(
          heap, heap->GetHeapRegionAllocator(), MemSpaceType::SHARED_READ_ONLY_SPACE, initialCapacity, maximumCapacity)
{
}

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
    if (region == nullptr) { // LOCV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "SharedReadOnlySpace::Expand:region is nullptr";
    }
    allocator_.Reset(region->GetBegin(), region->GetEnd());
    AddRegion(region);
    return true;
}

uintptr_t SharedReadOnlySpace::Allocate(JSThread *thread, size_t size)
{
#if ECMASCRIPT_ENABLE_THREAD_STATE_CHECK
    if (UNLIKELY(!thread->IsInRunningStateOrProfiling())) { // LOCV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "Allocate must be in jsthread running state";
        UNREACHABLE();
    }
#endif
    thread->CheckSafepointIfSuspended();
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

void SharedReadOnlySpace::IterateOverObjects(const std::function<void(TaggedObject *object)> &visitor) const
{
    size_t size = allocator_.Available();
    if (size != 0) {
        FreeObject::FillFreeObject(heap_, allocator_.GetTop(), size);
    }
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

SharedHugeObjectSpace::SharedHugeObjectSpace(BaseHeap *heap, HeapRegionAllocator *heapRegionAllocator,
                                             size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, heapRegionAllocator, MemSpaceType::SHARED_HUGE_OBJECT_SPACE, initialCapacity, maximumCapacity)
{
    triggerLocalFullMarkLimit_ = maximumCapacity * HUGE_OBJECT_SIZE_RATIO;
}


uintptr_t SharedHugeObjectSpace::Allocate(JSThread *thread, size_t objectSize, AllocateEventType allocType)
{
#if ECMASCRIPT_ENABLE_THREAD_STATE_CHECK
    if (UNLIKELY(!thread->IsInRunningStateOrProfiling())) { // LOCV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "Allocate must be in jsthread running state";
        UNREACHABLE();
    }
#endif
    // In HugeObject allocation, we have a revervation of 8 bytes for markBitSet in objectSize.
    // In case Region is not aligned by 16 bytes, HUGE_OBJECT_BITSET_SIZE is 8 bytes more.
    size_t alignedSize = AlignUp(objectSize + sizeof(Region) + HUGE_OBJECT_BITSET_SIZE, PANDA_POOL_ALIGNMENT_IN_BYTES);
    if (allocType == AllocateEventType::NORMAL) {
        thread->CheckSafepointIfSuspended();
        CheckAndTriggerLocalFullMark(thread, alignedSize);
    }
    LockHolder lock(allocateLock_);
    if (CommittedSizeExceed(alignedSize)) {
        LOG_ECMA_MEM(INFO) << "Committed size " << committedSize_ << " of huge object space is too big.";
        return 0;
    }
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, alignedSize, thread, heap_);
    if (region == nullptr) { // LOCV_EXCL_BR_LINE
        LOG_ECMA(FATAL) << "SharedHugeObjectSpace::Allocate:region is nullptr";
    }
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

void SharedHugeObjectSpace::CheckAndTriggerLocalFullMark(JSThread *thread, size_t size)
{
    if (committedSize_ >= triggerLocalFullMarkLimit_) {
        reinterpret_cast<SharedHeap*>(heap_)->TryTriggerLocalConcurrentMarking();
    } else {
        auto localHeap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
        if (!thread->IsJitThread()) {
            localHeap->TryTriggerFullMarkBySharedSize(size);
        }
    }
}
}  // namespace panda::ecmascript
