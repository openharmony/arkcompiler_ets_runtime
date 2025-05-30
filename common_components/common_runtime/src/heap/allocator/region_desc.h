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
#ifndef ARK_COMMON_REGION_INFO_H
#define ARK_COMMON_REGION_INFO_H

#include <cstdint>
#include <cstddef>
#include <list>
#include <map>
#include <set>
#include <thread>
#include <vector>
#include "macros.h"
#ifdef _WIN64
#include <memoryapi.h>
#include <errhandlingapi.h>
#include <handleapi.h>
#else
#include <sys/mman.h>
#endif
#include "common_components/common_runtime/src/base/globals.h"
#include "common_components/common_runtime/src/base/mem_utils.h"
#include "common_components/common_runtime/src/base/rw_lock.h"
#include "common_components/common_runtime/src/heap/collector/copy_data_manager.h"
#include "common_components/common_runtime/src/heap/collector/gc_infos.h"
#include "common_components/common_runtime/src/heap/collector/region_bitmap.h"
#include "common_components/log/log.h"
#include "securec.h"
#ifdef ARKCOMMON_ASAN_SUPPORT
#include "common_components/common_runtime/src/sanitizer/sanitizer_interface.h"
#endif

namespace panda {
template<typename T>
class BitFields {
public:
    // pos: the position where the bit locates. It starts from 0.
    // bitLen: the length that is to be read.
    T AtomicGetValue(size_t pos, size_t bitLen) const
    {
        T value = __atomic_load_n(&fieldVal_, __ATOMIC_ACQUIRE);
        T bitMask = ((1 << bitLen) - 1) << pos;
        return (value & bitMask) >> pos;
    }

    void AtomicSetValue(size_t pos, size_t bitLen, T newValue)
    {
        do {
            T oldValue = fieldVal_;
            T bitMask = ((1 << bitLen) - 1) << pos;
            T unchangedBitMask = ~bitMask;
            T newFieldValue = ((newValue << pos) & bitMask) | (oldValue & unchangedBitMask);
            if (__atomic_compare_exchange_n(&fieldVal_, &oldValue, newFieldValue, false, __ATOMIC_ACQ_REL,
                                            __ATOMIC_ACQUIRE)) {
                return;
            }
        } while (true);
    }

private:
    T fieldVal_;
};
// this class is the metadata of region, it contains all the information needed to manage its corresponding memory.
// Region memory is composed of several Units, described by UnitInfo.
// sizeof(RegionDesc) must be equal to sizeof(UnitInfo). We rely on this fact to calculate region-related address.

// region info is stored in the metadata of its primary unit (i.e. the first unit).
class RegionDesc {
public:
    enum RouteState : uint8_t {
        INITIAL = 0,
        COPYABLE,
        ROUTING,
        ROUTED,
        COMPACTED,
        COPYED,
    };

    static const size_t UNIT_SIZE; // same as system page size

    // regarding a object as a large object when the size is greater than 8 units.
    static const size_t LARGE_OBJECT_DEFAULT_THRESHOLD;

    // release a large object when the size is greater than 4096KB.
    static constexpr size_t LARGE_OBJECT_RELEASE_THRESHOLD = 4096 * KB;

    RegionDesc()
    {
        metadata.allocPtr = reinterpret_cast<uintptr_t>(nullptr);
        metadata.traceLine = std::numeric_limits<uintptr_t>::max();
        metadata.forwardLine = std::numeric_limits<uintptr_t>::max();
        metadata.fixLine = std::numeric_limits<uintptr_t>::max();
        metadata.freeSlot = nullptr;
        metadata.regionStart = reinterpret_cast<uintptr_t>(nullptr);
        metadata.regionEnd = reinterpret_cast<uintptr_t>(nullptr);
        metadata.toSpaceRegion = false;
    }
    static inline RegionDesc* NullRegion()
    {
        static RegionDesc nullRegion;
        return &nullRegion;
    }

    RegionLiveDesc* GetLiveInfo()
    {
        RegionLiveDesc* liveInfo = __atomic_load_n(&metadata.liveInfo, std::memory_order_acquire);
        if (reinterpret_cast<HeapAddress>(liveInfo) == RegionLiveDesc::TEMPORARY_PTR) {
            return nullptr;
        }
        return liveInfo;
    }
    NO_INLINE RegionLiveDesc* AllocLiveInfo(RegionLiveDesc* liveInfo)
    {
        RegionLiveDesc* newValue = reinterpret_cast<RegionLiveDesc*>(RegionLiveDesc::TEMPORARY_PTR);
        if (__atomic_compare_exchange_n(&metadata.liveInfo, &liveInfo, newValue, false, std::memory_order_seq_cst,
                                        std::memory_order_relaxed)) {
            RegionLiveDesc* allocatedLiveInfo = HeapBitmapManager::GetHeapBitmapManager().AllocateRegionLiveDesc();
            allocatedLiveInfo->relatedRegion = this;
            __atomic_store_n(&metadata.liveInfo, allocatedLiveInfo, std::memory_order_release);
            DLOG(REGION, "region %p@%#zx alloc liveinfo %p", this, GetRegionStart(), metadata.liveInfo);
            return allocatedLiveInfo;
        }
        return nullptr;
    }
    ALWAYS_INLINE RegionLiveDesc* GetOrAllocLiveInfo()
    {
        do {
            RegionLiveDesc* liveInfo = __atomic_load_n(&metadata.liveInfo, std::memory_order_acquire);
            if (UNLIKELY_CC(reinterpret_cast<uintptr_t>(liveInfo) == RegionLiveDesc::TEMPORARY_PTR)) {
                continue;
            }
            if (LIKELY_CC(liveInfo != nullptr)) {
                return liveInfo;
            }
            liveInfo = AllocLiveInfo(liveInfo);
            if(liveInfo != nullptr) {
                return liveInfo;
            }
        } while (true);
        return nullptr;
    }

    RegionBitmap* GetMarkBitmap()
    {
        RegionLiveDesc* liveInfo = GetLiveInfo();
        if (liveInfo == nullptr) {
            return nullptr;
        }
        RegionBitmap* bitmap = __atomic_load_n(&liveInfo->markBitmap, std::memory_order_acquire);
        if (reinterpret_cast<HeapAddress>(bitmap) == RegionLiveDesc::TEMPORARY_PTR) {
            return nullptr;
        }
        return bitmap;
    }

    NO_INLINE RegionBitmap* AllocMarkBitmap(RegionLiveDesc* liveInfo, RegionBitmap* bitmap)
    {
        RegionBitmap* newValue = reinterpret_cast<RegionBitmap*>(RegionLiveDesc::TEMPORARY_PTR);
        if (__atomic_compare_exchange_n(&liveInfo->markBitmap, &bitmap, newValue, false, std::memory_order_seq_cst,
                                        std::memory_order_relaxed)) {
            RegionBitmap* allocated =
                HeapBitmapManager::GetHeapBitmapManager().AllocateRegionBitmap(GetRegionSize());
            __atomic_store_n(&liveInfo->markBitmap, allocated, std::memory_order_release);
            DLOG(REGION, "region %p@%#zx liveinfo %p alloc markbitmap %p",
                 this, GetRegionStart(), metadata.liveInfo, metadata.liveInfo->markBitmap);
            return allocated;
        }
        return nullptr;
    }
    ALWAYS_INLINE RegionBitmap* GetOrAllocMarkBitmap()
    {
        RegionLiveDesc* liveInfo = GetOrAllocLiveInfo();
        do {
            RegionBitmap* bitmap = __atomic_load_n(&liveInfo->markBitmap, std::memory_order_acquire);
            if (UNLIKELY_CC(reinterpret_cast<uintptr_t>(bitmap) == RegionLiveDesc::TEMPORARY_PTR)) {
                continue;
            }
            if (LIKELY_CC(bitmap != nullptr)) {
                return bitmap;
            }
            bitmap = AllocMarkBitmap(liveInfo, bitmap);
            if (bitmap != nullptr) {
                return bitmap;
            }
        } while (true);

        return nullptr;
    }

    RegionBitmap* GetResurrectBitmap()
    {
        RegionLiveDesc* liveInfo = GetLiveInfo();
        if (liveInfo == nullptr) {
            return nullptr;
        }
        RegionBitmap* bitmap = __atomic_load_n(&liveInfo->resurrectBitmap, std::memory_order_acquire);
        if (reinterpret_cast<HeapAddress>(bitmap) == RegionLiveDesc::TEMPORARY_PTR) {
            return nullptr;
        }
        return bitmap;
    }

    RegionBitmap* GetOrAllocResurrectBitmap()
    {
        RegionLiveDesc* liveInfo = GetOrAllocLiveInfo();
        do {
            RegionBitmap* bitmap = __atomic_load_n(&liveInfo->resurrectBitmap, std::memory_order_acquire);
            if (UNLIKELY_CC(reinterpret_cast<uintptr_t>(bitmap) == RegionLiveDesc::TEMPORARY_PTR)) {
                continue;
            }
            if (LIKELY_CC(bitmap != nullptr)) {
                return bitmap;
            }
            RegionBitmap* newValue = reinterpret_cast<RegionBitmap*>(RegionLiveDesc::TEMPORARY_PTR);
            if (__atomic_compare_exchange_n(&liveInfo->resurrectBitmap, &bitmap, newValue, false,
                                            std::memory_order_seq_cst, std::memory_order_relaxed)) {
                RegionBitmap* allocated =
                    HeapBitmapManager::GetHeapBitmapManager().AllocateRegionBitmap(GetRegionSize());
                __atomic_store_n(&liveInfo->resurrectBitmap, allocated, std::memory_order_release);
                DLOG(REGION, "region %p@%#zx liveinfo %p alloc resurrectbitmap %p",
                     this, GetRegionStart(), metadata.liveInfo, metadata.liveInfo->resurrectBitmap);
                return allocated;
            }
        } while (true);

        return nullptr;
    }

    RegionBitmap* GetEnqueueBitmap()
    {
        RegionLiveDesc* liveInfo = GetLiveInfo();
        if (liveInfo == nullptr) {
            return nullptr;
        }
        RegionBitmap* bitmap = __atomic_load_n(&liveInfo->enqueueBitmap, std::memory_order_acquire);
        if (reinterpret_cast<HeapAddress>(bitmap) == RegionLiveDesc::TEMPORARY_PTR) {
            return nullptr;
        }
        return bitmap;
    }

    RegionBitmap* GetOrAllocEnqueueBitmap()
    {
        RegionLiveDesc* liveInfo = GetOrAllocLiveInfo();
        do {
            RegionBitmap* bitmap = __atomic_load_n(&liveInfo->enqueueBitmap, std::memory_order_acquire);
            if (UNLIKELY_CC(reinterpret_cast<uintptr_t>(bitmap) == RegionLiveDesc::TEMPORARY_PTR)) {
                continue;
            }
            if (LIKELY_CC(bitmap != nullptr)) {
                return bitmap;
            }
            RegionBitmap* newValue = reinterpret_cast<RegionBitmap*>(RegionLiveDesc::TEMPORARY_PTR);
            if (__atomic_compare_exchange_n(&liveInfo->enqueueBitmap, &bitmap, newValue, false,
                                            std::memory_order_seq_cst, std::memory_order_relaxed)) {
                RegionBitmap* allocated =
                    HeapBitmapManager::GetHeapBitmapManager().AllocateRegionBitmap(GetRegionSize());
                __atomic_store_n(&liveInfo->enqueueBitmap, allocated, std::memory_order_release);
                DLOG(REGION, "region %p@%#zx liveinfo %p alloc enqueuebitmap %p",
                     this, GetRegionStart(), metadata.liveInfo, metadata.liveInfo->enqueueBitmap);
                return allocated;
            }
        } while (true);

        return nullptr;
    }

    void ResetMarkBit()
    {
        SetMarkedRegionFlag(0);
        SetEnqueuedRegionFlag(0);
        SetResurrectedRegionFlag(0);
    }

    NO_INLINE bool MarkObjectForLargeRegion(const BaseObject* obj)
    {
        if (metadata.regionBits.AtomicGetValue(RegionBitOffset::BIT_OFFSET_MARKED_REGION, 1) != 1) {
            SetMarkedRegionFlag(1);
            return false;
        }
        return true;
    }
    ALWAYS_INLINE bool MarkObject(const BaseObject* obj)
    {
        if (IsLargeRegion()) {
            return MarkObjectForLargeRegion(obj);
        }
        // top1 issue
        size_t offset = GetAddressOffset(reinterpret_cast<HeapAddress>(obj));
        bool marked = GetOrAllocMarkBitmap()->MarkBits(offset);
        DCHECK_CC(IsMarkedObject(obj));
        return marked;
    }

    bool ResurrentObject(const BaseObject* obj)
    {
        if (IsLargeRegion()) {
            if (metadata.regionBits.AtomicGetValue(RegionBitOffset::BIT_OFFSET_RESURRECTED_REGION, 1) != 1) {
                SetResurrectedRegionFlag(1);
                return false;
            }
            return true;
        }
        size_t offset = GetAddressOffset(reinterpret_cast<HeapAddress>(obj));
        bool marked = GetOrAllocResurrectBitmap()->MarkBits(offset);
        CHECK_CC(IsResurrectedObject(obj));
        return marked;
    }

    bool EnqueueObject(const BaseObject* obj)
    {
        if (IsLargeRegion()) {
            if (metadata.regionBits.AtomicGetValue(RegionBitOffset::BIT_OFFSET_ENQUEUED_REGION, 1) != 1) {
                SetEnqueuedRegionFlag(1);
                return false;
            }
            return true;
        }
        size_t offset = GetAddressOffset(reinterpret_cast<HeapAddress>(obj));
        bool marked = GetOrAllocEnqueueBitmap()->MarkBits(offset);
        CHECK_CC(IsEnqueuedObject(obj));
        return marked;
    }

    bool IsResurrectedObject(const BaseObject* obj)
    {
        if (IsLargeRegion()) {
            return (metadata.regionBits.AtomicGetValue(RegionBitOffset::BIT_OFFSET_RESURRECTED_REGION, 1) == 1);
        }
        RegionBitmap* resurrectBitmap = GetResurrectBitmap();
        if (resurrectBitmap == nullptr) {
            return false;
        }
        size_t offset = GetAddressOffset(reinterpret_cast<HeapAddress>(obj));
        return resurrectBitmap->IsMarked(offset);
    }

    bool IsMarkedObject(const BaseObject* obj)
    {
        if (IsLargeRegion()) {
            return (metadata.regionBits.AtomicGetValue(RegionBitOffset::BIT_OFFSET_MARKED_REGION, 1) == 1);
        }
        RegionBitmap* markBitmap = GetMarkBitmap();
        if (markBitmap == nullptr) {
            return false;
        }
        size_t offset = GetAddressOffset(reinterpret_cast<HeapAddress>(obj));
        return markBitmap->IsMarked(offset);
    }

    bool IsEnqueuedObject(const BaseObject* obj)
    {
        if (IsLargeRegion()) {
            return (metadata.regionBits.AtomicGetValue(RegionBitOffset::BIT_OFFSET_ENQUEUED_REGION, 1) == 1);
        }
        RegionBitmap* enqueBitmap = GetEnqueueBitmap();
        if (enqueBitmap == nullptr) {
            return false;
        }
        size_t offset = GetAddressOffset(reinterpret_cast<HeapAddress>(obj));
        return enqueBitmap->IsMarked(offset);
    }

    void CheckAndMarkObject(const BaseObject* obj)
    {
        if (!IsMarkedObject(obj)) {
            MarkObject(obj);
        }
    }

    ALWAYS_INLINE_CC size_t GetAddressOffset(HeapAddress address)
    {
        DCHECK_CC(GetRegionStart() <= address);
        return (address - metadata.regionStart);
    }

    enum class UnitRole : uint8_t {
        // for the head unit
        FREE_UNITS = 0,
        SMALL_SIZED_UNITS,
        LARGE_SIZED_UNITS,

        SUBORDINATE_UNIT,
    };

    // region is and must be one of following types during its whole lifecycle.
    // one-to-one mapping to region-lists.

    enum class RegionType : uint8_t {
        FREE_REGION,

        THREAD_LOCAL_REGION,
        RECENT_FULL_REGION,
        FROM_REGION,
        LONE_FROM_REGION,
        EXEMPTED_FROM_REGION,
        TO_REGION,

        // pinned object will not be forwarded by concurrent copying gc.
        FULL_PINNED_REGION,
        RECENT_PINNED_REGION,
        FIXED_PINNED_REGION,
        FULL_FIXED_PINNED_REGION,

        // region for raw-pointer objects which are exposed to runtime thus can not be moved by any gc.
        // raw-pointer region becomes pinned region when none of its member objects are used as raw pointer.
        RAW_POINTER_REGION,

        // allocation context is able and responsible to determine whether it is safe to be collected.
        TL_RAW_POINTER_REGION,

        RECENT_LARGE_REGION,
        OLD_LARGE_REGION,

        GARBAGE_REGION,
    };

    static void Initialize(size_t nUnit, uintptr_t regionInfoAddr, uintptr_t heapAddress)
    {
        UnitInfo::totalUnitCount = nUnit;
        UnitInfo::unitInfoStart = regionInfoAddr;
        UnitInfo::heapStartAddress = heapAddress;
    }

    static RegionDesc* GetRegionDesc(uint32_t idx)
    {
        UnitInfo* unit = RegionDesc::UnitInfo::GetUnitInfo(idx);
        ASSERT(((uintptr_t)unit % 8 == 0));
        if (static_cast<UnitRole>(unit->GetMetadata().unitRole) == UnitRole::SUBORDINATE_UNIT) {
            return unit->GetMetadata().ownerRegion;
        } else {
            return reinterpret_cast<RegionDesc*>(unit);
        }
    }

    static RegionDesc* GetRegionDescAt(uintptr_t allocAddr)
    {
        UnitInfo* unit = reinterpret_cast<UnitInfo*>(UnitInfo::heapStartAddress -
                                                     (((allocAddr - UnitInfo::heapStartAddress) / UNIT_SIZE) + 1) *
                                                         sizeof(RegionDesc));
        ASSERT(((uintptr_t)unit % 8 == 0));
        if (static_cast<UnitRole>(unit->GetMetadata().unitRole) == UnitRole::SUBORDINATE_UNIT) {
            return unit->GetMetadata().ownerRegion;
        } else {
            return reinterpret_cast<RegionDesc*>(unit);
        }
    }

    static void InitFreeRegion(size_t unitIdx, size_t nUnit)
    {
        RegionDesc* region = reinterpret_cast<RegionDesc*>(RegionDesc::UnitInfo::GetUnitInfo(unitIdx));
        region->InitRegionDesc(nUnit, UnitRole::FREE_UNITS);
    }

    static RegionDesc* InitRegion(size_t unitIdx, size_t nUnit, RegionDesc::UnitRole uclass)
    {
        RegionDesc* region = reinterpret_cast<RegionDesc*>(RegionDesc::UnitInfo::GetUnitInfo(unitIdx));
        region->InitRegion(nUnit, uclass);
        return region;
    }

    static RegionDesc* InitRegionAt(uintptr_t addr, size_t nUnit, RegionDesc::UnitRole uclass)
    {
        size_t idx = RegionDesc::UnitInfo::GetUnitIdxAt(addr);
        return InitRegion(idx, nUnit, uclass);
    }

    static HeapAddress GetUnitAddress(size_t unitIdx) { return UnitInfo::GetUnitAddress(unitIdx); }

    static void ClearUnits(size_t idx, size_t cnt)
    {
        uintptr_t unitAddress = RegionDesc::GetUnitAddress(idx);
        size_t size = cnt * RegionDesc::UNIT_SIZE;
        DLOG(REGION, "clear dirty units[%zu+%zu, %zu) @[%#zx+%zu, %#zx)", idx, cnt, idx + cnt, unitAddress, size,
             RegionDesc::GetUnitAddress(idx + cnt));
        panda::MemorySet(unitAddress, size, 0, size);
    }

    static void ReleaseUnits(size_t idx, size_t cnt)
    {
        void* unitAddress = reinterpret_cast<void*>(RegionDesc::GetUnitAddress(idx));
        size_t size = cnt * RegionDesc::UNIT_SIZE;
        DLOG(REGION, "release physical memory for units [%zu+%zu, %zu) @[%p+%zu, 0x%zx)", idx, cnt, idx + cnt,
             unitAddress, size, RegionDesc::GetUnitAddress(idx + cnt));
#if defined(_WIN64)
        LOGE_IF(UNLIKELY_CC(!VirtualFree(unitAddress, size, MEM_DECOMMIT))) <<
            "VirtualFree failed in ReturnPage, errno: " << GetLastError();

#elif defined(__APPLE__)
        MemorySet(reinterpret_cast<uintptr_t>(unitAddress), size, 0, size);
        (void)madvise(unitAddress, size, MADV_DONTNEED);
#else
        (void)madvise(unitAddress, size, MADV_DONTNEED);
#endif
#ifdef ARKCOMMON_ASAN_SUPPORT
        Sanitizer::OnHeapMadvise(unitAddress, size);
#endif
    }

    BaseObject* GetFirstObject() const { return reinterpret_cast<BaseObject*>(GetRegionStart()); }

    bool IsEmpty() const
    {
        ASSERT_LOGF(IsSmallRegion(), "wrong region type");
        return GetRegionAllocPtr() == GetRegionStart();
    }

    size_t GetRegionSize() const
    {
        DCHECK_CC(metadata.regionEnd > GetRegionStart());
        return metadata.regionEnd - GetRegionStart();
    }

    size_t GetUnitCount() const { return GetRegionSize() / UNIT_SIZE; }

    size_t GetAvailableSize() const
    {
        ASSERT_LOGF(IsSmallRegion(), "wrong region type");
        return GetRegionEnd() - GetRegionAllocPtr();
    }

    size_t GetRegionAllocatedSize() const { return GetRegionAllocPtr() - GetRegionStart(); }

#if defined(GCINFO_DEBUG) && GCINFO_DEBUG
    void DumpRegionDesc(LogType type) const;
    const char* GetTypeName() const;
#endif

    void VisitAllObjects(const std::function<void(BaseObject*)>&& func);
    void VisitAllObjectsWithFixedSize(size_t cellCount, const std::function<void(BaseObject*)>&& func);
    void VisitAllObjectsBeforeFix(const std::function<void(BaseObject*)>&& func);
    bool VisitLiveObjectsUntilFalse(const std::function<bool(BaseObject*)>&& func);

    // reset so that this region can be reused for allocation
    void InitFreeUnits()
    {
        size_t nUnit = GetUnitCount();
        UnitInfo* unit = reinterpret_cast<UnitInfo*>(this) - (nUnit - 1);
        for (size_t i = 0; i < nUnit; ++i) {
            unit[i].ToFreeRegion();
        }
    }

    // the interface can only be used to clear live info after gc.
    void CheckAndClearLiveInfo(RegionLiveDesc* liveInfo)
    {
        // Garbage region may be reused by other thread. For the sake of safety, we don't clean it here.
        // We will clean it before the region is accessable.
        if (IsGarbageRegion()) {
            return;
        }
        // Check the value whether is expected, in order to avoid resetting a reused region.
        if (metadata.liveInfo == liveInfo) {
            metadata.liveInfo = nullptr;
            metadata.liveByteCount = 0;
        }
    }

    void ClearLiveInfo()
    {
        if (metadata.liveInfo != nullptr) {
            DCHECK_CC(metadata.liveInfo->relatedRegion == this);
            metadata.liveInfo = nullptr;
            DLOG(REGION, "region %p@%#zx+%zu clear liveinfo %p type %u", this, this->GetRegionStart(),
                this->GetRegionSize(), metadata.liveInfo, this->GetRegionType());
        }
        metadata.liveByteCount = 0;
    }

    void SetToSpaceRegion(bool toSpaceRegion)
    {
        metadata.toSpaceRegion = toSpaceRegion;
    }
    bool IsToSpaceRegion()
    {
        return metadata.toSpaceRegion;
    }

    // These interfaces are used to make sure the writing operations of value in C++ Bit Field will be atomic.
    void SetUnitRole(UnitRole role)
    {
        metadata.unitBits.AtomicSetValue(0, BITS_4, static_cast<uint8_t>(role));
    }
    void SetRegionType(RegionType type)
    {
        metadata.regionBits.AtomicSetValue(RegionBitOffset::BIT_OFFSET_REGION_TYPE,
            BITS_5, static_cast<uint8_t>(type));
    }

    void SetMarkedRegionFlag(uint8_t flag)
    {
        metadata.regionBits.AtomicSetValue(RegionBitOffset::BIT_OFFSET_MARKED_REGION, 1, flag);
    }
    void SetEnqueuedRegionFlag(uint8_t flag)
    {
        metadata.regionBits.AtomicSetValue(RegionBitOffset::BIT_OFFSET_ENQUEUED_REGION, 1, flag);
    }
    void SetResurrectedRegionFlag(uint8_t flag)
    {
        metadata.regionBits.AtomicSetValue(RegionBitOffset::BIT_OFFSET_RESURRECTED_REGION, 1, flag);
    }

    void SetFixedRegionFlag(uint8_t flag)
    {
        metadata.regionBits.AtomicSetValue(RegionBitOffset::BIT_OFFSET_FIXED_REGION, 1, flag);
    }

    bool IsFixedRegionFlag()
    {
        return metadata.regionBits.AtomicGetValue(RegionBitOffset::BIT_OFFSET_FIXED_REGION, 1);
    }

    void SetRegionCellCount(uint8_t cellCount)
    {
        // 7: region cell count is 7 bits.
        metadata.regionBits.AtomicSetValue(RegionBitOffset::BIT_OFFSET_REGION_CELLCOUNT, 7, cellCount);
    }

    uint16_t GetRegionCellCount()
    {
        // 7: region cell count is 7 bits.
        return metadata.regionBits.AtomicGetValue(RegionBitOffset::BIT_OFFSET_REGION_CELLCOUNT, 7);
    }

    RegionType GetRegionType() const
    {
        return static_cast<RegionType>(metadata.regionBits.AtomicGetValue(RegionBitOffset::BIT_OFFSET_REGION_TYPE,
                                                                          BITS_5));
    }

    UnitRole GetUnitRole() const { return static_cast<UnitRole>(metadata.unitRole); }

    size_t GetUnitIdx() const { return RegionDesc::UnitInfo::GetUnitIdx(reinterpret_cast<const UnitInfo*>(this)); }

    HeapAddress GetRegionStart() const { return RegionDesc::GetUnitAddress(GetUnitIdx()); }

    HeapAddress GetRegionEnd() const { return metadata.regionEnd; }

    void SetRegionAllocPtr(HeapAddress addr) { metadata.allocPtr = addr; }

    HeapAddress GetRegionAllocPtr() const { return metadata.allocPtr; }

    HeapAddress GetTraceLine() const { return metadata.traceLine; }
    HeapAddress GetCopyLine() const { return metadata.forwardLine; }
    HeapAddress GetFixLine() const { return metadata.fixLine; }

    void SetTraceLine()
    {
        if (metadata.traceLine == std::numeric_limits<uintptr_t>::max()) {
            uintptr_t line = GetRegionAllocPtr();
            metadata.traceLine = line;
            DLOG(REGION, "set region %p@%#zx+%zu trace-line %#zx type %u", this, this->GetRegionStart(),
                this->GetRegionSize(), this->GetTraceLine(), this->GetRegionType());
        }
    }

    void SetCopyLine()
    {
        if (metadata.forwardLine == std::numeric_limits<uintptr_t>::max()) {
            uintptr_t line = GetRegionAllocPtr();
            metadata.forwardLine = line;
            DLOG(REGION, "set region %p@%#zx+%zu copy-line %#zx type %u", this, this->GetRegionStart(),
                this->GetRegionSize(), this->GetCopyLine(), this->GetRegionType());
        }
    }

    void SetFixLine()
    {
        if (metadata.fixLine == std::numeric_limits<uintptr_t>::max()) {
            uintptr_t line = GetRegionAllocPtr();
            metadata.fixLine = line;
            DLOG(REGION, "set region %p@%#zx+%zu fix-line %#zx type %u", this, this->GetRegionStart(),
                this->GetRegionSize(), this->GetFixLine(), this->GetRegionType());
        }
    }

    void ClearTraceCopyFixLine()
    {
        metadata.traceLine = std::numeric_limits<uintptr_t>::max();
        metadata.forwardLine = std::numeric_limits<uintptr_t>::max();
        metadata.fixLine = std::numeric_limits<uintptr_t>::max();
    }

    void ClearFreeSlot()
    {
        metadata.freeSlot = nullptr;
    }

    bool IsNewObjectSinceTrace(const BaseObject* obj)
    {
        return GetTraceLine() <= reinterpret_cast<uintptr_t>(obj);
    }

    bool IsNewObjectSinceForward(const BaseObject* obj)
    {
        return GetCopyLine() <= reinterpret_cast<uintptr_t>(obj);
    }

    bool IsNewObjectSinceFix(const BaseObject* obj)
    {
        return GetFixLine() <= reinterpret_cast<uintptr_t>(obj);
    }

    bool IsNewRegionSinceForward() const
    {
        return GetCopyLine() == GetRegionStart();
    }

    bool IsNewRegionSinceFix() const
    {
        return GetFixLine() == GetRegionStart();
    }

    int32_t IncRawPointerObjectCount()
    {
        int32_t oldCount = __atomic_fetch_add(&metadata.rawPointerObjectCount, 1, __ATOMIC_SEQ_CST);
        LOGF_CHECK(oldCount >= 0) << "region " << this << " has wrong raw pointer count " << oldCount;
        LOGF_CHECK(oldCount < MAX_RAW_POINTER_COUNT) << "inc raw-pointer-count overflow";
        return oldCount;
    }

    int32_t DecRawPointerObjectCount()
    {
        int32_t oldCount = __atomic_fetch_sub(&metadata.rawPointerObjectCount, 1, __ATOMIC_SEQ_CST);
        LOGF_CHECK(oldCount > 0) << "dec raw-pointer-count underflow, please check whether releaseRawData is overused.";
        return oldCount;
    }

    int32_t GetRawPointerObjectCount() const
    {
        return __atomic_load_n(&metadata.rawPointerObjectCount, __ATOMIC_SEQ_CST);
    }

    bool CompareAndSwapRawPointerObjectCount(int32_t expectVal, int32_t newVal)
    {
        return __atomic_compare_exchange_n(&metadata.rawPointerObjectCount, &expectVal, newVal, false, __ATOMIC_SEQ_CST,
                                           __ATOMIC_ACQUIRE);
    }

    uintptr_t Alloc(size_t size)
    {
        size_t limit = GetRegionEnd();
        if (metadata.allocPtr + size <= limit) {
            uintptr_t addr = metadata.allocPtr;
            metadata.allocPtr += size;
            return addr;
        } else {
            return 0;
        }
    }

    // for regions shared by multithreads
    uintptr_t AtomicAlloc(size_t size)
    {
        uintptr_t addr = __atomic_fetch_add(&metadata.allocPtr, size, __ATOMIC_ACQ_REL);
        // should not check allocPtr, because it might be shared
        if ((addr < GetRegionEnd()) && (size <= GetRegionEnd() - addr)) {
            return addr;
        }
        if (addr <= GetRegionEnd()) {
            __atomic_store_n(&metadata.allocPtr, addr, __ATOMIC_SEQ_CST);
        }
        return 0;
    }

    // copyable during concurrent copying gc.
    bool IsSmallRegion() const { return static_cast<UnitRole>(metadata.unitRole) == UnitRole::SMALL_SIZED_UNITS; }

    ALWAYS_INLINE bool IsLargeRegion() const { return static_cast<UnitRole>(metadata.unitRole) == UnitRole::LARGE_SIZED_UNITS; }

    bool IsFixedRegion() const
    {
        return (GetRegionType()  == RegionType::FIXED_PINNED_REGION) ||
            (GetRegionType()  == RegionType::FULL_FIXED_PINNED_REGION);
    }
    
    bool IsThreadLocalRegion() const
    {
        return GetRegionType()  == RegionType::THREAD_LOCAL_REGION;
    }

    bool IsPinnedRegion() const
    {
        return (GetRegionType()  == RegionType::FULL_PINNED_REGION) ||
               (GetRegionType()  == RegionType::RECENT_PINNED_REGION);
    }

    RegionDesc* GetPrevRegion() const
    {
        if (UNLIKELY_CC(metadata.prevRegionIdx == NULLPTR_IDX)) {
            return nullptr;
        }
        return reinterpret_cast<RegionDesc*>(UnitInfo::GetUnitInfo(metadata.prevRegionIdx));
    }

    bool CollectPinnedGarbage(BaseObject* obj, size_t cellCount)
    {
        std::lock_guard<std::mutex> lg(metadata.regionMutex);
        if (IsFreePinnedObject(obj)) {
            return false;
        }
        size_t size = (cellCount + 1) * sizeof(uint64_t);
        ObjectSlot* head = reinterpret_cast<ObjectSlot*>(obj);
        head->SetNext(metadata.freeSlot, size);
        metadata.freeSlot = head;
        return true;
    }

    HeapAddress GetFreeSlot()
    {
        if (metadata.freeSlot == nullptr) {
            return 0;
        }
        ObjectSlot* res = metadata.freeSlot;
        metadata.freeSlot = reinterpret_cast<ObjectSlot*>(res->next_);
        res->next_ = 0;
        res->isFree_ = 0;
        return reinterpret_cast<HeapAddress>(res);
    }

    HeapAddress AllocPinnedFromFreeList()
    {
        std::lock_guard<std::mutex> lg(metadata.regionMutex);
        HeapAddress addr = GetFreeSlot();
        if (addr == 0) {
            RegionDesc* region = GetNextRegion();
            do {
                if (region == nullptr) {
                    break;
                }
                addr = region->GetFreeSlot();
                region = region->GetNextRegion();
            } while (addr == 0);
        }
        return addr;
    }

    bool IsFreePinnedObject(BaseObject* object)
    {
        ObjectSlot* slot = reinterpret_cast<ObjectSlot*>(object);
        return slot->isFree_;
    }

    void SetPrevRegion(const RegionDesc* r)
    {
        if (UNLIKELY_CC(r == nullptr)) {
            metadata.prevRegionIdx = NULLPTR_IDX;
            return;
        }
        size_t prevIdx = r->GetUnitIdx();
        ASSERT_LOGF(prevIdx < NULLPTR_IDX, "exceeds the maxinum limit for region info");
        metadata.prevRegionIdx = static_cast<uint32_t>(prevIdx);
    }

    RegionDesc* GetNextRegion() const
    {
        if (UNLIKELY_CC(metadata.nextRegionIdx == NULLPTR_IDX)) {
            return nullptr;
        }
        DCHECK_CC(metadata.nextRegionIdx < UnitInfo::totalUnitCount);
        return reinterpret_cast<RegionDesc*>(UnitInfo::GetUnitInfo(metadata.nextRegionIdx));
    }

    void SetNextRegion(const RegionDesc* r)
    {
        if (UNLIKELY_CC(r == nullptr)) {
            metadata.nextRegionIdx = NULLPTR_IDX;
            return;
        }
        size_t nextIdx = r->GetUnitIdx();
        ASSERT_LOGF(nextIdx < NULLPTR_IDX, "exceeds the maxinum limit for region info");
        metadata.nextRegionIdx = static_cast<uint32_t>(nextIdx);
    }

    bool IsFromRegion() const { return GetRegionType()  == RegionType::FROM_REGION; }

    bool IsUnmovableFromRegion() const
    {
        return GetRegionType()  == RegionType::EXEMPTED_FROM_REGION ||
            GetRegionType()  == RegionType::RAW_POINTER_REGION;
    }

    bool IsToRegion() const { return GetRegionType()  == RegionType::TO_REGION; }

    bool IsGarbageRegion() const { return GetRegionType()  == RegionType::GARBAGE_REGION; }
    bool IsFreeRegion() const { return static_cast<UnitRole>(metadata.unitRole) == UnitRole::FREE_UNITS; }

    bool IsValidRegion() const
    {
        return static_cast<UnitRole>(metadata.unitRole) == UnitRole::SMALL_SIZED_UNITS ||
            static_cast<UnitRole>(metadata.unitRole) == UnitRole::LARGE_SIZED_UNITS;
    }

    uint32_t GetLiveByteCount() const { return metadata.liveByteCount; }

    void ResetLiveByteCount() { metadata.liveByteCount = 0; }

    void AddLiveByteCount(uint32_t count)
    {
        (void)__atomic_fetch_add(&metadata.liveByteCount, count, __ATOMIC_ACQ_REL);
    }

    void RemoveFromList()
    {
        RegionDesc* prev = GetPrevRegion();
        RegionDesc* next = GetNextRegion();
        if (prev != nullptr) {
            prev->SetNextRegion(next);
        }
        if (next != nullptr) {
            next->SetPrevRegion(prev);
        }
        this->SetNextRegion(nullptr);
        this->SetPrevRegion(nullptr);
    }

private:
    static constexpr int32_t MAX_RAW_POINTER_COUNT = std::numeric_limits<int32_t>::max();
    static constexpr int32_t BITS_4 = 4;
    static constexpr int32_t BITS_5 = 5;

    enum RegionBitOffset : uint8_t {
        BIT_OFFSET_REGION_TYPE = 0,
        // use mark-bitmap pointer instead
        BIT_OFFSET_MARKED_REGION = BITS_5,
        BIT_OFFSET_ENQUEUED_REGION = 6,
        BIT_OFFSET_RESURRECTED_REGION = 7,
        BIT_OFFSET_FIXED_REGION = 8,
        BIT_OFFSET_REGION_CELLCOUNT = 9
    };

    struct ObjectSlot {
        HeapAddress next_ : 48;
        HeapAddress isFree_ : 1;
        HeapAddress padding : 15;

        void SetNext(ObjectSlot* slot, size_t size)
        {
            next_ = reinterpret_cast<HeapAddress>(slot);
            isFree_ = 1;
            size_t extraSize = size - sizeof(ObjectSlot);
            if (extraSize > 0) {
                uintptr_t start = reinterpret_cast<uintptr_t>(this) + sizeof(ObjectSlot);
                LOGE_IF((memset_s(reinterpret_cast<void*>(start), extraSize, 0, extraSize) != EOK)) << "memset_s fail";
            }
        }
    };

    struct UnitMetadata {
        struct { // basic data for RegionDesc
            // for fast allocation, always at the start.
            uintptr_t allocPtr;
            // watermark set when gc phase transitions to pre-trace.
            uintptr_t traceLine;
            uintptr_t forwardLine;
            uintptr_t fixLine;
            ObjectSlot* freeSlot;
            uintptr_t regionStart;
            uintptr_t regionEnd;

            uint32_t nextRegionIdx;
            uint32_t prevRegionIdx; // support fast deletion for region list.

            uint32_t liveByteCount;
            int32_t rawPointerObjectCount;
        };

        union {
            RegionLiveDesc* liveInfo = nullptr;
            RegionDesc* ownerRegion; // if unit is SUBORDINATE_UNIT
        };

        // the writing operation in C++ Bit-Field feature is not atomic, the we wants to
        // change the value, we must use specific interface implenmented by BitFields.
        union {
            struct {
                uint8_t unitRole : BITS_4;
            };
            BitFields<uint8_t> unitBits;
        };

        // the writing operation in C++ Bit-Field feature is not atomic, the we wants to
        // change the value, we must use specific interface implenmented by BitFields.
        union {
            struct {
                RegionType regionType : BITS_5;

                // true if this unit belongs to a ghost region, which is an unreal region for keeping reclaimed
                // from-region. ghost region is set up to memorize a from-region before from-space is forwarded. this
                // flag is cleared when ghost-from-space is cleared. Note this flag is essentially important for
                // FindToVersion().
                uint8_t isMarked : 1;
                uint8_t isEnqueued : 1;
                uint8_t isResurrected : 1;
                uint8_t isFixed : 1;
                uint8_t cellCount : 7;
            };
            BitFields<uint16_t> regionBits;
        };

        bool toSpaceRegion;
        std::mutex regionMutex;
    };

    class UnitInfo {
    public:
        // propgated from RegionManager
        static uintptr_t heapStartAddress; // the address of the first region space to allocate objects
        static size_t totalUnitCount;
        static uintptr_t unitInfoStart; // the address of the first UnitInfo

        constexpr static uint32_t INVALID_IDX = std::numeric_limits<uint32_t>::max();
        static size_t GetUnitIdxAt(uintptr_t allocAddr)
        {
            if (heapStartAddress <= allocAddr && allocAddr < (heapStartAddress + totalUnitCount * UNIT_SIZE)) {
                return (allocAddr - heapStartAddress) / UNIT_SIZE;
            }

            LOG_COMMON(FATAL) << "Unresolved fatal";
            UNREACHABLE_CC();
        }

        static UnitInfo* GetUnitInfoAt(uintptr_t allocAddr)
        {
            return GetUnitInfo(GetUnitIdxAt(allocAddr));
        }

        // the start address for allocation
        static HeapAddress GetUnitAddress(size_t idx)
        {
            CHECK_CC(idx < totalUnitCount);
            return heapStartAddress + idx * UNIT_SIZE;
        }

        static UnitInfo* GetUnitInfo(size_t idx)
        {
            CHECK_CC(idx < totalUnitCount);
            return reinterpret_cast<UnitInfo*>(heapStartAddress - (idx + 1) * sizeof(UnitInfo));
        }

        static size_t GetUnitIdx(const UnitInfo* unit)
        {
            uintptr_t ptr = reinterpret_cast<uintptr_t>(unit);
            ASSERT(unitInfoStart <= ptr && ptr < heapStartAddress);
            return ((heapStartAddress - ptr) / sizeof(UnitInfo)) - 1;
        }

        UnitInfo() = delete;
        UnitInfo(const UnitInfo&) = delete;
        UnitInfo& operator=(const UnitInfo&) = delete;
        ~UnitInfo() = delete;

        // These interfaces are used to make sure the writing operations of value in C++ Bit Field will be atomic.
        void SetUnitRole(UnitRole role) { metadata_.unitBits.AtomicSetValue(0, BITS_4, static_cast<uint8_t>(role)); }
        void SetRegionType(RegionType type)
        {
            metadata_.regionBits.AtomicSetValue(RegionBitOffset::BIT_OFFSET_REGION_TYPE, BITS_5,
                                                static_cast<uint8_t>(type));
        }

        void SetMarkedRegionFlag(uint8_t flag)
        {
            metadata_.regionBits.AtomicSetValue(RegionBitOffset::BIT_OFFSET_MARKED_REGION, 1, flag);
        }

        void SetEnqueuedRegionFlag(uint8_t flag)
        {
            metadata_.regionBits.AtomicSetValue(RegionBitOffset::BIT_OFFSET_ENQUEUED_REGION, 1, flag);
        }

        void SetResurrectedRegionFlag(uint8_t flag)
        {
            metadata_.regionBits.AtomicSetValue(RegionBitOffset::BIT_OFFSET_RESURRECTED_REGION, 1, flag);
        }

        void InitSubordinateUnit(RegionDesc* owner)
        {
            SetUnitRole(UnitRole::SUBORDINATE_UNIT);
            metadata_.ownerRegion = owner;
        }

        void ToFreeRegion() { InitFreeRegion(GetUnitIdx(this), 1); }

        void ClearUnit() { ClearUnits(GetUnitIdx(this), 1); }

        void ReleaseUnit() { ReleaseUnits(GetUnitIdx(this), 1); }

        UnitMetadata& GetMetadata() { return metadata_; }

        UnitRole GetUnitRole() const { return static_cast<UnitRole>(metadata_.unitRole); }

    private:
        UnitMetadata metadata_;
    };

    void InitRegionDesc(size_t nUnit, UnitRole uClass)
    {
        metadata.toSpaceRegion = false;
        metadata.allocPtr = GetRegionStart();
        metadata.regionStart = GetRegionStart();
        metadata.regionEnd = metadata.allocPtr + nUnit * RegionDesc::UNIT_SIZE;
        metadata.prevRegionIdx = NULLPTR_IDX;
        metadata.nextRegionIdx = NULLPTR_IDX;
        metadata.liveByteCount = 0;
        metadata.liveInfo = nullptr;
        metadata.freeSlot = nullptr;
        SetRegionType(RegionType::FREE_REGION);
        SetUnitRole(uClass);
        ClearTraceCopyFixLine();
        SetMarkedRegionFlag(0);
        SetEnqueuedRegionFlag(0);
        SetResurrectedRegionFlag(0);
        SetFixedRegionFlag(0);
        __atomic_store_n(&metadata.rawPointerObjectCount, 0, __ATOMIC_SEQ_CST);
    }

    void InitRegion(size_t nUnit, UnitRole uClass)
    {
        InitRegionDesc(nUnit, uClass);

        // initialize region's subordinate units.
        UnitInfo* unit = reinterpret_cast<UnitInfo*>(this) - (nUnit - 1);
        for (size_t i = 0; i < nUnit - 1; i++) {
            unit[i].InitSubordinateUnit(this);
        }
    }

    static constexpr uint32_t NULLPTR_IDX = UnitInfo::INVALID_IDX;
    UnitMetadata metadata;
};
} // namespace panda
#endif // ARK_COMMON_REGION_INFO_H
