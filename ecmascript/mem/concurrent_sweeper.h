/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_CONCURRENT_SWEEPER_H
#define ECMASCRIPT_MEM_CONCURRENT_SWEEPER_H

#include <array>
#include <atomic>

#include "ecmascript/mem/space.h"
#include "ecmascript/taskpool/task.h"

#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
// CONFIG_DISABLE means concurrent sweeper is disabled by options or macros and cannot be changed.
// REQUEST_DISABLE means we want to disable concurrent sweeper while it is sweeping.
// REQUEST_DISABLE can be ragarded as enable and will be changed into disable after finishing sweeping.
enum class EnableConcurrentSweepType : uint8_t {
    ENABLE,
    CONFIG_DISABLE,
    DISABLE,
    REQUEST_DISABLE
};

class ConcurrentSweeper {
public:
    ConcurrentSweeper(Heap *heap, EnableConcurrentSweepType type);
    ~ConcurrentSweeper() = default;

    NO_COPY_SEMANTIC(ConcurrentSweeper);
    NO_MOVE_SEMANTIC(ConcurrentSweeper);

    void PostTask(bool fullGC = false);
    void Sweep(bool fullGC = false);

    void WaitAllTaskFinished();
    // Help to finish sweeping task. It can be called through js thread
    void EnsureAllTaskFinished();
    // Ensure task finish. It can be called through js thread
    void EnsureTaskFinished(MemSpaceType type);
    void EnsureTaskFinishedNoCheck(MemSpaceType type);

    void TryFillSweptRegion();
    void ClearRSetInRange(Region *current, uintptr_t freeStart, uintptr_t freeEnd);

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
        SweeperTask(int32_t id, ConcurrentSweeper *sweeper, MemSpaceType type)
            : Task(id), sweeper_(sweeper), type_(type) {};
        ~SweeperTask() override = default;
        bool Run(uint32_t threadIndex) override;

        NO_COPY_SEMANTIC(SweeperTask);
        NO_MOVE_SEMANTIC(SweeperTask);

    private:
        ConcurrentSweeper *sweeper_;
        MemSpaceType type_;
    };

    void AsyncSweepSpace(MemSpaceType type, bool isMain);

    void WaitingTaskFinish(MemSpaceType type);

    std::array<Mutex, FREE_LIST_NUM> mutexs_;
    std::array<ConditionVariable, FREE_LIST_NUM> cvs_;
    std::array<std::atomic_int, FREE_LIST_NUM> remainingTaskNum_ = {0, 0, 0};

    Heap *heap_;
    EnableConcurrentSweepType enableType_ {EnableConcurrentSweepType::CONFIG_DISABLE};
    bool isSweeping_ {false};
    MemSpaceType startSpaceType_ = MemSpaceType::OLD_SPACE;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_CONCURRENT_SWEEPER_H
