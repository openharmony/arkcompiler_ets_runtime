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

#include "common_components/heap/allocator/region_space.h"

#include "common_components/heap/collector/collector.h"
#include "common_components/heap/collector/collector_resources.h"
#include "common_components/platform/os.h"
#if defined(COMMON_SANITIZER_SUPPORT)
#include "common_components/sanitizer/sanitizer_interface.h"
#endif
#include "common_components/common/scoped_object_access.h"
#include "common_components/heap/heap.h"

namespace common {
RegionDesc* RegionSpace::AllocateThreadLocalRegion(bool expectPhysicalMem, AllocType allocType)
{
    RegionDesc* region = regionManager_.TakeRegion(expectPhysicalMem, true);
    if (region != nullptr) {
        if (IsGcThread()) {
            toSpace_.AddThreadLocalRegion(region);
            DLOG(REGION, "alloc to-region %p @0x%zx+%zu units[%zu+%zu, %zu) type %u",
                region, region->GetRegionStart(), region->GetRegionSize(),
                region->GetUnitIdx(), region->GetUnitCount(), region->GetUnitIdx() + region->GetUnitCount(),
                region->GetRegionType());
        } else {
            GCPhase phase = Mutator::GetMutator()->GetMutatorPhase();
            if (phase == GC_PHASE_ENUM || phase == GC_PHASE_MARK || phase == GC_PHASE_REMARK_SATB ||
                phase == GC_PHASE_POST_MARK) {
                region->SetTraceLine();
            } else if (phase == GC_PHASE_PRECOPY || phase == GC_PHASE_COPY) {
                region->SetTraceLine();
                region->SetCopyLine();
            } else if (phase == GC_PHASE_FIX) {
                region->SetTraceLine();
                region->SetCopyLine();
                region->SetFixLine();
            }

            if (allocType == AllocType::MOVEABLE_OBJECT) {
                youngSpace_.AddThreadLocalRegion(region);
            } else if (allocType == AllocType::MOVEABLE_OLD_OBJECT) {
                oldSpace_.AddThreadLocalRegion(region);
            } else {
                UNREACHABLE();
            }
            DLOG(REGION, "alloc tl-region %p @0x%zx+%zu units[%zu+%zu, %zu) type %u, gc phase: %u",
                region, region->GetRegionStart(), region->GetRegionSize(),
                region->GetUnitIdx(), region->GetUnitCount(), region->GetUnitIdx() + region->GetUnitCount(),
                region->GetRegionType(), phase);
        }
    }

    return region;
}

void RegionSpace::DumpAllRegionStats(const char* msg) const
{
    VLOG(DEBUG, msg);

    youngSpace_.DumpRegionStats();
    oldSpace_.DumpRegionStats();
    fromSpace_.DumpRegionStats();
    toSpace_.DumpRegionStats();
    regionManager_.DumpRegionStats();

    size_t usedUnits = GetUsedUnitCount();
    VLOG(DEBUG, "\tused units: %zu (%zu B)", usedUnits, usedUnits * RegionDesc::UNIT_SIZE);
}

HeapAddress RegionSpace::TryAllocateOnce(size_t allocSize, AllocType allocType)
{
    if (UNLIKELY_CC(allocType == AllocType::READ_ONLY_OBJECT)) {
        return regionManager_.AllocReadOnly(allocSize);
    }
    if (UNLIKELY_CC(allocSize >= RegionDesc::LARGE_OBJECT_DEFAULT_THRESHOLD)) {
        return regionManager_.AllocLarge(allocSize);
    }
    if (UNLIKELY_CC(allocType == AllocType::PINNED_OBJECT)) {
        return regionManager_.AllocPinned(allocSize);
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
            VLOG(INFO, "gc is triggered for OOM");
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

uintptr_t RegionSpace::AllocRegion()
{
    RegionDesc* region = regionManager_.TakeRegion(false, false);
    ASSERT(region != nullptr);

    GCPhase phase = Mutator::GetMutator()->GetMutatorPhase();
    if (phase == GC_PHASE_ENUM || phase == GC_PHASE_MARK || phase == GC_PHASE_REMARK_SATB ||
        phase == GC_PHASE_POST_MARK) {
        region->SetTraceLine();
    } else if (phase == GC_PHASE_PRECOPY || phase == GC_PHASE_COPY) {
        region->SetTraceLine();
        region->SetCopyLine();
    } else if (phase == GC_PHASE_FIX) {
        region->SetTraceLine();
        region->SetCopyLine();
        region->SetFixLine();
    }

    DLOG(REGION, "alloc small object region %p @0x%zx+%zu units[%zu+%zu, %zu) type %u",
        region, region->GetRegionStart(), region->GetRegionSize(),
        region->GetUnitIdx(), region->GetUnitCount(), region->GetUnitIdx() + region->GetUnitCount(),
        region->GetRegionType());
    youngSpace_.AddFullRegion(region);

    uintptr_t start = region->GetRegionStart();
    uintptr_t addr = region->Alloc(region->GetRegionEnd() - region->GetRegionAllocPtr());
    ASSERT(addr != 0);

    return start;
}

uintptr_t RegionSpace::AllocPinnedRegion()
{
    RegionDesc* region = regionManager_.TakeRegion(false, false);
    ASSERT(region != nullptr);

    GCPhase phase = Mutator::GetMutator()->GetMutatorPhase();
    if (phase == GC_PHASE_ENUM || phase == GC_PHASE_MARK || phase == GC_PHASE_REMARK_SATB ||
        phase == GC_PHASE_POST_MARK) {
        region->SetTraceLine();
    } else if (phase == GC_PHASE_PRECOPY || phase == GC_PHASE_COPY) {
        region->SetTraceLine();
        region->SetCopyLine();
    } else if (phase == GC_PHASE_FIX) {
        region->SetTraceLine();
        region->SetCopyLine();
        region->SetFixLine();
    }

    DLOG(REGION, "alloc pinned region @0x%zx+%zu type %u", region->GetRegionStart(),
         region->GetRegionAllocatedSize(),
         region->GetRegionType());
    regionManager_.AddRecentPinnedRegion(region);

    uintptr_t start = region->GetRegionStart();
    uintptr_t addr = region->Alloc(region->GetRegionEnd() - region->GetRegionAllocPtr());
    ASSERT(addr != 0);

    return start;
}

uintptr_t RegionSpace::AllocLargeRegion(size_t size)
{
    return regionManager_.AllocLarge(size, false);
}

uintptr_t RegionSpace::AllocJitFortRegion(size_t size)
{
    uintptr_t addr = regionManager_.AllocLarge(size, false);
    os::PrctlSetVMA(reinterpret_cast<void *>(addr), size, "ArkTS Code");
    regionManager_.MarkJitFortMemAwaitingInstall(reinterpret_cast<BaseObject*>(addr));
    return addr;
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
#if defined(COMMON_TSAN_SUPPORT)
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
    } else if (LIKELY_CC(allocType == AllocType::MOVEABLE_OBJECT || allocType == AllocType::MOVEABLE_OLD_OBJECT)) {
        AllocationBuffer* allocBuffer = AllocationBuffer::GetOrCreateAllocBuffer();
        internalAddr = allocBuffer->Allocate(allocSize, allocType);
    } else {
        // Unreachable for serialization
        UNREACHABLE_CC();
    }
    if (internalAddr == 0) {
        return 0;
    }
#if defined(COMMON_TSAN_SUPPORT)
    Sanitizer::TsanAllocObject(reinterpret_cast<void *>(internalAddr), allocSize);
#endif
    return internalAddr + HEADER_SIZE;
}

void RegionSpace::CopyRegion(RegionDesc* region)
{
    LOGF_CHECK(region->IsFromRegion()) << "region type " << static_cast<uint8_t>(region->GetRegionType());
    DLOG(COPY, "try forward region %p @0x%zx+%zu type %u, live bytes %u",
        region, region->GetRegionStart(), region->GetRegionAllocatedSize(),
        region->GetRegionType(), region->GetLiveByteCount());

    if (region->GetLiveByteCount() == 0) {
        return;
    }

    int32_t rawPointerCount = region->GetRawPointerObjectCount();
    CHECK(rawPointerCount == 0);
    Collector& collector = Heap::GetHeap().GetCollector();
    bool forwarded = region->VisitLiveObjectsUntilFalse(
        [&collector](BaseObject* obj) { return collector.ForwardObject(obj); });
    if (!forwarded) {
        DLOG(COPY, "failure to forward region %p @0x%zx+%zu units[%zu+%zu, %zu) type %u, %u live bytes",
            region, region->GetRegionStart(), region->GetRegionAllocatedSize(),
            region->GetUnitIdx(), region->GetUnitCount(), region->GetUnitIdx() + region->GetUnitCount(),
            region->GetRegionType(), region->GetLiveByteCount());

        fromSpace_.DeleteFromRegion(region);
        // since this region is possibly partially-forwarded, treat it as to-region.
        toSpace_.AddFullRegion(region);
    }
}

void RegionSpace::Init(const RuntimeParam& param)
{
    MemoryMap::Option opt = MemoryMap::DEFAULT_OPTIONS;
    opt.tag = "region_heap";
    size_t heapSize = param.heapParam.heapSize * KB;
    size_t totalSize = RegionManager::GetHeapMemorySize(heapSize);
    size_t regionNum = RegionManager::GetHeapUnitCount(heapSize);
#if defined(COMMON_ASAN_SUPPORT)
    // asan's memory alias technique needs a shareable page
    opt.flags &= ~MAP_PRIVATE;
    opt.flags |= MAP_SHARED;
    DLOG(SANITIZER, "mmap flags set to 0x%x", opt.flags);
#endif
    // this must succeed otherwise it won't return
    map_ = MemoryMap::MapMemoryAlignInner4G(totalSize, totalSize, opt);

    size_t metadataSize = RegionManager::GetMetadataSize(regionNum);
    uintptr_t baseAddr = reinterpret_cast<uintptr_t>(map_->GetBaseAddr());
    os::PrctlSetVMA(reinterpret_cast<void*>(baseAddr), metadataSize, "ArkTS Heap CMCGC Metadata");
    os::PrctlSetVMA(reinterpret_cast<void*>(baseAddr + metadataSize), totalSize - metadataSize,
                    "ArkTS Heap CMCGC RegionHeap");

#if defined(COMMON_SANITIZER_SUPPORT)
    Sanitizer::OnHeapAllocated(map->GetBaseAddr(), map->GetMappedSize());
#endif

    HeapAddress metadata = reinterpret_cast<HeapAddress>(map_->GetBaseAddr());
    fromSpace_.SetExemptedRegionThreshold(param.heapParam.exemptionThreshold);
    regionManager_.Initialize(regionNum, metadata);
    reservedStart_ = regionManager_.GetRegionHeapStart();
    reservedEnd_ = reinterpret_cast<HeapAddress>(map_->GetMappedEndAddr());
#if defined(COMMON_DUMP_ADDRESS)
    VLOG(DEBUG, "region metadata@%zx, heap @[0x%zx+%zu, 0x%zx)", metadata, reservedStart, reservedEnd - reservedStart,
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
void AllocationBuffer::ClearThreadLocalRegion()
{
    if (LIKELY_CC(tlRegion_ != RegionDesc::NullRegion())) {
        RegionSpace& heap = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
        heap.HandleFullThreadLocalRegion<AllocBufferType::YOUNG>(tlRegion_);
        tlRegion_ = RegionDesc::NullRegion();
    }
    if (LIKELY_CC(tlOldRegion_ != RegionDesc::NullRegion())) {
        RegionSpace& heap = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());
        heap.HandleFullThreadLocalRegion<AllocBufferType::OLD>(tlOldRegion_);
        tlOldRegion_ = RegionDesc::NullRegion();
    }
}

void AllocationBuffer::Unregister()
{
    Heap::GetHeap().UnregisterAllocBuffer(*this);
}

AllocationBuffer* AllocationBuffer::GetAllocBuffer() { return ThreadLocal::GetAllocBuffer(); }

AllocationBuffer::~AllocationBuffer()
{
    ClearThreadLocalRegion();
}

void AllocationBuffer::Init()
{
    static_assert(offsetof(AllocationBuffer, tlRegion_) == 0,
                  "need to modify the offset of this value in llvm-project at the same time");
    tlRegion_ = RegionDesc::NullRegion();
    tlOldRegion_ = RegionDesc::NullRegion();
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

    DLOG(ALLOC, "alloc to 0x%zx(%zu)", addr, totalSize);
    return addr;
}

HeapAddress AllocationBuffer::Allocate(size_t totalSize, AllocType allocType)
{
    // a hoisted specific fast path which can be inlined
    HeapAddress addr = 0;
    if (UNLIKELY_CC(allocType == AllocType::RAW_POINTER_OBJECT)) {
        return AllocateRawPointerObject(totalSize);
    }

    ASSERT_LOGF(allocType == AllocType::MOVEABLE_OBJECT || allocType == AllocType::MOVEABLE_OLD_OBJECT,
                "unexpected alloc type");

    if (allocType == AllocType::MOVEABLE_OBJECT) {
        if (LIKELY_CC(tlRegion_ != RegionDesc::NullRegion())) {
            addr = tlRegion_->Alloc(totalSize);
        }
    } else if (allocType == AllocType::MOVEABLE_OLD_OBJECT) {
        if (LIKELY_CC(tlOldRegion_ != RegionDesc::NullRegion())) {
            addr = tlOldRegion_->Alloc(totalSize);
        }
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
    RegionSpace& heapSpace = reinterpret_cast<RegionSpace&>(Heap::GetHeap().GetAllocator());

    // allocate from thread local region
    if (allocType == AllocType::MOVEABLE_OBJECT) {
        if (LIKELY_CC(tlRegion_ != RegionDesc::NullRegion())) {
            // allocation failed because region is full.
            if (tlRegion_->IsThreadLocalRegion()) {
                heapSpace.HandleFullThreadLocalRegion<AllocBufferType::YOUNG>(tlRegion_);
                tlRegion_ = RegionDesc::NullRegion();
            }
        }
    } else if (allocType == AllocType::MOVEABLE_OLD_OBJECT) {
        if (LIKELY_CC(tlOldRegion_ != RegionDesc::NullRegion())) {
            HeapAddress addr = tlOldRegion_->Alloc(totalSize);
            if (addr != 0) {
                return addr;
            }

            // allocation failed because region is full.
            if (tlOldRegion_->IsThreadLocalRegion()) {
                heapSpace.HandleFullThreadLocalRegion<AllocBufferType::OLD>(tlOldRegion_);
                tlOldRegion_ = RegionDesc::NullRegion();
            }
        }
    }

    RegionDesc* r = heapSpace.AllocateThreadLocalRegion(false, allocType);
    if (UNLIKELY_CC(r == nullptr)) {
        return 0;
    }

    if (allocType == AllocType::MOVEABLE_OBJECT) {
        tlRegion_ = r;
    } else if (allocType == AllocType::MOVEABLE_OLD_OBJECT) {
        tlOldRegion_ = r;
    }

    return r->Alloc(totalSize);
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
            region = AllocateThreadLocalRegion(true);
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

void RegionSpace::MarkRememberSet(const std::function<void(BaseObject*)>& func)
{
    oldSpace_.MarkRememberSet(func);
    regionManager_.MarkRememberSet(func);
}
} // namespace common
