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

#ifndef ECMASCRIPT_MEM_SPARSE_SPACE_H
#define ECMASCRIPT_MEM_SPARSE_SPACE_H

#include "ecmascript/mem/space-inl.h"
#include "ecmascript/mem/mem_common.h"

#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
#define CHECK_OBJECT_AND_INC_OBJ_SIZE(size)                                   \
    if (object != 0) {                                                        \
        IncreaseLiveObjectSize(size);                                         \
        if (!heap_->IsFullMark() || heap_->GetJSThread()->IsReadyToMark()) {  \
            Region::ObjectAddressToRange(object)->IncreaseAliveObject(size);  \
        }                                                                     \
        InvokeAllocationInspector(object, size, size);                        \
        return object;                                                        \
    }
#else
#define CHECK_OBJECT_AND_INC_OBJ_SIZE(size)                                   \
    if (object != 0) {                                                        \
        IncreaseLiveObjectSize(size);                                         \
        if (!heap_->IsFullMark() || heap_->GetJSThread()->IsReadyToMark()) {  \
            Region::ObjectAddressToRange(object)->IncreaseAliveObject(size);  \
        }                                                                     \
        return object;                                                        \
    }
#endif

enum class SweepState : uint8_t {
    NO_SWEEP,
    SWEEPING,
    SWEPT
};

namespace panda::ecmascript {
class LocalSpace;

class SparseSpace : public Space {
public:
    SparseSpace(Heap *heap, MemSpaceType type, size_t initialCapacity, size_t maximumCapacity);
    ~SparseSpace() override
    {
        delete allocator_;
    }
    NO_COPY_SEMANTIC(SparseSpace);
    NO_MOVE_SEMANTIC(SparseSpace);

    void Initialize() override;
    void Reset();

    uintptr_t Allocate(size_t size, bool allowGC = true);
    bool Expand();

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
    void FreeRegionFromSpace(Region *region);
    Region *TryToGetSuitableSweptRegion(size_t size);

    void FreeRegion(Region *current, bool isMain = true);
    void FreeLiveRange(Region *current, uintptr_t freeStart, uintptr_t freeEnd, bool isMain);

    void DetachFreeObjectSet(Region *region);

    void IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const;
    void IterateOldToNewOverObjects(
        const std::function<void(TaggedObject *object, JSTaggedValue value)> &visitor) const;

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

    void SetOvershootSize(size_t size)
    {
        overshootSize_ = size;
    }

    size_t GetOvershootSize()
    {
        return overshootSize_;
    }

    void AdjustOvershootSize()
    {
        if (overshootSize_ > 0 && maximumCapacity_ > committedSize_) {
            size_t size = maximumCapacity_ - committedSize_;
            overshootSize_ = overshootSize_ > size ? overshootSize_ - size : 0;
        }
    }

    size_t GetTotalAllocatedSize() const;

    void InvokeAllocationInspector(Address object, size_t size, size_t alignedSize);

protected:
    FreeListAllocator *allocator_;
    SweepState sweepState_ = SweepState::NO_SWEEP;
    Heap *heap_ {nullptr};

private:
    // For sweeping
    uintptr_t AllocateAfterSweepingCompleted(size_t size);

    os::memory::Mutex lock_;
    std::vector<Region *> sweepingList_;
    std::vector<Region *> sweptList_;
    size_t liveObjectSize_ {0};
    size_t overshootSize_ {0};
};

class OldSpace : public SparseSpace {
public:
    OldSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity);
    ~OldSpace() override = default;
    NO_COPY_SEMANTIC(OldSpace);
    NO_MOVE_SEMANTIC(OldSpace);

    Region *TrySweepToGetSuitableRegion(size_t size);
    Region *TryToGetExclusiveRegion(size_t size);

    // CSet
    void SelectCSet();
    void CheckRegionSize();
    void RevertCSet();
    void ReclaimCSet();

    unsigned long GetSelectedRegionNumber() const
    {
        return std::max(committedSize_ / PARTIAL_GC_MAX_COLLECT_REGION_RATE, PARTIAL_GC_INITIAL_COLLECT_REGION_SIZE);
    }

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

    template<class Callback>
    void EnumerateCollectRegionSet(const Callback &cb) const
    {
        for (Region *current : collectRegionSet_) {
            if (current != nullptr) {
                cb(current);
            }
        }
    }

    void Merge(LocalSpace *localSpace);
private:
    static constexpr int64_t PARTIAL_GC_MAX_EVACUATION_SIZE_FOREGROUND = 2_MB;
    static constexpr int64_t PARTIAL_GC_MAX_EVACUATION_SIZE_BACKGROUND = 6_MB;
    static constexpr unsigned long long PARTIAL_GC_MAX_COLLECT_REGION_RATE = 2_MB;
    static constexpr unsigned long long PARTIAL_GC_INITIAL_COLLECT_REGION_SIZE = 24;
    static constexpr size_t PARTIAL_GC_MIN_COLLECT_REGION_SIZE = 5;

    CVector<Region *> collectRegionSet_;
    os::memory::Mutex lock_;
    size_t mergeSize_ {0};
};

class NonMovableSpace : public SparseSpace {
public:
    NonMovableSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity);
    ~NonMovableSpace() override = default;
    NO_COPY_SEMANTIC(NonMovableSpace);
    NO_MOVE_SEMANTIC(NonMovableSpace);

    uintptr_t  CheckAndAllocate(size_t size);
};

class AppSpawnSpace : public SparseSpace {
public:
    AppSpawnSpace(Heap *heap, size_t initialCapacity);
    ~AppSpawnSpace() override = default;
    NO_COPY_SEMANTIC(AppSpawnSpace);
    NO_MOVE_SEMANTIC(AppSpawnSpace);

    void IterateOverMarkedObjects(const std::function<void(TaggedObject *object)> &visitor) const;
};

class LocalSpace : public SparseSpace {
public:
    LocalSpace() = delete;
    LocalSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity);
    ~LocalSpace() override = default;
    NO_COPY_SEMANTIC(LocalSpace);
    NO_MOVE_SEMANTIC(LocalSpace);

    uintptr_t Allocate(size_t size, bool isExpand = true);
    bool AddRegionToList(Region *region);
    void FreeBumpPoint();
    void Stop();
};

class MachineCodeSpace : public SparseSpace {
public:
    MachineCodeSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity);
    ~MachineCodeSpace() override = default;
    NO_COPY_SEMANTIC(MachineCodeSpace);
    NO_MOVE_SEMANTIC(MachineCodeSpace);  // Note: Expand() left for define
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SPARSE_SPACE_H
