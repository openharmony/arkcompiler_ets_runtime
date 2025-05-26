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

#include "common_components/common_runtime/src/heap/allocator/region_manager.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unistd.h>

#include "common_components/base_runtime/hooks.h"
#include "common_components/common_runtime/src/heap/allocator/region_desc.h"
#include "common_components/common_runtime/src/heap/allocator/region_space.h"
#include "common_components/common_runtime/src/base/c_string.h"
#include "common_components/common_runtime/src/heap/collector/collector.h"
#include "common_components/common_runtime/src/heap/collector/trace_collector.h"
#include "common_components/common_runtime/src/common/base_object.h"
#include "common_components/common_runtime/src/common/scoped_object_access.h"
#include "common_components/common_runtime/src/heap/heap.h"
#include "common_components/common_runtime/src/mutator/mutator.inline.h"
#include "common_components/common_runtime/src/mutator/mutator_manager.h"

#if defined(ARKCOMMON_TSAN_SUPPORT)
#include "common_components/common_runtime/src/sanitizer/sanitizer_interface.h"
#endif
#include "common_components/log/log.h"
#include "common_interfaces/base_runtime.h"

namespace panda {
uintptr_t RegionDesc::UnitInfo::totalUnitCount = 0;
uintptr_t RegionDesc::UnitInfo::unitInfoStart = 0;
uintptr_t RegionDesc::UnitInfo::heapStartAddress = 0;

static size_t GetPageSize() noexcept
{
#if defined(_WIN64)
    SYSTEM_INFO systeminfo;
    GetSystemInfo(&systeminfo);
    if (systeminfo.dwPageSize != 0) {
        return systeminfo.dwPageSize;
    } else {
        // default page size is 4KB if get system page size failed.
        return 4 * KB;
    }
#elif defined(__APPLE__)
    // default page size is 4KB in MacOS
    return 4 * KB;
#else
    return getpagesize();
#endif
}

// System default page size
const size_t ARK_COMMON_PAGE_SIZE = GetPageSize();
const size_t AllocatorUtils::ALLOC_PAGE_SIZE = panda::ARK_COMMON_PAGE_SIZE;
// region unit size: same as system page size
const size_t RegionDesc::UNIT_SIZE = panda::ARK_COMMON_PAGE_SIZE;
// regarding a object as a large object when the size is greater than 32KB or one page size,
// depending on the system page size.
const size_t RegionDesc::LARGE_OBJECT_DEFAULT_THRESHOLD = panda::ARK_COMMON_PAGE_SIZE > (32 * KB) ?
                                                            panda::ARK_COMMON_PAGE_SIZE : 32 * KB;
// max size of per region is 128KB.
const size_t RegionManager::MAX_UNIT_COUNT_PER_REGION = (128 * KB) / panda::ARK_COMMON_PAGE_SIZE;
// size of huge page is 2048KB.
const size_t RegionManager::HUGE_PAGE = (2048 * KB) / panda::ARK_COMMON_PAGE_SIZE;;

class CopyTask : public HeapWork {
public:
    CopyTask(RegionManager& manager, RegionDesc& region, size_t regionCnt)
        : regionManager_(manager), startRegion_(region), regionCount_(regionCnt) {}

    ~CopyTask() = default;

    void Execute(size_t) override
    {
        RegionDesc* currentRegion = &startRegion_;
        for (size_t count = 0; (count < regionCount_) && currentRegion != nullptr; ++count) {
            RegionDesc* region = currentRegion;
            currentRegion = currentRegion->GetNextRegion();
            regionManager_.CopyRegion(region);
        }

        AllocationBuffer* allocBuffer = AllocationBuffer::GetAllocBuffer();
        if (LIKELY_CC(allocBuffer != nullptr)) {
            allocBuffer->ClearRegion(); // clear thread local region for gc threads.
        }
    }

private:
    RegionManager& regionManager_;
    RegionDesc& startRegion_;
    size_t regionCount_;
};

#if defined(GCINFO_DEBUG) && GCINFO_DEBUG
void RegionDesc::DumpRegionDesc(LogType type) const
{
    DLOG(type, "Region index: %zu, type: %s, address: 0x%zx-0x%zx, allocated(B) %zu, live(B) %zu", GetUnitIdx(),
         GetTypeName(), GetRegionStart(), GetRegionEnd(), GetRegionAllocatedSize(), GetLiveByteCount());
}

const char* RegionDesc::GetTypeName() const
{
    static constexpr const char* regionNames[] = {
        "undefined region",
        "thread local region",
        "recent fullregion",
        "from region",
        "unmovable from region",
        "to region",
        "full pinned region",
        "recent pinned region",
        "raw pointer pinned region",
        "tl raw pointer region",
        "large region",
        "recent large region",
        "garbage region",
    };
    return regionNames[static_cast<uint8_t>(GetRegionType())];
}
#endif

void RegionDesc::VisitAllObjects(const std::function<void(BaseObject*)>&& func)
{
    if (IsFixedRegion()) {
        size_t size = static_cast<size_t>(GetRegionCellCount()) * sizeof(uint64_t);
        uintptr_t position = GetRegionStart();
        uintptr_t end = GetRegionEnd();
        while (position < end) {
            BaseObject* obj = reinterpret_cast<BaseObject*>(position);
            position += size;
            if (position > end) {
                break;
            }
            if (IsFreePinnedObject(obj)) {
                continue;
            }
            func(obj);
        }
        return;
    }
    if (IsLargeRegion()) {
        func(reinterpret_cast<BaseObject*>(GetRegionStart()));
    } else if (IsSmallRegion()) {
        uintptr_t position = GetRegionStart();
        uintptr_t allocPtr = GetRegionAllocPtr();
        while (position < allocPtr) {
            // GetAllocSize should before call func, because object maybe destroy in compact gc.
            func(reinterpret_cast<BaseObject*>(position));
            size_t size = RegionSpace::GetAllocSize(*reinterpret_cast<BaseObject*>(position));
            position += size;
        }
    }
}

void RegionDesc::VisitAllObjectsWithFixedSize(size_t cellCount, const std::function<void(BaseObject*)>&& func)
{
    CHECK_CC(GetRegionType() == RegionType::FIXED_PINNED_REGION ||
        GetRegionType() == RegionType::FULL_FIXED_PINNED_REGION);
    size_t size = (cellCount + 1) * sizeof(uint64_t);
    uintptr_t position = GetRegionStart();
    uintptr_t end = GetRegionEnd();
    while (position < end) {
        // GetAllocSize should before call func, because object maybe destroy in compact gc.
        BaseObject* obj = reinterpret_cast<BaseObject*>(position);
        position += size;
        if (position > end) {
            break;
        }
        func(obj);
    }
}

void RegionDesc::VisitAllObjectsBeforeFix(const std::function<void(BaseObject*)>&& func)
{
    if (IsLargeRegion() && !IsNewRegionSinceFix()) {
        func(reinterpret_cast<BaseObject*>(GetRegionStart()));
    } else if (IsSmallRegion()) {
        uintptr_t position = GetRegionStart();
        uintptr_t allocPtr = GetRegionAllocPtr();
        uintptr_t fixline = GetFixLine();

        uintptr_t end = std::min(fixline, allocPtr);
        while (position < end) {
            // GetAllocSize should before call func, because object maybe destroy in compact gc.
            func(reinterpret_cast<BaseObject*>(position));
            size_t size = RegionSpace::GetAllocSize(*reinterpret_cast<BaseObject*>(position));
            position += size;
        }
    }
}

bool RegionDesc::VisitLiveObjectsUntilFalse(const std::function<bool(BaseObject*)>&& func)
{
    // no need to visit this region.
    if (GetLiveByteCount() == 0) {
        return true;
    }

    TraceCollector& collector = reinterpret_cast<TraceCollector&>(Heap::GetHeap().GetCollector());
    if (IsLargeRegion()) {
        return func(reinterpret_cast<BaseObject*>(GetRegionStart()));
    }
    if (IsSmallRegion()) {
        uintptr_t position = GetRegionStart();
        uintptr_t allocPtr = GetRegionAllocPtr();
        while (position < allocPtr) {
            BaseObject* obj = reinterpret_cast<BaseObject*>(position);
            if (collector.IsSurvivedObject(obj) && !func(obj)) { return false; }
            position += RegionSpace::GetAllocSize(*obj);
        }
    }
    return true;
}

void RegionList::MergeRegionList(RegionList& srcList, RegionDesc::RegionType regionType)
{
    RegionList regionList("region list cache");
    srcList.MoveTo(regionList);
    RegionDesc* head = regionList.GetHeadRegion();
    RegionDesc* tail = regionList.GetTailRegion();
    if (head == nullptr) {
        return;
    }
    std::lock_guard<std::mutex> lock(listMutex_);
    regionList.SetElementType(regionType);
    IncCounts(regionList.GetRegionCount(), regionList.GetUnitCount());
    if (listHead_ == nullptr) {
        listHead_ = head;
        listTail_ = tail;
    } else {
        tail->SetNextRegion(listHead_);
        listHead_->SetPrevRegion(tail);
        listHead_ = head;
    }
}

void RegionList::PrependRegion(RegionDesc* region, RegionDesc::RegionType type)
{
    std::lock_guard<std::mutex> lock(listMutex_);
    PrependRegionLocked(region, type);
}

void RegionList::PrependRegionLocked(RegionDesc* region, RegionDesc::RegionType type)
{
    if (region == nullptr) {
        return;
    }

    DLOG(REGION, "%s (%zu, %zu)+(%zu, %zu) prepend region %p@%#zx+%zu type %u->%u", listName_,
        regionCount_, unitCount_, 1llu, region->GetUnitCount(), region, region->GetRegionStart(),
        region->GetRegionAllocatedSize(), region->GetRegionType(), type);

    region->SetRegionType(type);
    region->SetPrevRegion(nullptr);
    IncCounts(1, region->GetUnitCount());
    region->SetNextRegion(listHead_);
    if (listHead_ == nullptr) {
        ASSERT_LOGF(listTail_ == nullptr, "PrependRegion listTail is not null");
        listTail_ = region;
    } else {
        listHead_->SetPrevRegion(region);
    }
    listHead_ = region;
}

void RegionList::DeleteRegionLocked(RegionDesc* del)
{
    ASSERT_LOGF(listHead_ != nullptr && listTail_ != nullptr, "illegal region list");

    RegionDesc* pre = del->GetPrevRegion();
    RegionDesc* next = del->GetNextRegion();

    del->SetNextRegion(nullptr);
    del->SetPrevRegion(nullptr);

    DLOG(REGION, "%s (%zu, %zu)-(%zu, %zu) delete region %p@%#zx+%zu type %u", listName_,
        regionCount_, unitCount_, 1llu, del->GetUnitCount(),
        del, del->GetRegionStart(), del->GetRegionAllocatedSize(), del->GetRegionType());

    DecCounts(1, del->GetUnitCount());

    if (listHead_ == del) { // delete head
        ASSERT_LOGF(pre == nullptr, "Delete Region pre is not null");
        listHead_ = next;
        if (listHead_ == nullptr) { // now empty
            listTail_ = nullptr;
            return;
        }
    } else {
        pre->SetNextRegion(next);
    }

    if (listTail_ == del) { // delete tail
        ASSERT_LOGF(next == nullptr, "Delete Region next is not null");
        listTail_ = pre;
        if (listTail_ == nullptr) { // now empty
            listHead_ = nullptr;
            return;
        }
    } else {
        next->SetPrevRegion(pre);
    }
}

#ifndef NDEBUG
void RegionList::DumpRegionList(const char* msg)
{
    DLOG(REGION, "dump region list %s", msg);
    std::lock_guard<std::mutex> lock(listMutex_);
    for (RegionDesc *region = listHead_; region != nullptr; region = region->GetNextRegion()) {
        DLOG(REGION, "region %p @0x%zx+%zu units [%zu+%zu, %zu) type %u prev %p next %p", region,
            region->GetRegionStart(), region->GetRegionAllocatedSize(),
            region->GetUnitIdx(), region->GetUnitCount(), region->GetUnitIdx() + region->GetUnitCount(),
            region->GetRegionType(), region->GetPrevRegion(), region->GetNextRegion());
    }
}
#endif
inline void RegionManager::TagHugePage(RegionDesc* region, size_t num) const
{
#if defined (__linux__) || defined(PANDA_TARGET_OHOS)
    (void)madvise(reinterpret_cast<void*>(region->GetRegionStart()), num * RegionDesc::UNIT_SIZE, MADV_HUGEPAGE);
#else
    (void)region;
    (void)num;
#endif
}

inline void RegionManager::UntagHugePage(RegionDesc* region, size_t num) const
{
#if defined (__linux__) || defined(PANDA_TARGET_OHOS)
    (void)madvise(reinterpret_cast<void*>(region->GetRegionStart()), num * RegionDesc::UNIT_SIZE, MADV_NOHUGEPAGE);
#else
    (void)region;
    (void)num;
#endif
}

size_t FreeRegionManager::ReleaseGarbageRegions(size_t targetCachedSize)
{
    size_t dirtyBytes = dirtyUnitTree_.GetTotalCount() * RegionDesc::UNIT_SIZE;
    if (dirtyBytes <= targetCachedSize) {
        VLOG(REPORT, "release heap garbage memory 0 bytes, cache %zu(%zu) bytes", dirtyBytes, targetCachedSize);
        return 0;
    }

    size_t releasedBytes = 0;
    while (dirtyBytes > targetCachedSize) {
        std::lock_guard<std::mutex> lock1(dirtyUnitTreeMutex_);
        auto node = dirtyUnitTree_.RootNode();
        if (node == nullptr) { break; }
        uint32_t idx = node->GetIndex();
        uint32_t num = node->GetCount();
        dirtyUnitTree_.ReleaseRootNode();

        std::lock_guard<std::mutex> lock2(releasedUnitTreeMutex_);
        LOGF_CHECK(releasedUnitTree_.MergeInsert(idx, num, true)) <<
            "tid " << GetTid() << ": failed to release garbage units[" <<
            idx << "+" << num << ", " << (idx + num) << ")";
        releasedBytes += (num * RegionDesc::UNIT_SIZE);
        dirtyBytes = dirtyUnitTree_.GetTotalCount() * RegionDesc::UNIT_SIZE;
    }
    VLOG(REPORT, "release heap garbage memory %zu bytes, cache %zu(%zu) bytes",
         releasedBytes, dirtyBytes, targetCachedSize);
    return releasedBytes;
}

void RegionManager::SetMaxUnitCountForRegion()
{
    maxUnitCountPerRegion_ = BaseRuntime::GetInstance()->GetHeapParam().regionSize * KB / RegionDesc::UNIT_SIZE;
}

void RegionManager::SetLargeObjectThreshold()
{
    size_t regionSize = BaseRuntime::GetInstance()->GetHeapParam().regionSize * KB;
    if (regionSize < RegionDesc::LARGE_OBJECT_DEFAULT_THRESHOLD) {
        largeObjectThreshold_ = regionSize;
    } else {
        largeObjectThreshold_ = RegionDesc::LARGE_OBJECT_DEFAULT_THRESHOLD;
    }
}

void RegionManager::SetGarbageThreshold()
{
    fromSpaceGarbageThreshold_ = BaseRuntime::GetInstance()->GetGCParam().garbageThreshold;
}

void RegionManager::Initialize(size_t nRegion, uintptr_t regionInfoAddr)
{
    size_t metadataSize = GetMetadataSize(nRegion);
#ifdef _WIN64
    MemoryMap::CommitMemory(reinterpret_cast<void*>(regionInfoAddr), metadataSize);
#endif
    regionInfoStart_ = regionInfoAddr;
    regionHeapStart_ = regionInfoAddr + metadataSize;
    regionHeapEnd_ = regionHeapStart_ + nRegion * RegionDesc::UNIT_SIZE;
    inactiveZone_ = regionHeapStart_;
    SetMaxUnitCountForRegion();
    SetLargeObjectThreshold();
    SetGarbageThreshold();
    // propagate region heap layout
    RegionDesc::Initialize(nRegion, regionInfoAddr, regionHeapStart_);
    freeRegionManager_.Initialize(nRegion);
    exemptedRegionThreshold_ = BaseRuntime::GetInstance()->GetHeapParam().exemptionThreshold;

    DLOG(REPORT, "region info @0x%zx+%zu, heap [0x%zx, 0x%zx), unit count %zu", regionInfoAddr, metadataSize,
         regionHeapStart_, regionHeapEnd_, nRegion);
}

void RegionManager::ReclaimRegion(RegionDesc* region)
{
    size_t num = region->GetUnitCount();
    size_t unitIndex = region->GetUnitIdx();
    if (num >= HUGE_PAGE) {
        UntagHugePage(region, num);
    }
    DLOG(REGION, "reclaim region %p@%#zx+%zu type %u", region, region->GetRegionStart(),
        region->GetRegionAllocatedSize(), region->GetRegionType());

    region->InitFreeUnits();
    freeRegionManager_.AddGarbageUnits(unitIndex, num);
}

size_t RegionManager::ReleaseRegion(RegionDesc* region)
{
    size_t res = region->GetRegionSize();
    size_t num = region->GetUnitCount();
    size_t unitIndex = region->GetUnitIdx();
    if (num >= HUGE_PAGE) {
        UntagHugePage(region, num);
    }
    DLOG(REGION, "release region %p @%#zx+%zu type %u", region, region->GetRegionStart(),
        region->GetRegionAllocatedSize(), region->GetRegionType());

    region->InitFreeUnits();
    RegionDesc::ReleaseUnits(unitIndex, num);
    freeRegionManager_.AddReleaseUnits(unitIndex, num);
    return res;
}

void RegionManager::ReassembleFromSpace()
{
    fromRegionList_.MergeRegionList(toRegionList_, RegionDesc::RegionType::FROM_REGION);
    fromRegionList_.MergeRegionList(tlToRegionList_, RegionDesc::RegionType::FROM_REGION);
    fromRegionList_.MergeRegionList(exemptedFromRegionList_, RegionDesc::RegionType::FROM_REGION);
}

void RegionManager::CountLiveObject(const BaseObject* obj)
{
    RegionDesc* region = RegionDesc::GetRegionDescAt(reinterpret_cast<HeapAddress>(obj));
    region->AddLiveByteCount(obj->GetSize());
}

void RegionManager::AssembleSmallGarbageCandidates()
{
    fromRegionList_.MergeRegionList(recentFullRegionList_, RegionDesc::RegionType::FROM_REGION);
}

void RegionManager::AssembleLargeGarbageCandidates()
{
    oldLargeRegionList_.MergeRegionList(recentLargeRegionList_, RegionDesc::RegionType::OLD_LARGE_REGION);
}

void RegionManager::AssemblePinnedGarbageCandidates(bool collectAll)
{
    oldPinnedRegionList_.MergeRegionList(recentPinnedRegionList_, RegionDesc::RegionType::FULL_PINNED_REGION);
    RegionDesc* region = oldPinnedRegionList_.GetHeadRegion();
    while (region != nullptr) {
        RegionDesc* nextRegion = region->GetNextRegion();
        if (collectAll && (region->GetRawPointerObjectCount() > 0)) {
            oldPinnedRegionList_.DeleteRegion(region);
            rawPointerRegionList_.PrependRegion(region, RegionDesc::RegionType::RAW_POINTER_REGION);
        }

        region = nextRegion;
    }
    for (size_t i = 0; i < FIXED_PINNED_REGION_COUNT; i++) {
        oldFixedPinnedRegionList_[i]->MergeRegionList(*fixedPinnedRegionList_[i],
            RegionDesc::RegionType::FULL_FIXED_PINNED_REGION);
    }
}

// copy only regions whose garbage bytes is greater than or equal to exemptedRegionThreshold.
void RegionManager::ExemptFromRegions()
{
    size_t forwardBytes = 0;
    size_t floatingGarbage = 0;
    size_t oldFromBytes = fromRegionList_.GetUnitCount() * RegionDesc::UNIT_SIZE;
    RegionDesc* fromRegion = fromRegionList_.GetHeadRegion();
    while (fromRegion != nullptr) {
        size_t threshold = static_cast<size_t>(exemptedRegionThreshold_ * fromRegion->GetRegionSize());
        size_t liveBytes = fromRegion->GetLiveByteCount();
        long rawPtrCnt = fromRegion->GetRawPointerObjectCount();
        if (liveBytes > threshold) { // ignore this region
            RegionDesc* del = fromRegion;
            DLOG(REGION, "region %p @0x%zx+%zu exempted by forwarding: %zu units, %u live bytes", del,
                del->GetRegionStart(), del->GetRegionAllocatedSize(),
                del->GetUnitCount(), del->GetLiveByteCount());

            fromRegion = fromRegion->GetNextRegion();
            if (fromRegionList_.TryDeleteRegion(del, RegionDesc::RegionType::FROM_REGION,
                                                RegionDesc::RegionType::EXEMPTED_FROM_REGION)) {
                ExemptFromRegion(del);
            }
            floatingGarbage += (del->GetRegionSize() - del->GetLiveByteCount());
        } else if (rawPtrCnt > 0) {
            RegionDesc* del = fromRegion;
            DLOG(REGION, "region %p @0x%zx+%zu pinned by forwarding: %zu units, %u live bytes rawPtr cnt %u",
                del, del->GetRegionStart(), del->GetRegionAllocatedSize(),
                del->GetUnitCount(), del->GetLiveByteCount(), rawPtrCnt);

            fromRegion = fromRegion->GetNextRegion();
            if (fromRegionList_.TryDeleteRegion(del, RegionDesc::RegionType::FROM_REGION,
                                                RegionDesc::RegionType::RAW_POINTER_REGION)) {
                rawPointerRegionList_.PrependRegion(del, RegionDesc::RegionType::RAW_POINTER_REGION);
            }
            floatingGarbage += (del->GetRegionSize() - del->GetLiveByteCount());
        } else {
            forwardBytes += fromRegion->GetLiveByteCount();
            fromRegion = fromRegion->GetNextRegion();
        }
    }

    size_t newFromBytes = fromRegionList_.GetUnitCount() * RegionDesc::UNIT_SIZE;
    size_t exemptedFromBytes = exemptedFromRegionList_.GetUnitCount() * RegionDesc::UNIT_SIZE;
    VLOG(REPORT, "exempt from-space: %zu B - %zu B -> %zu B, %zu B floating garbage, %zu B to copy",
         oldFromBytes, exemptedFromBytes, newFromBytes, floatingGarbage, forwardBytes);
}

void RegionManager::ForEachObjectUnsafe(const std::function<void(BaseObject*)>& visitor) const
{
    for (uintptr_t regionAddr = regionHeapStart_; regionAddr < inactiveZone_;) {
        RegionDesc* region = RegionDesc::GetRegionDescAt(regionAddr);
        regionAddr = region->GetRegionEnd();
        if (!region->IsValidRegion() || region->IsFreeRegion() || region -> IsGarbageRegion()) {
            continue;
        }
        region->VisitAllObjects([&visitor](BaseObject* object) { visitor(object); });
    }
}

void RegionManager::ForEachObjectSafe(const std::function<void(BaseObject*)>& visitor) const
{
    ScopedEnterSaferegion enterSaferegion(false);
    ScopedStopTheWorld stw("visit-all-objects");
    ForEachObjectUnsafe(visitor);
}

RegionDesc* RegionManager::TakeRegion(size_t num, RegionDesc::UnitRole type, bool expectPhysicalMem, bool allowGC)
{
    // a chance to invoke heuristic gc.
    if (allowGC && !Heap::GetHeap().IsGcStarted()) {
        Collector& collector = Heap::GetHeap().GetCollector();
        size_t threshold = collector.GetGCStats().GetThreshold();
        size_t allocated = Heap::GetHeap().GetAllocator().GetAllocatedBytes();
        if (allocated >= threshold) {
            DLOG(ALLOC, "request heu gc: allocated %zu, threshold %zu", allocated, threshold);
            collector.RequestGC(GC_REASON_HEU, true);
        }
    }

    // check for allocation since we do not want gc threads and mutators do any harm to each other.
    size_t size = num * RegionDesc::UNIT_SIZE;
    RequestForRegion(size);

    RegionDesc* head = garbageRegionList_.TakeHeadRegion();
    if (head != nullptr) {
        DLOG(REGION, "take garbage region %p@%#zx+%zu", head, head->GetRegionStart(), head->GetRegionSize());
        if (head->GetUnitCount() == num) {
            auto idx = head->GetUnitIdx();
            RegionDesc::ClearUnits(idx, num);
            DLOG(REGION, "reuse garbage region %p@%#zx+%zu", head, head->GetRegionStart(), head->GetRegionSize());
            return RegionDesc::InitRegion(idx, num, type);
        } else {
            DLOG(REGION, "reclaim garbage region %p@%#zx+%zu", head, head->GetRegionStart(), head->GetRegionSize());
            ReclaimRegion(head);
        }
    }

    RegionDesc* region = freeRegionManager_.TakeRegion(num, type, expectPhysicalMem);
    if (region != nullptr) {
        if (num >= HUGE_PAGE) {
            TagHugePage(region, num);
        }
        return region;
    }

    // when free regions are not enough for allocation
    if (num <= GetInactiveUnitCount()) {
        uintptr_t addr = inactiveZone_.fetch_add(size);
        if (addr < regionHeapEnd_ - size) {
            region = RegionDesc::InitRegionAt(addr, num, type);
            size_t idx = region->GetUnitIdx();
#ifdef _WIN64
            MemoryMap::CommitMemory(
                reinterpret_cast<void*>(RegionDesc::GetUnitAddress(idx)), num * RegionDesc::UNIT_SIZE);
#endif
            (void)idx; // eliminate compilation warning
            DLOG(REGION, "take inactive units [%zu+%zu, %zu) at [0x%zx, 0x%zx)", idx, num, idx + num,
                 RegionDesc::GetUnitAddress(idx), RegionDesc::GetUnitAddress(idx + num));
            if (num >= HUGE_PAGE) {
                TagHugePage(region, num);
            }
            if (expectPhysicalMem) {
                RegionDesc::ClearUnits(idx, num);
            }
            return region;
        } else {
            (void)inactiveZone_.fetch_sub(size);
        }
    }

    return nullptr;
}

uintptr_t RegionManager::AllocRegion()
{
    RegionDesc* region = TakeRegion(maxUnitCountPerRegion_, RegionDesc::UnitRole::SMALL_SIZED_UNITS, false, false);
    DCHECK_CC(region != nullptr);

    GCPhase phase = Mutator::GetMutator()->GetMutatorPhase();
    if (phase == GC_PHASE_ENUM || phase == GC_PHASE_MARK || phase == GC_PHASE_REMARK_SATB ||
        phase == GC_PHASE_POST_MARK) {
        region->SetTraceLine();
    } else if (phase == GC_PHASE_PRECOPY || phase == GC_PHASE_COPY) {
        region->SetCopyLine();
    } else if (phase == GC_PHASE_FIX) {
        region->SetCopyLine();
        region->SetFixLine();
    }

    DLOG(REGION, "alloc small object region %p @0x%zx+%zu units[%zu+%zu, %zu) type %u",
        region, region->GetRegionStart(), region->GetRegionSize(),
        region->GetUnitIdx(), region->GetUnitCount(), region->GetUnitIdx() + region->GetUnitCount(),
        region->GetRegionType());
    recentFullRegionList_.PrependRegion(region, RegionDesc::RegionType::RECENT_FULL_REGION);

    uintptr_t start = region->GetRegionStart();
    uintptr_t addr = region->Alloc(region->GetRegionSize());
    DCHECK_CC(addr != 0);

    return start;
}

uintptr_t RegionManager::AllocPinnedRegion()
{
    RegionDesc* region = TakeRegion(maxUnitCountPerRegion_, RegionDesc::UnitRole::SMALL_SIZED_UNITS, false, false);
    DCHECK_CC(region != nullptr);
    GCPhase phase = Mutator::GetMutator()->GetMutatorPhase();
    if (phase == GC_PHASE_ENUM || phase == GC_PHASE_MARK || phase == GC_PHASE_REMARK_SATB ||
        phase == GC_PHASE_POST_MARK) {
        region->SetTraceLine();
    } else if (phase == GC_PHASE_PRECOPY || phase == GC_PHASE_COPY) {
        region->SetCopyLine();
    } else if (phase == GC_PHASE_FIX) {
        region->SetCopyLine();
        region->SetFixLine();
    }

    DLOG(REGION, "alloc pinned region @0x%zx+%zu type %u", region->GetRegionStart(),
         region->GetRegionAllocatedSize(),
         region->GetRegionType());
    recentPinnedRegionList_.PrependRegion(region, RegionDesc::RegionType::RECENT_PINNED_REGION);

    uintptr_t start = region->GetRegionStart();
    uintptr_t addr = region->Alloc(region->GetRegionSize());
    DCHECK_CC(addr != 0);

    return start;
}

uintptr_t RegionManager::AllocLargeReion(size_t size)
{
    return AllocLarge(size, false);
}

void RegionManager::CopyFromRegions(GCThreadPool* threadPool)
{
    if (threadPool != nullptr) {
        int32_t threadNum = threadPool->GetMaxThreadNum() + 1;
        // We won't change fromRegionList during gc, so we can use it without lock.
        size_t totalRegionCount = fromRegionList_.GetRegionCount();
        if (UNLIKELY_CC(totalRegionCount == 0)) {
            return;
        }
        size_t regionCntEachTask = totalRegionCount / static_cast<size_t>(threadNum);
        size_t leftRegionCnt = totalRegionCount % static_cast<size_t>(threadNum);
        RegionDesc* region = fromRegionList_.GetHeadRegion();
        // we start threadPool before adding work so that we can concurrently add tasks;
        threadPool->Start();
        for (int32_t i = 0; i < threadNum; ++i) {
            ASSERT_LOGF(region != nullptr, "from region list records wrong region info");
            RegionDesc* startRegion = region;
            for (size_t count = 0; count < regionCntEachTask; ++count) {
                region = region->GetNextRegion();
            }
            threadPool->AddWork(new (std::nothrow) CopyTask(*this, *startRegion, regionCntEachTask));
        }
        threadPool->AddWork(new (std::nothrow) CopyTask(*this, *region, leftRegionCnt));
        threadPool->WaitFinish();

        // clear thread local region for gc control thread.
        // do not rely on AllocationBuffer::ClearRegion at the end of CopyTask(), which may not be executed.
        AllocationBuffer* allocBuffer = AllocationBuffer::GetAllocBuffer();
        if (LIKELY_CC(allocBuffer != nullptr)) {
            allocBuffer->ClearRegion();
        }
    } else {
        CopyFromRegions();
    }
}

void RegionManager::ExemptFromRegion(RegionDesc* region)
{
    exemptedFromRegionList_.PrependRegion(region, RegionDesc::RegionType::EXEMPTED_FROM_REGION);
}

void RegionManager::CopyFromRegions()
{
    // iterate each region in fromRegionList
    RegionDesc* fromRegion = fromRegionList_.GetHeadRegion();
    while (fromRegion != nullptr) {
        ASSERT_LOGF(fromRegion->IsValidRegion(), "region is not head when get head region of from region list");
        RegionDesc* region = fromRegion;
        fromRegion = fromRegion->GetNextRegion();
        CopyRegion(region);
    }

    VLOG(REPORT, "copy %zu from-region units", fromRegionList_.GetUnitCount());

    AllocationBuffer* allocBuffer = AllocationBuffer::GetAllocBuffer();
    if (LIKELY_CC(allocBuffer != nullptr)) {
        allocBuffer->ClearRegion(); // clear region for next GC
    }
}

static void FixRecentRegion(TraceCollector& collector, RegionDesc* region)
{
    // use fixline to skip new region after fix
    // visit object before fix line to avoid race condition with mutator
    region->VisitAllObjectsBeforeFix([&collector, region](BaseObject* object) {
        if (region->IsNewObjectSinceForward(object)) {
            // handle dead objects in tl-regions for concurrent gc.
            if (collector.IsToVersion(object)) {
                collector.FixObjectRefFields(object);
                object->SetForwardState(panda::BaseStateWord::ForwardState::NORMAL);
            } else {
                DLOG(FIX, "fix: skip new obj %p<%p>(%zu)", object, object->GetTypeInfo(), object->GetSize());
            }
        } else if (region->IsNewObjectSinceTrace(object) || collector.IsSurvivedObject(object)) {
            collector.FixObjectRefFields(object);
        } else { // handle dead objects in tl-regions for concurrent gc.
            FillFreeObject(object, RegionSpace::GetAllocSize(*object));
            DLOG(FIX, "skip dead obj %p<%p>(%zu)", object, object->GetTypeInfo(), object->GetSize());
        }
    });
}

static void FixRecentRegionList(TraceCollector& collector, RegionList& list)
{
    list.VisitAllRegions([&collector](RegionDesc* region) {
        DLOG(REGION, "fix region %p@%#zx+%zu", region, region->GetRegionStart(), region->GetLiveByteCount());
        FixRecentRegion(collector, region);
    });
}

static void FixToRegion(TraceCollector& collector, RegionDesc* region)
{
    region->VisitAllObjects([&collector](BaseObject* object) {
        collector.FixObjectRefFields(object);
    });
}

static void FixToRegionList(TraceCollector& collector, RegionList& list)
{
    list.VisitAllRegions([&collector](RegionDesc* region) {
        DLOG(REGION, "fix region %p@%#zx+%zu", region, region->GetRegionStart(), region->GetLiveByteCount());
        FixToRegion(collector, region);
    });
}

static void FixOldRegion(TraceCollector& collector, RegionDesc* region)
{
    region->VisitAllObjects([&collector](BaseObject* object) {
        if (collector.IsSurvivedObject(object)) {
            collector.FixObjectRefFields(object);
        } else {
            FillFreeObject(object, RegionSpace::GetAllocSize(*object));
            DLOG(FIX, "fix: skip dead old obj %p<%p>(%zu)", object, object->GetTypeInfo(), object->GetSize());
        }
    });
}

void RegionManager::FixFixedRegionList(TraceCollector& collector, RegionList& list, size_t cellCount, GCStats& stats)
{
    size_t garbageSize = 0;
    RegionDesc* region = list.GetHeadRegion();
    while (region != nullptr) {
        if (region->GetLiveByteCount() == 0) {
            RegionDesc* del = region;
            region = region->GetNextRegion();
            list.DeleteRegion(del);

            garbageSize += CollectRegion(del);
            continue;
        }
        region->VisitAllObjectsWithFixedSize(cellCount,
            [&collector, &region, &cellCount, &garbageSize](BaseObject* object) {
            if (collector.IsSurvivedObject(object)) {
                collector.FixObjectRefFields(object);
            } else {
                DLOG(ALLOC, "reclaim pinned obj %p", object);
                garbageSize += (cellCount + 1) * sizeof(uint64_t);
                region->CollectPinnedGarbage(object, cellCount);
            }
        });
        region = region->GetNextRegion();
    }
    stats.pinnedGarbageSize += garbageSize;
}

static void FixOldRegionList(TraceCollector& collector, RegionList& list)
{
    list.VisitAllRegions([&collector](RegionDesc* region) {
        DLOG(REGION, "fix region %p@%#zx+%zu", region, region->GetRegionStart(), region->GetLiveByteCount());
        FixOldRegion(collector, region);
    });
}

void RegionManager::FixOldPinnedRegionList(TraceCollector& collector, RegionList& list, GCStats& stats)
{
    size_t garbageSize = 0;
    RegionDesc* region = list.GetHeadRegion();
    while (region != nullptr) {
        if (region->GetLiveByteCount() == 0) {
            RegionDesc* del = region;
            region = region->GetNextRegion();
            list.DeleteRegion(del);

            garbageSize += CollectRegion(del);
            continue;
        }
        DLOG(REGION, "fix region %p@%#zx+%zu", region, region->GetRegionStart(), region->GetLiveByteCount());
        FixOldRegion(collector, region);
        region = region->GetNextRegion();
    }
    stats.pinnedGarbageSize += garbageSize;
}

void RegionManager::FixAllRegionLists()
{
    TraceCollector& collector = reinterpret_cast<TraceCollector&>(Heap::GetHeap().GetCollector());
    // fix all objects.
    FixToRegionList(collector, toRegionList_);
    FixToRegionList(collector, tlToRegionList_);
    GCStats& stats = Heap::GetHeap().GetCollector().GetGCStats();
    // fix only survived objects.
    FixOldRegionList(collector, exemptedFromRegionList_);
    FixOldRegionList(collector, oldLargeRegionList_);

    // fix survived object but should be with line judgement.
    FixRecentRegionList(collector, tlRegionList_);
    FixRecentRegionList(collector, recentFullRegionList_);
    FixRecentRegionList(collector, recentLargeRegionList_);
    FixRecentRegionList(collector, recentPinnedRegionList_);
    FixRecentRegionList(collector, rawPointerRegionList_);
    FixOldPinnedRegionList(collector, oldPinnedRegionList_, stats);
    for (size_t i = 0; i < FIXED_PINNED_REGION_COUNT; i++) {
        FixFixedRegionList(collector, *oldFixedPinnedRegionList_[i], i, stats);
        FixRecentRegionList(collector, *fixedPinnedRegionList_[i]);
    }
}

size_t RegionManager::CollectLargeGarbage()
{
    size_t garbageSize = 0;
    TraceCollector& collector = reinterpret_cast<TraceCollector&>(Heap::GetHeap().GetCollector());
    RegionDesc* region = oldLargeRegionList_.GetHeadRegion();
    while (region != nullptr) {
        HeapAddress addr = region->GetRegionStart();
        BaseObject* obj = reinterpret_cast<BaseObject*>(addr);
        if (!collector.IsSurvivedObject(obj) && !region->IsNewObjectSinceTrace(obj)) {
            DLOG(REGION, "reclaim large region %p@0x%zx+%zu type %u", region, region->GetRegionStart(),
                 region->GetRegionAllocatedSize(), region->GetRegionType());

            RegionDesc* del = region;
            region = region->GetNextRegion();
            oldLargeRegionList_.DeleteRegion(del);
            if (del->GetRegionSize() > RegionDesc::LARGE_OBJECT_RELEASE_THRESHOLD) {
                garbageSize += ReleaseRegion(del);
            } else {
                garbageSize += CollectRegion(del);
            }
        } else {
            DLOG(REGION, "clear mark-bit for large region %p@0x%zx+%zu type %u", region, region->GetRegionStart(),
                 region->GetRegionAllocatedSize(), region->GetRegionType());
            region = region->GetNextRegion();
        }
    }

    region = recentLargeRegionList_.GetHeadRegion();
    while (region != nullptr) {
        region = region->GetNextRegion();
    }

    return garbageSize;
}

#if defined(GCINFO_DEBUG) && GCINFO_DEBUG
void RegionManager::DumpRegionDesc() const
{
    if (!ENABLE_LOG(ALLOC)) {
        return;
    }
    for (uintptr_t regionAddr = regionHeapStart; regionAddr < inactiveZone;) {
        RegionDesc* region = RegionDesc::GetRegionDescAt(regionAddr);
        regionAddr = region->GetRegionEnd();
        if (!region->IsFreeRegion()) {
            region->DumpRegionDesc(ALLOC);
        }
    }
}
#endif

void RegionManager::DumpRegionStats(const char* msg) const
{
    size_t totalSize = regionHeapEnd_ - regionHeapStart_;
    size_t totalUnits = totalSize / RegionDesc::UNIT_SIZE;
    size_t activeSize = inactiveZone_ - regionHeapStart_;
    size_t activeUnits = activeSize / RegionDesc::UNIT_SIZE;

    size_t tlRegions = tlRegionList_.GetRegionCount();
    size_t tlUnits = tlRegionList_.GetUnitCount();
    size_t tlSize = tlUnits * RegionDesc::UNIT_SIZE;
    size_t allocTLSize = tlRegionList_.GetAllocatedSize();

    size_t fromRegions = fromRegionList_.GetRegionCount();
    size_t fromUnits = fromRegionList_.GetUnitCount();
    size_t fromSize = fromUnits * RegionDesc::UNIT_SIZE;
    size_t allocFromSize = fromRegionList_.GetAllocatedSize();

    size_t exemptedFromRegions = exemptedFromRegionList_.GetRegionCount();
    size_t exemptedFromUnits = exemptedFromRegionList_.GetUnitCount();
    size_t exemptedFromSize = exemptedFromUnits * RegionDesc::UNIT_SIZE;
    size_t allocExemptedFromSize = exemptedFromRegionList_.GetAllocatedSize();

    size_t toRegions = toRegionList_.GetRegionCount() + tlToRegionList_.GetRegionCount();
    size_t toUnits = toRegionList_.GetUnitCount() + tlToRegionList_.GetUnitCount();
    size_t toSize = toUnits * RegionDesc::UNIT_SIZE;
    size_t allocToSize = toRegionList_.GetAllocatedSize() + tlToRegionList_.GetAllocatedSize();

    size_t recentFullRegions = recentFullRegionList_.GetRegionCount();
    size_t recentFullUnits = recentFullRegionList_.GetUnitCount();
    size_t recentFullSize = recentFullUnits * RegionDesc::UNIT_SIZE;
    size_t allocRecentFullSize = recentFullRegionList_.GetAllocatedSize();

    size_t garbageRegions = garbageRegionList_.GetRegionCount();
    size_t garbageUnits = garbageRegionList_.GetUnitCount();
    size_t garbageSize = garbageUnits * RegionDesc::UNIT_SIZE;
    size_t allocGarbageSize = garbageRegionList_.GetAllocatedSize();

    size_t pinnedRegions = oldPinnedRegionList_.GetRegionCount();
    size_t pinnedUnits = oldPinnedRegionList_.GetUnitCount();
    size_t pinnedSize = pinnedUnits * RegionDesc::UNIT_SIZE;
    size_t allocPinnedSize = oldPinnedRegionList_.GetAllocatedSize();

    size_t recentPinnedRegions = recentPinnedRegionList_.GetRegionCount();
    size_t recentPinnedUnits = recentPinnedRegionList_.GetUnitCount();
    size_t recentPinnedSize = recentPinnedUnits * RegionDesc::UNIT_SIZE;
    size_t allocRecentPinnedSize = recentPinnedRegionList_.GetAllocatedSize();

    size_t largeRegions = oldLargeRegionList_.GetRegionCount();
    size_t largeUnits = oldLargeRegionList_.GetUnitCount();
    size_t largeSize = largeUnits * RegionDesc::UNIT_SIZE;
    size_t allocLargeSize = oldLargeRegionList_.GetAllocatedSize();

    size_t recentlargeRegions = recentLargeRegionList_.GetRegionCount();
    size_t recentlargeUnits = recentLargeRegionList_.GetUnitCount();
    size_t recentLargeSize = recentlargeUnits * RegionDesc::UNIT_SIZE;
    size_t allocRecentLargeSize = recentLargeRegionList_.GetAllocatedSize();

    size_t usedUnits = GetUsedUnitCount();
    size_t releasedUnits = freeRegionManager_.GetReleasedUnitCount();
    size_t dirtyUnits = freeRegionManager_.GetDirtyUnitCount();
    size_t listedUnits = fromUnits + exemptedFromUnits + toUnits + garbageUnits +
        recentFullUnits + largeUnits + recentlargeUnits + pinnedUnits + recentPinnedUnits;

    VLOG(REPORT, msg);

    VLOG(REPORT, "\ttotal units: %zu (%zu B)", totalUnits, totalSize);
    VLOG(REPORT, "\tactive units: %zu (%zu B)", activeUnits, activeSize);

    VLOG(REPORT, "\ttl-regions %zu: %zu units (%zu B, alloc %zu)", tlRegions,  tlUnits, tlSize, allocTLSize);
    VLOG(REPORT, "\tfrom-regions %zu: %zu units (%zu B, alloc %zu)", fromRegions,  fromUnits, fromSize, allocFromSize);
    VLOG(REPORT, "\texempted from-regions %zu: %zu units (%zu B, alloc %zu)",
         exemptedFromRegions, exemptedFromUnits, exemptedFromSize, allocExemptedFromSize);
    VLOG(REPORT, "\tto-regions %zu: %zu units (%zu B, alloc %zu)", toRegions, toUnits, toSize, allocToSize);
    VLOG(REPORT, "\trecent-full regions %zu: %zu units (%zu B, alloc %zu)",
         recentFullRegions, recentFullUnits, recentFullSize, allocRecentFullSize);
    VLOG(REPORT, "\tgarbage regions %zu: %zu units (%zu B, alloc %zu)",
         garbageRegions, garbageUnits, garbageSize, allocGarbageSize);
    VLOG(REPORT, "\tpinned regions %zu: %zu units (%zu B, alloc %zu)",
         pinnedRegions, pinnedUnits, pinnedSize, allocPinnedSize);
    VLOG(REPORT, "\trecent pinned regions %zu: %zu units (%zu B, alloc %zu)",
         recentPinnedRegions, recentPinnedUnits, recentPinnedSize, allocRecentPinnedSize);
    VLOG(REPORT, "\tlarge-object regions %zu: %zu units (%zu B, alloc %zu)",
         largeRegions, largeUnits, largeSize, allocLargeSize);
    VLOG(REPORT, "\trecent large-object regions %zu: %zu units (%zu B, alloc %zu)",
         recentlargeRegions, recentlargeUnits, recentLargeSize, allocRecentLargeSize);

    VLOG(REPORT, "\tlisted units: %zu (%zu B)", listedUnits, listedUnits * RegionDesc::UNIT_SIZE);
    VLOG(REPORT, "\tused units: %zu (%zu B)", usedUnits, usedUnits * RegionDesc::UNIT_SIZE);
    VLOG(REPORT, "\treleased units: %zu (%zu B)", releasedUnits, releasedUnits * RegionDesc::UNIT_SIZE);
    VLOG(REPORT, "\tdirty units: %zu (%zu B)", dirtyUnits, dirtyUnits * RegionDesc::UNIT_SIZE);

    OHOS_HITRACE_COUNT("Ark_GC_totalSize", totalSize);
    OHOS_HITRACE_COUNT("Ark_GC_totalUnits", totalUnits);
    OHOS_HITRACE_COUNT("Ark_GC_activeSize", activeSize);
    OHOS_HITRACE_COUNT("Ark_GC_activeUnits", activeUnits);
    OHOS_HITRACE_COUNT("Ark_GC_tlRegions", tlRegions);
    OHOS_HITRACE_COUNT("Ark_GC_tlUnits", tlUnits);
    OHOS_HITRACE_COUNT("Ark_GC_tlSize", tlSize);
    OHOS_HITRACE_COUNT("Ark_GC_allocTLSize", allocTLSize);
    OHOS_HITRACE_COUNT("Ark_GC_fromRegions", fromRegions);
    OHOS_HITRACE_COUNT("Ark_GC_fromUnits", fromUnits);
    OHOS_HITRACE_COUNT("Ark_GC_fromSize", fromSize);
    OHOS_HITRACE_COUNT("Ark_GC_allocFromSize", allocFromSize);
    OHOS_HITRACE_COUNT("Ark_GC_exemptedFromRegions", exemptedFromRegions);
    OHOS_HITRACE_COUNT("Ark_GC_exemptedFromUnits", exemptedFromUnits);
    OHOS_HITRACE_COUNT("Ark_GC_exemptedFromSize", exemptedFromSize);
    OHOS_HITRACE_COUNT("Ark_GC_allocExemptedFromSize", allocExemptedFromSize);
    OHOS_HITRACE_COUNT("Ark_GC_toRegions", toRegions);
    OHOS_HITRACE_COUNT("Ark_GC_toUnits", toUnits);
    OHOS_HITRACE_COUNT("Ark_GC_toSize", toSize);
    OHOS_HITRACE_COUNT("Ark_GC_allocToSize", allocToSize);
    OHOS_HITRACE_COUNT("Ark_GC_recentFullRegions", recentFullRegions);
    OHOS_HITRACE_COUNT("Ark_GC_recentFullUnits", recentFullUnits);
    OHOS_HITRACE_COUNT("Ark_GC_recentFullSize", recentFullSize);
    OHOS_HITRACE_COUNT("Ark_GC_allocRecentFullSize", allocRecentFullSize);
    OHOS_HITRACE_COUNT("Ark_GC_garbageRegions", garbageRegions);
    OHOS_HITRACE_COUNT("Ark_GC_garbageUnits", garbageUnits);
    OHOS_HITRACE_COUNT("Ark_GC_garbageSize", garbageSize);
    OHOS_HITRACE_COUNT("Ark_GC_allocGarbageSize", allocGarbageSize);
    OHOS_HITRACE_COUNT("Ark_GC_pinnedRegions", pinnedRegions);
    OHOS_HITRACE_COUNT("Ark_GC_pinnedUnits", pinnedUnits);
    OHOS_HITRACE_COUNT("Ark_GC_pinnedSize", pinnedSize);
    OHOS_HITRACE_COUNT("Ark_GC_allocPinnedSize", allocPinnedSize);
    OHOS_HITRACE_COUNT("Ark_GC_recentPinnedRegions", recentPinnedRegions);
    OHOS_HITRACE_COUNT("Ark_GC_recentPinnedUnits", recentPinnedUnits);
    OHOS_HITRACE_COUNT("Ark_GC_recentPinnedSize", recentPinnedSize);
    OHOS_HITRACE_COUNT("Ark_GC_allocRecentPinnedSize", allocRecentPinnedSize);
    OHOS_HITRACE_COUNT("Ark_GC_largeRegions", largeRegions);
    OHOS_HITRACE_COUNT("Ark_GC_largeUnits", largeUnits);
    OHOS_HITRACE_COUNT("Ark_GC_largeSize", largeSize);
    OHOS_HITRACE_COUNT("Ark_GC_allocLargeSize", allocLargeSize);
    OHOS_HITRACE_COUNT("Ark_GC_recentlargeRegions", recentlargeRegions);
    OHOS_HITRACE_COUNT("Ark_GC_recentlargeUnits", recentlargeUnits);
    OHOS_HITRACE_COUNT("Ark_GC_recentLargeSize", recentLargeSize);
    OHOS_HITRACE_COUNT("Ark_GC_allocRecentLargeSize", allocRecentLargeSize);
    OHOS_HITRACE_COUNT("Ark_GC_usedUnits", usedUnits);
    OHOS_HITRACE_COUNT("Ark_GC_releasedUnits", releasedUnits);
    OHOS_HITRACE_COUNT("Ark_GC_dirtyUnits", dirtyUnits);
    OHOS_HITRACE_COUNT("Ark_GC_listedUnits", listedUnits);
}

RegionDesc* RegionManager::AllocateThreadLocalRegion(bool expectPhysicalMem)
{
    RegionDesc* region = TakeRegion(maxUnitCountPerRegion_, RegionDesc::UnitRole::SMALL_SIZED_UNITS, expectPhysicalMem);
    if (region != nullptr) {
        if (IsGcThread()) {
            tlToRegionList_.PrependRegion(region, RegionDesc::RegionType::TO_REGION);
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
                region->SetCopyLine();
            } else if (phase == GC_PHASE_FIX) {
                region->SetCopyLine();
                region->SetFixLine();
            }

            tlRegionList_.PrependRegion(region, RegionDesc::RegionType::THREAD_LOCAL_REGION);
            DLOG(REGION, "alloc tl-region %p @0x%zx+%zu units[%zu+%zu, %zu) type %u, gc phase: %u",
                region, region->GetRegionStart(), region->GetRegionSize(),
                region->GetUnitIdx(), region->GetUnitCount(), region->GetUnitIdx() + region->GetUnitCount(),
                region->GetRegionType(), phase);
        }
    }

    return region;
}

void RegionManager::RequestForRegion(size_t size)
{
    if (IsGcThread()) {
        // gc thread is always permitted for allocation.
        return;
    }

    Heap& heap = Heap::GetHeap();
    GCStats& gcstats = heap.GetCollector().GetGCStats();
    size_t allocatedBytes = GetAllocatedSize() - gcstats.liveBytesAfterGC;
    constexpr double pi = 3.14;
    size_t availableBytesAfterGC = heap.GetMaxCapacity() - gcstats.liveBytesAfterGC;
    double heuAllocRate = std::cos((pi / 2.0) * allocatedBytes / availableBytesAfterGC) * gcstats.collectionRate;
    // for maximum performance, choose the larger one.
    double allocRate = std::max(
        static_cast<double>(BaseRuntime::GetInstance()->GetHeapParam().allocationRate) * MB / SECOND_TO_NANO_SECOND,
        heuAllocRate);
    ASSERT_LOGF(allocRate > 0.00001, "allocRate is zero"); // If it is less than 0.00001, it is considered as 0
    size_t waitTime = static_cast<size_t>(size / allocRate);
    uint64_t now = TimeUtil::NanoSeconds();
    if (prevRegionAllocTime_ + waitTime <= now) {
        prevRegionAllocTime_ = TimeUtil::NanoSeconds();
        return;
    }

    uint64_t sleepTime = std::min<uint64_t>(BaseRuntime::GetInstance()->GetHeapParam().allocationWaitTime,
                                  prevRegionAllocTime_ + waitTime - now);
    DLOG(ALLOC, "wait %zu ns to alloc %zu(B)", sleepTime, size);
    std::this_thread::sleep_for(std::chrono::nanoseconds{ sleepTime });
    prevRegionAllocTime_ = TimeUtil::NanoSeconds();
}

void RegionManager::CopyRegion(RegionDesc* region)
{
    LOGF_CHECK(region->IsFromRegion()) << "region type " << static_cast<uint8_t>(region->GetRegionType());

    DLOG(COPY, "try copy region %p @0x%zx+%zu type %u, live bytes %u",
        region, region->GetRegionStart(), region->GetRegionAllocatedSize(),
        region->GetRegionType(), region->GetLiveByteCount());

    if (region->GetLiveByteCount() == 0) {
        return;
    }

    int32_t rawPointerCount = region->GetRawPointerObjectCount();
    CHECK_CC(rawPointerCount == 0);
    Collector& collector = Heap::GetHeap().GetCollector();
    bool forwarded = region->VisitLiveObjectsUntilFalse(
        [&collector](BaseObject* obj) { return collector.ForwardObject(obj); });
    if (forwarded) {
        // region->SetRouteState(RegionDesc::RouteState::COPYED)
        // CollectRegion(region)
    } else {
        DLOG(COPY, "failure to copy region %p @0x%zx+%zu units[%zu+%zu, %zu) type %u, %u live bytes",
            region, region->GetRegionStart(), region->GetRegionAllocatedSize(),
            region->GetUnitIdx(), region->GetUnitCount(), region->GetUnitIdx() + region->GetUnitCount(),
            region->GetRegionType(), region->GetLiveByteCount());

        fromRegionList_.DeleteRegion(region);
        // since this region is possibly partially-forwarded, treat it as to-region so its state-word can be restored.
        EnlistToRegion(region);
    }
}

uintptr_t RegionManager::AllocPinnedFromFreeList(size_t cellCount)
{
    GCPhase mutatorPhase = Mutator::GetMutator()->GetMutatorPhase();
    RegionList* list = oldFixedPinnedRegionList_[cellCount];
    std::lock_guard<std::mutex> lock(list->GetListMutex());
    uintptr_t allocPtr = list->AllocFromFreeListInLock();
    // For making bitmap comform with live object count, do not mark object repeated.
    if (allocPtr == 0 || mutatorPhase == GCPhase::GC_PHASE_IDLE) {
        return allocPtr;
    }

    // Mark new allocated pinned object.
    BaseObject* object = reinterpret_cast<BaseObject*>(allocPtr);
    (reinterpret_cast<TraceCollector*>(&Heap::GetHeap().GetCollector()))->MarkObject(object, cellCount);
    return allocPtr;
}
} // namespace panda
