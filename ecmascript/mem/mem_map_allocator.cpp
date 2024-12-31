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
#include "ecmascript/platform/os.h"

namespace panda::ecmascript {
MemMapAllocator *MemMapAllocator::GetInstance()
{
    static MemMapAllocator *vmAllocator_ = new MemMapAllocator();
    return vmAllocator_;
}

void MemMapAllocator::InitializeRegularRegionMap([[maybe_unused]] size_t alignment)
{
#if defined(PANDA_TARGET_64) && !WIN_OR_MAC_OR_IOS_PLATFORM
    size_t initialRegularObjectCapacity = std::min(capacity_ / 2, INITIAL_REGULAR_OBJECT_CAPACITY);
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
    size_t initialHugeObjectCapacity = std::min(capacity_ / 2, INITIAL_HUGE_OBJECT_CAPACITY);
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
                                 const std::string &spaceName, bool regular, bool isMachineCode, bool isEnableJitFort,
                                 bool shouldPageTag)
{
    MemMap mem;
    PageTagType type = isMachineCode ? PageTagType::MACHINE_CODE : PageTagType::HEAP;

    if (regular) {
        mem = memMapPool_.GetRegularMemFromCommitted(size);
        if (mem.GetMem() != nullptr) {
            bool res = PageProtectMem(isMachineCode, mem.GetMem(), mem.GetSize(), isEnableJitFort);
            if (!res) {
                return MemMap();
            }
            if (shouldPageTag) {
                PageTag(mem.GetMem(), size, type, spaceName, threadId);
            }
            return mem;
        }
        if (UNLIKELY(memMapTotalSize_ + size > capacity_)) {
            LOG_GC(ERROR) << "memory map overflow";
            return MemMap();
        }
        mem = memMapPool_.GetMemFromCache(size);
        if (mem.GetMem() != nullptr) {
            memMapTotalSize_ += size;
            bool res = PageProtectMem(isMachineCode, mem.GetMem(), mem.GetSize(), isEnableJitFort);
            if (!res) {
                return MemMap();
            }
            if (shouldPageTag) {
                PageTag(mem.GetMem(), size, type, spaceName, threadId);
            }
            return mem;
        }
        mem = PageMap(REGULAR_REGION_MMAP_SIZE, PAGE_PROT_NONE, alignment);
        memMapPool_.InsertMemMap(mem);
        mem = memMapPool_.SplitMemFromCache(mem);
    } else {
        if (UNLIKELY(memMapTotalSize_ + size > capacity_)) { // LCOV_EXCL_BR_LINE
            LOG_GC(ERROR) << "memory map overflow";
            return MemMap();
        }
        mem = memMapFreeList_.GetMemFromList(size);
    }
    if (mem.GetMem() != nullptr) {
        bool res = PageProtectMem(isMachineCode, mem.GetMem(), mem.GetSize(), isEnableJitFort);
        if (!res) {
            return MemMap();
        }
        if (shouldPageTag) {
            PageTag(mem.GetMem(), mem.GetSize(), type, spaceName, threadId);
        }
        memMapTotalSize_ += mem.GetSize();
    }
    return mem;
}

void MemMapAllocator::CacheOrFree(void *mem, size_t size, bool isRegular, size_t cachedSize, bool shouldPageTag,
                                  bool skipCache)
{
    // Clear ThreadId tag and tag the mem with ARKTS HEAP.
    if (shouldPageTag) {
        PageTag(mem, size, PageTagType::HEAP);
    }
    if (!skipCache && isRegular && !memMapPool_.IsRegularCommittedFull(cachedSize)) {
        // Cache regions to accelerate allocation.
        memMapPool_.AddMemToCommittedCache(mem, size);
        return;
    }
    Free(mem, size, isRegular);
    if (!skipCache && isRegular && memMapPool_.ShouldFreeMore(cachedSize) > 0) {
        int freeNum = memMapPool_.ShouldFreeMore(cachedSize);
        for (int i = 0; i < freeNum; i++) {
            void *freeMem = memMapPool_.GetRegularMemFromCommitted(size).GetMem();
            if (freeMem != nullptr) {
                Free(freeMem, size, isRegular);
            } else {
                return;
            }
        }
    }
}

void MemMapAllocator::Free(void *mem, size_t size, bool isRegular)
{
    memMapTotalSize_ -= size;
    if (!PageProtect(mem, size, PAGE_PROT_NONE)) { // LCOV_EXCL_BR_LINE
        return;
    }
    PageRelease(mem, size);
    if (isRegular) {
        memMapPool_.AddMemToCache(mem, size);
    } else {
        memMapFreeList_.AddMemToList(MemMap(mem, size));
    }
}

void MemMapAllocator::AdapterSuitablePoolCapacity()
{
    size_t physicalSize = PhysicalSize();
    capacity_ = std::min<size_t>(physicalSize * DEFAULT_CAPACITY_RATE, MAX_MEM_POOL_CAPACITY);
    LOG_GC(INFO) << "Ark Auto adapter memory pool capacity:" << capacity_;
}

void MemMapAllocator::TransferToInfiniteModeForGC()
{
    capacity_ = std::numeric_limits<size_t>::max();
    LOG_GC(INFO) << "MemMapAllocator transfer to infinite mode:" << capacity_;
}
}  // namespace panda::ecmascript
