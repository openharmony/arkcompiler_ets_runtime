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

#include "common_components/heap/allocator/region_manager.h"

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <unistd.h>

#include "common_components/base_runtime/hooks.h"
#include "common_components/heap/allocator/region_desc.h"
#include "common_components/heap/allocator/region_space.h"
#include "common_components/base/c_string.h"
#include "common_components/heap/collector/collector.h"
#include "common_components/heap/collector/trace_collector.h"
#include "common_components/common/base_object.h"
#include "common_components/common/scoped_object_access.h"
#include "common_components/heap/heap.h"
#include "common_components/mutator/mutator.inline.h"
#include "common_components/mutator/mutator_manager.h"

#if defined(COMMON_TSAN_SUPPORT)
#include "common_components/sanitizer/sanitizer_interface.h"
#endif
#include "common_components/log/log.h"
#include "common_components/taskpool/taskpool.h"
#include "common_interfaces/base_runtime.h"

#if defined(_WIN64)
#include <sysinfoapi.h>
#endif

namespace common {
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
const size_t COMMON_PAGE_SIZE = GetPageSize();
const size_t AllocatorUtils::ALLOC_PAGE_SIZE = COMMON_PAGE_SIZE;
// region unit size: same as system page size
const size_t RegionDesc::UNIT_SIZE = COMMON_PAGE_SIZE;
// regarding a object as a large object when the size is greater than 32KB or one page size,
// depending on the system page size.
const size_t RegionDesc::LARGE_OBJECT_DEFAULT_THRESHOLD = COMMON_PAGE_SIZE > (32 * KB) ?
                                                            COMMON_PAGE_SIZE : 32 * KB;
// max size of per region is 128KB.
const size_t RegionManager::MAX_UNIT_COUNT_PER_REGION = (128 * KB) / COMMON_PAGE_SIZE;
// size of huge page is 2048KB.
const size_t RegionManager::HUGE_PAGE = (2048 * KB) / COMMON_PAGE_SIZE;

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
        size_t size = static_cast<size_t>(GetRegionCellCount() + 1) * sizeof(uint64_t);
        uintptr_t position = GetRegionStart();
        uintptr_t end = GetRegionEnd();
        if (!IsFixedRegionFlag()) {
            end = GetRegionAllocPtr();
        }
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
        if (IsJitFortAwaitInstallFlag()) {
            return;
        }
        func(reinterpret_cast<BaseObject *>(GetRegionStart()));
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

void RegionDesc::VisitAllObjectsBeforeFix(const std::function<void(BaseObject *)> &&func)
{
    if (IsLargeRegion()) {
        if (IsJitFortAwaitInstallFlag()) {
            return;
        }
        if (!IsNewRegionSinceFix()) {
            func(reinterpret_cast<BaseObject *>(GetRegionStart()));
        }
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
        if (IsJitFortAwaitInstallFlag()) {
            return true;
        }
        return func(reinterpret_cast<BaseObject *>(GetRegionStart()));
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

void RegionDesc::VisitRememberSet(const std::function<void(BaseObject*)>& func)
{
    RegionRSet* rSet = GetRSet();
    if (IsLargeRegion()) {
        if (rSet->IsMarkedCard(0)) {
            func(reinterpret_cast<BaseObject*>(GetRegionStart()));
        }
        return;
    }
    rSet->VisitAllMarkedCard(func, GetRegionStart());
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

static const char *RegionDescRegionTypeToString(RegionDesc::RegionType type)
{
    static constexpr const char *enumStr[] = {
        [static_cast<uint8_t>(RegionDesc::RegionType::FREE_REGION)] = "FREE_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::THREAD_LOCAL_REGION)] = "THREAD_LOCAL_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::RECENT_FULL_REGION)] = "RECENT_FULL_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::FROM_REGION)] = "FROM_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::LONE_FROM_REGION)] = "LONE_FROM_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::EXEMPTED_FROM_REGION)] = "EXEMPTED_FROM_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::TO_REGION)] = "TO_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::FULL_PINNED_REGION)] = "FULL_PINNED_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::RECENT_PINNED_REGION)] = "RECENT_PINNED_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::FIXED_PINNED_REGION)] = "FIXED_PINNED_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::FULL_FIXED_PINNED_REGION)] = "FULL_FIXED_PINNED_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::RAW_POINTER_REGION)] = "RAW_POINTER_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::TL_RAW_POINTER_REGION)] = "TL_RAW_POINTER_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::RECENT_LARGE_REGION)] = "RECENT_LARGE_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::LARGE_REGION)] = "LARGE_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::GARBAGE_REGION)] = "GARBAGE_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::READ_ONLY_REGION)] = "READ_ONLY_REGION",
        [static_cast<uint8_t>(RegionDesc::RegionType::APPSPAWN_REGION)] = "APPSPAWN_REGION",
    };
    return enumStr[static_cast<uint8_t>(type)];
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

    os::PrctlSetVMA(reinterpret_cast<void *>(region->GetRegionStart()), region->GetRegionAllocatedSize(),
                    (std::string("ArkTS Heap CMCGC Region ") + RegionDescRegionTypeToString(type)).c_str());

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
        VLOG(DEBUG, "release heap garbage memory 0 bytes, cache %zu(%zu) bytes", dirtyBytes, targetCachedSize);
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
    VLOG(DEBUG, "release heap garbage memory %zu bytes, cache %zu(%zu) bytes",
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
    // propagate region heap layout
    RegionDesc::Initialize(nRegion, regionInfoAddr, regionHeapStart_);
    freeRegionManager_.Initialize(nRegion);

    DLOG(REPORT, "region info @0x%zx+%zu, heap [0x%zx, 0x%zx), unit count %zu", regionInfoAddr, metadataSize,
         regionHeapStart_, regionHeapEnd_, nRegion);
#ifdef USE_HWASAN
    ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<const volatile void *>(regionInfoAddr),
        metadataSize + nRegion * RegionDesc::UNIT_SIZE);
    const uintptr_t p_addr = regionInfoAddr;
    const uintptr_t p_size = metadataSize + nRegion * RegionDesc::UNIT_SIZE;
    LOG_COMMON(DEBUG) << std::hex << "set [" << p_addr <<
                         std::hex << ", " << p_addr + p_size << ") unpoisoned\n";
#endif
}

void RegionManager::ReclaimRegion(RegionDesc* region)
{
    size_t num = region->GetUnitCount();
    size_t unitIndex = region->GetUnitIdx();
    if (num >= HUGE_PAGE) {
        UntagHugePage(region, num);
    }
    DLOG(REGION, "reclaim region %p@%#zx+%zu type %u", region,
         region->GetRegionStart(), region->GetRegionAllocatedSize(),
         region->GetRegionType());

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

void RegionManager::CountLiveObject(const BaseObject* obj)
{
    RegionDesc* region = RegionDesc::GetRegionDescAt(reinterpret_cast<HeapAddress>(obj));
    region->AddLiveByteCount(obj->GetSize());
}

void RegionManager::AssembleLargeGarbageCandidates()
{
    largeRegionList_.MergeRegionList(recentLargeRegionList_, RegionDesc::RegionType::LARGE_REGION);
}

void RegionManager::AssemblePinnedGarbageCandidates()
{
    pinnedRegionList_.MergeRegionList(recentPinnedRegionList_, RegionDesc::RegionType::FULL_PINNED_REGION);
    RegionDesc* region = pinnedRegionList_.GetHeadRegion();
    for (size_t i = 0; i < FIXED_PINNED_REGION_COUNT; i++) {
        fixedPinnedRegionList_[i]->MergeRegionList(*recentFixedPinnedRegionList_[i],
            RegionDesc::RegionType::FULL_FIXED_PINNED_REGION);
    }
}

void RegionManager::ClearRSet()
{
    auto clearFunc = [](RegionDesc* region) {
        region->ClearRSet();
    };
    recentPinnedRegionList_.VisitAllRegions(clearFunc);
    pinnedRegionList_.VisitAllRegions(clearFunc);
    recentLargeRegionList_.VisitAllRegions(clearFunc);
    largeRegionList_.VisitAllRegions(clearFunc);
    rawPointerRegionList_.VisitAllRegions(clearFunc);
    appSpawnRegionList_.VisitAllRegions(clearFunc);
    for (size_t i = 0; i < FIXED_PINNED_REGION_COUNT; i++) {
        recentFixedPinnedRegionList_[i]->VisitAllRegions(clearFunc);
        fixedPinnedRegionList_[i]->VisitAllRegions(clearFunc);
    }
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

void RegionManager::ForEachAwaitingJitFortUnsafe(const std::function<void(BaseObject*)>& visitor) const
{
    ASSERT(BaseRuntime::GetInstance()->GetMutatorManager().WorldStopped());
    for (const auto jitFort : awaitingJitFort_) {
        visitor(jitFort);
    }
}

RegionDesc *RegionManager::TakeRegion(size_t num, RegionDesc::UnitRole type, bool expectPhysicalMem, bool allowGC)
{
    // a chance to invoke heuristic gc.
    if (allowGC && !Heap::GetHeap().IsGcStarted()) {
        Heap::GetHeap().TryHeuristicGC();
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

static void FixRecentRegion(TraceCollector& collector, RegionDesc* region)
{
    // use fixline to skip new region after fix
    // visit object before fix line to avoid race condition with mutator
    region->VisitAllObjectsBeforeFix([&collector, region](BaseObject* object) {
        if (region->IsNewObjectSinceForward(object)) {
            // handle dead objects in tl-regions for concurrent gc.
            if (collector.IsToVersion(object)) {
                collector.FixObjectRefFields(object);
                object->SetForwardState(BaseStateWord::ForwardState::NORMAL);
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

void RegionManager::FixRecentRegionList(TraceCollector& collector, RegionList& list)
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

void RegionManager::FixToRegionList(TraceCollector& collector, RegionList& list)
{
    list.VisitAllRegions([&collector](RegionDesc* region) {
        DLOG(REGION, "fix region %p@%#zx+%zu", region, region->GetRegionStart(), region->GetLiveByteCount());
        FixToRegion(collector, region);
    });
}

void RegionManager::FixFixedRegionList(TraceCollector& collector, RegionList& list, size_t cellCount, GCStats& stats)
{
    size_t garbageSize = 0;
    RegionDesc* region = list.GetHeadRegion();
    while (region != nullptr) {
        auto liveBytes = region->GetLiveByteCount();
        if (liveBytes == 0) {
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
        region->SetFixedRegionFlag(1);
        region = region->GetNextRegion();
    }
    stats.pinnedGarbageSize += garbageSize;
}

static void FixRegion(TraceCollector& collector, RegionDesc* region)
{
    region->VisitAllObjects([&collector](BaseObject* object) {
        if (collector.IsSurvivedObject(object)) {
            collector.FixObjectRefFields(object);
        } else {
            FillFreeObject(object, RegionSpace::GetAllocSize(*object));
            DLOG(FIX, "fix: skip dead obj %p<%p>(%zu)", object, object->GetTypeInfo(), object->GetSize());
        }
    });
}

void RegionManager::FixRegionList(TraceCollector& collector, RegionList& list)
{
    list.VisitAllRegions([&collector](RegionDesc* region) {
        DLOG(REGION, "fix region %p@%#zx+%zu", region, region->GetRegionStart(), region->GetLiveByteCount());
        FixRegion(collector, region);
    });
}

static void FixOldRegion(TraceCollector& collector, RegionDesc* region)
{
    region->VisitAllObjects([&collector, &region](BaseObject* object) {
        if (region->IsNewObjectSinceTrace(object) || collector.IsSurvivedObject(object) || region->IsInRSet(object)) {
            DLOG(FIX, "fix: mature obj %p<%p>(%zu)", object, object->GetTypeInfo(), object->GetSize());
            collector.FixObjectRefFields(object);
        }
    });
}

void RegionManager::FixOldRegionList(TraceCollector& collector, RegionList& list)
{
    list.VisitAllRegions([&collector](RegionDesc* region) {
        DLOG(REGION, "fix mature region %p@%#zx+%zu", region, region->GetRegionStart(), region->GetLiveByteCount());
        FixOldRegion(collector, region);
    });
}

static void FixRecentOldRegion(TraceCollector& collector, RegionDesc* region)
{
    region->VisitAllObjectsBeforeFix([&collector, &region](BaseObject* object) {
        if (region->IsNewObjectSinceTrace(object) || collector.IsSurvivedObject(object) || region->IsInRSet(object)) {
            DLOG(FIX, "fix: mature obj %p<%p>(%zu)", object, object->GetTypeInfo(), object->GetSize());
            collector.FixObjectRefFields(object);
        }
    });
}

void RegionManager::FixRecentOldRegionList(TraceCollector& collector, RegionList& list)
{
    list.VisitAllRegions([&collector](RegionDesc* region) {
        DLOG(REGION, "fix mature region %p@%#zx+%zu", region, region->GetRegionStart(), region->GetLiveByteCount());
        FixRecentOldRegion(collector, region);
    });
}

void RegionManager::FixPinnedRegionList(TraceCollector& collector, RegionList& list, GCStats& stats)
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
        FixRegion(collector, region);
        region = region->GetNextRegion();
    }
    stats.pinnedGarbageSize += garbageSize;
}

void RegionManager::FixAllRegionLists()
{
    TraceCollector& collector = reinterpret_cast<TraceCollector&>(Heap::GetHeap().GetCollector());

    // fix all objects.
    if (Heap::GetHeap().GetGCReason() == GC_REASON_YOUNG) {
        FixOldRegionList(collector, largeRegionList_);
        FixOldRegionList(collector, appSpawnRegionList_);

        // fix survived object but should be with line judgement.
        FixRecentOldRegionList(collector, recentLargeRegionList_);
        FixRecentOldRegionList(collector, recentPinnedRegionList_);
        FixRecentOldRegionList(collector, rawPointerRegionList_);
        FixOldRegionList(collector, pinnedRegionList_);
        for (size_t i = 0; i < FIXED_PINNED_REGION_COUNT; i++) {
            FixRecentOldRegionList(collector, *recentFixedPinnedRegionList_[i]);
            FixOldRegionList(collector, *fixedPinnedRegionList_[i]);
        }
        return;
    }
    GCStats& stats = Heap::GetHeap().GetCollector().GetGCStats();
    // fix only survived objects.
    FixRegionList(collector, largeRegionList_);
    FixRegionList(collector, appSpawnRegionList_);

    // fix survived object but should be with line judgement.
    FixRecentRegionList(collector, recentLargeRegionList_);
    FixRecentRegionList(collector, recentPinnedRegionList_);
    FixRecentRegionList(collector, rawPointerRegionList_);
    FixPinnedRegionList(collector, pinnedRegionList_, stats);
    for (size_t i = 0; i < FIXED_PINNED_REGION_COUNT; i++) {
        FixRecentRegionList(collector, *recentFixedPinnedRegionList_[i]);
        FixFixedRegionList(collector, *fixedPinnedRegionList_[i], i, stats);
    }
}

size_t RegionManager::CollectLargeGarbage()
{
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::CollectLargeGarbage", "");
    size_t garbageSize = 0;
    TraceCollector& collector = reinterpret_cast<TraceCollector&>(Heap::GetHeap().GetCollector());
    RegionDesc* region = largeRegionList_.GetHeadRegion();
    while (region != nullptr) {
        HeapAddress addr = region->GetRegionStart();
        BaseObject *obj = reinterpret_cast<BaseObject *>(addr);

        if (region->IsJitFortAwaitInstallFlag()) {
                region = region->GetNextRegion();
                continue;
        }
        if (!collector.IsSurvivedObject(obj) && !region->IsNewObjectSinceTrace(obj)) {
            DLOG(REGION, "reclaim large region %p@0x%zx+%zu type %u", region, region->GetRegionStart(),
                 region->GetRegionAllocatedSize(), region->GetRegionType());

            RegionDesc* del = region;
            region = region->GetNextRegion();
            largeRegionList_.DeleteRegion(del);
            if (IsMachineCodeObject(reinterpret_cast<HeapAddress>(obj))) {
                JitFortUnProt(del->GetRegionSize(), reinterpret_cast<void*>(obj));
            }
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

void RegionManager::DumpRegionStats() const
{
    size_t totalSize = regionHeapEnd_ - regionHeapStart_;
    size_t totalUnits = totalSize / RegionDesc::UNIT_SIZE;
    size_t activeSize = inactiveZone_ - regionHeapStart_;
    size_t activeUnits = activeSize / RegionDesc::UNIT_SIZE;

    size_t garbageRegions = garbageRegionList_.GetRegionCount();
    size_t garbageUnits = garbageRegionList_.GetUnitCount();
    size_t garbageSize = garbageUnits * RegionDesc::UNIT_SIZE;
    size_t allocGarbageSize = garbageRegionList_.GetAllocatedSize();

    size_t pinnedRegions = pinnedRegionList_.GetRegionCount();
    size_t pinnedUnits = pinnedRegionList_.GetUnitCount();
    size_t pinnedSize = pinnedUnits * RegionDesc::UNIT_SIZE;
    size_t allocPinnedSize = pinnedRegionList_.GetAllocatedSize();

    size_t recentPinnedRegions = recentPinnedRegionList_.GetRegionCount();
    size_t recentPinnedUnits = recentPinnedRegionList_.GetUnitCount();
    size_t recentPinnedSize = recentPinnedUnits * RegionDesc::UNIT_SIZE;
    size_t allocRecentPinnedSize = recentPinnedRegionList_.GetAllocatedSize();

    size_t largeRegions = largeRegionList_.GetRegionCount();
    size_t largeUnits = largeRegionList_.GetUnitCount();
    size_t largeSize = largeUnits * RegionDesc::UNIT_SIZE;
    size_t allocLargeSize = largeRegionList_.GetAllocatedSize();

    size_t recentlargeRegions = recentLargeRegionList_.GetRegionCount();
    size_t recentlargeUnits = recentLargeRegionList_.GetUnitCount();
    size_t recentLargeSize = recentlargeUnits * RegionDesc::UNIT_SIZE;
    size_t allocRecentLargeSize = recentLargeRegionList_.GetAllocatedSize();

    size_t releasedUnits = freeRegionManager_.GetReleasedUnitCount();
    size_t dirtyUnits = freeRegionManager_.GetDirtyUnitCount();

    VLOG(DEBUG, "\ttotal units: %zu (%zu B)", totalUnits, totalSize);
    VLOG(DEBUG, "\tactive units: %zu (%zu B)", activeUnits, activeSize);

    VLOG(DEBUG, "\tgarbage regions %zu: %zu units (%zu B, alloc %zu)",
         garbageRegions, garbageUnits, garbageSize, allocGarbageSize);
    VLOG(DEBUG, "\tpinned regions %zu: %zu units (%zu B, alloc %zu)",
         pinnedRegions, pinnedUnits, pinnedSize, allocPinnedSize);
    VLOG(DEBUG, "\trecent pinned regions %zu: %zu units (%zu B, alloc %zu)",
         recentPinnedRegions, recentPinnedUnits, recentPinnedSize, allocRecentPinnedSize);
    VLOG(DEBUG, "\tlarge-object regions %zu: %zu units (%zu B, alloc %zu)",
         largeRegions, largeUnits, largeSize, allocLargeSize);
    VLOG(DEBUG, "\trecent large-object regions %zu: %zu units (%zu B, alloc %zu)",
         recentlargeRegions, recentlargeUnits, recentLargeSize, allocRecentLargeSize);

    VLOG(DEBUG, "\treleased units: %zu (%zu B)", releasedUnits, releasedUnits * RegionDesc::UNIT_SIZE);
    VLOG(DEBUG, "\tdirty units: %zu (%zu B)", dirtyUnits, dirtyUnits * RegionDesc::UNIT_SIZE);
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

uintptr_t RegionManager::AllocPinnedFromFreeList(size_t cellCount)
{
    GCPhase mutatorPhase = Mutator::GetMutator()->GetMutatorPhase();
    // workaround: make sure once fixline is set, newly allocated objects are after fixline
    if (mutatorPhase == GC_PHASE_FIX) {
        return 0;
    }

    RegionList* list = fixedPinnedRegionList_[cellCount];
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

uintptr_t RegionManager::AllocReadOnly(size_t size, bool allowGC)
{
    uintptr_t addr = 0;
    std::mutex& regionListMutex = readOnlyRegionList_.GetListMutex();

    std::lock_guard<std::mutex> lock(regionListMutex);
    RegionDesc* headRegion = readOnlyRegionList_.GetHeadRegion();
    if (headRegion != nullptr) {
        addr = headRegion->Alloc(size);
    }
    if (addr == 0) {
        RegionDesc* region =
            TakeRegion(maxUnitCountPerRegion_, RegionDesc::UnitRole::SMALL_SIZED_UNITS, false, allowGC);
        if (region == nullptr) {
            return 0;
        }
        DLOG(REGION, "alloc read only region @0x%zx+%zu type %u", region->GetRegionStart(),
             region->GetRegionAllocatedSize(),
             region->GetRegionType());
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

        // To make sure the allocedSize are consistent, it must prepend region first then alloc object.
        readOnlyRegionList_.PrependRegionLocked(region, RegionDesc::RegionType::READ_ONLY_REGION);
        addr = region->Alloc(size);
    }

    DLOG(ALLOC, "alloc read only obj 0x%zx(%zu)", addr, size);
    return addr;
}

void RegionManager::VisitRememberSet(const std::function<void(BaseObject*)>& func)
{
    auto visitFunc = [&func](RegionDesc* region) {
        region->VisitAllObjects([&region, &func](BaseObject* obj) {
            if (region->IsInRSet(obj)) {
                func(obj);
            }
        });
    };
    recentPinnedRegionList_.VisitAllRegions(visitFunc);
    pinnedRegionList_.VisitAllRegions(visitFunc);
    recentLargeRegionList_.VisitAllRegions(visitFunc);
    largeRegionList_.VisitAllRegions(visitFunc);
    appSpawnRegionList_.VisitAllRegions(visitFunc);
    rawPointerRegionList_.VisitAllRegions(visitFunc);
    for (size_t i = 0; i < FIXED_PINNED_REGION_COUNT; i++) {
        recentFixedPinnedRegionList_[i]->VisitAllRegions(visitFunc);
        fixedPinnedRegionList_[i]->VisitAllRegions(visitFunc);
    }
}
} // namespace common
