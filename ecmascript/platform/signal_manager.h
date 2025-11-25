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

#ifndef ECMASCRIPT_PLATFORM_SIGNAL_MANAGER_H
#define ECMASCRIPT_PLATFORM_SIGNAL_MANAGER_H

#include <cstddef>
#include <cstdint>
#include <vector>

namespace panda::ecmascript {

// represent a function pointer type for signal handlers
// type would be: bool handler(int signo, siginfo_t *info, void *ucontext);
using SigchainHandler = bool (*)(int, void *, void *);
constexpr size_t SIGCHAIN_ACTION_MAX_SIZE = 256;
constexpr size_t SIGCHAIN_ACTION_ALIGNMENT = 8;
using SigchainAction = struct SigchainAction__ *;
#define DECL_SIGCHAIN_ACTION(name)                                                                                     \
    alignas(SIGCHAIN_ACTION_ALIGNMENT) char name##SigchainInternalBuffer[SIGCHAIN_ACTION_MAX_SIZE]{};                  \
    auto name = reinterpret_cast<SigchainAction>(name##SigchainInternalBuffer)

class SignalManager {
public:
    SignalManager() = default;
    virtual ~SignalManager() = default;
    static void Initialize();
    static SignalManager &GetSignalManager(int signo);
    virtual void AddSpecialHandler(SigchainAction newAction) = 0;
    virtual void RemoveSpecialHandler(SigchainHandler handler) = 0;
    virtual bool IsClaimed() = 0;
    virtual int Claim() = 0;

    static size_t GetSigchainActionSize();
    static bool InitSigchainAction(SigchainAction action, const std::vector<int> &signos, SigchainHandler handler);

protected:
    constexpr static const size_t SPECIAL_HANDLER_SOLT_COUNT = 2;
};
} // namespace panda::ecmascript

#endif // ECMASCRIPT_PLATFORM_SIGNAL_MANAGER_H