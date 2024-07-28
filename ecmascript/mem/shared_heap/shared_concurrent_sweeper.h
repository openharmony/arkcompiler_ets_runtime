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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CONCURRENT_SWEEPER_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CONCURRENT_SWEEPER_H

#include "ecmascript/mem/concurrent_sweeper.h"

namespace panda::ecmascript {
class SharedHeap;

class SharedConcurrentSweeper {
public:
    SharedConcurrentSweeper(SharedHeap *heap, EnableConcurrentSweepType type);
    ~SharedConcurrentSweeper() = default;

    NO_COPY_SEMANTIC(SharedConcurrentSweeper);
    NO_MOVE_SEMANTIC(SharedConcurrentSweeper);

    void PostTask(bool isFullGC);
    void Sweep(bool isFullGC);

    void WaitAllTaskFinished();
    // Help to finish sweeping task. It can be called through js thread
    void EnsureAllTaskFinished();
    // Ensure task finish. It can be called through js thread
    void EnsureTaskFinished(MemSpaceType type);

    void TryFillSweptRegion();

    void EnableConcurrentSweep(EnableConcurrentSweepType type);

    bool IsSweeping()
    {
        return isSweeping_;
    }

    bool ConcurrentSweepEnabled()
    {
        return !IsDisabled();
    }

    void ConfigConcurrentSweep(bool enabled)
    {
        enableType_ = enabled ? EnableConcurrentSweepType::ENABLE :
                      EnableConcurrentSweepType::CONFIG_DISABLE;
    }

    bool IsDisabled() const
    {
        return enableType_ == EnableConcurrentSweepType::DISABLE ||
            enableType_ == EnableConcurrentSweepType::CONFIG_DISABLE;
    }

    bool IsRequestDisabled() const
    {
        return enableType_ == EnableConcurrentSweepType::REQUEST_DISABLE;
    }

    bool IsConfigDisabled() const
    {
        return enableType_ == EnableConcurrentSweepType::CONFIG_DISABLE;
    }
private:
    class SweeperTask : public Task {
    public:
        SweeperTask(int32_t id, SharedConcurrentSweeper *sweeper, MemSpaceType type)
            : Task(id), sweeper_(sweeper), type_(type) {};
        ~SweeperTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(SweeperTask);
        NO_MOVE_SEMANTIC(SweeperTask);

    private:
        SharedConcurrentSweeper *sweeper_;
        MemSpaceType type_;
    };

    void AsyncSweepSpace(MemSpaceType type, bool isMain);

    void WaitingTaskFinish(MemSpaceType type);

    std::array<Mutex, SHARED_SWEEPING_SPACE_NUM> mutexs_;
    std::array<ConditionVariable, SHARED_SWEEPING_SPACE_NUM> cvs_;
    std::array<std::atomic_int, SHARED_SWEEPING_SPACE_NUM> remainingTaskNum_ = {0, 0};

    SharedHeap *sHeap_;
    EnableConcurrentSweepType enableType_ {EnableConcurrentSweepType::CONFIG_DISABLE};
    bool isSweeping_ {false};
    bool isFullGC_ {false};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CONCURRENT_SWEEPER_H
