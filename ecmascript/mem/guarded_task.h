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

#ifndef ECMASCRIPT_MEM_GUARDED_TASK_H
#define ECMASCRIPT_MEM_GUARDED_TASK_H

#include "common_components/taskpool/task.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
class GuardedTask : public common::Task {
public:
    explicit GuardedTask(int32_t id) : Task(id) {}
    virtual ~GuardedTask() = default;
    virtual bool RunInternal(uint32_t threadIndex) = 0;
    virtual bool Scheduable() = 0;

    bool Run(uint32_t threadIndex) override
    {
        if (!Scheduable()) {
            return false;
        }
        return RunInternal(threadIndex);
    }

    NO_COPY_SEMANTIC_CC(GuardedTask);
    NO_MOVE_SEMANTIC_CC(GuardedTask);
};

class EpochGuardedTaskMonitor {
public:
    EpochGuardedTaskMonitor(uint32_t capacity) : capacity_(capacity) {}

    std::pair<bool, uint32_t> TryPostTask()
    {
        LockHolder lock(mutex_);
        if (!FastCheckCanDistributeTask()) {
            return {false, currentEpoch_};
        }
        postTaskCount_++;
        return {true, currentEpoch_};
    }

    // Used to post one task without capacity check.
    uint32_t ForcePostTask()
    {
        LockHolder lock(mutex_);
        postTaskCount_++;
        return currentEpoch_;
    }

    bool TryRunTask(uint32_t taskEpoch)
    {
        LockHolder lock(mutex_);
        if (IsExpired(taskEpoch)) {
            if (--postTaskCount_ == 0) {
                cv_.SignalAll();
            }
            return false;
        }
        runningTaskCount_++;
        return true;
    }

    void NotifyFinish()
    {
        LockHolder lock(mutex_);
        postTaskCount_--;
        if (--runningTaskCount_ == 0) {
            cv_.SignalAll();
        }
    }

    void WaitRunningTaskFinished()
    {
        LockHolder lock(mutex_);
        while (runningTaskCount_ > 0) {
            cv_.Wait(&mutex_);
        }
        currentEpoch_++;
    }

    void WaitAllTaskFinished()
    {
        LockHolder lock(mutex_);
        while (postTaskCount_ > 0) {
            cv_.Wait(&mutex_);
        }
    }

    void UpdateCapacity(uint32_t newCapacity)
    {
        LockHolder lock(mutex_);
        capacity_ = newCapacity;
    }

    uint32_t GetTotalTaskCount()
    {
        LockHolder lock(mutex_);
        return postTaskCount_;
    }

    bool IsExpired(uint32_t taskEpoch) const
    {
        return taskEpoch != currentEpoch_;
    }

    inline bool FastCheckCanDistributeTask()
    {
        return postTaskCount_ < capacity_;
    }

private:
    uint32_t currentEpoch_ {0};
    uint32_t postTaskCount_ {0};
    uint32_t runningTaskCount_ {0};
    uint32_t capacity_ {0};
    ConditionVariable cv_;
    Mutex mutex_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_GUARDED_TASK_H
