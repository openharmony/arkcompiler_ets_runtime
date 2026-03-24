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
#include <sys/syscall.h>
#include <unistd.h>

#include "ecmascript/platform/os.h"

namespace panda::ecmascript {

#ifdef PANDA_TARGET_ARM64
static long Syscall(unsigned long n, unsigned long a, unsigned long b, unsigned long c, unsigned long d,
                    unsigned long e, unsigned long f)
{
    register unsigned long x8 asm("x8") = n;
    register unsigned long x0 asm("x0") = a;
    register unsigned long x1 asm("x1") = b;
    register unsigned long x2 asm("x2") = c;
    register unsigned long x3 asm("x3") = d;
    register unsigned long x4 asm("x4") = e;
    register unsigned long x5 asm("x5") = f;
    asm volatile("svc 0" : "=r"(x0) : "r"(x8), "0"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5) : "memory", "cc");
    return x0;
}
#endif

static inline void *InlineMmap(void *addr, size_t len, int prot, int flags, int fd, off_t offset)
{
#ifdef PANDA_TARGET_ARM64
    long res = Syscall(SYS_mmap, (unsigned long)addr, len, prot, flags, fd, offset);
    if (res < 0) {
        errno = (int)res;
        return MAP_FAILED;
    } else {
        return (void *)res;
    }
#else
    return mmap(addr, len, prot, flags, fd, offset);
#endif
}

static inline int InlineMunmap(void *addr, size_t len)
{
#ifdef PANDA_TARGET_ARM64
    long res = Syscall(SYS_munmap, (unsigned long)addr, len, 0, 0, 0, 0);
    if (res < 0) {
        errno = (int)res;
    }
    return (int)res;
#else
    return munmap(addr, len);
#endif
}

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

MemMap JitFortPageMap(size_t size, int prot, size_t alignment, void *addr, int flags)
{
    ASSERT(size == AlignUp(size, PageSize()));
    ASSERT(alignment == AlignUp(alignment, PageSize()));
    size_t allocSize = size + alignment;
    int newFlags = static_cast<int>(MAP_PRIVATE | MAP_ANONYMOUS | static_cast<unsigned int>(flags));
    void *result = InlineMmap(addr, allocSize, prot, newFlags, -1, 0);
    if (reinterpret_cast<intptr_t>(result) == -1) {
        LOG_ECMA(FATAL) << "mmap failed with error code:" << strerror(errno);
    }
    if (alignment != 0) {
        auto alignResult = AlignUp(reinterpret_cast<uintptr_t>(result), alignment);
        size_t leftSize = alignResult - reinterpret_cast<uintptr_t>(result);
        size_t rightSize = alignment - leftSize;
        void *alignEndResult = reinterpret_cast<void *>(alignResult + size);
        InlineMunmap(result, leftSize);
        InlineMunmap(alignEndResult, rightSize);
        result = reinterpret_cast<void *>(alignResult);
    }
    return MemMap(result, size);
}

void JitFortPageUnmap(MemMap it)
{
    InlineMunmap(it.GetMem(), it.GetSize());
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
