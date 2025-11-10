/*
 * Copyright (c) 2022-2025 Huawei Device Co., Ltd.
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
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif

#include "common_components/taskpool/taskpool.h"
#include "ecmascript/cross_vm/unified_gc/unified_gc.h"
#include "ecmascript/cross_vm/unified_gc/unified_gc_marker.h"
#include "ecmascript/mem/idle_gc_trigger.h"
#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/mem/parallel_evacuator.h"
#include "ecmascript/mem/parallel_marker.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/mem/shared_heap/shared_gc_evacuator.h"
#include "ecmascript/mem/shared_heap/shared_gc_marker-inl.h"
#include "ecmascript/mem/shared_heap/shared_gc.h"
#include "ecmascript/mem/shared_heap/shared_full_gc.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_marker.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/jit/jit.h"
#if !WIN_OR_MAC_OR_IOS_PLATFORM
#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"
#endif
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif
#include "ecmascript/dfx/tracing/tracing.h"
#if defined(ENABLE_DUMP_IN_FAULTLOG)
#include "syspara/parameter.h"
#endif

#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT) && defined(PANDA_TARGET_OHOS) && defined(ENABLE_HISYSEVENT)
#include "parameters.h"
#include "hisysevent.h"
static constexpr uint32_t DEC_TO_INT = 100;
static size_t g_threshold = OHOS::system::GetUintParameter<size_t>("persist.dfx.leak.threshold", 85);
static uint64_t g_lastHeapDumpTime = 0;
static bool g_debugLeak = OHOS::system::GetBoolParameter("debug.dfx.tags.enableleak", false);
static constexpr uint64_t HEAP_DUMP_REPORT_INTERVAL = 24 * 3600 * 1000;
static bool g_betaVersion = OHOS::system::GetParameter("const.logsystem.versiontype", "unknown") == "beta";
static bool g_developMode = (OHOS::system::GetParameter("persist.hiview.leak_detector", "unknown") == "enable") ||
                            (OHOS::system::GetParameter("persist.hiview.leak_detector", "unknown") == "true");
static bool g_futVersion = OHOS::system::GetIntParameter("const.product.dfx.fans.stage", 0) == 1;
#endif

namespace panda::ecmascript {
SharedHeap *SharedHeap::instance_ = nullptr;

void SharedHeap::CreateNewInstance()
{
    ASSERT(instance_ == nullptr);
    size_t heapShared = 0;
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT) && defined(PANDA_TARGET_OHOS) && defined(ENABLE_HISYSEVENT)
    heapShared = OHOS::system::GetUintParameter<size_t>("persist.ark.heap.sharedsize", 0) * 1_MB;
    if (Runtime::GetInstance()->GetEnableLargeHeap()) {
        heapShared = panda::ecmascript::MAX_SHARED_HEAP_SIZE;
    }
#endif
    EcmaParamConfiguration config(EcmaParamConfiguration::HeapType::SHARED_HEAP,
        MemMapAllocator::GetInstance()->GetCapacity(), heapShared);
    instance_ = new SharedHeap(config);
}

SharedHeap *SharedHeap::GetInstance()
{
    ASSERT(instance_ != nullptr);
    return instance_;
}

void SharedHeap::DestroyInstance()
{
    ASSERT(instance_ != nullptr);
    instance_->Destroy();
    delete instance_;
    instance_ = nullptr;
}

void SharedHeap::ForceCollectGarbageWithoutDaemonThread(TriggerGCType gcType, GCReason gcReason, JSThread *thread)
{
    ASSERT(!dThread_->IsRunning());
    SuspendAllScope scope(thread);
    SharedGCScope sharedGCScope;  // SharedGCScope should be after SuspendAllScope.
    RecursionScope recurScope(this, HeapType::SHARED_HEAP);
    CheckInHeapProfiler();
    GetEcmaGCStats()->RecordStatisticBeforeGC(gcType, gcReason);
    if (UNLIKELY(ShouldVerifyHeap())) { // LCOV_EXCL_BR_LINE
        // pre gc heap verify
        LOG_ECMA(DEBUG) << "pre gc shared heap verify";
        sharedGCMarker_->MergeBackAndResetRSetWorkListHandler();
        SharedHeapVerification(this, VerifyKind::VERIFY_PRE_SHARED_GC).VerifyAll();
    }
    switch (gcType) { // LCOV_EXCL_BR_LINE
        case TriggerGCType::SHARED_PARTIAL_GC:
        case TriggerGCType::SHARED_GC: {
            sharedGC_->RunPhases();
            break;
        }
        case TriggerGCType::SHARED_FULL_GC: {
            sharedFullGC_->RunPhases();
            break;
        }
        default: // LOCV_EXCL_BR_LINE
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }
    if (UNLIKELY(ShouldVerifyHeap())) { // LCOV_EXCL_BR_LINE
        // pre gc heap verify
        LOG_ECMA(DEBUG) << "after gc shared heap verify";
        SharedHeapVerification(this, VerifyKind::VERIFY_POST_SHARED_GC).VerifyAll();
    }
    CollectGarbageFinish(false, gcType);
    InvokeSharedNativePointerCallbacks();
}

bool SharedHeap::CheckAndTriggerSharedGC(JSThread *thread)
{
    if (thread->IsSharedConcurrentMarkingOrFinished() && !ObjectExceedMaxHeapSize()) {
        return false;
    }
    size_t sharedGCThreshold = globalSpaceAllocLimit_ + spaceOvershoot_.load(std::memory_order_relaxed);
    if ((OldSpaceExceedLimit() || GetHeapObjectSize() > sharedGCThreshold) &&
        !NeedStopCollection()) {
        CollectGarbage<TriggerGCType::SHARED_GC, GCReason::ALLOCATION_LIMIT>(thread);
        return true;
    }
    return false;
}

bool SharedHeap::CheckHugeAndTriggerSharedGC(JSThread *thread, size_t size)
{
    if (sHugeObjectSpace_->CheckOOM(size)) {
        CollectGarbage<TriggerGCType::SHARED_GC, GCReason::ALLOCATION_LIMIT>(thread);
        return true;
    }
    if (thread->IsSharedConcurrentMarkingOrFinished() && !ObjectExceedMaxHeapSize()) {
        return false;
    }
    if (GetHeapObjectSize() > globalSpaceAllocLimit_ && !NeedStopCollection()) {
        CollectGarbage<TriggerGCType::SHARED_GC, GCReason::ALLOCATION_LIMIT>(thread);
        return true;
    }
    return false;
}

void SharedHeap::CollectGarbageNearOOM(JSThread *thread)
{
    auto fragmentationSize = sOldSpace_->GetCommittedSize() - sOldSpace_->GetHeapObjectSize();
    if (fragmentationSize >= fragmentationLimitForSharedFullGC_) {
        CollectGarbage<TriggerGCType::SHARED_FULL_GC,  GCReason::ALLOCATION_FAILED>(thread);
    } else {
        CollectGarbage<TriggerGCType::SHARED_GC, GCReason::ALLOCATION_FAILED>(thread);
    }
}
// Shared gc trigger
void SharedHeap::AdjustGlobalSpaceAllocLimit()
{
    globalSpaceAllocLimit_ = std::max(GetHeapObjectSize() * growingFactor_,
                                      config_.GetDefaultGlobalAllocLimit());
    globalSpaceAllocLimit_ = std::min(std::min(globalSpaceAllocLimit_, GetCommittedSize() + growingStep_),
                                      config_.GetMaxHeapSize());
    globalSpaceConcurrentMarkLimit_ = static_cast<size_t>(globalSpaceAllocLimit_ *
                                                          TRIGGER_SHARED_CONCURRENT_MARKING_OBJECT_LIMIT_RATE);
    constexpr double OBJECT_INCREMENT_FACTOR_FOR_MARK_LIMIT = 1.1;
    size_t markLimitByIncrement = static_cast<size_t>(GetHeapObjectSize() * OBJECT_INCREMENT_FACTOR_FOR_MARK_LIMIT);
    globalSpaceConcurrentMarkLimit_ = std::max(globalSpaceConcurrentMarkLimit_, markLimitByIncrement);
    LOG_ECMA_IF(optionalLogEnabled_, INFO) << "Shared gc adjust global space alloc limit to: "
        << globalSpaceAllocLimit_;
}

bool SharedHeap::ObjectExceedMaxHeapSize() const
{
    return OldSpaceExceedLimit() || sHugeObjectSpace_->CommittedSizeExceed();
}

void SharedHeap::StartConcurrentMarking(TriggerGCType gcType, MarkReason markReason)
{
    ASSERT(JSThread::GetCurrent() == dThread_);
    GetEcmaGCStats()->SetMarkReason(markReason);
    sConcurrentMarker_->Mark(gcType);
}

bool SharedHeap::CheckCanTriggerConcurrentMarking(JSThread *thread)
{
    return thread->IsReadyToSharedConcurrentMark() &&
           sConcurrentMarker_ != nullptr && sConcurrentMarker_->IsEnabled();
}

void SharedHeap::Initialize(NativeAreaAllocator *nativeAreaAllocator, HeapRegionAllocator *heapRegionAllocator,
    const JSRuntimeOptions &option, DaemonThread *dThread)
{
    sGCStats_ = new SharedGCStats(this, option.EnableGCTracer());
    nativeAreaAllocator_ = nativeAreaAllocator;
    heapRegionAllocator_ = heapRegionAllocator;
    shouldVerifyHeap_ = option.EnableHeapVerify();
    parallelGC_ = option.EnableParallelGC();
    optionalLogEnabled_ = option.EnableOptionalLog();
    size_t maxHeapSize = config_.GetMaxHeapSize();
    size_t nonmovableSpaceCapacity = config_.GetDefaultNonMovableSpaceSize();
    sNonMovableSpace_ = new SharedNonMovableSpace(this, nonmovableSpaceCapacity, nonmovableSpaceCapacity);

    size_t readOnlySpaceCapacity = config_.GetDefaultReadOnlySpaceSize();
    size_t oldSpaceCapacity =
        AlignUp((maxHeapSize - nonmovableSpaceCapacity - readOnlySpaceCapacity) / 2, DEFAULT_REGION_SIZE); // 2: half
    globalSpaceAllocLimit_ = config_.GetDefaultGlobalAllocLimit();
    globalSpaceConcurrentMarkLimit_ = static_cast<size_t>(globalSpaceAllocLimit_ *
                                                          TRIGGER_SHARED_CONCURRENT_MARKING_OBJECT_LIMIT_RATE);

    sOldSpace_ = new SharedOldSpace(this, oldSpaceCapacity, oldSpaceCapacity);
    sCompressSpace_ = new SharedOldSpace(this, oldSpaceCapacity, oldSpaceCapacity);
    sReadOnlySpace_ = new SharedReadOnlySpace(this, readOnlySpaceCapacity, readOnlySpaceCapacity);
    sHugeObjectSpace_ = new SharedHugeObjectSpace(this, heapRegionAllocator_, oldSpaceCapacity, oldSpaceCapacity);
    sharedMemController_ = new SharedMemController(this);
    externalStringTable_ = new ExternalStringTable();
    sAppSpawnSpace_ = new SharedAppSpawnSpace(this, oldSpaceCapacity);
    growingFactor_ = config_.GetSharedHeapLimitGrowingFactor();
    growingStep_ = config_.GetSharedHeapLimitGrowingStep();
    incNativeSizeTriggerSharedCM_= config_.GetStepNativeSizeInc();
    incNativeSizeTriggerSharedGC_ = config_.GetMaxNativeSizeInc();
    fragmentationLimitForSharedFullGC_ = config_.GetFragmentationLimitForSharedFullGC();
    dThread_ = dThread;
}

void SharedHeap::Destroy()
{
    if (sWorkManager_ != nullptr) {
        delete sWorkManager_;
        sWorkManager_ = nullptr;
    }
    if (sOldSpace_ != nullptr) {
        sOldSpace_->Reset();
        delete sOldSpace_;
        sOldSpace_ = nullptr;
    }
    if (sCompressSpace_ != nullptr) {
        sCompressSpace_->Reset();
        delete sCompressSpace_;
        sCompressSpace_ = nullptr;
    }
    if (sNonMovableSpace_ != nullptr) {
        sNonMovableSpace_->Reset();
        delete sNonMovableSpace_;
        sNonMovableSpace_ = nullptr;
    }
    if (sHugeObjectSpace_ != nullptr) {
        sHugeObjectSpace_->Destroy();
        delete sHugeObjectSpace_;
        sHugeObjectSpace_ = nullptr;
    }
    if (sReadOnlySpace_ != nullptr) {
        sReadOnlySpace_->ClearReadOnly();
        sReadOnlySpace_->Destroy();
        delete sReadOnlySpace_;
        sReadOnlySpace_ = nullptr;
    }
    if (sAppSpawnSpace_ != nullptr) {
        sAppSpawnSpace_->Reset();
        delete sAppSpawnSpace_;
        sAppSpawnSpace_ = nullptr;
    }
    if (sharedGC_ != nullptr) {
        delete sharedGC_;
        sharedGC_ = nullptr;
    }
    if (sharedFullGC_ != nullptr) {
        delete sharedFullGC_;
        sharedFullGC_ = nullptr;
    }
    if (sEvacuator_ != nullptr) {
        delete sEvacuator_;
        sEvacuator_ = nullptr;
    }
    nativeAreaAllocator_ = nullptr;
    heapRegionAllocator_ = nullptr;

    if (sSweeper_ != nullptr) {
        delete sSweeper_;
        sSweeper_ = nullptr;
    }
    if (sConcurrentMarker_ != nullptr) {
        delete sConcurrentMarker_;
        sConcurrentMarker_ = nullptr;
    }
    if (sharedGCMarker_ != nullptr) {
        delete sharedGCMarker_;
        sharedGCMarker_ = nullptr;
    }
    if (sharedGCMovableMarker_ != nullptr) {
        delete sharedGCMovableMarker_;
        sharedGCMovableMarker_ = nullptr;
    }
    if (sharedMemController_ != nullptr) {
        delete sharedMemController_;
        sharedMemController_ = nullptr;
    }
    if (externalStringTable_ != nullptr) {
        delete externalStringTable_;
        externalStringTable_ = nullptr;
    }
    if (Runtime::GetInstance()->IsHybridVm() && unifiedGC_ != nullptr) {
        delete unifiedGC_;
        unifiedGC_ = nullptr;
    }

    dThread_ = nullptr;
}

void SharedHeap::PostInitialization(const GlobalEnvConstants *globalEnvConstants, const JSRuntimeOptions &option)
{
    globalEnvConstants_ = globalEnvConstants;
    uint32_t totalThreadNum = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    maxMarkTaskCount_ = totalThreadNum - 1;
    sWorkManager_ = new SharedGCWorkManager(this, totalThreadNum + 1);
    sharedGCMarker_ = new SharedGCMarker(sWorkManager_);
    sharedGCMovableMarker_ = new SharedGCMovableMarker(sWorkManager_, this);
    sConcurrentMarker_ = new SharedConcurrentMarker(option.EnableSharedConcurrentMark() ?
        EnableConcurrentMarkType::ENABLE : EnableConcurrentMarkType::CONFIG_DISABLE);
    sSweeper_ = new SharedConcurrentSweeper(this, option.EnableConcurrentSweep() ?
        EnableConcurrentSweepType::ENABLE : EnableConcurrentSweepType::CONFIG_DISABLE);
    sharedGC_ = new SharedGC(this);
    sEvacuator_ = new SharedGCEvacuator(this);
    sharedFullGC_ = new SharedFullGC(this);
    if (Runtime::GetInstance()->IsHybridVm()) {
        unifiedGC_ = new UnifiedGC();
    }
}

void SharedHeap::PostGCMarkingTask(SharedParallelMarkPhase sharedTaskPhase)
{
    IncreaseTaskCount();
    common::Taskpool::GetCurrentTaskpool()->PostTask(std::make_unique<ParallelMarkTask>(dThread_->GetThreadId(),
                                                                                this, sharedTaskPhase));
}

bool SharedHeap::ParallelMarkTask::Run(uint32_t threadIndex)
{
    // Synchronizes-with. Ensure that WorkManager::Initialize must be seen by MarkerThreads.
    while (!sHeap_->GetWorkManager()->HasInitialized());
    switch (taskPhase_) {
        case SharedParallelMarkPhase::SHARED_MARK_TASK:
            sHeap_->GetSharedGCMarker()->ProcessMarkStack(threadIndex);
            break;
        case SharedParallelMarkPhase::SHARED_COMPRESS_TASK:
            sHeap_->GetSharedGCMovableMarker()->ProcessMarkStack(threadIndex);
            break;
        default: // LOCV_EXCL_BR_LINE
            break;
    }
    sHeap_->ReduceTaskCount();
    return true;
}

bool SharedHeap::AsyncClearTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedHeap::AsyncClearTask::Run", "");
    sHeap_->ReclaimRegions(gcType_);
    return true;
}

void SharedHeap::NotifyGCCompleted()
{
    ASSERT(JSThread::GetCurrent() == dThread_);
    LockHolder lock(waitGCFinishedMutex_);
    gcFinished_ = true;
    waitGCFinishedCV_.SignalAll();
}

void SharedHeap::WaitGCFinished(JSThread *thread)
{
    ASSERT(thread->GetThreadId() != dThread_->GetThreadId());
    ASSERT(thread->IsInRunningState());
    ThreadSuspensionScope scope(thread);
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SuspendTime::WaitGCFinished", "");
    LockHolder lock(waitGCFinishedMutex_);
    while (!gcFinished_) {
        waitGCFinishedCV_.Wait(&waitGCFinishedMutex_);
    }
}

void SharedHeap::WaitGCFinishedAfterAllJSThreadEliminated()
{
    ASSERT(Runtime::GetInstance()->vmCount_ == 0);
    LockHolder lock(waitGCFinishedMutex_);
    while (!gcFinished_) {
        waitGCFinishedCV_.Wait(&waitGCFinishedMutex_);
    }
}

void SharedHeap::DaemonCollectGarbage([[maybe_unused]]TriggerGCType gcType, [[maybe_unused]]GCReason gcReason)
{
    RecursionScope recurScope(this, HeapType::SHARED_HEAP);
    ASSERT(gcType == TriggerGCType::SHARED_GC || gcType == TriggerGCType::SHARED_PARTIAL_GC ||
        gcType == TriggerGCType::SHARED_FULL_GC);
    ASSERT(JSThread::GetCurrent() == dThread_);
    {
        ThreadManagedScope runningScope(dThread_);
        SuspendAllScope scope(dThread_);
        SharedGCScope sharedGCScope;  // SharedGCScope should be after SuspendAllScope.
        CheckInHeapProfiler();
        gcType_ = gcType;
        GetEcmaGCStats()->RecordStatisticBeforeGC(gcType, gcReason);
        if (UNLIKELY(ShouldVerifyHeap())) { // LCOV_EXCL_BR_LINE
            // pre gc heap verify
            LOG_ECMA(DEBUG) << "pre gc shared heap verify";
            sharedGCMarker_->MergeBackAndResetRSetWorkListHandler();
            SharedHeapVerification(this, VerifyKind::VERIFY_PRE_SHARED_GC).VerifyAll();
        }
        switch (gcType) {
            case TriggerGCType::SHARED_PARTIAL_GC:
            case TriggerGCType::SHARED_GC: {
                sharedGC_->RunPhases();
                break;
            }
            case TriggerGCType::SHARED_FULL_GC: {
                sharedFullGC_->RunPhases();
                break;
            }
            default: // LOCV_EXCL_BR_LINE
                LOG_ECMA(FATAL) << "this branch is unreachable";
                UNREACHABLE();
                break;
        }

        if (UNLIKELY(ShouldVerifyHeap())) { // LCOV_EXCL_BR_LINE
            // after gc heap verify
            LOG_ECMA(DEBUG) << "after gc shared heap verify";
            SharedHeapVerification(this, VerifyKind::VERIFY_POST_SHARED_GC).VerifyAll();
        }
        CollectGarbageFinish(true, gcType);
    }
    InvokeSharedNativePointerCallbacks();
    // Don't process weak node nativeFinalizeCallback here. These callbacks would be called after localGC.
}

void SharedHeap::WaitAllTasksFinished(JSThread *thread)
{
    WaitGCFinished(thread);
    sSweeper_->WaitAllTaskFinished();
    WaitClearTaskFinished();
}

void SharedHeap::WaitAllTasksFinishedAfterAllJSThreadEliminated()
{
    WaitGCFinishedAfterAllJSThreadEliminated();
    sSweeper_->WaitAllTaskFinished();
    WaitClearTaskFinished();
}

bool SharedHeap::CheckOngoingConcurrentMarking()
{
    if (sConcurrentMarker_->IsEnabled() && !dThread_->IsReadyToConcurrentMark() &&
        sConcurrentMarker_->IsTriggeredConcurrentMark()) {
        // This is only called in SharedGC to decide whether to remark, so do not need to wait marking finish here
        return true;
    }
    return false;
}

void SharedHeap::CheckInHeapProfiler()
{
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    Runtime::GetInstance()->GCIterateThreadList([this](JSThread *thread) {
        if (thread->GetEcmaVM()->GetHeapProfile() != nullptr) {
            inHeapProfiler_ = true;
            return;
        }
    });
#else
    inHeapProfiler_ = false;
#endif
}

void SharedHeap::Prepare(bool inTriggerGCThread)
{
    WaitRunningTaskFinished();
    if (inTriggerGCThread) {
        sSweeper_->EnsureAllTaskFinished();
    } else {
        sSweeper_->WaitAllTaskFinished();
    }
    WaitClearTaskFinished();
}

SharedHeap::SharedGCScope::SharedGCScope()
{
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        std::shared_ptr<pgo::PGOProfiler> pgoProfiler =  thread->GetEcmaVM()->GetPGOProfiler();
        if (pgoProfiler != nullptr) {
            pgoProfiler->SuspendByGC();
        }
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
        thread->SetGcState(true);
#endif
    });
}

SharedHeap::SharedGCScope::~SharedGCScope()
{
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->ProcessGCListeners();
        std::shared_ptr<pgo::PGOProfiler> pgoProfiler =  thread->GetEcmaVM()->GetPGOProfiler();
        if (pgoProfiler != nullptr) {
            pgoProfiler->ResumeByGC();
        }
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
        thread->SetGcState(false);
#endif
    });
}

void SharedHeap::PrepareRecordRegionsForReclaim()
{
    sOldSpace_->SetRecordRegion();
    sNonMovableSpace_->SetRecordRegion();
    sHugeObjectSpace_->SetRecordRegion();
}

void SharedHeap::Reclaim(TriggerGCType gcType)
{
    PrepareRecordRegionsForReclaim();
    sHugeObjectSpace_->ReclaimHugeRegion();

    if (parallelGC_) {
        clearTaskFinished_ = false;
        common::Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<AsyncClearTask>(dThread_->GetThreadId(), this, gcType));
    } else {
        ReclaimRegions(gcType);
    }
}

void SharedHeap::ReclaimRegions(TriggerGCType gcType)
{
    if (gcType == TriggerGCType::SHARED_FULL_GC) {
        sCompressSpace_->Reset();
    }
    sOldSpace_->ReclaimCSets();
    sSweeper_->WaitAllTaskFinished();
    EnumerateOldSpaceRegionsWithRecord([] (Region *region) {
        region->ClearMarkGCBitset();
        region->ClearCrossRegionRSet();
    });
    if (!clearTaskFinished_) {
        LockHolder holder(waitClearTaskFinishedMutex_);
        clearTaskFinished_ = true;
        waitClearTaskFinishedCV_.SignalAll();
    }
}

void SharedHeap::DisableParallelGC(JSThread *thread)
{
    WaitAllTasksFinished(thread);
    dThread_->WaitFinished();
    parallelGC_ = false;
    maxMarkTaskCount_ = 0;
    sSweeper_->ConfigConcurrentSweep(false);
    sConcurrentMarker_->ConfigConcurrentMark(false);
}

void SharedHeap::EnableParallelGC(JSRuntimeOptions &option)
{
    uint32_t totalThreadNum = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    maxMarkTaskCount_ = totalThreadNum - 1;
    parallelGC_ = option.EnableParallelGC();
    if (auto workThreadNum = sWorkManager_->GetTotalThreadNum();
        workThreadNum != totalThreadNum + 1) {
        LOG_ECMA_MEM(ERROR) << "TheadNum mismatch, totalThreadNum(sWorkerManager): " << workThreadNum << ", "
                            << "totalThreadNum(taskpool): " << (totalThreadNum + 1);
        delete sWorkManager_;
        sWorkManager_ = new SharedGCWorkManager(this, totalThreadNum + 1);
        UpdateWorkManager(sWorkManager_);
    }
    sConcurrentMarker_->ConfigConcurrentMark(option.EnableSharedConcurrentMark());
    sSweeper_->ConfigConcurrentSweep(option.EnableConcurrentSweep());
}

void SharedHeap::UpdateWorkManager(SharedGCWorkManager *sWorkManager)
{
    sConcurrentMarker_->ResetWorkManager(sWorkManager);
    sharedGCMarker_->ResetWorkManager(sWorkManager);
    sharedGCMovableMarker_->ResetWorkManager(sWorkManager);
    sharedGC_->ResetWorkManager(sWorkManager);
    sharedFullGC_->ResetWorkManager(sWorkManager);
}

void SharedHeap::TryTriggerLocalConcurrentMarking()
{
    if (localFullMarkTriggered_) {
        return;
    }
    if (reinterpret_cast<std::atomic<bool>*>(&localFullMarkTriggered_)->exchange(true, std::memory_order_relaxed)
            != false) { // LCOV_EXCL_BR_LINE
        return;
    }
    ASSERT(localFullMarkTriggered_ == true);
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        thread->SetFullMarkRequest();
    });
}

size_t SharedHeap::VerifyHeapObjects(VerifyKind verifyKind) const
{
    size_t failCount = 0;
    {
        VerifyObjectVisitor verifier(this, &failCount, verifyKind);
        sOldSpace_->IterateOverObjects(verifier);
    }
    {
        VerifyObjectVisitor verifier(this, &failCount, verifyKind);
        sNonMovableSpace_->IterateOverObjects(verifier);
    }
    {
        VerifyObjectVisitor verifier(this, &failCount, verifyKind);
        sHugeObjectSpace_->IterateOverObjects(verifier);
    }
    {
        VerifyObjectVisitor verifier(this, &failCount, verifyKind);
        sAppSpawnSpace_->IterateOverMarkedObjects(verifier);
    }
    return failCount;
}

void SharedHeap::CollectGarbageFinish(bool inDaemon, TriggerGCType gcType)
{
    if (inDaemon) {
        ASSERT(JSThread::GetCurrent() == dThread_);
#ifndef NDEBUG
        ASSERT(dThread_->HasLaunchedSuspendAll());
#endif
        dThread_->FinishRunningTask();
        NotifyGCCompleted();
        // Update to forceGC_ is in DaemeanSuspendAll, and protected by the Runtime::mutatorLock_,
        // so do not need lock.
        smartGCStats_.forceGC_ = false;
    }
    localFullMarkTriggered_ = false;
    // Record alive object size after shared gc and other stats
    UpdateHeapStatsAfterGC(gcType);
    // Adjust shared gc trigger threshold
    AdjustGlobalSpaceAllocLimit();
    spaceOvershoot_.store(0, std::memory_order_relaxed);
    GetEcmaGCStats()->RecordStatisticAfterGC();
    GetEcmaGCStats()->PrintGCStatistic();
    ProcessAllGCListeners();
    if (shouldThrowOOMError_ || shouldForceThrowOOMError_) {
        // LocalHeap could do FullGC later instead of Fatal at once if only set `shouldThrowOOMError_` because there
        // is kind of partial compress GC in LocalHeap, but SharedHeap differs.
        DumpHeapSnapshotBeforeOOM(Runtime::GetInstance()->GetMainThread(), SharedHeapOOMSource::SHARED_GC);
        LOG_GC(FATAL) << "SharedHeap OOM";
        UNREACHABLE();
    }
}

void SharedHeap::SetGCThreadQosPriority(common::PriorityMode mode)
{
    dThread_->SetQosPriority(mode);
    common::Taskpool::GetCurrentTaskpool()->SetThreadPriority(mode);
}

bool SharedHeap::IsReadyToConcurrentMark() const
{
    return dThread_->IsReadyToConcurrentMark();
}

bool SharedHeap::ObjectExceedJustFinishStartupThresholdForGC() const
{
    size_t heapObjectSizeThresholdForGC = config_.GetMaxHeapSize() * JUST_FINISH_STARTUP_SHARED_THRESHOLD_RATIO;
    return ObjectExceedMaxHeapSize() || GetHeapObjectSize() > heapObjectSizeThresholdForGC;
}

bool SharedHeap::ObjectExceedJustFinishStartupThresholdForCM() const
{
    size_t heapObjectSizeThresholdForGC = config_.GetMaxHeapSize() * JUST_FINISH_STARTUP_SHARED_THRESHOLD_RATIO;
    size_t heapObjectSizeThresholdForCM = heapObjectSizeThresholdForGC
                                        * JUST_FINISH_STARTUP_SHARED_CONCURRENT_MARK_RATIO;
    return ObjectExceedMaxHeapSize() || GetHeapObjectSize() > heapObjectSizeThresholdForCM;
}

bool SharedHeap::CheckIfNeedStopCollectionByStartup()
{
    StartupStatus startupStatus = GetStartupStatus();
    switch (startupStatus) {
        case StartupStatus::ON_STARTUP:
            if (!ObjectExceedMaxHeapSize()) {
                return true;
            }
            break;
        case StartupStatus::JUST_FINISH_STARTUP:
            if (!ObjectExceedJustFinishStartupThresholdForGC()) {
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

bool SharedHeap::NeedStopCollection()
{
    if (CheckIfNeedStopCollectionByStartup()) {
        return true;
    }

    if (!InSensitiveStatus()) {
        return false;
    }

    if (!ObjectExceedMaxHeapSize()) {
        return true;
    }
    return false;
}

void SharedHeap::TryAdjustSpaceOvershootByConfigSize()
{
    if (InGC() || !IsReadyToConcurrentMark()) {
        // no need to reserve space if SharedGC or SharedConcurrentMark is already triggered
        return;
    }
    // set overshoot size to increase gc threashold larger 8MB than current heap size.
    int64_t heapObjectSize = static_cast<int64_t>(GetHeapObjectSize());
    int64_t remainSizeBeforeGC = static_cast<int64_t>(globalSpaceAllocLimit_) - heapObjectSize;
    int64_t overshootSize = static_cast<int64_t>(config_.GetOldSpaceStepOvershootSize()) - remainSizeBeforeGC;
    // overshoot size should be larger than 0.
    spaceOvershoot_.store(std::max(overshootSize, (int64_t)0), std::memory_order_relaxed);
}

void SharedHeap::CompactHeapBeforeFork(JSThread *thread)
{
    ThreadManagedScope managedScope(thread);
    WaitGCFinished(thread);
    sharedFullGC_->SetForAppSpawn(true);
    CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::OTHER>(thread);
    sharedFullGC_->SetForAppSpawn(false);
}

void SharedHeap::MoveOldSpaceToAppspawn()
{
    auto committedSize = sOldSpace_->GetCommittedSize();
    sAppSpawnSpace_->SetInitialCapacity(committedSize);
    sAppSpawnSpace_->SetMaximumCapacity(committedSize);
    sOldSpace_->SetInitialCapacity(sOldSpace_->GetInitialCapacity() - committedSize);
    sOldSpace_->SetMaximumCapacity(sOldSpace_->GetMaximumCapacity() - committedSize);
    sCompressSpace_->SetInitialCapacity(sOldSpace_->GetInitialCapacity());
    sCompressSpace_->SetMaximumCapacity(sOldSpace_->GetMaximumCapacity());
#ifdef ECMASCRIPT_SUPPORT_HEAPSAMPLING
    sAppSpawnSpace_->SwapAllocationCounter(sOldSpace_);
#endif
    sOldSpace_->EnumerateRegions([&](Region *region) {
        region->SetRegionSpaceFlag(RegionSpaceFlag::IN_SHARED_APPSPAWN_SPACE);
        // Region in SharedHeap do not need PageTag threadId.
        PageTag(region, region->GetCapacity(), PageTagType::HEAP, region->GetSpaceTypeName());
        sAppSpawnSpace_->AddRegion(region);
        sAppSpawnSpace_->IncreaseLiveObjectSize(region->AliveObject());
    });
    sOldSpace_->GetRegionList().Clear();
    sOldSpace_->Reset();
}

void SharedHeap::ReclaimForAppSpawn()
{
    sSweeper_->WaitAllTaskFinished();
    sHugeObjectSpace_->ReclaimHugeRegion();
    sCompressSpace_->Reset();
    MoveOldSpaceToAppspawn();
    auto cb = [] (Region *region) {
        region->ClearMarkGCBitset();
    };
    sNonMovableSpace_->EnumerateRegions(cb);
    sHugeObjectSpace_->EnumerateRegions(cb);
}

void SharedHeap::DumpHeapSnapshotBeforeOOM([[maybe_unused]]JSThread *thread,
                                           [[maybe_unused]] SharedHeapOOMSource source)
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT) && defined(ENABLE_DUMP_IN_FAULTLOG)
    AppFreezeFilterCallback appfreezeCallback = Runtime::GetInstance()->GetAppFreezeFilterCallback();
    std::string eventConfig;
    bool shouldDump = (appfreezeCallback == nullptr || appfreezeCallback(getprocpid(), true, eventConfig));
    EcmaVM *vm = thread->GetEcmaVM();
    vm->GetEcmaGCKeyStats()->SendSysEventBeforeDump("OOMDump", GetEcmaParamConfiguration().GetMaxHeapSize(),
                                                    GetHeapObjectSize(), eventConfig);
    if (!shouldDump) {
        LOG_ECMA(INFO) << "SharedHeap::DumpHeapSnapshotBeforeOOM, no dump quota.";
        return;
    }
#endif
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
#if defined(ENABLE_DUMP_IN_FAULTLOG)
    HeapProfilerInterface *heapProfile = nullptr;
    if (source == SharedHeapOOMSource::SHARED_GC) {
#ifndef NDEBUG
        // If OOM during SharedGC, use main JSThread and create a new HeapProfile instancre to dump when GC completed.
        ASSERT(thread == Runtime::GetInstance()->GetMainThread() && JSThread::GetCurrent()->HasLaunchedSuspendAll());
#endif
        heapProfile = HeapProfilerInterface::CreateNewInstance(vm);
    } else {
        if (vm->GetHeapProfile() != nullptr) {
            LOG_ECMA(ERROR) << "SharedHeap::DumpHeapSnapshotBeforeOOM, HeapProfile is nullptr";
            return;
        }
        heapProfile = HeapProfilerInterface::GetInstance(vm);
    }
    // Filter appfreeze when dump.
    LOG_ECMA(INFO) << "SharedHeap::DumpHeapSnapshotBeforeOOM, trigger oom dump";
    base::BlockHookScope blockScope;
    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = DumpFormat::BINARY;
    dumpOption.isVmMode = true;
    dumpOption.isPrivate = false;
    dumpOption.captureNumericValue = false;
    dumpOption.isFullGC = false;
    dumpOption.isSimplify = true;
    dumpOption.isSync = true;
    dumpOption.isBeforeFill = false;
    dumpOption.isDumpOOM = true;
    if (source == SharedHeapOOMSource::SHARED_GC) {
        heapProfile->DumpHeapSnapshotForOOM(dumpOption, true);
        HeapProfilerInterface::DestroyInstance(heapProfile);
    } else {
        heapProfile->DumpHeapSnapshotForOOM(dumpOption);
        HeapProfilerInterface::Destroy(vm);
    }
#endif // ENABLE_DUMP_IN_FAULTLOG
#endif // ECMASCRIPT_SUPPORT_SNAPSHOT
}

Heap::Heap(EcmaVM *ecmaVm)
    : BaseHeap(ecmaVm->GetEcmaParamConfiguration()),
      ecmaVm_(ecmaVm), thread_(ecmaVm->GetJSThread()), sHeap_(SharedHeap::GetInstance()) {}

void Heap::Initialize()
{
    enablePageTagThreadId_ = ecmaVm_->GetJSOptions().EnablePageTagThreadId();
    memController_ = new MemController(this);
    nativeAreaAllocator_ = ecmaVm_->GetNativeAreaAllocator();
    heapRegionAllocator_ = ecmaVm_->GetHeapRegionAllocator();
    size_t maxHeapSize = config_.GetMaxHeapSize();
    size_t minSemiSpaceCapacity = config_.GetMinSemiSpaceSize();
    size_t maxSemiSpaceCapacity = config_.GetMaxSemiSpaceSize();
    activeSemiSpace_ = new SemiSpace(this, minSemiSpaceCapacity, maxSemiSpaceCapacity);
    activeSemiSpace_->Restart();
    activeSemiSpace_->SetWaterLine();

    auto topAddress = activeSemiSpace_->GetAllocationTopAddress();
    auto endAddress = activeSemiSpace_->GetAllocationEndAddress();
    thread_->ReSetNewSpaceAllocationAddress(topAddress, endAddress);
    sOldTlab_ = new ThreadLocalAllocationBuffer(this);
    thread_->ReSetSOldSpaceAllocationAddress(sOldTlab_->GetTopAddress(), sOldTlab_->GetEndAddress());
    sNonMovableTlab_ = new ThreadLocalAllocationBuffer(this);
    thread_->ReSetSNonMovableSpaceAllocationAddress(sNonMovableTlab_->GetTopAddress(),
                                                    sNonMovableTlab_->GetEndAddress());
    inactiveSemiSpace_ = new SemiSpace(this, minSemiSpaceCapacity, maxSemiSpaceCapacity);

    // whether should verify heap duration gc
    shouldVerifyHeap_ = ecmaVm_->GetJSOptions().EnableHeapVerify();
    // not set up from space

    size_t readOnlySpaceCapacity = config_.GetDefaultReadOnlySpaceSize();
    readOnlySpace_ = new ReadOnlySpace(this, readOnlySpaceCapacity, readOnlySpaceCapacity);
    appSpawnSpace_ = new AppSpawnSpace(this, maxHeapSize);
    size_t nonmovableSpaceCapacity = config_.GetDefaultNonMovableSpaceSize();
    if (ecmaVm_->GetJSOptions().WasSetMaxNonmovableSpaceCapacity()) { // LCOV_EXCL_BR_LINE
        nonmovableSpaceCapacity = ecmaVm_->GetJSOptions().MaxNonmovableSpaceCapacity();
    }
    nonMovableSpace_ = new NonMovableSpace(this, nonmovableSpaceCapacity, nonmovableSpaceCapacity);
    if (!g_isEnableCMCGC) {
        nonMovableSpace_->Initialize();
    }
    size_t snapshotSpaceCapacity = config_.GetDefaultSnapshotSpaceSize();
    snapshotSpace_ = new SnapshotSpace(this, snapshotSpaceCapacity, snapshotSpaceCapacity);
    size_t machineCodeSpaceCapacity = config_.GetDefaultMachineCodeSpaceSize();
    machineCodeSpace_ = new MachineCodeSpace(this, machineCodeSpaceCapacity, machineCodeSpaceCapacity);

    size_t capacities = minSemiSpaceCapacity * 2 + nonmovableSpaceCapacity + snapshotSpaceCapacity +
        machineCodeSpaceCapacity + readOnlySpaceCapacity;
    if (maxHeapSize < capacities || maxHeapSize - capacities < MIN_OLD_SPACE_LIMIT) { // LOCV_EXCL_BR_LINE
        LOG_ECMA_MEM(FATAL) << "HeapSize is too small to initialize oldspace, heapSize = " << maxHeapSize;
    }
    size_t oldSpaceCapacity = maxHeapSize - capacities;
    globalSpaceAllocLimit_ = maxHeapSize - minSemiSpaceCapacity;
    globalSpaceNativeLimit_ = INIT_GLOBAL_SPACE_NATIVE_SIZE_LIMIT;
    oldSpace_ = new OldSpace(this, oldSpaceCapacity, oldSpaceCapacity);
    compressSpace_ = new OldSpace(this, oldSpaceCapacity, oldSpaceCapacity);
    oldSpace_->Initialize();

    hugeObjectSpace_ = new HugeObjectSpace(this, heapRegionAllocator_, oldSpaceCapacity, oldSpaceCapacity);
    hugeMachineCodeSpace_ = new HugeMachineCodeSpace(this, heapRegionAllocator_, oldSpaceCapacity, oldSpaceCapacity);
    maxEvacuateTaskCount_ = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    maxMarkTaskCount_ = std::min<size_t>(ecmaVm_->GetJSOptions().GetGcThreadNum(),
        maxEvacuateTaskCount_ - 1);

    LOG_GC(DEBUG) << "heap initialize: heap size = " << (maxHeapSize / 1_MB) << "MB"
                 << ", semispace capacity = " << (minSemiSpaceCapacity / 1_MB) << "MB"
                 << ", nonmovablespace capacity = " << (nonmovableSpaceCapacity / 1_MB) << "MB"
                 << ", snapshotspace capacity = " << (snapshotSpaceCapacity / 1_MB) << "MB"
                 << ", machinecodespace capacity = " << (machineCodeSpaceCapacity / 1_MB) << "MB"
                 << ", oldspace capacity = " << (oldSpaceCapacity / 1_MB) << "MB"
                 << ", globallimit = " << (globalSpaceAllocLimit_ / 1_MB) << "MB"
                 << ", gcThreadNum = " << maxMarkTaskCount_;
    parallelGC_ = ecmaVm_->GetJSOptions().EnableParallelGC();
    bool concurrentMarkerEnabled = ecmaVm_->GetJSOptions().EnableConcurrentMark();
    markType_ = MarkType::MARK_YOUNG;
#if ECMASCRIPT_DISABLE_CONCURRENT_MARKING
    concurrentMarkerEnabled = false;
#endif
    workManager_ = new WorkManager(this, common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() + 1);
    fullGC_ = new FullGC(this);

    partialGC_ = new PartialGC(this);
    sweeper_ = new ConcurrentSweeper(this, ecmaVm_->GetJSOptions().EnableConcurrentSweep() ?
        EnableConcurrentSweepType::ENABLE : EnableConcurrentSweepType::CONFIG_DISABLE);
    concurrentMarker_ = new ConcurrentMarker(this, concurrentMarkerEnabled ? EnableConcurrentMarkType::ENABLE :
        EnableConcurrentMarkType::CONFIG_DISABLE);
    nonMovableMarker_ = new NonMovableMarker(this);
    compressGCMarker_ = new CompressGCMarker(this);
    if (Runtime::GetInstance()->IsHybridVm()) {
        unifiedGCMarker_ = new UnifiedGCMarker(this);
    }
    evacuator_ = new ParallelEvacuator(this);
    incrementalMarker_ = new IncrementalMarker(this);
    gcListeners_.reserve(16U);
    nativeSizeTriggerGCThreshold_ = config_.GetMaxNativeSizeInc();
    incNativeSizeTriggerGC_ = config_.GetStepNativeSizeInc();
    nativeSizeOvershoot_ = config_.GetNativeSizeOvershoot();
    asyncClearNativePointerThreshold_ = config_.GetAsyncClearNativePointerThreshold();
    idleGCTrigger_ = new IdleGCTrigger(this, sHeap_, thread_, GetEcmaVM()->GetJSOptions().EnableOptionalLog());
}

void Heap::ResetLargeCapacity()
{
    size_t minSemiSpaceCapacity = config_.GetMinSemiSpaceSize();
    size_t readOnlySpaceCapacity = config_.GetDefaultReadOnlySpaceSize();
    size_t nonMovableSpaceCapacity = config_.GetDefaultNonMovableSpaceSize();
    if (ecmaVm_->GetJSOptions().WasSetMaxNonmovableSpaceCapacity()) {
        nonMovableSpaceCapacity = ecmaVm_->GetJSOptions().MaxNonmovableSpaceCapacity();
    }
    size_t machineCodeSpaceCapacity = config_.GetDefaultMachineCodeSpaceSize();
    size_t capacities = minSemiSpaceCapacity * 2 + nonMovableSpaceCapacity +
        machineCodeSpaceCapacity + readOnlySpaceCapacity;
    if (MAX_HEAP_SIZE < capacities || MAX_HEAP_SIZE - capacities < MIN_OLD_SPACE_LIMIT) {
        LOG_ECMA_MEM(FATAL) << "Capacities is too big to reset oldspace: " << capacities;
    }
    size_t newOldCapacity = MAX_HEAP_SIZE - capacities;
    LOG_ECMA(INFO) << "Main thread heap reset old capacity size: " << newOldCapacity;
    oldSpace_->SetInitialCapacity(newOldCapacity);
    oldSpace_->SetMaximumCapacity(newOldCapacity);
    compressSpace_->SetInitialCapacity(newOldCapacity);
    compressSpace_->SetMaximumCapacity(newOldCapacity);
    hugeObjectSpace_->SetInitialCapacity(newOldCapacity);
    hugeObjectSpace_->SetMaximumCapacity(newOldCapacity);
}

void SharedHeap::ResetLargeCapacity()
{
    size_t nonMovableSpaceCapacity = config_.GetDefaultNonMovableSpaceSize();
    size_t readOnlySpaceCapacity = config_.GetDefaultReadOnlySpaceSize();
    size_t capacities = nonMovableSpaceCapacity + readOnlySpaceCapacity;
    if (MAX_SHARED_HEAP_SIZE < capacities || MAX_SHARED_HEAP_SIZE - capacities < MIN_OLD_SPACE_LIMIT) {
        LOG_ECMA_MEM(FATAL) << "Shared capacities is too big to reset oldspace: " << capacities;
    }
    size_t newOldCapacity = AlignUp((MAX_SHARED_HEAP_SIZE - capacities) / 2, DEFAULT_REGION_SIZE);
    LOG_ECMA(INFO) << "Shared heap reset old capacity size: " << newOldCapacity;
    sOldSpace_->SetInitialCapacity(newOldCapacity);
    sOldSpace_->SetMaximumCapacity(newOldCapacity);
    sCompressSpace_->SetInitialCapacity(newOldCapacity);
    sCompressSpace_->SetMaximumCapacity(newOldCapacity);
    sHugeObjectSpace_->SetInitialCapacity(newOldCapacity);
    sHugeObjectSpace_->SetMaximumCapacity(newOldCapacity);
}

void Heap::ResetTlab()
{
    sOldTlab_->Reset();
    sNonMovableTlab_->Reset();
}

void Heap::FillBumpPointerForTlab()
{
    sOldTlab_->FillBumpPointer();
    sNonMovableTlab_->FillBumpPointer();
}

void Heap::ProcessSharedGCMarkingLocalBuffer()
{
    if (sharedGCData_.sharedConcurrentMarkingLocalBuffer_ != nullptr) {
        ASSERT(thread_->IsSharedConcurrentMarkingOrFinished());
        sHeap_->GetWorkManager()->PushLocalBufferToGlobal(sharedGCData_.sharedConcurrentMarkingLocalBuffer_);
        ASSERT(sharedGCData_.sharedConcurrentMarkingLocalBuffer_ == nullptr);
    }
}

void Heap::ProcessSharedGCRSetWorkList()
{
    if (sharedGCData_.rSetWorkListHandler_ != nullptr) {
        ASSERT(thread_->IsSharedConcurrentMarkingOrFinished());
        ASSERT(this == sharedGCData_.rSetWorkListHandler_->GetHeap());
        sHeap_->GetSharedGCMarker()->ProcessThenMergeBackRSetFromBoundJSThread(sharedGCData_.rSetWorkListHandler_);
        // The current thread may end earlier than the deamon thread.
        // To ensure the accuracy of the state range, set true is executed on js thread and deamon thread.
        // Reentrant does not cause exceptions because all the values are set to false.
        thread_->SetProcessingLocalToSharedRset(false);
        ASSERT(sharedGCData_.rSetWorkListHandler_ == nullptr);
    }
}

const GlobalEnvConstants *Heap::GetGlobalConst() const
{
    return thread_->GlobalConstants();
}

void Heap::Destroy()
{
    ProcessSharedGCRSetWorkList();
    ProcessSharedGCMarkingLocalBuffer();
    if (sOldTlab_ != nullptr) {
        sOldTlab_->Reset();
        delete sOldTlab_;
        sOldTlab_ = nullptr;
    }
    if (sNonMovableTlab_!= nullptr) {
        sNonMovableTlab_->Reset();
        delete sNonMovableTlab_;
        sNonMovableTlab_= nullptr;
    }
    if (workManager_ != nullptr) {
        delete workManager_;
        workManager_ = nullptr;
    }
    if (activeSemiSpace_ != nullptr) {
        activeSemiSpace_->Destroy();
        delete activeSemiSpace_;
        activeSemiSpace_ = nullptr;
    }
    if (inactiveSemiSpace_ != nullptr) {
        inactiveSemiSpace_->Destroy();
        delete inactiveSemiSpace_;
        inactiveSemiSpace_ = nullptr;
    }
    if (oldSpace_ != nullptr) {
        oldSpace_->Reset();
        delete oldSpace_;
        oldSpace_ = nullptr;
    }
    if (compressSpace_ != nullptr) {
        compressSpace_->Destroy();
        delete compressSpace_;
        compressSpace_ = nullptr;
    }
    if (nonMovableSpace_ != nullptr) {
        nonMovableSpace_->Reset();
        delete nonMovableSpace_;
        nonMovableSpace_ = nullptr;
    }
    if (snapshotSpace_ != nullptr) {
        snapshotSpace_->Destroy();
        delete snapshotSpace_;
        snapshotSpace_ = nullptr;
    }
    if (machineCodeSpace_ != nullptr) {
        machineCodeSpace_->Reset();
        delete machineCodeSpace_;
        machineCodeSpace_ = nullptr;
    }
    if (hugeObjectSpace_ != nullptr) {
        hugeObjectSpace_->Destroy();
        delete hugeObjectSpace_;
        hugeObjectSpace_ = nullptr;
    }
    if (hugeMachineCodeSpace_ != nullptr) {
        hugeMachineCodeSpace_->Destroy();
        delete hugeMachineCodeSpace_;
        hugeMachineCodeSpace_ = nullptr;
    }
    if (readOnlySpace_ != nullptr && mode_ != HeapMode::SHARE) {
        readOnlySpace_->ClearReadOnly();
        readOnlySpace_->Destroy();
        delete readOnlySpace_;
        readOnlySpace_ = nullptr;
    }
    if (appSpawnSpace_ != nullptr) {
        appSpawnSpace_->Reset();
        delete appSpawnSpace_;
        appSpawnSpace_ = nullptr;
    }
    if (partialGC_ != nullptr) {
        delete partialGC_;
        partialGC_ = nullptr;
    }
    if (fullGC_ != nullptr) {
        delete fullGC_;
        fullGC_ = nullptr;
    }

    nativeAreaAllocator_ = nullptr;
    heapRegionAllocator_ = nullptr;

    if (memController_ != nullptr) {
        delete memController_;
        memController_ = nullptr;
    }
    if (sweeper_ != nullptr) {
        delete sweeper_;
        sweeper_ = nullptr;
    }
    if (concurrentMarker_ != nullptr) {
        delete concurrentMarker_;
        concurrentMarker_ = nullptr;
    }
    if (incrementalMarker_ != nullptr) {
        delete incrementalMarker_;
        incrementalMarker_ = nullptr;
    }
    if (nonMovableMarker_ != nullptr) {
        delete nonMovableMarker_;
        nonMovableMarker_ = nullptr;
    }
    if (compressGCMarker_ != nullptr) {
        delete compressGCMarker_;
        compressGCMarker_ = nullptr;
    }
    if (Runtime::GetInstance()->IsHybridVm() && unifiedGCMarker_ != nullptr) {
        delete unifiedGCMarker_;
        unifiedGCMarker_ = nullptr;
    }
    if (evacuator_ != nullptr) {
        delete evacuator_;
        evacuator_ = nullptr;
    }
}

void Heap::Prepare()
{
    MEM_ALLOCATE_AND_GC_TRACE(ecmaVm_, HeapPrepare);
    WaitRunningTaskFinished();
    sweeper_->EnsureAllTaskFinished();
    WaitClearTaskFinished();
}

void Heap::GetHeapPrepare()
{
    // Ensure local and shared heap prepared.
    Prepare();
    SharedHeap *sHeap = SharedHeap::GetInstance();
    sHeap->Prepare(false);
}

void Heap::Resume(TriggerGCType gcType)
{
    activeSemiSpace_->SetWaterLine();

    if (mode_ != HeapMode::SPAWN &&
        activeSemiSpace_->AdjustCapacity(inactiveSemiSpace_->GetAllocatedSizeSinceGC(), thread_)) {
        // if activeSpace capacity changes， oldSpace maximumCapacity should change, too.
        size_t multiple = 2;
        size_t oldSpaceMaxLimit = 0;
        if (activeSemiSpace_->GetInitialCapacity() >= inactiveSemiSpace_->GetInitialCapacity()) {
            size_t delta = activeSemiSpace_->GetInitialCapacity() - inactiveSemiSpace_->GetInitialCapacity();
            oldSpaceMaxLimit = oldSpace_->GetMaximumCapacity() - delta * multiple;
        } else {
            size_t delta = inactiveSemiSpace_->GetInitialCapacity() - activeSemiSpace_->GetInitialCapacity();
            oldSpaceMaxLimit = oldSpace_->GetMaximumCapacity() + delta * multiple;
        }
        inactiveSemiSpace_->SetInitialCapacity(activeSemiSpace_->GetInitialCapacity());
    }

    PrepareRecordRegionsForReclaim();
    hugeObjectSpace_->ReclaimHugeRegion();
    hugeMachineCodeSpace_->ReclaimHugeRegion();
    if (parallelGC_) {
        if (gcType == TriggerGCType::OLD_GC) {
            isCSetClearing_.store(true, std::memory_order_release);
        }
        clearTaskFinished_ = false;
        common::Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<AsyncClearTask>(GetJSThread()->GetThreadId(), this, gcType));
    } else {
        ReclaimRegions(gcType);
    }
}

void Heap::ResumeForAppSpawn()
{
    sweeper_->WaitAllTaskFinished();
    hugeObjectSpace_->ReclaimHugeRegion();
    hugeMachineCodeSpace_->ReclaimHugeRegion();
    inactiveSemiSpace_->ReclaimRegions();
    oldSpace_->Reset();
    auto cb = [] (Region *region) {
        region->ClearMarkGCBitset();
    };
    nonMovableSpace_->EnumerateRegions(cb);
    machineCodeSpace_->EnumerateRegions(cb);
    hugeObjectSpace_->EnumerateRegions(cb);
    hugeMachineCodeSpace_->EnumerateRegions(cb);
}

void Heap::CompactHeapBeforeFork()
{
    CollectGarbage(TriggerGCType::APPSPAWN_FULL_GC);
}

void Heap::DisableParallelGC()
{
    WaitAllTasksFinished();
    parallelGC_ = false;
    maxEvacuateTaskCount_ = 0;
    maxMarkTaskCount_ = 0;
    sweeper_->ConfigConcurrentSweep(false);
    concurrentMarker_->ConfigConcurrentMark(false);
    common::Taskpool::GetCurrentTaskpool()->Destroy(GetJSThread()->GetThreadId());
}

void Heap::EnableParallelGC()
{
    parallelGC_ = ecmaVm_->GetJSOptions().EnableParallelGC();
    maxEvacuateTaskCount_ = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    if (auto totalThreadNum = workManager_->GetTotalThreadNum();
        totalThreadNum != maxEvacuateTaskCount_ + 1) {
        LOG_ECMA_MEM(WARN) << "TheadNum mismatch, totalThreadNum(workerManager): " << totalThreadNum << ", "
                           << "totalThreadNum(taskpool): " << (maxEvacuateTaskCount_ + 1);
        delete workManager_;
        workManager_ = new WorkManager(this, maxEvacuateTaskCount_ + 1);
        UpdateWorkManager(workManager_);
    }
    ASSERT(maxEvacuateTaskCount_ > 0);
    maxMarkTaskCount_ = std::min<size_t>(ecmaVm_->GetJSOptions().GetGcThreadNum(),
                                         maxEvacuateTaskCount_ - 1);
    bool concurrentMarkerEnabled = ecmaVm_->GetJSOptions().EnableConcurrentMark();
#if ECMASCRIPT_DISABLE_CONCURRENT_MARKING
    concurrentMarkerEnabled = false;
#endif
    sweeper_->ConfigConcurrentSweep(ecmaVm_->GetJSOptions().EnableConcurrentSweep());
    concurrentMarker_->ConfigConcurrentMark(concurrentMarkerEnabled);
}

TriggerGCType Heap::SelectGCType() const
{
    // If concurrent mark is enabled, the TryTriggerConcurrentMarking decide which GC to choose.
    if (concurrentMarker_->IsEnabled() && !thread_->IsReadyToConcurrentMark()) {
        return YOUNG_GC;
    }
    if (!OldSpaceExceedLimit() && !OldSpaceExceedCapacity(activeSemiSpace_->GetCommittedSize()) &&
        GetHeapObjectSize() <= globalSpaceAllocLimit_  + oldSpace_->GetOvershootSize() &&
        !GlobalNativeSizeLargerThanLimit()) {
        return YOUNG_GC;
    }
    return OLD_GC;
}

void Heap::CollectGarbageImpl(TriggerGCType gcType, GCReason reason)
{
    ASSERT("CollectGarbageImpl should not be called" && !g_isEnableCMCGC);
    Jit::JitGCLockHolder lock(GetEcmaVM()->GetJSThread());
    {
#if ECMASCRIPT_ENABLE_THREAD_STATE_CHECK
        if (UNLIKELY(!thread_->IsInRunningStateOrProfiling())) { // LOCV_EXCL_BR_LINE
            LOG_ECMA(FATAL) << "Local GC must be in jsthread running state";
            UNREACHABLE();
        }
#endif
        if (thread_->IsCrossThreadExecutionEnable() || GetOnSerializeEvent()) {
            ProcessGCListeners();
            return;
        }
        RecursionScope recurScope(this, HeapType::LOCAL_HEAP);
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
        [[maybe_unused]] GcStateScope scope(thread_);
#endif
        CHECK_NO_GC;
        if (UNLIKELY(ShouldVerifyHeap())) { // LCOV_EXCL_BR_LINE
            // pre gc heap verify
            LOG_ECMA(DEBUG) << "pre gc heap verify";
            ProcessSharedGCRSetWorkList();
            Verification(this, VerifyKind::VERIFY_PRE_GC).VerifyAll();
        }

#if ECMASCRIPT_SWITCH_GC_MODE_TO_FULL_GC
        gcType = TriggerGCType::FULL_GC;
#endif
        if (fullGCRequested_ && thread_->IsReadyToConcurrentMark() && gcType != TriggerGCType::FULL_GC) {
            gcType = TriggerGCType::FULL_GC;
        }
        if (oldGCRequested_ && gcType != TriggerGCType::FULL_GC) {
            gcType = TriggerGCType::OLD_GC;
        }
        if (shouldThrowOOMError_) {
            // Force Full GC after failed Old GC to avoid OOM
            LOG_ECMA(INFO) << "Old space is almost OOM, attempt trigger full gc to avoid OOM.";
            gcType = TriggerGCType::FULL_GC;
        }
        oldGCRequested_ = false;
        oldSpace_->AdjustOvershootSize();

        size_t originalNewSpaceSize = activeSemiSpace_->GetHeapObjectSize();
        if (!GetJSThread()->IsReadyToConcurrentMark() && markType_ == MarkType::MARK_FULL) {
            GetEcmaGCStats()->SetGCReason(reason);
        } else {
            GetEcmaGCStats()->RecordStatisticBeforeGC(gcType, reason);
        }
        memController_->StartCalculationBeforeGC();
        StatisticHeapObject(gcType);
        gcType_ = gcType;
        {
            pgo::PGODumpPauseScope pscope(GetEcmaVM()->GetPGOProfiler());
            switch (gcType) {
                case TriggerGCType::YOUNG_GC:
                    // Use partial GC for young generation.
                    if (!concurrentMarker_->IsEnabled() && !incrementalMarker_->IsTriggeredIncrementalMark()) {
                        SetMarkType(MarkType::MARK_YOUNG);
                    }
                    if (markType_ == MarkType::MARK_FULL) {
                        // gcType_ must be sure. Functions ProcessNativeReferences need to use it.
                        gcType_ = TriggerGCType::OLD_GC;
                    }
                    partialGC_->RunPhases();
                    break;
                case TriggerGCType::OLD_GC: {
                    bool fullConcurrentMarkRequested = false;
                    // Check whether it's needed to trigger full concurrent mark instead of trigger old gc
                    if (concurrentMarker_->IsEnabled() &&
                        (thread_->IsReadyToConcurrentMark() || markType_ == MarkType::MARK_YOUNG) &&
                        reason == GCReason::ALLOCATION_LIMIT) {
                        fullConcurrentMarkRequested = true;
                    }
                    if (concurrentMarker_->IsEnabled() && markType_ == MarkType::MARK_YOUNG) {
                        // Wait for existing concurrent marking tasks to be finished (if any),
                        // and reset concurrent marker's status for full mark.
                        bool concurrentMark = CheckOngoingConcurrentMarking();
                        if (concurrentMark) {
                            concurrentMarker_->Reset();
                        }
                    }
                    SetMarkType(MarkType::MARK_FULL);
                    if (fullConcurrentMarkRequested && idleTask_ == IdleTaskType::NO_TASK) {
                        LOG_ECMA(INFO)
                            << "Trigger old gc here may cost long time, trigger full concurrent mark instead";
                        oldSpace_->SetOvershootSize(config_.GetOldSpaceStepOvershootSize());
                        TriggerConcurrentMarking(MarkReason::OLD_GC_WITHOUT_FULLMARK);
                        oldGCRequested_ = true;
                        ProcessGCListeners();
                        memController_->ResetCalculationWithoutGC();
                        return;
                    }
                    partialGC_->RunPhases();
                    break;
                }
                case TriggerGCType::FULL_GC:
                    fullGC_->SetForAppSpawn(false);
                    fullGC_->RunPhases();
                    if (fullGCRequested_) {
                        fullGCRequested_ = false;
                    }
                    break;
                case TriggerGCType::APPSPAWN_FULL_GC:
                    fullGC_->SetForAppSpawn(true);
                    fullGC_->RunPhasesForAppSpawn();
                    break;
                default: // LOCV_EXCL_BR_LINE
                    LOG_ECMA(FATAL) << "this branch is unreachable";
                    UNREACHABLE();
                    break;
            }
            ASSERT(thread_->IsPropertyCacheCleared());
        }
        UpdateHeapStatsAfterGC(gcType_);
        ClearIdleTask();
        // Adjust the old space capacity and global limit for the first partial GC with full mark.
        // Trigger full mark next time if the current survival rate is much less than half the average survival rates.
        AdjustBySurvivalRate(originalNewSpaceSize);
        memController_->StopCalculationAfterGC(gcType);
        if (gcType == TriggerGCType::FULL_GC || IsConcurrentFullMark()) {
            // Only when the gc type is not semiGC and after the old space sweeping has been finished,
            // the limits of old space and global space can be recomputed.
            RecomputeLimits();
            ResetNativeSizeAfterLastGC();
            OPTIONAL_LOG(ecmaVm_, INFO) << " GC after: is full mark" << IsConcurrentFullMark()
                                        << " global object size " << GetHeapObjectSize()
                                        << " global committed size " << GetCommittedSize()
                                        << " global limit " << globalSpaceAllocLimit_;
            markType_ = MarkType::MARK_YOUNG;
        }
        if (concurrentMarker_->IsRequestDisabled()) {
            concurrentMarker_->EnableConcurrentMarking(EnableConcurrentMarkType::DISABLE);
        }
        // GC log
        GetEcmaGCStats()->RecordStatisticAfterGC();
#ifdef ENABLE_HISYSEVENT
        GetEcmaGCKeyStats()->IncGCCount();
        if (GetEcmaGCKeyStats()->CheckIfMainThread() && GetEcmaGCKeyStats()->CheckIfKeyPauseTime()) {
            GetEcmaGCKeyStats()->AddGCStatsToKey();
        }
#endif
        GetEcmaGCStats()->PrintGCStatistic();
    }

    if (gcType_ == TriggerGCType::OLD_GC) {
        // During full concurrent mark, non movable space can have 2M overshoot size temporarily, which means non
        // movable space max heap size can reach to 18M temporarily, but after partial old gc, the size must retract to
        // below 16M, Otherwise, old GC will be triggered frequently. Non-concurrent mark period, non movable space max
        // heap size is 16M, if exceeded, an OOM exception will be thrown, this check is to do this.
        CheckNonMovableSpaceOOM();
    }
    // OOMError object is not allowed to be allocated during gc process, so throw OOMError after gc
    if (shouldThrowOOMError_ && gcType_ == TriggerGCType::FULL_GC) {
        oldSpace_->ResetCommittedOverSizeLimit();
        if (oldSpace_->CommittedSizeExceed()) { // LCOV_EXCL_BR_LINE
            sweeper_->EnsureAllTaskFinished();
            DumpHeapSnapshotBeforeOOM();
            StatisticHeapDetail();
            ThrowOutOfMemoryError(thread_, oldSpace_->GetMergeSize(), " OldSpace::Merge");
        }
        oldSpace_->ResetMergeSize();
        shouldThrowOOMError_ = false;
    }
    // Allocate region failed during GC, MUST throw OOM here
    if (shouldForceThrowOOMError_) {
        sweeper_->EnsureAllTaskFinished();
        DumpHeapSnapshotBeforeOOM();
        StatisticHeapDetail();
        ThrowOutOfMemoryError(thread_, DEFAULT_REGION_SIZE, " HeapRegionAllocator::AllocateAlignedRegion");
    }
    // Update record heap object size after gc if in sensitive status
    if (GetSensitiveStatus() == AppSensitiveStatus::ENTER_HIGH_SENSITIVE) {
        SetRecordHeapObjectSizeBeforeSensitive(GetHeapObjectSize());
    }

    if (UNLIKELY(ShouldVerifyHeap())) { // LCOV_EXCL_BR_LINE
        // verify post gc heap verify
        LOG_ECMA(DEBUG) << "post gc heap verify";
        Verification(this, VerifyKind::VERIFY_POST_GC).VerifyAll();
    }

#if defined(ECMASCRIPT_SUPPORT_TRACING)
    auto tracing = GetEcmaVM()->GetTracing();
    if (tracing != nullptr) {
        tracing->TraceEventRecordMemory();
    }
#endif
    ProcessGCListeners();

#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT) && defined(PANDA_TARGET_OHOS) && defined(ENABLE_HISYSEVENT)
    if (!hasOOMDump_ && (g_betaVersion || g_developMode)) {
        ThresholdReachedDump();
    }
#endif

    if (GetEcmaGCKeyStats()->CheckIfMainThread()) {
        GetEcmaGCKeyStats()->ProcessLongGCEvent();
    }

    if (GetEcmaVM()->IsEnableBaselineJit() || GetEcmaVM()->IsEnableFastJit()) {
        // check machine code space if enough
        int remainSize = static_cast<int>(config_.GetDefaultMachineCodeSpaceSize()) -
            static_cast<int>(GetMachineCodeSpace()->GetHeapObjectSize());
        Jit::GetInstance()->CheckMechineCodeSpaceMemory(GetEcmaVM()->GetJSThread(), remainSize);
    }
}

void Heap::CollectGarbage(TriggerGCType gcType, GCReason reason)
{
    if (g_isEnableCMCGC) {
        common::GCReason cmcReason = common::GC_REASON_USER;
        bool async = true;
        if (gcType == TriggerGCType::FULL_GC || gcType == TriggerGCType::SHARED_FULL_GC ||
            gcType == TriggerGCType::APPSPAWN_FULL_GC || gcType == TriggerGCType::APPSPAWN_SHARED_FULL_GC ||
            reason == GCReason::ALLOCATION_FAILED) {
            cmcReason = common::GC_REASON_BACKUP;
            async = false;
        }
        ThreadNativeScope scope(thread_);
        common::BaseRuntime::RequestGC(cmcReason, async, common::GC_TYPE_FULL);
        return;
    }
    CollectGarbageImpl(gcType, reason);
    ProcessGCCallback();
}

void Heap::ProcessGCCallback()
{
    // Weak node nativeFinalizeCallback may execute JS and change the weakNodeList status,
    // even lead to another GC, so this have to invoke after this GC process.
    if (g_isEnableCMCGC) {
        thread_->InvokeWeakNodeFreeGlobalCallBack();
    }
    thread_->InvokeWeakNodeNativeFinalizeCallback();
    // PostTask for ProcessNativeDelete
    CleanCallback();
    JSFinalizationRegistry::CheckAndCall(thread_);
    // clear env cache
    thread_->ClearCache();
}

void BaseHeap::ThrowOutOfMemoryError(JSThread *thread, size_t size, std::string functionName,
    bool NonMovableObjNearOOM)
{ // LCOV_EXCL_START
    GetEcmaGCStats()->PrintGCMemoryStatistic();
    std::ostringstream oss;
    if (NonMovableObjNearOOM) {
        oss << "OutOfMemory when nonmovable live obj size: " << size << " bytes"
            << " function name: " << functionName.c_str();
    } else {
        oss << "OutOfMemory when trying to allocate " << size << " bytes" << " function name: "
            << functionName.c_str();
    }
    LOG_ECMA_MEM(ERROR) << oss.str().c_str();
    THROW_OOM_ERROR(thread, oss.str().c_str());
} // LCOV_EXCL_STOP

void BaseHeap::SetMachineCodeOutOfMemoryError(JSThread *thread, size_t size, std::string functionName)
{
    std::ostringstream oss;
    oss << "OutOfMemory when trying to allocate " << size << " bytes" << " function name: "
        << functionName.c_str();
    LOG_ECMA_MEM(ERROR) << oss.str().c_str();

    EcmaVM *ecmaVm = thread->GetEcmaVM();
    ObjectFactory *factory = ecmaVm->GetFactory();
    JSHandle<JSObject> error = factory->GetJSError(ErrorType::OOM_ERROR, oss.str().c_str(), StackCheck::NO);
    thread->SetException(error.GetTaggedValue());
}

void BaseHeap::ThrowOutOfMemoryErrorForDefault(JSThread *thread, size_t size, std::string functionName,
    bool NonMovableObjNearOOM)
{ // LCOV_EXCL_START
    GetEcmaGCStats()->PrintGCMemoryStatistic();
    std::ostringstream oss;
    if (NonMovableObjNearOOM) {
        oss << "OutOfMemory when nonmovable live obj size: " << size << " bytes"
            << " function name: " << functionName.c_str();
    } else {
        oss << "OutOfMemory when trying to allocate " << size << " bytes" << " function name: " << functionName.c_str();
    }
    LOG_ECMA_MEM(ERROR) << oss.str().c_str();
    EcmaVM *ecmaVm = thread->GetEcmaVM();
    JSHandle<GlobalEnv> env = ecmaVm->GetGlobalEnv();
    JSHandle<JSObject> error = JSHandle<JSObject>::Cast(env->GetOOMErrorObject());

    thread->SetException(error.GetTaggedValue());
    ecmaVm->HandleUncatchableError();
} // LCOV_EXCL_STOP

void BaseHeap::FatalOutOfMemoryError(size_t size, std::string functionName)
{ // LCOV_EXCL_START
    GetEcmaGCStats()->PrintGCMemoryStatistic();
    LOG_ECMA_MEM(FATAL) << "OOM fatal when trying to allocate " << size << " bytes"
                        << " function name: " << functionName.c_str();
} // LCOV_EXCL_STOP

void Heap::CheckNonMovableSpaceOOM()
{
    if (nonMovableSpace_->GetHeapObjectSize() > MAX_NONMOVABLE_LIVE_OBJ_SIZE) { // LCOV_EXCL_BR_LINE
        sweeper_->EnsureAllTaskFinished();
        DumpHeapSnapshotBeforeOOM();
        StatisticHeapDetail();
        ThrowOutOfMemoryError(thread_, nonMovableSpace_->GetHeapObjectSize(), "Heap::CheckNonMovableSpaceOOM", true);
    }
}

void Heap::AdjustBySurvivalRate(size_t originalNewSpaceSize)
{
    promotedSize_ = GetEvacuator()->GetPromotedSize();
    if (originalNewSpaceSize <= 0) {
        return;
    }
    semiSpaceCopiedSize_ = activeSemiSpace_->GetHeapObjectSize();
    double copiedRate = semiSpaceCopiedSize_ * 1.0 / originalNewSpaceSize;
    double promotedRate = promotedSize_ * 1.0 / originalNewSpaceSize;
    double survivalRate = std::min(copiedRate + promotedRate, 1.0);
    OPTIONAL_LOG(ecmaVm_, INFO) << " copiedRate: " << copiedRate << " promotedRate: " << promotedRate
                                << " survivalRate: " << survivalRate;
    if (!oldSpaceLimitAdjusted_) {
        memController_->AddSurvivalRate(survivalRate);
        AdjustOldSpaceLimit();
    } else {
        double averageSurvivalRate = memController_->GetAverageSurvivalRate();
        // 2 means half
        if ((averageSurvivalRate / 2) > survivalRate && averageSurvivalRate > GROW_OBJECT_SURVIVAL_RATE) {
            SetFullMarkRequestedState(true);
            OPTIONAL_LOG(ecmaVm_, INFO) << " Current survival rate: " << survivalRate
                << " is less than half the average survival rates: " << averageSurvivalRate
                << ". Trigger full mark next time.";
            // Survival rate of full mark is precise. Reset recorded survival rates.
            memController_->ResetRecordedSurvivalRates();
        }
        memController_->AddSurvivalRate(survivalRate);
    }
}

size_t Heap::VerifyHeapObjects(VerifyKind verifyKind) const
{
    size_t failCount = 0;
    {
        VerifyObjectVisitor verifier(this, &failCount, verifyKind);
        activeSemiSpace_->IterateOverObjects(verifier);
    }

    {
        if (verifyKind == VerifyKind::VERIFY_EVACUATE_YOUNG ||
            verifyKind == VerifyKind::VERIFY_EVACUATE_OLD ||
            verifyKind == VerifyKind::VERIFY_EVACUATE_FULL) {
                inactiveSemiSpace_->EnumerateRegions([this](Region *region) {
                    region->IterateAllMarkedBits([this](void *addr) {
                        VerifyObjectVisitor::VerifyInactiveSemiSpaceMarkedObject(this, addr);
                    });
                });
            }
    }

    {
        VerifyObjectVisitor verifier(this, &failCount, verifyKind);
        oldSpace_->IterateOverObjects(verifier);
    }

    {
        VerifyObjectVisitor verifier(this, &failCount, verifyKind);
        appSpawnSpace_->IterateOverMarkedObjects(verifier);
    }

    {
        VerifyObjectVisitor verifier(this, &failCount, verifyKind);
        nonMovableSpace_->IterateOverObjects(verifier);
    }

    {
        VerifyObjectVisitor verifier(this, &failCount, verifyKind);
        hugeObjectSpace_->IterateOverObjects(verifier);
    }
    {
        VerifyObjectVisitor verifier(this, &failCount, verifyKind);
        hugeMachineCodeSpace_->IterateOverObjects(verifier);
    }
    {
        VerifyObjectVisitor verifier(this, &failCount, verifyKind);
        machineCodeSpace_->IterateOverObjects(verifier);
    }
    {
        VerifyObjectVisitor verifier(this, &failCount, verifyKind);
        snapshotSpace_->IterateOverObjects(verifier);
    }
    return failCount;
}

size_t Heap::VerifyOldToNewRSet(VerifyKind verifyKind) const
{
    size_t failCount = 0;
    VerifyObjectVisitor verifier(this, &failCount, verifyKind);
    oldSpace_->IterateOldToNewOverObjects(verifier);
    appSpawnSpace_->IterateOldToNewOverObjects(verifier);
    nonMovableSpace_->IterateOldToNewOverObjects(verifier);
    machineCodeSpace_->IterateOldToNewOverObjects(verifier);
    return failCount;
}

void Heap::AdjustOldSpaceLimit()
{
    if (oldSpaceLimitAdjusted_) {
        return;
    }
    size_t minGrowingStep = ecmaVm_->GetEcmaParamConfiguration().GetMinGrowingStep();
    size_t oldSpaceAllocLimit = GetOldSpace()->GetInitialCapacity();
    size_t newOldSpaceAllocLimit = std::max(oldSpace_->GetHeapObjectSize() + minGrowingStep,
        static_cast<size_t>(oldSpaceAllocLimit * memController_->GetAverageSurvivalRate()));
    if (newOldSpaceAllocLimit <= oldSpaceAllocLimit) {
        GetOldSpace()->SetInitialCapacity(newOldSpaceAllocLimit);
    } else {
        oldSpaceLimitAdjusted_ = true;
    }

    size_t newGlobalSpaceAllocLimit = std::max(GetHeapObjectSize() + minGrowingStep,
        static_cast<size_t>(globalSpaceAllocLimit_ * memController_->GetAverageSurvivalRate()));
    if (newGlobalSpaceAllocLimit < globalSpaceAllocLimit_) {
        globalSpaceAllocLimit_ = newGlobalSpaceAllocLimit;
    }
    OPTIONAL_LOG(ecmaVm_, INFO) << "AdjustOldSpaceLimit oldSpaceAllocLimit_: " << oldSpaceAllocLimit
        << " globalSpaceAllocLimit_: " << globalSpaceAllocLimit_;
}

void BaseHeap::OnAllocateEvent([[maybe_unused]] EcmaVM *ecmaVm, [[maybe_unused]] TaggedObject* address,
                               [[maybe_unused]] size_t size)
{
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    HeapProfilerInterface *profiler = ecmaVm->GetHeapProfile();
    if (profiler != nullptr) {
        base::BlockHookScope blockScope;
        profiler->AllocationEvent(address, size);
    }
#endif
}

void Heap::DumpHeapSnapshotBeforeOOM()
{
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT) && defined(ENABLE_DUMP_IN_FAULTLOG)
    AppFreezeFilterCallback appfreezeCallback = Runtime::GetInstance()->GetAppFreezeFilterCallback();
    std::string eventConfig;
    bool shouldDump = (appfreezeCallback == nullptr || appfreezeCallback(getprocpid(), true, eventConfig));
    GetEcmaGCKeyStats()->SendSysEventBeforeDump("OOMDump", GetHeapLimitSize(), GetLiveObjectSize(), eventConfig);
    if (!shouldDump) {
        LOG_ECMA(INFO) << "Heap::DumpHeapSnapshotBeforeOOM, no dump quota.";
        return;
    }
#endif
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT)
#if defined(ENABLE_DUMP_IN_FAULTLOG)
    if (ecmaVm_->GetHeapProfile() != nullptr) {
        LOG_ECMA(ERROR) << "Heap::DumpHeapSnapshotBeforeOOM, HeapProfile is nullptr";
        return;
    }
    // Filter appfreeze when dump.
    LOG_ECMA(INFO) << " Heap::DumpHeapSnapshotBeforeOOM, trigger oom dump";
    base::BlockHookScope blockScope;
    HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(ecmaVm_);
    hasOOMDump_ = true;
    // Vm should always allocate young space successfully. Really OOM will occur in the non-young spaces.
    DumpSnapShotOption dumpOption;
    dumpOption.dumpFormat = DumpFormat::BINARY;
    dumpOption.isVmMode = true;
    dumpOption.isPrivate = false;
    dumpOption.captureNumericValue = false;
    dumpOption.isFullGC = false;
    dumpOption.isSimplify = true;
    dumpOption.isSync = true;
    dumpOption.isBeforeFill = false;
    dumpOption.isDumpOOM = true;
    heapProfile->DumpHeapSnapshotForOOM(dumpOption);
    HeapProfilerInterface::Destroy(ecmaVm_);
#endif // ENABLE_DUMP_IN_FAULTLOG
#endif // ECMASCRIPT_SUPPORT_SNAPSHOT
}

void Heap::AdjustSpaceSizeForAppSpawn()
{
    SetHeapMode(HeapMode::SPAWN);
    size_t minSemiSpaceCapacity = config_.GetMinSemiSpaceSize();
    activeSemiSpace_->SetInitialCapacity(minSemiSpaceCapacity);
    auto committedSize = appSpawnSpace_->GetCommittedSize();
    appSpawnSpace_->SetInitialCapacity(committedSize);
    appSpawnSpace_->SetMaximumCapacity(committedSize);
    oldSpace_->SetInitialCapacity(oldSpace_->GetInitialCapacity() - committedSize);
    oldSpace_->SetMaximumCapacity(oldSpace_->GetMaximumCapacity() - committedSize);
}

void Heap::AddAllocationInspectorToAllSpaces(AllocationInspector *inspector)
{
    ASSERT(inspector != nullptr);
    // activeSemiSpace_/inactiveSemiSpace_:
    // only add an inspector to activeSemiSpace_, and while sweeping for gc, inspector need be swept.
    activeSemiSpace_->AddAllocationInspector(inspector);
    // oldSpace_/compressSpace_:
    // only add an inspector to oldSpace_, and while sweeping for gc, inspector need be swept.
    oldSpace_->AddAllocationInspector(inspector);
    // readOnlySpace_ need not allocationInspector.
    // appSpawnSpace_ need not allocationInspector.
    nonMovableSpace_->AddAllocationInspector(inspector);
    machineCodeSpace_->AddAllocationInspector(inspector);
    hugeObjectSpace_->AddAllocationInspector(inspector);
    hugeMachineCodeSpace_->AddAllocationInspector(inspector);
}

void Heap::ClearAllocationInspectorFromAllSpaces()
{
    activeSemiSpace_->ClearAllocationInspector();
    oldSpace_->ClearAllocationInspector();
    nonMovableSpace_->ClearAllocationInspector();
    machineCodeSpace_->ClearAllocationInspector();
    hugeObjectSpace_->ClearAllocationInspector();
    hugeMachineCodeSpace_->ClearAllocationInspector();
}

void Heap::RecomputeLimits()
{
    double gcSpeed = memController_->CalculateMarkCompactSpeedPerMS();
    double mutatorSpeed = memController_->GetCurrentOldSpaceAllocationThroughputPerMS();
    size_t oldSpaceSize = oldSpace_->GetHeapObjectSize() + hugeObjectSpace_->GetHeapObjectSize() +
        hugeMachineCodeSpace_->GetHeapObjectSize();
    size_t newSpaceCapacity = activeSemiSpace_->GetInitialCapacity();

    double growingFactor = memController_->CalculateGrowingFactor(gcSpeed, mutatorSpeed);
    size_t maxOldSpaceCapacity = oldSpace_->GetMaximumCapacity() - newSpaceCapacity;
    size_t newOldSpaceLimit = memController_->CalculateAllocLimit(oldSpaceSize, MIN_OLD_SPACE_LIMIT,
        maxOldSpaceCapacity, newSpaceCapacity, growingFactor);
    size_t maxGlobalSize = config_.GetMaxHeapSize() - newSpaceCapacity;
    size_t newGlobalSpaceLimit = memController_->CalculateAllocLimit(GetHeapObjectSize(), MIN_HEAP_SIZE,
                                                                     maxGlobalSize, newSpaceCapacity, growingFactor);
    globalSpaceAllocLimit_ = newGlobalSpaceLimit;
    oldSpace_->SetInitialCapacity(newOldSpaceLimit);
    globalSpaceNativeLimit_ = memController_->CalculateAllocLimit(GetGlobalNativeSize(), MIN_HEAP_SIZE,
                                                                  MAX_GLOBAL_NATIVE_LIMIT, newSpaceCapacity,
                                                                  growingFactor);
    globalSpaceNativeLimit_ = std::max(globalSpaceNativeLimit_, GetGlobalNativeSize()
                                        + config_.GetMinNativeLimitGrowingStep());
    OPTIONAL_LOG(ecmaVm_, INFO) << "RecomputeLimits oldSpaceAllocLimit_: " << newOldSpaceLimit
        << " globalSpaceAllocLimit_: " << globalSpaceAllocLimit_
        << " globalSpaceNativeLimit_:" << globalSpaceNativeLimit_;
    if ((oldSpace_->GetHeapObjectSize() * 1.0 / SHRINK_OBJECT_SURVIVAL_RATE) < oldSpace_->GetCommittedSize() &&
        (oldSpace_->GetCommittedSize() / 2) > newOldSpaceLimit) { // 2: means half
        OPTIONAL_LOG(ecmaVm_, INFO) << " Old space heap object size is too much lower than committed size"
                                    << " heapObjectSize: "<< oldSpace_->GetHeapObjectSize()
                                    << " Committed Size: " << oldSpace_->GetCommittedSize();
        SetFullMarkRequestedState(true);
    }
}

bool Heap::CheckAndTriggerOldGC(size_t size)
{
    bool isFullMarking = IsConcurrentFullMark() && GetJSThread()->IsMarking();
    bool isNativeSizeLargeTrigger = isFullMarking ? false : GlobalNativeSizeLargerThanLimit();
    if (isFullMarking && oldSpace_->GetOvershootSize() == 0) {
        oldSpace_->SetOvershootSize(config_.GetOldSpaceStepOvershootSize());
    }
    if ((isNativeSizeLargeTrigger || OldSpaceExceedLimit() || OldSpaceExceedCapacity(size) ||
        GetHeapObjectSize() > globalSpaceAllocLimit_ + oldSpace_->GetOvershootSize()) &&
        !NeedStopCollection()) {
        if (isFullMarking && oldSpace_->GetOvershootSize() < config_.GetOldSpaceMaxOvershootSize()) {
            oldSpace_->IncreaseOvershootSize(config_.GetOldSpaceStepOvershootSize());
            return false;
        }
        CollectGarbage(TriggerGCType::OLD_GC, GCReason::ALLOCATION_LIMIT);
        if (!oldGCRequested_) {
            return true;
        }
    }
    return false;
}

bool Heap::CheckAndTriggerHintGC(MemoryReduceDegree degree, GCReason reason)
{
    if (InSensitiveStatus()) {
        return false;
    }
    if (g_isEnableCMCGC) {
        common::MemoryReduceDegree cmcDegree = common::MemoryReduceDegree::LOW;
        if (degree == MemoryReduceDegree::HIGH) {
            cmcDegree = common::MemoryReduceDegree::HIGH;
        }
        return common::BaseRuntime::CheckAndTriggerHintGC(cmcDegree);
    }
    LOG_GC(INFO) << "HintGC degree:"<< static_cast<int>(degree) << " reason:" << GCStats::GCReasonToString(reason);
    switch (degree) {
        case MemoryReduceDegree::LOW: {
            if (idleGCTrigger_->HintGCInLowDegree<Heap>(this)) {
                if (CheckCanTriggerConcurrentMarking()) {
                    markType_ = MarkType::MARK_FULL;
                    TriggerConcurrentMarking(MarkReason::HINT_GC);
                    LOG_GC(INFO) << " MemoryReduceDegree::LOW TriggerConcurrentMark.";
                    return true;
                }
            }
            if (idleGCTrigger_->HintGCInLowDegree<SharedHeap>(sHeap_)) {
                if (sHeap_->CheckCanTriggerConcurrentMarking(thread_)) {
                    LOG_GC(INFO) << " MemoryReduceDegree::LOW TriggerSharedConcurrentMark.";
                    sHeap_->TriggerConcurrentMarking<TriggerGCType::SHARED_GC, MarkReason::HINT_GC>(thread_);
                    return true;
                }
            }
            break;
        }
        case MemoryReduceDegree::MIDDLE: {
            if (idleGCTrigger_->HintGCInMiddleDegree<Heap>(this)) {
                CollectGarbage(TriggerGCType::FULL_GC, reason);
                return true;
            }
            if (idleGCTrigger_->HintGCInMiddleDegree<SharedHeap>(sHeap_)) {
                sHeap_->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::HINT_GC>(thread_);
                return true;
            }
            break;
        }
        case MemoryReduceDegree::HIGH: {
            bool result = false;
            if (idleGCTrigger_->HintGCInHighDegree<Heap>(this)) {
                CollectGarbage(TriggerGCType::FULL_GC, reason);
                result = true;
            }
            if (idleGCTrigger_->HintGCInHighDegree<SharedHeap>(sHeap_)) {
                sHeap_->CollectGarbage<TriggerGCType::SHARED_FULL_GC, GCReason::HINT_GC>(thread_);
                result = true;
            }
            return result;
        }
        default: // LCOV_EXCL_BR_LINE
            LOG_GC(INFO) << "HintGC invalid degree value: " << static_cast<int>(degree);
            break;
    }
    return false;
}

bool Heap::CheckOngoingConcurrentMarkingImpl(ThreadType threadType, int threadIndex,
                                             [[maybe_unused]] const char* traceName)
{
    if (!concurrentMarker_->IsEnabled() || !concurrentMarker_->IsTriggeredConcurrentMark() ||
        thread_->IsReadyToConcurrentMark()) {
        return false;
    }
    TRACE_GC(GCStats::Scope::ScopeId::WaitConcurrentMarkFinished, GetEcmaVM()->GetEcmaGCStats());
    if (thread_->IsMarking()) {
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, traceName, "");
        if (threadType == ThreadType::JS_THREAD) {
            MEM_ALLOCATE_AND_GC_TRACE(ecmaVm_, WaitConcurrentMarkingFinished);
            GetNonMovableMarker()->ProcessMarkStack(threadIndex);
            WaitConcurrentMarkingFinished();
        } else if (threadType == ThreadType::DAEMON_THREAD) {
            CHECK_DAEMON_THREAD();
            GetNonMovableMarker()->ProcessMarkStack(threadIndex);
            WaitConcurrentMarkingFinished();
        }
    }
    WaitRunningTaskFinished();
    memController_->RecordAfterConcurrentMark(markType_, concurrentMarker_);
    return true;
}

void Heap::ClearIdleTask()
{
    SetIdleTask(IdleTaskType::NO_TASK);
    idleTaskFinishTime_ = incrementalMarker_->GetCurrentTimeInMs();
}

void Heap::TryTriggerIdleCollection()
{
    if (idleTask_ != IdleTaskType::NO_TASK || !GetJSThread()->IsReadyToConcurrentMark() || !enableIdleGC_) {
        return;
    }
    if (thread_->IsMarkFinished() && concurrentMarker_->IsTriggeredConcurrentMark()) {
        SetIdleTask(IdleTaskType::FINISH_MARKING);
        EnableNotifyIdle();
        CalculateIdleDuration();
        return;
    }

    double newSpaceAllocSpeed = memController_->GetNewSpaceAllocationThroughputPerMS();
    double newSpaceConcurrentMarkSpeed = memController_->GetNewSpaceConcurrentMarkSpeedPerMS();
    double newSpaceAllocToLimitDuration = (static_cast<double>(activeSemiSpace_->GetInitialCapacity()) -
                                           static_cast<double>(activeSemiSpace_->GetCommittedSize())) /
                                           newSpaceAllocSpeed;
    double newSpaceMarkDuration = activeSemiSpace_->GetHeapObjectSize() / newSpaceConcurrentMarkSpeed;
    double newSpaceRemainSize = (newSpaceAllocToLimitDuration - newSpaceMarkDuration) * newSpaceAllocSpeed;
    // 2 means double
    if (newSpaceRemainSize < 2 * DEFAULT_REGION_SIZE) {
        SetIdleTask(IdleTaskType::YOUNG_GC);
        SetMarkType(MarkType::MARK_YOUNG);
        EnableNotifyIdle();
        CalculateIdleDuration();
        return;
    }
}

void Heap::CalculateIdleDuration()
{
    size_t updateReferenceSpeed = 0;
    // clear native object duration
    size_t clearNativeObjSpeed = 0;
    if (markType_ == MarkType::MARK_YOUNG) {
        updateReferenceSpeed = GetEcmaGCStats()->GetGCSpeed(SpeedData::YOUNG_UPDATE_REFERENCE_SPEED);
        clearNativeObjSpeed = GetEcmaGCStats()->GetGCSpeed(SpeedData::YOUNG_CLEAR_NATIVE_OBJ_SPEED);
    } else if (markType_ == MarkType::MARK_FULL) {
        updateReferenceSpeed = GetEcmaGCStats()->GetGCSpeed(SpeedData::UPDATE_REFERENCE_SPEED);
        clearNativeObjSpeed = GetEcmaGCStats()->GetGCSpeed(SpeedData::OLD_CLEAR_NATIVE_OBJ_SPEED);
    }

    // update reference duration
    idlePredictDuration_ = 0.0f;
    if (updateReferenceSpeed != 0) {
        idlePredictDuration_ += (float)GetHeapObjectSize() / updateReferenceSpeed;
    }

    if (clearNativeObjSpeed != 0) {
        idlePredictDuration_ += (float)GetNativePointerListSize() / clearNativeObjSpeed;
    }

    // sweep and evacuate duration
    size_t youngEvacuateSpeed = GetEcmaGCStats()->GetGCSpeed(SpeedData::YOUNG_EVACUATE_SPACE_SPEED);
    double survivalRate = GetEcmaGCStats()->GetAvgSurvivalRate();
    if (markType_ == MarkType::MARK_YOUNG && youngEvacuateSpeed != 0) {
        idlePredictDuration_ += activeSemiSpace_->GetHeapObjectSize() * survivalRate / youngEvacuateSpeed;
    } else if (markType_ == MarkType::MARK_FULL) {
        size_t sweepSpeed = GetEcmaGCStats()->GetGCSpeed(SpeedData::SWEEP_SPEED);
        size_t oldEvacuateSpeed = GetEcmaGCStats()->GetGCSpeed(SpeedData::OLD_EVACUATE_SPACE_SPEED);
        if (sweepSpeed != 0) {
            idlePredictDuration_ += (float)GetHeapObjectSize() / sweepSpeed;
        }
        if (oldEvacuateSpeed != 0) {
            size_t collectRegionSetSize = GetEcmaGCStats()->GetRecordData(
                RecordData::COLLECT_REGION_SET_SIZE);
            idlePredictDuration_ += (survivalRate * activeSemiSpace_->GetHeapObjectSize() + collectRegionSetSize) /
                                    oldEvacuateSpeed;
        }
    }

    // Idle YoungGC mark duration
    size_t markSpeed = GetEcmaGCStats()->GetGCSpeed(SpeedData::MARK_SPEED);
    if (idleTask_ == IdleTaskType::YOUNG_GC && markSpeed != 0) {
        idlePredictDuration_ += (float)activeSemiSpace_->GetHeapObjectSize() / markSpeed;
    }
    OPTIONAL_LOG(ecmaVm_, INFO) << "Predict idle gc pause: " << idlePredictDuration_ << "ms";
}

void Heap::TryTriggerIncrementalMarking()
{
    if (!GetJSThread()->IsReadyToConcurrentMark() || idleTask_ != IdleTaskType::NO_TASK || !enableIdleGC_) {
        return;
    }
    size_t oldSpaceAllocLimit = oldSpace_->GetInitialCapacity();
    size_t oldSpaceHeapObjectSize = oldSpace_->GetHeapObjectSize() + hugeObjectSpace_->GetHeapObjectSize() +
        hugeMachineCodeSpace_->GetHeapObjectSize();
    double oldSpaceAllocSpeed = memController_->GetOldSpaceAllocationThroughputPerMS();
    double oldSpaceIncrementalMarkSpeed = incrementalMarker_->GetAverageIncrementalMarkingSpeed();
    double oldSpaceAllocToLimitDuration = (oldSpaceAllocLimit - oldSpaceHeapObjectSize) / oldSpaceAllocSpeed;
    double oldSpaceMarkDuration = GetHeapObjectSize() / oldSpaceIncrementalMarkSpeed;

    double oldSpaceRemainSize = (oldSpaceAllocToLimitDuration - oldSpaceMarkDuration) * oldSpaceAllocSpeed;
    // mark finished before allocate limit
    if ((oldSpaceRemainSize < DEFAULT_REGION_SIZE) || GetHeapObjectSize() >= globalSpaceAllocLimit_) {
        // The object allocated in incremental marking should lower than limit,
        // otherwise select trigger concurrent mark.
        size_t allocateSize = oldSpaceAllocSpeed * oldSpaceMarkDuration;
        if (allocateSize < ALLOCATE_SIZE_LIMIT) {
            EnableNotifyIdle();
            SetIdleTask(IdleTaskType::INCREMENTAL_MARK);
        }
    }
}

bool Heap::CheckCanTriggerConcurrentMarking()
{
    return concurrentMarker_->IsEnabled() && thread_->IsReadyToConcurrentMark() &&
        !incrementalMarker_->IsTriggeredIncrementalMark() &&
        (idleTask_ == IdleTaskType::NO_TASK || idleTask_ == IdleTaskType::YOUNG_GC);
}

bool Heap::TryTriggerConcurrentMarking(MarkReason markReason)
{
    if (g_isEnableCMCGC) {
        return false;
    }
    // When concurrent marking is enabled, concurrent marking will be attempted to trigger.
    // When the size of old space or global space reaches the limit, isFullMarkNeeded will be set to true.
    // If the predicted duration of current full mark may not result in the new and old spaces reaching their limit,
    // full mark will be triggered.
    // In the same way, if the size of the new space reaches the capacity, and the predicted duration of current
    // young mark may not result in the new space reaching its limit, young mark can be triggered.
    // If it spends much time in full mark, the compress full GC will be requested when the spaces reach the limit.
    // If the global space is larger than half max heap size, we will turn to use full mark and trigger partial GC.
    if (!CheckCanTriggerConcurrentMarking()) {
        return false;
    }
    if (fullMarkRequested_) {
        markType_ = MarkType::MARK_FULL;
        OPTIONAL_LOG(ecmaVm_, INFO) << " fullMarkRequested, trigger full mark.";
        TriggerConcurrentMarking(markReason);
        return true;
    }
    if (InSensitiveStatus() && !ObjectExceedHighSensitiveThresholdForCM()) {
        return false;
    }
    if (IsJustFinishStartup() && !ObjectExceedJustFinishStartupThresholdForCM()) {
        return false;
    }

    double oldSpaceMarkDuration = 0;
    double newSpaceMarkDuration = 0;
    double newSpaceRemainSize = 0;
    double newSpaceAllocToLimitDuration = 0;
    double oldSpaceAllocToLimitDuration = 0;
    double oldSpaceAllocSpeed = memController_->GetOldSpaceAllocationThroughputPerMS();
    double oldSpaceConcurrentMarkSpeed = memController_->GetFullSpaceConcurrentMarkSpeedPerMS();
    size_t oldSpaceHeapObjectSize = oldSpace_->GetHeapObjectSize() + hugeObjectSpace_->GetHeapObjectSize() +
        hugeMachineCodeSpace_->GetHeapObjectSize();
    size_t globalHeapObjectSize = GetHeapObjectSize();
    size_t oldSpaceAllocLimit = oldSpace_->GetInitialCapacity();
    if (oldSpaceConcurrentMarkSpeed == 0 || oldSpaceAllocSpeed == 0) {
        if (oldSpaceHeapObjectSize >= oldSpaceAllocLimit || globalHeapObjectSize >= globalSpaceAllocLimit_ ||
            GlobalNativeSizeLargerThanLimit()) {
            markType_ = MarkType::MARK_FULL;
            OPTIONAL_LOG(ecmaVm_, INFO) << "Trigger the first full mark";
            TriggerConcurrentMarking(markReason);
            return true;
        }
    } else {
        if (oldSpaceHeapObjectSize >= oldSpaceAllocLimit || globalHeapObjectSize >= globalSpaceAllocLimit_ ||
            GlobalNativeSizeLargerThanLimit()) {
            markType_ = MarkType::MARK_FULL;
            TriggerConcurrentMarking(markReason);
            OPTIONAL_LOG(ecmaVm_, INFO) << "Trigger full mark";
            return true;
        }
        oldSpaceAllocToLimitDuration = (oldSpaceAllocLimit - oldSpaceHeapObjectSize) / oldSpaceAllocSpeed;
        oldSpaceMarkDuration = GetHeapObjectSize() / oldSpaceConcurrentMarkSpeed;
        // oldSpaceRemainSize means the predicted size which can be allocated after the full concurrent mark.
        double oldSpaceRemainSize = (oldSpaceAllocToLimitDuration - oldSpaceMarkDuration) * oldSpaceAllocSpeed;
        if (oldSpaceRemainSize > 0 && oldSpaceRemainSize < DEFAULT_REGION_SIZE) {
            markType_ = MarkType::MARK_FULL;
            TriggerConcurrentMarking(markReason);
            OPTIONAL_LOG(ecmaVm_, INFO) << "Trigger full mark";
            return true;
        }
    }

    double newSpaceAllocSpeed = memController_->GetNewSpaceAllocationThroughputPerMS();
    double newSpaceConcurrentMarkSpeed = memController_->GetNewSpaceConcurrentMarkSpeedPerMS();
    if (newSpaceConcurrentMarkSpeed == 0 || newSpaceAllocSpeed == 0) {
        if (activeSemiSpace_->GetCommittedSize() >= config_.GetSemiSpaceTriggerConcurrentMark()) {
            markType_ = MarkType::MARK_YOUNG;
            TriggerConcurrentMarking(markReason);
            OPTIONAL_LOG(ecmaVm_, INFO) << "Trigger the first semi mark" << fullGCRequested_;
            return true;
        }
        return false;
    }
    size_t semiSpaceCapacity = activeSemiSpace_->GetInitialCapacity() + activeSemiSpace_->GetOvershootSize();
    size_t semiSpaceCommittedSize = activeSemiSpace_->GetCommittedSize();
    bool triggerMark = semiSpaceCapacity <= semiSpaceCommittedSize;
    if (!triggerMark) {
        newSpaceAllocToLimitDuration = (semiSpaceCapacity - semiSpaceCommittedSize) / newSpaceAllocSpeed;
        newSpaceMarkDuration = activeSemiSpace_->GetHeapObjectSize() / newSpaceConcurrentMarkSpeed;
        // newSpaceRemainSize means the predicted size which can be allocated after the semi concurrent mark.
        newSpaceRemainSize = (newSpaceAllocToLimitDuration - newSpaceMarkDuration) * newSpaceAllocSpeed;
        triggerMark = newSpaceRemainSize < DEFAULT_REGION_SIZE;
    }

    if (triggerMark) {
        markType_ = MarkType::MARK_YOUNG;
        TriggerConcurrentMarking(markReason);
        OPTIONAL_LOG(ecmaVm_, INFO) << "Trigger semi mark";
        return true;
    }
    return false;
}

void Heap::TryTriggerFullMarkOrGCByNativeSize()
{
    // In high sensitive scene and native size larger than limit, trigger old gc directly
    if (InSensitiveStatus() && GlobalNativeSizeLargerToTriggerGC()) {
        CollectGarbage(TriggerGCType::OLD_GC, GCReason::NATIVE_LIMIT);
    } else if (GlobalNativeSizeLargerThanLimit()) {
        if (concurrentMarker_->IsEnabled()) {
            SetFullMarkRequestedState(true);
            TryTriggerConcurrentMarking(MarkReason::NATIVE_LIMIT);
        } else {
            CheckAndTriggerOldGC();
        }
    }
}

bool Heap::TryTriggerFullMarkBySharedLimit()
{
    bool keepFullMarkRequest = false;
    if (concurrentMarker_->IsEnabled()) {
        if (!CheckCanTriggerConcurrentMarking()) {
            return keepFullMarkRequest;
        }
        markType_ = MarkType::MARK_FULL;
        if (ConcurrentMarker::TryIncreaseTaskCounts()) {
            GetEcmaGCStats()->SetMarkReason(MarkReason::SHARED_LIMIT);
            concurrentMarker_->Mark();
        } else {
            // need retry full mark request again.
            keepFullMarkRequest = true;
        }
    }
    return keepFullMarkRequest;
}

void Heap::CheckAndTriggerTaskFinishedGC()
{
    if (g_isEnableCMCGC) {
        return;
    }
    size_t objectSizeOfTaskBegin = GetRecordObjectSize();
    size_t objectSizeOfTaskFinished = GetHeapObjectSize();
    size_t nativeSizeOfTaskBegin = GetRecordNativeSize();
    size_t nativeSizeOfTaskFinished = GetGlobalNativeSize();
    // GC would be triggered when heap size increase more than Max(20M, 10%*SizeOfTaskBegin)
    bool objectSizeFlag = objectSizeOfTaskFinished > objectSizeOfTaskBegin &&
        objectSizeOfTaskFinished - objectSizeOfTaskBegin > std::max(TRIGGER_OLDGC_OBJECT_SIZE_LIMIT,
            TRIGGER_OLDGC_OBJECT_LIMIT_RATE * objectSizeOfTaskBegin);
    bool nativeSizeFlag = nativeSizeOfTaskFinished > nativeSizeOfTaskBegin &&
        nativeSizeOfTaskFinished - nativeSizeOfTaskBegin > std::max(TRIGGER_OLDGC_NATIVE_SIZE_LIMIT,
            TRIGGER_OLDGC_NATIVE_LIMIT_RATE * nativeSizeOfTaskBegin);
    if (objectSizeFlag || nativeSizeFlag) {
        CollectGarbage(TriggerGCType::OLD_GC, GCReason::TRIGGER_BY_TASKPOOL);
        RecordOrResetObjectSize(0);
        RecordOrResetNativeSize(0);
    }
}

bool Heap::IsMarking() const
{
    return thread_->IsMarking();
}

void Heap::TryTriggerFullMarkBySharedSize(size_t size)
{
    newAllocatedSharedObjectSize_ += size;
    if (newAllocatedSharedObjectSize_ >= NEW_ALLOCATED_SHARED_OBJECT_SIZE_LIMIT) {
        if (thread_->IsMarkFinished() && GetConcurrentMarker()->IsTriggeredConcurrentMark() &&
            !GetOnSerializeEvent() && InSensitiveStatus()) {
            GetConcurrentMarker()->HandleMarkingFinished(GCReason::SHARED_LIMIT);
            newAllocatedSharedObjectSize_ = 0;
        } else if (concurrentMarker_->IsEnabled()) {
            SetFullMarkRequestedState(true);
            TryTriggerConcurrentMarking(MarkReason::SHARED_LIMIT);
            newAllocatedSharedObjectSize_ = 0;
        }
    }
}

bool Heap::IsReadyToConcurrentMark() const
{
    return thread_->IsReadyToConcurrentMark();
}

void Heap::IncreaseNativeBindingSize(JSNativePointer *object)
{
    size_t size = object->GetBindingSize();
    IncreaseNativeBindingSize(size);
}

void Heap::IncreaseNativeBindingSize(size_t size)
{
    if (size == 0) {
        return;
    }
    nativeBindingSize_ += size;
}

void Heap::DecreaseNativeBindingSize(size_t size)
{
    ASSERT(size <= nativeBindingSize_);
    nativeBindingSize_ -= size;
}

void Heap::PrepareRecordRegionsForReclaim()
{
    activeSemiSpace_->SetRecordRegion();
    oldSpace_->SetRecordRegion();
    snapshotSpace_->SetRecordRegion();
    nonMovableSpace_->SetRecordRegion();
    hugeObjectSpace_->SetRecordRegion();
    machineCodeSpace_->SetRecordRegion();
    hugeMachineCodeSpace_->SetRecordRegion();
}

void Heap::TriggerConcurrentMarking(MarkReason markReason)
{
    ASSERT(idleTask_ != IdleTaskType::INCREMENTAL_MARK);
    if (idleTask_ == IdleTaskType::YOUNG_GC && IsConcurrentFullMark()) {
        ClearIdleTask();
        DisableNotifyIdle();
    }
    if (concurrentMarker_->IsEnabled() && !fullGCRequested_ && ConcurrentMarker::TryIncreaseTaskCounts()) {
        GetEcmaGCStats()->SetMarkReason(markReason);
        concurrentMarker_->Mark();
    }
}

void Heap::WaitAllTasksFinished()
{
    WaitRunningTaskFinished();
    sweeper_->EnsureAllTaskFinished();
    WaitClearTaskFinished();
    if (concurrentMarker_->IsEnabled() && thread_->IsMarking() && concurrentMarker_->IsTriggeredConcurrentMark()) {
        concurrentMarker_->WaitMarkingFinished();
    }
}

void Heap::WaitConcurrentMarkingFinished()
{
    concurrentMarker_->WaitMarkingFinished();
}

void Heap::PostParallelGCTask(ParallelGCTaskPhase gcTask)
{
    IncreaseTaskCount();
    common::Taskpool::GetCurrentTaskpool()->PostTask(
        std::make_unique<ParallelGCTask>(GetJSThread()->GetThreadId(), this, gcTask));
}

void Heap::ChangeGCParams(bool inBackground)
{
    const double doubleOne = 1.0;
    inBackground_ = inBackground;
    if (g_isEnableCMCGC) {
        common::BaseRuntime::ChangeGCParams(inBackground);
        return;
    }
    if (inBackground) {
        LOG_GC(INFO) << "app is inBackground";
        if (GetHeapObjectSize() - heapAliveSizeAfterGC_ > BACKGROUND_GROW_LIMIT &&
            GetCommittedSize() >= MIN_BACKGROUNG_GC_LIMIT &&
            doubleOne * GetHeapObjectSize() / GetCommittedSize() <= MIN_OBJECT_SURVIVAL_RATE) {
            CollectGarbage(TriggerGCType::FULL_GC, GCReason::SWITCH_BACKGROUND);
        }
        if (GetMemGrowingType() != MemGrowingType::PRESSURE) {
            SetMemGrowingType(MemGrowingType::CONSERVATIVE);
            LOG_GC(DEBUG) << "Heap Growing Type CONSERVATIVE";
        }
        common::Taskpool::GetCurrentTaskpool()->SetThreadPriority(common::PriorityMode::BACKGROUND);
    } else {
        LOG_GC(INFO) << "app is not inBackground";
        if (GetMemGrowingType() != MemGrowingType::PRESSURE) {
            SetMemGrowingType(MemGrowingType::HIGH_THROUGHPUT);
            LOG_GC(DEBUG) << "Heap Growing Type HIGH_THROUGHPUT";
        }
        concurrentMarker_->EnableConcurrentMarking(EnableConcurrentMarkType::ENABLE);
        sweeper_->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
        maxMarkTaskCount_ = std::min<size_t>(ecmaVm_->GetJSOptions().GetGcThreadNum(),
            common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() - 1);
        maxEvacuateTaskCount_ = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
        common::Taskpool::GetCurrentTaskpool()->SetThreadPriority(common::PriorityMode::FOREGROUND);
    }
}

GCStats *Heap::GetEcmaGCStats()
{
    return ecmaVm_->GetEcmaGCStats();
}

GCKeyStats *Heap::GetEcmaGCKeyStats()
{
    return ecmaVm_->GetEcmaGCKeyStats();
}

JSObjectResizingStrategy *Heap::GetJSObjectResizingStrategy()
{
    return ecmaVm_->GetJSObjectResizingStrategy();
}

void Heap::TriggerIdleCollection(int idleMicroSec)
{
    if (idleTask_ == IdleTaskType::NO_TASK) {
        if (incrementalMarker_->GetCurrentTimeInMs() - idleTaskFinishTime_ > IDLE_MAINTAIN_TIME) {
            DisableNotifyIdle();
        }
        return;
    }

    // Incremental mark initialize and process
    if (idleTask_ == IdleTaskType::INCREMENTAL_MARK &&
        incrementalMarker_->GetIncrementalGCStates() != IncrementalGCStates::REMARK) {
        incrementalMarker_->TriggerIncrementalMark(idleMicroSec);
        if (incrementalMarker_->GetIncrementalGCStates() == IncrementalGCStates::REMARK) {
            CalculateIdleDuration();
        }
        return;
    }

    if (idleMicroSec < idlePredictDuration_ && idleMicroSec < IDLE_TIME_LIMIT) {
        return;
    }

    switch (idleTask_) {
        case IdleTaskType::FINISH_MARKING: {
            if (markType_ == MarkType::MARK_FULL) {
                CollectGarbage(TriggerGCType::OLD_GC, GCReason::IDLE);
            } else {
                CollectGarbage(TriggerGCType::YOUNG_GC, GCReason::IDLE);
            }
            break;
        }
        case IdleTaskType::YOUNG_GC:
            CollectGarbage(TriggerGCType::YOUNG_GC, GCReason::IDLE);
            break;
        case IdleTaskType::INCREMENTAL_MARK:
            incrementalMarker_->TriggerIncrementalMark(idleMicroSec);
            break;
        default: // LCOV_EXCL_BR_LINE
            break;
    }
    ClearIdleTask();
}

void Heap::NotifyMemoryPressure(bool inHighMemoryPressure)
{
    if (inHighMemoryPressure) {
        LOG_GC(INFO) << "app is inHighMemoryPressure";
        SetMemGrowingType(MemGrowingType::PRESSURE);
    } else {
        LOG_GC(INFO) << "app is not inHighMemoryPressure";
        SetMemGrowingType(MemGrowingType::CONSERVATIVE);
    }
}

void Heap::NotifyFinishColdStart(bool isMainThread)
{
    if (!FinishStartupEvent()) {
        LOG_GC(WARN) << "SmartGC: app cold start last status is not ON_STARTUP, just return";
        return;
    }
    ASSERT(!OnStartupEvent());
    LOG_GC(INFO) << "SmartGC: app cold start just finished";

    if (isMainThread && ObjectExceedJustFinishStartupThresholdForCM()) {
        TryTriggerConcurrentMarking(MarkReason::OTHER);
    }

    auto startIdleMonitor = JSNApi::GetStartIdleMonitorCallback();
    if (startIdleMonitor != nullptr) {
        startIdleMonitor();
    }

    if (startupDurationInMs_ == 0) {
        startupDurationInMs_ = DEFAULT_STARTUP_DURATION_MS;
    }

    // restrain GC from 2s to 8s
    uint64_t delayTimeInMs = FINISH_STARTUP_TIMEPOINT_MS - startupDurationInMs_;
    common::Taskpool::GetCurrentTaskpool()->PostDelayedTask(
        std::make_unique<FinishGCRestrainTask>(GetJSThread()->GetThreadId(), this),
        delayTimeInMs);
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
        "SmartGC: app startup just finished, FinishGCRestrainTask create", "");
}

void Heap::NotifyFinishColdStartSoon()
{
    if (!OnStartupEvent()) {
        return;
    }

    // post 2s task
    startupDurationInMs_ = DEFAULT_STARTUP_DURATION_MS;
#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT) && defined(PANDA_TARGET_OHOS) && defined(ENABLE_HISYSEVENT)
    startupDurationInMs_ = OHOS::system::GetUintParameter<uint64_t>("persist.ark.startupDuration",
                                                                    DEFAULT_STARTUP_DURATION_MS);
    startupDurationInMs_ = std::max(startupDurationInMs_, static_cast<uint64_t>(MIN_CONFIGURABLE_STARTUP_DURATION_MS));
    startupDurationInMs_ = std::min(startupDurationInMs_, static_cast<uint64_t>(MAX_CONFIGURABLE_STARTUP_DURATION_MS));
#endif
    common::Taskpool::GetCurrentTaskpool()->PostDelayedTask(
        std::make_unique<FinishColdStartTask>(GetJSThread()->GetThreadId(), this),
        startupDurationInMs_);
}

void Heap::NotifyWarmStartup()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SmartGC: warm startup GC restrain start", "");
    // warm startup use the same GC restrain policy as cold startup
    LOG_GC(INFO) << "SmartGC: warm startup use the same GC restrain policy as cold startup";
    if (g_isEnableCMCGC) {
        common::BaseRuntime::NotifyWarmStart();
        return;
    }
    if (AllowWarmStartGcRestrain()) {
        NotifyPostFork();
        NotifyFinishColdStartSoon();
    }
}

bool Heap::AllowWarmStartGcRestrain()
{
    bool hasFinishedMark = ecmaVm_->GetJSThread()->IsConcurrentMarkingOrFinished();
    if (hasFinishedMark) {
        LOG_ECMA(WARN) << "SmartGC: app warm start gc, but gc mark is concurrent marking or finished, skip";
        return false;
    }
    StartupStatus status = GetStartupStatus();
    bool isGcRestraining = (status == StartupStatus::ON_STARTUP || status == StartupStatus::JUST_FINISH_STARTUP);
    if (isGcRestraining) {
        LOG_ECMA(WARN) << "SmartGC: app warm start gc, but it is already in GC restraining now, skip";
        return false;
    }
    return true;
}

void Heap::NotifyHighSensitive(bool isStart)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
        ("SmartGC: set high sensitive status: " + std::to_string(isStart)).c_str(), "");
    isStart ? SetSensitiveStatus(AppSensitiveStatus::ENTER_HIGH_SENSITIVE)
        : SetSensitiveStatus(AppSensitiveStatus::EXIT_HIGH_SENSITIVE);
    LOG_GC(DEBUG) << "SmartGC: set high sensitive status: " << isStart;

    if (g_isEnableCMCGC) {
        common::BaseRuntime::NotifyHighSensitive(isStart);
    }
}

bool Heap::HandleExitHighSensitiveEvent()
{
    AppSensitiveStatus status = GetSensitiveStatus();
    if (status == AppSensitiveStatus::EXIT_HIGH_SENSITIVE
        && CASSensitiveStatus(status, AppSensitiveStatus::NORMAL_SCENE) && !OnStartupEvent()) {
        // Set record heap obj size 0 after exit high senstive
        SetRecordHeapObjectSizeBeforeSensitive(0);

        // fixme: IncrementalMarking and IdleCollection is currently not enabled
        TryTriggerIncrementalMarking();
        TryTriggerIdleCollection();
        bool success = TryTriggerConcurrentMarking(MarkReason::EXIT_HIGH_SENSITIVE);
        if (success) {
            // set overshoot size to increase gc threashold larger 8MB than current heap size.
            TryIncreaseNewSpaceOvershootByConfigSize();
        }
        return success;
    }
    return false;
}

// On high sensitive scene, heap object size can reach to MaxHeapSize - 8M temporarily, 8M is reserved for
// concurrent mark
bool Heap::ObjectExceedMaxHeapSize() const
{
    size_t configMaxHeapSize = config_.GetMaxHeapSize();
    size_t overshootSize = config_.GetOldSpaceStepOvershootSize();
    return GetHeapObjectSize() > configMaxHeapSize - overshootSize;
}

bool Heap::ObjectExceedHighSensitiveThresholdForCM() const
{
    size_t recordSizeBeforeSensitive = GetRecordHeapObjectSizeBeforeSensitive();
    return GetHeapObjectSize() > (recordSizeBeforeSensitive + config_.GetIncObjSizeThresholdInSensitive())
                                 * MIN_SENSITIVE_OBJECT_SURVIVAL_RATE;
}

bool Heap::ObjectExceedJustFinishStartupThresholdForGC() const
{
    size_t heapObjectSizeThresholdForGC = config_.GetMaxHeapSize() * JUST_FINISH_STARTUP_LOCAL_THRESHOLD_RATIO;
    return GetHeapObjectSize() > heapObjectSizeThresholdForGC;
}

bool Heap::ObjectExceedJustFinishStartupThresholdForCM() const
{
    size_t heapObjectSizeThresholdForGC = config_.GetMaxHeapSize() * JUST_FINISH_STARTUP_LOCAL_THRESHOLD_RATIO;
    size_t heapObjectSizeThresholdForCM = heapObjectSizeThresholdForGC
                                        * JUST_FINISH_STARTUP_LOCAL_CONCURRENT_MARK_RATIO;
    return GetHeapObjectSize() > heapObjectSizeThresholdForCM;
}

void Heap::TryIncreaseNewSpaceOvershootByConfigSize()
{
    if (InGC() || !IsReadyToConcurrentMark()) {
        // overShootSize will be adjusted when resume heap during GC and
        // no need to reserve space for newSpace if ConcurrentMark is already triggered
        return;
    }
    // need lock because conflict may occur when handle exit sensitive status by main thread
    // and handle finish startup by child thread happen at the same time
    LockHolder lock(setNewSpaceOvershootSizeMutex_);
    // set overshoot size to increase gc threashold larger 8MB than current heap size.
    int64_t initialCapacity = static_cast<int64_t>(GetNewSpace()->GetInitialCapacity());
    int64_t committedSize = static_cast<int64_t>(GetNewSpace()->GetCommittedSize());
    int64_t semiRemainSize = initialCapacity - committedSize;
    int64_t overshootSize =
        static_cast<int64_t>(config_.GetOldSpaceStepOvershootSize()) - semiRemainSize;
    // overshoot size should be larger than 0.
    GetNewSpace()->SetOverShootSize(std::max(overshootSize, (int64_t)0));
}

void Heap::TryIncreaseOvershootByConfigSize()
{
    TryIncreaseNewSpaceOvershootByConfigSize();
    sHeap_->TryAdjustSpaceOvershootByConfigSize();
}

bool Heap::CheckIfNeedStopCollectionByStartup()
{
    StartupStatus startupStatus = GetStartupStatus();
    switch (startupStatus) {
        case StartupStatus::ON_STARTUP:
            // During app cold start, gc threshold adjust to max heap size
            if (!ObjectExceedMaxHeapSize()) {
                return true;
            }
            break;
        case StartupStatus::JUST_FINISH_STARTUP:
            // During app cold start just finished, gc threshold adjust to a quarter of max heap size
            if (!ObjectExceedJustFinishStartupThresholdForGC()) {
                return true;
            }
            break;
        default:
            break;
    }
    return false;
}

bool Heap::CheckIfNeedStopCollectionByHighSensitive()
{
    AppSensitiveStatus sensitiveStatus = GetSensitiveStatus();
    if (sensitiveStatus != AppSensitiveStatus::ENTER_HIGH_SENSITIVE) {
        return false;
    }

    size_t objSize = GetHeapObjectSize();
    size_t recordSizeBeforeSensitive = GetRecordHeapObjectSizeBeforeSensitive();
    if (recordSizeBeforeSensitive == 0) {
        recordSizeBeforeSensitive = objSize;
        SetRecordHeapObjectSizeBeforeSensitive(recordSizeBeforeSensitive);
    }

    if (objSize < recordSizeBeforeSensitive + config_.GetIncObjSizeThresholdInSensitive()
        && !ObjectExceedMaxHeapSize()) {
        if (!IsNearGCInSensitive() &&
            objSize > (recordSizeBeforeSensitive + config_.GetIncObjSizeThresholdInSensitive())
            * MIN_SENSITIVE_OBJECT_SURVIVAL_RATE) {
            SetNearGCInSensitive(true);
        }
        return true;
    }

    OPTIONAL_LOG(ecmaVm_, INFO) << "SmartGC: heap obj size: " << GetHeapObjectSize()
        << ", exceed sensitive gc threshold";
    return false;
}

bool Heap::NeedStopCollection()
{
    // gc is not allowed during value serialize
    if (onSerializeEvent_) {
        return true;
    }

    // check high sensitive before checking startup because we still need to record
    // current heap object size when high sensitive happens in startup duration
    if (CheckIfNeedStopCollectionByHighSensitive()) {
        return true;
    }

    if (CheckIfNeedStopCollectionByStartup()) {
        return true;
    }

    return false;
}

bool Heap::ParallelGCTask::Run(uint32_t threadIndex)
{
    // Synchronizes-with. Ensure that WorkManager::Initialize must be seen by MarkerThreads.
    ASSERT(heap_->GetWorkManager()->HasInitialized());
    while (!heap_->GetWorkManager()->HasInitialized());
    switch (taskPhase_) {
        case ParallelGCTaskPhase::OLD_HANDLE_GLOBAL_POOL_TASK:
            heap_->GetNonMovableMarker()->ProcessMarkStack(threadIndex);
            break;
        case ParallelGCTaskPhase::COMPRESS_HANDLE_GLOBAL_POOL_TASK:
            heap_->GetCompressGCMarker()->ProcessMarkStack(threadIndex);
            break;
        case ParallelGCTaskPhase::CONCURRENT_HANDLE_GLOBAL_POOL_TASK:
            heap_->GetConcurrentMarker()->ProcessConcurrentMarkTask(threadIndex);
            break;
        case ParallelGCTaskPhase::UNIFIED_HANDLE_GLOBAL_POOL_TASK:
            heap_->GetUnifiedGCMarker()->ProcessMarkStack(threadIndex);
            break;
        default: // LOCV_EXCL_BR_LINE
            LOG_GC(FATAL) << "this branch is unreachable, type: " << static_cast<int>(taskPhase_);
            UNREACHABLE();
    }
    heap_->ReduceTaskCount();
    return true;
}

bool Heap::AsyncClearTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "AsyncClearTask::Run", "");
    heap_->ReclaimRegions(gcType_);
    return true;
}

bool Heap::FinishColdStartTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    heap_->NotifyFinishColdStart(false);
    return true;
}

bool Heap::FinishGCRestrainTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    heap_->CancelJustFinishStartupEvent();
    LOG_GC(INFO) << "SmartGC: app cold start finished";
    return true;
}

void Heap::CleanCallback()
{
    auto &concurrentCallbacks = this->GetEcmaVM()->GetConcurrentNativePointerCallbacks();
    if (!concurrentCallbacks.empty()) {
        common::Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<DeleteCallbackTask>(thread_->GetThreadId(), concurrentCallbacks)
        );
    }
    ASSERT(concurrentCallbacks.empty());

    AsyncNativeCallbacksPack &asyncCallbacksPack = this->GetEcmaVM()->GetAsyncNativePointerCallbacksPack();
    if (asyncCallbacksPack.Empty()) {
        ASSERT(asyncCallbacksPack.TotallyEmpty());
        return;
    }
    AsyncNativeCallbacksPack *asyncCallbacks = new AsyncNativeCallbacksPack();
    std::swap(*asyncCallbacks, asyncCallbacksPack);
    NativePointerTaskCallback asyncTaskCb = thread_->GetAsyncCleanTaskCallback();
    size_t currentSize = 0;
    if (g_isEnableCMCGC) {
        currentSize = asyncCallbacks->GetTotalBindingSize();
    }
    if (asyncTaskCb != nullptr && thread_->IsMainThreadFast() &&
        (pendingAsyncNativeCallbackSize_ + currentSize) < asyncClearNativePointerThreshold_) {
        IncreasePendingAsyncNativeCallbackSize(asyncCallbacks->GetTotalBindingSize());
        asyncCallbacks->RegisterFinishNotify([this] (size_t bindingSize) {
            this->DecreasePendingAsyncNativeCallbackSize(bindingSize);
        });
        asyncTaskCb(asyncCallbacks);
    } else {
        ThreadNativeScope nativeScope(thread_);
        asyncCallbacks->ProcessAll("ArkCompiler");
        delete asyncCallbacks;
    }
    ASSERT(asyncCallbacksPack.TotallyEmpty());
}

bool Heap::DeleteCallbackTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    for (auto iter : nativePointerCallbacks_) {
        if (iter.first != nullptr) {
            iter.first(std::get<0>(iter.second),
                std::get<1>(iter.second), std::get<2>(iter.second)); // 2 is the param.
        }
    }
    return true;
}

size_t Heap::GetArrayBufferSize() const
{
    size_t result = 0;
    sweeper_->EnsureAllTaskFinished();
    this->IterateOverObjects([&result](TaggedObject *obj) {
        JSHClass* jsClass = obj->GetClass();
        result += jsClass->IsArrayBuffer() ? jsClass->GetObjectSize() : 0;
    });
    return result;
}

size_t Heap::GetLiveObjectSize() const
{
    size_t objectSize = 0;
    sweeper_->EnsureAllTaskFinished();
    this->IterateOverObjects([&objectSize]([[maybe_unused]] TaggedObject *obj) {
        objectSize += obj->GetSize();
    });
    return objectSize;
}

size_t Heap::GetHeapLimitSize() const
{
    // Obtains the theoretical upper limit of space that can be allocated to JS heap.
    return config_.GetMaxHeapSize();
}

bool BaseHeap::IsAlive(TaggedObject *object) const
{
    if (g_isEnableCMCGC) {
        if (!common::Heap::IsHeapAddress(object)) {
            LOG_GC(ERROR) << "The region is already free";
            return false;
        }
    } else {
        if (!ContainObject(object)) {
            LOG_GC(ERROR) << "The region is already free";
            return false;
        }
    }

    bool isFree = object->GetClass() != nullptr && FreeObject::Cast(ToUintPtr(object))->IsFreeObject();
    if (isFree) {
        Region *region = Region::ObjectAddressToRange(object);
        LOG_GC(ERROR) << "The object " << object << " in "
                            << region->GetSpaceTypeName()
                            << " already free";
    }
    return !isFree;
}

bool BaseHeap::ContainObject(TaggedObject *object) const
{
    /*
     * fixme: There's no absolutely safe appraoch to doing this, given that the region object is currently
     * allocated and maintained in the JS object heap. We cannot safely tell whether a region object
     * calculated from an object address is still valid or alive in a cheap way.
     * This will introduce inaccurate result to verify if an object is contained in the heap, and it may
     * introduce additional incorrect memory access issues.
     * Unless we can tolerate the performance impact of iterating the region list of each space and change
     * the implementation to that approach, don't rely on current implementation to get accurate result.
     */
    Region *region = Region::ObjectAddressToRange(object);
    return region->InHeapSpace();
}

void SharedHeap::UpdateHeapStatsAfterGC(TriggerGCType gcType)
{
    heapAliveSizeAfterGC_ = GetHeapObjectSize();
    fragmentSizeAfterGC_ = GetCommittedSize() - GetHeapObjectSize();
    if (gcType == TriggerGCType::SHARED_FULL_GC) {
        heapBasicLoss_ = fragmentSizeAfterGC_;
    }
}

void Heap::UpdateHeapStatsAfterGC(TriggerGCType gcType)
{
    if (gcType == TriggerGCType::YOUNG_GC) {
        return;
    }
    heapAliveSizeAfterGC_ = GetHeapObjectSize();
    heapAliveSizeExcludesYoungAfterGC_ = heapAliveSizeAfterGC_ - activeSemiSpace_->GetHeapObjectSize();
    fragmentSizeAfterGC_ = GetCommittedSize() - heapAliveSizeAfterGC_;
    if (gcType == TriggerGCType::FULL_GC) {
        heapBasicLoss_ = fragmentSizeAfterGC_;
    }
}

void Heap::PrintHeapInfo(TriggerGCType gcType) const
{
    OPTIONAL_LOG(ecmaVm_, INFO) << "-----------------------Statistic Heap Object------------------------";
    OPTIONAL_LOG(ecmaVm_, INFO) << "GC Reason:" << ecmaVm_->GetEcmaGCStats()->GCReasonToString()
                                << ";OnStartup:" << static_cast<int>(GetStartupStatus())
                                << ";OnHighSensitive:" << static_cast<int>(GetSensitiveStatus())
                                << ";ConcurrentMark Status:" << static_cast<int>(thread_->GetMarkStatus());
    OPTIONAL_LOG(ecmaVm_, INFO) << "Heap::CollectGarbage, gcType(" << gcType << "), Concurrent Mark("
                                << concurrentMarker_->IsEnabled() << "), Full Mark(" << IsConcurrentFullMark();
    OPTIONAL_LOG(ecmaVm_, INFO) << "), ActiveSemi(" << activeSemiSpace_->GetHeapObjectSize() << "/"
                 << activeSemiSpace_->GetInitialCapacity() << "), NonMovable(" << nonMovableSpace_->GetHeapObjectSize()
                 << "/" << nonMovableSpace_->GetCommittedSize() << "/" << nonMovableSpace_->GetInitialCapacity()
                 << "), Old(" << oldSpace_->GetHeapObjectSize() << "/" << oldSpace_->GetCommittedSize() << "/"
                 << oldSpace_->GetInitialCapacity() << "), HugeObject(" << hugeObjectSpace_->GetHeapObjectSize() << "/"
                 << hugeObjectSpace_->GetCommittedSize() << "/" << hugeObjectSpace_->GetInitialCapacity()
                 << "), ReadOnlySpace(" << readOnlySpace_->GetCommittedSize() << "/"
                 << readOnlySpace_->GetInitialCapacity() << "), AppspawnSpace(" << appSpawnSpace_->GetHeapObjectSize()
                 << "/" << appSpawnSpace_->GetCommittedSize() << "/" << appSpawnSpace_->GetInitialCapacity()
                 << "), NativeBindingSize(" << nativeBindingSize_
                 << "), NativeLimitSize(" << globalSpaceNativeLimit_
                 << "), GlobalLimitSize(" << globalSpaceAllocLimit_ << ").";
}

void Heap::StatisticHeapObject(TriggerGCType gcType) const
{
    PrintHeapInfo(gcType);
#if ECMASCRIPT_ENABLE_HEAP_DETAIL_STATISTICS
    StatisticHeapDetail();
#endif
}

void Heap::StatisticHeapDetail()
{
    Prepare();
    static const int JS_TYPE_SUM = static_cast<int>(JSType::TYPE_LAST) + 1;
    int typeCount[JS_TYPE_SUM] = { 0 };
    static const int MIN_COUNT_THRESHOLD = 1000;

    nonMovableSpace_->IterateOverObjects([&typeCount] (TaggedObject *object) {
        typeCount[static_cast<int>(object->GetClass()->GetObjectType())]++;
    });
    for (int i = 0; i < JS_TYPE_SUM; i++) {
        if (typeCount[i] > MIN_COUNT_THRESHOLD) {
            LOG_ECMA(INFO) << "NonMovable space type " << JSHClass::DumpJSType(JSType(i))
                           << " count:" << typeCount[i];
        }
        typeCount[i] = 0;
    }

    oldSpace_->IterateOverObjects([&typeCount] (TaggedObject *object) {
        typeCount[static_cast<int>(object->GetClass()->GetObjectType())]++;
    });
    for (int i = 0; i < JS_TYPE_SUM; i++) {
        if (typeCount[i] > MIN_COUNT_THRESHOLD) {
            LOG_ECMA(INFO) << "Old space type " << JSHClass::DumpJSType(JSType(i))
                           << " count:" << typeCount[i];
        }
        typeCount[i] = 0;
    }

    activeSemiSpace_->IterateOverObjects([&typeCount] (TaggedObject *object) {
        typeCount[static_cast<int>(object->GetClass()->GetObjectType())]++;
    });
    for (int i = 0; i < JS_TYPE_SUM; i++) {
        if (typeCount[i] > MIN_COUNT_THRESHOLD) {
            LOG_ECMA(INFO) << "Active semi space type " << JSHClass::DumpJSType(JSType(i))
                           << " count:" << typeCount[i];
        }
        typeCount[i] = 0;
    }
}

void Heap::UpdateWorkManager(WorkManager *workManager)
{
    concurrentMarker_->workManager_ = workManager;
    fullGC_->workManager_ = workManager;
    incrementalMarker_->workManager_ = workManager;
    nonMovableMarker_->workManager_ = workManager;
    compressGCMarker_->workManager_ = workManager;
    if (Runtime::GetInstance()->IsHybridVm()) {
        unifiedGCMarker_->workManager_ = workManager;
    }
    partialGC_->workManager_ = workManager;
}

MachineCode *Heap::GetMachineCodeObject(uintptr_t pc) const
{
    MachineCodeSpace *machineCodeSpace = GetMachineCodeSpace();
    MachineCode *machineCode = reinterpret_cast<MachineCode*>(machineCodeSpace->GetMachineCodeObject(pc));
    if (machineCode != nullptr) {
        return machineCode;
    }
    HugeMachineCodeSpace *hugeMachineCodeSpace = GetHugeMachineCodeSpace();
    return reinterpret_cast<MachineCode*>(hugeMachineCodeSpace->GetMachineCodeObject(pc));
}

void Heap::SetMachineCodeObject(uintptr_t start, uintptr_t end, uintptr_t address) const
{
    machineCodeSpace_->StoreMachineCodeObjectLocation(start, end, address);
}

std::tuple<uint64_t, uint8_t *, int, kungfu::CalleeRegAndOffsetVec> Heap::CalCallSiteInfo(uintptr_t retAddr) const
{
    MachineCodeSpace *machineCodeSpace = GetMachineCodeSpace();
    MachineCode *code = nullptr;
    // 1. find return
    // 2. gc
    machineCodeSpace->IterateOverObjects([&code, &retAddr](TaggedObject *obj) {
        if (code != nullptr || !JSTaggedValue(obj).IsMachineCodeObject()) {
            return;
        }
        if (MachineCode::Cast(obj)->IsInText(retAddr)) {
            code = MachineCode::Cast(obj);
            return;
        }
    });
    if (code == nullptr) {
        HugeMachineCodeSpace *hugeMachineCodeSpace = GetHugeMachineCodeSpace();
        hugeMachineCodeSpace->IterateOverObjects([&code, &retAddr](TaggedObject *obj) {
            if (code != nullptr || !JSTaggedValue(obj).IsMachineCodeObject()) {
                return;
            }
            if (MachineCode::Cast(obj)->IsInText(retAddr)) {
                code = MachineCode::Cast(obj);
                return;
            }
        });
    }

    if (code == nullptr ||
        (code->GetPayLoadSizeInBytes() ==
         code->GetInstructionsSize() + code->GetStackMapOrOffsetTableSize())) { // baseline code
        return {};
    }
    return code->CalCallSiteInfo();
};

GCListenerId Heap::AddGCListener(FinishGCListener listener, void *data)
{
    gcListeners_.emplace_back(std::make_pair(listener, data));
    return std::prev(gcListeners_.cend());
}

void Heap::ProcessGCListeners()
{
    for (auto &&[listener, data] : gcListeners_) {
        listener(data);
    }
}

void SharedHeap::ProcessAllGCListeners()
{
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        const_cast<Heap *>(thread->GetEcmaVM()->GetHeap())->ProcessGCListeners();
    });
}

#if defined(ECMASCRIPT_SUPPORT_SNAPSHOT) && defined(PANDA_TARGET_OHOS) && defined(ENABLE_HISYSEVENT)
uint64_t Heap::GetCurrentTickMillseconds()
{
    return std::chrono::duration_cast<std::chrono::milliseconds>(
    std::chrono::steady_clock::now().time_since_epoch()).count();
}

void Heap::SetJsDumpThresholds(size_t thresholds) const
{
    if (thresholds < MIN_JSDUMP_THRESHOLDS || thresholds > MAX_JSDUMP_THRESHOLDS) {
        LOG_GC(INFO) << "SetJsDumpThresholds thresholds is invaild" << thresholds;
        return;
    }
    g_threshold = thresholds;
}

void Heap::ThresholdReachedDump()
{
    size_t limitSize = GetHeapLimitSize();
    if (!limitSize) {
        LOG_GC(INFO) << "ThresholdReachedDump limitSize is invaild";
        return;
    }
    size_t nowPrecent = GetHeapObjectSize() * DEC_TO_INT / limitSize;
    if (g_debugLeak || (nowPrecent >= g_threshold && (g_lastHeapDumpTime == 0 ||
        GetCurrentTickMillseconds() - g_lastHeapDumpTime > HEAP_DUMP_REPORT_INTERVAL))) {
            size_t liveObjectSize = GetLiveObjectSize();
            size_t nowPrecentRecheck = liveObjectSize * DEC_TO_INT / limitSize;
            LOG_GC(INFO) << "ThresholdReachedDump nowPrecentCheck is " << nowPrecentRecheck;
            if (nowPrecentRecheck < g_threshold) {
                return;
            }
            g_lastHeapDumpTime = GetCurrentTickMillseconds();
            base::BlockHookScope blockScope;
            HeapProfilerInterface *heapProfile = HeapProfilerInterface::GetInstance(ecmaVm_);
            AppFreezeFilterCallback appfreezeCallback = Runtime::GetInstance()->GetAppFreezeFilterCallback();
            std::string eventConfig;
            bool shouldDump = (appfreezeCallback == nullptr || appfreezeCallback(getprocpid(), true, eventConfig));
            GetEcmaGCKeyStats()->SendSysEventBeforeDump("thresholdReachedDump",
                                                        GetHeapLimitSize(), GetLiveObjectSize(), eventConfig);
            if (shouldDump) {
                LOG_ECMA(INFO) << "ThresholdReachedDump and avoid freeze success.";
            } else {
                LOG_ECMA(WARN) << "ThresholdReachedDump but avoid freeze failed.";
                return;
            }
            DumpSnapShotOption dumpOption;
            dumpOption.dumpFormat = DumpFormat::BINARY;
            dumpOption.isVmMode = true;
            dumpOption.isPrivate = false;
            dumpOption.captureNumericValue = false;
            dumpOption.isFullGC = false;
            dumpOption.isSimplify = true;
            dumpOption.isSync = false;
            dumpOption.isBeforeFill = false;
            dumpOption.isDumpOOM = true; // aim's to do binary dump
            heapProfile->DumpHeapSnapshotForOOM(dumpOption);
            hasOOMDump_ = false;
            HeapProfilerInterface::Destroy(ecmaVm_);
        }
}
#endif

void Heap::RemoveGCListener(GCListenerId listenerId)
{
    gcListeners_.erase(listenerId);
}

void BaseHeap::IncreaseTaskCount()
{
    LockHolder holder(waitTaskFinishedMutex_);
    runningTaskCount_++;
}

void BaseHeap::WaitRunningTaskFinished()
{
    LockHolder holder(waitTaskFinishedMutex_);
    while (runningTaskCount_ > 0) {
        waitTaskFinishedCV_.Wait(&waitTaskFinishedMutex_);
    }
}

bool BaseHeap::CheckCanDistributeTask()
{
    LockHolder holder(waitTaskFinishedMutex_);
    return runningTaskCount_ < maxMarkTaskCount_;
}

void BaseHeap::ReduceTaskCount()
{
    LockHolder holder(waitTaskFinishedMutex_);
    runningTaskCount_--;
    if (runningTaskCount_ == 0) {
        waitTaskFinishedCV_.SignalAll();
    }
}

void BaseHeap::WaitClearTaskFinished()
{
    LockHolder holder(waitClearTaskFinishedMutex_);
    while (!clearTaskFinished_) {
        waitClearTaskFinishedCV_.Wait(&waitClearTaskFinishedMutex_);
    }
}
}  // namespace panda::ecmascript
