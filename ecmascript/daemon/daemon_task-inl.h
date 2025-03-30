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

#ifndef ECMASCRIPT_DAEMON_TASK_INL_H
#define ECMASCRIPT_DAEMON_TASK_INL_H

#include "ecmascript/daemon/daemon_task.h"
#include "ecmascript/daemon/daemon_thread.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {

template<TriggerGCType gcType, MarkReason markReason>
void TriggerConcurrentMarkTaskRunner()
{
    SharedHeap::GetInstance()->StartConcurrentMarking(gcType, markReason);
}

template<TriggerGCType gcType, MarkReason markReason>
TriggerConcurrentMarkTask<gcType, markReason>::TriggerConcurrentMarkTask(JSThread *thread)
    : DaemonTask(thread, DaemonTaskType::TRIGGER_CONCURRENT_MARK, DaemonTaskGroup::GC_GROUP,
                 &TriggerConcurrentMarkTaskRunner<gcType, markReason>) {}

template<TriggerGCType gcType, GCReason gcReason>
void TriggerCollectGarbageTaskRunner()
{
    SharedHeap::GetInstance()->DaemonCollectGarbage(gcType, gcReason);
}

template<TriggerGCType gcType, GCReason gcReason>
TriggerCollectGarbageTask<gcType, gcReason>::TriggerCollectGarbageTask(JSThread *thread)
    : DaemonTask(thread, DaemonTaskType::TRIGGER_COLLECT_GARBAGE, DaemonTaskGroup::GC_GROUP,
                 &TriggerCollectGarbageTaskRunner<gcType, gcReason>) {}

template<TriggerGCType gcType, GCReason gcReason>
void TriggerUnifiedGCMarkTaskRunner()
{
    SharedHeap::GetInstance()->StartUnifiedGCMark(gcType, gcReason);
}

template<TriggerGCType gcType, GCReason gcReason>
TriggerUnifiedGCMarkTask<gcType, gcReason>::TriggerUnifiedGCMarkTask(JSThread *thread)
    : DaemonTask(thread, DaemonTaskType::TRIGGER_UNIFIED_GC_MARK, DaemonTaskGroup::GC_GROUP,
                 &TriggerUnifiedGCMarkTaskRunner<gcType, gcReason>) {}

inline void TerminateDaemonTaskRunner()
{
    DaemonThread *thread = DaemonThread::GetInstance();
    thread->MarkTerminate();
    thread->FinishRunningTask();
}

inline TerminateDaemonTask::TerminateDaemonTask(JSThread *thread)
    : DaemonTask(thread, DaemonTaskType::TERMINATE_DAEMON, DaemonTaskGroup::TERMINATE_GROUP,
                 &TerminateDaemonTaskRunner) {}
}  // namespace panda::ecmascript
#endif //ECMASCRIPT_DAEMON_TASK_INL_H