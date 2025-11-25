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

#include <mutex>
#include <csignal>

#define ARK_INTERNAL_GUARD_SIGNAL_MANAGER_UNIX
#include "signal_manager_unix.h"

#include "ecmascript/log_wrapper.h"
#include "ecmascript/platform/signal_manager.h"

#include <securec.h>

namespace panda::ecmascript {
size_t SignalManager::GetSigchainActionSize() { return sizeof(UnixSigchainAction); }

bool SignalManager::InitSigchainAction(
    SigchainAction action, const std::vector<int> &signos, SigchainHandler handler)
{
    if (action == nullptr) {
        return false;
    }
    auto sa = Cast(action);
    if (memset_s(sa, sizeof(UnixSigchainAction), 0, sizeof(UnixSigchainAction)) != EOK) {
        LOG_ECMA(ERROR) << "memset failed, reason: " << strerror(errno);
        return false;
    }
    sigemptyset(&sa->mask_);
    for (auto signo : signos) {
        if (signo <= 0 || signo >= NSIG) {
            continue;
        }
        if (sigaddset(&sa->mask_, signo) != EOK) {
            LOG_ECMA(ERROR) << "sigaddset failed, signo: " << signo;
            return false;
        }
    }
    sa->handler_ = handler;
    return true;
}

volatile uintptr_t &SignalHandlingScope::GetHandlingSignalBits(int idx)
{
    thread_local static volatile uintptr_t signalHandlingKey[SIGNAL_BITS_COUNT]{};
    return signalHandlingKey[idx];
}

bool SignalHandlingScope::IsHandlingSignal()
{
    for (size_t i = 0; i < SIGNAL_BITS_COUNT; i++) {
        if (GetHandlingSignalBits(i) != 0) {
            return true;
        }
    }
    return false;
}

bool SignalHandlingScope::IsHandlingSignal(int signo)
{
    if (signo <= 0 || signo >= NSIG - 1) {
        return false;
    }
    auto bits = GetHandlingSignalBits(signo % MAX_UINTPTR_SIZE);
    uintptr_t mask = static_cast<uintptr_t>(1) << ((signo - 1) % MAX_SIGNAL_NUM); // -1: skip 0, invalid signal
    return (bits & mask) != 0;
}

bool SignalHandlingScope::SetHandlingSignal(int signo, bool value)
{
    if (signo <= 0 || signo >= NSIG - 1) {
        return false;
    }
    auto &bits = GetHandlingSignalBits(signo % MAX_UINTPTR_SIZE);
    const uintptr_t bitMask = static_cast<uintptr_t>(1) << ((signo - 1) % MAX_SIGNAL_NUM); // -1: skip 0, invalid signal
    auto ret = bits & bitMask;
    if (value) {
        bits |= bitMask;
    } else {
        bits &= ~bitMask;
    }
    return ret != 0;
}

} // namespace panda::ecmascript