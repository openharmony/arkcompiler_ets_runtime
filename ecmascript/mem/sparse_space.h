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
#include "ecmascript/mem/jit_fort.h"

#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
#define CHECK_OBJECT_AND_INC_OBJ_SIZE(size)                                             \
    if (object != 0) {                                                                  \
        IncreaseLiveObjectSize(size);                                                   \
        if (!heap_->IsConcurrentFullMark() || heap_->IsReadyToConcurrentMark()) {       \
            Region::ObjectAddressToRange(object)->IncreaseAliveObject(size);            \
        }                                                                               \
        InvokeAllocationInspector(object, size, size);                                  \
        return object;                                                                  \
    }
#else
#define CHECK_OBJECT_AND_INC_OBJ_SIZE(size)                                             \
    if (object != 0) {                                                                  \
        IncreaseLiveObjectSize(size);                                                   \
        if (!heap_->IsConcurrentFullMark() || heap_->IsReadyToConcurrentMark()) {       \
            Region::ObjectAddressToRange(object)->IncreaseAliveObject(size);            \
        }                                                                               \
        return object;                                                                  \
    }
#endif

enum class SweepState : uint8_t {
    NO_SWEEP,
    SWEEPING,
    SWEPT
};

namespace panda::ecmascript {
class LocalSpace;
class SemiSpace;

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
    void ResetTopPointer(uintptr_t top);

    uintptr_t Allocate(size_t size, bool allowGC = true);
    bool Expand();

    // For sweeping
    virtual void PrepareSweeping();
    virtual void Sweep();
    virtual void AsyncSweep(bool isMain);

    bool TryFillSweptRegion();
    // Ensure All region finished sweeping
    bool FinishFillSweptRegion();

    void AddSweepingRegion(Region *region);
    void SortSweepingRegion();
    Region *GetSweepingRegionSafe();
    void AddSweptRegionSafe(Region *region);
    Region *GetSweptRegionSafe();
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

    void IncreaseOvershootSize(size_t size)
    {
        overshootSize_ += size;
    }

    size_t GetOvershootSize() const
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

    bool CommittedSizeExceed() const
    {
        return committedSize_ >= maximumCapacity_ + overshootSize_ + outOfMemoryOvershootSize_;
    }

    size_t GetTotalAllocatedSize() const;

    void InvokeAllocationInspector(Address object, size_t size, size_t alignedSize);

protected:
    FreeListAllocator<FreeObject> *allocator_;
    SweepState sweepState_ = SweepState::NO_SWEEP;
    Heap *localHeap_ {nullptr};
    size_t liveObjectSize_ {0};
    uintptr_t AllocateAfterSweepingCompleted(size_t size);

private:
    Mutex lock_;
    std::vector<Region *> sweepingList_;
    std::vector<Region *> sweptList_;
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

    uintptr_t AllocateFast(size_t size);
    uintptr_t AllocateSlow(size_t size, bool tryFast);
    // CSet
    void SelectCSet();
    void CheckRegionSize();
    void RevertCSet();
    void ReclaimCSet();

    bool SwapRegion(Region *region, SemiSpace *fromSpace);

    void PrepareSweepNewToOldRegions();
    void SweepNewToOldRegions();

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

    void IncreaseCommittedOverSizeLimit(size_t size)
    {
        committedOverSizeLimit_ += size;
    }

    void ResetCommittedOverSizeLimit()
    {
        DecreaseOutOfMemoryOvershootSize(committedOverSizeLimit_);
        committedOverSizeLimit_ = 0;
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

    size_t GetCollectSetRegionCount() const
    {
        return collectRegionSet_.size();
    }

    void Merge(LocalSpace *localSpace);
private:
    static constexpr int64_t PARTIAL_GC_MAX_EVACUATION_SIZE_FOREGROUND = 2_MB;
    static constexpr int64_t PARTIAL_GC_MAX_EVACUATION_SIZE_BACKGROUND = 6_MB;
    static constexpr unsigned long long PARTIAL_GC_MAX_COLLECT_REGION_RATE = 2_MB;
    static constexpr unsigned long long PARTIAL_GC_INITIAL_COLLECT_REGION_SIZE = 24;
    static constexpr size_t PARTIAL_GC_MIN_COLLECT_REGION_SIZE = 5;

    void FreeRegionFromSpace(Region *region);

    CVector<Region *> collectRegionSet_;
    Mutex lock_;
    size_t mergeSize_ {0};
    size_t committedOverSizeLimit_ {0};
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

    uintptr_t AllocateSync(size_t size);

    void IterateOverMarkedObjects(const std::function<void(TaggedObject *object)> &visitor) const;
private:
    Mutex mutex_;
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
private:
    void ForceExpandInGC();
};

class MachineCode;
struct MachineCodeDesc;
class MachineCodeSpace : public SparseSpace {
public:
    MachineCodeSpace(Heap *heap, size_t initialCapacity, size_t maximumCapacity);
    ~MachineCodeSpace() override;
    NO_COPY_SEMANTIC(MachineCodeSpace);
    NO_MOVE_SEMANTIC(MachineCodeSpace);  // Note: Expand() left for define
    uintptr_t GetMachineCodeObject(uintptr_t pc);
    size_t CheckMachineCodeObject(uintptr_t curPtr, uintptr_t &machineCode, uintptr_t pc);
    void AsyncSweep(bool isMain) override;
    void Sweep() override;
    void PrepareSweeping() override;
    uintptr_t Allocate(size_t size, bool allowGC = true);
    uintptr_t Allocate(size_t size, MachineCodeDesc *desc, bool allowGC = true);
    uintptr_t PUBLIC_API JitFortAllocate(MachineCodeDesc *desc);
    void StoreMachineCodeObjectLocation(uintptr_t start, uintptr_t end, uintptr_t address);

    inline void MarkJitFortMemAlive(MachineCode *obj)
    {
        if (jitFort_) {
            jitFort_->MarkJitFortMemAlive(obj);
        }
    }

    inline void MarkJitFortMemInstalled(MachineCode *obj)
    {
        if (jitFort_) {
            jitFort_->MarkJitFortMemInstalled(obj);
        }
    }

    inline bool IsSweeping()
    {
        return sweepState_ == SweepState::SWEEPING ;
    }

    inline JitFort *GetJitFort()
    {
        return jitFort_;
    }

    bool InJitFortRange(uintptr_t address) const
    {
        if (jitFort_) {
            return jitFort_->InRange(address);
        }
        return false;
    }

    // interval map is used to cache the location of machinecode object on heap.
    // This support GetMachineCodeObject
    struct IntervalMap {
        using TextStart = uintptr_t ;
        using TextEnd = uintptr_t;
        using MachineCodeAddress = uintptr_t;
        // For <MachineCode> text at [<start, end>), store <start> as key, and <end, MachineCode> as value.
        // Invariant is that intervals never overlap.
        std::map<TextStart, std::pair<TextEnd, MachineCodeAddress>> intervals;

        // Getter should only happen in enumroot
        MachineCodeAddress GetMachineCodeObject(uintptr_t pc)
        {
            // first interval whose start > pc
            auto it = intervals.upper_bound(pc);
            if (it == intervals.begin()) {
                UNREACHABLE();
            }
            -- it;
            auto start = it->first;
            auto end = it->second.first;
            if (pc >= start && pc < end) {
                return it->second.second;
            }
            UNREACHABLE();
        }

        void InsertMachineCodeObject(TextStart start, TextEnd end, MachineCodeAddress obj)
        {
            LOG_JIT(INFO) << "Insert MachineCodeObject 0x" << std::hex << obj << " , code range 0x" << std::hex << start
                          << " to 0x" << std::hex << end;
            // first interval whose start > our start
            auto it = intervals.upper_bound(start);
            // find first overlapping interval, if any
            if (it != intervals.begin()) {
                auto prev = std::prev(it);
                auto prevEnd = prev->second.first;
                if (prevEnd > start) {
                    it = prev;
                }
            }
            // Remove all overlapping intervals, if any
            while (it != intervals.end()) {
                if (it->first < end) {
                    LOG_JIT(INFO) << "MachoneCodeObject at 0x" << std::hex << it->second.second
                                  << " was freed: 0x" << std::hex << it->first
				  << " - 0x" << std::hex << it->second.first;
                    it = intervals.erase(it);
                } else {
                    break;
                }
            }
            intervals[start] = {end, obj};
        }
    };

private:
    IntervalMap machineCodeObjectLocations;
    JitFort *jitFort_ {nullptr};
    Mutex asyncSweepMutex_;
    friend class Heap;
    friend class ConcurrentSweeper;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SPARSE_SPACE_H
