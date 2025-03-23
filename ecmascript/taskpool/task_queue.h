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

#ifndef ECMASCRIPT_TASKPOOL_TASK_QUEUE_H
#define ECMASCRIPT_TASKPOOL_TASK_QUEUE_H

#include <algorithm>
#include <atomic>
#include <deque>
#include <map>
#include <chrono>
#include <memory>
#include <functional>

#include "ecmascript/taskpool/task.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {
using SteadyTimePoint = std::chrono::steady_clock::time_point;
class TaskQueue {
public:
    TaskQueue() = default;
    ~TaskQueue() = default;

    NO_COPY_SEMANTIC(TaskQueue);
    NO_MOVE_SEMANTIC(TaskQueue);

    void PostTask(std::unique_ptr<Task> task);
    void PostDelayedTask(std::unique_ptr<Task> task, uint64_t delayMilliseconds);
    std::unique_ptr<Task> PopTask();

    void Terminate();
    void TerminateTask(int32_t id, TaskType type);
    void ForEachTask(const std::function<void(Task*)> &f);

private:
    void MoveExpiredTask();
    void WaitForTask();

    std::deque<std::unique_ptr<Task>> tasks_;

    struct DelayedTaskCompare {
        bool operator()(const SteadyTimePoint& left, const SteadyTimePoint& right) const
        {
            return (std::chrono::duration_cast<std::chrono::duration<double>>(right - left)).count() > 0;
        }
    };

    std::multimap<SteadyTimePoint, std::unique_ptr<Task>, DelayedTaskCompare> delayedTasks_;

    std::atomic_bool terminate_ = false;
    Mutex mtx_;
    ConditionVariable cv_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TASKPOOL_TASK_QUEUE_H
