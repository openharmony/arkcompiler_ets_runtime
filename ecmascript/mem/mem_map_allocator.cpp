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

#include "ecmascript/mem/mem_map_allocator.h"

#include "common_components/platform/cpu.h"
#include "ecmascript/mem/tagged_state_word.h"
#include "ecmascript/platform/os.h"
#include "ecmascript/platform/parameters.h"
#include "libpandabase/mem/mem.h"

namespace panda::ecmascript {
MemMapAllocator *MemMapAllocator::GetInstance()
{
    static MemMapAllocator *vmAllocator_ = new MemMapAllocator();
    return vmAllocator_;
}

void MemMapAllocator::InitializeRegularRegionMap([[maybe_unused]] size_t alignment)
{
#if defined(PANDA_TARGET_64) && !WIN_OR_MAC_OR_IOS_PLATFORM
    size_t initialRegularObjectCapacity = std::min(capacity_ / 3, INITIAL_REGULAR_OBJECT_CAPACITY);
    size_t i = 0;
    while (i < MEM_MAP_RETRY_NUM) {
        void *addr = reinterpret_cast<void *>(ToUintPtr(RandomGenerateBigAddr(REGULAR_OBJECT_MEM_MAP_BEGIN_ADDR)) +
            i * STEP_INCREASE_MEM_MAP_ADDR);
        MemMap memMap = PageMap(initialRegularObjectCapacity, PAGE_PROT_NONE, alignment, addr);
        if (ToUintPtr(memMap.GetMem()) >= ToUintPtr(addr)) {
            PageTag(memMap.GetMem(), memMap.GetSize(), PageTagType::HEAP);
            PageRelease(memMap.GetMem(), memMap.GetSize());
            memMapPool_.InsertMemMap(memMap);
            memMapPool_.SplitMemMapToCache(memMap);
            break;
        } else {
            PageUnmap(memMap);
            LOG_ECMA(ERROR) << "Regular object mem map big addr fail: " << errno;
        }
        i++;
    }
#endif
}

void MemMapAllocator::InitializeHugeRegionMap(size_t alignment)
{
    size_t initialHugeObjectCapacity = std::min(capacity_ / 3, INITIAL_HUGE_OBJECT_CAPACITY);
#if defined(PANDA_TARGET_64) && !WIN_OR_MAC_OR_IOS_PLATFORM
    size_t i = 0;
    while (i <= MEM_MAP_RETRY_NUM) {
        void *addr = reinterpret_cast<void *>(ToUintPtr(RandomGenerateBigAddr(HUGE_OBJECT_MEM_MAP_BEGIN_ADDR)) +
            i * STEP_INCREASE_MEM_MAP_ADDR);
        MemMap memMap = PageMap(initialHugeObjectCapacity, PAGE_PROT_NONE, alignment, addr);
        if (ToUintPtr(memMap.GetMem()) >= ToUintPtr(addr) || i == MEM_MAP_RETRY_NUM) {
            PageTag(memMap.GetMem(), memMap.GetSize(), PageTagType::HEAP);
            PageRelease(memMap.GetMem(), memMap.GetSize());
            memMapFreeList_.Initialize(memMap, capacity_);
            break;
        } else {
            PageUnmap(memMap);
            LOG_ECMA(ERROR) << "Huge object mem map big addr fail: " << errno;
        }
        i++;
    }
#else
    MemMap hugeMemMap = PageMap(initialHugeObjectCapacity, PAGE_PROT_NONE, alignment);
    PageTag(hugeMemMap.GetMem(), hugeMemMap.GetSize(), PageTagType::HEAP);
    PageRelease(hugeMemMap.GetMem(), hugeMemMap.GetSize());
    memMapFreeList_.Initialize(hugeMemMap, capacity_);
#endif
}

void MemMapAllocator::InitializeCompressRegionMap(size_t alignment)
{
    size_t initialNonmovableObjectCapacity = std::min(capacity_ / 2, INITIAL_NONMOVALBE_OBJECT_CAPACITY);

#if defined(PANDA_TARGET_64)
    size_t alignNonmovableObjectCapacity = initialNonmovableObjectCapacity * 2;
#else
    size_t alignNonmovableObjectCapacity = initialNonmovableObjectCapacity;
#endif
#if defined(PANDA_TARGET_64) && !WIN_OR_MAC_OR_IOS_PLATFORM
    size_t i = 0;
    while (i <= MEM_MAP_RETRY_NUM) {
        void *addr = reinterpret_cast<void *>(ToUintPtr(RandomGenerateBigAddr(HUGE_OBJECT_MEM_MAP_BEGIN_ADDR)) +
            i * STEP_INCREASE_MEM_MAP_ADDR);
        MemMap memMap = PageMap(alignNonmovableObjectCapacity, PAGE_PROT_NONE, alignment, addr);
        if (ToUintPtr(memMap.GetMem()) >= ToUintPtr(addr) || i == MEM_MAP_RETRY_NUM) {
            memMap = AlignMemMapTo4G(memMap, initialNonmovableObjectCapacity);
            compressMemMapPool_.InsertMemMap(memMap);
            compressMemMapPool_.SplitMemMapToCache(memMap);
            break;
        } else {
            PageUnmap(memMap);
            LOG_ECMA(ERROR) << "Nonmovable object mem map big addr fail: " << errno;
        }
        i++;
    }
#else
    MemMap memMap = PageMap(alignNonmovableObjectCapacity, PAGE_PROT_NONE, alignment);
    memMap = AlignMemMapTo4G(memMap, initialNonmovableObjectCapacity);
    compressMemMapPool_.InsertMemMap(memMap);
    compressMemMapPool_.SplitMemMapToCache(memMap);
#endif
}

// Truncate mem to make sure align to same range inner 4G.
MemMap MemMapAllocator::AlignMemMapTo4G(const MemMap &memMap, size_t targetSize)
{
#if defined(PANDA_TARGET_64)
    uintptr_t startAddr = ToUintPtr(memMap.GetMem());
    uintptr_t alignAddr = AlignUp(startAddr, 4_GB);

    size_t leftSize = alignAddr - startAddr;
    uintptr_t remainderAddr = alignAddr;
    if (leftSize > memMap.GetSize()) {
        remainderAddr = startAddr;
    } else if (leftSize > targetSize) {
        remainderAddr = alignAddr - targetSize;
    }

#ifndef PANDA_TARGET_WINDOWS
    uintptr_t leftUnmapAddr = startAddr;
    size_t leftUnmapSize = remainderAddr - leftUnmapAddr;
    PageUnmap(MemMap(ToVoidPtr(leftUnmapAddr), leftUnmapSize));

    uintptr_t rightUnmapAddr = remainderAddr + targetSize;
    size_t rightUnmapSize = (startAddr + memMap.GetSize()) - rightUnmapAddr;
    PageUnmap(MemMap(ToVoidPtr(rightUnmapAddr), rightUnmapSize));
#endif

    static constexpr uint64_t mask = 0xFFFFFFFF00000000;
    TaggedStateWord::BASE_ADDRESS = remainderAddr & mask;

    MemMap reminderMemMap(ToVoidPtr(remainderAddr), targetSize);
    PageTag(reminderMemMap.GetOriginAddr(), reminderMemMap.GetSize(), PageTagType::HEAP);
    PageRelease(reminderMemMap.GetMem(), reminderMemMap.GetSize());

    return reminderMemMap;
#else
    TaggedStateWord::BASE_ADDRESS = 0;
    PageTag(memMap.GetMem(), memMap.GetSize(), PageTagType::HEAP);
    PageRelease(memMap.GetMem(), memMap.GetSize());
    return memMap;
#endif
}

static bool PageProtectMem(bool machineCodeSpace, void *mem, size_t size, [[maybe_unused]] bool isEnableJitFort)
{
    int prot = machineCodeSpace ? PAGE_PROT_EXEC_READWRITE : PAGE_PROT_READWRITE;

    if (!machineCodeSpace) {
        return PageProtect(mem, size, prot);
    }

    // MachineCode and HugeMachineCode space pages:
#if defined(PANDA_TARGET_ARM64) && defined(PANDA_TARGET_OHOS)
    if (isEnableJitFort) {
        // if JitFort enabled, Jit code will be in JitFort space, so only need READWRITE here
        return PageProtect(mem, size, PAGE_PROT_READWRITE);
    } else {
        // else Jit code will be in MachineCode space, need EXEC_READWRITE and MAP_EXECUTABLE (0x1000)
        void *addr = PageMapExecFortSpace(mem, size, PAGE_PROT_EXEC_READWRITE);
        if (addr != mem) {
            return false;
        }
        return true;
    }
#else
    // not running phone kernel. Jit code will be MachineCode space
    return PageProtect(mem, size, PAGE_PROT_EXEC_READWRITE);
#endif
}

MemMap MemMapAllocator::Allocate(const uint32_t threadId, size_t size, size_t alignment,
                                 const std::string &spaceName, bool regular, [[maybe_unused]]bool isCompress,
                                 bool isMachineCode, bool isEnableJitFort, bool shouldPageTag)
{
    PageTagType type = isMachineCode ? PageTagType::MACHINE_CODE : PageTagType::HEAP;

    if (regular) {
        if (isCompress) {
            return AllocateFromCompressPool(threadId, size, alignment, spaceName, isMachineCode,
                isEnableJitFort, shouldPageTag, type);
        } else {
            return AllocateFromMemPool(threadId, size, alignment, spaceName, isMachineCode,
                isEnableJitFort, shouldPageTag, type);
        }
    } else {
        if (UNLIKELY(memMapTotalSize_ + size > capacity_)) { // LCOV_EXCL_BR_LINE
            LOG_GC(ERROR) << "memory map overflow";
            return MemMap();
        }
        MemMap mem = memMapFreeList_.GetMemFromList(size);
        if (mem.GetMem() != nullptr) {
            InitialMemPool(mem, threadId, size, spaceName, isMachineCode, isEnableJitFort, shouldPageTag, type);
            IncreaseMemMapTotalSize(size);
        }
        return mem;
    }
}

MemMap MemMapAllocator::AllocateFromCompressPool(const uint32_t threadId, size_t size, size_t alignment,
                                                 const std::string &spaceName, bool isMachineCode,
                                                 bool isEnableJitFort, bool shouldPageTag, PageTagType type)
{
    MemMap mem = compressMemMapPool_.GetRegularMemFromCommitted(size);
    if (mem.GetMem() != nullptr) {
        InitialMemPool(mem, threadId, size, spaceName, isMachineCode, isEnableJitFort, shouldPageTag, type);
        return mem;
    }
    if (UNLIKELY(memMapTotalSize_ + size > capacity_)) {
        LOG_GC(ERROR) << "memory map overflow";
        return MemMap();
    }

    mem = compressMemMapPool_.GetMemFromCache(size);
    if (mem.GetMem() != nullptr) {
        memMapTotalSize_ += size;
        InitialMemPool(mem, threadId, size, spaceName, isMachineCode, isEnableJitFort, shouldPageTag, type);
        return mem;
    }

#if !defined(PANDA_TARGET_64)
    mem = PageMap(REGULAR_REGION_MMAP_SIZE, PAGE_PROT_NONE, alignment);
    compressMemMapPool_.InsertMemMap(mem);
    mem = compressMemMapPool_.SplitMemFromCache(mem);
    if (mem.GetMem() != nullptr) {
        InitialMemPool(mem, threadId, size, spaceName, isMachineCode, isEnableJitFort, shouldPageTag, type);
        memMapTotalSize_ += mem.GetSize();
        return mem;
    }
#endif
    LOG_GC(ERROR) << "compress pool overflow";
    return MemMap();
}

MemMap MemMapAllocator::AllocateFromMemPool(const uint32_t threadId, size_t size, size_t alignment,
                                            const std::string &spaceName, bool isMachineCode, bool isEnableJitFort,
                                            bool shouldPageTag, PageTagType type)
{
    MemMap mem = memMapPool_.GetRegularMemFromCommitted(size);
    if (mem.GetMem() != nullptr) {
        InitialMemPool(mem, threadId, size, spaceName, isMachineCode, isEnableJitFort, shouldPageTag, type);
        return mem;
    }
    if (UNLIKELY(memMapTotalSize_ + size > capacity_)) {
        LOG_GC(ERROR) << "memory map overflow";
        return MemMap();
    }

    mem = memMapPool_.GetMemFromCache(size);
    if (mem.GetMem() != nullptr) {
        InitialMemPool(mem, threadId, size, spaceName, isMachineCode, isEnableJitFort, shouldPageTag, type);
        IncreaseMemMapTotalSize(size);
        return mem;
    }

    mem = PageMap(REGULAR_REGION_MMAP_SIZE, PAGE_PROT_NONE, alignment);
    memMapPool_.InsertMemMap(mem);
    mem = memMapPool_.SplitMemFromCache(mem);
    if (mem.GetMem() != nullptr) {
        InitialMemPool(mem, threadId, size, spaceName, isMachineCode, isEnableJitFort, shouldPageTag, type);
        IncreaseMemMapTotalSize(size);
    }
    return mem;
}

MemMap MemMapAllocator::InitialMemPool(MemMap &mem, const uint32_t threadId, size_t size, const std::string &spaceName,
                                       bool isMachineCode, bool isEnableJitFort,
                                       bool shouldPageTag, PageTagType type)
{
    bool res = PageProtectMem(isMachineCode, mem.GetMem(), mem.GetSize(), isEnableJitFort);
    if (!res) {
        return MemMap();
    }
    if (shouldPageTag) {
        PageTag(mem.GetMem(), size, type, spaceName, threadId);
    }
    return mem;
}

void MemMapAllocator::CacheOrFree(void *mem, size_t size, bool isRegular, bool isCompress, size_t cachedSize,
                                  bool shouldPageTag, bool skipCache)
{
    // Clear ThreadId tag and tag the mem with ARKTS HEAP.
    if (shouldPageTag) {
        PageTag(mem, size, PageTagType::HEAP);
    }
    if (!skipCache && isRegular && !isCompress && !memMapPool_.IsRegularCommittedFull(cachedSize)) {
        // Cache regions to accelerate allocation.
        memMapPool_.AddMemToCommittedCache(mem, size);
        return;
    }
    Free(mem, size, isRegular, isCompress);
    if (!skipCache && isRegular && !isCompress && memMapPool_.ShouldFreeMore(cachedSize) > 0) {
        int freeNum = memMapPool_.ShouldFreeMore(cachedSize);
        for (int i = 0; i < freeNum; i++) {
            void *freeMem = memMapPool_.GetRegularMemFromCommitted(size).GetMem();
            if (freeMem != nullptr) {
                Free(freeMem, size, isRegular, isCompress);
            } else {
                return;
            }
        }
    }
}

void MemMapAllocator::Free(void *mem, size_t size, bool isRegular, bool isCompress)
{
    DecreaseMemMapTotalSize(size);
    if (!PageProtect(mem, size, PAGE_PROT_NONE)) { // LCOV_EXCL_BR_LINE
        return;
    }
    PageRelease(mem, size);
    if (isRegular) {
        if (isCompress) {
            compressMemMapPool_.AddMemToCache(mem, size);
        } else {
            memMapPool_.AddMemToCache(mem, size);
        }
    } else {
        memMapFreeList_.AddMemToList(MemMap(mem, size));
    }
}

void MemMapAllocator::AdapterSuitablePoolCapacity(bool isLargeHeap)
{
    size_t physicalSize = PhysicalSize();
    uint64_t poolSize;
    if (isLargeHeap) {
        poolSize = LARGE_HEAP_POOL_SIZE;
    } else {
        poolSize = GetPoolSize(MAX_MEM_POOL_CAPACITY);
    }
    capacity_ = std::min<size_t>(physicalSize * DEFAULT_CAPACITY_RATE, poolSize);
    LOG_GC(INFO) << "Ark Auto adapter memory pool capacity:" << capacity_;
}

void MemMapAllocator::TransferToInfiniteModeForGC()
{
    capacity_ = std::numeric_limits<size_t>::max();
    LOG_GC(INFO) << "MemMapAllocator transfer to infinite mode:" << capacity_;
}
}  // namespace panda::ecmascript
