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

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/unified_gc/unified_gc_marker-inl.h"
#include "ecmascript/mem/verification.h"

namespace panda::ecmascript {
void UnifiedGCMarker::Mark()
{
    RecursionScope recurScope(this);
    {
        ThreadManagedScope runningScope(dThread_);
        SuspendAllScope scope(dThread_);
        if (heap_->DaemonCheckOngoingConcurrentMarking()) {
            LOG_GC(DEBUG) << "UnifiedGC after ConcurrentMarking";
            heap_->GetConcurrentMarker()->Reset();
        }
        heap_->SetMarkType(MarkType::MARK_FULL);
#ifdef PANDA_JS_ETS_HYBRID_MODE
        ASSERT(stsVMInterface_ != nullptr);
        stsVMInterface_->StartXGCBarrier();
#endif // PANDA_JS_ETS_HYBRID_MODE
        TRACE_GC(GCStats::Scope::ScopeId::Mark, heap_->GetEcmaVM()->GetEcmaGCStats());
        LOG_GC(DEBUG) << "UnifiedGCMarker: Mark Begin";
        ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "UnifiedGCMarker::Mark");
        CHECK_DAEMON_THREAD();
        ClockScope clockScope;
        InitializeMarking(DAEMON_THREAD_INDEX);
        DoMarking(DAEMON_THREAD_INDEX);
        if (UNLIKELY(heap_->ShouldVerifyHeap())) {
            Verification::VerifyMark(heap_);
        }
        Finish(clockScope.TotalSpentTime());
    }
}

void UnifiedGCMarker::Initialize()
{
    LockHolder holder(initializeMutex_);
    if (!initialized_) {
        heap_->UnifiedGCPrepare();
        heap_->GetAppSpawnSpace()->EnumerateRegions([](Region *current) {
            current->ClearMarkGCBitset();
        });
        workManager_->Initialize(TriggerGCType::UNIFIED_GC, ParallelGCTaskPhase::UNIFIED_HANDLE_GLOBAL_POOL_TASK);
        initialized_ = true;
    }
}

void UnifiedGCMarker::InitializeMarking(uint32_t threadId)
{
    CHECK_DAEMON_THREAD();
    Initialize();
    UnifiedGCMarkRootsScope scope(heap_->GetJSThread());
    MarkRoots(threadId);
}

void UnifiedGCMarker::DoMarking(uint32_t threadId)
{
    CHECK_DAEMON_THREAD();
    ProcessMarkStack(threadId);
    heap_->WaitRunningTaskFinished();
#ifdef PANDA_JS_ETS_HYBRID_MODE
    auto noMarkTaskCheck = [heap = this->heap_]() -> bool {
        return heap->GetRunningTaskCount() == 0;
    };
    while (!stsVMInterface_->WaitForConcurrentMark(noMarkTaskCheck)) {
        heap_->WaitRunningTaskFinished();
    }
    stsVMInterface_->RemarkStartBarrier();
    while (!stsVMInterface_->WaitForRemark(noMarkTaskCheck)) {
        heap_->WaitRunningTaskFinished();
    }
#endif // PANDA_JS_ETS_HYBRID_MODE
}

void UnifiedGCMarker::Finish(float spendTime)
{
    CHECK_DAEMON_THREAD();
    SetDuration(spendTime);
    workManager_->Finish();
    initialized_ = false;
    heap_->Resume(TriggerGCType::UNIFIED_GC);
    if (UNLIKELY(heap_->ShouldVerifyHeap())) { // LCOV_EXCL_BR_LINE
        // verify post unified gc heap verify
        LOG_ECMA(DEBUG) << "post unified gc heap verify";
        Verification(heap_, VerifyKind::VERIFY_POST_GC).VerifyAll();
    }
    dThread_->FinishRunningTask();
#ifdef PANDA_JS_ETS_HYBRID_MODE
    stsVMInterface_->FinishXGCBarrier();
#endif // PANDA_JS_ETS_HYBRID_MODE
}

void UnifiedGCMarker::MarkFromObject(TaggedObject *object)
{
    Initialize();
    Region *objectRegion = Region::ObjectAddressToRange(object);

    if (objectRegion->InSharedHeap()) {
        return;
    }

    if (objectRegion->AtomicMark(object)) {
        workManager_->PushObjectToGlobal(object);
    }
}

void UnifiedGCMarker::ProcessMarkStack(uint32_t threadId)
{
    auto cb = [&](ObjectSlot s) { MarkValue(threadId, s); };
    EcmaObjectRangeVisitor visitor = [this, threadId, cb](TaggedObject *root, ObjectSlot start,
                                                          ObjectSlot end, VisitObjectArea area) {
        if (area == VisitObjectArea::IN_OBJECT) {
            if (VisitBodyInObj(root, start, end, cb)) {
                return;
            }
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            MarkValue(threadId, slot);
        }
    };
    TaggedObject *obj = nullptr;
    while (workManager_->Pop(threadId, &obj)) {
        JSHClass *jsHclass = obj->SynchronizedGetClass();
        MarkObject(threadId, jsHclass);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHclass, visitor);
    }
}
}  // namespace panda::ecmascript