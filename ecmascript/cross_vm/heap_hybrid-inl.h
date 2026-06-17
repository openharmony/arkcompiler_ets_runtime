/*
* Copyright (c) 2025-2026 Huawei Device Co., Ltd.
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
#include "ecmascript/runtime.h"

namespace panda::ecmascript {
    template<TriggerGCType gcType, GCReason gcReason>
    bool SharedHeap::TriggerUnifiedGCMark(JSThread *thread)
    {
        ASSERT(gcType == TriggerGCType::UNIFIED_GC && gcReason == GCReason::CROSSREF_CAUSE);
        LockHolder lock(waitGCFinishedMutex_);
        DaemonThread::PostTaskResult res = DaemonThread::GetInstance()
            ->CheckAndPostTask(TriggerUnifiedGCMarkTask<gcType, gcReason>(thread));
        if (res == DaemonThread::PostTaskResult::SUCCESS) {
            ASSERT(gcFinished_);
            gcFinished_ = false;
        }
        return res == DaemonThread::PostTaskResult::SUCCESS;
    }
#ifdef PANDA_JS_ETS_HYBRID_MODE
    inline bool Heap::GetStsTriggerXGC([[maybe_unused]] const char *callerName)
    {
        auto *runtime = Runtime::GetInstance();
        if (runtime != nullptr) {
            auto *stsIface = runtime->GetSTSVMInterface();
            if (stsIface != nullptr && Runtime::GetInstance()->IsHybridVm()) {
                LOG_GC(INFO) << callerName << " trigger XGC from dynamic side";
                return true;
            }
        }
        LOG_GC(INFO) << callerName << " XGC skipped: STSVMInterface is null";
        return false;
    }

    inline bool Heap::TryTriggerUnifiedGCMark([[maybe_unused]] const char *callerName)
    {
        auto *sharedHeap = SharedHeap::GetInstance();
        bool unifiedGcTriggered =
            sharedHeap->TriggerUnifiedGCMark<TriggerGCType::UNIFIED_GC, GCReason::CROSSREF_CAUSE>(thread_);
        LOG_GC(INFO) << callerName << " XGC trigger result: unified=" << unifiedGcTriggered;
        return unifiedGcTriggered;
    }

    inline bool Heap::TryTriggerXGC([[maybe_unused]] const char *callerName)
    {
        if (TryTriggerUnifiedGCMark(callerName)) {
            bool etsGcTriggered = Runtime::GetInstance()->GetSTSVMInterface()->TriggerXGC();
            if (!etsGcTriggered) {
                SharedHeap::GetInstance()->NotifyUnifiedGCInterrupt();
            }
            SharedHeap::GetInstance()->WaitGCFinished(thread_);
            return etsGcTriggered;
        }
        return false;
    }

    inline bool Heap::TryTriggerXGCAndReclaim(const char *callerName)
    {
        if (GetStsTriggerXGC(callerName)) {
            bool xgcTriggered = TryTriggerXGC(callerName);
            LOG_GC(INFO) << callerName << " XGC trigger result: " << xgcTriggered;
            if (xgcTriggered) {
                CollectGarbage(TriggerGCType::FULL_GC, GCReason::ALLOCATION_FAILED);
            }
            return xgcTriggered;
        }
        return false;
    }
#endif  // PANDA_JS_ETS_HYBRID_MODE
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_CROSS_VM_HEAP_HYBRID_INL_H
