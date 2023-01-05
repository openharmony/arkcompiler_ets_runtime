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
#include "libpandabase/os/mutex.h"

namespace panda::ecmascript {
class Taskpool {
public:
    PUBLIC_API static Taskpool *GetCurrentTaskpool();

    Taskpool() = default;
    PUBLIC_API ~Taskpool()
    {
        os::memory::LockHolder lock(mutex_);
        runner_->TerminateThread();
        isInitialized_ = 0;
    }

    NO_COPY_SEMANTIC(Taskpool);
    NO_MOVE_SEMANTIC(Taskpool);

    void Initialize(int threadNum = DEFAULT_TASKPOOL_THREAD_NUM);
    void Destroy(int32_t id);

    void PostTask(std::unique_ptr<Task> task) const
    {
        ASSERT(isInitialized_ > 0);
        runner_->PostTask(std::move(task));
    }

    // Terminate a task of a specified type
    void TerminateTask(int32_t id, TaskType type = TaskType::ALL);

    uint32_t GetTotalThreadNum() const
    {
        return runner_->GetTotalThreadNum();
    }

    bool IsInThreadPool(std::thread::id id) const
    {
        return runner_->IsInThreadPool(id);
    }

private:
    uint32_t TheMostSuitableThreadNum(uint32_t threadNum) const;

    std::unique_ptr<Runner> runner_;
    int isInitialized_ = 0;
    os::memory::Mutex mutex_;
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PALTFORM_PLATFORM_H
