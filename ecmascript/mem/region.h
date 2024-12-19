/*
 * Copyright (c) 2022-2024 Huawei Device Co., Ltd.
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

#include <type_traits>

#include "ecmascript/base/aligned_struct.h"
#include "ecmascript/base/asan_interface.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/free_object_list.h"
#include "ecmascript/mem/gc_bitset.h"
#include "ecmascript/mem/remembered_set.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/platform/map.h"

#include "ecmascript/platform/mutex.h"

#include "securec.h"

namespace panda {
namespace ecmascript {
class JSThread;

enum RegionSpaceFlag {
    UNINITIALIZED = 0,
    // We should avoid using the lower 3 bits (bits 0 to 2).
    // If ZAP_MEM is enabled, the value of the lower 3 bits conflicts with the INVALID_VALUE.

    // Bits 3 to 7 are reserved to denote the space where the region is located.
    IN_EDEN_SPACE = 0x08,
    IN_YOUNG_SPACE = 0x09,
    IN_SNAPSHOT_SPACE = 0x0A,
    IN_HUGE_OBJECT_SPACE = 0x0B,
    IN_OLD_SPACE = 0x0C,
    IN_NON_MOVABLE_SPACE = 0x0D,
    IN_MACHINE_CODE_SPACE = 0x0E,
    IN_READ_ONLY_SPACE = 0X0F,
    IN_APPSPAWN_SPACE = 0x10,
    IN_HUGE_MACHINE_CODE_SPACE = 0x11,
    IN_SHARED_NON_MOVABLE = 0x12,
    IN_SHARED_OLD_SPACE = 0x13,
    IN_SHARED_APPSPAWN_SPACE = 0X14,
    IN_SHARED_HUGE_OBJECT_SPACE = 0x15,
    IN_SHARED_READ_ONLY_SPACE = 0x16,

    VALID_SPACE_MASK = 0xFF,

    GENERAL_YOUNG_BEGIN = IN_EDEN_SPACE,
    GENERAL_YOUNG_END = IN_YOUNG_SPACE,
    GENERAL_OLD_BEGIN = IN_SNAPSHOT_SPACE,
    GENERAL_OLD_END = IN_HUGE_MACHINE_CODE_SPACE,
    SHARED_SPACE_BEGIN = IN_SHARED_NON_MOVABLE,
    SHARED_SPACE_END = IN_SHARED_READ_ONLY_SPACE,
    SHARED_SWEEPABLE_SPACE_BEGIN = IN_SHARED_NON_MOVABLE,
    SHARED_SWEEPABLE_SPACE_END = IN_SHARED_HUGE_OBJECT_SPACE,

    HEAP_SPACE_BEGIN = IN_EDEN_SPACE,
    HEAP_SPACE_END = IN_SHARED_READ_ONLY_SPACE
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
    // ONLY used for heap verification.
    IN_INACTIVE_SEMI_SPACE = 1 << 13,
    IN_NEW_TO_OLD_SET = 1 << 14,
};

// Currently only use for region in LinearSpace, to check if the region is allocated during concurrent marking.
enum class RegionTypeFlag : uint8_t {
    DEFAULT = 0,
    // We should avoid using the lower 3 bits (bits 0 to 2).
    // If ZAP_MEM is enabled, the value of the lower 3 bits conflicts with the INVALID_VALUE.

    // Region is allocated before concurrent marking, but some new object may be allocated here
    // during concurrent marking.
    HALF_FRESH = 0x08,
    // Region is allocated during concurrent marking.
    FRESH = 0x09,
};

enum RSetType {
    OLD_TO_NEW,
    LOCAL_TO_SHARE,
};

static inline std::string ToSpaceTypeName(uint8_t space)
{
    switch (space) {
        case RegionSpaceFlag::IN_EDEN_SPACE:
            return "eden space";
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
        case RegionSpaceFlag::IN_HUGE_MACHINE_CODE_SPACE:
            return "huge machine code space";
        case RegionSpaceFlag::IN_SHARED_NON_MOVABLE:
            return "shared non movable space";
        case RegionSpaceFlag::IN_SHARED_OLD_SPACE:
            return "shared old space";
        case RegionSpaceFlag::IN_SHARED_READ_ONLY_SPACE:
            return "shared read only space";
        case RegionSpaceFlag::IN_SHARED_HUGE_OBJECT_SPACE:
            return "shared huge object space";
        case RegionSpaceFlag::IN_SHARED_APPSPAWN_SPACE:
            return "shared appspawn space";
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
    Region(NativeAreaAllocator *allocator, uintptr_t allocateBase, uintptr_t begin, uintptr_t end,
        RegionSpaceFlag spaceType, RegionTypeFlag typeFlag)
        : packedData_(begin, end, spaceType, typeFlag),
          nativeAreaAllocator_(allocator),
          allocateBase_(allocateBase),
          end_(end),
          highWaterMark_(end),
          aliveObject_(0),
          wasted_(0),
          snapshotData_(0) {}

    // JitFort space is divided into regions (JitForRegion) to enable
    // reusing free_object_list and free_object_set operations for
    // JitFort space, and GC marking actually happens in corresponding
    // MachineCode objects where JitFort space is allocated to. So no
    // gc mark bits needed in JitFortRegions.
    Region(NativeAreaAllocator *allocator, uintptr_t allocateBase, uintptr_t end,
        RegionSpaceFlag spaceType)
        : packedData_(allocateBase, spaceType), // no markGCBitset_ for JitFort
          nativeAreaAllocator_(allocator),
          allocateBase_(allocateBase),
          end_(end),
          highWaterMark_(end),
          aliveObject_(0),
          wasted_(0),
          snapshotData_(0) {}

    ~Region() = default;

    NO_COPY_SEMANTIC(Region);
    NO_MOVE_SEMANTIC(Region);

    enum RegionSpaceKind { InYoung, InGeneralOld, Other };

    template <RegionSpaceKind kind>
    class Updater final {
    public:
        Updater(uintptr_t updateAddress, Region& region)
            : bitsetUpdater_(updateAddress),
              region_(region)
        {
        }

        NO_COPY_SEMANTIC(Updater);

        ARK_INLINE ~Updater()
        {
            Flush();
        }

        ARK_INLINE void UpdateLocalToShare()
        {
            bitsetUpdater_.Update(LocalToShareIdx);
        }

        template <RegionSpaceKind T = kind, std::enable_if_t<T == InYoung, int>  = 0>
        ARK_INLINE void UpdateNewToEden()
        {
            bitsetUpdater_.Update(NewToEdenIdx);
        }

        template <RegionSpaceKind T = kind, std::enable_if_t<T == InGeneralOld, int>  = 0>
        ARK_INLINE void UpdateOldToNew()
        {
            bitsetUpdater_.Update(OldToNewIdx);
        }

        ARK_INLINE void Next()
        {
            if (bitsetUpdater_.Next()) {
                Flush();
            }
        }

    private:
        ARK_INLINE void Consume(size_t idx, uintptr_t updateAddress, uint32_t mask);

        ARK_INLINE void Flush();

        static constexpr size_t CalculateBitSetNum()
        {
            constexpr size_t InYoungBitSetNum = 2;
            constexpr size_t InGeneralOldBitSetNum = 2;
            constexpr size_t OtherBitSetNum = 1;
            switch (kind) {
                case InYoung:
                    return InYoungBitSetNum;
                case InGeneralOld:
                    return InGeneralOldBitSetNum;
                case Other:
                    return OtherBitSetNum;
            }
            return 0;
        }

        static constexpr size_t BitSetNum = CalculateBitSetNum();
        static constexpr size_t LocalToShareIdx = 0;
        static constexpr size_t NewToEdenIdx = 1; // NewToEden and OldToNew can't be used at same time.
        static constexpr size_t OldToNewIdx = 1;
        GCBitSetUpdater<BitSetNum> bitsetUpdater_;
        Region& region_;
    };

    void Initialize()
    {
        lock_ = new Mutex();
        if (InSparseSpace()) {
            InitializeFreeObjectSets();
        }
    }

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

    uint8_t GetSpaceType() const
    {
        return packedData_.flags_.spaceFlag_;
    }

    // Mark bitset
    GCBitset *GetMarkGCBitset() const;
    bool AtomicMark(void *address);
    // Objects in fresh region should only mark in JS Thread.
    bool NonAtomicMark(void *address);
    void ClearMark(void *address);
    bool Test(void *addr) const;
    bool Test(uintptr_t addr) const;
    // ONLY used for heap verification.
    bool TestNewToEden(uintptr_t addr);
    bool TestOldToNew(uintptr_t addr);
    bool TestLocalToShare(uintptr_t addr);
    template <typename Visitor>
    void IterateAllMarkedBits(Visitor &&visitor) const;
    void ClearMarkGCBitset();
    // local to share remembered set
    bool HasLocalToShareRememberedSet() const;
    RememberedSet *ExtractLocalToShareRSet();
    void InsertLocalToShareRSet(uintptr_t addr);
    template<RegionSpaceKind kind>
    Updater<kind> GetBatchRSetUpdater(uintptr_t addr);
    void AtomicInsertLocalToShareRSet(uintptr_t addr);
    void ClearLocalToShareRSetInRange(uintptr_t start, uintptr_t end);
    void AtomicClearLocalToShareRSetInRange(uintptr_t start, uintptr_t end);
    void AtomicClearSweepingLocalToShareRSetInRange(uintptr_t start, uintptr_t end);
    template <typename Visitor>
    void IterateAllLocalToShareBits(Visitor visitor);
    void DeleteLocalToShareRSet();
    void DeleteSweepingLocalToShareRSet();
    // Cross region remembered set
    void InsertCrossRegionRSet(uintptr_t addr);
    void AtomicInsertCrossRegionRSet(uintptr_t addr);
    template <typename Visitor>
    void IterateAllCrossRegionBits(Visitor visitor) const;
    void ClearCrossRegionRSet();
    void ClearCrossRegionRSetInRange(uintptr_t start, uintptr_t end);
    void AtomicClearCrossRegionRSetInRange(uintptr_t start, uintptr_t end);
    void DeleteCrossRegionRSet();
    // New to eden remembered set
    void InsertNewToEdenRSet(uintptr_t addr);
    void AtomicInsertNewToEdenRSet(uintptr_t addr);
    void ClearNewToEdenRSet(uintptr_t addr);
    // Old to new remembered set
    void InsertOldToNewRSet(uintptr_t addr);
    void ClearOldToNewRSet(uintptr_t addr);

    template <typename Visitor>
    void IterateAllNewToEdenBits(Visitor visitor);
    template <typename Visitor>
    void IterateAllOldToNewBits(Visitor visitor);
    RememberedSet* GetNewToEdenRSet();
    void ClearNewToEdenRSet();
    void ClearNewToEdenRSetInRange(uintptr_t start, uintptr_t end);
    void DeleteNewToEdenRSet();
    void ClearOldToNewRSet();
    void ClearOldToNewRSetInRange(uintptr_t start, uintptr_t end);
    void DeleteOldToNewRSet();

    void AtomicClearSweepingOldToNewRSetInRange(uintptr_t start, uintptr_t end);
    void ClearSweepingOldToNewRSetInRange(uintptr_t start, uintptr_t end);
    void DeleteSweepingOldToNewRSet();
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

    static size_t GetRegionAvailableSize()
    {
        size_t regionHeaderSize = AlignUp(sizeof(Region), static_cast<size_t>(MemAlignment::MEM_ALIGN_REGION));
        size_t bitsetSize = GCBitset::SizeOfGCBitset(DEFAULT_REGION_SIZE - regionHeaderSize);
        return DEFAULT_REGION_SIZE - regionHeaderSize - bitsetSize;
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

    void ResetRegionFlag(RegionSpaceFlag spaceFlag, RegionGCFlags gcFlag)
    {
        packedData_.flags_.spaceFlag_ = spaceFlag;
        packedData_.flags_.gcFlags_ = gcFlag;
    }

    uint8_t GetRegionSpaceFlag();

    void SetRegionSpaceFlag(RegionSpaceFlag flag)
    {
        packedData_.flags_.spaceFlag_ = flag;
    }
    bool InEdenSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_EDEN_SPACE;
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
        return InGeneralNewSpace() || InOldSpace();
    }

    bool InGeneralNewSpace() const
    {
        auto flag = packedData_.flags_.spaceFlag_;
        return flag >= RegionSpaceFlag::GENERAL_YOUNG_BEGIN && flag <= RegionSpaceFlag::GENERAL_YOUNG_END;
    }

    bool InGeneralOldSpace() const
    {
        ASSERT(packedData_.flags_.spaceFlag_ != 0);
        auto flag = packedData_.flags_.spaceFlag_;
        return flag >= RegionSpaceFlag::GENERAL_OLD_BEGIN && flag <= RegionSpaceFlag::GENERAL_OLD_END;
    }

    bool InHugeObjectSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_HUGE_OBJECT_SPACE;
    }

    bool InMachineCodeSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_MACHINE_CODE_SPACE;
    }

    bool InHugeMachineCodeSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_HUGE_MACHINE_CODE_SPACE;
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

    bool InSharedOldSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_SHARED_OLD_SPACE;
    }

    bool InSharedNonMovableSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_SHARED_NON_MOVABLE;
    }

    bool InSharedHugeObjectSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_SHARED_HUGE_OBJECT_SPACE;
    }

    bool InSharedReadOnlySpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_SHARED_READ_ONLY_SPACE;
    }

    bool InSharedAppSpawnSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_SHARED_APPSPAWN_SPACE;
    }

    bool InAppSpawnSpace() const
    {
        return packedData_.flags_.spaceFlag_ == RegionSpaceFlag::IN_APPSPAWN_SPACE;
    }

    // Not including shared read only space.
    bool InSharedSweepableSpace() const
    {
        auto flag = packedData_.flags_.spaceFlag_;
        return flag >= RegionSpaceFlag::SHARED_SWEEPABLE_SPACE_BEGIN &&
               flag <= RegionSpaceFlag::SHARED_SWEEPABLE_SPACE_END;
    }

    bool InSharedHeap() const
    {
        auto flag = packedData_.flags_.spaceFlag_;
        return flag >= RegionSpaceFlag::SHARED_SPACE_BEGIN && flag <= RegionSpaceFlag::SHARED_SPACE_END;
    }

    bool InSparseSpace() const
    {
        auto flag = packedData_.flags_.spaceFlag_;
        switch (flag) {
            case RegionSpaceFlag::IN_OLD_SPACE:
            case RegionSpaceFlag::IN_NON_MOVABLE_SPACE:
            case RegionSpaceFlag::IN_MACHINE_CODE_SPACE:
            case RegionSpaceFlag::IN_APPSPAWN_SPACE:
            case RegionSpaceFlag::IN_SHARED_NON_MOVABLE:
            case RegionSpaceFlag::IN_SHARED_OLD_SPACE:
                return true;
            default:
                return false;
        }
    }

    bool InHeapSpace() const
    {
        uint8_t space = packedData_.flags_.spaceFlag_;
        return space >= RegionSpaceFlag::HEAP_SPACE_BEGIN && space <= RegionSpaceFlag::HEAP_SPACE_END;
    }

    bool InCollectSet() const
    {
        return IsGCFlagSet(RegionGCFlags::IN_COLLECT_SET);
    }

    bool InGeneralNewSpaceOrCSet() const
    {
        return InGeneralNewSpace() || InCollectSet();
    }

    bool InNewToNewSet() const
    {
        return IsGCFlagSet(RegionGCFlags::IN_NEW_TO_NEW_SET);
    }

    bool InNewToOldSet() const
    {
        return IsGCFlagSet(RegionGCFlags::IN_NEW_TO_OLD_SET);
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

    // ONLY used for heap verification.
    bool InInactiveSemiSpace() const
    {
        return IsGCFlagSet(RegionGCFlags::IN_INACTIVE_SEMI_SPACE);
    }

    // ONLY used for heap verification.
    bool InActiveSemiSpace() const
    {
        return InYoungSpace() && !InInactiveSemiSpace();
    }

    RegionTypeFlag GetRegionTypeFlag() const
    {
        return packedData_.typeFlag_;
    }

    void SetRegionTypeFlag(RegionTypeFlag typeFlag)
    {
        packedData_.typeFlag_ = typeFlag;
    }

    void ResetRegionTypeFlag()
    {
        SetRegionTypeFlag(RegionTypeFlag::DEFAULT);
    }

    bool IsFreshRegion() const
    {
        return GetRegionTypeFlag() == RegionTypeFlag::FRESH;
    }

    bool IsHalfFreshRegion() const
    {
        return GetRegionTypeFlag() == RegionTypeFlag::HALF_FRESH;
    }

    // ONLY used for heap verification.
    void SetInactiveSemiSpace()
    {
        SetGCFlag(RegionGCFlags::IN_INACTIVE_SEMI_SPACE);
    }

    // ONLY used for heap verification.
    void ResetInactiveSemiSpace()
    {
        ClearGCFlag(RegionGCFlags::IN_INACTIVE_SEMI_SPACE);
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
        FreeObjectSet<FreeObject> **sets = new FreeObjectSet<FreeObject> *[FreeObjectList<FreeObject>::NumberOfSets()];
        for (int i = 0; i < FreeObjectList<FreeObject>::NumberOfSets(); i++) {
            sets[i] = new FreeObjectSet<FreeObject>(i);
        }
        freeObjectSets_ = Span<FreeObjectSet<FreeObject> *>(sets, FreeObjectList<FreeObject>::NumberOfSets());
    }

    void DestroyFreeObjectSets()
    {
        for (int i = 0; i < FreeObjectList<FreeObject>::NumberOfSets(); i++) {
            delete freeObjectSets_[i];
            freeObjectSets_[i] = nullptr;
        }
        delete[] freeObjectSets_.data();
    }

    FreeObjectSet<FreeObject> *GetFreeObjectSet(SetType type)
    {
        // Thread safe
        if (freeObjectSets_[type] == nullptr) {
            freeObjectSets_[type] = new FreeObjectSet<FreeObject>(type);
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

    void IncreaseAliveObjectSafe(size_t size)
    {
        ASSERT(aliveObject_ + size <= GetSize());
        aliveObject_ += size;
    }

    void IncreaseAliveObject(size_t size)
    {
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

    void SwapOldToNewRSetForCS()
    {
        sweepingOldToNewRSet_ = packedData_.oldToNewSet_;
        packedData_.oldToNewSet_ = nullptr;
    }

    void SwapLocalToShareRSetForCS()
    {
        sweepingLocalToShareRSet_ = packedData_.localToShareSet_;
        packedData_.localToShareSet_ = nullptr;
    }

    void SetLocalHeap(uintptr_t localHeap)
    {
        ASSERT(localHeap != (uintptr_t)nullptr);
        localHeap_ = localHeap;
    }

    uintptr_t GetLocalHeap(void)
    {
        return localHeap_;
    }

    // should call in js-thread
    void MergeOldToNewRSetForCS();
    void MergeLocalToShareRSetForCS();

    // should call in daemon-thread, or in js-thread in RUNNING state
    void MergeLocalToShareRSetForCM(RememberedSet *set);

    struct alignas(JSTaggedValue::TaggedTypeSize()) PackedPtr : public base::AlignedPointer {
        uint8_t spaceFlag_;
        uint16_t  gcFlags_;
    };

    struct PackedData : public base::AlignedStruct<JSTaggedValue::TaggedTypeSize(),
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedPointer,
                                                 base::AlignedSize> {
        enum class Index : size_t {
            FlagsIndex = 0,
            TypeFlagIndex,
            MarkGCBitSetIndex,
            OldToNewSetIndex,
            LocalToShareSetIndex,
            BeginIndex,
            BitSetSizeIndex,
            NumOfMembers
        };

        static_assert(static_cast<size_t>(Index::NumOfMembers) == NumOfTypes);

        inline PackedData(uintptr_t begin, uintptr_t end, RegionSpaceFlag spaceType, RegionTypeFlag typeFlag)
        {
            flags_.spaceFlag_ = spaceType;
            flags_.gcFlags_ = 0;
            typeFlag_ = typeFlag;
            bitsetSize_ = (spaceType == RegionSpaceFlag::IN_HUGE_OBJECT_SPACE ||
                           spaceType == RegionSpaceFlag::IN_HUGE_MACHINE_CODE_SPACE ||
                           spaceType == RegionSpaceFlag::IN_SHARED_HUGE_OBJECT_SPACE) ?
                GCBitset::BYTE_PER_WORD : GCBitset::SizeOfGCBitset(end - begin);
            markGCBitset_ = new (ToVoidPtr(begin)) GCBitset();
            markGCBitset_->Clear(bitsetSize_);
            begin_ = AlignUp(begin + bitsetSize_, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT));
            // The object region marked with poison until it is allocated if is_asan is true
#ifdef ARK_ASAN_ON
            ASAN_POISON_MEMORY_REGION(reinterpret_cast<void *>(begin_), (end - begin_));
#endif
        }

        inline PackedData(uintptr_t begin, RegionSpaceFlag spaceType)
        {
            flags_.spaceFlag_ = spaceType;
            flags_.gcFlags_ = 0;
            typeFlag_ = RegionTypeFlag::DEFAULT;
            // no markGCBitset
            begin_ = begin;
            markGCBitset_ = nullptr;
        }

        static size_t GetFlagsOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::FlagsIndex)>(isArch32);
        }

        static size_t GetTypeFlagOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::TypeFlagIndex)>(isArch32);
        }

        static size_t GetGCBitsetOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::MarkGCBitSetIndex)>(isArch32);
        }

        static size_t GetNewToEdenSetOffset(bool isArch32)
        {
            // NewToEdenRSet is Union with OldToNewRSet
            return GetOffset<static_cast<size_t>(Index::OldToNewSetIndex)>(isArch32);
        }

        static size_t GetOldToNewSetOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::OldToNewSetIndex)>(isArch32);
        }

        static size_t GetLocalToShareSetOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::LocalToShareSetIndex)>(isArch32);
        }

        static size_t GetBeginOffset(bool isArch32)
        {
            return GetOffset<static_cast<size_t>(Index::BeginIndex)>(isArch32);
        }

        alignas(EAS) PackedPtr flags_;
        // Use different UIntPtr from flags_ to prevent the potential data race.
        // Be careful when storing to this value, currently this is only from JS_Thread during ConcurrentMarking,
        // or from GC_Thread during GC ClearTask.
        alignas(EAS) RegionTypeFlag typeFlag_;
        alignas(EAS) GCBitset *markGCBitset_ {nullptr};
        // OldToNewRSet only for general OldSpace, NewToEdenRSet only for YoungSpace. Their pointers can union
        union {
            alignas(EAS) RememberedSet *oldToNewSet_ {nullptr};
            alignas(EAS) RememberedSet *newToEdenSet_;
        };
        alignas(EAS) RememberedSet *localToShareSet_ {nullptr};
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
    RememberedSet *GetOrCreateNewToEdenRememberedSet();
    RememberedSet *GetOrCreateOldToNewRememberedSet();
    RememberedSet *GetOrCreateLocalToShareRememberedSet();

    inline RememberedSet *CreateNewToEdenRememberedSet();
    inline RememberedSet *CreateOldToNewRememberedSet();
    inline RememberedSet *CreateLocalToShareRememberedSet();

    PackedData packedData_;
    NativeAreaAllocator *nativeAreaAllocator_;

    uintptr_t allocateBase_;
    uintptr_t end_;
    uintptr_t highWaterMark_;
    std::atomic_size_t aliveObject_ {0};
    size_t gcAliveSize_ {0};
    Region *next_ {nullptr};
    Region *prev_ {nullptr};

    RememberedSet *crossRegionSet_ {nullptr};
    RememberedSet *sweepingOldToNewRSet_ {nullptr};
    RememberedSet *sweepingLocalToShareRSet_ {nullptr};
    Span<FreeObjectSet<FreeObject> *> freeObjectSets_;
    Mutex *lock_ {nullptr};
    uint64_t wasted_;
    // snapshotdata_ is used to encode the region for snapshot. Its upper 32 bits are used to store the size of
    // the huge object, and the lower 32 bits are used to store the region index
    uint64_t snapshotData_;
    uintptr_t localHeap_ {0};

    friend class Snapshot;
    friend class SnapshotProcessor;
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_MEM_REGION_H
