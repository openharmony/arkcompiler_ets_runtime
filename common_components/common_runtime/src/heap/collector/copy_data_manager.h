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
#ifndef ARK_COMMON_COPY_DATA_MANAGER_H
#define ARK_COMMON_COPY_DATA_MANAGER_H
#include "common_components/common_runtime/src/base/immortal_wrapper.h"
#include "common_components/common_runtime/src/heap/heap.h"
#if defined(__linux__) || defined(PANDA_TARGET_OHOS) || defined(__APPLE__)
#include <sys/mman.h>
#endif
#include "common_components/common_runtime/src/base/log_file.h"
#include "common_components/common_runtime/src/base/mem_utils.h"
#include "common_components/common_runtime/src/base/sys_call.h"
#include "common_components/common_runtime/src/heap/collector/region_bitmap.h"

#ifdef _WIN64
#include "common_components/common_runtime/src/base/atomic_spin_lock.h"
#include <errhandlingapi.h>
#include <handleapi.h>
#include <memoryapi.h>
#else
#include <sys/mman.h>
#endif

namespace panda {

class HeapBitmapManager {
    class Memory {
    public:
        struct Zone {
            enum ZoneType : size_t {
                LIVE_INFO,
                BIT_MAP,
                ZONE_TYPE_CNT,
            };
            uintptr_t zoneStartAddress = 0;
            std::atomic<uintptr_t> zonePosition;
        };
        Memory() = default;
        void InitializeMemory(uintptr_t start, size_t sz, size_t unitCount)
        {
            startAddress_ = start;
            size_ = sz;
            InitZones(unitCount);
        }
        void InitZones(size_t unitCount)
        {
            uintptr_t start = startAddress_;
            allocZone_[Zone::ZoneType::LIVE_INFO].zoneStartAddress = start;
            allocZone_[Zone::ZoneType::LIVE_INFO].zonePosition = start;
#if defined(_WIN64)
            lastCommitEndAddr[Zone::ZoneType::LIVE_INFO].store(start);
#endif
            start += unitCount * sizeof(RegionLiveDesc);
            allocZone_[Zone::ZoneType::BIT_MAP].zoneStartAddress = start;
            allocZone_[Zone::ZoneType::BIT_MAP].zonePosition = start;
#if defined(_WIN64)
            lastCommitEndAddr[Zone::ZoneType::BIT_MAP].store(start);
#endif
        }
        uintptr_t Allocate(Zone::ZoneType type, size_t sz)
        {
#if defined(_WIN64)
            allocSpinLock.Lock();
            uintptr_t startAddr = allocZone[type].zonePosition.fetch_add(sz);
            uintptr_t endAddr = startAddr + sz;
            uintptr_t lastAddr = lastCommitEndAddr[type].load(std::memory_order_relaxed);
            if (endAddr <= lastAddr) {
                allocSpinLock.Unlock();
                return startAddr;
            }
            size_t pageSize = RoundUp(sz, panda::ARK_COMMON_PAGE_SIZE);
            LOGE_IF(UNLIKELY_CC(!VirtualAlloc(reinterpret_cast<void*>(lastAddr), pageSize, MEM_COMMIT,
                PAGE_READWRITE))) << "VirtualAlloc commit failed in Allocate, errno: " << GetLastError();
            lastCommitEndAddr[type].store(lastAddr + pageSize);
            allocSpinLock.Unlock();
            return startAddr;
#else
            return allocZone_[type].zonePosition.fetch_add(sz);
#endif
        }
        void ReleaseMemory()
        {
#if defined(_WIN64)
            LOGE_IF(UNLIKELY_CC(!VirtualFree(reinterpret_cast<void*>(startAddress), size, MEM_DECOMMIT))) <<
                "VirtualFree failed in ReturnPage, errno: " << GetLastError();
#elif defined(__APPLE__)
            panda::MemorySet(startAddress, size, 0, size);
            (void)madvise(reinterpret_cast<void*>(startAddress), size, MADV_DONTNEED);
#else
            panda::MemorySet(startAddress_, size_, 0, size_);
            DLOG(REGION, "clear copy-data @[%#zx+%zu, %#zx)", startAddress_, size_, startAddress_ + size_);
            if (madvise(reinterpret_cast<void*>(startAddress_), size_, MADV_DONTNEED) == 0) {
                DLOG(REGION, "release copy-data @[%#zx+%zu, %#zx)", startAddress_, size_, startAddress_ + size_);
            }
#endif
            for (size_t i = 0; i < Zone::ZoneType::ZONE_TYPE_CNT; ++i) {
                allocZone_[i].zonePosition = allocZone_[i].zoneStartAddress;
#if defined(_WIN64)
                lastCommitEndAddr[i].store(allocZone[i].zoneStartAddress);
#endif
            }
        }

    private:
        Zone allocZone_[Zone::ZONE_TYPE_CNT];
        uintptr_t startAddress_ = 0;
        size_t size_ = 0;
#if defined(_WIN64)
        std::atomic<uintptr_t> lastCommitEndAddr[Zone::ZONE_TYPE_CNT];
        AtomicSpinLock allocSpinLock;
#endif
    };

public:
    HeapBitmapManager() = default;
    ~HeapBitmapManager()
    {
#ifdef _WIN64
        if (!VirtualFree(reinterpret_cast<void*>(heapBitmapStart), 0, MEM_RELEASE)) {
            LOG_COMMON(ERROR) << "VirtualFree error for HeapBitmapManager";
        }
#else
        if (munmap(reinterpret_cast<void*>(heapBitmapStart_), allHeapBitmapSize_) != 0) {
            LOG_COMMON(ERROR) << "munmap error for HeapBitmapManager";
        }
#endif
    }

    static HeapBitmapManager& GetHeapBitmapManager();

    void InitializeHeapBitmap();

    void ClearHeapBitmap() { heapBitmap_[0].ReleaseMemory(); }

    RegionBitmap* AllocateRegionBitmap(size_t regionSize)
    {
        uintptr_t addr =
            heapBitmap_[0].Allocate(Memory::Zone::ZoneType::BIT_MAP, RegionBitmap::GetRegionBitmapSize(regionSize));
        RegionBitmap* bitmap = reinterpret_cast<RegionBitmap*>(addr);
        CHECK_CC(bitmap != nullptr);
        new (bitmap) RegionBitmap(regionSize);
        return bitmap;
    }

    RegionLiveDesc* AllocateRegionLiveDesc()
    {
        return reinterpret_cast<RegionLiveDesc*>(
            heapBitmap_[0].Allocate(Memory::Zone::ZoneType::LIVE_INFO, sizeof(RegionLiveDesc)));
    }

private:
    size_t GetHeapBitmapSize(size_t heapSize)
    {
        const size_t REGION_UNIT_SIZE = panda::ARK_COMMON_PAGE_SIZE; // must be equal to RegionDesc::UNIT_SIZE
        heapSize = RoundUp<size_t>(heapSize, REGION_UNIT_SIZE);
        size_t unitCnt = heapSize / REGION_UNIT_SIZE;
        regionUnitCount_ = unitCnt;
        // 64: 1 bit in bitmap marks 8bytes in region, i.e., 1 byte in bitmap marks 64 bytes in region.
        constexpr uint8_t bitMarksSize = 64;
        // 3 bitmaps for each region: markBitmap,resurrectBitmap, enqueueBitmap.
        constexpr uint8_t nBitmapTypes = 3;
        return unitCnt * sizeof(RegionLiveDesc) +
               unitCnt * (sizeof(RegionBitmap) + (REGION_UNIT_SIZE / bitMarksSize)) * nBitmapTypes;
    }

    Memory heapBitmap_[1];
    size_t regionUnitCount_ = 0;
    uintptr_t heapBitmapStart_ = 0;
    size_t allHeapBitmapSize_ = 0;
};
} // namespace panda
#endif // ARK_COMMON_COPY_DATA_MANAGER_H
