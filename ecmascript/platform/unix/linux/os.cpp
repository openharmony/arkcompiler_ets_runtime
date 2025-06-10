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
#include <sys/mman.h>
#include <sys/prctl.h>
#include <sys/ptrace.h>
#include <sys/statvfs.h>
#include <sys/xattr.h>
#include <unistd.h>

#include "ecmascript/log_wrapper.h"

#ifndef PR_SET_VMA
#define PR_SET_VMA 0x53564d41
#endif

#ifndef PR_SET_VMA_ANON_NAME
#define PR_SET_VMA_ANON_NAME 0
#endif

#define JITFORT_QUERY_ENCAPS 'E'
#define HM_PR_SET_JITFORT 0x6a6974

namespace panda::ecmascript {
size_t MallocUsableSize(void *p)
{
    return malloc_usable_size(p);
}

int PrctlSetVMA(const void *p, const size_t size, const char *tag)
{
    return prctl(PR_SET_VMA, PR_SET_VMA_ANON_NAME, p, size, tag);
}

long PtracePeektext(int pid, uintptr_t addr)
{
    return ptrace(PTRACE_PEEKTEXT, pid, addr, NULL);
}

void BindSmallCpuCore()
{
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset); // 0: bind this thread to cpu0
    CPU_SET(1, &cpuset); // 1: bind this thread to cpu1
    CPU_SET(2, &cpuset); // 2: bind this thread to cpu2
    CPU_SET(3, &cpuset); // 3: bind this thread to cpu3

    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) == -1) {
        LOG_ECMA(ERROR) << "Set CPU affinity failed";
    }
}

void BindMidCpuCore()
{
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    for (size_t i = 0; i < 7; i++) { // 7: 0-3 little core, 4-6 mid core
        CPU_SET(i, &cpuset);
    }

    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) == -1) {
        LOG_ECMA(ERROR) << "Set CPU affinity failed";
    }
}

void BindAllCpuCore()
{
    cpu_set_t cpuset;

    CPU_ZERO(&cpuset);
    for (size_t i = 0; i < common::NumberOfCpuCore(); i++) {
        CPU_SET(i, &cpuset);
    }

    if (sched_setaffinity(0, sizeof(cpuset), &cpuset) == -1) {
        LOG_ECMA(ERROR) << "Set CPU affinity failed";
    }
}

void *PageMapExecFortSpace(void *addr, size_t size, int prot)
{
    void *res = mmap(addr, size, prot, MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED | MAP_EXECUTABLE, -1, 0);
    if (res == MAP_FAILED) {
        LOG_ECMA(ERROR) << "PageMapExecFortSpace mmap failed, addr = " << addr << ", size = " << size <<
            ", prot = " << prot << ", error code is " << errno;
    }
    return res;
}

bool HasJitFortACL()
{
    return (prctl(HM_PR_SET_JITFORT, JITFORT_QUERY_ENCAPS, 0) == 0);
}

void SetSecurityLabel(const std::string& path)
{
    auto xattrValueSize = getxattr(path.c_str(), XATTR_KEY, nullptr, 0);
    if (xattrValueSize == static_cast<ssize_t>(DEFAULT_DATA_LENGTH)) {
        char xattrValue[DEFAULT_DATA_LENGTH + 1];
        xattrValueSize = getxattr(path.c_str(), XATTR_KEY, xattrValue, xattrValueSize);
        xattrValue[DEFAULT_DATA_LENGTH] = '\0';
    }

    if (setxattr(path.c_str(), XATTR_KEY, DEFAULT_DATA_LEVEL.data(), DEFAULT_DATA_LEVEL.size(), 0) < 0) {
        LOG_ECMA(WARN) << "set label failed! level: " << DEFAULT_DATA_LEVEL << ", file: " << path;
    }
}

bool CheckDiskSpace(const std::string& path, size_t requiredBytes)
{
    struct statvfs stat;
    if (statvfs(path.c_str(), &stat) != 0) {
        return false;
    }
    size_t availableBytes = static_cast<size_t>(stat.f_bavail) * stat.f_frsize;
    return availableBytes >= requiredBytes;
}
}  // namespace panda::ecmascript
