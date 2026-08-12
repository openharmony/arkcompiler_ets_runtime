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

#include <algorithm>
#include <fcntl.h>
#include <sys/mman.h>
#include <sys/syscall.h>
#include <unistd.h>

#include "ecmascript/platform/os.h"

namespace panda::ecmascript {

#if defined(PANDA_TARGET_ARM64) && defined(PANDA_TARGET_OHOS)
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
#if defined(PANDA_TARGET_ARM64) && defined(PANDA_TARGET_OHOS)
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
#if defined(PANDA_TARGET_ARM64) && defined(PANDA_TARGET_OHOS)
    long res = Syscall(SYS_munmap, (unsigned long)addr, len, 0, 0, 0, 0);
    if (res < 0) {
        errno = (int)res;
    }
    return (int)res;
#else
    return munmap(addr, len);
#endif
}

MemMap PageMap(size_t size, int prot, size_t alignment, void *addr, int flags, bool jitfort)
{
    ASSERT(size == AlignUp(size, PageSize()));
    ASSERT(alignment == AlignUp(alignment, PageSize()));
    size_t allocSize = size + alignment;
    int newFlags = static_cast<int>(MAP_PRIVATE | MAP_ANONYMOUS | static_cast<unsigned int>(flags));
    void *result =
        jitfort ? InlineMmap(addr, allocSize, prot, newFlags, -1, 0) : mmap(addr, allocSize, prot, newFlags, -1, 0);
    if (reinterpret_cast<intptr_t>(result) == -1) {
        LOG_ECMA(FATAL) << "mmap failed with error code:" << strerror(errno);
    }
    if (alignment != 0) {
        auto alignResult = AlignUp(reinterpret_cast<uintptr_t>(result), alignment);
        size_t leftSize = alignResult - reinterpret_cast<uintptr_t>(result);
        size_t rightSize = alignment - leftSize;
        void *alignEndResult = reinterpret_cast<void *>(alignResult + size);
        jitfort ? InlineMunmap(result, leftSize) : munmap(result, leftSize);
        jitfort ? InlineMunmap(alignEndResult, rightSize) : munmap(alignEndResult, rightSize);
        result = reinterpret_cast<void *>(alignResult);
    }
    return MemMap(result, size);
}

void PageUnmap(MemMap it, bool jitfort)
{
    if (jitfort) {
        InlineMunmap(it.GetMem(), it.GetSize());
    } else {
        munmap(it.GetMem(), it.GetSize());
    }
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
    if (PageProtect(it.GetMem(), it.GetSize(), PAGE_PROT_NONE) != 0) {
        return;
    }
    PageUnmap(it);
}

void PageRelease(void *mem, size_t size)
{
    madvise(mem, size, MADV_DONTNEED);
}

// --------------------------------------------------------------------------
// Shared fill template, see map.h.
namespace {
// One template per slot width: index 0 is 4-byte words, index 1 is 8-byte.
struct FillTemplate {
    int fd = -1;
    size_t size = 0;
    uint64_t word = 0;
};
FillTemplate g_fillTemplates[2];

// -1 for a width that is not a tagged slot width.
int TemplateIndex(size_t wordSize)
{
    if (wordSize == sizeof(uint32_t)) {
        return 0;
    }
    if (wordSize == sizeof(uint64_t)) {
        return 1;
    }
    return -1;
}
}  // namespace

bool CreateFillTemplate(size_t size, uint64_t fillWord, size_t wordSize)
{
    int idx = TemplateIndex(wordSize);
    // The pattern has to tile the page exactly, or the last slot of a page
    // would be truncated.
    if (idx < 0 || size == 0 || size % PageSize() != 0 || size % wordSize != 0) {
        return false;
    }
    FillTemplate &t = g_fillTemplates[idx];
    if (t.fd >= 0) {
        // Idempotent: a second VM in the same process reuses the first template.
        return t.size >= size && t.word == fillWord;
    }
    if (wordSize == sizeof(uint32_t) && fillWord > UINT32_MAX) {
        return false;
    }
#ifdef SYS_memfd_create
    int fd = static_cast<int>(syscall(SYS_memfd_create, "ArkTS Hole", MFD_CLOEXEC | MFD_ALLOW_SEALING));
#else
    int fd = -1;
#endif
    if (fd < 0) {
        return false;
    }
    if (ftruncate(fd, static_cast<off_t>(size)) != 0) {
        close(fd);
        return false;
    }
    // Fill it once through a shared mapping, then drop that mapping so the fd
    // can be sealed read-only.
    void *tmp = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);
    if (tmp == MAP_FAILED) {
        close(fd);
        return false;
    }
    if (wordSize == sizeof(uint32_t)) {
        uint32_t *words = reinterpret_cast<uint32_t *>(tmp);
        uint32_t word = static_cast<uint32_t>(fillWord);
        for (size_t i = 0; i < size / sizeof(uint32_t); i++) {
            words[i] = word;
        }
    } else {
        uint64_t *words = reinterpret_cast<uint64_t *>(tmp);
        for (size_t i = 0; i < size / sizeof(uint64_t); i++) {
            words[i] = fillWord;
        }
    }
    munmap(tmp, size);
    // Sealing blocks shared-writable mappings and truncation; MAP_PRIVATE
    // PROT_WRITE mappings stay legal, which is exactly what we need.
#ifdef F_ADD_SEALS
    fcntl(fd, F_ADD_SEALS, F_SEAL_SEAL | F_SEAL_SHRINK | F_SEAL_GROW | F_SEAL_WRITE);
#endif
    t.fd = fd;
    t.size = size;
    t.word = fillWord;
    return true;
}

void DestroyFillTemplates()
{
    for (auto &t : g_fillTemplates) {
        if (t.fd >= 0) {
            close(t.fd);
            t.fd = -1;
            t.size = 0;
            t.word = 0;
        }
    }
}

bool IsFillTemplateReady(size_t wordSize)
{
    int idx = TemplateIndex(wordSize);
    return idx >= 0 && g_fillTemplates[idx].fd >= 0;
}

bool MapFillTemplate(void *addr, size_t size, size_t wordSize)
{
    int idx = TemplateIndex(wordSize);
    if (idx < 0 || addr == nullptr || size == 0) {
        return false;
    }
    const FillTemplate &t = g_fillTemplates[idx];
    if (t.fd < 0) {
        return false;
    }
    size_t pageSize = PageSize();
    if (reinterpret_cast<uintptr_t>(addr) % pageSize != 0 || size % pageSize != 0) {
        return false;
    }
    uint8_t *cur = reinterpret_cast<uint8_t *>(addr);
    size_t done = 0;
    while (done < size) {
        size_t chunk = std::min(t.size, size - done);
        void *got = mmap(cur + done, chunk, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_FIXED, t.fd,
                         0);
        if (got == MAP_FAILED) {
            // Undo the part that succeeded so the caller sees all-or-nothing.
            if (done > 0) {
                UnmapFillTemplate(addr, done);
            }
            return false;
        }
        done += chunk;
    }
    return true;
}

bool UnmapFillTemplate(void *addr, size_t size)
{
    if (addr == nullptr || size == 0) {
        return false;
    }
    void *got = mmap(addr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED, -1, 0);
    return got != MAP_FAILED;
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

int PageProtect(void *mem, size_t size, int prot)
{
    int ret = mprotect(mem, size, prot);
    if (ret != 0) {
        LOG_ECMA(ERROR) << "PageProtect mem = " << mem << ", size = " << size <<
            ", change to " << prot << " failed, ret = " << ret << ", error code is " << errno;
        return ret;
    }
    return 0;
}

size_t PageSize()
{
    return getpagesize();
}
}  // namespace panda::ecmascript
