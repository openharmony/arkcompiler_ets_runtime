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

#include "ecmascript/daemon/daemon_task-inl.h"
#include "ecmascript/daemon/daemon_thread.h"
#include "ecmascript/runtime.h"

#ifdef ENABLE_QOS
#include "qos.h"
#endif

namespace panda::ecmascript {

DaemonThread *DaemonThread::instance_ = nullptr;

void DaemonThread::CreateNewInstance()
{
    ASSERT(instance_ == nullptr);
    instance_ = new DaemonThread();
    instance_->StartRunning();
}

DaemonThread *DaemonThread::GetInstance()
{
    ASSERT(instance_ != nullptr);
    return instance_;
}

void DaemonThread::DestroyInstance()
{
    ASSERT(instance_ != nullptr);
    instance_->Stop();
    delete instance_;
    instance_ = nullptr;
}

void DaemonThread::StartRunning()
{
    ASSERT(thread_ == nullptr);
    ASSERT(!IsRunning());
    ASSERT(HasRequestedTermination());
    ASSERT(tasks_.empty());
    ASSERT(GetThreadId() == 0);
    terminationRequested_ = false;
    thread_ = std::make_unique<std::thread>([this] {this->Run();});
    Taskpool::GetCurrentTaskpool()->Initialize();
}

void DaemonThread::Stop()
{
    [[maybe_unused]] bool res = CheckAndPostTask(TerminateDaemonTask(nullptr));
    ASSERT(res);
    thread_->join();
    thread_.reset();
    Taskpool::GetCurrentTaskpool()->Destroy(GetThreadId());
    ASSERT(!IsInRunningState());
    ASSERT(!IsRunning());
    ASSERT(thread_ == nullptr);
    ASSERT(tasks_.empty());
    ResetThreadId();
}

bool DaemonThread::IsRunning() const
{
    return running_;
}

void DaemonThread::Terminate()
{
    ASSERT(JSThread::GetCurrent() == this);
    running_ = false;
}

bool DaemonThread::HasRequestedTermination() const
{
    return terminationRequested_;
}

void DaemonThread::RequestTermination()
{
    ASSERT(!terminationRequested_);
    terminationRequested_ = true;
}

bool DaemonThread::CheckAndPostTask(DaemonTask task)
{
    LockHolder holder(mtx_);
    if (UNLIKELY(HasRequestedTermination())) {
        LOG_GC(FATAL) << "Try to post task to daemon thread when termination has been requested, taskType = "
                      << static_cast<uint32_t>(task.GetTaskType());
        UNREACHABLE();
    }
    if (AddTaskGroup(task.GetTaskGroup())) {
        task.PostTaskPrologue();
        tasks_.emplace_back(task);
        cv_.Signal();
        return true;
    }
    return false;
}

void DaemonThread::Run()
{
    ASSERT(!IsRunning());
    running_ = true;
    os::thread::native_handle_type thread = os::thread::GetNativeHandle();
    os::thread::SetThreadName(thread, "OS_GC_Thread");
    ASSERT(JSThread::GetCurrent() == nullptr);
    RegisterThread(this);
    SetThreadId();
    ASSERT(JSThread::GetCurrent() == this);
#ifdef ENABLE_QOS
    OHOS::QOS::SetThreadQos(OHOS::QOS::QosLevel::QOS_USER_INITIATED);
#endif
    while (IsRunning()) {
        ASSERT(!IsInRunningState());
        DaemonTask task = PopTask();
        runningGroup_ = task.GetTaskGroup();
        task.Run();
        ASSERT(runningGroup_ == DaemonTaskGroup::NONE);
    }
    ASSERT(postedGroups_ == 0);
    ASSERT(tasks_.empty());
    UnregisterThread(this);
}

bool DaemonThread::AddTaskGroup(DaemonTaskGroup taskGroup)
{
    if ((postedGroups_ & static_cast<uint32_t>(taskGroup)) != 0) {
        return false;
    }
    postedGroups_ |= static_cast<uint32_t>(taskGroup);
    return true;
}

void DaemonThread::FinishRunningTask()
{
    ASSERT(runningGroup_ != DaemonTaskGroup::NONE);
    ASSERT((postedGroups_ & static_cast<uint32_t>(runningGroup_)) != 0);
    // Update to postedGroups_ is in DaemeanSuspendAll, and protected by the Runtime::mutatorLock_,
    // so do not need lock; the runningGroup_ is only used in daemon thread, so do not need lock too.
    postedGroups_ &= ~static_cast<uint32_t>(runningGroup_);
    runningGroup_ = DaemonTaskGroup::NONE;
}

DaemonTask DaemonThread::PopTask()
{
    LockHolder holder(mtx_);
    while (true) {
        if (!tasks_.empty()) {
            DaemonTask task = tasks_.front();
            tasks_.pop_front();
            return task;
        }
        cv_.Wait(&mtx_);
    }
}

void DaemonThread::SetSharedMarkStatus(SharedMarkStatus markStatus)
{
    ASSERT(os::thread::GetCurrentThreadId() == GetThreadId());
    markStatus_.store(markStatus, std::memory_order_release);
    Runtime::GetInstance()->GCIterateThreadList([&](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        thread->SetSharedMarkStatus(markStatus);
    });
}

#ifndef NDEBUG
MutatorLock::MutatorLockState DaemonThread::GetMutatorLockState() const
{
    return mutatorLockState_;
}

void DaemonThread::SetMutatorLockState(MutatorLock::MutatorLockState newState)
{
    mutatorLockState_ = newState;
}
#endif
}  // namespace panda::ecmascript