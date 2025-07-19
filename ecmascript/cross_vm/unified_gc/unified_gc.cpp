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
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/mem/heap.h"

namespace panda::ecmascript {
#ifdef PANDA_JS_ETS_HYBRID_MODE
std::atomic<bool> UnifiedGC::isInterruptUnifiedGC_ {false};
#endif // PANDA_JS_ETS_HYBRID_MODE

void UnifiedGC::RunPhases()
{
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        if (heap->DaemonCheckOngoingConcurrentMarking()) {
            LOG_GC(DEBUG) << "UnifiedGC after ConcurrentMarking";
            heap->GetConcurrentMarker()->Reset();
        }
    });
    Initialize();
    Mark();
    Finish();
}

void UnifiedGC::Initialize()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "UnifiedGC::Initialize", "");
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        TRACE_GC(GCStats::Scope::ScopeId::Initialize, heap->GetEcmaGCStats());
        heap->GetUnifiedGCMarker()->Initialize();
    });
}

void UnifiedGC::Mark()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "UnifiedGC::Mark", "");
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        TRACE_GC(GCStats::Scope::ScopeId::Mark, heap->GetEcmaGCStats());
        heap->GetUnifiedGCMarker()->InitialMark(DAEMON_THREAD_INDEX);
    });
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        heap->GetUnifiedGCMarker()->ProcessMarkStack(DAEMON_THREAD_INDEX);
    });
#ifdef PANDA_JS_ETS_HYBRID_MODE
    static const auto noMarkTaskCheck = []() -> bool {
        bool noMarkTask = true;
        Runtime::GetInstance()->GCIterateThreadList([&noMarkTask](JSThread *thread) {
            Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
            if (heap->GetRunningTaskCount() != 0) {
                noMarkTask = false;
                return;
            }
        });
        return noMarkTask;
    };
    while (!stsVMInterface_->WaitForConcurrentMark(noMarkTaskCheck)) {
        Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
            Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
            heap->WaitRunningTaskFinished();
        });
    }
    stsVMInterface_->RemarkStartBarrier();
    while (!stsVMInterface_->WaitForRemark(noMarkTaskCheck)) {
        Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
            Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
            heap->WaitRunningTaskFinished();
        });
    }
#endif // PANDA_JS_ETS_HYBRID_MODE
}

void UnifiedGC::Finish()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "UnifiedGC::Finish", "");
    DaemonThread::GetInstance()->FinishRunningTask();
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        Heap *heap = const_cast<Heap *>(thread->GetEcmaVM()->GetHeap());
        TRACE_GC(GCStats::Scope::ScopeId::Finish, heap->GetEcmaGCStats());
        heap->GetUnifiedGCMarker()->Finish();
    });
}
}  // namespace panda::ecmascript