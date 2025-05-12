/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#include "common_components/base_runtime/base_runtime_param.h"

#include <condition_variable>
#include <thread>
#include <set>
#if defined(_WIN64)
#include <windows.h>
#elif defined(__APPLE__)
#include "sys/sysctl.h"
#else
#include "sys/sysinfo.h"
#endif

#include "common_components/common_runtime/src/mutator/mutator_manager.h"
#include "common_components/common_runtime/src/heap_manager.h"
#include "common_components/common_runtime/src/log_manager.h"
#include "common_components/log/log.h"

namespace panda {
size_t BaseRuntimeParam::sysMemSize_ = 1 * GB;

void BaseRuntimeParam::CheckSysmemSize()
{
#if defined(_WIN64)
    MEMORYSTATUSEX memInfo;
    memInfo.dwLength = sizeof(MEMORYSTATUSEX);
    bool ret = GlobalMemoryStatusEx(&memInfo);
    if (ret) {
        sysMemSize_ = memInfo.ullTotalPhys;
    } else {
        LOG_COMMON(ERROR) << "Get system memory failed.\n";
    }
#elif defined(__APPLE__)
    const int sz = 2;
    int mib[sz];
    size_t length = sizeof(sysMemSize_);
    mib[0] = CTL_HW;
    mib[1] = HW_MEMSIZE;
    sysctl(mib, sz, &sysMemSize_, &length, nullptr, 0);
#else
    struct sysinfo memInfo;
    int ret = sysinfo(&memInfo);
    if (ret == 0) {
        sysMemSize_ = memInfo.totalram * memInfo.mem_unit;
    } else {
        LOG_COMMON(ERROR) << "Get system memory failed. msg: " << strerror(errno);
    }
#endif
}

size_t BaseRuntimeParam::InitHeapSize(size_t defaultParam)
{
    auto env = std::getenv("arkHeapSize");
    if (env == nullptr) {
        return defaultParam;
    }
    size_t size = CString::ParseSizeFromEnv(env);

#if defined(PANDA_TARGET_OHOS)
    // 64UL * KB: The minimum heap size in OHOS, measured in KB, the value is 64MB.
    size_t minSize = 64UL * KB;
#else
    // 4UL * KB: The minimum heap size, measured in KB, the value is 4MB.
    size_t minSize = 4UL * KB;
#endif
    size_t maxSize = sysMemSize_ / KB;
    if (size >= minSize && size <= maxSize) {
        return size;
    } else {
        LOG_COMMON(ERROR) << "Unsupported ark HeapSize parameter. The unit must be added when configuring, "
            "it supports 'kb', 'mb', 'gb'. Valid ark HeapSize range is [" << (minSize / KB) <<
            "MB, system memory size].";
    }
    return defaultParam;
}

size_t BaseRuntimeParam::InitRegionSize(size_t defaultParam)
{
    auto env = std::getenv("arkRegionSize");
    if (env == nullptr) {
        return defaultParam;
    }
    size_t size = CString::ParseSizeFromEnv(env);
    // The minimum region size is system page size, measured in KB.
    size_t minSize = panda::ARK_COMMON_PAGE_SIZE / KB;
    // 64UL: The maximum region size, measured in KB, the value is 2048 KB.
    size_t maxSize = 2 * 1024UL;
    if (size >= minSize && size <= maxSize) {
        return size;
    } else {
        LOG_COMMON(ERROR) << "Unsupported arkRegionSize parameter. Valid arkRegionSize range is [" <<
        minSize << "KB, 2048KB].";
    }
    return defaultParam;
}

double BaseRuntimeParam::InitGarbageRatio(double defaultParam)
{
    auto env = std::getenv("arkGarbageThreshold");
    if (env == nullptr) {
        return defaultParam;
    }
    double size = CString::ParsePosDecFromEnv(env);
    // 10: The minimum garbage ratio, measured in percent, the value is 10%.
    double minSize = 0.10;
    // 100: The maximum garbage ratio, measured in percent, the value is 100%.
    double maxSize = 1;
    if (size - minSize >= 0 && maxSize - size >= 0) {
        return size;
    } else {
        LOG_COMMON(ERROR) <<
            "Unsupported arkGarbageThreshold parameter. Valid arkGarbageThreshold range is [0.1, 1].\n";
    }
    return defaultParam;
}

double BaseRuntimeParam::InitPercentParameter(const char* name, double minSize, double maxSize, double defaultParam)
{
    auto env = std::getenv(name);
    if (env != nullptr) {
        double parameter = CString::ParsePosDecFromEnv(env);
        if (parameter - minSize > 0 && maxSize - parameter >= 0) {
            return parameter;
        } else {
            LOG_COMMON(ERROR) << "Unsupported " << name << " parameter. Valid " <<
            name << " range is [" << minSize << ", " << maxSize << "].";
        }
    }
    return defaultParam;
}

size_t BaseRuntimeParam::InitSizeParameter(const char* name, size_t minSize, size_t defaultParam)
{
    auto env = std::getenv(name);
    if (env != nullptr) {
        size_t parameter = CString::ParseSizeFromEnv(env);
        if (parameter > minSize) {
            return parameter;
        } else {
            LOG_COMMON(ERROR) << "Unsupported " << name << " parameter. Valid " << name <<
            " range must be greater than " << minSize << ".\n";
        }
    }
    return defaultParam;
}

size_t BaseRuntimeParam::InitTimeParameter(const char* name, size_t minSize, size_t defaultParam)
{
    auto env = std::getenv(name);
    if (env != nullptr) {
        size_t parameter = CString::ParseTimeFromEnv(env);
        if (parameter > minSize) {
            return parameter;
        } else {
            LOG_COMMON(ERROR) << "Unsupported " << name << " parameter. Valid " << name <<
                " range must be greater than " << minSize;
        }
    }
    return defaultParam;
}

double BaseRuntimeParam::InitDecParameter(const char* name, double minSize, double defaultParam)
{
    auto env = std::getenv(name);
    if (env != nullptr) {
        double parameter = CString::ParsePosDecFromEnv(env);
        if (parameter - minSize > 0) {
            return parameter;
        } else {
            LOG_COMMON(ERROR) << "Unsupported " << name << " parameter. Valid " << name <<
                " range must be greater than " << minSize;
        }
    }
    return defaultParam;
}

/**
 * Determine the default stack size and heap size according to system memory.
 * If system memory size is less then 1GB, heap size is 64MB and stack size is 64KB.
 * Otherwise heap size is 256MB and stack size is 1MB.
 */
RuntimeParam BaseRuntimeParam::InitRuntimeParam()
{
    CheckSysmemSize();
    // For address aligns inner 4G range, max capacity must smaller than 4G. Because of metadata.
    size_t initHeapSize = InitHeapSize(sysMemSize_ > 1 * GB ? 3.6 * MB : 64 * KB);
    RuntimeParam param = {
        .heapParam = {
#if defined(PANDA_TARGET_OHOS)
            // Default region size is 1024KB.
            .regionSize = InitRegionSize(1024UL),
#else
            // Default region size is 64KB.
            .regionSize = InitRegionSize(64UL),
#endif
            // Default heap size is 256MB if system memory size is greater than 1GB, otherwise is 64MB.
            .heapSize = initHeapSize,
            /*
             * The minimux live region threshold is 0% of region,
             * the maximum is 100% of region, default to 80% of region.
             */
            .exemptionThreshold = InitPercentParameter("arkExemptionThreshold", 0.0, 1.0, 0.8),
            /*
             * The minimux heap utilization is 0% of heap,
             * the maximum is 100% of heap, default to 80% of heap.
             */
            .heapUtilization = InitPercentParameter("arkHeapUtilization", 0.0, 1.0, 0.8),
            // Default heap growth is (1 + 0.15) = 1.15.
            .heapGrowth = InitDecParameter("arkHeapGrowth", 0.0, 0.15),
            // Default alloction rate is 10240MB/s.
            .allocationRate = InitDecParameter("arkAlloctionRate", 0.0, 10240),
            // Default alloction wait time is 1000ns.
            .allocationWaitTime = static_cast<size_t>(InitTimeParameter("arkAlloctionWaitTime", 0, 1000)),
        },
        .gcParam = {
            // Default gc threshold is heapSize.
            .gcThreshold = InitSizeParameter("arkGCThreshold", 0, initHeapSize) * KB,
            // Default garbage ration is 50% of from space.
            .garbageThreshold = InitGarbageRatio(0.5),
            // Default GC interval is 150ms.
            .gcInterval = static_cast<uint64_t>(InitTimeParameter("arkGCInterval", 0,
                150 * MILLI_SECOND_TO_NANO_SECOND)),
            // Default backup GC interval is 240s.
            .backupGCInterval = static_cast<uint64_t>(InitTimeParameter("arkBackupGCInterval", 0,
                240 * SECOND_TO_NANO_SECOND)),
            // Default GC thread factor is 2.
            .gcThreads = 2,
        },
    };
    return param;
}
} // namespace panda
