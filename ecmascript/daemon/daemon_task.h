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

#ifndef ECMASCRIPT_DAEMON_TASK_H
#define ECMASCRIPT_DAEMON_TASK_H

#include "ecmascript/common.h"

namespace panda::ecmascript {
class JSThread;

enum class SharedMarkStatus : uint8_t {
    READY_TO_CONCURRENT_MARK,
    CONCURRENT_MARKING_OR_FINISHED,
};

enum class DaemonTaskType : uint32_t {
    TRIGGER_CONCURRENT_MARK,
    TRIGGER_COLLECT_GARBAGE,
    TRIGGER_UNIFIED_GC_MARK,
    TERMINATE_DAEMON,
};

/**
 * Tasks in the same TaskGroup do not need to be posted repeatedly,
 * e.g. if a TRIGGER_CONCURRENT_MARK is posted, TRIGGER_COLLECT_GARBAGE is not needed.
*/
enum class DaemonTaskGroup : uint32_t {
    NONE = 0,
    GC_GROUP = 1 << 0,
    TERMINATE_GROUP = 1 << 1,
};

using TaskRunner = void(*)();

class DaemonTask {
public:
    explicit DaemonTask(JSThread *thread, DaemonTaskType taskType, DaemonTaskGroup taskGroup, TaskRunner runner)
        : thread_(thread), taskType_(taskType), taskGroup_(taskGroup), runner_(runner) {}
    ~DaemonTask() = default;

    JSThread *GetJSThread() const
    {
        return thread_;
    }

    DaemonTaskType GetTaskType() const
    {
        return taskType_;
    }

    DaemonTaskGroup GetTaskGroup() const
    {
        return taskGroup_;
    }

    void Run()
    {
        runner_();
    }

private:
    JSThread *thread_;
    DaemonTaskType taskType_;
    DaemonTaskGroup taskGroup_;
    TaskRunner runner_;
};

template<TriggerGCType gcType, MarkReason markReason>
class TriggerConcurrentMarkTask : public DaemonTask {
public:
    explicit TriggerConcurrentMarkTask(JSThread *thread);
};

template<TriggerGCType gcType, GCReason gcReason>
class TriggerCollectGarbageTask : public DaemonTask {
public:
    explicit TriggerCollectGarbageTask(JSThread *thread);
};

template<TriggerGCType gcType, GCReason gcReason>
class TriggerUnifiedGCMarkTask : public DaemonTask {
public:
    explicit TriggerUnifiedGCMarkTask(JSThread *thread);
};

class TerminateDaemonTask : public DaemonTask {
public:
    explicit TerminateDaemonTask(JSThread *thread);
};
}  // namespace panda::ecmascript
#endif //ECMASCRIPT_DAEMON_TASK_H