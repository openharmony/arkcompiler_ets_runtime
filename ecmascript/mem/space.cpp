/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/space-inl.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/heap_region_allocator.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/space.h"

namespace panda::ecmascript {
Space::Space(HeapRegionAllocator *heapRegionAllocator,
             MemSpaceType spaceType, size_t initialCapacity,
             size_t maximumCapacity)
    : heapRegionAllocator_(heapRegionAllocator),
      spaceType_(spaceType),
      initialCapacity_(initialCapacity),
      maximumCapacity_(maximumCapacity),
      committedSize_(0)
{
}

void Space::AddAllocationInspector(AllocationInspector* inspector)
{
    allocationCounter_.AddAllocationInspector(inspector);
}

void Space::ClearAllocationInspector()
{
    allocationCounter_.ClearAllocationInspector();
}

void Space::SwapAllocationCounter(Space *space)
{
    std::swap(allocationCounter_, space->allocationCounter_);
}

void Space::Destroy()
{
    ReclaimRegions();
}

void Space::ReclaimRegions()
{
    EnumerateRegions([this](Region *current) { ClearAndFreeRegion(current); });
    regionList_.Clear();
    committedSize_ = 0;
}

void Space::ClearAndFreeRegion(Region *region)
{
    LOG_ECMA_MEM(DEBUG) << "Clear region from:" << region << " to " << ToSpaceTypeName(spaceType_);
    region->DeleteCrossRegionRSet();
    region->DeleteOldToNewRSet();
    region->DeleteSweepingRSet();
    DecreaseCommitted(region->GetCapacity());
    DecreaseObjectSize(region->GetSize());
    if (spaceType_ == MemSpaceType::OLD_SPACE || spaceType_ == MemSpaceType::NON_MOVABLE ||
        spaceType_ == MemSpaceType::MACHINE_CODE_SPACE || spaceType_ == MemSpaceType::LOCAL_SPACE ||
        spaceType_ == MemSpaceType::APPSPAWN_SPACE) {
        region->DestroyFreeObjectSets();
    }
    heapRegionAllocator_->FreeRegion(region);
}

HugeObjectSpace::HugeObjectSpace(Heap *heap, HeapRegionAllocator *heapRegionAllocator,
                                 size_t initialCapacity, size_t maximumCapacity)
    : Space(heapRegionAllocator, MemSpaceType::HUGE_OBJECT_SPACE, initialCapacity, maximumCapacity),
      heap_(heap)
{
}

uintptr_t HugeObjectSpace::Allocate(size_t objectSize, JSThread *thread)
{
    // In HugeObject allocation, we have a revervation of 8 bytes for markBitSet in objectSize.
    // In case Region is not aligned by 16 bytes, HUGE_OBJECT_BITSET_SIZE is 8 bytes more.
    size_t alignedSize = AlignUp(objectSize + sizeof(Region) + HUGE_OBJECT_BITSET_SIZE, PANDA_POOL_ALIGNMENT_IN_BYTES);
    if (heap_->OldSpaceExceedCapacity(alignedSize)) {
        LOG_ECMA_MEM(INFO) << "Committed size " << committedSize_ << " of huge object space is too big.";
        return 0;
    }
    Region *region = heapRegionAllocator_->AllocateAlignedRegion(this, alignedSize, thread);
    AddRegion(region);
    // It need to mark unpoison when huge object being allocated.
    ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<void *>(region->GetBegin()), objectSize);
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
    InvokeAllocationInspector(region->GetBegin(), objectSize);
#endif
    return region->GetBegin();
}

void HugeObjectSpace::Sweep()
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

size_t HugeObjectSpace::GetHeapObjectSize() const
{
    return committedSize_;
}

void HugeObjectSpace::IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const
{
    EnumerateRegions([&](Region *region) {
        uintptr_t curPtr = region->GetBegin();
        objectVisitor(reinterpret_cast<TaggedObject *>(curPtr));
    });
}

void HugeObjectSpace::ReclaimHugeRegion()
{
    if (hugeNeedFreeList_.IsEmpty()) {
        return;
    }
    do {
        Region *last = hugeNeedFreeList_.PopBack();
        ClearAndFreeRegion(last);
    } while (!hugeNeedFreeList_.IsEmpty());
}

void HugeObjectSpace::InvokeAllocationInspector(Address object, size_t objectSize)
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
