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
#include <sys/types.h>
#include <sys/xattr.h>
#include <unistd.h>

#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {
size_t MallocUsableSize(void *p)
{
    return malloc_size(p);
}

int PrctlSetVMA([[maybe_unused]] const void *p, [[maybe_unused]] const size_t size, [[maybe_unused]] const char *tag)
{
    return -1;
}

long PtracePeektext([[maybe_unused]] int pid, [[maybe_unused]] uintptr_t addr)
{
    return static_cast<long>(-1);
}

void BindSmallCpuCore()
{
    LOG_ECMA(INFO) << "Bind Small Core in macos not support";
}

void BindMidCpuCore()
{
    LOG_ECMA(INFO) << "Bind Mid Core in macos not support";
}

void BindAllCpuCore()
{
    LOG_ECMA(INFO) << "Bind All Core in macos not support";
}

void *PageMapExecFortSpace(void *addr, [[maybe_unused]] size_t size, [[maybe_unused]] int prot)
{
    // basically no op
    return addr;
}

void SetSecurityLabel(const std::string& path)
{
    auto xattrValueSize = getxattr(path.c_str(), XATTR_KEY, nullptr, 0, 0, 0);
    if (xattrValueSize == static_cast<ssize_t>(DEFAULT_DATA_LENGTH)) {
        char xattrValue[DEFAULT_DATA_LENGTH + 1];
        xattrValueSize = getxattr(path.c_str(), XATTR_KEY, xattrValue, xattrValueSize, 0, 0);
        xattrValue[DEFAULT_DATA_LENGTH] = '\0';
    }

    if (setxattr(path.c_str(), XATTR_KEY, DEFAULT_DATA_LEVEL.data(), DEFAULT_DATA_LEVEL.size(), 0, 0) < 0) {
        LOG_ECMA(WARN) << "set label failed! level: " << DEFAULT_DATA_LEVEL << ", file: " << path;
    }
}

bool HasJitFortACL()
{
    LOG_ECMA(INFO) << "Get Jit Fort ACL in mac not support";
    return false;
}

void InitializeMallocConfig()
{
    LOG_ECMA(INFO) << "Initialize Malloc Config in mac not support";
}

bool CheckDiskSpace(const std::string& path, size_t requiredBytes)
{
    LOG_ECMA(INFO) << "Check Disk Space not support in Mac";
    return true;
}
}  // namespace panda::ecmascript
