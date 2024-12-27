/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include <cinttypes>
#include <dlfcn.h>
#include "securec.h"

#include "ecmascript/mem/mem.h"
#if defined(ENABLE_UNWINDER) && defined(__aarch64__)
#if defined(__clang__)
#pragma clang diagnostic push
#pragma clang diagnostic ignored "-Wunused-parameter"
#pragma clang diagnostic ignored "-Wshadow"
#endif
#include "fp_unwinder.h"
#if defined(__clang__)
#pragma clang diagnostic pop
#endif
#endif

namespace panda::ecmascript {
static const std::string LIB_UNWIND_SO_NAME = "libunwind.so";
static const std::string LIB_UNWIND_Z_SO_NAME = "libunwind.z.so";
static const int MAX_STACK_SIZE = 16;
static const int LOG_BUF_LEN = 1024;

using UnwBackTraceFunc = int (*)(void**, int);

static std::map<void *, Dl_info> stackInfoCache;

#if defined(ENABLE_UNWINDER) && defined(__aarch64__)
static inline ARK_INLINE void GetPcFpRegs([[maybe_unused]] void *regs)
{
    asm volatile(
    "1:\n"
    "adr x12, 1b\n"
    "stp x12, x29, [%[base], #0]\n"
    : [base] "+r"(regs)
    :
    : "x12", "memory");
}
#endif

bool GetPcs(size_t &size, uintptr_t* pcs)
{
#if defined(ENABLE_UNWINDER) && defined(__aarch64__)
    uintptr_t regs[2] = {0}; // 2: pc and fp reg
    GetPcFpRegs(regs);
    uintptr_t pc = regs[0];
    uintptr_t fp = regs[1];
    size = OHOS::HiviewDFX::FpUnwinder::GetPtr()->Unwind(pc, fp, pcs, MAX_STACK_SIZE);
    if (size <= 1) {
        size = OHOS::HiviewDFX::FpUnwinder::GetPtr()->UnwindSafe(pc, fp, pcs, MAX_STACK_SIZE);
    }
#else
    static UnwBackTraceFunc unwBackTrace = nullptr;
    if (!unwBackTrace) {
        void *handle = dlopen(LIB_UNWIND_SO_NAME.c_str(), RTLD_NOW);
        if (handle == nullptr) {
            handle = dlopen(LIB_UNWIND_Z_SO_NAME.c_str(), RTLD_NOW);
            if (handle == nullptr) {
                LOG_ECMA(INFO) << "dlopen libunwind.so failed";
                return false;
            }
        }
        unwBackTrace = reinterpret_cast<UnwBackTraceFunc>(dlsym(handle, "unw_backtrace"));
        if (unwBackTrace == nullptr) {
            LOG_ECMA(INFO) << "dlsym unw_backtrace failed";
            return false;
        }
    }
    size = unwBackTrace(reinterpret_cast<void**>(pcs), MAX_STACK_SIZE);
#endif
    return true;
}

void Backtrace(std::ostringstream &stack, bool enableCache)
{
    uintptr_t pcs[MAX_STACK_SIZE] = {0};
    size_t unwSz = 0;
    if (!GetPcs(unwSz, pcs)) {
        return;
    }
    stack << "=====================Backtrace========================";
#if defined(ENABLE_UNWINDER) && defined(__aarch64__)
    size_t i = 0;
#else
    size_t i = 1;
#endif
    for (; i < unwSz; i++) {
        Dl_info info;
        auto iter = stackInfoCache.find(reinterpret_cast<void *>(pcs[i]));
        if (enableCache && iter != stackInfoCache.end()) {
            info = iter->second;
        } else {
            if (!dladdr(reinterpret_cast<void *>(pcs[i]), &info)) {
                break;
            }
            if (enableCache) {
                stackInfoCache.emplace(reinterpret_cast<void *>(pcs[i]), info);
            }
        }
        const char *file = info.dli_fname ? info.dli_fname : "";
        uint64_t offset = info.dli_fbase ? pcs[i] - ToUintPtr(info.dli_fbase) : 0;
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
        stack << buf;
    }
}
} // namespace panda::ecmascript
