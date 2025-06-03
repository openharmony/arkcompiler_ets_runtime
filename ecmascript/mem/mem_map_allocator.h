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

#ifndef ECMASCRIPT_MEM_MEM_MAP_ALLOCATOR_H
#define ECMASCRIPT_MEM_MEM_MAP_ALLOCATOR_H

#include <chrono>
#include <deque>
#include <map>
#include <random>
#include <set>

#include "ecmascript/ecma_macros.h"
#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/mem_common.h"
#include "ecmascript/platform/map.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
// Regular region with length of DEFAULT_REGION_SIZE(256kb)
class MemMapPool {
public:
    MemMapPool() = default;
    ~MemMapPool() = default;

    void Finalize()
    {
        LockHolder lock(lock_);
        for (auto &it : memMapVector_) {
            PageUnmap(it);
        }
        for (auto &it : regularMapCommitted_) {
            PageUnmap(it);
        }
        regularMapCommitted_.clear();
        memMapVector_.clear();
        memMapCache_.clear();
    }

    NO_COPY_SEMANTIC(MemMapPool);
    NO_MOVE_SEMANTIC(MemMapPool);

    MemMap GetMemFromCache([[maybe_unused]] size_t size)
    {
        ASSERT(size == REGULAR_MMAP_SIZE);
        LockHolder lock(lock_);
        if (!memMapCache_.empty()) {
            MemMap mem = memMapCache_.back();
            memMapCache_.pop_back();
            return mem;
        }
        return MemMap();
    }

    MemMap GetRegularMemFromCommitted([[maybe_unused]] size_t size)
    {
        ASSERT(size == REGULAR_MMAP_SIZE);
        LockHolder lock(lock_);
        if (!regularMapCommitted_.empty()) {
            MemMap mem = regularMapCommitted_.back();
            regularMapCommitted_.pop_back();
            return mem;
        }
        return MemMap();
    }

    bool IsRegularCommittedFull(size_t cachedSize)
    {
        LockHolder lock(lock_);
        size_t size = regularMapCommitted_.size();
        return size >= (cachedSize / REGULAR_MMAP_SIZE) ? true : false;
    }

    int ShouldFreeMore(size_t cachedSize)
    {
        LockHolder lock(lock_);
        int result = static_cast<int>(regularMapCommitted_.size());
        return result - static_cast<int>(cachedSize / REGULAR_MMAP_SIZE);
    }

    void AddMemToCommittedCache(void *mem, size_t size)
    {
        ASSERT(size == REGULAR_MMAP_SIZE);
        LockHolder lock(lock_);
        regularMapCommitted_.emplace_back(mem, size);
    }

    void AddMemToCache(void *mem, size_t size)
    {
        ASSERT(size == REGULAR_MMAP_SIZE);
        LockHolder lock(lock_);
        memMapCache_.emplace_back(mem, size);
    }

    MemMap SplitMemFromCache(MemMap memMap)
    {
        LockHolder lock(lock_);
        auto remainderMem = reinterpret_cast<uintptr_t>(memMap.GetMem()) + REGULAR_MMAP_SIZE;
        size_t remainderSize = AlignDown(memMap.GetSize() - REGULAR_MMAP_SIZE, REGULAR_MMAP_SIZE);
        size_t count = remainderSize / REGULAR_MMAP_SIZE;
        while (count-- > 0) {
            memMapCache_.emplace_back(reinterpret_cast<void *>(remainderMem), REGULAR_MMAP_SIZE);
            remainderMem = remainderMem + REGULAR_MMAP_SIZE;
        }
        return MemMap(memMap.GetMem(), REGULAR_MMAP_SIZE);
    }

    void SplitMemMapToCache(MemMap memMap)
    {
        auto memAddr = reinterpret_cast<uintptr_t>(memMap.GetMem());
        size_t memTotalSize = AlignDown(memMap.GetSize(), REGULAR_MMAP_SIZE);
        size_t count = memTotalSize / REGULAR_MMAP_SIZE;
        while (count-- > 0) {
            memMapCache_.emplace_back(reinterpret_cast<void *>(memAddr), REGULAR_MMAP_SIZE);
            memAddr += REGULAR_MMAP_SIZE;
        }
    }

    void InsertMemMap(MemMap memMap)
    {
        LockHolder lock(lock_);
        memMapVector_.emplace_back(memMap);
    }

private:
    static constexpr size_t REGULAR_MMAP_SIZE = 256_KB;
    Mutex lock_;
    std::deque<MemMap> memMapCache_;
    std::vector<MemMap> regularMapCommitted_;
    std::vector<MemMap> memMapVector_;
};

// Non regular region with length of DEFAULT_REGION_SIZE(256kb) multiple
class MemMapFreeList {
public:
    MemMapFreeList() = default;
    ~MemMapFreeList() = default;

    void Initialize(MemMap memMap, size_t capacity)
    {
        memMaps_.emplace_back(memMap);
        freeList_.emplace(memMap.GetSize(), memMap);
        capacity_ = capacity;
    }

    void Finalize()
    {
        for (auto &memMap : memMaps_) {
            PageUnmap(memMap);
        }
        memMaps_.clear();
        freeList_.clear();
    }

    void ResetCapacity(size_t capacity)
    {
        capacity_ = capacity;
    }

    NO_COPY_SEMANTIC(MemMapFreeList);
    NO_MOVE_SEMANTIC(MemMapFreeList);

    void MergeList()
    {
        auto it = freeList_.begin();
        while (it != freeList_.end()) {
            bool isEqual = false;
            void *startMem = (*it).second.GetMem();
            size_t newSize = (*it).second.GetSize();
            auto startIt = it++;
            if (it == freeList_.end()) {
                break;
            }
            auto next = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(startMem) + newSize);
            while (it != freeList_.end() && next == (*it).second.GetMem()) {
                newSize += (*it).second.GetSize();
                it = freeList_.erase(it);
                next = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(startMem) + newSize);
                isEqual = true;
            }
            if (isEqual) {
                freeList_.erase(startIt);
                freeList_.emplace(newSize, MemMap(startMem, newSize));
            }
        }
    }

    MemMap GetMemFromList(size_t size)
    {
        if (freeListPoolSize_ + size > capacity_) {
            LOG_GC(ERROR) << "Freelist pool oom: overflow(" << freeListPoolSize_ << ")";
            return MemMap();
        }
        LockHolder lock(lock_);
        auto iterate = freeList_.lower_bound(size);
        if (iterate == freeList_.end()) {
            MergeList();
            iterate = freeList_.lower_bound(size);
            // Unable to get memory from freeList, use PageMap
            if (iterate == freeList_.end()) {
                size_t incrementCapacity = std::max(size, INCREMENT_HUGE_OBJECT_CAPACITY);
                MemMap smemMap = PageMap(incrementCapacity, PAGE_PROT_NONE, DEFAULT_REGION_SIZE);
                LOG_GC(INFO) << "Huge object mem pool increase PageMap size: " << smemMap.GetSize();
                memMaps_.emplace_back(smemMap);
                freeList_.emplace(smemMap.GetSize(), smemMap);
                iterate = freeList_.lower_bound(size);
                ASSERT(iterate != freeList_.end());
            }
        }
        MemMap memMap = iterate->second;
        size_t remainderSize = memMap.GetSize() - size;
        freeList_.erase(iterate);
        if (remainderSize >= DEFAULT_REGION_SIZE) {
            auto next = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(memMap.GetMem()) + size);
            freeList_.emplace(remainderSize, MemMap(next, remainderSize));
        }
        freeListPoolSize_ += size;
        return MemMap(memMap.GetMem(), size);
    }

    void AddMemToList(MemMap memMap)
    {
        LockHolder lock(lock_);
        freeListPoolSize_ -= memMap.GetSize();
        freeList_.emplace(memMap.GetSize(), memMap);
    }

private:
    Mutex lock_;
    std::vector<MemMap> memMaps_;
    std::multimap<size_t, MemMap> freeList_;
    std::atomic_size_t freeListPoolSize_ {0};
    size_t capacity_ {0};
};

class MemMapAllocator {
public:
    MemMapAllocator() = default;
    ~MemMapAllocator() = default;

    NO_COPY_SEMANTIC(MemMapAllocator);
    NO_MOVE_SEMANTIC(MemMapAllocator);

    void Initialize(size_t alignment, bool isLargeHeap)
    {
        AdapterSuitablePoolCapacity(isLargeHeap);
        memMapTotalSize_ = 0;
#ifndef USE_CMC_GC
        InitializeHugeRegionMap(alignment);
        InitializeRegularRegionMap(alignment);
#endif
    }

    void Finalize()
    {
        memMapTotalSize_ = 0;
        capacity_ = 0;
        memMapFreeList_.Finalize();
        memMapPool_.Finalize();
    }

    size_t GetCapacity()
    {
        return capacity_;
    }

    void ResetLargePoolSize()
    {
        capacity_ = LARGE_HEAP_POOL_SIZE;
        memMapFreeList_.ResetCapacity(capacity_);
    }

    void IncreaseMemMapTotalSize(size_t bytes)
    {
        memMapTotalSize_.fetch_add(bytes);
        ECMA_BYTRACE_COUNT_TRACE(HITRACE_TAG_ARK, "Heap size (KB)", memMapTotalSize_ / 1_KB);
    }

    void DecreaseMemMapTotalSize(size_t bytes)
    {
        memMapTotalSize_.fetch_sub(bytes);
        ECMA_BYTRACE_COUNT_TRACE(HITRACE_TAG_ARK, "Heap size (KB)", memMapTotalSize_ / 1_KB);
    }

    static MemMapAllocator *GetInstance();

    MemMap Allocate(const uint32_t threadId, size_t size, size_t alignment,
                    const std::string &spaceName, bool regular, bool isCompress, bool isMachineCode,
                    bool isEnableJitFort, bool shouldPageTag);

    void CacheOrFree(void *mem, size_t size, bool isRegular, bool isCompress, size_t cachedSize,
                     bool shouldPageTag, bool skipCache);

    // This is only used when allocating region failed during GC, since it's unsafe to do HeapDump or throw OOM,
    // just make MemMapAllocator infinite to complete this GC, this will temporarily lead that all JSThread could
    // always AllcationRegion success, breaking the global region limit, but thread calling this will soon complete
    // GC and then fatal.
    void TransferToInfiniteModeForGC();

private:
    void InitializeRegularRegionMap(size_t alignment);
    void InitializeHugeRegionMap(size_t alignment);
    void InitializeCompressRegionMap(size_t alignment);

    MemMap AllocateFromMemPool(MemMapPool &pool, const uint32_t threadId, size_t size, size_t alignment,
                               const std::string &spaceName, bool isMachineCode, bool isEnableJitFort,
                               bool shouldPageTag, PageTagType type);
    MemMap InitialMemPool(MemMap &mem, const uint32_t threadId, size_t size, const std::string &spaceName,
                          bool isMachineCode, bool isEnableJitFort, bool shouldPageTag, PageTagType type);
    MemMap AlignMemMapTo4G(const MemMap &memMap, size_t targetSize);
    // Random generate big mem map addr to avoid js heap is written by others
    void *RandomGenerateBigAddr(uint64_t addr)
    {
        // Use the current time as the seed
        unsigned seed = static_cast<unsigned>(std::chrono::system_clock::now().time_since_epoch().count());
        std::mt19937_64 generator(seed);

        // Generate a random number between 0 and RANDOM_NUM_MAX
        std::uniform_int_distribution<uint64_t> distribution(0, RANDOM_NUM_MAX);
        uint64_t randomNum = distribution(generator);

        // Big addr random change in 0x2000000000 ~ 0x2FF0000000
        return reinterpret_cast<void *>(addr + (randomNum << RANDOM_SHIFT_BIT));
    }

    static constexpr size_t REGULAR_REGION_MMAP_SIZE = 4_MB;
    static constexpr uint64_t HUGE_OBJECT_MEM_MAP_BEGIN_ADDR = 0x1000000000;
    static constexpr uint64_t REGULAR_OBJECT_MEM_MAP_BEGIN_ADDR = 0x2000000000;
    static constexpr uint64_t STEP_INCREASE_MEM_MAP_ADDR = 0x1000000000;
    static constexpr size_t RANDOM_NUM_MAX = 0xFF;
    static constexpr size_t RANDOM_SHIFT_BIT = 28;
    static constexpr size_t MEM_MAP_RETRY_NUM = 10;

    void AdapterSuitablePoolCapacity(bool isLargeHeap);
    void Free(void *mem, size_t size, bool isRegular, bool isCompress);
    MemMapPool memMapPool_;
    MemMapPool compressMemMapPool_;
    MemMapFreeList memMapFreeList_;
    std::atomic_size_t memMapTotalSize_ {0};
    size_t capacity_ {0};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_MEM_MAP_ALLOCATOR_H
