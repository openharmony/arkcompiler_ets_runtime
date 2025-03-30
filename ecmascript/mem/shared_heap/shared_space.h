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

#ifndef ECMASCRIPT_MEM_SHARED_SHARED_SPACE_H
#define ECMASCRIPT_MEM_SHARED_SHARED_SPACE_H

#include "ecmascript/mem/mem_common.h"
#include "ecmascript/mem/sparse_space.h"

namespace panda::ecmascript {
#define CHECK_SOBJECT_NOT_NULL()                                                        \
    if (object != 0) {                                                                  \
        return object;                                                                  \
    }

class SharedHeap;
class SharedLocalSpace;

class SharedSparseSpace : public Space {
public:
    SharedSparseSpace(SharedHeap *heap, MemSpaceType type, size_t initialCapacity, size_t maximumCapacity);
    ~SharedSparseSpace() override
    {
        delete allocator_;
    }
    NO_COPY_SEMANTIC(SharedSparseSpace);
    NO_MOVE_SEMANTIC(SharedSparseSpace);

    void Reset();

    uintptr_t AllocateWithoutGC(JSThread *thread, size_t size);

    uintptr_t Allocate(JSThread *thread, size_t size, bool allowGC = true);
    uintptr_t TryAllocateAndExpand(JSThread *thread, size_t size, bool expand);

    // For work deserialize
    void ResetTopPointer(uintptr_t top);
    uintptr_t AllocateNoGCAndExpand(JSThread *thread, size_t size);
    Region *AllocateDeserializeRegion(JSThread *thread);
    void MergeDeserializeAllocateRegions(const std::vector<Region *> &allocateRegions);

    // For sweeping
    void PrepareSweeping();
    void AsyncSweep(bool isMain);
    void Sweep();

    bool TryFillSweptRegion();
    // Ensure All region finished sweeping
    bool FinishFillSweptRegion();

    void AddSweepingRegion(Region *region);
    void SortSweepingRegion();
    Region *GetSweepingRegionSafe();
    void AddSweptRegionSafe(Region *region);
    Region *GetSweptRegionSafe();

    void FreeRegion(Region *current, bool isMain = true);
    void FreeLiveRange(uintptr_t freeStart, uintptr_t freeEnd, bool isMain);

    void IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const;

    size_t GetHeapObjectSize() const;

    void IncreaseAllocatedSize(size_t size);

    void IncreaseLiveObjectSize(size_t size)
    {
        liveObjectSize_ += size;
    }

    void DecreaseLiveObjectSize(size_t size)
    {
        liveObjectSize_ -= size;
    }

    bool CommittedSizeExceed()
    {
        return committedSize_ >= maximumCapacity_ + outOfMemoryOvershootSize_;
    }

    void CheckAndTriggerLocalFullMark();

    size_t GetTotalAllocatedSize() const;

    void InvokeAllocationInspector(Address object, size_t size, size_t alignedSize);

    void DetachFreeObjectSet(Region *region);

protected:
    bool Expand(JSThread *thread);
    FreeListAllocator<FreeObject> *allocator_;
    SweepState sweepState_ = SweepState::NO_SWEEP;
    SharedHeap *sHeap_ {nullptr};

private:
    static constexpr double LIVE_OBJECT_SIZE_RATIO = 0.8;

    uintptr_t AllocateWithExpand(JSThread *thread, size_t size);
    uintptr_t TryAllocate(JSThread *thread, size_t size);
    // For sweeping
    uintptr_t AllocateAfterSweepingCompleted(JSThread *thread, size_t size);
    void IncAllocSObjectSize(uintptr_t object, size_t size);

    Mutex lock_;
    Mutex allocateLock_;
    std::vector<Region *> sweepingList_;
    std::vector<Region *> sweptList_;
    size_t liveObjectSize_ {0};
    size_t triggerLocalFullMarkLimit_ {0};
};

class SharedAppSpawnSpace : public SharedSparseSpace {
public:
    SharedAppSpawnSpace(SharedHeap *heap, size_t initialCapacity);
    ~SharedAppSpawnSpace() override = default;
    NO_COPY_SEMANTIC(SharedAppSpawnSpace);
    NO_MOVE_SEMANTIC(SharedAppSpawnSpace);

    void IterateOverMarkedObjects(const std::function<void(TaggedObject *object)> &visitor) const;
};

class SharedNonMovableSpace : public SharedSparseSpace {
public:
    SharedNonMovableSpace(SharedHeap *heap, size_t initialCapacity, size_t maximumCapacity);
    ~SharedNonMovableSpace() override = default;
    NO_COPY_SEMANTIC(SharedNonMovableSpace);
    NO_MOVE_SEMANTIC(SharedNonMovableSpace);
};

class SharedOldSpace : public SharedSparseSpace {
public:
    SharedOldSpace(SharedHeap *heap, size_t initialCapacity, size_t maximumCapacity);
    ~SharedOldSpace() override = default;
    NO_COPY_SEMANTIC(SharedOldSpace);
    NO_MOVE_SEMANTIC(SharedOldSpace);

    static constexpr int64_t MAX_EVACUATION_SIZE = 2_MB;
    static constexpr size_t MIN_COLLECT_REGION_SIZE = 5;

    size_t GetMergeSize() const
    {
        return mergeSize_;
    }

    void IncreaseMergeSize(size_t size)
    {
        mergeSize_ += size;
    }

    void ResetMergeSize()
    {
        mergeSize_ = 0;
    }

    void Merge(SharedLocalSpace *localSpace);
    void SelectCSets();
    void RevertCSets();
    void ReclaimCSets();
    void AddCSetRegion(Region *region);
    void RemoveCSetRegion(Region *region);

    template<class Callback>
    void EnumerateCollectRegionSet(Callback &&cb) const
    {
        for (Region *region : collectRegionSet_) {
            if (region != nullptr) {
                cb(region);
            }
        }
    }

    size_t GetCollectSetRegionCount() const
    {
        return collectRegionSet_.size();
    }

private:
    Mutex lock_;
    size_t mergeSize_ {0};
    std::vector<Region *> collectRegionSet_;
};

class SharedLocalSpace : public SharedSparseSpace {
public:
    SharedLocalSpace() = delete;
    SharedLocalSpace(SharedHeap *heap, size_t initialCapacity, size_t maximumCapacity);
    ~SharedLocalSpace() override = default;
    NO_COPY_SEMANTIC(SharedLocalSpace);
    NO_MOVE_SEMANTIC(SharedLocalSpace);

    uintptr_t Allocate(size_t size, bool isExpand = true);
    bool AddRegionToList(Region *region);
    void FreeBumpPoint();
    void Stop();
private:
    void ForceExpandInSharedGC(JSThread *thread);
};

class SharedReadOnlySpace : public Space {
public:
    SharedReadOnlySpace(SharedHeap *heap, size_t initialCapacity, size_t maximumCapacity);
    ~SharedReadOnlySpace() override = default;
    void SetReadOnly()
    {
        auto cb = [](Region *region) {
            region->SetReadOnlyAndMarked();
        };
        EnumerateRegions(cb);
    }

    void IterateOverObjects(const std::function<void(TaggedObject *object)> &visitor) const;

    void ClearReadOnly()
    {
        auto cb = [](Region *region) {
            region->ClearReadOnly();
        };
        EnumerateRegions(cb);
    }

    bool Expand(JSThread *thread);

    uintptr_t Allocate(JSThread *thread, size_t size);

    NO_COPY_SEMANTIC(SharedReadOnlySpace);
    NO_MOVE_SEMANTIC(SharedReadOnlySpace);

private:
    Mutex allocateLock_;
    BumpPointerAllocator allocator_;
};

class SharedHugeObjectSpace : public Space {
public:
    SharedHugeObjectSpace(BaseHeap *heap, HeapRegionAllocator *regionAllocator, size_t initialCapacity,
                    size_t maximumCapacity);
    ~SharedHugeObjectSpace() override = default;
    NO_COPY_SEMANTIC(SharedHugeObjectSpace);
    NO_MOVE_SEMANTIC(SharedHugeObjectSpace);
    // Sometimes it is unsafe to checkSafePoint here, e.g. in deserialize, if do checkSafePoint JSThread may be
    // suspended and then do SharedGC, which will free some regions in SharedHeap that are allocated at the beginning
    // of deserializing for further object allocating, but no object has been allocated on at this moment.
    uintptr_t Allocate(JSThread *thread, size_t objectSize, AllocateEventType allocType = AllocateEventType::NORMAL);
    void Sweep();
    size_t GetHeapObjectSize() const;
    void IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const;

    void ReclaimHugeRegion();

    void InvokeAllocationInspector(Address object, size_t objectSize);

    bool CommittedSizeExceed(size_t size = 0) const
    {
        return committedSize_ + size >= maximumCapacity_ + outOfMemoryOvershootSize_;
    }

    void CheckAndTriggerLocalFullMark(JSThread *thread, size_t size);
private:
    static constexpr size_t HUGE_OBJECT_BITSET_SIZE = 16;
    static constexpr double HUGE_OBJECT_SIZE_RATIO = 0.8;

    size_t triggerLocalFullMarkLimit_ {0};
    EcmaList<Region> hugeNeedFreeList_ {};
    Mutex allocateLock_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_SHARED_SPACE_H
