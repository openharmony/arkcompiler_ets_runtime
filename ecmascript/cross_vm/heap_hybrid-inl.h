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
#ifndef ECMASCRIPT_CROSS_VM_HEAP_HYBRID_INL_H
#define ECMASCRIPT_CROSS_VM_HEAP_HYBRID_INL_H

#include "ecmascript/cross_vm/heap_hybrid.h"

#include "ecmascript/cross_vm/daemon_task_hybrid-inl.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {
    template<TriggerGCType gcType, GCReason gcReason>
    bool SharedHeap::TriggerUnifiedGCMark(JSThread *thread) const
    {
        ASSERT(gcType == TriggerGCType::UNIFIED_GC && gcReason == GCReason::CROSSREF_CAUSE);
        return DaemonThread::GetInstance()->CheckAndPostTask(TriggerUnifiedGCMarkTask<gcType, gcReason>(thread));
    }
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_CROSS_VM_HEAP_HYBRID_INL_H
