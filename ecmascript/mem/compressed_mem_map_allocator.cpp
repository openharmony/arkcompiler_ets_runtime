/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

namespace panda::ecmascript {
void MemMapAllocator::InitializeRegularRegionMapForCompressedPointer(size_t capacity, void *addr)
{
    MemMap mem(addr, capacity);
    memMapPool_.InsertMemMap(mem);
    memMapPool_.SplitMemMapToCache(mem);
}

void MemMapAllocator::InitializeHugeRegionMapForCompressedPointer(size_t capacity, void *addr)
{
    MemMap mem(addr, capacity);
    memMapFreeList_.Initialize(mem);
}

MemMap MemMapAllocator::Allocate(const uint32_t threadId, size_t size, size_t alignment,
                                 const std::string &spaceName, bool regular, [[maybe_unused]] bool isCompress,
                                 bool isMachineCode, bool isEnableJitFort, bool shouldPageTag,
                                 bool skipCheckCapacity)
{
    if (UNLIKELY(memMapTotalSize_ + size > capacity_)) {
        LOG_GC(ERROR) << "memory map overflow";
        if (!skipCheckCapacity) {
            return MemMap();
        }
    }
    PageTagType type = isMachineCode ? PageTagType::MACHINE_CODE : PageTagType::HEAP;

    if (regular) {
        MemMap mem = AllocateFromMemPool(threadId, size, alignment, spaceName, isMachineCode,
            isEnableJitFort, shouldPageTag, type);
        if (mem.GetMem() == nullptr) {
            mem = memMapFreeList_.GetMemFromList(size);
            if (mem.GetMem() != nullptr) {
                InitialMemPool(mem, threadId, size, spaceName, isMachineCode, isEnableJitFort, true, type);
                IncreaseMemMapTotalSize(size);
            }
        }
        return mem;
    } else {
        // GC do not allocate Huge object
        ASSERT(!skipCheckCapacity);
        MemMap mem = memMapFreeList_.GetMemFromList(size);
        if (mem.GetMem() == nullptr) {
            mem = memMapPool_.TryGetHugeMem(size);
        }
        if (mem.GetMem() != nullptr) {
            InitialMemPool(mem, threadId, size, spaceName, isMachineCode, isEnableJitFort, true, type);
            IncreaseMemMapTotalSize(size);
        }
        return mem;
    }
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

    mem = memMapPool_.GetMemFromCache(size);
    if (mem.GetMem() != nullptr) {
        InitialMemPool(mem, threadId, size, spaceName, isMachineCode, isEnableJitFort, shouldPageTag, type);
        IncreaseMemMapTotalSize(size);
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

void MemMapAllocator::ReleaseMemory(void *mem, size_t size, bool isRegular, [[maybe_unused]] bool isCompress)
{
    if (PageProtect(mem, size, PAGE_PROT_NONE) != 0) { // LCOV_EXCL_BR_LINE
        return;
    }
    PageRelease(mem, size);
    if (isRegular) {
        memMapPool_.AddMemToCache(mem, size);
    } else {
        memMapFreeList_.AddMemToList(MemMap(mem, size));
    }
}
}  // namespace panda::ecmascript
