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

#ifndef ECMASCRIPT_MEM_REGION_H
#define ECMASCRIPT_MEM_REGION_H

#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/base/asan_interface.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/free_object_list.h"
#include "ecmascript/mem/gc_bitset.h"
#include "ecmascript/mem/remembered_set.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/platform/map.h"

#include "libpandabase/os/mutex.h"

#include "securec.h"

namespace panda {
namespace ecmascript {
class JSThread;

enum RegionSpaceFlag {
    UNINITIALIZED = 0,
    // We should avoid using the lower 3 bits (bits 0 to 2).
    // If ZAP_MEM is enabled, the value of the lower 3 bits conflicts with the INVALID_VALUE.

    // Bits 3 to 7 are reserved to denote the space where the region is located.
    IN_YOUNG_SPACE = 0x08,
    IN_SNAPSHOT_SPACE = 0x09,
    IN_HUGE_OBJECT_SPACE = 0x0A,
    IN_OLD_SPACE = 0x0B,
    IN_NON_MOVABLE_SPACE = 0x0C,
    IN_MACHINE_CODE_SPACE = 0x0D,
    IN_READ_ONLY_SPACE = 0x0E,
    IN_APPSPAWN_SPACE = 0X0F,

    VALID_SPACE_MASK = 0xFF,
};

enum RegionGCFlags {
    // We should avoid using the lower 3 bits (bits 0 to 2).
    // If ZAP_MEM is enabled, the value of the lower 3 bits conflicts with the INVALID_VALUE.

    // Below flags are used for GC, and each flag has a dedicated bit starting from the 3rd bit.
    NEVER_EVACUATE = 1 << 3,
    HAS_AGE_MARK = 1 << 4,
    BELOW_AGE_MARK = 1 << 5,
    IN_COLLECT_SET = 1 << 6,
    IN_NEW_TO_NEW_SET = 1 << 7,
    // Bits 8 to 10 (the lower 3 bits for the next byte) are also excluded for the sake of
    // INVALID_VALUE in ZAP_MEM.
    HAS_BEEN_SWEPT = 1 << 11,
    NEED_RELOCATE = 1 << 12,
};

static inline std::string ToSpaceTypeName(uint8_t space)
{
    switch (space) {
        case RegionSpaceFlag::IN_YOUNG_SPACE:
            return "young space";
        case RegionSpaceFlag::IN_SNAPSHOT_SPACE:
            return "snapshot space";
        case RegionSpaceFlag::IN_HUGE_OBJECT_SPACE:
            return "huge object space";
        case RegionSpaceFlag::IN_OLD_SPACE:
            return "old space";
        case RegionSpaceFlag::IN_NON_MOVABLE_SPACE:
            return "non movable space";
        case RegionSpaceFlag::IN_MACHINE_CODE_SPACE:
            return "machine code space";
        case RegionSpaceFlag::IN_READ_ONLY_SPACE:
            return "read only space";
        case RegionSpaceFlag::IN_APPSPAWN_SPACE:
            return "appspawn space";
        default:
            return "invalid space";
    }
}

// |---------------------------------------------------------------------------------------|
// |                                   Region (256 kb)                                     |
// |---------------------------------|--------------------------------|--------------------|
// |     Head (sizeof(Region))       |         Mark bitset (4kb)      |      Data          |
// |---------------------------------|--------------------------------|--------------------|

class Region {
public:
    Region(JSThread *thread, uintptr_t allocateBase, uintptr_t begin, uintptr_t end, RegionSpaceFlag spaceType)
        : packedData_(begin, end, spaceType),
          thread_(thread),
          allocateBase_(allocateBase),
          end_(end),
          highWaterMark_(end),
          aliveObject_(0),
          wasted_(0),
          snapshotData_(0)
    {
        lock_ = new os::memory::Mutex();
    }

    ~Region() = default;

    NO_COPY_SEMANTIC(Region);
    NO_MOVE_SEMANTIC(Region);

    void LinkNext(Region *next)
    {
        next_ = next;
    }

    Region *GetNext() const
    {
        return next_;
    }

    void LinkPrev(Region *prev)
    {
        prev_ = prev;
    }

    Region *GetPrev() const
    {
        return prev_;
    }

    uintptr_t GetBegin() const
    {
        return packedData_.begin_;
    }

    uintptr_t GetEnd() const
    {
        return end_;
    }

    uintptr_t GetHighWaterMark() const
    {
        return highWaterMark_;
    }

    size_t GetCapacity() const
    {
        return end_ - allocateBase_;
    }

    size_t GetSize() const
    {
        return end_ - packedData_.begin_;
    }

    JSThread *GetJSThread() const
    {
        return thread_;
    }

    bool IsGCFlagSet(RegionGCFlags flag) const
    {
        return (packedData_.flags_.gcFlags_ & flag) == flag;
    }

    void SetGCFlag(RegionGCFlags flag)
    {
        packedData_.flags_.gcFlags_ |= flag;
    }

    void ClearGCFlag(RegionGCFlags flag)
    {
        // NOLINTNEXTLINE(hicpp-signed-bitwise)
        packedData_.flags_.gcFlags_ &= ~flag;
    }

    std::string GetSpaceTypeName()
    {
        return ToSpaceTypeName(packedData_.flags_.spaceFlag_);
    }

    // Mark bitset
    GCBitset *GetMarkGCBitset() const;
    bool AtomicMark(void *address);
    void ClearMark(void *address);
    bool Test(void *addr) const;
    template <typename Visitor>
    void IterateAllMarkedBits(Visitor visitor) const;
    void ClearMarkGCBitset();
    // Cross region remembered set
    void InsertCrossRegionRSet(uintptr_t addr);
    void AtomicInsertCrossRegionRSet(uintptr_t addr);
    template <typename Visitor>
    void IterateAllCrossRegionBits(Visitor visitor) const;
    void ClearCrossRegionRSet();
    void ClearCrossRegionRSetInRange(uintptr_t start, uintptr_t end);
    void AtomicClearCrossRegionRSetInRange(uintptr_t start, uintptr_t end);
    void DeleteCrossRegionRSet();
    // Old to new remembered set
    void InsertOldToNewRSet(uintptr_t addr);
    void ClearOldToNewRSet(uintptr_t addr);
    template <typename Visitor>
    void IterateAllOldToNewBits(Visitor visitor);
    void ClearOldToNewRSet();
    void ClearOldToNewRSetInRange(uintptr_t start, uintptr_t end);
    void DeleteOldToNewRSet();

    void AtomicClearSweepingRSetInRange(uintptr_t start, uintptr_t end);
    void ClearSweepingRSetInRange(uintptr_t start, uintptr_t end);
    void DeleteSweepingRSet();
    template <typename Visitor>
    void AtomicIterateAllSweepingRSetBits(Visitor visitor);
    template <typename Visitor>
    void IterateAllSweepingRSetBits(Visitor visitor);

    static Region *ObjectAddressToRange(TaggedObject *obj)
    {
        return reinterpret_cast<Region *>(ToUintPtr(obj) & ~DEFAULT_REGION_MASK);
    }

    static Region *ObjectAddressToRange(uintptr_t objAddress)
    {
        return reinterpret_cast<Region *>(objAddress & ~DEFAULT_REGION_MASK);
    }

    void ClearMembers()
    {
        if (lock_ != nullptr) {
            delete lock_;
            lock_ = nullptr;
        }
    }

    void Invalidate()
    {
        ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<void *>(GetBegin()), GetSize());
        packedData_.flags_.spaceFlag_ = RegionSpaceFlag::UNINITIALIZED;
    }

    bool InYoungSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_YOUNG_SPACE;
    }

    bool InOldSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_OLD_SPACE;
    }

    bool InYoungOrOldSpace() const
    {
        return InYoungSpace() || InOldSpace();
    }

    bool InHugeObjectSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_HUGE_OBJECT_SPACE;
    }

    bool InMachineCodeSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_MACHINE_CODE_SPACE;
    }

    bool InNonMovableSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_NON_MOVABLE_SPACE;
    }

    bool InSnapshotSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_SNAPSHOT_SPACE;
    }

    bool InReadOnlySpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_READ_ONLY_SPACE;
    }

    bool InAppSpawnSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_APPSPAWN_SPACE;
    }

    bool InHeapSpace() const
    {
        uint8_t space = packedData_.flags_.spaceFlag_;
        return (space == RegionSpaceFlag::IN_YOUNG_SPACE ||
                space == RegionSpaceFlag::IN_OLD_SPACE ||
                space == RegionSpaceFlag::IN_HUGE_OBJECT_SPACE ||
                space == RegionSpaceFlag::IN_MACHINE_CODE_SPACE ||
                space == RegionSpaceFlag::IN_NON_MOVABLE_SPACE ||
                space == RegionSpaceFlag::IN_SNAPSHOT_SPACE ||
                space == RegionSpaceFlag::IN_READ_ONLY_SPACE ||
                space == RegionSpaceFlag::IN_APPSPAWN_SPACE);
    }

    bool InCollectSet() const
    {
        return IsGCFlagSet(RegionGCFlags::IN_COLLECT_SET);
    }

    bool InYoungSpaceOrCSet() const
    {
        return InYoungSpace() || InCollectSet();
    }

    bool InNewToNewSet() const
    {
        return IsGCFlagSet(RegionGCFlags::IN_NEW_TO_NEW_SET);
    }

    bool HasAgeMark() const
    {
        return IsGCFlagSet(RegionGCFlags::HAS_AGE_MARK);
    }

    bool BelowAgeMark() const
    {
        return IsGCFlagSet(RegionGCFlags::BELOW_AGE_MARK);
    }

    bool NeedRelocate() const
    {
        return IsGCFlagSet(RegionGCFlags::NEED_RELOCATE);
    }

    void SetSwept()
    {
        SetGCFlag(RegionGCFlags::HAS_BEEN_SWEPT);
    }

    void ResetSwept()
    {
        ClearGCFlag(RegionGCFlags::HAS_BEEN_SWEPT);
    }

    bool InRange(uintptr_t address) const
    {
        return address >= packedData_.begin_ && address <= end_;
    }

    uintptr_t GetAllocateBase() const
    {
        return allocateBase_;
    }

    size_t GetAllocatedBytes(uintptr_t top = 0)
    {
        ASSERT(top == 0 || InRange(top));
        return (top == 0) ? (highWaterMark_ - packedData_.begin_) : (top - packedData_.begin_);
    }

    void SetHighWaterMark(uintptr_t mark)
    {
        ASSERT(InRange(mark));
        highWaterMark_ = mark;
    }

    void SetReadOnlyAndMarked()
    {
        packedData_.markGCBitset_->SetAllBits(packedData_.bitsetSize_);
        PageProtect(reinterpret_cast<void *>(allocateBase_), GetCapacity(), PAGE_PROT_READ);
    }

    void ClearReadOnly()
    {
        PageProtect(reinterpret_cast<void *>(allocateBase_), GetCapacity(), PAGE_PROT_READWRITE);
    }

    void InitializeFreeObjectSets()
    {
        freeObjectSets_ = Span<FreeObjectSet *>(new FreeObjectSet *[FreeObjectList::NumberOfSets()](),
            FreeObjectList::NumberOfSets());
    }

    void DestroyFreeObjectSets()
    {
        for (auto set : freeObjectSets_) {
            delete set;
        }
        delete[] freeObjectSets_.data();
    }

    FreeObjectSet *GetFreeObjectSet(SetType type)
    {
        // Thread safe
        if (freeObjectSets_[type] == nullptr) {
            freeObjectSets_[type] = new FreeObjectSet(type);
        }
        return freeObjectSets_[type];
    }

    template<class Callback>
    void EnumerateFreeObjectSets(Callback cb)
    {
        for (auto set : freeObjectSets_) {
            cb(set);
        }
    }

    template<class Callback>
    void REnumerateFreeObjectSets(Callback cb)
    {
        auto last = freeObjectSets_.crbegin();
        auto first = freeObjectSets_.crend();
        for (; last != first; last++) {
            if (!cb(*last)) {
                break;
            }
        }
    }

    inline bool IsMarking() const;

    void IncreaseAliveObjectSafe(size_t size)
    {
        ASSERT(aliveObject_ + size <= GetSize());
        aliveObject_ += size;
    }

    void IncreaseAliveObject(size_t size)
    {
        ASSERT(aliveObject_ + size <= GetSize());
        aliveObject_.fetch_add(size, std::memory_order_relaxed);
    }

    void SetRegionAliveSize()
    {
        gcAliveSize_ = aliveObject_;
    }

    void ResetAliveObject()
    {
        aliveObject_ = 0;
    }

    size_t AliveObject() const
    {
        return aliveObject_.load(std::memory_order_relaxed);
    }

    size_t GetGCAliveSize() const
    {
        return gcAliveSize_;
    }

    bool MostObjectAlive() const
    {
        return aliveObject_ > MOST_OBJECT_ALIVE_THRESHOLD_PERCENT * GetSize();
    }

    bool BelowCompressThreasholdAlive() const
    {
        return gcAliveSize_ < COMPRESS_THREASHOLD_PERCENT * GetSize();
    }

    void ResetWasted()
    {
        wasted_ = 0;
    }

    void IncreaseWasted(uint64_t size)
    {
        wasted_ += size;
    }

    uint64_t GetWastedSize()
    {
        return wasted_;
    }

    uint64_t GetSnapshotData()
    {
        return snapshotData_;
    }

    void SetSnapshotData(uint64_t value)
    {
        snapshotData_ = value;
    }

    void SwapRSetForConcurrentSweeping()
    {
        sweepingRSet_ = packedData_.oldToNewSet_;
        packedData_.oldToNewSet_ = nullptr;
    }

    // should call in js-thread
    void MergeRSetForConcurrentSweeping();

    struct alignas(JSTaggedValue::TaggedTypeSize()) PackedPtr : public base::AlignedPointer {
        uint8_t spaceFlag_;
        uint16_t  gcFlags_;
    };

    struct PackedData : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedSize> {
        enum class Index : size_t {
            FlagIndex = 0,
            MarkGCBitSetIndex,
            OldToNewSetIndex,
            BeginIndex,
            BitSetSizeIndex,
            NumOfMembers
        };

        static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

        inline PackedData(uintptr_t begin, uintptr_t end, RegionSpaceFlag spaceType)
        {
            flags_.spaceFlag_ = spaceType;
            flags_.gcFlags_ = 0;
            bitsetSize_ = (spaceType == RegionSpaceFlag::IN_HUGE_OBJECT_SPACE) ?
                GCBitset::BYTE_PER_WORD : GCBitset::SizeOfGCBitset(end - begin);
            markGCBitset_ = new (ToVoidPtr(begin)) GCBitset();
            markGCBitset_->Clear(bitsetSize_);
            begin_ = AlignUp(begin + bitsetSize_, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            // The object region marked with poison until it is allocated if is_asan is true
            ASAN_POISON_MEMORY_REGION(reinterpret_cast<void *>(begin_), (end - begin_));
        }

        static size_t GetFlagOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::FlagIndex)>(isArch32);
        }

        static size_t GetGCBitsetOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::MarkGCBitSetIndex)>(isArch32);
        }

        static size_t GetOldToNewSetOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::OldToNewSetIndex)>(isArch32);
        }

        static size_t GetBeginOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::BeginIndex)>(isArch32);
        }

        alignas(EAS) PackedPtr flags_;
        alignas(EAS) GCBitset *markGCBitset_ {nullptr};
        alignas(EAS) RememberedSet *oldToNewSet_ {nullptr};
        alignas(EAS) uintptr_t begin_ {0};
        alignas(EAS) size_t bitsetSize_ {0};
    };
    STATIC_ASSERT_EQ_ARCH(sizeof(PackedData), PackedData::SizeArch32, PackedData::SizeArch64);

    static constexpr double MOST_OBJECT_ALIVE_THRESHOLD_PERCENT = 0.8;
    static constexpr double AVERAGE_REGION_EVACUATE_SIZE = MOST_OBJECT_ALIVE_THRESHOLD_PERCENT *
                                                           DEFAULT_REGION_SIZE / 2;  // 2 means half
private:
    static constexpr double COMPRESS_THREASHOLD_PERCENT = 0.1;

    RememberedSet *CreateRememberedSet();
    RememberedSet *GetOrCreateCrossRegionRememberedSet();
    RememberedSet *GetOrCreateOldToNewRememberedSet();

    PackedData packedData_;
    /*
     * The thread instance here is used by the GC barriers to get marking related information
     * and perform marking related operations. The barriers will indirectly access such information
     * via. the objects' associated regions.
     * fixme: Figure out a more elegant solution to bridging the barrier
     * and the information / operations it depends on. Then we can get rid of this from the region,
     * and consequently, the region allocator, the spaces using the region allocator, etc.
     */
    JSThread *thread_;

    uintptr_t allocateBase_;
    uintptr_t end_;
    uintptr_t highWaterMark_;
    std::atomic_size_t aliveObject_ {0};
    size_t gcAliveSize_ {0};
    Region *next_ {nullptr};
    Region *prev_ {nullptr};

    RememberedSet *crossRegionSet_ {nullptr};
    RememberedSet *sweepingRSet_ {nullptr};
    Span<FreeObjectSet *> freeObjectSets_;
    os::memory::Mutex *lock_ {nullptr};
    uint64_t wasted_;
    // snapshotdata_ is used to encode the region for snapshot. Its upper 32 bits are used to store the size of
    // the huge object, and the lower 32 bits are used to store the region index
    uint64_t snapshotData_;

    friend class Snapshot;
    friend class SnapshotProcessor;
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_MEM_REGION_H
