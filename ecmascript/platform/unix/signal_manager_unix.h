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

#ifndef ARK_INTERNAL_GUARD_SIGNAL_MANAGER_UNIX
#error "This file is not public API. Please use signal_manager.h instead."
#endif

#ifndef ECMASCRIPT_PLATFORM_UNIX_SIGNAL_MANAGER_UNIX_H
#define ECMASCRIPT_PLATFORM_UNIX_SIGNAL_MANAGER_UNIX_H

#include "ecmascript/platform/signal_manager.h"

#include <limits>
#include <csignal>

namespace panda::ecmascript {
struct UnixSigchainAction {
    SigchainHandler handler_;
    sigset_t mask_;
};

inline UnixSigchainAction *Cast(SigchainAction action) { return reinterpret_cast<UnixSigchainAction *>(action); }
using UnixSigchainHandler = void (*)(int signo, siginfo_t *info, void *context);
template <typename T> inline constexpr T RoundUp(T x, T n) { return (x + n - 1) & -n; }

class SignalHandlingScope {
public:
    explicit SignalHandlingScope(int signo) : signo_(signo), original_(SetHandlingSignal(signo, true)) {}
    ~SignalHandlingScope() { SetHandlingSignal(signo_, original_); };

    static volatile uintptr_t &GetHandlingSignalBits(int idx);
    static bool IsHandlingSignal();

    static bool IsHandlingSignal(int signo);
    static bool SetHandlingSignal(int signo, bool value);

private:
    static size_t GetSigBitsIndex(int signo);
    static uintptr_t GenSignalBitMask(int signo);

    constexpr static size_t MAX_SIGNAL_NUM = NSIG - 1; // -1: skip invalid signal 0
    constexpr static size_t MAX_UINTPTR_SIZE = std::numeric_limits<uintptr_t>::digits;
    constexpr static size_t SIGNAL_BITS_COUNT = RoundUp(MAX_SIGNAL_NUM, MAX_UINTPTR_SIZE) / MAX_UINTPTR_SIZE;

    int signo_;
    bool original_;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_PLATFORM_UNIX_SIGNAL_MANAGER_UNIX_H
