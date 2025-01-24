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

#ifndef ECMASCRIPT_TASKPOOL_TASK_H
#define ECMASCRIPT_TASKPOOL_TASK_H

#include "libpandabase/macros.h"

namespace panda::ecmascript {
enum class TaskType : uint8_t {
    PGO_SAVE_TASK,
    PGO_RESET_OUT_PATH_TASK,
    PGO_DUMP_TASK,
    ALL,
};

static constexpr int32_t ALL_TASK_ID = -1;
// Tasks not managed by VM
static constexpr int32_t GLOBAL_TASK_ID = 0;

class Task {
public:
    explicit Task(int32_t id) : id_(id) {};
    virtual ~Task() = default;
    virtual bool Run(uint32_t threadIndex) = 0;

    NO_COPY_SEMANTIC(Task);
    NO_MOVE_SEMANTIC(Task);

    virtual TaskType GetTaskType() const
    {
        return TaskType::ALL;
    }

    int32_t GetId() const
    {
        return id_;
    }

    void Terminated()
    {
        terminate_ = true;
    }

    bool IsTerminate() const
    {
        return terminate_;
    }

private:
    int32_t id_ {0};
    volatile bool terminate_ {false};
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_TASKPOOL_TASK_H
