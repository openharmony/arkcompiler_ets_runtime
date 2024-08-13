/*
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_SPACE_H
#define ECMASCRIPT_MEM_SPACE_H

#include "ecmascript/mem/allocation_inspector.h"
#include "ecmascript/mem/allocator.h"
#include "ecmascript/mem/c_containers.h"
#include "ecmascript/mem/ecma_list.h"
#include "ecmascript/mem/heap_region_allocator.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/region.h"

#include "securec.h"

namespace panda::ecmascript {
enum MemSpaceType {
    OLD_SPACE = 0,
    NON_MOVABLE,
    MACHINE_CODE_SPACE,
    HUGE_OBJECT_SPACE,
    EDEN_SPACE,
    SEMI_SPACE,
    SNAPSHOT_SPACE,
    COMPRESS_SPACE,
    LOCAL_SPACE,
    READ_ONLY_SPACE,
    APPSPAWN_SPACE,
    HUGE_MACHINE_CODE_SPACE,
    SHARED_OLD_SPACE,
    SHARED_NON_MOVABLE,
    SHARED_READ_ONLY_SPACE,
    SHARED_HUGE_OBJECT_SPACE,
    SHARED_LOCAL_SPACE,
    SHARED_COMPRESS_SPACE,
    SHARED_APPSPAWN_SPACE,
    SPACE_TYPE_LAST,  // Count of different types

    SHARED_BEGIN = SHARED_OLD_SPACE,
    SHARED_END = SHARED_HUGE_OBJECT_SPACE,
    // Free region means memory maybe always in use and can not be evacuated
    FREE_LIST_NUM = MACHINE_CODE_SPACE - OLD_SPACE + 1,
    SHARED_SWEEPING_SPACE_BEGIN = SHARED_OLD_SPACE,
    SHARED_SWEEPING_SPACE_END = SHARED_NON_MOVABLE,
    SHARED_SWEEPING_SPACE_NUM = SHARED_SWEEPING_SPACE_END - SHARED_SWEEPING_SPACE_BEGIN + 1,
};

enum class MemSpaceKind {
    LOCAL = 0,
    SHARED = 1
};

enum class AllocateEventType {
    NORMAL,
    DESERIALIZE,
};

static inline bool IsSMemSpace(MemSpaceType type)
{
    return (type >= MemSpaceType::SHARED_BEGIN) && (type <= MemSpaceType::SHARED_END);
}

static inline std::string ToSpaceTypeName(MemSpaceType type)
{
    switch (type) {
        case OLD_SPACE:
            return "old space";
        case NON_MOVABLE:
            return "non movable space";
        case MACHINE_CODE_SPACE:
            return "machine code space";
        case HUGE_OBJECT_SPACE:
            return "huge object space";
        case EDEN_SPACE:
            return "eden space";
        case SEMI_SPACE:
            return "semi space";
        case SNAPSHOT_SPACE:
            return "snapshot space";
        case COMPRESS_SPACE:
            return "compress space";
        case LOCAL_SPACE:
            return "local space";
        case READ_ONLY_SPACE:
            return "read only space";
        case APPSPAWN_SPACE:
            return "appspawn space";
        case HUGE_MACHINE_CODE_SPACE:
            return "huge machine code space";
        case SHARED_NON_MOVABLE:
            return "shared non movable space";
        case SHARED_OLD_SPACE:
            return "shared old space";
        case SHARED_READ_ONLY_SPACE:
            return "shared read only space";
        case SHARED_HUGE_OBJECT_SPACE:
            return "shared huge object space";
        case SHARED_COMPRESS_SPACE:
            return "compress space";
        case SHARED_LOCAL_SPACE:
            return "shared local space";
        case SHARED_APPSPAWN_SPACE:
            return "shared appspawn space";
        default:
            return "unknown space";
    }
}

class Space {
public:
    Space(BaseHeap* heap, HeapRegionAllocator *regionAllocator, MemSpaceType spaceType, size_t initialCapacity,
          size_t maximumCapacity);
    virtual ~Space() = default;
    NO_COPY_SEMANTIC(Space);
    NO_MOVE_SEMANTIC(Space);

    size_t GetMaximumCapacity() const
    {
        return maximumCapacity_;
    }

    void SetMaximumCapacity(size_t maximumCapacity)
    {
        maximumCapacity_ = maximumCapacity;
    }

    size_t GetOverShootMaximumCapacity() const
    {
        return maximumCapacity_ + outOfMemoryOvershootSize_;
    }

    size_t GetInitialCapacity() const
    {
        return initialCapacity_;
    }

    void SetInitialCapacity(size_t initialCapacity)
    {
        initialCapacity_ = initialCapacity;
    }

    size_t GetCommittedSize() const
    {
        return committedSize_;
    }

    void IncreaseCommitted(size_t bytes)
    {
        committedSize_ += bytes;
    }

    void DecreaseCommitted(size_t bytes)
    {
        committedSize_ -= bytes;
    }

    void IncreaseObjectSize(size_t bytes)
    {
        objectSize_ += bytes;
    }

    void DecreaseObjectSize(size_t bytes)
    {
        objectSize_ -= bytes;
    }

    size_t GetObjectSize()
    {
        return objectSize_;
    }

    size_t GetOutOfMemoryOvershootSize() const
    {
        return outOfMemoryOvershootSize_;
    }

    void IncreaseOutOfMemoryOvershootSize(size_t size)
    {
        outOfMemoryOvershootSize_ += size;
    }

    void DecreaseOutOfMemoryOvershootSize(size_t size)
    {
        ASSERT(outOfMemoryOvershootSize_ >= size);
        outOfMemoryOvershootSize_ -= size;
    }

    MemSpaceType GetSpaceType() const
    {
        return spaceType_;
    }

    inline RegionSpaceFlag GetRegionFlag() const;

    uintptr_t GetAllocateAreaBegin() const
    {
        return regionList_.GetLast()->GetBegin();
    }

    uintptr_t GetAllocateAreaEnd() const
    {
        return regionList_.GetLast()->GetEnd();
    }

    Region *GetCurrentRegion() const
    {
        return regionList_.GetLast();
    }

    Region *GetFirstRegion() const
    {
        return regionList_.GetFirst();
    }

    uint32_t GetRegionCount()
    {
        return regionList_.GetLength();
    }

    EcmaList<Region> &GetRegionList()
    {
        return regionList_;
    }

    const EcmaList<Region> &GetRegionList() const
    {
        return regionList_;
    }

    void SetRecordRegion()
    {
        recordRegion_ = GetCurrentRegion();
    }

    bool IsOOMDumpSpace()
    {
        return spaceType_ == SEMI_SPACE || spaceType_ == OLD_SPACE || spaceType_ == NON_MOVABLE ||
            spaceType_ == HUGE_OBJECT_SPACE;
    }

    // methods for allocation inspector
    void AddAllocationInspector(AllocationInspector* inspector);
    void ClearAllocationInspector();
    void SwapAllocationCounter(Space *space);

    template <class Callback>
    inline void EnumerateRegions(const Callback &cb, Region *region = nullptr) const;
    template <class Callback>
    inline void EnumerateRegionsWithRecord(const Callback &cb) const;

    inline void AddRegion(Region *region);
    inline void RemoveRegion(Region *region);

    virtual void Initialize() {};
    void Destroy();

    void ReclaimRegions(size_t cachedSize = 0);

protected:
    void ClearAndFreeRegion(Region *region, size_t cachedSize = 0);
    
    BaseHeap *heap_ {nullptr};
    HeapRegionAllocator *heapRegionAllocator_ {nullptr};
    EcmaList<Region> regionList_ {};
    MemSpaceType spaceType_ {};
    size_t initialCapacity_ {0};
    size_t maximumCapacity_ {0};
    size_t committedSize_ {0};
    size_t objectSize_ {0};
    size_t outOfMemoryOvershootSize_ {0};
    Region *recordRegion_ {nullptr};
    AllocationCounter allocationCounter_;
};

class HugeObjectSpace : public Space {
public:
    HugeObjectSpace(Heap *heap, HeapRegionAllocator *regionAllocator, size_t initialCapacity,
                    size_t maximumCapacity);
    HugeObjectSpace(Heap *heap, HeapRegionAllocator *regionAllocator, size_t initialCapacity,
                    size_t maximumCapacity, MemSpaceType spaceType);
    ~HugeObjectSpace() override = default;
    NO_COPY_SEMANTIC(HugeObjectSpace);
    NO_MOVE_SEMANTIC(HugeObjectSpace);
    // Sometimes it is unsafe to checkSafePoint here, e.g. in deserialize, if do checkSafePoint JSThread may be
    // suspended and then do SharedGC, which will free some regions in SharedHeap that are allocated at the beginning
    // of deserializing for further object allocating, but no object has been allocated on at this moment.
    uintptr_t Allocate(size_t objectSize, JSThread *thread, AllocateEventType allocType = AllocateEventType::NORMAL);
    void Sweep();
    size_t GetHeapObjectSize() const;
    void IterateOverObjects(const std::function<void(TaggedObject *object)> &objectVisitor) const;

    void ReclaimHugeRegion();

    void InvokeAllocationInspector(Address object, size_t objectSize);

protected:
    static constexpr size_t HUGE_OBJECT_BITSET_SIZE = 16;
private:
    EcmaList<Region> hugeNeedFreeList_ {};
};

class HugeMachineCodeSpace : public HugeObjectSpace {
public:
    HugeMachineCodeSpace(Heap *heap, HeapRegionAllocator *regionAllocator, size_t initialCapacity,
                         size_t maximumCapacity);
    uintptr_t GetMachineCodeObject(uintptr_t pc) const;
    uintptr_t Allocate(size_t objectSize, JSThread *thread, void *desc,
        AllocateEventType allocType = AllocateEventType::NORMAL);
    uintptr_t Allocate(size_t objectSize, JSThread *thread);
    Region *AllocateFort(size_t objectSize, JSThread *thread, void *desc);
};

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SPACE_H
