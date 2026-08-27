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

#include "ecmascript/mem/shared_heap/shared_cc.h"
#include "common_components/taskpool/taskpool.h"
#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/daemon/daemon_thread.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/layout_info.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/shared_heap/shared_cc_evacuator-inl.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/mem/shared_heap/shared_gc_marker-inl.h"
#include "ecmascript/mem/shared_heap/shared_gc_visitor-inl.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/mem/work_manager.h"
#include "ecmascript/runtime.h"

namespace panda::ecmascript {

SharedCC::SharedCC(SharedHeap *heap)
    : sHeap_(heap),
      dThread_(DaemonThread::GetInstance()),
      marker_(heap->GetConcurrentMarker())
{
    uint32_t totalThreads = common::MAX_TASKPOOL_THREAD_NUM + 1;
    tlabAllocators_.reserve(totalThreads);
    for (uint32_t i = 0; i < totalThreads; i++) {
        tlabAllocators_.emplace_back(std::make_unique<SharedTlabAllocator>(sHeap_));
    }
}

void SharedCC::RunConcurrentMarkPhase(GCReason gcReason)
{
    {
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
                          "SharedCC::WaitSensitiveStatusFinished", "");
        // Avoid a sensitive-state deadlock during preparation.
        sHeap_->WaitSensitiveStatusFinished();
    }
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
        ("SharedCC::RunConcurrentMarkPhase;GCReason" +
         std::to_string(static_cast<int>(gcReason))).c_str(), "");
    // TotalGC is finalized in FinalizeAndReclaim; see totalGcTimer_ and the TRACE_GC note in gc_stats.h.
    totalGcTimer_.Reset();

    PrepareMainThread();
    ConcurrentMark();
}

void SharedCC::RunPhases(GCReason gcReason)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
        ("SharedCC::RunPhases;GCReason" +
         std::to_string(static_cast<int>(gcReason))).c_str(), "");
    ReMarkAndPrepare(gcReason); // Phase 2: STW
    ProcessMainThreadRSet();    // Phase 3: non-STW, prioritizes the main-thread detached RSet
    ParallelCopy();             // Phase 4a
    PostStringTableSweepTask(); // Phase 4b
    UpdateReferences();         // Phase 4c
    WaitStringTableSweep();     // Phase 4d
    FinalizeAndReclaim();       // Phase 5: snapshot STW, concurrent wait, reclaim-dispatch STW
}

void SharedCC::ConcurrentMark()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::ConcurrentMark", "");
    TRACE_GC(GCStats::Scope::ScopeId::ConcurrentMark, sHeap_->GetEcmaGCStats());
    marker_->Mark(TriggerGCType::SHARED_CC);
}

void SharedCC::ReMarkAndPrepare(GCReason gcReason)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::ReMarkAndPrepare", "");
    TRACE_GC(GCStats::Scope::ScopeId::ReMark, sHeap_->GetEcmaGCStats());
    ThreadManagedScope runningScope(dThread_);
    {
        SuspendAllScope scope(dThread_);
        EnterSharedGCScope();
        sHeap_->CheckProfilerEnabled();
        sHeap_->GetEcmaGCStats()->RecordStatisticBeforeGC(TriggerGCType::SHARED_CC, gcReason);
        marker_->ReMark();
        auto stringTableCleaner = Runtime::GetInstance()->GetEcmaStringTable()->GetCleaner();
        concurrentProcessStringTable_ = stringTableCleaner->IsEnableConcurrentSweep();
        SuspendIdleThreads();
        // Finish LocalCC copy before detaching its LocalToShared RSet.
        // ResetTlab must precede Sweep/PostTask to avoid racing with free-object construction.
        Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
            Heap *heap = thread->GetEcmaVM()->GetHeap();
            heap->WaitAndHandleCCFinished();
            heap->WaitRunningMarkTaskFinished();
            heap->ResetTlab();
        });
        sHeap_->GetSweeper()->Sweep(true);
        sHeap_->GetSweeper()->PostTask(true);
        PrepareForCopy();
        ProcessWeakReference();
        UpdateRoot();
        marker_->Reset(false);
        EstimatePostCCSize();
        sHeap_->UpdateGCThresholds(TriggerGCType::SHARED_CC);
        ccRunning_ = true;
        LogThreadStatesBeforeCopy();
        // The remaining phases are concurrent unless mutators are held by CC_SUSPEND.
        sHeap_->FinishGCTask();
        sHeap_->NotifyGCCompleted();
    }
}

void SharedCC::FinalizeAndReclaimInSTW(float previousStwDuration)
{
    // Post-snapshot marker tasks read slots already updated in Phase 4.
    FinalizeCopy();
    sHeap_->GetSweeper()->TryFillSweptRegion();
    sHeap_->Reclaim(TriggerGCType::SHARED_CC);
    if (UNLIKELY(sHeap_->ShouldVerifyHeap())) {
        // Taskpool clear is outside SuspendAll; wait before verification.
        sHeap_->WaitClearTaskFinished();
        SharedHeapVerification(sHeap_, VerifyKind::VERIFY_POST_SHARED_GC).VerifyAll();
    }

    // Include synchronous reclaim in the GC and STW durations.
    sHeap_->GetEcmaGCStats()->RecordScopeDuration(
        GCStats::Scope::ScopeId::TotalGC, totalGcTimer_.TotalSpentTime());
    sHeap_->GetEcmaGCStats()->RecordScopeDuration(
        GCStats::Scope::ScopeId::SuspendAll, previousStwDuration + stw3Timer_.TotalSpentTime());
    sHeap_->FinishGCStats(TriggerGCType::SHARED_CC);

    RestoreThreadStates();
    ExitSharedGCScope();
    {
        LockHolder lock(waitMutex_);
        ccRunning_ = false;
        waitCV_.SignalAll();
    }
}

void SharedCC::FinalizeAndReclaim()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::FinalizeAndReclaim", "");
    ThreadManagedScope runningScope(dThread_);

    // Snapshot marker tasks that may hold FROM references.
    float snapshotStwDuration = 0.0F;
    std::vector<std::shared_ptr<RunningMarkTaskSnapshot>> markTaskSnapshots;
    stw3Timer_.Reset();
    {
        SuspendAllScope scope(dThread_);
        Runtime::GetInstance()->GCIterateThreadList([&markTaskSnapshots](JSThread *thread) {
            auto snapshot = thread->GetEcmaVM()->GetHeap()->SnapshotRunningMarkTasks();
            if (!snapshot->IsFinished()) {
                markTaskSnapshots.emplace_back(snapshot);
            }
        });
        if (markTaskSnapshots.empty()) {
            FinalizeAndReclaimInSTW(0.0F);
        } else {
            snapshotStwDuration = stw3Timer_.TotalSpentTime();
        }
    }
    if (markTaskSnapshots.empty()) {
        return;
    }

    for (auto &snapshot : markTaskSnapshots) {
        snapshot->Wait();
    }

    stw3Timer_.Reset();
    {
        SuspendAllScope scope(dThread_);
        FinalizeAndReclaimInSTW(snapshotStwDuration);
    }
}

void SharedCC::EnterSharedGCScope()
{
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        std::shared_ptr<PGOProfiler> pgoProfiler = thread->GetEcmaVM()->GetPGOProfiler();
        if (pgoProfiler != nullptr) {
            pgoProfiler->SuspendByGC();
        }
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
        thread->SetGcState(true);
#endif
    });
}

void SharedCC::ExitSharedGCScope()
{
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        ASSERT(thread->IsSuspended() || thread->HasLaunchedSuspendAll() ||
               os::thread::GetCurrentThreadId() == DaemonThread::GetInstance()->GetThreadId());
        const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->ProcessGCListeners();
        std::shared_ptr<PGOProfiler> pgoProfiler = thread->GetEcmaVM()->GetPGOProfiler();
        // LocalCC PostGC resumes PGO if copying is still active.
        if (pgoProfiler != nullptr && !thread->IsConcurrentCopying()) {
            pgoProfiler->ResumeByGC();
        }
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
        thread->SetGcState(false);
#endif
    });
}

void SharedCC::PrepareMainThread()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::PrepareMainThread", "");
    TRACE_GC(GCStats::Scope::ScopeId::Initialize, sHeap_->GetEcmaGCStats());
    // Other threads are posted in CCMarkFlipFunction during ConcurrentMark.
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        if (thread != Runtime::GetInstance()->GetMainThread() ||
            !thread->HasPostTaskToThreadCallback()) {
            return;
        }
        if (thread->TryMarkCCTaskPending()) {
            thread->PostTaskToThread([thread]() {
                thread->ExecuteSharedCCStubSwitch();
            });
        }
    });
    WaitMainThreadReady();
}

void SharedCC::SuspendIdleThreads()
{
    Runtime::GetInstance()->GCIterateThreadList([this](JSThread *t) {
        t->WithCCStatusLock([&](SharedCCStatus &status) {
            if (!concurrentProcessStringTable_) {
                t->SetCCSuspend();
                status = SharedCCStatus::SUSPENDED;
                return;
            }
            if (status != SharedCCStatus::READY && status != SharedCCStatus::SUSPENDED) {
                if (t->GetLastLeaveFrame() == nullptr) {
                    t->HoldReadBarrier(ReadBarrierOwner::SHARED_CC);
                    status = SharedCCStatus::READY;
                } else {
                    t->SetCCSuspend();
                    status = SharedCCStatus::SUSPENDED;
                }
            }
        });
    });
    Runtime::GetInstance()->IterateAllThreadList([](JSThread *t) {
        if (t->IsJitThread()) {
            t->SetCCSuspend();
        }
    });
}

void SharedCC::EstimatePostCCSize()
{
    size_t totalAliveSize = 0;
    sHeap_->GetCompressSpace()->EnumerateRegions([&totalAliveSize](Region *r) {
        totalAliveSize += r->AliveObject();
    });
    sHeap_->GetOldSpace()->SetPreservedSize(totalAliveSize);
}

void SharedCC::PrepareForCopy()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::PrepareForCopy", "");
    sHeap_->SwapOldSpace();

    sHeap_->GetCompressSpace()->EnumerateRegions([](Region *r) {
        r->SetRegionTypeFlag(RegionTypeFlag::FROM);
    });

    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        if (thread->GetSharedCCStatus() == SharedCCStatus::READY) {
            thread->AcquireReadBarrier(ReadBarrierOwner::SHARED_CC);
        }
    });

    CollectUpdateRegions();
    if (concurrentProcessStringTable_) {
        SetStringTableCopyOrSweeping(true);
    }

    InstallSharedCCEvacuators();
}

void SharedCC::CollectUpdateRegions()
{
    sharedWorkloads_.clear();
    ASSERT(rSetHandlers_.empty());
    localRSetRegionCount_ = 0;
    mainThreadRSetHandler_ = nullptr;
    mainThreadRSetRegionCount_ = 0;
    rSetHandlers_.reserve(Runtime::GetInstance()->GetThreadListSize());
    JSThread *mainThread = Runtime::GetInstance()->GetMainThread();

    auto collectShared = [this](Region *region) {
        sharedWorkloads_.push_back(region);
    };
    sHeap_->GetNonMovableSpace()->EnumerateRegions(collectShared);
    sHeap_->GetHugeObjectSpace()->EnumerateRegions(collectShared);
    sHeap_->GetAppSpawnSpace()->EnumerateRegions(collectShared);

    Runtime::GetInstance()->GCIterateThreadList([this, mainThread](JSThread *thread) {
        auto *heap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
        heap->GetSweeper()->EnsureAllTaskFinished();
        size_t regionCount = heap->GetRegionCount();
        auto *handler = new RSetWorkListHandler(heap, thread);
        heap->SetRSetWorkListHandler(handler);
        thread->SetProcessingLocalToSharedRset(true);
        if (thread == mainThread) {
            mainThreadRSetHandler_ = handler;
            mainThreadRSetRegionCount_ = regionCount;
            return;
        }
        rSetHandlers_.push_back(handler);
        localRSetRegionCount_ += regionCount;
    });
}

void SharedCC::ProcessWeakReference()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::ProcessWeakReference", "");
    TRACE_GC(GCStats::Scope::ScopeId::UpdateWeekRef, sHeap_->GetEcmaGCStats());

    SharedCCEvacuator evacuator(sHeap_, GetTlabAllocator(DAEMON_THREAD_INDEX));
    UpdateRecordWeakReference(evacuator);

    WeakRootVisitor weakVisitor = [&evacuator](TaggedObject *object) -> TaggedObject* {
        Region *objectRegion = Region::ObjectAddressToRange(object);
        if (!objectRegion) {
            return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
        }
        if (objectRegion->IsFromRegion()) {
            if (!objectRegion->Test(object)) {
                return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
            }
            MarkWord markWord(object, RELAXED_LOAD);
            if (markWord.IsForwardingAddress()) {
                return markWord.ToForwardingAddress();
            }
            return evacuator.Copy(object, markWord);
        }
        if (!objectRegion->InSharedSweepableSpace() || objectRegion->Test(object)) {
            return object;
        }
        return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
    };
    Runtime::GetInstance()->ProcessSharedDelete(weakVisitor);

    Runtime::GetInstance()->GCIterateThreadList([weakVisitor](JSThread *thread) {
        thread->IterateWeakRoots(weakVisitor);
        thread->IterateWeakEcmaGlobalStorage(weakVisitor, GCKind::SHARED_GC);
        thread->GetEcmaVM()->ProcessSnapShotEnv(weakVisitor);
        thread->GetEcmaVM()->ProcessPendingRemovalModules(weakVisitor);
        thread->ClearVMCachedConstantPool();
    });
}

void SharedCC::InstallSharedCCEvacuators()
{
    LockHolder lock(evacuatorsMutex_);
    evacuators_.clear();
    Runtime::GetInstance()->GCIterateThreadList([this](JSThread *thread) {
        auto *evacuator = new SharedCCEvacuator(SharedHeap::GetInstance());
        evacuators_.push_back(evacuator);
        thread->InstallSharedCCEvacuator(evacuator);
    });
}

void SharedCC::FinalizeSharedCCEvacuators()
{
    LockHolder lock(evacuatorsMutex_);
    for (auto *evacuator : evacuators_) {
        evacuator->GetTlabAllocator()->Finalize();
        delete evacuator;
    }
    evacuators_.clear();
}

void SharedCC::UpdateRoot()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::UpdateRoot", "");
    TRACE_GC(GCStats::Scope::ScopeId::UpdateRoot, sHeap_->GetEcmaGCStats());

    SharedCCEvacuator evacuator(sHeap_, GetTlabAllocator(DAEMON_THREAD_INDEX));
    SharedCCRootVisitor rootVisitor(&evacuator);
    Runtime::GetInstance()->IterateSharedRoot(rootVisitor);

    Runtime::GetInstance()->GCIterateThreadList([&rootVisitor](JSThread *thread) {
        ObjectXRay::VisitVMRoots(thread->GetEcmaVM(), rootVisitor);
        thread->GetEcmaVM()->IterateGlobalEnvField(rootVisitor);
        thread->Iterate(rootVisitor);
    });
}

void SharedCCRootVisitor::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    UpdateObjectSlotRoot(slot);
}

void SharedCCRootVisitor::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        UpdateObjectSlotRoot(slot);
    }
}

void SharedCCRootVisitor::VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base, ObjectSlot derived,
                                                  uintptr_t baseOldObject)
{
    UpdateObjectSlotRoot(base);
    if (JSTaggedValue(base.GetTaggedType()).IsHeapObject()) {
        derived.Update(base.GetTaggedType() + derived.GetTaggedType() - baseOldObject);
    }
}

void SharedCCRootVisitor::UpdateObjectSlotRoot(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return;
    }
    TaggedObject *object = value.GetHeapObject();
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (!objectRegion->IsFromRegion()) {
        return;
    }
    MarkWord markWord(object, RELAXED_LOAD);
    if (markWord.IsForwardingAddress()) {
        slot.Update(markWord.ToForwardingAddress());
    } else {
        TaggedObject *toObject = evacuator_->Copy(object, markWord);
        slot.Update(toObject);
    }
}

void SharedCC::ParallelCopy()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::ParallelCopy", "");
    TRACE_GC(GCStats::Scope::ScopeId::Evacuate, sHeap_->GetEcmaGCStats());

    ASSERT(runningTaskCount_ == 0);
    int workerCount = CalculateCopyThreadNum();
    if (copyTasks_.empty()) {
        LOG_GC(DEBUG) << "SharedCC: ParallelCopy skipped, no FROM regions";
        return;
    }

    // +1 for daemon thread itself
    runningTaskCount_ = workerCount + 1;

    for (int i = 0; i < workerCount; i++) {
        common::Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<SharedCCCopyTask>(copyTasks_, taskIter_, runningTaskCount_, this));
    }

    SharedCCCopyTask daemonTask(copyTasks_, taskIter_, runningTaskCount_, this);
    daemonTask.Run(DAEMON_THREAD_INDEX);
    WaitFinished();
}

int SharedCC::CalculateCopyThreadNum()
{
    copyTasks_.clear();
    taskIter_ = 0;

    // Enumerate all FROM regions in compress space (the old sharedOldSpace after swap)
    auto collectTask = [this](Region *region) {
        copyTasks_.emplace_back(region);
    };

    sHeap_->GetCompressSpace()->EnumerateRegions(collectTask);

    uint32_t count = copyTasks_.size();
    constexpr uint32_t regionPerThread = 8;
    uint32_t maxThreadNum = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    return static_cast<int>(std::min(std::max(1U, count / regionPerThread), maxThreadNum));
}

int SharedCC::CalculateUpdateThreadNum()
{
    size_t count = sharedWorkloads_.size() + localRSetRegionCount_;
    constexpr uint32_t regionPerThread = 8;
    uint32_t maxThreadNum = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    size_t workerCount = std::max<size_t>(1, count / regionPerThread);
    return static_cast<int>(std::min(workerCount, static_cast<size_t>(maxThreadNum)));
}

void SharedCC::NotifyTasksFinished()
{
    LockHolder lock(waitMutex_);
    waitCV_.SignalAll();
}

bool SharedCCCopyTask::Run(uint32_t threadIndex)
{
    auto *allocator = cc_->GetTlabAllocator(threadIndex);
    SharedCCEvacuator evacuator(cc_->GetHeap(), allocator);

    Region *region = GetNextTask();
    while (region) {
        ASSERT(region->IsFromRegion());
        region->IterateAllMarkedBits([&](void *mem) {
            TaggedObject *object = reinterpret_cast<TaggedObject *>(mem);
            MarkWord markWord(object, RELAXED_LOAD);
            TaggedObject *toObject = nullptr;
            if (!markWord.IsForwardingAddress()) {
                toObject = evacuator.Copy(object, markWord);
            } else {
                toObject = markWord.ToForwardingAddress();
            }
            // Mark to-space copy so update phase skips new allocations.
            Region::ObjectAddressToRange(toObject)->AtomicMark(toObject);
        });
        region = GetNextTask();
    }

    if (runningTaskCount_.fetch_sub(1, std::memory_order_seq_cst) == 1) {
        cc_->NotifyTasksFinished();
    }

    return true;
}

Region* SharedCCCopyTask::GetNextTask()
{
    uint32_t idx = static_cast<uint32_t>(taskIter_.fetch_add(1U, std::memory_order_relaxed));
    if (idx < totalSize_) {
        return tasks_[idx];
    }
    return nullptr;
}

bool SharedCCUpdateTask::Run(uint32_t threadIndex)
{
    auto *allocator = cc_->GetTlabAllocator(threadIndex);
    SharedCCEvacuator evacuator(cc_->GetHeap(), allocator);
    for (size_t i = 0; i < cc_->rSetHandlers_.size(); i++) {
        cc_->ProcessRSetInternal(cc_->rSetHandlers_[i], evacuator);
    }

    SharedCCUpdateVisitor updateVisitor;
    auto processShared = [&updateVisitor](Region *region) {
        region->IterateAllMarkedBits([&](void *mem) {
            TaggedObject *object = reinterpret_cast<TaggedObject *>(mem);
            JSHClass *jsHclass = object->SynchronizedGetClass();
            ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, jsHclass, updateVisitor);
        });
    };
    while (true) {
        size_t idx = cc_->sharedIter_.fetch_add(1U, std::memory_order_relaxed);
        if (idx >= cc_->sharedWorkloads_.size()) {
            break;
        }
        processShared(cc_->sharedWorkloads_[idx]);
    }

    if (runningTaskCount_.fetch_sub(1, std::memory_order_seq_cst) == 1) {
        cc_->NotifyTasksFinished();
    }
    return true;
}

void SharedCC::ProcessMainThreadRSet()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::ProcessMainThreadRSet", "");
    RSetWorkListHandler *mainHandler = mainThreadRSetHandler_;
    if (mainHandler == nullptr) {
        return;
    }
    size_t count = mainThreadRSetRegionCount_;
    constexpr uint32_t regionPerThread = 8;
    uint32_t maxThreadNum = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    size_t workerCount = std::min(std::max<size_t>(1, count / regionPerThread),
                                  static_cast<size_t>(maxThreadNum));

    ASSERT(runningTaskCount_ == 0);
    runningTaskCount_ = workerCount + 1;

    for (size_t i = 0; i < workerCount; i++) {
        common::Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<SharedCCMainThreadRSetTask>(this, mainHandler, runningTaskCount_));
    }

    SharedCCMainThreadRSetTask daemonTask(this, mainHandler, runningTaskCount_);
    daemonTask.Run(DAEMON_THREAD_INDEX);
    WaitFinished();
}

void SharedCC::ProcessRSetInternal(RSetWorkListHandler *handler, SharedCCEvacuator &evacuator)
{
    ASSERT(handler != nullptr);
    handler->ProcessAll([&evacuator](void *mem, auto referenceTypeWrapper) -> bool {
        constexpr ReferenceType refType = decltype(referenceTypeWrapper)::value;
        ObjectSlotBase<refType> slot(ToUintPtr(mem));
        ProcessRSetSlot<refType>(slot, evacuator);
        // Keep the bit so MergeBack can restore it to the active RSet.
        return true;
    });
}

void SharedCC::ProcessRSetFromBoundJSThread(RSetWorkListHandler *handler)
{
    ASSERT(handler != nullptr);
    ASSERT(JSThread::GetCurrent() == handler->GetOwnerThreadUnsafe());
    ASSERT(JSThread::GetCurrent()->IsInRunningState());
    JSThread *thread = JSThread::GetCurrent();
    SharedCCEvacuator *evacuator = thread->GetSharedCCEvacuator();
    ASSERT(evacuator != nullptr);
    ProcessRSetInternal(handler, *evacuator);
    handler->WaitFinishedThenMergeBack();
}

void SharedCC::WaitFinished()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::WaitFinished", "");
    LockHolder lock(waitMutex_);
    while (runningTaskCount_ > 0) {
        waitCV_.Wait(&waitMutex_);
    }
}

void SharedCC::UpdateReferences()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::UpdateReferences", "");
    TRACE_GC(GCStats::Scope::ScopeId::UpdateReference, sHeap_->GetEcmaGCStats());

    sharedIter_.store(0, std::memory_order_relaxed);

    auto collectToSpace = [this](Region *region) {
        sharedWorkloads_.push_back(region);
    };
    uint32_t totalTaskpoolThreads = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    for (uint32_t i = 0; i <= totalTaskpoolThreads; i++) {
        GetTlabAllocator(i)->GetSharedLocalSpace()->EnumerateRegions(collectToSpace);
    }
    {
        LockHolder lock(evacuatorsMutex_);
        for (auto *evacuator : evacuators_) {
            evacuator->GetTlabAllocator()->GetSharedLocalSpace()->EnumerateRegions(collectToSpace);
        }
    }

    int workerCount = CalculateUpdateThreadNum();
    runningTaskCount_ = workerCount + 1;

    for (int i = 0; i < workerCount; i++) {
        common::Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<SharedCCUpdateTask>(this, runningTaskCount_));
    }

    SharedCCUpdateTask daemonTask(this, runningTaskCount_);
    daemonTask.Run(DAEMON_THREAD_INDEX);
    WaitFinished();
    sharedWorkloads_.clear();
}

void SharedCC::MergeBackAndResetRSetWorkListHandlers()
{
    if (mainThreadRSetHandler_ != nullptr) {
        mainThreadRSetHandler_->MergeBack();
        delete mainThreadRSetHandler_;
    }
    for (auto *handler : rSetHandlers_) {
        handler->MergeBack();
        delete handler;
    }
    rSetHandlers_.clear();
    localRSetRegionCount_ = 0;
    mainThreadRSetHandler_ = nullptr;
    mainThreadRSetRegionCount_ = 0;
}

void SharedCC::FinalizeCopy()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::FinalizeCopy", "");
    TRACE_GC(GCStats::Scope::ScopeId::Finalize, sHeap_->GetEcmaGCStats());
    sHeap_->GetOldSpace()->ResetPreservedSize();
    uint32_t totalTaskpoolThreads = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    for (uint32_t i = 0; i <= totalTaskpoolThreads; i++) {
        GetTlabAllocator(i)->Finalize();
    }
    FinalizeSharedCCEvacuators();
    MergeBackAndResetRSetWorkListHandlers();
    FinishConcurrentStringTableSweep();
}

void SharedCC::RestoreThreadStates()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::RestoreThreadStates", "");
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        thread->InstallSharedCCEvacuator(nullptr);
        thread->SetSharedCCStatus(SharedCCStatus::IDLE);
        thread->SetProcessingLocalToSharedRset(false);
        thread->ReleaseReadBarrier(ReadBarrierOwner::SHARED_CC);
        thread->TryRestoreNormalStubs();
        thread->ClearCCSuspend();
    });
    Runtime::GetInstance()->IterateAllThreadList([](JSThread *t) {
        if (t->IsJitThread()) {
            t->ClearCCSuspend();
        }
    });
    if (concurrentProcessStringTable_) {
        SetStringTableCopyOrSweeping(false);
    }
}

void SharedCC::PrepareNewThread(JSThread *thread)
{
    // Lock order: CC status -> evacuators.
    thread->WithCCStatusLock([&](SharedCCStatus &status) {
        LockHolder evacuatorLock(evacuatorsMutex_);
        if (!ccRunning_) {
            return;
        }
        if (thread->GetSharedCCEvacuator() == nullptr) {
            auto *evacuator = new SharedCCEvacuator(sHeap_);
            evacuators_.push_back(evacuator);
            thread->InstallSharedCCEvacuator(evacuator);
        }
        if (concurrentProcessStringTable_) {
            thread->AcquireReadBarrier(ReadBarrierOwner::SHARED_CC);
            status = SharedCCStatus::READY;
        } else {
            thread->SetCCSuspend();
            status = SharedCCStatus::SUSPENDED;
        }
    });
}

void SharedCCUpdateVisitor::VisitObjectRangeImpl(BaseObject *root, ObjectSlot start, ObjectSlot end,
    VisitObjectArea area)
{
    auto rootObject = TaggedObject::Cast(root);

    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        HandleInObjectArea(rootObject, start, end);
        return;
    }

    for (ObjectSlot slot = start; slot < end; slot++) {
        HandleSlot(slot);
    }
}

void SharedCCUpdateVisitor::VisitCompressedObjectRangeImpl(BaseObject *root, CompressedObjectSlot start,
    CompressedObjectSlot end)
{
    TaggedObject *rootObject = TaggedObject::Cast(root);
    for (CompressedObjectSlot slot = start; slot < end; slot++) {
        HandleSlot(slot);
    }
}

template <ReferenceType refType>
void SharedCCUpdateVisitor::HandleSlot(ObjectSlotBase<refType> slot)
{
    TaggedValueType<refType> value = slot.GetTaggedValue();
    if (!value.IsHeapObject()) {
        return;
    }

    TaggedObject *rawObject = value.GetRawHeapObject();
    Region *objectRegion = Region::ObjectAddressToRange(rawObject);
    if (!objectRegion->IsFromRegion()) {
        return;
    }

    TaggedObject *object = value.GetHeapObject();
    MarkWord markWord(object, RELAXED_LOAD);

    ASSERT(markWord.IsForwardingAddress());
    TaggedObject *dst = markWord.ToForwardingAddress();
    if constexpr (ReferenceIsCompressed<refType>) {
        ASSERT(!value.IsWeakForHeapObject());
        slot.CASUpdate(rawObject, dst);
    } else {
        if (value.IsWeakForHeapObject()) {
            slot.CASUpdateWeak(rawObject, dst);
        } else {
            slot.CASUpdate(rawObject, dst);
        }
    }
}

void SharedCCUpdateVisitor::HandleInObjectArea(TaggedObject *rootObject, ObjectSlot startSlot,
    ObjectSlot endSlot)
{
    JSHClass *hclass = rootObject->SynchronizedGetClass();
    ASSERT(!hclass->IsAllTaggedProp());
    int index = 0;
    LayoutInfo *layout = LayoutInfo::UncheckCast(
        hclass->GetLayout<RBMode::FAST_NO_RB>(THREAD_ARG_PLACEHOLDER).GetTaggedObject());
    ObjectSlot realEnd(reinterpret_cast<uintptr_t>(startSlot.SlotAddress()));
    realEnd += layout->GetPropertiesCapacity();
    ObjectSlot actualEnd = endSlot > realEnd ? realEnd : endSlot;

    for (ObjectSlot slot = startSlot; slot < actualEnd; slot++) {
        PropertyAttributes attr = layout->GetAttr<RBMode::FAST_NO_RB>(THREAD_ARG_PLACEHOLDER, index++);
        if (attr.IsTaggedRep()) {
            HandleSlot(slot);
        }
    }
}

static const WeakRootVisitor &GetStringTableWeakVisitor()
{
    static const WeakRootVisitor visitor = [](TaggedObject *header) -> TaggedObject* {
        Region *objectRegion = Region::ObjectAddressToRange(header);
        if (!objectRegion) {
            return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
        }
        if (objectRegion->IsFromRegion()) {
            MarkWord markWord(header, RELAXED_LOAD);
            if (markWord.IsForwardingAddress()) {
                return markWord.ToForwardingAddress();
            }
            return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
        }
        if (!objectRegion->InSharedSweepableSpace() || objectRegion->Test(header)) {
            return header;
        }
        return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
    };
    return visitor;
}

void SharedCC::PostStringTableSweepTask()
{
    auto stringTableCleaner = Runtime::GetInstance()->GetEcmaStringTable()->GetCleaner();
    if (concurrentProcessStringTable_) {
        stringTableCleaner->PostConcurrentSweepWeakRefTask(GetStringTableWeakVisitor());
    } else {
        stringTableCleaner->PostSweepWeakRefTask(GetStringTableWeakVisitor());
    }
}

void SharedCC::WaitStringTableSweep()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::WaitStringTableSweep", "");
    auto stringTableCleaner = Runtime::GetInstance()->GetEcmaStringTable()->GetCleaner();
    if (concurrentProcessStringTable_) {
        stringTableCleaner->WaitConcurrentSweepWeakRefTaskFinished();
    } else {
        stringTableCleaner->JoinAndWaitSweepWeakRefTask(GetStringTableWeakVisitor());
    }
}

void SharedCC::FinishConcurrentStringTableSweep()
{
    auto stringTableCleaner = Runtime::GetInstance()->GetEcmaStringTable()->GetCleaner();
    if (concurrentProcessStringTable_) {
        stringTableCleaner->FinishConcurrentSweepInSTW(dThread_);
    }
}

void SharedCC::SetStringTableCopyOrSweeping(bool enabled)
{
    auto *chainedHashMap = reinterpret_cast<DisableCMCGCNormalTrait::ChainedHashMapType*>(
        Runtime::GetInstance()->GetEcmaStringTable()->GetChainedHashMap());
    if (enabled) {
        chainedHashMap->StartSweeping();
    } else {
        chainedHashMap->FinishSweeping();
    }
}

void SharedCC::UpdateRecordWeakReference(SharedCCEvacuator &evacuator)
{
    auto workManager = sHeap_->GetWorkManager();
    auto processWeakReference = [&evacuator](SharedGCWorkNodeHolder *holder) {
        ProcessQueue *queue = holder->GetWeakReferenceQueue();
        UpdateSharedCCWeakReferences(queue, evacuator);
    };
    auto totalThreadCount = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() + 1;
    for (uint32_t i = 0; i < totalThreadCount; i++) {
        processWeakReference(workManager->GetSharedGCWorkNodeHolder(i));
    }
    workManager->ForEachExtraTemporaryWorkNodeHolder(processWeakReference);
}

void SharedCC::LogThreadStatesBeforeCopy()
{
    if (!sHeap_->GetEcmaGCStats()->EnableGCTracer()) {
        return;
    }
    int readyCount = 0;
    int suspendedCount = 0;
    Runtime::GetInstance()->GCIterateThreadList([&](JSThread *thread) {
        auto status = thread->GetSharedCCStatus();
        if (status == SharedCCStatus::READY) {
            readyCount++;
        } else if (status == SharedCCStatus::SUSPENDED) {
            suspendedCount++;
        }
    });
    LOG_GC(DEBUG) << "SharedCC: threads before copy - READY=" << readyCount
                  << ", SUSPENDED=" << suspendedCount;
}

void SharedCC::NotifyMainThreadReady()
{
    LockHolder holder(sHeap_->GetSharedGCSyncMutex());
    sHeap_->GetSharedGCSyncCV().SignalAll();
}

void SharedCC::WaitMainThreadReady()
{
    LOG_GC(DEBUG) << "SharedCC: WaitMainThreadReady Begin";

    JSThread *mainThread = Runtime::GetInstance()->GetMainThread();
    if (mainThread == nullptr) {
        LOG_GC(DEBUG) << "SharedCC: WaitMainThreadReady End, no main thread";
        return;
    }

    if (mainThread->GetSharedCCStatus() == SharedCCStatus::READY) {
        LOG_GC(DEBUG) << "SharedCC: WaitMainThreadReady End, already READY";
        return;
    }

    Mutex &mtx = sHeap_->GetSharedGCSyncMutex();
    ConditionVariable &cv = sHeap_->GetSharedGCSyncCV();
    LockHolder lock(mtx);

    while (true) {
        bool done = false;
        mainThread->WithCCStatusLock([&](SharedCCStatus &status) {
            if (status == SharedCCStatus::READY) {
                LOG_GC(DEBUG) << "SharedCC: WaitMainThreadReady End, READY via callback";
                done = true;
                return;
            }
            if (mainThread->IsWaitingSharedGCFinished()) {
                mainThread->SetCCSuspend();
                status = SharedCCStatus::SUSPENDED;
                LOG_GC(DEBUG) << "SharedCC: main thread blocked in WaitGCFinished, set SUSPENDED";
                done = true;
            }
        });
        if (done) {
            return;
        }
        cv.Wait(&mtx);
    }
}

bool SharedCCMainThreadRSetTask::Run(uint32_t threadIndex)
{
    auto *allocator = cc_->GetTlabAllocator(threadIndex);
    SharedCCEvacuator evacuator(cc_->GetHeap(), allocator);
    cc_->ProcessRSetInternal(handler_, evacuator);

    if (runningTaskCount_.fetch_sub(1, std::memory_order_seq_cst) == 1) {
        cc_->NotifyTasksFinished();
    }

    return true;
}

template <ReferenceType refType>
void ProcessRSetSlot(ObjectSlotBase<refType> slot, SharedCCEvacuator &evacuator)
{
    TaggedValueType<refType> value = slot.GetTaggedValue();
    if (!value.IsHeapObject()) {
        return;
    }

    TaggedObject *rawObject = value.GetRawHeapObject();
    Region *objectRegion = Region::ObjectAddressToRange(rawObject);
    if (!objectRegion->IsFromRegion()) {
        return;
    }

    if (!objectRegion->Test(rawObject)) {
        return;
    }

    TaggedObject *object = value.GetHeapObject();
    MarkWord markWord(object, RELAXED_LOAD);
    TaggedObject *dst = markWord.IsForwardingAddress()
        ? markWord.ToForwardingAddress()
        : evacuator.Copy(object, markWord);

    if constexpr (ReferenceIsCompressed<refType>) {
        ASSERT(!value.IsWeakForHeapObject());
        slot.CASUpdate(rawObject, dst);
    } else {
        if (value.IsWeakForHeapObject()) {
            slot.CASUpdateWeak(rawObject, dst);
        } else {
            slot.CASUpdate(rawObject, dst);
        }
    }
}
}  // namespace panda::ecmascript
