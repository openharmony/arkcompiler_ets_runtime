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

#include <ctime>
#include <malloc/malloc.h>
#include <sys/sysctl.h>
#include <unistd.h>

#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {
size_t MallocUsableSize(void *p)
{
    return malloc_size(p);
}

uint32_t NumberOfCpuCore()
{
    return static_cast<uint32_t>(sysconf(_SC_NPROCESSORS_ONLN));
}

size_t PhysicalSize()
{
    static constexpr int MIB_LENGTH = 2;
    int mib[2];
    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    int64_t size = 0;
    size_t bufferLength = sizeof(size);
    if (sysctl(mib, MIB_LENGTH, &size, &bufferLength, NULL, 0) != 0) {
        LOG_ECMA(FATAL) << "sysctl error";
    }
    return static_cast<size_t>(size);
}

int PrctlSetVMA([[maybe_unused]] const void *p, [[maybe_unused]] const size_t size, [[maybe_unused]] const char *tag)
{
    return -1;
}

long PtracePeektext([[maybe_unused]] int pid, [[maybe_unused]] uintptr_t addr)
{
    return static_cast<long>(-1);
}
}  // namespace panda::ecmascript
