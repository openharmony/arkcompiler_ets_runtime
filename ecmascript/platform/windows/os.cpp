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
#include <sysinfoapi.h>
#include <timezoneapi.h>

#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {
size_t MallocUsableSize(void *ptr)
{
    return _msize(ptr);
}

int PrctlSetVMA([[maybe_unused]] const void *ptr, [[maybe_unused]] const size_t size, [[maybe_unused]] const char *tag)
{
    return -1;
}

long PtracePeektext([[maybe_unused]] int pid, [[maybe_unused]] uintptr_t addr)
{
    return static_cast<long>(-1);
}

void BindSmallCpuCore()
{
    LOG_ECMA(INFO) << "Bind Small Core in windows not support";
}

void BindMidCpuCore()
{
    LOG_ECMA(INFO) << "Bind Mid Core in windows not support";
}

void BindAllCpuCore()
{
    LOG_ECMA(INFO) << "Bind All Core in windows not support";
}

void *PageMapExecFortSpace(void *addr, [[maybe_unused]] size_t size, [[maybe_unused]] int prot)
{
    // basically no op
    return addr;
}

void SetSecurityLabel([[maybe_unused]] const std::string& path)
{
    LOG_ECMA(INFO) << "Set Security Label in windows not support";
}

void InitializeMallocConfig()
{
    LOG_ECMA(INFO) << "Initialize Malloc Config in windows not support";
}

bool CheckDiskSpace([[maybe_unused]] const std::string& path, [[maybe_unused]] size_t requiredBytes)
{
    LOG_ECMA(INFO) << "Check Disk Space not support in Windows";
    return true;
}
}  // namespace panda::ecmascript
