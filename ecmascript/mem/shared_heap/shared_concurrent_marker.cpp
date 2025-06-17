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

#include "ecmascript/mem/shared_heap/shared_concurrent_marker.h"

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/mem/shared_heap/shared_gc_marker-inl.h"
#include "ecmascript/mem/shared_heap/shared_gc_visitor-inl.h"
#include "ecmascript/mem/verification.h"

namespace panda::ecmascript {
SharedConcurrentMarker::SharedConcurrentMarker(EnableConcurrentMarkType type)
    : sHeap_(SharedHeap::GetInstance()),
      dThread_(DaemonThread::GetInstance()),
      sWorkManager_(sHeap_->GetWorkManager()),
      enableMarkType_(type) {}

void SharedConcurrentMarker::EnableConcurrentMarking(EnableConcurrentMarkType type)
{
    if (IsConfigDisabled()) {
        return;
    }
    if (IsEnabled() && !dThread_->IsReadyToConcurrentMark() && type == EnableConcurrentMarkType::DISABLE) {
        enableMarkType_ = EnableConcurrentMarkType::REQUEST_DISABLE;
    } else {
        enableMarkType_ = type;
    }
}

void SharedConcurrentMarker::MarkRoots(SharedMarkType markType)
{
    SharedGCMarkRootVisitor sharedGCMarkRootVisitor(sWorkManager_, DAEMON_THREAD_INDEX);
    sHeap_->GetSharedGCMarker()->MarkRoots(sharedGCMarkRootVisitor, markType);
}

void SharedConcurrentMarker::Mark(TriggerGCType gcType)
{
    RecursionScope recurScope(this);
    gcType_ = gcType;
    sHeap_->WaitSensitiveStatusFinished();
    {
        ThreadManagedScope runningScope(dThread_);
        SuspendAllScope scope(dThread_);
        TRACE_GC(GCStats::Scope::ScopeId::ConcurrentMark, sHeap_->GetEcmaGCStats());
        LOG_GC(DEBUG) << "SharedConcurrentMarker: Concurrent Marking Begin";
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, ("SharedConcurrentMarker::Mark;MarkReason"
        + std::to_string(static_cast<int>(sHeap_->GetEcmaGCStats()->GetMarkReason()))
        + ";Sensitive" + std::to_string(static_cast<int>(sHeap_->GetSensitiveStatus()))
        + ";IsInBackground" + std::to_string(sHeap_->IsInBackground())
        + ";Startup" + std::to_string(static_cast<int>(sHeap_->GetStartupStatus()))
        + ";Old" + std::to_string(sHeap_->GetOldSpace()->GetCommittedSize())
        + ";huge" + std::to_string(sHeap_->GetHugeObjectSpace()->GetCommittedSize())
        + ";NonMov" + std::to_string(sHeap_->GetNonMovableSpace()->GetCommittedSize())
        + ";TotCommit" + std::to_string(sHeap_->GetCommittedSize())
        + ";NativeBindingSize" + std::to_string(sHeap_->GetNativeSizeAfterLastGC())
        + ";NativeLimitGC" + std::to_string(sHeap_->GetNativeSizeTriggerSharedGC())
        + ";NativeLimitCM" + std::to_string(sHeap_->GetNativeSizeTriggerSharedCM())).c_str(), "");
        CHECK_DAEMON_THREAD();
        // TODO: support shared runtime state
        if (UNLIKELY(sHeap_->ShouldVerifyHeap())) {
            SharedHeapVerification(sHeap_, VerifyKind::VERIFY_PRE_SHARED_GC).VerifyAll();
        }
        InitializeMarking();
    }
    // Daemon thread do not need to post task to GC_Thread
    ASSERT(!dThread_->IsInRunningState());
    DoMarking();
    HandleMarkingFinished();
}

void SharedConcurrentMarker::Finish()
{
    sWorkManager_->Finish();
}

void SharedConcurrentMarker::ReMark()
{
    CHECK_DAEMON_THREAD();
#ifndef NDEBUG
    ASSERT(dThread_->HasLaunchedSuspendAll());
#endif
    TRACE_GC(GCStats::Scope::ScopeId::ReMark, sHeap_->GetEcmaGCStats());
    LOG_GC(DEBUG) << "SharedConcurrentMarker: Remarking Begin";
    // TODO: support shared runtime state
    SharedGCMarker *sharedGCMarker = sHeap_->GetSharedGCMarker();
    // If enable shared concurrent mark, the recorded weak reference slots from local to share may be changed
    // during LocalGC. For now just re-scan the local_to_share bit to record and update these weak references.
    MarkRoots(SharedMarkType::CONCURRENT_MARK_REMARK);
    sharedGCMarker->DoMark<SharedMarkType::CONCURRENT_MARK_REMARK>(DAEMON_THREAD_INDEX);
    sharedGCMarker->MergeBackAndResetRSetWorkListHandler();
    sHeap_->WaitRunningTaskFinished();
}

void SharedConcurrentMarker::Reset(bool clearGCBits)
{
    Finish();
    dThread_->SetSharedMarkStatus(SharedMarkStatus::READY_TO_CONCURRENT_MARK);
    isConcurrentMarking_ = false;
    if (clearGCBits) {
        sHeap_->GetOldSpace()->RevertCSets();
        // Shared gc clear GC bits in ReclaimRegions after GC
        auto callback = [](Region *region) {
            region->ClearMarkGCBitset();
            region->ResetAliveObject();
            region->ClearCrossRegionRSet();
        };
        sHeap_->EnumerateOldSpaceRegions(callback);
    }
}

void SharedConcurrentMarker::ResetWorkManager(SharedGCWorkManager *sWorkManager)
{
    sWorkManager_ = sWorkManager;
}

void SharedConcurrentMarker::InitializeMarking()
{
    CHECK_DAEMON_THREAD();
    // TODO: support shared runtime state
    sHeap_->Prepare(true);
    if (gcType_ == TriggerGCType::SHARED_PARTIAL_GC) {
        sHeap_->GetOldSpace()->SelectCSets();
    }
    isConcurrentMarking_ = true;
    dThread_->SetSharedMarkStatus(SharedMarkStatus::CONCURRENT_MARKING_OR_FINISHED);

    sHeapObjectSize_ = sHeap_->GetHeapObjectSize();
    sHeap_->GetAppSpawnSpace()->EnumerateRegions([](Region *current) {
        current->ClearMarkGCBitset();
        current->ClearCrossRegionRSet();
    });
    sHeap_->EnumerateOldSpaceRegions([](Region *current) {
        ASSERT(current->InSharedSweepableSpace());
        current->ResetAliveObject();
    });
    sWorkManager_->Initialize(TriggerGCType::SHARED_GC, SharedParallelMarkPhase::SHARED_MARK_TASK);
    MarkRoots(SharedMarkType::CONCURRENT_MARK_INITIAL_MARK);
}

void SharedConcurrentMarker::DoMarking()
{
    ClockScope clockScope;
    sHeap_->GetSharedGCMarker()->DoMark<SharedMarkType::CONCURRENT_MARK_INITIAL_MARK>(DAEMON_THREAD_INDEX);
    sHeap_->WaitRunningTaskFinished();
    FinishMarking(clockScope.TotalSpentTime());
}

void SharedConcurrentMarker::FinishMarking(float spendTime)
{
    sHeapObjectSize_ = sHeap_->GetHeapObjectSize();
    SetDuration(spendTime);
}

void SharedConcurrentMarker::HandleMarkingFinished()
{
    sHeap_->WaitSensitiveStatusFinished();
    sHeap_->DaemonCollectGarbage(gcType_, GCReason::HANDLE_MARKING_FINISHED);
}
}  // namespace panda::ecmascript
