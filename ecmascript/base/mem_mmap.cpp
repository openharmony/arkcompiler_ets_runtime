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

#include "ecmascript/base/mem_mmap.h"

#if defined(PANDA_TARGET_WINDOWS)
#include <memoryapi.h>
#else
#include <sys/mman.h>
#endif

namespace panda::ecmascript::base {
void *MemMmap::Mmap(size_t allocSize, bool executable)
{
#if defined(PANDA_TARGET_WINDOWS)
    auto accessFlags = executable ? PAGE_EXECUTE_READWRITE : PAGE_READWRITE;
    return VirtualAlloc(nullptr, allocSize, MEM_COMMIT | MEM_RESERVE, accessFlags);  // commit memory immediately
#else
    int nullFileDescriptor = -1;  // null file descriptor for mapping anonymous space
    auto accessFlags = executable ? (PROT_READ | PROT_WRITE | PROT_EXEC) : (PROT_READ | PROT_WRITE);
    return mmap(nullptr, allocSize, accessFlags, MAP_ANONYMOUS | MAP_SHARED, nullFileDescriptor, 0);
#endif
}

int MemMmap::Munmap(void *addr, size_t allocSize)
{
#if defined(PANDA_TARGET_WINDOWS)
    return VirtualFree(addr, allocSize, MEM_RELEASE);
#else
    return munmap(addr, allocSize);
#endif
}
} // namespace panda::ecmascript::base