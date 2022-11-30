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

#include "ecmascript/platform/os.h"

#include <malloc.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#ifndef PR_SET_VMA
#define PR_SET_VMA 0x53564d41
#endif

#ifndef PR_SET_VMA_ANON_NAME
#define PR_SET_VMA_ANON_NAME 0
#endif

namespace panda::ecmascript {
size_t MallocUsableSize(void *p)
{
    return malloc_usable_size(p);
}

uint32_t NumberOfCpuCore()
{
    return static_cast<uint32_t>(sysconf(_SC_NPROCESSORS_ONLN));
}

size_t PhysicalSize()
{
    auto pages = sysconf(_SC_PHYS_PAGES);
    auto pageSize = sysconf(_SC_PAGE_SIZE);
    return pages * pageSize;
}

int PrctlSetVMA(const void *p, const size_t size, const char *tag)
{
    return prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, p, size, tag);
}

long PtracePeektext(int pid, uintptr_t addr)
{
    return ptrace(PTRACE_PEEKTEXT, pid, addr, NULL);
}
}  // namespace panda::ecmascript
