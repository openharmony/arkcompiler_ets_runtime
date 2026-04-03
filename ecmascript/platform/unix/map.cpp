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

#include <sys/mman.h>
#include <unistd.h>

#include "ecmascript/platform/os.h"

namespace panda::ecmascript {
MemMap PageMap(size_t size, int prot, size_t alignment, void *addr, int flags)
{
    ASSERT(size == AlignUp(size, PageSize()));
    ASSERT(alignment == AlignUp(alignment, PageSize()));
    size_t allocSize = size + alignment;
    int newFlags = static_cast<int>(MAP_PRIVATE | MAP_ANONYMOUS | static_cast<unsigned int>(flags));
    void *result = mmap(addr, allocSize, prot, newFlags, -1, 0);
    if (reinterpret_cast<intptr_t>(result) == -1) {
        LOG_ECMA(FATAL) << "mmap failed with error code:" << strerror(errno);
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
    PageTag(memMap.GetMem(), memMap.GetSize(), PageTagType::MACHINE_CODE);
    return memMap;
}

void MachineCodePageUnmap(MemMap it)
{
    PageClearTag(it.GetMem(), it.GetSize());
    if (!PageProtect(it.GetMem(), it.GetSize(), PAGE_PROT_NONE)) {
        return;
    }
    PageUnmap(it);
}

void PageRelease(void *mem, size_t size)
{
    madvise(mem, size, MADV_DONTNEED);
}

void PagePreRead(void *mem, size_t size)
{
    madvise(mem, size, MADV_WILLNEED);
}

void PageTag(void *mem, size_t size, PageTagType type, [[maybe_unused]] const std::string &spaceName,
    [[maybe_unused]] const uint32_t threadId)
{
#if defined(CROSS_PLATFORM)
    const char *tag = GetPageTagString(type);
    PrctlSetVMA(mem, size, tag);
#else
    const std::string &tag = GetPageTagString(type, spaceName, threadId);
    PrctlSetVMA(mem, size, tag.c_str());
#endif
}

void PageClearTag(void *mem, size_t size)
{
    PrctlSetVMA(mem, size, nullptr);
}

bool PageProtect(void *mem, size_t size, int prot)
{
    int ret = mprotect(mem, size, prot);
    if (ret != 0) {
        LOG_ECMA(ERROR) << "PageProtect mem = " << mem << ", size = " << size <<
            ", change to " << prot << " failed, ret = " << ret << ", error code is " << errno;
        return false;
    }
    return true;
}

size_t PageSize()
{
    return getpagesize();
}
}  // namespace panda::ecmascript
