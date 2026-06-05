/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/platform/debug_signal.h"

#include <csignal>
#include <ctime>
#include <unistd.h>

#if defined(PANDA_TARGET_ARM64)
#include <asm/unistd.h>
#else
#include <sys/syscall.h>
#endif

#include "ecmascript/log_wrapper.h"

struct debug_msg {
    uint64_t timestamp;
    const char *msg;
};
using DebugMsg = debug_msg;

namespace panda::ecmascript {
#if defined(PANDA_TARGET_ARM64)
static long Syscall(unsigned long n, unsigned long a, unsigned long b, unsigned long c, unsigned long d,
                    unsigned long e, unsigned long f)
{
    register unsigned long x8 asm("x8") = n;
    register unsigned long x0 asm("x0") = a;
    register unsigned long x1 asm("x1") = b;
    register unsigned long x2 asm("x2") = c;
    register unsigned long x3 asm("x3") = d;
    register unsigned long x4 asm("x4") = e;
    register unsigned long x5 asm("x5") = f;
    asm volatile("svc 0" : "=r"(x0) : "r"(x8), "0"(x0), "r"(x1), "r"(x2), "r"(x3), "r"(x4), "r"(x5) : "memory", "cc");
    return x0;
}
#endif

void NotifyMultiThreadError(const std::string &msg)
{
    static constexpr int NUMBER_ONE_THOUSAND = 1000;
    static constexpr int NUMBER_ONE_MILLION = 1000000;

    struct timespec ts;
    (void)clock_gettime(CLOCK_REALTIME, &ts);
    DebugMsg debugMessage = {0, NULL};
    debugMessage.timestamp = ((uint64_t)ts.tv_sec * NUMBER_ONE_THOUSAND) +
        (((uint64_t)ts.tv_nsec) / NUMBER_ONE_MILLION);
    debugMessage.msg = msg.c_str();

    static constexpr int SIGNO = 42;
    static constexpr int SI_CODE = -4;
    siginfo_t info;
    info.si_code = SI_CODE;
    info.si_value.sival_ptr = &debugMessage;

#if defined(PANDA_TARGET_ARM64)
    pid_t tid = static_cast<pid_t>(Syscall(__NR_gettid, 0, 0, 0, 0, 0, 0));
    if (Syscall(__NR_rt_tgsigqueueinfo, static_cast<unsigned long>(getpid()),
                static_cast<unsigned long>(tid), static_cast<unsigned long>(SIGNO),
                reinterpret_cast<unsigned long>(&info), 0, 0) == -1) {
#else
    if (syscall(SYS_rt_tgsigqueueinfo, getpid(), syscall(SYS_gettid), SIGNO, &info) == -1) {
#endif
        LOG_FULL(ERROR) << msg;
    }
}
} // namespace panda::ecmascript
