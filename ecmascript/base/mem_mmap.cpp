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

#include <cstdlib>
#include "ecmascript/mem/mem_map_allocator.h"
#include "ecmascript/base/mem_mmap.h"

#if defined(PANDA_TARGET_WINDOWS)
#include <io.h>
#include <sysinfoapi.h>
#elif defined(PANDA_TARGET_MACOS)
#include "sys/sysctl.h"
#else
#include <sys/mman.h>
#include "sys/sysinfo.h"
#endif
namespace panda::ecmascript::base {
#ifdef PANDA_TARGET_WINDOWS
void *MemMmap::mmap(size_t size, int fd, off_t offset)
{
    HANDLE handle = ((fd == -1) ? INVALID_HANDLE_VALUE : reinterpret_cast<HANDLE>(_get_osfhandle(fd)));
    HANDLE extra = CreateFileMapping(handle, nullptr, PAGE_READWRITE,
                                     (DWORD) ((uint64_t) size >> 32),
                                     (DWORD) (size & 0xffffffff),
                                     nullptr);
    if (extra == nullptr) {
        return nullptr;
    }

    void *data = MapViewOfFile(extra, FILE_MAP_WRITE | FILE_MAP_READ,
                               (DWORD) ((uint64_t) offset >> 32),
                               (DWORD) (offset & 0xffffffff),
                               size);
    CloseHandle(extra);
    return data;
}
#endif

void *MemMmap::Mmap(size_t allocSize)
{
#ifdef PANDA_TARGET_UNIX
    void *addr = mmap(nullptr, allocSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_SHARED, -1, 0);
#else
    void *addr = MemMmap::mmap(allocSize, -1, 0);
#endif
    return addr;
}

void MemMmap::Munmap(void *addr, size_t allocSize)
{
#ifdef PANDA_TARGET_UNIX
    munmap(addr, allocSize);
#else
    UnmapViewOfFile(addr);
    allocSize = 0;
#endif
}
} // namespace panda::ecmascript::base