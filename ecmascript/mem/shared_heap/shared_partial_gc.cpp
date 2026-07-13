/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/shared_heap/shared_partial_gc.h"

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_marker.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/mem/shared_heap/shared_gc_evacuator.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/mem/work_manager-inl.h"

namespace panda::ecmascript {
void SharedPartialGC::RunPhases()
{
    ASSERT("SharedPartialGC should be disabled" && !g_isEnableCMCGC);
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, ("SharedPartialGC::RunPhases;GCReason"
        + std::to_string(static_cast<int>(sHeap_->GetEcmaGCStats()->GetGCReason()))
        + ";MarkReason" + std::to_string(static_cast<int>(sHeap_->GetEcmaGCStats()->GetMarkReason()))
        + ";Sensitive" + std::to_string(static_cast<int>(sHeap_->GetSensitiveStatus()))
        + ";IsInBackground" + std::to_string(Runtime::GetInstance()->IsInBackground())
        + ";Startup" + std::to_string(static_cast<int>(sHeap_->GetStartupStatus()))
        + ";Old" + std::to_string(sHeap_->GetOldSpace()->GetCommittedSize())
        + ";huge" + std::to_string(sHeap_->GetHugeObjectSpace()->GetCommittedSize())
        + ";NonMov" + std::to_string(sHeap_->GetNonMovableSpace()->GetCommittedSize())
        + ";TotCommit" + std::to_string(sHeap_->GetCommittedSize())
        + ";NativeBindingSize" + std::to_string(sHeap_->GetNativeSizeAfterLastGC())
        + ";NativeLimitGC" + std::to_string(sHeap_->GetNativeSizeTriggerSharedGC())
        + ";NativeLimitCM" + std::to_string(sHeap_->GetNativeSizeTriggerSharedCM())).c_str(), "");
    TRACE_GC(GCStats::Scope::ScopeId::TotalGC, sHeap_->GetEcmaGCStats());
    markingInProgress_ = sHeap_->CheckOngoingConcurrentMarking();
    Initialize();
    Mark();
    if (UNLIKELY(sHeap_->ShouldVerifyHeap())) {
        SharedHeapVerification(sHeap_, VerifyKind::VERIFY_SHARED_GC_MARK).VerifyMark(markingInProgress_);
    }
    PreSweep();
    Evacuate();
    Sweep();
    if (UNLIKELY(sHeap_->ShouldVerifyHeap())) {
        SharedHeapVerification(sHeap_, VerifyKind::VERIFY_SHARED_GC_SWEEP).VerifySweep(markingInProgress_);
    }
    Finish();
    sHeap_->ResetNativeSizeAfterLastGC();
}

class SharedPartialGCSuspendCallback : public Closure {
public:
    explicit SharedPartialGCSuspendCallback(GCReason reason) : gcReason_(reason) {}
    ~SharedPartialGCSuspendCallback() override = default;

    NO_COPY_SEMANTIC(SharedPartialGCSuspendCallback);
    NO_MOVE_SEMANTIC(SharedPartialGCSuspendCallback);

    void Run([[maybe_unused]] JSThread *thread) override
    {
        ASSERT(thread->IsDaemonThread());
        SharedHeap *sHeap = SharedHeap::GetInstance();
        TriggerGCType type = TriggerGCType::SHARED_PARTIAL_GC;
        sHeap->CheckInHeapProfiler();
        sHeap->SetGCType(type);
        sHeap->GetEcmaGCStats()->RecordStatisticBeforeGC(type, gcReason_);
        sHeap->GetSharedPartialGC()->PreProcessLocalThread();
        sHeap->GetSharedPartialGC()->RunPhases();
        sHeap->CollectGarbageFinish(true, type);
    }

private:
    GCReason gcReason_ {GCReason::OTHER};
};

class SharedPartialGCFlipFunction : public Closure {
public:
    SharedPartialGCFlipFunction() = default;
    ~SharedPartialGCFlipFunction() override = default;

    NO_COPY_SEMANTIC(SharedPartialGCFlipFunction);
    NO_MOVE_SEMANTIC(SharedPartialGCFlipFunction);

    void Run(JSThread *thread) override
    {
        EcmaVM *vm = thread->GetEcmaVM();
        auto sHeap = SharedHeap::GetInstance();
        if (sHeap->HasCSetRegions()) {
            sHeap->GetSharedGCEvacuator()->FlipUpdateLocalRoot(vm);
        }
        sHeap->GetSharedPartialGC()->FlipUpdateWeakRoot(thread);
    }
};

void SharedPartialGC::RunFlip(GCReason reason)
{
    Runtime *runtime = Runtime::GetInstance();
    ThreadManagedScope runningScope(dThread_);
    SharedPartialGCSuspendCallback suspendCallback(reason);
    SharedPartialGCFlipFunction flipFunction;
    runtime->FlipAllThreads(dThread_, &suspendCallback, &flipFunction);
    // run in daemon thread concurrently.
    sHeap_->GetSharedGCEvacuator()->ProcessLocalToShareRSet();
    sHeap_->GetSharedGCEvacuator()->ProcessAndWaitSRegionUpdateFinished();
    SuspendAllScope scope(dThread_);
    sHeap_->Reclaim(TriggerGCType::SHARED_PARTIAL_GC);
    PostProcessLocalThread();
    sHeap_->GetSharedGCEvacuator()->MergeBackAndResetRSetWorkListHandler();
    SignalConcurrentUpdateFinished();
}

void SharedPartialGC::PreSweep()
{
    if (markingInProgress_) {
        sHeap_->GetOldSpace()->SelectCSets();
    }
    SharedGC::PreSweep();
}

void SharedPartialGC::Evacuate()
{
    if (sHeap_->HasCSetRegions()) {
        sHeap_->GetSharedGCEvacuator()->Evacuate();
    }
}

void SharedPartialGC::Sweep()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedPartialGC::Sweep", "");
    TRACE_GC(GCStats::Scope::ScopeId::Sweep, sHeap_->GetEcmaGCStats());
    auto stringTableCleaner = Runtime::GetInstance()->GetEcmaStringTable()->GetCleaner();
    if (stringTableCleaner->IsEnableConcurrentSweep()) {
        concurrentProcessStringTable_ = sHeap_->IsParallelGCEnabled() &&
                                        !Runtime::GetInstance()->GetEcmaStringTable()->IsInUse();
    } else {
        concurrentProcessStringTable_ = false;
    }
    if (!concurrentProcessStringTable_) {
        LOG_GC(DEBUG) << "process string table stw";
        stringTableCleaner->PostSweepWeakRefTask(UpdateWeakRootVisitor);
    }
    Runtime::GetInstance()->ProcessSharedDelete(UpdateWeakRootVisitor);
    if (!concurrentProcessStringTable_) {
        stringTableCleaner->JoinAndWaitSweepWeakRefTask(UpdateWeakRootVisitor);
    }
    sHeap_->GetSweeper()->PostTask(false);

    if (concurrentProcessStringTable_) {
        LOG_GC(DEBUG) << "process string table concurrently";
        stringTableCleaner->PostConcurrentSweepWeakRefTask(UpdateWeakRootVisitor);
    }
}

void SharedPartialGC::FlipUpdateWeakRoot(JSThread *thread)
{
    thread->IterateWeakEcmaGlobalStorage(UpdateWeakRootVisitor, GCKind::SHARED_GC);
    thread->GetEcmaVM()->ProcessSnapShotEnv(UpdateWeakRootVisitor);
    if (sHeap_->HasCSetRegions()) {
        thread->ClearVMCachedConstantPool();
    }
}

void SharedPartialGC::Finish()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedPartialGC::Finish", "");
    TRACE_GC(GCStats::Scope::ScopeId::Finish, sHeap_->GetEcmaGCStats());
    if (markingInProgress_) {
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "ConcurrentMarker::Reset", "");
        sHeap_->GetConcurrentMarker()->Reset(false);
    } else {
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "WorkManager::Finish", "");
        sWorkManager_->Finish();
    }
    sHeap_->GetSweeper()->TryFillSweptRegion();
}

void SharedPartialGC::WaitConcurrentUpdateFinished()
{
    LockHolder holder(waitConcurrentUpdateFinishedMutex_);
    while (!concurrentUpdateTaskFinished_) {
        waitConcurrentUpdateFinishedCV_.Wait(&waitConcurrentUpdateFinishedMutex_);
    }
}

void SharedPartialGC::SignalConcurrentUpdateFinished()
{
    if (!concurrentUpdateTaskFinished_) {
        LockHolder holder(waitConcurrentUpdateFinishedMutex_);
        concurrentUpdateTaskFinished_ = true;
        waitConcurrentUpdateFinishedCV_.SignalAll();
    }
}

void SharedPartialGC::WaitConcurrentUpdateFinished(JSThread *thread)
{
    panda::ecmascript::ThreadNativeScope scope(thread);
    WaitConcurrentUpdateFinished();
}

TaggedObject* SharedPartialGC::UpdateWeakRootVisitor(TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (UNLIKELY(objectRegion == nullptr)) {
        // existence of string table entry with null value is possible
        // only if concurrent sweeping for string table is enabled
        return nullptr;
    }
    if (!objectRegion->InSharedSweepableSpace()) {
        return object;
    }
    if (objectRegion->InSCollectSet()) {
        MarkWord markWord(object, RELAXED_LOAD);
        if (markWord.IsForwardingAddress()) {
            return markWord.ToForwardingAddress();
        }
        return nullptr;
    }
    if (objectRegion->Test(object)) {
        return object;
    }
    return nullptr;
}

void SharedPartialGC::PreProcessLocalThread()
{
    concurrentUpdateTaskFinished_ = false;
    Runtime::GetInstance()->IterateAllThreadList([](JSThread *thread) {
        if (!thread->ReadyForGCIterating()) {
            if (thread->IsJitThread()) {
                thread->SuspendThread(true, nullptr);
            }
            return;
        }
        auto heap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
        heap->WaitAndHandleCCFinished();
        heap->ResetTlab();
        std::shared_ptr<pgo::PGOProfiler> pgoProfiler =  thread->GetEcmaVM()->GetPGOProfiler();
        if (pgoProfiler != nullptr) {
            pgoProfiler->SuspendByGC();
        }
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
        thread->SetGcState(true);
#endif
        if (!thread->HasSwitchedToStwStub() || thread->GetLastLeaveFrame() == nullptr) {
            thread->SwitchAllStub(false);
            thread->SetReadBarrierState(true);
        } else {
            thread->SuspendThread(true, nullptr);
            thread->SetSuspendByConcurrentTask(true);
        }
    });
}

void SharedPartialGC::PostProcessLocalThread()
{
    Runtime::GetInstance()->IterateAllThreadList([](JSThread *thread) {
        ASSERT(thread->IsSuspended() || thread->HasLaunchedSuspendAll());
        if (!thread->ReadyForGCIterating()) {
            if (thread->IsJitThread()) {
                thread->ResumeThread(true);
            }
            return;
        }
        Heap *heap = thread->GetEcmaVM()->GetHeap();
        heap->ProcessGCListeners();
        // LocalCC also need read barrier.
        if (thread->SuspendByConcurrentTask()) {
            thread->ResumeThread(true);
            thread->SetSuspendByConcurrentTask(false);
        } else if (!thread->IsConcurrentCopying() && !heap->IsCCMark()) {
            thread->SetReadBarrierState(false);
            thread->SwitchAllStub(true);
        } else {
            return;
        }
        std::shared_ptr<pgo::PGOProfiler> pgoProfiler =  thread->GetEcmaVM()->GetPGOProfiler();
        if (pgoProfiler != nullptr) {
            pgoProfiler->ResumeByGC();
        }
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
        thread->SetGcState(false);
#endif
    });
}
}  // namespace panda::ecmascript
