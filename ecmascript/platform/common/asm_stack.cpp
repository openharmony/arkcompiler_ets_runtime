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

#include <sys/resource.h>
#include "ecmascript/platform/asm_stack.h"
#include "ecmascript/js_thread.h"

namespace panda::ecmascript {

size_t GetAsmStackLimit()
{
    // js stack limit
    uintptr_t currentStackPos = JSThread::GetCurrentStackPosition();
    size_t defaultStackSize = EcmaParamConfiguration::GetDefalutStackSize();
    size_t result = currentStackPos > defaultStackSize ? currentStackPos - defaultStackSize : 0;
    int ret = -1;
    void *stackAddr = nullptr;
    size_t size = 0;
#if defined(ENABLE_FFRT_INTERFACES)
    if (!ffrt_get_current_coroutine_stack(&stackAddr, &size)) {
        pthread_attr_t attr;
        ret = pthread_getattr_np(pthread_self(), &attr);
        if (ret != 0) {
            LOG_ECMA(ERROR) << "Get current thread attr failed";
            return result;
        }
        ret = pthread_attr_getstack(&attr, &stackAddr, &size);
        if (pthread_attr_destroy(&attr) != 0) {
            LOG_ECMA(ERROR) << "Destroy current thread attr failed";
        }
        if (ret != 0) {
            LOG_ECMA(ERROR) << "Get current thread stack size failed";
            return result;
        }
    }
#else
    pthread_attr_t attr;
    ret = pthread_getattr_np(pthread_self(), &attr);
    if (ret != 0) {
        LOG_ECMA(ERROR) << "Get current thread attr failed";
        return result;
    }
    ret = pthread_attr_getstack(&attr, &stackAddr, &size);
    if (pthread_attr_destroy(&attr) != 0) {
        LOG_ECMA(ERROR) << "Destroy current thread attr failed";
    }
    if (ret != 0) {
        LOG_ECMA(ERROR) << "Get current thread stack size failed";
        return result;
    }
#endif

    bool isMainThread = IsMainThread();
    uintptr_t threadStackLimit = reinterpret_cast<uintptr_t>(stackAddr);
    uintptr_t threadStackStart = threadStackLimit + size;
    if (isMainThread) {
        struct rlimit rl;
        ret = getrlimit(RLIMIT_STACK, &rl);
        if (ret != 0) {
            LOG_ECMA(ERROR) << "Get current thread stack size failed";
            return result;
        }
        if (rl.rlim_cur > JSThread::DEFAULT_MAX_SYSTEM_STACK_SIZE) {
            LOG_ECMA(ERROR) << "Get current thread stack size exceed " << JSThread::DEFAULT_MAX_SYSTEM_STACK_SIZE
                            << " : " << rl.rlim_cur;
            return result;
        }
        threadStackLimit = threadStackStart - rl.rlim_cur;
    }

    if (result < threadStackLimit) {
        result = threadStackLimit;
    }
    LOG_INTERPRETER(DEBUG) << "Current thread stack start: " << reinterpret_cast<void *>(threadStackStart);
    LOG_INTERPRETER(DEBUG) << "Used stack before js stack start: "
                           << reinterpret_cast<void *>(threadStackStart - currentStackPos);
    LOG_INTERPRETER(DEBUG) << "Current thread asm stack limit: " << reinterpret_cast<void *>(result);
    uintptr_t currentThreadAsmStackLimit = result;
    // To avoid too much times of stack overflow checking, we only check stack overflow before push vregs or
    // parameters of variable length. So we need a reserved size of stack to make sure stack won't be overflowed
    // when push other data.
    result += EcmaParamConfiguration::GetDefaultReservedStackSize();
    if (threadStackStart <= result) {
        LOG_FULL(FATAL) << "Too small stackSize to run jsvm"
                        << ", CurrentStackPosition: " << reinterpret_cast<void *>(currentStackPos)
                        << ", StackAddr: " << stackAddr << ", Size: " << reinterpret_cast<void *>(size)
                        << ", ThreadStackLimit: " << reinterpret_cast<void *>(threadStackLimit)
                        << ", ThreadStackStart: " << reinterpret_cast<void *>(threadStackStart)
                        << ", Used stack before js stack start: "
                        << reinterpret_cast<void *>(threadStackStart - currentStackPos)
                        << ", Current thread asm stack limit: " << reinterpret_cast<void *>(currentThreadAsmStackLimit)
                        << ", Result: " << reinterpret_cast<void *>(result);
    }
    return result;
}

bool IsMainThread()
{
    return getpid() == syscall(SYS_gettid);
}
}