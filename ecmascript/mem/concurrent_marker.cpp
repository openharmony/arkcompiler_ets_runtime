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

#include "ecmascript/mem/concurrent_marker.h"

#include "ecmascript/mem/parallel_marker.h"
#include "ecmascript/mem/old_gc_visitor-inl.h"
#include "ecmascript/mem/young_gc_visitor-inl.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
size_t ConcurrentMarker::taskCounts_ = 0;
Mutex ConcurrentMarker::taskCountMutex_;

ConcurrentMarker::ConcurrentMarker(Heap *heap, EnableConcurrentMarkType type)
    : heap_(heap),
      vm_(heap->GetEcmaVM()),
      thread_(vm_->GetJSThread()),
      workManager_(heap->GetWorkManager()),
      enableMarkType_(type)
{
    thread_->SetMarkStatus(MarkStatus::READY_TO_MARK);
}

void ConcurrentMarker::EnableConcurrentMarking(EnableConcurrentMarkType type)
{
    if (IsConfigDisabled()) {
        return;
    }
    if (IsEnabled() && !thread_->IsReadyToConcurrentMark() && type == EnableConcurrentMarkType::DISABLE) {
        enableMarkType_ = EnableConcurrentMarkType::REQUEST_DISABLE;
    } else {
        enableMarkType_ = type;
    }
}

void ConcurrentMarker::MarkRoots()
{
    if (heap_->IsYoungMark()) {
        YoungGCMarkRootVisitor youngGCMarkRootVisitor(workManager_->GetWorkNodeHolder(MAIN_THREAD_INDEX));
        heap_->GetNonMovableMarker()->MarkRoots(youngGCMarkRootVisitor);
    } else {
        OldGCMarkRootVisitor oldGCMarkRootVisitor(workManager_->GetWorkNodeHolder(MAIN_THREAD_INDEX));
        heap_->GetNonMovableMarker()->MarkRoots(oldGCMarkRootVisitor);
    }
}

void ConcurrentMarker::Mark()
{
    GCStats *gcStats = heap_->GetEcmaVM()->GetEcmaGCStats();
    RecursionScope recurScope(this);
    TRACE_GC(GCStats::Scope::ScopeId::ConcurrentMark, gcStats);
    LOG_GC(DEBUG) << "ConcurrentMarker: Concurrent Marking Begin";
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "ConcurrentMarker::Mark" + std::to_string(heap_->IsFullMarkRequested())
        + ";Reason" + std::to_string(static_cast<int>(gcStats->GetGCReason()))
        + ";Sensitive" + std::to_string(static_cast<int>(heap_->GetSensitiveStatus()))
        + ";IsInBackground" + std::to_string(heap_->IsInBackground())
        + ";Startup" + std::to_string(static_cast<int>(heap_->GetStartupStatus()))
        + ";ConMark" + std::to_string(static_cast<int>(heap_->GetJSThread()->GetMarkStatus()))
        + ";Young" + std::to_string(heap_->GetNewSpace()->GetCommittedSize())
        + ";Old" + std::to_string(heap_->GetOldSpace()->GetCommittedSize())
        + ";TotalCommit" + std::to_string(heap_->GetCommittedSize())
        + ";NativeBindingSize" + std::to_string(heap_->GetNativeBindingSize())
        + ";NativeLimitSize" + std::to_string(heap_->GetGlobalSpaceNativeLimit()));
    MEM_ALLOCATE_AND_GC_TRACE(vm_, ConcurrentMarking);
    InitializeMarking();
    clockScope_.Reset();
    heap_->PostParallelGCTask(ParallelGCTaskPhase::CONCURRENT_HANDLE_GLOBAL_POOL_TASK);
}

void ConcurrentMarker::Finish()
{
    workManager_->Finish();
}

void ConcurrentMarker::ReMark()
{
    TRACE_GC(GCStats::Scope::ScopeId::ReMark, heap_->GetEcmaVM()->GetEcmaGCStats());
    LOG_GC(DEBUG) << "ConcurrentMarker: Remarking Begin";
    MEM_ALLOCATE_AND_GC_TRACE(vm_, ReMarking);
    Marker *nonMovableMarker = heap_->GetNonMovableMarker();
    MarkRoots();
    nonMovableMarker->ProcessMarkStack(MAIN_THREAD_INDEX);
    heap_->WaitRunningTaskFinished();
    // MarkJitCodeMap must be call after other mark work finish to make sure which jserror object js alive.
    nonMovableMarker->MarkJitCodeMap(MAIN_THREAD_INDEX);
}

void ConcurrentMarker::HandleMarkingFinished(GCReason gcReason)  // js-thread wait for sweep
{
    LockHolder lock(waitMarkingFinishedMutex_);
    ASSERT(markingFinished_);
    TriggerGCType gcType;
    if (heap_->IsConcurrentFullMark()) {
        gcType = TriggerGCType::OLD_GC;
    } else {
        gcType = TriggerGCType::YOUNG_GC;
    }
    heap_->CollectGarbage(gcType, gcReason);
}

void ConcurrentMarker::WaitMarkingFinished()  // call in EcmaVm thread, wait for mark finished
{
    LockHolder lock(waitMarkingFinishedMutex_);
    while (!markingFinished_) {
        waitMarkingFinishedCV_.Wait(&waitMarkingFinishedMutex_);
    }
}

void ConcurrentMarker::Reset(bool revertCSet)
{
    ASSERT(runningTaskCount_ == 0);
    Finish();
    thread_->SetMarkStatus(MarkStatus::READY_TO_MARK);
    isConcurrentMarking_ = false;
    markingFinished_ = false;
    notifyMarkingFinished_ = false;
    if (revertCSet) {
        // Partial gc clear cset when evacuation allocator finalize
        heap_->GetOldSpace()->RevertCSet();
        auto callback = [](Region *region) {
            region->ResetRegionTypeFlag();
            region->ClearMarkGCBitset();
            region->ClearCrossRegionRSet();
            region->ResetAliveObject();
        };
        if (heap_->IsConcurrentFullMark()) {
            heap_->EnumerateRegions(callback);
        } else {
            heap_->EnumerateNewSpaceRegions(callback);
        }
    }
}

void ConcurrentMarker::InitializeMarking()
{
    ASSERT(runningTaskCount_ == 0);
    MEM_ALLOCATE_AND_GC_TRACE(vm_, ConcurrentMarkingInitialize);
    heap_->Prepare();
    ASSERT(VerifyAllRegionsNonFresh());
    heap_->GetNewSpace()->RecordCurrentRegionAsHalfFresh();
    isConcurrentMarking_ = true;
    thread_->SetMarkStatus(MarkStatus::MARKING);

    if (heap_->IsConcurrentFullMark()) {
        heapObjectSize_ = heap_->GetHeapObjectSize();
        heap_->GetOldSpace()->SelectCSet();
        heap_->GetAppSpawnSpace()->EnumerateRegions([](Region *current) {
            current->ClearMarkGCBitset();
            current->ClearCrossRegionRSet();
        });
        // The alive object size of Region in OldSpace will be recalculated.
        heap_->EnumerateNonNewSpaceRegions([](Region *current) {
            current->ResetAliveObject();
        });
    } else {
        heapObjectSize_ = heap_->GetNewSpace()->GetHeapObjectSize();
    }
    workManager_->Initialize(TriggerGCType::OLD_GC, ParallelGCTaskPhase::CONCURRENT_HANDLE_GLOBAL_POOL_TASK);
    if (heap_->IsYoungMark()) {
        NonMovableMarker *marker = static_cast<NonMovableMarker*>(heap_->GetNonMovableMarker());
        {
            ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::MarkOldToNew");
            marker->ProcessOldToNewNoMarkStack(MAIN_THREAD_INDEX);
        }
        marker->ProcessSnapshotRSetNoMarkStack(MAIN_THREAD_INDEX);
    }
    MarkRoots();
    workManager_->GetWorkNodeHolder(MAIN_THREAD_INDEX)->PushWorkNodeToGlobal(false);
}

bool ConcurrentMarker::ShouldNotifyMarkingFinished()
{
    if (runningTaskCount_.fetch_sub(1, std::memory_order_relaxed) != 1) {
        return false;
    }
    return reinterpret_cast<std::atomic<bool>*>(&notifyMarkingFinished_)
        ->exchange(true, std::memory_order_relaxed) == false;
}

void ConcurrentMarker::FinishMarking()
{
    LockHolder lock(waitMarkingFinishedMutex_);
    ASSERT(!markingFinished_);
    ASSERT(notifyMarkingFinished_);
    float spendTime = clockScope_.TotalSpentTime();
    if (heap_->IsYoungMark()) {
        heapObjectSize_ = heap_->GetNewSpace()->GetHeapObjectSize();
    } else if (heap_->IsConcurrentFullMark()) {
        heapObjectSize_ = heap_->GetHeapObjectSize();
    }
    SetDuration(spendTime);
    if (heap_->IsFullMarkRequested()) {
        heap_->SetFullMarkRequestedState(false);
    }
    thread_->SetMarkStatus(MarkStatus::MARK_FINISHED);
    thread_->SetCheckSafePointStatus();
    markingFinished_ = true;
    waitMarkingFinishedCV_.Signal();
    DecreaseTaskCounts();
}

void ConcurrentMarker::ProcessConcurrentMarkTask(uint32_t threadId)
{
    runningTaskCount_.fetch_add(1, std::memory_order_relaxed);
    heap_->GetNonMovableMarker()->ProcessMarkStack(threadId);
    if (ShouldNotifyMarkingFinished()) {
        FinishMarking();
        heap_->GetIdleGCTrigger()->TryPostHandleMarkFinished();
    }
}

bool ConcurrentMarker::VerifyAllRegionsNonFresh()
{
    bool ok = true;
    heap_->EnumerateRegions([&ok](Region *region) {
        ok &= !region->IsFreshRegion() && !region->IsHalfFreshRegion();
    });
    return ok;
}
}  // namespace panda::ecmascript
