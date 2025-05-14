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
#ifndef ARK_COMMON_RUN_TIME_H
#define ARK_COMMON_RUN_TIME_H

#include "common_interfaces/base/common.h"

namespace panda {
// The manager to coordinate other runtime components

class HeapManager;
class LogManager;
class MutatorManager;
#ifdef _WIN64
class WinModuleManager;
#endif

class Runtime {
public:
    // return singleton of runtime instance.
    static inline Runtime& Current()
    {
        CHECK_CC(runtime != nullptr);
        return *runtime;
    }

    static inline Runtime* CurrentRef() { return runtime; }

    inline HeapManager& GetHeapManager()
    {
        CHECK_CC(heapManager_ != nullptr);
        return *heapManager_;
    }

    inline MutatorManager& GetMutatorManager()
    {
        CHECK_CC(mutatorManager_ != nullptr);
        return *mutatorManager_;
    }

#ifdef _WIN64
    inline WinModuleManager& GetWinModuleManager()
    {
        CHECK_CC(winModuleManager != nullptr);
        return *winModuleManager;
    }
#endif

    virtual RuntimeParam GetRuntimeParam() const = 0;
    virtual void SetGCThreshold(uint64_t threshold) = 0;

protected:
    static Runtime* runtime;

    Runtime() = default;
    virtual ~Runtime() = default;

    HeapManager* heapManager_ = nullptr;       // managed heap
    LogManager* logManager_ = nullptr;         // log
    MutatorManager* mutatorManager_ = nullptr; // manage all mutators registered
#ifdef _WIN64
    WinModuleManager* winModuleManager = nullptr; // manage windows module data
#endif
};
} // namespace panda
#endif // ARK_COMMON_RUNTIME_H
