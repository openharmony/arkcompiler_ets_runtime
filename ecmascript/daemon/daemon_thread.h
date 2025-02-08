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

#ifndef ECMASCRIPT_DAEMON_THREAD_H
#define ECMASCRIPT_DAEMON_THREAD_H

#include <atomic>
#include <deque>
#include <thread>

#include "ecmascript/daemon/daemon_task.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mutator_lock.h"
#include "ecmascript/platform/mutex.h"

namespace panda::ecmascript {

static constexpr uint32_t DAEMON_THREAD_INDEX = 0;

class DaemonThread : public JSThread {
public:
    static void CreateNewInstance();
    static DaemonThread *GetInstance();
    static void DestroyInstance();

    using ThreadId = uint32_t;

    void StartRunning();

    void Stop();                            // In JS thread, stop daemon thread, and then call `TaskPool.Destroy`

    void Terminate();                       // In daemon thread

    bool HasRequestedTermination() const;

    void RequestTermination();

    bool CheckAndPostTask(DaemonTask task);

    /**
     * Called in daemon thread, and manually call this in DaemonTask at the appropriate time,
     * e.g. for GC task, call this before ResumeAll instead of the task complete ended, to prevent in some
     * time sequence, all js_thread is resumed and one of these post another GC task, but the PostedGroup
     * have not been cleaned, leading the task lost.
    */
    void FinishRunningTask();

    SharedMarkStatus GetSharedMarkStatus() const
    {
        return markStatus_.load(std::memory_order_acquire);
    }

    void SetSharedMarkStatus(SharedMarkStatus markStatus);

    bool IsReadyToConcurrentMark() const
    {
        return GetSharedMarkStatus() == SharedMarkStatus::READY_TO_CONCURRENT_MARK;
    }

    bool IsConcurrentMarkingOrFinished() const
    {
        return GetSharedMarkStatus() == SharedMarkStatus::CONCURRENT_MARKING_OR_FINISHED;
    }

#ifndef NDEBUG
    MutatorLock::MutatorLockState GetMutatorLockState() const;
    void SetMutatorLockState(MutatorLock::MutatorLockState newState);
#endif

private:
    DaemonThread() : JSThread(ThreadType::DAEMON_THREAD) {}
    ~DaemonThread() = default;

    bool IsRunning() const;

    void Run();

    bool AddTaskGroup(DaemonTaskGroup taskGroup);

    DaemonTask PopTask();

    static DaemonThread *instance_;

    // `terminationRequested_` means whether termination has beed requested, and is used in any threads except daemon
    // inside `mtx_`.
    bool terminationRequested_ {true};

    // `running_` is only used in daemon thread, means whether the termination task has been executed,
    // and need to stop running.
    bool running_ {false};

    std::atomic<SharedMarkStatus> markStatus_ {SharedMarkStatus::READY_TO_CONCURRENT_MARK};

    std::unique_ptr<std::thread> thread_;

    std::deque<DaemonTask> tasks_;
    DaemonTaskGroup runningGroup_ {DaemonTaskGroup::NONE};
    uint32_t postedGroups_ {0};
    Mutex mtx_;
    ConditionVariable cv_;
#ifndef NDEBUG
    MutatorLock::MutatorLockState mutatorLockState_ = MutatorLock::MutatorLockState::UNLOCKED;
#endif
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_DAEMON_THREAD_H