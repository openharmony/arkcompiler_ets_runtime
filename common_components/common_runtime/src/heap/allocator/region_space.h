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
#ifndef ARK_COMMON_REGION_SPACE_H
#define ARK_COMMON_REGION_SPACE_H

#include <cassert>
#include <list>
#include <map>
#include <set>
#include <thread>
#include <vector>

#include "common_components/common_runtime/src/heap/allocator/alloc_util.h"
#include "common_components/common_runtime/src/heap/allocator/allocator.h"
#include "common_components/common_runtime/src/heap/allocator/region_manager.h"
#include "common_components/common_runtime/src/mutator/mutator.h"
#if defined(ARKCOMMON_SANITIZER_SUPPORT)
#include "common_components/common_runtime/src/sanitizer/sanitizer_interface.h"
#endif
#include "common_interfaces/base_runtime.h"

namespace panda {
// RegionSpace aims to be the API for other components of runtime
// the complication of implementation is delegated to RegionManager
// allocator should not depend on any assumptions on the details of RegionManager
class RegionSpace : public Allocator {
public:
    static size_t ToAllocatedSize(size_t objSize)
    {
        size_t size = objSize + HEADER_SIZE;
        return RoundUp<size_t>(size, ALLOC_ALIGN);
    }

    static size_t GetAllocSize(const BaseObject& obj)
    {
        size_t objSize = obj.GetSize();
        return ToAllocatedSize(objSize);
    }

    RegionSpace() = default;
    NO_INLINE_CC virtual ~RegionSpace()
    {
        if (allocBufferManager_ != nullptr) {
            delete allocBufferManager_;
            allocBufferManager_ = nullptr;
        }
#if defined(ARKCOMMON_SANITIZER_SUPPORT)
        Sanitizer::OnHeapDeallocated(map->GetBaseAddr(), map->GetMappedSize());
#endif
        MemoryMap::DestroyMemoryMap(map_);
    }

    void Init(const HeapParam&) override;

    HeapAddress Allocate(size_t size, AllocType allocType) override;

    HeapAddress AllocateNoGC(size_t size, AllocType allocType) override;

    RegionManager& GetRegionManager() noexcept { return regionManager_; }

    HeapAddress GetSpaceStartAddress() const override { return reservedStart_; }

    HeapAddress GetSpaceEndAddress() const override { return reservedEnd_; }

    size_t GetCurrentCapacity() const override { return regionManager_.GetInactiveZone() - reservedStart_; }
    size_t GetMaxCapacity() const override { return reservedEnd_ - reservedStart_; }

    inline size_t GetRecentAllocatedSize() const { return regionManager_.GetRecentAllocatedSize(); }

    // size of objects survived in previous gc.
    inline size_t GetSurvivedSize() const { return regionManager_.GetSurvivedSize(); }

    size_t GetUsedPageSize() const override { return regionManager_.GetUsedRegionSize(); }

    inline size_t GetTargetSize() const
    {
        double heapUtilization = BaseRuntime::GetInstance()->GetHeapParam().heapUtilization;
        return static_cast<size_t>(GetUsedPageSize() / heapUtilization);
    }

    size_t GetAllocatedBytes() const override { return regionManager_.GetAllocatedSize(); }

    size_t LargeObjectSize() const override { return regionManager_.GetLargeObjectSize(); }

    size_t FromSpaceSize() const { return regionManager_.GetFromSpaceSize(); }

    size_t PinnedSpaceSize() const { return regionManager_.GetPinnedSpaceSize(); }

    inline size_t ToSpaceSize() const { return regionManager_.GetToSpaceSize(); }

#ifndef NDEBUG
    bool IsHeapObject(HeapAddress addr) const override;
#endif

    size_t ReclaimGarbageMemory(bool releaseAll) override
    {
        {
            ARK_COMMON_PHASE_TIMER("ReclaimGarbageRegions");
            regionManager_.ReclaimGarbageRegions();
        }

        ARK_COMMON_PHASE_TIMER("ReleaseGarbageMemory");
        if (releaseAll) {
            return regionManager_.ReleaseGarbageRegions(0);
        } else {
            size_t size = regionManager_.GetAllocatedSize();
            double cachedRatio = 1 - BaseRuntime::GetInstance()->GetHeapParam().heapUtilization;
            size_t targetCachedSize = static_cast<size_t>(size * cachedRatio);
            return regionManager_.ReleaseGarbageRegions(targetCachedSize);
        }
    }

    bool ForEachObject(const std::function<void(BaseObject*)>& visitor, bool safe) const override
    {
        if (UNLIKELY_CC(safe)) {
            regionManager_.ForEachObjectSafe(visitor);
        } else {
            regionManager_.ForEachObjectUnsafe(visitor);
        }
        return true;
    }

    void ExemptFromSpace()
    {
        ARK_COMMON_PHASE_TIMER("ExemptFromRegions");
        regionManager_.ExemptFromRegions();
    }

    BaseObject* RouteObject(BaseObject* fromObj, size_t size)
    {
        AllocationBuffer* buffer = AllocationBuffer::GetOrCreateAllocBuffer();
        uintptr_t toAddr = buffer->ToSpaceAllocate(size, AllocType::MOVEABLE_OBJECT);
        return reinterpret_cast<BaseObject*>(toAddr);
    }

    // void PrepareFromSpace() { regionManager_.PrepareFromRegionList(); }

    // void ClearAllLiveInfo() { regionManager_.ClearAllLiveInfo(); }

    void CopyFromSpace(GCThreadPool* threadPool)
    {
        ARK_COMMON_PHASE_TIMER("CopyFromRegions");
        regionManager_.CopyFromRegions(threadPool);
    }

    void FixHeap() { regionManager_.FixAllRegionLists(); }

    using RootSet = MarkStack<BaseObject*>;

    size_t CollectLargeGarbage() { return regionManager_.CollectLargeGarbage(); }

    void CollectFromSpaceGarbage()
    {
        regionManager_.CollectFromSpaceGarbage();
        regionManager_.ReassembleFromSpace();
    }

    void ClearAllGCInfo()
    {
        regionManager_.ClearAllGCInfo();
    }

    void AssembleGarbageCandidates(bool collectAll = false)
    {
        regionManager_.AssembleSmallGarbageCandidates();
        regionManager_.AssemblePinnedGarbageCandidates(collectAll);
        regionManager_.AssembleLargeGarbageCandidates();
    }

    void DumpRegionStats(const char* msg) const { regionManager_.DumpRegionStats(msg); }

    void CountLiveObject(const BaseObject* obj) { regionManager_.CountLiveObject(obj); }

    void PrepareTrace() { regionManager_.PrepareTrace(); }
    void PrepareForward() { regionManager_.PrepareForward(); }
    void PrepareFixForPin() { regionManager_.PrepareFixForPin(); }
    void FeedHungryBuffers() override;

    // markObj
    static bool MarkObject(const BaseObject* obj)
    {
        RegionDesc* regionInfo = RegionDesc::GetRegionDescAt(reinterpret_cast<HeapAddress>(obj));
        return regionInfo->MarkObject(obj);
    }
    static bool ResurrentObject(const BaseObject* obj)
    {
        RegionDesc* regionInfo = RegionDesc::GetRegionDescAt(reinterpret_cast<HeapAddress>(obj));
        return regionInfo->ResurrentObject(obj);
    }

    static bool EnqueueObject(const BaseObject* obj)
    {
        RegionDesc* regionInfo = RegionDesc::GetRegionDescAt(reinterpret_cast<HeapAddress>(obj));
        return regionInfo->EnqueueObject(obj);
    }

    static bool IsMarkedObject(const BaseObject* obj)
    {
        RegionDesc* regionInfo = RegionDesc::GetRegionDescAt(reinterpret_cast<HeapAddress>(obj));
        return regionInfo->IsMarkedObject(obj);
    }

    static bool IsResurrectedObject(const BaseObject* obj)
    {
        RegionDesc* regionInfo = RegionDesc::GetRegionDescAt(reinterpret_cast<HeapAddress>(obj));
        return regionInfo->IsResurrectedObject(obj);
    }

    static bool IsEnqueuedObject(const BaseObject* obj)
    {
        RegionDesc* regionInfo = RegionDesc::GetRegionDescAt(reinterpret_cast<HeapAddress>(obj));
        return regionInfo->IsEnqueuedObject(obj);
    }

    static bool IsNewObjectSinceTrace(const BaseObject* object)
    {
        RegionDesc* region = RegionDesc::GetRegionDescAt(reinterpret_cast<uintptr_t>(object));
        ASSERT_LOGF(region != nullptr, "region is nullptr");
        return region->IsNewObjectSinceTrace(object);
    }

    void AddRawPointerObject(BaseObject* obj) { regionManager_.AddRawPointerObject(obj); }

    void RemoveRawPointerObject(BaseObject* obj) { regionManager_.RemoveRawPointerObject(obj); }

    friend class Allocator;

private:
    enum class TryAllocationThreshold {
        RESCHEDULE = 3,
        TRIGGER_OOM = 5,
    };
    HeapAddress TryAllocateOnce(size_t allocSize, AllocType allocType);
    bool ShouldRetryAllocation(size_t& tryTimes) const;
    HeapAddress reservedStart_ = 0;
    HeapAddress reservedEnd_ = 0;
    RegionManager regionManager_;
    MemoryMap* map_{ nullptr };
};
} // namespace panda
#endif // ARK_COMMON_REGION_SPACE_H
