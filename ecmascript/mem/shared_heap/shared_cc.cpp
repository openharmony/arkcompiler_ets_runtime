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

void SharedCC::Trigger(GCReason gcReason)
{
    CHECK_DAEMON_THREAD();
    RunPhases(gcReason);
}

void SharedCC::RunPhases(GCReason gcReason)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
        ("SharedCC::RunPhases;GCReason " + std::to_string(static_cast<int>(gcReason))
        + ";MarkReason" + std::to_string(static_cast<int>(sHeap_->GetEcmaGCStats()->GetMarkReason()))).c_str(), "");
    GCStats *gcStats = sHeap_->GetEcmaGCStats();
    TRACE_GC(GCStats::Scope::ScopeId::TotalGC, gcStats);

    PrepareAllThreads();

    {
        WriteLockHolder gcWriteLock(BaseHeap::gcExclusiveRWLock_);
        ConcurrentMark();           // Phase 1: concurrent (brief STW for init)
        ReMarkAndPrepare(gcReason); // Phase 2: STW — remark, swap FROM/TO, forward roots
        ParallelCopy();             // Phase 3a: concurrent (mutators run)
        PostStringTableSweepTask(); // Phase 3b: post sweep (concurrent with UpdateReferences)
        UpdateReferences();         // Phase 3c: concurrent (mutators run)
        WaitStringTableSweep();     // Phase 3d: block until sweep done (mutators still run)
        FinalizeAndReclaim();       // Phase 4: STW — finalize copy, sweep, reclaim

        if (UNLIKELY(sHeap_->ShouldVerifyHeap())) {
            VerifyHeap();           // Phase 5: STW (rare)
        }
    } // write lock released — safe to interact with main thread

    Finish();                       // Phase 6: post-GC notifications (outside write lock)
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
    SuspendAllScope scope(dThread_);
    EnterSharedGCScope();
    sHeap_->CheckInHeapProfiler();
    sHeap_->GetEcmaGCStats()->RecordStatisticBeforeGC(TriggerGCType::SHARED_CC, gcReason);
    marker_->ReMark();
    SuspendIdleThreads();
    LogThreadStatesBeforeCopy();
    sHeap_->GetSweeper()->Sweep(true);
    sHeap_->GetSweeper()->PostTask(true);
    PrepareForCopy();
    ProcessWeakReference();
    UpdateRoot();
    marker_->Reset(false);
    EstimatePostCCSize();
    sHeap_->FinishGCTask();
    sHeap_->UpdateGCThresholds(TriggerGCType::SHARED_CC);
    ccRunning_ = true;
}

void SharedCC::FinalizeAndReclaim()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::FinalizeAndReclaim", "");
    TRACE_GC(GCStats::Scope::ScopeId::SuspendAll, sHeap_->GetEcmaGCStats());
    ThreadManagedScope runningScope(dThread_);
    SuspendAllScope scope(dThread_);

    FinalizeCopy();
    sHeap_->FinishGCStats(TriggerGCType::SHARED_CC);

    // Sweep was already prepared and posted in PrepareForCopy (STW1).
    // Ensure async sweep is finished before Reclaim.
    sHeap_->GetSweeper()->TryFillSweptRegion();
    sHeap_->Reclaim(TriggerGCType::SHARED_CC);

    ccRunning_ = false;
    RestoreThreadStates();
    ExitSharedGCScope();
    sHeap_->NotifyGCCompleted();
}

void SharedCC::VerifyHeap()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::VerifyHeap", "");
    ThreadManagedScope runningScope(dThread_);
    SuspendAllScope scope(dThread_);
    SharedHeapVerification(sHeap_, VerifyKind::VERIFY_POST_SHARED_GC).VerifyHeap();
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
        if (pgoProfiler != nullptr) {
            pgoProfiler->ResumeByGC();
        }
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
        thread->SetGcState(false);
#endif
    });
}

void SharedCC::PrepareAllThreads()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::PrepareAllThreads", "");
    TRACE_GC(GCStats::Scope::ScopeId::Initialize, sHeap_->GetEcmaGCStats());
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        if (!thread->HasPostTaskToThreadCallback()) {
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
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *t) {
        t->WithCCStatusLock([&](SharedCCStatus &status) {
            if (status != SharedCCStatus::READY && status != SharedCCStatus::SUSPENDED) {
                if (t->GetLastLeaveFrame() == nullptr) {
                    t->SwitchAllStub(false);
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
        auto *heap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
        heap->ResetTlab();
        thread->SetReadBarrierState(true);
    });

    CollectUpdateRegions();
    SetStringTableCopyOrSweeping(true);

    InstallSharedCCEvacuators();
}

void SharedCC::CollectUpdateRegions()
{
    sharedWorkloads_.clear();
    for (auto *tw : threadWorkloads_) {
        delete tw;
    }
    threadWorkloads_.clear();

    auto collectShared = [this](Region *region) {
        sharedWorkloads_.push_back(region);
    };
    sHeap_->GetNonMovableSpace()->EnumerateRegions(collectShared);
    sHeap_->GetHugeObjectSpace()->EnumerateRegions(collectShared);
    sHeap_->GetAppSpawnSpace()->EnumerateRegions(collectShared);

    Runtime::GetInstance()->GCIterateThreadList([this](JSThread *thread) {
        auto *tw = new ThreadWorkload();
        tw->thread = thread;
        auto *heap = const_cast<Heap*>(thread->GetEcmaVM()->GetHeap());
        heap->EnumerateRegions([tw](Region *region) {
            tw->localRegions.push_back(region);
        });
        int num = static_cast<int>(tw->localRegions.size());
        tw->nextIndex_.store(num - 1, std::memory_order_relaxed);
        tw->remainItems_ = num;
        threadWorkloads_.push_back(tw);
    });
}

void SharedCC::ProcessWeakReference()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::ProcessWeakReference", "");
    TRACE_GC(GCStats::Scope::ScopeId::UpdateWeekRef, sHeap_->GetEcmaGCStats());

    SharedCCEvacuator evacuator(sHeap_, GetTlabAllocator(MAIN_THREAD_INDEX));
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
        thread->IterateWeakEcmaGlobalStorage(weakVisitor, GCKind::SHARED_GC);
        thread->GetEcmaVM()->ProcessSnapShotEnv(weakVisitor);
        thread->ClearVMCachedConstantPool();
    });
}

void SharedCC::InstallSharedCCEvacuators()
{
    evacuators_.clear();
    Runtime::GetInstance()->GCIterateThreadList([this](JSThread *thread) {
        auto *evacuator = new SharedCCEvacuator(SharedHeap::GetInstance());
        evacuators_.push_back({thread, evacuator});
        thread->InstallSharedCCEvacuator(evacuator);
    });
}

void SharedCC::FinalizeSharedCCEvacuators()
{
    for (auto &entry : evacuators_) {
        entry.evacuator->GetTlabAllocator()->Finalize();
        if (entry.thread != nullptr) {
            entry.thread->InstallSharedCCEvacuator(nullptr);
        }
        delete entry.evacuator;
    }
    evacuators_.clear();
}

void SharedCC::UpdateRoot()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::UpdateRoot", "");
    TRACE_GC(GCStats::Scope::ScopeId::UpdateRoot, sHeap_->GetEcmaGCStats());

    SharedCCEvacuator evacuator(sHeap_, GetTlabAllocator(MAIN_THREAD_INDEX));
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
    if (workerCount == 0 && copyTasks_.empty()) {
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
    daemonTask.Run(MAIN_THREAD_INDEX);
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
    uint32_t count = sharedWorkloads_.size();
    for (auto *tw : threadWorkloads_) {
        count += tw->localRegions.size();
    }
    constexpr uint32_t regionPerThread = 8;
    uint32_t maxThreadNum = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    return static_cast<int>(std::min(std::max(1U, count / regionPerThread), maxThreadNum));
}

void SharedCC::OnUpdateFinished()
{
    LockHolder lock(waitMutex_);
    waitCV_.SignalAll();
}

void SharedCC::WaitUpdateFinished()
{
    LockHolder lock(waitMutex_);
    while (runningTaskCount_ > 0) {
        waitCV_.Wait(&waitMutex_);
    }
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
        cc_->OnCopyFinished();
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
    SharedCCUpdateVisitor updateVisitor;

    auto processShared = [&updateVisitor](Region *region) {
        region->IterateAllMarkedBits([&](void *mem) {
            TaggedObject *object = reinterpret_cast<TaggedObject *>(mem);
            JSHClass *jsHclass = object->SynchronizedGetClass();
            ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, jsHclass, updateVisitor);
        });
    };

    auto processLocal = [&updateVisitor](Region *region) {
        region->IterateAllLocalToShareBits([&updateVisitor](void *mem) {
            ObjectSlot slot(ToUintPtr(mem));
            updateVisitor.HandleSlot(slot);
            return true;
        });
    };

    while (true) {
        size_t idx = cc_->sharedIter_.fetch_add(1U, std::memory_order_relaxed);
        if (idx >= cc_->sharedWorkloads_.size()) {
            break;
        }
        processShared(cc_->sharedWorkloads_[idx]);
    }

    for (auto *tw : cc_->threadWorkloads_) {
        int done = 0;
        while (true) {
            int idx = tw->nextIndex_.fetch_sub(1, std::memory_order_relaxed);
            if (idx < 0) {
                break;
            }
            processLocal(tw->localRegions[idx]);
            done++;
        }
        if (done > 0) {
            LockHolder lock(tw->mutex_);
            tw->remainItems_ -= done;
            if (tw->remainItems_ == 0) {
                tw->cv_.SignalAll();
            }
        }
    }

    if (runningTaskCount_.fetch_sub(1, std::memory_order_seq_cst) == 1) {
        cc_->OnUpdateFinished();
    }

    return true;
}

void SharedCC::OnCopyFinished()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::OnCopyFinished", "");
    TRACE_GC(GCStats::Scope::ScopeId::WaitFinish, sHeap_->GetEcmaGCStats());

    LockHolder lock(waitMutex_);
    sHeap_->SetGCState(false);
    waitCV_.SignalAll();
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
        for (auto &entry : evacuators_) {
            entry.evacuator->GetTlabAllocator()->GetSharedLocalSpace()->EnumerateRegions(collectToSpace);
        }
    }

    int workerCount = CalculateUpdateThreadNum();
    runningTaskCount_ = workerCount + 1;

    for (int i = 0; i < workerCount; i++) {
        common::Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<SharedCCUpdateTask>(this, runningTaskCount_));
    }

    SharedCCUpdateTask daemonTask(this, runningTaskCount_);
    daemonTask.Run(MAIN_THREAD_INDEX);
    WaitUpdateFinished();
    sharedWorkloads_.clear();
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
    for (auto *tw : threadWorkloads_) {
        delete tw;
    }
    threadWorkloads_.clear();
    FinishConcurrentStringTableSweep();
}

void SharedCC::RestoreThreadStates()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::RestoreThreadStates", "");
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        thread->SetSharedCCStatus(SharedCCStatus::IDLE);
        thread->SetReadBarrierState(false);
        thread->SwitchAllStub(true);
        thread->ClearCCSuspend();
    });
    Runtime::GetInstance()->IterateAllThreadList([](JSThread *t) {
        if (t->IsJitThread()) {
            t->ClearCCSuspend();
        }
    });
    SetStringTableCopyOrSweeping(false);
}

void SharedCC::PrepareNewThread(JSThread *thread)
{
    if (!ccRunning_) {
        return;
    }
    {
        LockHolder lock(evacuatorsMutex_);
        auto *evacuator = new SharedCCEvacuator(sHeap_);
        evacuators_.push_back({thread, evacuator});
        thread->InstallSharedCCEvacuator(evacuator);
    }
    thread->SwitchAllStub(false);
    thread->SetReadBarrierState(true);
    thread->SetSharedCCStatus(SharedCCStatus::READY);
}

void SharedCC::SkipThreadWorkload(JSThread *thread)
{
    for (auto *tw : threadWorkloads_) {
        if (tw->thread != thread) {
            continue;
        }
        int done = 0;
        while (true) {
            int idx = tw->nextIndex_.fetch_sub(1, std::memory_order_relaxed);
            if (idx < 0) {
                break;
            }
            done++;
        }
        if (done > 0) {
            LockHolder lock(tw->mutex_);
            tw->remainItems_ -= done;
            if (tw->remainItems_ == 0) {
                tw->cv_.SignalAll();
            }
        }
        LockHolder lock(tw->mutex_);
        while (tw->remainItems_ > 0) {
            tw->cv_.Wait(&tw->mutex_);
        }
        return;
    }
}

void SharedCC::Finish()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::Finish", "");
    TRACE_GC(GCStats::Scope::ScopeId::Finish, sHeap_->GetEcmaGCStats());
    sHeap_->InvokeSharedNativePointerCallbacks();
}

void SharedCCUpdateVisitor::VisitObjectRangeImpl(BaseObject *root, uintptr_t start, uintptr_t end,
    VisitObjectArea area)
{
    ObjectSlot startSlot(start);
    ObjectSlot endSlot(end);
    auto rootObject = TaggedObject::Cast(root);

    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        HandleInObjectArea(rootObject, startSlot, endSlot);
        return;
    }

    for (ObjectSlot slot = startSlot; slot < endSlot; slot++) {
        HandleSlot(slot);
    }
}

void SharedCCUpdateVisitor::HandleSlot(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
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
    if (value.IsWeakForHeapObject()) {
        slot.CASUpdateWeak(rawObject, dst);
    } else {
        slot.CASUpdate(rawObject, dst);
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

void SharedCC::PostStringTableSweepTask()
{
    WeakRootVisitor weakVisitor = [](TaggedObject *header) -> TaggedObject* {
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

    auto stringTableCleaner = Runtime::GetInstance()->GetEcmaStringTable()->GetCleaner();
    stringTableCleaner->PostConcurrentSweepWeakRefTask(weakVisitor);
}

void SharedCC::WaitStringTableSweep()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedCC::WaitStringTableSweep", "");
    auto stringTable = Runtime::GetInstance()->GetEcmaStringTable();
    stringTable->WaitConcurrentSweepWeakRefTaskFinished();
}

void SharedCC::FinishConcurrentStringTableSweep()
{
    auto stringTable = Runtime::GetInstance()->GetEcmaStringTable();
    stringTable->FinishConcurrentSweepInSTW(dThread_);
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
}  // namespace panda::ecmascript
