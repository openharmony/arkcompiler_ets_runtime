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

#include "ecmascript/taskpool/taskpool.h"

#include "ecmascript/platform/os.h"

#if defined(ENABLE_FFRT_INTERFACES)
#include "ffrt_inner.h"
#include "c/executor_task.h"
#endif

namespace panda::ecmascript {
Taskpool *Taskpool::GetCurrentTaskpool()
{
#if defined(ENABLE_FFRT_INTERFACES)
    static Taskpool *taskpool = new FFRTTaskpool();
#else
    static Taskpool *taskpool = new ThreadedTaskpool();
#endif
    return taskpool;
}

void ThreadedTaskpool::InitializeWithHooks(int32_t threadNum,
    const std::function<void(os::thread::native_handle_type)> prologueHook,
    const std::function<void(os::thread::native_handle_type)> epilogueHook)
{
    LockHolder lock(mutex_);
    if (isInitialized_++ <= 0) {
        runner_ = std::make_unique<Runner>(TheMostSuitableThreadNum(threadNum), prologueHook, epilogueHook);
    }
}

void ThreadedTaskpool::Destroy(int32_t id)
{
    ASSERT(id != 0);
    LockHolder lock(mutex_);
    if (isInitialized_ <= 0) {
        return;
    }
    isInitialized_--;
    if (isInitialized_ == 0) {
        runner_->TerminateThread();
    } else {
        runner_->TerminateTask(id, TaskType::ALL);
    }
}

void ThreadedTaskpool::TerminateTask(int32_t id, TaskType type)
{
    if (isInitialized_ <= 0) {
        return;
    }
    runner_->TerminateTask(id, type);
}

uint32_t ThreadedTaskpool::TheMostSuitableThreadNum(uint32_t threadNum) const
{
    if (threadNum > 0) {
        return std::min<uint32_t>(threadNum, MAX_TASKPOOL_THREAD_NUM);
    }
    uint32_t numOfThreads = std::min<uint32_t>(NumberOfCpuCore() / 2, MAX_TASKPOOL_THREAD_NUM);
    if (numOfThreads > MIN_TASKPOOL_THREAD_NUM) {
        return numOfThreads - 1;        // 1 for daemon thread.
    }
    return MIN_TASKPOOL_THREAD_NUM;     // At least MIN_TASKPOOL_THREAD_NUM GC threads, and 1 extra daemon thread.
}

void ThreadedTaskpool::ForEachTask(const std::function<void(Task*)> &f)
{
    if (isInitialized_ <= 0) {
        return;
    }
    runner_->ForEachTask(f);
}

GCWorkerPool *GCWorkerPool::GetCurrentTaskpool()
{
    static GCWorkerPool *taskpool = new GCWorkerPool();
    return taskpool;
}

#if defined(ENABLE_FFRT_INTERFACES)
void FFRTTaskpool::TerminateTask(int32_t id, TaskType type)
{
    LockHolder lock(mutex_);
    for (auto &[task, handler] : cancellableTasks_) {
        if (id != ALL_TASK_ID && id != task->GetId()) {
            continue;
        }
        if (type != TaskType::ALL && type != task->GetTaskType()) {
            continue;
        }

        // Return non-zero if the task is doing or has finished, so calling terminated is meaningless.
        if (ffrt::skip(handler) == 0) {
            task->Terminated();
        }
    }
}

void FFRTTaskpool::Destroy(int32_t id)
{
    TerminateTask(id, TaskType::ALL);
}

uint32_t FFRTTaskpool::TheMostSuitableThreadNum(uint32_t threadNum) const
{
    if (threadNum > 0) {
        return std::min<uint32_t>(threadNum, MAX_TASKPOOL_THREAD_NUM);
    }
    uint32_t numOfThreads = std::min<uint32_t>(NumberOfCpuCore() / 2, MAX_TASKPOOL_THREAD_NUM);
    return std::max<uint32_t>(numOfThreads, MIN_TASKPOOL_THREAD_NUM);
}

void FFRTTaskpool::PostTask(std::unique_ptr<Task> task)
{
    constexpr uint32_t FFRT_TASK_STACK_SIZE = 8 * 1024 * 1024; // 8MB

    ffrt::task_attr taskAttr;
    ffrt_task_attr_init(&taskAttr);
    ffrt_task_attr_set_name(&taskAttr, "Ark_FFRTTaskpool_Task");
    ffrt_task_attr_set_qos(&taskAttr, ffrt_qos_user_initiated);
    ffrt_task_attr_set_stack_size(&taskAttr, FFRT_TASK_STACK_SIZE);

    if (LIKELY(!task->IsCancellable())) {
        auto ffrtTask = [task = task.release()]() {
            task->Run(ffrt::this_task::get_id());
            delete task;
        };
        ffrt::submit(ffrtTask, {}, {}, taskAttr);
    } else {
        std::shared_ptr<Task> sTask(std::move(task));
        auto ffrtTask = [this, sTask]() {
            sTask->Run(ffrt::this_task::get_id());
            LockHolder lock(mutex_);
            cancellableTasks_.erase(sTask);
        };
        LockHolder lock(mutex_);
        ffrt::task_handle handler = ffrt::submit_h(ffrtTask, {}, {}, taskAttr);
        cancellableTasks_.emplace(sTask, std::move(handler));
    }
}
#endif
}  // namespace panda::ecmascript
