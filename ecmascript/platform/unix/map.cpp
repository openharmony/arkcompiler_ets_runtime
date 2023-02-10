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

#include "ecmascript/platform/map.h"

#include <cerrno>
#include <sys/mman.h>
#include <unistd.h>

#include "ecmascript/log_wrapper.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/platform/os.h"

namespace panda::ecmascript {
MemMap PageMap(size_t size, int prot, size_t alignment)
{
    ASSERT(size == AlignUp(size, PageSize()));
    ASSERT(alignment == AlignUp(alignment, PageSize()));
    size_t allocSize = size + alignment;
    void *result = mmap(nullptr, allocSize, prot, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
    if (reinterpret_cast<intptr_t>(result) == -1) {
        LOG_ECMA(FATAL) << "mmap failed with error code:" << errno;
    }
    if (alignment != 0) {
        auto alignResult = AlignUp(reinterpret_cast<uintptr_t>(result), alignment);
        size_t leftSize = alignResult - reinterpret_cast<uintptr_t>(result);
        size_t rightSize = alignment - leftSize;
        void *alignEndResult = reinterpret_cast<void *>(alignResult + size);
        munmap(result, leftSize);
        munmap(alignEndResult, rightSize);
        result = reinterpret_cast<void *>(alignResult);
    }
    return MemMap(result, size);
}

void PageUnmap(MemMap it)
{
    munmap(it.GetMem(), it.GetSize());
}

MemMap MachineCodePageMap(size_t size, int prot, size_t alignment)
{
    MemMap memMap = PageMap(size, prot, alignment);
    PageTag(memMap.GetMem(), memMap.GetSize());
    return memMap;
}

void MachineCodePageUnmap(MemMap it)
{
    PageTag(it.GetMem(), it.GetSize(), true);
    PageUnmap(it);
}

void PageRelease(void *mem, size_t size)
{
    madvise(mem, size, MADV_DONTNEED);
}

void PageTag(void *mem, size_t size, bool remove)
{
    if (remove) {
        PrctlSetVMA(mem, size, nullptr);
    } else {
        PrctlSetVMA(mem, size, "ArkJS Heap");
    }
}

void PageProtect(void *mem, size_t size, int prot)
{
    mprotect(mem, size, prot);
}

size_t PageSize()
{
    return getpagesize();
}
}  // namespace panda::ecmascript
