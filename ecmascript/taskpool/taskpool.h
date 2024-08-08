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

#ifndef ECMASCRIPT_TASKPOOL_TASKPOOL_H
#define ECMASCRIPT_TASKPOOL_TASKPOOL_H

#include <memory>

#include "ecmascript/common.h"
#include "ecmascript/taskpool/runner.h"
#include "ecmascript/platform/mutex.h"
#include "ecmascript/daemon/daemon_thread.h"

namespace panda::ecmascript {
class PUBLIC_API Taskpool {
public:
    PUBLIC_API static Taskpool *GetCurrentTaskpool();

    Taskpool() = default;
    virtual ~Taskpool() = default;

    NO_COPY_SEMANTIC(Taskpool);
    NO_MOVE_SEMANTIC(Taskpool);

    virtual void Initialize(int32_t threadNum = DEFAULT_TASKPOOL_THREAD_NUM) = 0;

    virtual void PostTask(std::unique_ptr<Task> task) = 0;

    virtual void Destroy(int32_t id) = 0;

    // Terminate a task of a specified type
    virtual void TerminateTask(int32_t id, TaskType type = TaskType::ALL) = 0;

    virtual uint32_t GetTotalThreadNum() const = 0;

private:
    virtual uint32_t TheMostSuitableThreadNum(uint32_t threadNum) const = 0;
};

class ThreadedTaskpool : public Taskpool {
public:
    ThreadedTaskpool() = default;
    ~ThreadedTaskpool()
    {
        LockHolder lock(mutex_);
        runner_->TerminateThread();
        isInitialized_ = 0;
    }

    NO_COPY_SEMANTIC(ThreadedTaskpool);
    NO_MOVE_SEMANTIC(ThreadedTaskpool);

    void Initialize(int32_t threadNum = DEFAULT_TASKPOOL_THREAD_NUM) override
    {
        InitializeWithHooks(threadNum, nullptr, nullptr);
    }

    void Destroy(int32_t id) override;

    void PostTask(std::unique_ptr<Task> task) override
    {
        if (isInitialized_ > 0) {
            runner_->PostTask(std::move(task));
        }
    }

    // Terminate a task of a specified type
    void TerminateTask(int32_t id, TaskType type = TaskType::ALL) override;

    uint32_t GetTotalThreadNum() const override
    {
        return runner_->GetTotalThreadNum();
    }

    void SetThreadPriority(PriorityMode mode)
    {
        runner_->SetQosPriority(mode);
    }

    bool IsInThreadPool(std::thread::id id) const
    {
        return runner_->IsInThreadPool(id);
    }

    void ForEachTask(const std::function<void(Task*)> &f);

protected:
    void InitializeWithHooks(int32_t threadNum,
        const std::function<void(os::thread::native_handle_type)> prologueHook,
        const std::function<void(os::thread::native_handle_type)> epilogueHook);

private:
    uint32_t TheMostSuitableThreadNum(uint32_t threadNum) const override;

    std::unique_ptr<Runner> runner_;
    volatile int isInitialized_ = 0;
    Mutex mutex_;
};

class GCWorkerPool : public ThreadedTaskpool {
public:
    PUBLIC_API static GCWorkerPool *GetCurrentTaskpool();

    GCWorkerPool() = default;
    ~GCWorkerPool() = default;

    NO_COPY_SEMANTIC(GCWorkerPool);
    NO_MOVE_SEMANTIC(GCWorkerPool);

    bool IsDaemonThreadOrInThreadPool(std::thread::id id) const
    {
        DaemonThread *dThread = DaemonThread::GetInstance();
        return IsInThreadPool(id) || (dThread != nullptr
            && dThread->GetThreadId() == JSThread::GetCurrentThreadId());
    }
};

#if defined(ENABLE_FFRT_INTERFACES)
class FFRTTaskpool : public Taskpool {
public:
    FFRTTaskpool() = default;
    ~FFRTTaskpool() = default;

    NO_COPY_SEMANTIC(FFRTTaskpool);
    NO_MOVE_SEMANTIC(FFRTTaskpool);

    void Initialize(int32_t threadNum = DEFAULT_TASKPOOL_THREAD_NUM) override
    {
        totalThreadNum_ = TheMostSuitableThreadNum(threadNum);
    }

    void Destroy(int32_t id) override;

    uint32_t GetTotalThreadNum() const override
    {
        return totalThreadNum_;
    }

    void PostTask(std::unique_ptr<Task> task) override;

    void TerminateTask(int32_t id, TaskType type = TaskType::ALL) override;

private:
    uint32_t TheMostSuitableThreadNum(uint32_t threadNum) const override;

    Mutex mutex_;
    std::unordered_map<std::shared_ptr<Task>, ffrt::task_handle> cancellableTasks_ {};
    std::atomic<uint32_t> totalThreadNum_ {0};
};
#endif
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PALTFORM_PLATFORM_H
