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

#include "ecmascript/platform/signal_manager.h"

#include "ecmascript/log_wrapper.h"
#define ARK_INTERNAL_GUARD_SIGNAL_MANAGER_UNIX
#include "ecmascript/platform/unix/signal_manager_unix.h"

#include <algorithm>
#include <cerrno>
#include <csignal>
#include <mutex>
#include <sigchain.h>
#include <vector>

namespace panda::ecmascript {
class OhosSignalManager : public SignalManager {
public:
    explicit OhosSignalManager() = default;
    static OhosSignalManager &GetSignalManager(int signal);

    int Claim() override;
    void AddSpecialHandler(SigchainAction newAction) override;
    void RemoveSpecialHandler(SigchainHandler handler) override;
    bool IsClaimed() override { return claimed_; };

private:
    void Register();
    bool Handler(int signo, siginfo_t *info, void *ucontext);
    static bool SigHandler(int signal, siginfo_t *info, void *ucontext);

    int signo_{0};

    bool claimed_{false};
    UnixSigchainAction specialHandlers_[SPECIAL_HANDLER_SOLT_COUNT];
    static OhosSignalManager instances_[NSIG - 1];
};
OhosSignalManager OhosSignalManager::instances_[NSIG - 1];

void SignalManager::Initialize()
{
    // Trigger early initialization of the signal handling subsystem.
    // On musl libc, this ensures our handler occupies the "special slot"
    // reserved for internal use, preventing potential conflicts later.
    GetSignalManager(SIGHUP);
}

SignalManager &SignalManager::GetSignalManager(int signo) { return OhosSignalManager::GetSignalManager(signo); }

OhosSignalManager &OhosSignalManager::GetSignalManager(int signal)
{
    static std::once_flag flag;
    std::call_once(flag, [] {
        // signo 0 is not valid signal, skip
        for (int i = 1; i < NSIG; i++) {
            instances_[i - 1].signo_ = i;
            // we need to claim all signals on ohos platform
            instances_[i - 1].Claim();
        }
    });
    if (signal <= 0 || signal >= NSIG) {
        // raise abort signal in signal handler would cause unexpected error
        if (!SignalHandlingScope::IsHandlingSignal()) {
            LOG_ECMA(FATAL) << "signal number is out of range";
        }
        static OhosSignalManager dummy{};
        return dummy;
    }
    return instances_[signal - 1];
}

void OhosSignalManager::AddSpecialHandler(SigchainAction action)
{
    for (auto &sa : specialHandlers_) {
        if (sa.handler_ == nullptr) {
            sa = *Cast(action);
            if (!IsClaimed()) {
                Register();
            }
            return;
        }
    }
    LOG_ECMA(FATAL) << "too many special signal handlers";
}

void OhosSignalManager::RemoveSpecialHandler(SigchainHandler handler)
{
    for (size_t i = 0; i < SPECIAL_HANDLER_SOLT_COUNT; i++) {
        if (specialHandlers_[i].handler_ == handler) {
            // move handlers to prev solt
            for (size_t j = i; j < SPECIAL_HANDLER_SOLT_COUNT - 1; j++) {
                specialHandlers_[j] = specialHandlers_[j + 1];
            }
            specialHandlers_[SPECIAL_HANDLER_SOLT_COUNT - 1].handler_ = nullptr;
            return;
        }
    }
    LOG_ECMA(FATAL) << "failed to find special handler to remove";
}

void OhosSignalManager::Register()
{
    claimed_ = true;
    // invalid signal will cause crash in add_special_signal_handler
    if (signo_ <= 0 || signo_ >= NSIG) {
        return;
    }
    struct signal_chain_action sa = {};
    sigfillset(&sa.sca_mask);
    sa.sca_sigaction = OhosSignalManager::SigHandler;
    sa.sca_flags = SA_RESTART | SA_SIGINFO | SA_ONSTACK;
    add_special_signal_handler(signo_, &sa);
}

int OhosSignalManager::Claim()
{
    if (!IsClaimed()) {
        Register();
    }
    return EOK;
}

bool OhosSignalManager::Handler(int signo, siginfo_t *info, void *ucontext)
{
    // protect do not re-enter signal handler
    if (SignalHandlingScope::IsHandlingSignal(signo)) {
        return false;
    }

    // save prev sigset
    [[maybe_unused]] SignalHandlingScope scope(signo);

    // handle special handlers at first
    for (auto &action : specialHandlers_) {
        sigset_t prevMask;
        auto &sa = action;
        if (sa.handler_ == nullptr) {
            break;
        }

        sigprocmask(SIG_SETMASK, &sa.mask_, &prevMask);
        // call special handlers
        if (sa.handler_(signo, info, ucontext)) {
            return true;
        }
        sigprocmask(SIG_SETMASK, &prevMask, nullptr);
    }
    return false;
}

bool OhosSignalManager::SigHandler(int signo, siginfo_t *info, void *ucontext)
{
    return GetSignalManager(signo).Handler(signo, info, ucontext);
};

} // namespace panda::ecmascript