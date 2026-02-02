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
#include <map>

#include "ecmascript/base/config.h"
#include "ecmascript/platform/mutex.h"
#include "securec.h"

#include "ecmascript/mem/mem.h"
#if defined(ENABLE_BACKTRACE_LOCAL)
#include "dfx_frame_formatter.h"
#include "fp_backtrace.h"
#endif

namespace panda::ecmascript {
static const std::string LIB_UNWIND_SO_NAME = "libunwind.so";
static const std::string LIB_UNWIND_Z_SO_NAME = "libunwind.z.so";
static const int MAX_STACK_SIZE = 32;
static const int LOG_BUF_LEN = 1024;

using UnwBackTraceFunc = int (*)(void**, int);

static std::map<void *, Dl_info> stackInfoCache;

RWLock rwMutex;

bool GetPcs(size_t &size, void** pcs)
{
#if defined(ENABLE_BACKTRACE_LOCAL)
    size = BacktraceHybrid(pcs, MAX_STACK_SIZE);
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
    size = unwBackTrace(pcs, MAX_STACK_SIZE);
#endif
    return true;
}

bool FindStackInfoCache(void* pc, Dl_info &info)
{
    ReadLockHolder lock(rwMutex);
    auto iter = stackInfoCache.find(pc);
    if (iter != stackInfoCache.end()) {
        info = iter->second;
        return true;
    } else {
        return false;
    }
}

void EmplaceStackInfoCache(void *pc, const Dl_info &info)
{
    WriteLockHolder lock(rwMutex);
    stackInfoCache.emplace(pc, info);
}

void Backtrace(std::ostringstream &stack, bool enableCache)
{
    void *pcs[MAX_STACK_SIZE] = { nullptr };
    size_t stackSize = 0;
    if (!GetPcs(stackSize, pcs)) {
        return;
    }
    stack << "=====================Backtrace========================";
#if defined(ENABLE_BACKTRACE_LOCAL)
    size_t i = 0;
#else
    size_t i = 1;
#endif
    for (; i < stackSize; i++) {
        Dl_info info;
        if (!FindStackInfoCache(pcs[i], info)) {
            if (!dladdr(pcs[i], &info)) {
                break;
            }
            if (enableCache) {
                EmplaceStackInfoCache(pcs[i], info);
            }
        }
        const char *file = info.dli_fname ? info.dli_fname : "";
        uint64_t offset = info.dli_fbase ? ToUintPtr(pcs[i]) - ToUintPtr(info.dli_fbase) : 0;
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

#if defined(ENABLE_BACKTRACE_LOCAL)
OHOS::HiviewDFX::FpBacktrace *FpBacktrace()
{
    static auto fp = OHOS::HiviewDFX::FpBacktrace::CreateInstance();
    return fp;
}
#endif

std::string SymbolicAddress(const void* const *pc,
                            int size,
                            const EcmaVM *vm)
{
    std::string stack;
#if defined(ENABLE_BACKTRACE_LOCAL)
    std::vector<OHOS::HiviewDFX::DfxFrame> frames;
    int index = 0;
    for (int i = 0; i < size; i++) {
        auto dfx_frame = FpBacktrace()->SymbolicAddress(const_cast<void *>(pc[i]));
        if (dfx_frame == nullptr) {
            continue;
        }
        dfx_frame->index = index++;
        if (dfx_frame->isJsFrame && vm != nullptr) {
            auto cb = vm->GetSourceMapTranslateCallback();
            if (cb != nullptr) {
                cb(dfx_frame->mapName, dfx_frame->line, dfx_frame->column, dfx_frame->packageName);
            }
        }
        frames.push_back(*dfx_frame);
    }
    stack = OHOS::HiviewDFX::DfxFrameFormatter::GetFramesStr(frames);
#endif
    return stack;
}

__attribute__((optnone)) int BacktraceHybrid(void** pcArray, uint32_t maxSize)
{
    uint32_t size = 0;
#if defined(ENABLE_BACKTRACE_LOCAL)
    size = FpBacktrace()->BacktraceFromFp(__builtin_frame_address(0), pcArray, maxSize);
#endif
    return static_cast<int>(size);
}

void UpdateStubFileRange(uint64_t stubFileStartAddr, uint64_t stubFileSize)
{
#if defined(ENABLE_BACKTRACE_LOCAL)
    OHOS::HiviewDFX::FpBacktrace::UpdateArkStackRange(stubFileStartAddr, stubFileStartAddr + stubFileSize);
#endif
}
} // namespace panda::ecmascript
