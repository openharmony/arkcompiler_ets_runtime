/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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
#ifndef ECMASCRIPT_CROSS_VM_DAEMON_TASK_HYBRID_H
#define ECMASCRIPT_CROSS_VM_DAEMON_TASK_HYBRID_H

#include "ecmascript/daemon/daemon_task.h"
#include "ecmascript/daemon/daemon_thread.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {

template<TriggerGCType gcType, GCReason gcReason>
void TriggerUnifiedGCMarkTaskRunner()
{
    SharedHeap::GetInstance()->StartUnifiedGCMark(gcType, gcReason);
}

template<TriggerGCType gcType, GCReason gcReason>
TriggerUnifiedGCMarkTask<gcType, gcReason>::TriggerUnifiedGCMarkTask(JSThread *thread)
    : DaemonTask(thread, DaemonTaskType::TRIGGER_UNIFIED_GC_MARK, DaemonTaskGroup::GC_GROUP,
                 &TriggerUnifiedGCMarkTaskRunner<gcType, gcReason>) {}

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_CROSS_VM_DAEMON_TASK_HYBRID_H
