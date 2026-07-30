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

namespace panda::ecmascript {
MemMapAllocator *MemMapAllocator::GetInstance()
{
    static MemMapAllocator *vmAllocator_ = new MemMapAllocator();
    return vmAllocator_;
}

static int PageProtectMem(bool machineCodeSpace, void *mem, size_t size, [[maybe_unused]] bool isEnableJitFort)
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

        return addr == mem ? 0 : -1;
    }
#else
    // not running phone kernel. Jit code will be MachineCode space
    return PageProtect(mem, size, PAGE_PROT_EXEC_READWRITE);
#endif
}

void MemMapAllocator::InitialMemPool(MemMap &mem, const uint32_t threadId, size_t size, const std::string &spaceName,
                                     bool isMachineCode, bool isEnableJitFort,
                                     bool shouldPageTag, PageTagType type)
{
    int res = PageProtectMem(isMachineCode, mem.GetMem(), mem.GetSize(), isEnableJitFort);
    if (res != 0) { // LCOV_EXCL_BR_LINE
        LOG_COMMON(FATAL) << "Page Protect failed. Ret of mprotect is " << res; // LCOV_EXCL_LINE
    }
    if (shouldPageTag) {
        PageTag(mem.GetMem(), size, type, spaceName, threadId);
    }
}

// MemUsage has been decreased for async free mem.
void MemMapAllocator::AsyncFree(void *mem, size_t size, bool isRegular, bool isCompress, bool shouldPageTag)
{
    if (shouldPageTag) {
        PageTag(mem, size, PageTagType::HEAP);
    }
    ReleaseMemory(mem, size, isRegular, isCompress);
}

void MemMapAllocator::Free(void *mem, size_t size, bool isRegular, bool isCompress)
{
    DecreaseMemMapTotalSize(size);
    ReleaseMemory(mem, size, isRegular, isCompress);
}

void MemMapAllocator::AdapterSuitablePoolCapacity(bool isLargeHeap)
{
    size_t physicalSize = common::PhysicalSize();
    uint64_t poolSize;
    if (isLargeHeap) {
        poolSize = LARGE_HEAP_POOL_SIZE;
    } else {
        poolSize = GetPoolSize(MAX_MEM_POOL_CAPACITY);
    }
    if (g_isEnableCMCGC) {
        constexpr double capacityRate = DEFAULT_CAPACITY_RATE;
        capacity_ = std::min<size_t>(physicalSize * capacityRate, poolSize);
#ifndef PANDA_TARGET_32
        // 2: double size, for cmc copy
        capacity_ *= 2;
#endif
    } else {
        capacity_ = std::min<size_t>(physicalSize * DEFAULT_CAPACITY_RATE, poolSize);
    }

    LOG_GC(INFO) << "Ark Auto adapter memory pool capacity:" << capacity_;
}
}  // namespace panda::ecmascript
