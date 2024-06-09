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

#include "ecmascript/mem/allocator-inl.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/mem/sparse_space.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/taskpool/taskpool.h"
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

void ConcurrentMarker::Mark()
{
    RecursionScope recurScope(this);
    TRACE_GC(GCStats::Scope::ScopeId::ConcurrentMark, heap_->GetEcmaVM()->GetEcmaGCStats());
    LOG_GC(DEBUG) << "ConcurrentMarker: Concurrent Marking Begin";
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "ConcurrentMarker::Mark");
    MEM_ALLOCATE_AND_GC_TRACE(vm_, ConcurrentMarking);
    InitializeMarking();
    Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<MarkerTask>(heap_->GetJSThread()->GetThreadId(), heap_));
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
    workManager_->SetPostGCTaskType(ParallelGCTaskPhase::OLD_HANDLE_GLOBAL_POOL_TASK);
    nonMovableMarker->MarkRoots(MAIN_THREAD_INDEX);
    nonMovableMarker->ProcessMarkStack(MAIN_THREAD_INDEX);
    heap_->WaitRunningTaskFinished();
    // MarkJitCodeMap must be call after other mark work finish to make sure which jserror object js alive.
    nonMovableMarker->MarkJitCodeMap(MAIN_THREAD_INDEX);
}

void ConcurrentMarker::HandleMarkingFinished()  // js-thread wait for sweep
{
    LockHolder lock(waitMarkingFinishedMutex_);
    if (notifyMarkingFinished_) {
        TriggerGCType gcType;
        if (heap_->IsConcurrentFullMark()) {
            gcType = TriggerGCType::OLD_GC;
        } else if (heap_->IsEdenMark()) {
            gcType = TriggerGCType::EDEN_GC;
        } else {
            gcType = TriggerGCType::YOUNG_GC;
        }
        heap_->CollectGarbage(gcType,
                              GCReason::ALLOCATION_LIMIT);
    }
}

void ConcurrentMarker::WaitMarkingFinished()  // call in EcmaVm thread, wait for mark finished
{
    LockHolder lock(waitMarkingFinishedMutex_);
    if (!notifyMarkingFinished_) {
        vmThreadWaitMarkingFinished_ = true;
        waitMarkingFinishedCV_.Wait(&waitMarkingFinishedMutex_);
    }
}

void ConcurrentMarker::Reset(bool revertCSet)
{
    Finish();
    thread_->SetMarkStatus(MarkStatus::READY_TO_MARK);
    isConcurrentMarking_ = false;
    notifyMarkingFinished_ = false;
    if (revertCSet) {
        // Partial gc clear cset when evacuation allocator finalize
        heap_->GetOldSpace()->RevertCSet();
        auto callback = [](Region *region) {
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
    MEM_ALLOCATE_AND_GC_TRACE(vm_, ConcurrentMarkingInitialize);
    heap_->Prepare();
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
    } else if (heap_->IsEdenMark()) {
        heapObjectSize_ = heap_->GetEdenSpace()->GetHeapObjectSize();
    } else {
        heapObjectSize_ = heap_->GetNewSpace()->GetHeapObjectSize();
    }
    workManager_->Initialize(TriggerGCType::OLD_GC, ParallelGCTaskPhase::CONCURRENT_HANDLE_GLOBAL_POOL_TASK);
    if (heap_->IsYoungMark()) {
        {
            ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::MarkOldToNew");
            heap_->GetNonMovableMarker()->ProcessOldToNewNoMarkStack(MAIN_THREAD_INDEX);
        }
        heap_->GetNonMovableMarker()->ProcessSnapshotRSetNoMarkStack(MAIN_THREAD_INDEX);
    } else if (heap_->IsEdenMark()) {
        {
            ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::MarkNewToEden");
            heap_->GetNonMovableMarker()->ProcessOldToNewNoMarkStack(MAIN_THREAD_INDEX);
            heap_->GetNonMovableMarker()->ProcessNewToEdenNoMarkStack(MAIN_THREAD_INDEX);
        }
        heap_->GetNonMovableMarker()->ProcessSnapshotRSetNoMarkStack(MAIN_THREAD_INDEX);
    }
    heap_->GetNonMovableMarker()->MarkRoots(MAIN_THREAD_INDEX);
}

bool ConcurrentMarker::MarkerTask::Run(uint32_t threadId)
{
    // Synchronizes-with. Ensure that WorkManager::Initialize must be seen by MarkerThreads.
    while (!heap_->GetWorkManager()->HasInitialized());
    ClockScope clockScope;
    heap_->GetNonMovableMarker()->ProcessMarkStackConcurrent(threadId);
    heap_->WaitRunningTaskFinished();
    heap_->GetConcurrentMarker()->FinishMarking(clockScope.TotalSpentTime());
    DecreaseTaskCounts();
    return true;
}

void ConcurrentMarker::FinishMarking(float spendTime)
{
    LockHolder lock(waitMarkingFinishedMutex_);
    thread_->SetMarkStatus(MarkStatus::MARK_FINISHED);
    thread_->SetCheckSafePointStatus();
    if (vmThreadWaitMarkingFinished_) {
        vmThreadWaitMarkingFinished_ = false;
        waitMarkingFinishedCV_.Signal();
    }
    notifyMarkingFinished_ = true;
    if (heap_->IsYoungMark()) {
        heapObjectSize_ = heap_->GetNewSpace()->GetHeapObjectSize();
    } else if (heap_->IsConcurrentFullMark()) {
        heapObjectSize_ = heap_->GetHeapObjectSize();
    } else if (heap_->IsEdenMark()) {
        heapObjectSize_ = heap_->GetEdenSpace()->GetHeapObjectSize();
    }
    SetDuration(spendTime);
    if (heap_->IsFullMarkRequested()) {
        heap_->SetFullMarkRequestedState(false);
    }
}
}  // namespace panda::ecmascript
