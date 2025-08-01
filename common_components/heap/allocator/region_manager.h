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

#ifndef COMMON_COMPONENTS_HEAP_ALLOCATOR_REGION_MANAGER_H
#define COMMON_COMPONENTS_HEAP_ALLOCATOR_REGION_MANAGER_H

#include <list>
#include <map>
#include <set>
#include <thread>
#include <vector>

#include "common_components/common/run_type.h"
#include "common_components/heap/allocator/alloc_buffer.h"
#include "common_components/heap/allocator/allocator.h"
#include "common_components/heap/allocator/free_region_manager.h"
#include "common_components/heap/allocator/region_list.h"
#include "common_components/heap/allocator/fix_heap.h"
#include "common_components/heap/allocator/slot_list.h"
#include "common_components/common_runtime/hooks.h"

namespace common {
using JitFortUnProtHookType = void (*)(size_t size, void* base);

class MarkingCollector;
class CompactCollector;
class RegionManager;
class Taskpool;
// RegionManager needs to know header size and alignment in order to iterate objects linearly
// and thus its Alloc should be rewrite with AllocObj(objSize)
class RegionManager {
public:
    constexpr static size_t FIXED_PINNED_REGION_COUNT = 128;
    constexpr static size_t FIXED_PINNED_THRESHOLD = sizeof(uint64_t) * FIXED_PINNED_REGION_COUNT;
    /* region memory layout:
        1. some paddings memory to aligned
        2. region info for each region, part of heap metadata
        3. region space for allocation, i.e., the heap  --- start address is aligend to `RegionDesc::UNIT_SIZE`
    */
    static size_t GetHeapMemorySize(size_t heapSize)
    {
        size_t regionNum = GetHeapUnitCount(heapSize);
        size_t metadataSize = GetMetadataSize(regionNum);
        // Add one more `RegionDesc::UNIT_SIZE` totalSize, because we need the region address is aligned to
        // `RegionDesc::UNIT_SIZE`, this need some paddings
        size_t totalSize = metadataSize + RoundUp<size_t>(heapSize, RegionDesc::UNIT_SIZE) + RegionDesc::UNIT_SIZE;
        return totalSize;
    }

    static size_t GetHeapUnitCount(size_t heapSize)
    {
        heapSize = RoundUp<size_t>(heapSize, RegionDesc::UNIT_SIZE);
        size_t regionNum = heapSize / RegionDesc::UNIT_SIZE;
        return regionNum;
    }

    static size_t GetMetadataSize(size_t regionNum)
    {
        size_t metadataSize = regionNum * sizeof(RegionDesc);
        return RoundUp<size_t>(metadataSize, COMMON_PAGE_SIZE);
    }

    void CollectFixTasks(FixHeapTaskList& taskList);
    void CollectFixHeapTaskForPinnedRegion(MarkingCollector& collector, RegionList& list, FixHeapTaskList& taskList);

    void Initialize(size_t regionNum, uintptr_t regionInfoStart);

    RegionManager()
        : freeRegionManager_(*this), garbageRegionList_("garbage regions"),
          pinnedRegionList_("pinned regions"), recentPinnedRegionList_("recent pinned regions"),
          rawPointerRegionList_("raw pointer pinned regions"), largeRegionList_("large regions"),
          recentLargeRegionList_("recent large regions"), readOnlyRegionList_("read only region"),
          appSpawnRegionList_("appSpawn regions")
    {
        for (size_t i = 0; i < FIXED_PINNED_REGION_COUNT; i++) {
            recentFixedPinnedRegionList_[i] = new RegionList("fixed recent pinned regions");
            fixedPinnedRegionList_[i] = new RegionList("fixed pinned regions");
        }
    }

    RegionManager(const RegionManager&) = delete;

    RegionManager& operator=(const RegionManager&) = delete;

    void FixFixedRegionList(MarkingCollector& collector, RegionList& list, size_t cellCount, GCStats& stats);

    using RootSet = MarkStack<BaseObject*>;

#if defined(GCINFO_DEBUG) && GCINFO_DEBUG
    void DumpRegionDesc() const;
#endif

    void DumpRegionStats() const;

    uintptr_t GetInactiveZone() const { return inactiveZone_; }

    uintptr_t GetRegionHeapStart() const { return regionHeapStart_; }

    RegionDesc* GetFirstRegion() const
    {
        if (regionHeapStart_ < inactiveZone_) {
            return RegionDesc::GetRegionDescAt(regionHeapStart_);
        }
        return nullptr;
    }

    ~RegionManager()
    {
        for (size_t i = 0; i < FIXED_PINNED_REGION_COUNT; i++) {
            if (recentFixedPinnedRegionList_[i] != nullptr) {
                delete recentFixedPinnedRegionList_[i];
                recentFixedPinnedRegionList_[i] = nullptr;
            }
            if (fixedPinnedRegionList_[i] != nullptr) {
                delete fixedPinnedRegionList_[i];
                fixedPinnedRegionList_[i] = nullptr;
            }
        }
    }
    // take a region with *num* units for allocation
    RegionDesc* TakeRegion(size_t num, RegionDesc::UnitRole, bool expectPhysicalMem = false, bool allowgc = true);

    RegionDesc* TakeRegion(bool expectPhysicalMem, bool allowgc)
    {
        return TakeRegion(1, RegionDesc::UnitRole::SMALL_SIZED_UNITS, expectPhysicalMem, allowgc);
    }

    void AddRecentPinnedRegion(RegionDesc* region)
    {
        recentPinnedRegionList_.PrependRegion(region, RegionDesc::RegionType::RECENT_PINNED_REGION);
    }

    uintptr_t AllocPinnedFromFreeList(size_t size);

    uintptr_t AllocReadOnly(size_t size, bool allowGC = true);

    uintptr_t AllocPinned(size_t size, bool allowGC = true)
    {
        uintptr_t addr = 0;
        if (!allowGC || size > FIXED_PINNED_THRESHOLD) {
            DLOG(ALLOC, "alloc pinned obj 0x%zx(%zu)", addr, size);
            return AllocNextFitPinned(size);
        }
        CHECK_CC(size % sizeof(uint64_t) == 0);
        size_t cellCount = size / sizeof(uint64_t) - 1;
        RegionList* list = recentFixedPinnedRegionList_[cellCount];
        std::mutex& listMutex = list->GetListMutex();
        listMutex.lock();
        RegionDesc* headRegion = list->GetHeadRegion();
        if (headRegion != nullptr) {
            addr = headRegion->Alloc(size);
        }
        if (addr == 0) {
            addr = AllocPinnedFromFreeList(cellCount);
        }
        if (addr == 0) {
            RegionDesc* region =
                TakeRegion(1, RegionDesc::UnitRole::SMALL_SIZED_UNITS, false, allowGC);
            if (region == nullptr) {
                listMutex.unlock();
                return 0;
            }
            DLOG(REGION, "alloc pinned region @0x%zx+%zu type %u", region->GetRegionStart(),
                 region->GetRegionAllocatedSize(),
                 region->GetRegionType());
            ASSERT(cellCount == static_cast<size_t>(static_cast<uint8_t>(cellCount)));
            region->SetRegionCellCount(static_cast<uint8_t>(cellCount));
            GCPhase phase = Mutator::GetMutator()->GetMutatorPhase();
            if (phase == GC_PHASE_ENUM || phase == GC_PHASE_MARK ||
                phase == GC_PHASE_REMARK_SATB || phase == GC_PHASE_POST_MARK) {
                region->SetMarkingLine();
            } else if (phase == GC_PHASE_PRECOPY || phase == GC_PHASE_COPY || phase == GC_PHASE_FIX) {
                region->SetMarkingLine();
                region->SetCopyLine();
            }
            // To make sure the allocedSize are consistent, it must prepend region first then alloc object.
            list->PrependRegionLocked(region, RegionDesc::RegionType::FIXED_PINNED_REGION);
            addr = region->Alloc(size);
        }
        DLOG(ALLOC, "alloc pinned obj 0x%zx(%zu)", addr, size);
        listMutex.unlock();
        return addr;
    }

    uintptr_t AllocNextFitPinned(size_t size, bool allowGC = true)
    {
        uintptr_t addr = 0;
        std::mutex& regionListMutex = recentPinnedRegionList_.GetListMutex();

        std::lock_guard<std::mutex> lock(regionListMutex);
        RegionDesc* headRegion = recentPinnedRegionList_.GetHeadRegion();
        if (headRegion != nullptr) {
            addr = headRegion->Alloc(size);
        }
        if (addr == 0) {
            RegionDesc* region =
                TakeRegion(1, RegionDesc::UnitRole::SMALL_SIZED_UNITS, false, allowGC);
            if (region == nullptr) {
                return 0;
            }
            DLOG(REGION, "alloc pinned region @0x%zx+%zu type %u", region->GetRegionStart(),
                 region->GetRegionAllocatedSize(),
                 region->GetRegionType());
            GCPhase phase = Mutator::GetMutator()->GetMutatorPhase();
            if (phase == GC_PHASE_ENUM || phase == GC_PHASE_MARK || phase == GC_PHASE_REMARK_SATB ||
                phase == GC_PHASE_POST_MARK) {
                region->SetMarkingLine();
            } else if (phase == GC_PHASE_PRECOPY || phase == GC_PHASE_COPY || phase == GC_PHASE_FIX) {
                region->SetMarkingLine();
                region->SetCopyLine();
            }

            // To make sure the allocedSize are consistent, it must prepend region first then alloc object.
            recentPinnedRegionList_.PrependRegionLocked(region, RegionDesc::RegionType::RECENT_PINNED_REGION);
            addr = region->Alloc(size);
        }

        DLOG(ALLOC, "alloc pinned obj 0x%zx(%zu)", addr, size);
        return addr;
    }

    // note: AllocSmall() is always performed by region owned by mutator thread
    // thus no need to do in RegionManager
    // caller assures size is truely large (> region size)
    uintptr_t AllocLarge(size_t size, bool allowGC = true)
    {
        size_t alignedSize = AlignUp<size_t>(size + RegionDesc::UNIT_HEADER_SIZE, RegionDesc::UNIT_SIZE);
        size_t regionCount = alignedSize / RegionDesc::UNIT_SIZE;
        RegionDesc* region = TakeRegion(regionCount, RegionDesc::UnitRole::LARGE_SIZED_UNITS, false, allowGC);
        if (region == nullptr) {
            return 0;
        }
        GCPhase phase = Mutator::GetMutator()->GetMutatorPhase();
        if (phase == GC_PHASE_ENUM || phase == GC_PHASE_MARK || phase == GC_PHASE_REMARK_SATB ||
            phase == GC_PHASE_POST_MARK) {
            region->SetMarkingLine();
        } else if (phase == GC_PHASE_PRECOPY || phase == GC_PHASE_COPY || phase == GC_PHASE_FIX) {
            region->SetMarkingLine();
            region->SetCopyLine();
        }

        DLOG(REGION, "alloc large region @0x%zx+%zu type %u", region->GetRegionStart(),
             region->GetRegionSize(), region->GetRegionType());
        uintptr_t addr = region->Alloc(size);
        ASSERT(addr > 0);
        recentLargeRegionList_.PrependRegion(region, RegionDesc::RegionType::RECENT_LARGE_REGION);
        return addr;
    }

    void CountLiveObject(const BaseObject* obj);

    void AssembleLargeGarbageCandidates();
    void AssemblePinnedGarbageCandidates();

    void ReassembleAppspawnSpace(RegionList& regionList)
    {
        appSpawnRegionList_.MergeRegionList(regionList, RegionDesc::RegionType::APPSPAWN_REGION);
    }

    void CollectFromSpaceGarbage(RegionList& fromList)
    {
        garbageRegionList_.MergeRegionList(fromList, RegionDesc::RegionType::GARBAGE_REGION);
    }

    void AddRawPointerRegion(RegionDesc* region)
    {
        rawPointerRegionList_.PrependRegion(region, RegionDesc::RegionType::RAW_POINTER_REGION);
    }

    size_t CollectRegion(RegionDesc* region)
    {
        DLOG(REGION, "collect region %p@%#zx+%zu type %u", region, region->GetRegionStart(),
             region->GetLiveByteCount(), region->GetRegionType());

#ifdef USE_HWASAN
        ASAN_POISON_MEMORY_REGION(reinterpret_cast<const volatile void *>(region->GetRegionBase()),
            region->GetRegionBaseSize());
        const uintptr_t p_addr = region->GetRegionBase();
        const uintptr_t p_size = region->GetRegionBaseSize();
        LOG_COMMON(DEBUG) << std::hex << "set [" << p_addr <<
                             std::hex << ", " << p_addr + p_size << ") poisoned\n";
#endif
        garbageRegionList_.PrependRegion(region, RegionDesc::RegionType::GARBAGE_REGION);
        if (region->IsLargeRegion()) {
            return region->GetRegionSize();
        } else {
            return region->GetRegionSize() - region->GetLiveByteCount();
        }
    }

    void ReclaimRegion(RegionDesc* region);
    size_t ReleaseRegion(RegionDesc* region);

    void ReclaimGarbageRegions()
    {
        RegionDesc* garbage = garbageRegionList_.TakeHeadRegion();
        while (garbage != nullptr) {
            ReclaimRegion(garbage);
            garbage = garbageRegionList_.TakeHeadRegion();
        }
    }

    size_t CollectLargeGarbage();

    // targetSize: size of memory which we do not release and keep it as cache for future allocation.
    size_t ReleaseGarbageRegions(size_t targetSize) { return freeRegionManager_.ReleaseGarbageRegions(targetSize); }

    void ForEachObjectUnsafe(const std::function<void(BaseObject*)>& visitor) const;
    void ForEachObjectSafe(const std::function<void(BaseObject*)>& visitor) const;
    void ForEachAwaitingJitFortUnsafe(const std::function<void(BaseObject*)>& visitor) const;

    size_t GetRecentAllocatedSize() const
    {
        return recentLargeRegionList_.GetAllocatedSize() + recentPinnedRegionList_.GetAllocatedSize();
    }

    size_t GetSurvivedSize() const
    {
        return pinnedRegionList_.GetAllocatedSize() + largeRegionList_.GetAllocatedSize();
    }

    size_t GetUsedUnitCount() const
    {
        return largeRegionList_.GetUnitCount() + recentLargeRegionList_.GetUnitCount() +
            pinnedRegionList_.GetUnitCount() + recentPinnedRegionList_.GetUnitCount() +
            rawPointerRegionList_.GetUnitCount() + readOnlyRegionList_.GetUnitCount() +
            appSpawnRegionList_.GetUnitCount();
    }

    size_t GetDirtyUnitCount() const { return freeRegionManager_.GetDirtyUnitCount(); }

    size_t GetInactiveUnitCount() const { return (regionHeapEnd_ - inactiveZone_) / RegionDesc::UNIT_SIZE; }

    inline size_t GetLargeObjectSize() const
    {
        return largeRegionList_.GetAllocatedSize() + recentLargeRegionList_.GetAllocatedSize();
    }

    size_t GetAllocatedSize() const
    {
        return largeRegionList_.GetAllocatedSize() + recentLargeRegionList_.GetAllocatedSize() +
            GetPinnedSpaceSize() + rawPointerRegionList_.GetAllocatedSize() +
            readOnlyRegionList_.GetAllocatedSize() + appSpawnRegionList_.GetAllocatedSize();
    }

    inline size_t GetPinnedSpaceSize() const
    {
        size_t pinnedSpaceSize =
            pinnedRegionList_.GetAllocatedSize() + recentPinnedRegionList_.GetAllocatedSize();
        for (size_t i = 0; i < FIXED_PINNED_REGION_COUNT; i++) {
            pinnedSpaceSize += recentFixedPinnedRegionList_[i]->GetAllocatedSize();
            pinnedSpaceSize += fixedPinnedRegionList_[i]->GetAllocatedSize();
        }
        return pinnedSpaceSize;
    }

    RegionDesc* GetNextNeighborRegion(RegionDesc* region) const
    {
        HeapAddress address = region->GetRegionEnd();
        if (address < inactiveZone_.load()) {
            return RegionDesc::GetRegionDescAt(address);
        }
        return nullptr;
    }

    // this method checks whether allocation is permitted for now, otherwise, it is suspened
    // until allocation does no harm to gc.
    void RequestForRegion(size_t size);

    void PrepareMarking()
    {
        AllocBufferVisitor visitor = [](AllocationBuffer& regionBuffer) {
            RegionDesc* region = regionBuffer.GetRegion<AllocBufferType::YOUNG>();
            if (region != RegionDesc::NullRegion()) {
                region->SetMarkingLine();
            }
            region = regionBuffer.GetRegion<AllocBufferType::OLD>();
            if (region != RegionDesc::NullRegion()) {
                region->SetMarkingLine();
            }
        };
        Heap::GetHeap().GetAllocator().VisitAllocBuffers(visitor);

        RegionDesc* pinRegion = recentPinnedRegionList_.GetHeadRegion();
        if (pinRegion != nullptr && pinRegion != RegionDesc::NullRegion()) {
            pinRegion->SetMarkingLine();
        }

        RegionDesc* readOnlyRegion = readOnlyRegionList_.GetHeadRegion();
        if (readOnlyRegion != nullptr && readOnlyRegion != RegionDesc::NullRegion()) {
            readOnlyRegion->SetMarkingLine();
        }

        for (size_t i = 0; i < FIXED_PINNED_REGION_COUNT; i++) {
            RegionDesc* region = recentFixedPinnedRegionList_[i]->GetHeadRegion();
            if (region != nullptr && region != RegionDesc::NullRegion()) {
                region->SetMarkingLine();
            }
        }
    }

    void PrepareForward()
    {
        AllocBufferVisitor visitor = [](AllocationBuffer& regionBuffer) {
            RegionDesc* region = regionBuffer.GetRegion<AllocBufferType::YOUNG>();
            if (region != RegionDesc::NullRegion()) {
                region->SetCopyLine();
            }
            region = regionBuffer.GetRegion<AllocBufferType::OLD>();
            if (region != RegionDesc::NullRegion()) {
                region->SetCopyLine();
            }
            region = regionBuffer.GetRegion<AllocBufferType::OLD>();
            if (region != RegionDesc::NullRegion()) {
                region->SetCopyLine();
            }
        };
        Heap::GetHeap().GetAllocator().VisitAllocBuffers(visitor);

        RegionDesc* pinRegion = recentPinnedRegionList_.GetHeadRegion();
        if (pinRegion != nullptr && pinRegion != RegionDesc::NullRegion()) {
            pinRegion->SetCopyLine();
        }

        RegionDesc* readOnlyRegion = readOnlyRegionList_.GetHeadRegion();
        if (readOnlyRegion != nullptr && readOnlyRegion != RegionDesc::NullRegion()) {
            readOnlyRegion->SetCopyLine();
        }

        for (size_t i = 0; i < FIXED_PINNED_REGION_COUNT; i++) {
            RegionDesc* region = recentFixedPinnedRegionList_[i]->GetHeadRegion();
            if (region != nullptr && region != RegionDesc::NullRegion()) {
                region->SetCopyLine();
            }
        }
    }

    void ClearAllGCInfo()
    {
        ClearGCInfo(largeRegionList_);
        ClearGCInfo(recentLargeRegionList_);
        ClearGCInfo(recentPinnedRegionList_);
        ClearGCInfo(rawPointerRegionList_);
        ClearGCInfo(pinnedRegionList_);
        ClearGCInfo(readOnlyRegionList_);
        ClearGCInfo(appSpawnRegionList_);
        for (size_t i = 0; i < FIXED_PINNED_REGION_COUNT; i++) {
            ClearGCInfo(*recentFixedPinnedRegionList_[i]);
            ClearGCInfo(*fixedPinnedRegionList_[i]);
        }
    }

    void SetReadOnlyToRORegionList()
    {
        auto visitor = [](RegionDesc* region) {
            if (region != nullptr) {
                region->SetReadOnly();
            }
        };
        readOnlyRegionList_.VisitAllRegions(visitor);
    }

    void ClearReadOnlyFromRORegionList()
    {
        auto visitor = [](RegionDesc* region) {
            if (region != nullptr) {
                region->ClearReadOnly();
            }
        };
        readOnlyRegionList_.VisitAllRegions(visitor);
    }

    void MarkRememberSet(const std::function<void(BaseObject*)>& func);
    void ClearRSet();

    void MarkJitFortMemInstalled(void *thread, BaseObject *obj)
    {
        std::lock_guard guard(awaitingJitFortMutex_);
        // GC is running, we should mark JitFort installled after GC finish
        if (Heap::GetHeap().GetGCPhase() != GCPhase::GC_PHASE_IDLE) {
            jitFortPostGCInstallTask_.emplace(nullptr, obj);
        } else {
            // a threadlocal JitFort mem
            if (thread) {
                MarkThreadLocalJitFortInstalled(thread, obj);
            } else {
                RegionDesc::GetAliveRegionDescAt(reinterpret_cast<uintptr_t>(obj))->SetJitFortAwaitInstallFlag(false);
            }
            awaitingJitFort_.erase(obj);
        }
    }

    void MarkJitFortMemAwaitingInstall(BaseObject *obj)
    {
        std::lock_guard guard(awaitingJitFortMutex_);
        RegionDesc::GetAliveRegionDescAt(reinterpret_cast<uintptr_t>(obj))->SetJitFortAwaitInstallFlag(true);
        awaitingJitFort_.insert(obj);
    }

    void HandlePostGCJitFortInstallTask()
    {
        ASSERT(Heap::GetHeap().GetGCPhase() == GCPhase::GC_PHASE_IDLE);
        while (!jitFortPostGCInstallTask_.empty()) {
            auto [thread, machineCode] = jitFortPostGCInstallTask_.top();
            MarkJitFortMemInstalled(thread, machineCode);
            jitFortPostGCInstallTask_.pop();
        }
    }

private:
    inline void TagHugePage(RegionDesc* region, size_t num) const;
    inline void UntagHugePage(RegionDesc* region, size_t num) const;

    void ClearGCInfo(RegionList& list)
    {
        RegionList tmp("temp region list");
        list.CopyListTo(tmp);
        tmp.VisitAllRegions([](RegionDesc* region) {
            region->ClearMarkingCopyLine();
            region->ClearLiveInfo();
            region->ResetMarkBit();
        });
    }

    FreeRegionManager freeRegionManager_;

    // region lists actually represent life cycle of regions.
    // each region must belong to only one list at any time.

    // cache for fromRegionList after forwarding.
    RegionList garbageRegionList_;

    // regions for small-sized object which is not movable.
    RegionList pinnedRegionList_;
    RegionList* fixedPinnedRegionList_[FIXED_PINNED_REGION_COUNT];

    // regions which allocated since last GC beginning.
    // record pinned regions in here first and move those regions
    // to pinned/fixedPinned RegionList when gc starts.
    RegionList recentPinnedRegionList_;
    RegionList* recentFixedPinnedRegionList_[FIXED_PINNED_REGION_COUNT];

    // region lists for small-sized raw-pointer objects (i.e. future, monitor)
    // which can not be moved ever (even during compaction).
    RegionList rawPointerRegionList_; // delete rawPointerRegion, use PinnedRegion

    // regions for large-sized objects.
    // large region is recorded here after large object is allocated.
    RegionList largeRegionList_;

    // large regions which allocated since last GC beginning.
    // record pinned regions in here first and move those when gc starts.
    RegionList recentLargeRegionList_;

    // regions for read only objects
    RegionList readOnlyRegionList_;

    // regions for appspawn region list.
    RegionList appSpawnRegionList_;

    uintptr_t regionInfoStart_ = 0; // the address of first RegionDesc

    uintptr_t regionHeapStart_ = 0; // the address of first region to allocate object
    uintptr_t regionHeapEnd_ = 0;

    // the time when previous region was allocated, which is assigned with returned value by timeutil::NanoSeconds().
    std::atomic<uint64_t> prevRegionAllocTime_ = { 0 };

    // heap space not allocated yet for even once. this value should not be decreased.
    std::atomic<uintptr_t> inactiveZone_ = { 0 };
    // Awaiting JitFort object has no references from other objects,
    // but we need to keep them as live untill jit compilation has finished installing.
    std::set<BaseObject*> awaitingJitFort_;
    std::stack<std::pair<void*, BaseObject*>> jitFortPostGCInstallTask_;
    std::mutex awaitingJitFortMutex_;

    friend class VerifyIterator;
};
} // namespace common

#endif // COMMON_COMPONENTS_HEAP_ALLOCATOR_REGION_MANAGER_H
