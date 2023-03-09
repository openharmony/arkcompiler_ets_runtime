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

namespace panda::ecmascript {
Taskpool *Taskpool::GetCurrentTaskpool()
{
    static Taskpool *taskpool = new Taskpool();
    return taskpool;
}

void Taskpool::Initialize(int threadNum)
{
    os::memory::LockHolder lock(mutex_);
    if (isInitialized_++ <= 0) {
        runner_ = std::make_unique<Runner>(TheMostSuitableThreadNum(threadNum));
    }
}

void Taskpool::Destroy(int32_t id)
{
    os::memory::LockHolder lock(mutex_);
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

void Taskpool::TerminateTask(int32_t id, TaskType type)
{
    {
        os::memory::LockHolder lock(mutex_);
        if (isInitialized_ <= 0) {
            return;
        }
    }
    runner_->TerminateTask(id, type);
}

uint32_t Taskpool::TheMostSuitableThreadNum(uint32_t threadNum) const
{
    if (threadNum > 0) {
        return std::min<uint32_t>(threadNum, MAX_TASKPOOL_THREAD_NUM);
    }
    uint32_t numOfThreads = std::min<uint32_t>(NumberOfCpuCore() / 2, MAX_TASKPOOL_THREAD_NUM);
    return std::max<uint32_t>(numOfThreads, MIN_TASKPOOL_THREAD_NUM);
}
}  // namespace panda::ecmascript
