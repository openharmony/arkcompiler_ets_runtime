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

#if defined(PANDA_TARGET_ARM64) && defined(PANDA_TARGET_OHOS)
#include <asm/unistd.h>
#else
#include <sys/syscall.h>
#endif

#ifndef SYS_gettid
#ifdef __NR_gettid
#define SYS_gettid __NR_gettid
#elif defined(__aarch64__)
#define SYS_gettid 178
#elif defined(__arm__)
#define SYS_gettid 224
#elif defined(__x86_64__)
#define SYS_gettid 186
#elif defined(__i386__)
#define SYS_gettid 224
#endif
#endif

#ifndef SYS_rt_tgsigqueueinfo
#if defined(__aarch64__)
#define SYS_rt_tgsigqueueinfo 240
#elif defined(__arm__)
#define SYS_rt_tgsigqueueinfo 363
#elif defined(__x86_64__)
#define SYS_rt_tgsigqueueinfo 335
#elif defined(__i386__)
#define SYS_rt_tgsigqueueinfo 332
#endif
#endif

#include "../../third_party/musl/include/info/linux/fatal_message.h"
#include "ecmascript/log_wrapper.h"

namespace panda::ecmascript {
#if defined(PANDA_TARGET_ARM64) && defined(PANDA_TARGET_OHOS)
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
    debug_msg_t debug_message = {0, NULL};
    debug_message.timestamp = ((uint64_t)ts.tv_sec * NUMBER_ONE_THOUSAND) +
        (((uint64_t)ts.tv_nsec) / NUMBER_ONE_MILLION);
    debug_message.msg = msg.c_str();

    // When signo = 42, use si_code = 4 mark the event as ArkTS_ENV_SAN
    static constexpr int SIGNO = 42;
    static constexpr int SI_CODE = -4;
    siginfo_t info;
    info.si_code = SI_CODE;
    info.si_value.sival_ptr = &debug_message;

#if defined(PANDA_TARGET_ARM64) && defined(PANDA_TARGET_OHOS)
    pid_t tid = static_cast<pid_t>(Syscall(SYS_gettid, 0, 0, 0, 0, 0, 0));
    if (Syscall(SYS_rt_tgsigqueueinfo, static_cast<unsigned long>(getpid()),
                static_cast<unsigned long>(tid), static_cast<unsigned long>(SIGNO),
                reinterpret_cast<unsigned long>(&info), 0, 0) == -1) {
#else
    if (syscall(SYS_rt_tgsigqueueinfo, getpid(), syscall(SYS_gettid), SIGNO, &info) == -1) {
#endif
        LOG_FULL(ERROR) << msg;
    }
}
} // namespace panda::ecmascript
