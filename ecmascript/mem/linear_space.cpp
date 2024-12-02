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
#include "ecmascript/mem/mem_controller.h"

namespace panda::ecmascript {
LinearSpace::LinearSpace(Heap *heap, MemSpaceType type, size_t initialCapacity, size_t maximumCapacity)
    : Space(heap, heap->GetHeapRegionAllocator(), type, initialCapacity, maximumCapacity),
      localHeap_(heap),
      thread_(heap->GetJSThread()),
      waterLine_(0)
{
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
    if (Expand(isPromoted)) {
        if (!isPromoted) {
            if (!localHeap_->NeedStopCollection() || localHeap_->IsNearGCInSensitive()) {
                localHeap_->TryTriggerIncrementalMarking();
                localHeap_->TryTriggerIdleCollection();
                localHeap_->TryTriggerConcurrentMarking();
            }
        }
        object = allocator_.Allocate(size);
    } else if (localHeap_->IsMarking() || !localHeap_->IsEmptyIdleTask()) {
        // Temporary adjust semi space capacity
        if (localHeap_->IsConcurrentFullMark()) {
            overShootSize_ = localHeap_->CalculateLinearSpaceOverShoot();
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
        !localHeap_->NeedStopCollection()) {
        return false;
    }

    uintptr_t top = allocator_.GetTop();
    auto currentRegion = GetCurrentRegion();
    if (currentRegion != nullptr) {
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
    }
    JSThread *thread = localHeap_->GetJSThread();
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, DEFAULT_REGION_SIZE, thread, localHeap_,
                                                                 thread_->IsConcurrentMarkingOrFinished());
    allocator_.Reset(region->GetBegin(), region->GetEnd());
    AddRegion(region);
    return true;
}

void LinearSpace::Stop()
{
    if (GetCurrentRegion() != nullptr) {
        GetCurrentRegion()->SetHighWaterMark(allocator_.GetTop());
    }
}

void LinearSpace::ResetAllocator()
{
    auto currentRegion = GetCurrentRegion();
    if (currentRegion != nullptr) {
        allocator_.Reset(currentRegion->GetBegin(), currentRegion->GetEnd(), currentRegion->GetHighWaterMark());
    }
}

void LinearSpace::IterateOverObjects(const std::function<void(TaggedObject *object)> &visitor) const
{
    auto current = GetCurrentRegion();
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

EdenSpace::EdenSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity)
    : LinearSpace(heap, MemSpaceType::EDEN_SPACE, initialCapacity, maximumCapacity)
{
    size_t memSize = AlignUp(maximumCapacity_, DEFAULT_REGION_SIZE);
    memMap_ = PageMap(memSize, PAGE_PROT_READWRITE, DEFAULT_REGION_SIZE);
    JSThread::ThreadId threadId = 0;
    if (heap->EnablePageTagThreadId()) {
        threadId = heap->GetJSThread()->GetThreadId();
    }
    PageTag(memMap_.GetMem(), memMap_.GetSize(), PageTagType::HEAP, ToSpaceTypeName(MemSpaceType::EDEN_SPACE),
            threadId);
    auto mem = ToUintPtr(memMap_.GetMem());
    auto count = memMap_.GetSize() / DEFAULT_REGION_SIZE;
    while (count-- > 0) {
        freeRegions_.emplace_back(ToVoidPtr(mem), DEFAULT_REGION_SIZE);
        mem = mem + DEFAULT_REGION_SIZE;
    }
}

EdenSpace::~EdenSpace()
{
    PageUnmap(memMap_);
}

void EdenSpace::Initialize()
{
    auto region = AllocRegion();
    if (UNLIKELY(region == nullptr)) { // LCOV_EXCL_BR_LINE
        LOG_GC(ERROR) << "EdenSpace::Initialize: region is nullptr";
        return;
    }
    AddRegion(region);
    allocator_.Reset(region->GetBegin(), region->GetEnd());
    localHeap_->InstallEdenAllocator();
}

void EdenSpace::Restart()
{
    overShootSize_ = 0;
    survivalObjectSize_ = 0;
    allocateAfterLastGC_ = 0;
    isFull_ = false;
    Initialize();
}

uintptr_t EdenSpace::AllocateSync(size_t size)
{
    LockHolder lock(lock_);
    return Allocate(size);
}

uintptr_t EdenSpace::Allocate(size_t size)
{
    if (isFull_) {
        return 0;
    }
    auto object = allocator_.Allocate(size);
    if (object != 0) {
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
        // can not heap sampling in gc.
        InvokeAllocationInspector(object, size, size);
#endif
        return object;
    }
    if (Expand()) {
        if (!localHeap_->NeedStopCollection()) {
            localHeap_->TryTriggerIncrementalMarking();
            localHeap_->TryTriggerIdleCollection();
            localHeap_->TryTriggerConcurrentMarking();
        }
        object = allocator_.Allocate(size);
    } else {
        isFull_ = true;
        localHeap_->ReleaseEdenAllocator();
    }
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
    if (object != 0) {
        InvokeAllocationInspector(object, size, size);
    }
#endif
    return object;
}

Region *EdenSpace::AllocRegion()
{
    if (freeRegions_.empty()) {
        return nullptr;
    }
    auto memmap = freeRegions_.back();
    freeRegions_.pop_back();
    heapRegionAllocator_->IncreaseAnnoMemoryUsage(memmap.GetSize());
    auto mem = reinterpret_cast<uintptr_t>(memmap.GetMem());
    // Check that the address is 256K byte aligned
    LOG_ECMA_IF(AlignUp(mem, PANDA_POOL_ALIGNMENT_IN_BYTES) != mem, FATAL) << "region not align by 256KB";

    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    uintptr_t begin = AlignUp(mem + sizeof(Region), static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
    uintptr_t end = mem + memmap.GetSize();
    auto region = new (ToVoidPtr(mem)) Region(localHeap_->GetNativeAreaAllocator(), mem, begin, end,
                                              GetRegionFlag(), RegionTypeFlag::DEFAULT);
    region->Initialize();
    return region;
}

bool EdenSpace::Expand()
{
    Region *region = AllocRegion();
    if (region == nullptr) {
        return false;
    }

    uintptr_t top = allocator_.GetTop();
    auto currentRegion = GetCurrentRegion();
    if (currentRegion != nullptr) {
        if (currentRegion->HasAgeMark()) {
            allocateAfterLastGC_ +=
                currentRegion->GetAllocatedBytes(top) - currentRegion->GetAllocatedBytes(waterLine_);
        } else {
            allocateAfterLastGC_ += currentRegion->GetAllocatedBytes(top);
        }
        currentRegion->SetHighWaterMark(top);
    }
    allocator_.Reset(region->GetBegin(), region->GetEnd());
    AddRegion(region);
    return true;
}

void EdenSpace::ReclaimRegions([[maybe_unused]] size_t cachedSize)
{
    const auto spaceName = ToSpaceTypeName(MemSpaceType::EDEN_SPACE);
    EnumerateRegions([this, &spaceName](Region *current) {
        LOG_GC(DEBUG) << "Clear region from: " << current << " to " << spaceName;
        current->DeleteLocalToShareRSet();
        DecreaseCommitted(current->GetCapacity());
        DecreaseObjectSize(current->GetSize());
        current->Invalidate();
        current->ClearMembers();
        void *mem = ToVoidPtr(current->GetAllocateBase());
        size_t memSize = current->GetCapacity();
        freeRegions_.emplace_back(mem, memSize);
        heapRegionAllocator_->DecreaseAnnoMemoryUsage(memSize);
    });
    regionList_.Clear();
    committedSize_ = 0;
}

size_t EdenSpace::GetHeapObjectSize() const
{
    return survivalObjectSize_ + allocateAfterLastGC_;
}

size_t EdenSpace::GetSurvivalObjectSize() const
{
    return survivalObjectSize_;
}

void EdenSpace::SetOverShootSize(size_t size)
{
    overShootSize_ = size;
}

size_t EdenSpace::GetAllocatedSizeSinceGC(uintptr_t top) const
{
    size_t currentRegionSize = 0;
    auto currentRegion = GetCurrentRegion();
    if (currentRegion != nullptr) {
        currentRegionSize = currentRegion->GetAllocatedBytes(top);
        if (currentRegion->HasAgeMark()) {
            currentRegionSize -= currentRegion->GetAllocatedBytes(waterLine_);
        }
    }
    return allocateAfterLastGC_ + currentRegionSize;
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
           0 : AlignUp((committedSize_ - maximumCapacity_) / 2, DEFAULT_REGION_SIZE); // 2 is the half.
}

bool SemiSpace::CommittedSizeIsLarge()
{
    return committedSize_ >= maximumCapacity_ * 2; // 2 is the half.
}

uintptr_t SemiSpace::AllocateSync(size_t size)
{
    LockHolder lock(lock_);
    return Allocate(size, true);
}

bool SemiSpace::SwapRegion(Region *region, SemiSpace *fromSpace)
{
    LockHolder lock(lock_);
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

void SemiSpace::SetWaterLine()
{
    waterLine_ = allocator_.GetTop();
    allocateAfterLastGC_ = 0;
    Region *last = GetCurrentRegion();
    if (last != nullptr) {
        last->SetGCFlag(RegionGCFlags::HAS_AGE_MARK);

        EnumerateRegions([&last](Region *current) {
            if (current != last) {
                current->SetGCFlag(RegionGCFlags::BELOW_AGE_MARK);
            }
        });
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
    } else if (curObjectSurvivalRate < SHRINK_OBJECT_SURVIVAL_RATE) {
        if (initialCapacity_ <= minimumCapacity_) {
            return false;
        }
        double speed = localHeap_->GetMemController()->GetNewSpaceAllocationThroughputPerMS();
        if (speed > LOW_ALLOCATION_SPEED_PER_MS) {
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
    auto currentRegion = GetCurrentRegion();
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
