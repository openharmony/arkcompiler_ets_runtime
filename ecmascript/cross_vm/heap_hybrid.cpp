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

#include "ecmascript/cross_vm/unified_gc/unified_gc.h"
#include "ecmascript/cross_vm/unified_gc/unified_gc_marker.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/mem/verification.h"

namespace panda::ecmascript {
void SharedHeap::StartUnifiedGCMark([[maybe_unused]]TriggerGCType gcType, [[maybe_unused]]GCReason gcReason)
{
    ASSERT(gcType == TriggerGCType::UNIFIED_GC && gcReason == GCReason::CROSSREF_CAUSE);
    ASSERT(JSThread::GetCurrent() == dThread_);
    {
        ThreadManagedScope runningScope(dThread_);
        SuspendAllScope scope(dThread_);
        Runtime *runtime = Runtime::GetInstance();
        std::vector<RecursionScope> recurScopes;
        // The approximate size is enough, because even if some thread creates and registers after here, it will keep
        // waiting in transition to RUNNING state before JSThread::SetReadyForGCIterating.
        recurScopes.reserve(runtime->ApproximateThreadListSize());
        runtime->GCIterateThreadList([&recurScopes](JSThread *thread) {
            Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
            recurScopes.emplace_back(heap, HeapType::LOCAL_HEAP);
        });
#ifdef PANDA_JS_ETS_HYBRID_MODE
        if (!unifiedGC_->StartXGCBarrier()) {
            unifiedGC_->SetInterruptUnifiedGC(false);
            dThread_->FinishRunningTask();
            return;
        }
#endif // PANDA_JS_ETS_HYBRID_MODE
        runtime->GCIterateThreadList([gcType](JSThread *thread) {
            Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
            if (UNLIKELY(heap->ShouldVerifyHeap())) { // LCOV_EXCL_BR_LINE
                // pre unified gc heap verify
                LOG_ECMA(DEBUG) << "pre unified gc heap verify";
                heap->ProcessSharedGCRSetWorkList();
                Verification(heap, VerifyKind::VERIFY_PRE_GC).VerifyAll();
            }
            heap->SetGCType(gcType);
        });
        unifiedGC_->RunPhases();
        runtime->GCIterateThreadList([](JSThread *thread) {
            Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
            if (UNLIKELY(heap->ShouldVerifyHeap())) { // LCOV_EXCL_BR_LINE
                // post unified gc heap verify
                LOG_ECMA(DEBUG) << "post unified gc heap verify";
                Verification(heap, VerifyKind::VERIFY_POST_GC).VerifyAll();
            }
        });
#ifdef PANDA_JS_ETS_HYBRID_MODE
        unifiedGC_->FinishXGCBarrier();
#endif // PANDA_JS_ETS_HYBRID_MODE
    }
}

void SharedHeap::CreateUnifiedGC()
{
    unifiedGC_ = new UnifiedGC();
}

void Heap::UnifiedGCPrepare()
{
    WaitRunningTaskFinished();
    sweeper_->EnsureAllTaskFinished();
    WaitClearTaskFinished();
}

void Heap::CreateUnifiedGCMarker()
{
    unifiedGCMarker_ = new UnifiedGCMarker(this);
}

uint32_t BaseHeap::GetRunningTaskCount()
{
    LockHolder holder(waitTaskFinishedMutex_);
    return runningTaskCount_;
}
}  // namespace panda::ecmascript
