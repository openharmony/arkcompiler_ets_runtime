/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/platform/backtrace.h"

#include <cstring>
#include <cinttypes>
#include <dlfcn.h>
#include <iomanip>
#include <ios>
#include <map>
#include <unwind.h>
#include "securec.h"

#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
static const std::string LIB_UNWIND_SO_NAME = "libunwind.so";
static const std::string LIB_UNWIND_Z_SO_NAME = "libunwind.z.so";
static const int MAX_STACK_SIZE = 16;
static const int LOG_BUF_LEN = 1024;

using UnwBackTraceFunc = int (*)(void**, int);

static std::map<void *, Dl_info> stackInfoCache;

void Backtrace(std::ostringstream &stack, bool enableCache)
{
    static UnwBackTraceFunc unwBackTrace = nullptr;
    if (!unwBackTrace) {
        void *handle = dlopen(LIB_UNWIND_SO_NAME.c_str(), RTLD_NOW);
        if (handle == nullptr) {
            handle = dlopen(LIB_UNWIND_Z_SO_NAME.c_str(), RTLD_NOW);
            if (handle == nullptr) {
                LOG_ECMA(INFO) << "dlopen libunwind.so failed";
                return;
            }
        }
        unwBackTrace = reinterpret_cast<UnwBackTraceFunc>(dlsym(handle, "unw_backtrace"));
        if (unwBackTrace == nullptr) {
            LOG_ECMA(INFO) << "dlsym unw_backtrace failed";
            return;
        }
    }

    void *buffer[MAX_STACK_SIZE] = { nullptr };
    int level = unwBackTrace(reinterpret_cast<void**>(&buffer), MAX_STACK_SIZE);
    stack << "=====================Backtrace========================";
    for (int i = 1; i < level; i++) {
        Dl_info info;
        auto iter = stackInfoCache.find(buffer[i]);
        if (enableCache && iter != stackInfoCache.end()) {
            info = iter->second;
        } else {
            if (!dladdr(buffer[i], &info)) {
                break;
            }
            if (enableCache) {
                stackInfoCache.emplace(buffer[i], info);
            }
        }
        const char *file =  info.dli_fname ? info.dli_fname : "";
        uint64_t offset = info.dli_fbase ? ToUintPtr(buffer[i]) - ToUintPtr(info.dli_fbase) : 0;
        char buf[LOG_BUF_LEN] = {0};
        char frameFormatWithMapName[] = "#%02zu pc %016" PRIx64 " %s";
        int ret = 0;
        ret = static_cast<int>(snprintf_s(buf, sizeof(buf), sizeof(buf) - 1, frameFormatWithMapName, \
            i, offset, file));
        if (ret <= 0) {
            LOG_ECMA(ERROR) << "Backtrace snprintf_s failed";
            return;
        }
        stack << std::endl;
        stack << std::string(buf, strlen(buf));
    }
}
} // namespace panda::ecmascript
