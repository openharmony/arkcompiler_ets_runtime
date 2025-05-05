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

#include "common_components/common_runtime/src/heap/allocator/region_space.h"


#include "common_components/common_runtime/src/heap/collector/collector.h"
#include "common_components/common_runtime/src/heap/collector/collector_resources.h"
#if defined(ARKCOMMON_SANITIZER_SUPPORT)
#include "common_components/common_runtime/src/sanitizer/sanitizer_interface.h"
#endif
#include "common_components/common_runtime/src/common/scoped_object_access.h"
#include "common_components/common_runtime/src/heap/heap.h"

namespace panda {
HeapAddress RegionSpace::TryAllocateOnce(size_t allocSize, AllocType allocType)
{
    if (UNLIKELY_CC(allocType == AllocType::PINNED_OBJECT)) {
        return regionManager_.AllocPinned(allocSize);
    }
    if (UNLIKELY_CC(allocSize >= regionManager_.GetLargeObjectThreshold())) {
        return regionManager_.AllocLarge(allocSize);
    }
    AllocationBuffer* allocBuffer = AllocationBuffer::GetOrCreateAllocBuffer();
    return allocBuffer->Allocate(allocSize, allocType);
}

bool RegionSpace::ShouldRetryAllocation(size_t& tryTimes) const
{
    {
        // check STW request.
        ScopedEnterSaferegion enterSaferegion(true);
    }

    if (!IsRuntimeThread() && tryTimes <= static_cast<size_t>(TryAllocationThreshold::RESCHEDULE)) {
        // re-add thread reschedule
        // ThreadResched() // reschedule this thread for throughput.
        return true;
    } else if (tryTimes < static_cast<size_t>(TryAllocationThreshold::TRIGGER_OOM)) {
        if (Heap::GetHeap().IsGcStarted()) {
            ScopedEnterSaferegion enterSaferegion(false);
            Heap::GetHeap().GetCollectorResources().WaitForGCFinish();
        } else {
            Heap::GetHeap().GetCollector().RequestGC(GC_REASON_HEU, false);
        }
        return true;
    } else if (tryTimes == static_cast<size_t>(TryAllocationThreshold::TRIGGER_OOM)) {
        if (!Heap::GetHeap().IsGcStarted()) {
            VLOG(REPORT, "gc is triggered for OOM");
            Heap::GetHeap().GetCollector().RequestGC(GC_REASON_OOM, false);
        } else {
            ScopedEnterSaferegion enterSaferegion(false);
            Heap::GetHeap().GetCollectorResources().WaitForGCFinish();
            tryTimes--;
        }
        return true;
    } else {
        Heap::throwOOM();
        return false;
    }
}

HeapAddress RegionSpace::Allocate(size_t size, AllocType allocType)
{
    size_t tryTimes = 0;
    uintptr_t internalAddr = 0;
    size_t allocSize = ToAllocatedSize(size);
    do {
        tryTimes++;
        internalAddr = TryAllocateOnce(allocSize, allocType);
        if (LIKELY_CC(internalAddr != 0)) {
            break;
        }
        if (IsGcThread()) {
            return 0; // it means gc doesn't have enough space to move this object.
        }
        if (!ShouldRetryAllocation(tryTimes)) {
            break;
        }
        (void)sched_yield();
    } while (true);
    if (internalAddr == 0) {
        return 0;
    }
#if defined(ARKCOMMON_TSAN_SUPPORT)
    Sanitizer::TsanAllocObject(reinterpret_cast<void *>(internalAddr), allocSize);
#endif
    return internalAddr + HEADER_SIZE;
}

// Only used for serialization in which allocType and target memory should keep consistency.
HeapAddress RegionSpace::AllocateNoGC(size_t size, AllocType allocType)
{
    bool allowGC = false;
    uintptr_t internalAddr = 0;
    size_t allocSize = ToAllocatedSize(size);
    if (UNLIKELY_CC(allocType == AllocType::PINNED_OBJECT)) {
        internalAddr = regionManager_.AllocPinned(allocSize, allowGC);
    } else if (LIKELY_CC(allocType == AllocType::MOVEABLE_OBJECT)) {
        AllocationBuffer* allocBuffer = AllocationBuffer::GetOrCreateAllocBuffer();
        internalAddr = allocBuffer->Allocate(allocSize, allocType);
    } else {
        // Unreachable for serialization
    }
    if (internalAddr == 0) {
        return 0;
    }
#if defined(ARKCOMMON_TSAN_SUPPORT)
    Sanitizer::TsanAllocObject(reinterpret_cast<void *>(internalAddr), allocSize);
#endif
    return internalAddr + HEADER_SIZE;
}

void RegionSpace::Init(const HeapParam& vmHeapParam)
{
    MemoryMap::Option opt = MemoryMap::DEFAULT_OPTIONS;
    opt.tag = "region_heap";
    size_t heapSize = vmHeapParam.heapSize * 1024;
    size_t totalSize = RegionManager::GetHeapMemorySize(heapSize);
    size_t regionNum = RegionManager::GetHeapUnitCount(heapSize);
#if defined(ARKCOMMON_ASAN_SUPPORT)
    // asan's memory alias technique needs a shareable page
    opt.flags &= ~MAP_PRIVATE;
    opt.flags |= MAP_SHARED;
    DLOG(SANITIZER, "mmap flags set to 0x%x", opt.flags);
#endif
    // this must succeed otherwise it won't return
    map_ = MemoryMap::MapMemoryAlignInner4G(totalSize, totalSize, opt);
#if defined(ARKCOMMON_SANITIZER_SUPPORT)
    Sanitizer::OnHeapAllocated(map->GetBaseAddr(), map->GetMappedSize());
#endif

    HeapAddress metadata = reinterpret_cast<HeapAddress>(map_->GetBaseAddr());
    regionManager_.Initialize(regionNum, metadata);
    reservedStart_ = regionManager_.GetRegionHeapStart();
    reservedEnd_ = reinterpret_cast<HeapAddress>(map_->GetMappedEndAddr());
#if defined(ARK_COMMON_DUMP_ADDRESS)
    VLOG(REPORT, "region metadata@%zx, heap @[0x%zx+%zu, 0x%zx)", metadata, reservedStart, reservedEnd - reservedStart,
         reservedEnd);
#endif
    Heap::OnHeapCreated(reservedStart_);
    Heap::OnHeapExtended(reservedEnd_);
}

AllocationBuffer* AllocationBuffer::GetOrCreateAllocBuffer()
{
    auto* buffer = AllocationBuffer::GetAllocBuffer();
    if (buffer == nullptr) {
        buffer = new (std::nothrow) AllocationBuffer();
        LOGF_CHECK(buffer != nullptr) << "new region alloc buffer fail";
        buffer->Init();
        ThreadLocal::SetAllocBuffer(buffer);
    }
    return buffer;
}
void AllocationBuffer::RefershRegion()
{
    if (LIKELY_CC(tlRegion_ != RegionDesc::NullRegion())) {
        RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
        RegionManager& manager = theAllocator.GetRegionManager();
        manager.RemoveThreadLocalRegion(tlRegion_);
        manager.EnlistFullThreadLocalRegion(tlRegion_);
        tlRegion_ = RegionDesc::NullRegion();
    }
}
AllocationBuffer* AllocationBuffer::GetAllocBuffer() { return ThreadLocal::GetAllocBuffer(); }

AllocationBuffer::~AllocationBuffer()
{
    if (LIKELY_CC(tlRegion_ != RegionDesc::NullRegion())) {
        RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
        RegionManager& manager = theAllocator.GetRegionManager();
        manager.RemoveThreadLocalRegion(tlRegion_);
        manager.EnlistFullThreadLocalRegion(tlRegion_);
        tlRegion_ = RegionDesc::NullRegion();
    }
}

void AllocationBuffer::Init()
{
    static_assert(offsetof(AllocationBuffer, tlRegion_) == 0,
                  "need to modify the offset of this value in llvm-project at the same time");
    tlRegion_ = RegionDesc::NullRegion();
    Heap::GetHeap().RegisterAllocBuffer(*this);
}

HeapAddress AllocationBuffer::ToSpaceAllocate(size_t totalSize, AllocType allocType)
{
    HeapAddress addr = 0;
    if (UNLIKELY_CC(allocType == AllocType::RAW_POINTER_OBJECT)) {
        return AllocateRawPointerObject(totalSize);
    }

    if (LIKELY_CC(tlRegion_ != RegionDesc::NullRegion())) {
        addr = tlRegion_->Alloc(totalSize);
    }

    if (UNLIKELY_CC(addr == 0)) {
        addr = AllocateImpl(totalSize, allocType);
    }

    DLOG(ALLOC, "alloc 0x%zx(%zu)", addr, totalSize);
    tlRegion_->SetToSpaceRegion(true);
    return addr;
}

HeapAddress AllocationBuffer::Allocate(size_t totalSize, AllocType allocType)
{
    // a hoisted specific fast path which can be inlined
    HeapAddress addr = 0;
    if (UNLIKELY_CC(allocType == AllocType::RAW_POINTER_OBJECT)) {
        return AllocateRawPointerObject(totalSize);
    }

    if (LIKELY_CC(tlRegion_ != RegionDesc::NullRegion())) {
        addr = tlRegion_->Alloc(totalSize);
    }

    if (UNLIKELY_CC(addr == 0)) {
        addr = AllocateImpl(totalSize, allocType);
    }

    DLOG(ALLOC, "alloc 0x%zx(%zu)", addr, totalSize);
    return addr;
}

// try an allocation but do not handle failure
HeapAddress AllocationBuffer::AllocateImpl(size_t totalSize, AllocType allocType)
{
    RegionSpace& theAllocator = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
    RegionManager& manager = theAllocator.GetRegionManager();

    // allocate from thread local region
    if (LIKELY_CC(tlRegion_ != RegionDesc::NullRegion())) {
        HeapAddress addr = tlRegion_->Alloc(totalSize);
        if (addr != 0) {
            return addr;
        }

        // allocation failed because region is full.
        if (tlRegion_->IsThreadLocalRegion()) {
            manager.RemoveThreadLocalRegion(tlRegion_);
            manager.EnlistFullThreadLocalRegion(tlRegion_);
            tlRegion_ = RegionDesc::NullRegion();
        }
    }

    RegionDesc* r = manager.AllocateThreadLocalRegion();
    if (UNLIKELY_CC(r == nullptr)) {
        return 0;
    }
    tlRegion_ = r;
    return r->Alloc(totalSize);
    // }
    // not enough region for thread local buffer.
    return 0;
}

HeapAddress AllocationBuffer::AllocateRawPointerObject(size_t totalSize)
{
    RegionDesc* region = tlRawPointerRegions_.GetHeadRegion();
    if (region != nullptr) {
        HeapAddress allocAddr = region->Alloc(totalSize);
        if (allocAddr != 0) {
            return allocAddr;
        }
    }
    RegionManager& manager = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator()).GetRegionManager();
    size_t needRegionNum = totalSize / RegionDesc::UNIT_SIZE + 1;
    // region should have at least 2 unit
    needRegionNum = (needRegionNum == 1) ? 2 : needRegionNum;
    region = manager.TakeRegion(needRegionNum, RegionDesc::UnitRole::SMALL_SIZED_UNITS);
    if (region == nullptr) {
        return 0;
    }
    // region is enough for totalSize.
    HeapAddress allocAddr = region->Alloc(totalSize);
    ASSERT_LOGF(allocAddr != 0, "allocation failure");
    tlRawPointerRegions_.PrependRegion(region, RegionDesc::RegionType::TL_RAW_POINTER_REGION);
    return allocAddr;
}

#ifndef NDEBUG
bool RegionSpace::IsHeapObject(HeapAddress addr) const
{
    if (!IsHeapAddress(addr)) {
        return false;
    }
    return true;
}
#endif
void RegionSpace::FeedHungryBuffers()
{
    ScopedObjectAccess soa;
    AllocBufferManager::HungryBuffers hungryBuffers;
    allocBufferManager_->SwapHungryBuffers(hungryBuffers);
    RegionDesc* region = nullptr;
    for (auto* buffer : hungryBuffers) {
        if (buffer->GetPreparedRegion() != nullptr) { continue; }
        if (region == nullptr) {
            region = regionManager_.AllocateThreadLocalRegion(true);
            if (region == nullptr) { return; }
        }
        if (buffer->SetPreparedRegion(region)) {
            region = nullptr;
        }
    }
    if (region != nullptr) {
        regionManager_.CollectRegion(region);
    }
}
} // namespace panda
