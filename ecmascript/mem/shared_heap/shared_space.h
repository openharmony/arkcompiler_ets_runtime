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
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
#define CHECK_SOBJECT_AND_INC_OBJ_SIZE(size)                                  \
    if (object != 0) {                                                        \
        IncreaseLiveObjectSize(size);                                         \
        if (sHeap_->IsReadyToMark()) {                                        \
            Region::ObjectAddressToRange(object)->IncreaseAliveObject(size);  \
        }                                                                     \
        InvokeAllocationInspector(object, size, size);                        \
        return object;                                                        \
    }
#else
#define CHECK_SOBJECT_AND_INC_OBJ_SIZE(size)                                  \
    if (object != 0) {                                                        \
        IncreaseLiveObjectSize(size);                                         \
        if (heap_->IsReadyToMark()) {                                         \
            Region::ObjectAddressToRange(object)->IncreaseAliveObject(size);  \
        }                                                                     \
        return object;                                                        \
    }
#endif

class SharedHeap;

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

    size_t GetTotalAllocatedSize() const;

    void InvokeAllocationInspector(Address object, size_t size, size_t alignedSize);

protected:
    FreeListAllocator *allocator_;
    SweepState sweepState_ = SweepState::NO_SWEEP;

private:
    uintptr_t AllocateWithExpand(JSThread *thread, size_t size);
    uintptr_t TryAllocate(JSThread *thread, size_t size);
    bool Expand(JSThread *thread);
    // For sweeping
    uintptr_t AllocateAfterSweepingCompleted(JSThread *thread, size_t size);

    Mutex lock_;
    Mutex allocateLock_;
    SharedHeap *sHeap_ {nullptr};
    std::vector<Region *> sweepingList_;
    std::vector<Region *> sweptList_;
    size_t liveObjectSize_ {0};
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
    uintptr_t Allocate(JSThread *thread, size_t objectSize);
    void Sweep();
    size_t GetHeapObjectSize() const;
    void IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const;

    void ReclaimHugeRegion();

    void InvokeAllocationInspector(Address object, size_t objectSize);

    bool CommittedSizeExceed(size_t size) const
    {
        return committedSize_ + size >= maximumCapacity_ + outOfMemoryOvershootSize_;
    }
private:
    static constexpr size_t HUGE_OBJECT_BITSET_SIZE = 16;
    EcmaList<Region> hugeNeedFreeList_ {};
    Mutex allocateLock_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_SHARED_SPACE_H
