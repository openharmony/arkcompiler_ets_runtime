/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/heap-inl.h"

#include "ecmascript/ecma_vm.h"
#include "ecmascript/free_object.h"
#include "ecmascript/js_finalization_registry.h"
#include "ecmascript/js_native_pointer.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/assert_scope.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/mem/full_gc.h"
#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem_controller.h"
#include "ecmascript/mem/partial_gc.h"
#include "ecmascript/mem/native_area_allocator.h"
#include "ecmascript/mem/parallel_evacuator.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/stw_young_gc.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/mem/work_manager.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/runtime_call_id.h"
#if !WIN_OR_MAC_OR_IOS_PLATFORM
#include "ecmascript/dfx/hprof/heap_profiler_interface.h"
#include "ecmascript/dfx/hprof/heap_profiler.h"
#endif
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
#include "ecmascript/dfx/cpu_profiler/cpu_profiler.h"
#endif

namespace panda::ecmascript {
Heap::Heap(EcmaVM *ecmaVm) : ecmaVm_(ecmaVm), thread_(ecmaVm->GetJSThread()),
                             nativeAreaAllocator_(ecmaVm->GetNativeAreaAllocator()),
                             heapRegionAllocator_(ecmaVm->GetHeapRegionAllocator()) {}

void Heap::Initialize()
{
    memController_ = new MemController(this);
    auto &config = ecmaVm_->GetEcmaParamConfiguration();
    size_t maxHeapSize = config.GetMaxHeapSize();
    size_t minSemiSpaceCapacity = config.GetMinSemiSpaceSize();
    size_t maxSemiSpaceCapacity = config.GetMaxSemiSpaceSize();
    activeSemiSpace_ = new SemiSpace(this, minSemiSpaceCapacity, maxSemiSpaceCapacity);
    activeSemiSpace_->Restart();
    activeSemiSpace_->SetWaterLine();
    auto topAddress = activeSemiSpace_->GetAllocationTopAddress();
    auto endAddress = activeSemiSpace_->GetAllocationEndAddress();
    thread_->ReSetNewSpaceAllocationAddress(topAddress, endAddress);
    inactiveSemiSpace_ = new SemiSpace(this, minSemiSpaceCapacity, maxSemiSpaceCapacity);
    // not set up from space

    size_t readOnlySpaceCapacity = config.GetDefaultReadOnlySpaceSize();
    readOnlySpace_ = new ReadOnlySpace(this, readOnlySpaceCapacity, readOnlySpaceCapacity);
    appSpawnSpace_ = new AppSpawnSpace(this, maxHeapSize);
    size_t nonmovableSpaceCapacity = config.GetDefaultNonMovableSpaceSize();
    if (ecmaVm_->GetJSOptions().WasSetMaxNonmovableSpaceCapacity()) {
        nonmovableSpaceCapacity = ecmaVm_->GetJSOptions().MaxNonmovableSpaceCapacity();
    }
    nonMovableSpace_ = new NonMovableSpace(this, nonmovableSpaceCapacity, nonmovableSpaceCapacity);
    nonMovableSpace_->Initialize();
    size_t snapshotSpaceCapacity = config.GetDefaultSnapshotSpaceSize();
    snapshotSpace_ = new SnapshotSpace(this, snapshotSpaceCapacity, snapshotSpaceCapacity);
    size_t machineCodeSpaceCapacity = config.GetDefaultMachineCodeSpaceSize();
    machineCodeSpace_ = new MachineCodeSpace(this, machineCodeSpaceCapacity, machineCodeSpaceCapacity);

    size_t capacities = minSemiSpaceCapacity * 2 + nonmovableSpaceCapacity + snapshotSpaceCapacity +
        machineCodeSpaceCapacity + readOnlySpaceCapacity;
    if (maxHeapSize < capacities || maxHeapSize - capacities < MIN_OLD_SPACE_LIMIT) {
        LOG_ECMA_MEM(FATAL) << "HeapSize is too small to initialize oldspace, heapSize = " << maxHeapSize;
    }
    size_t oldSpaceCapacity = maxHeapSize - capacities;
    globalSpaceAllocLimit_ = maxHeapSize - minSemiSpaceCapacity;
    globalSpaceNativeLimit_ = globalSpaceAllocLimit_;
    oldSpace_ = new OldSpace(this, oldSpaceCapacity, oldSpaceCapacity);
    compressSpace_ = new OldSpace(this, oldSpaceCapacity, oldSpaceCapacity);
    oldSpace_->Initialize();

    hugeObjectSpace_ = new HugeObjectSpace(this, heapRegionAllocator_, oldSpaceCapacity, oldSpaceCapacity);
    maxEvacuateTaskCount_ = Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
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
    workManager_ = new WorkManager(this, Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() + 1);
    stwYoungGC_ = new STWYoungGC(this, parallelGC_);
    fullGC_ = new FullGC(this);

    partialGC_ = new PartialGC(this);
    sweeper_ = new ConcurrentSweeper(this, ecmaVm_->GetJSOptions().EnableConcurrentSweep() ?
        EnableConcurrentSweepType::ENABLE : EnableConcurrentSweepType::CONFIG_DISABLE);
    concurrentMarker_ = new ConcurrentMarker(this, concurrentMarkerEnabled ? EnableConcurrentMarkType::ENABLE :
        EnableConcurrentMarkType::CONFIG_DISABLE);
    nonMovableMarker_ = new NonMovableMarker(this);
    semiGCMarker_ = new SemiGCMarker(this);
    compressGCMarker_ = new CompressGCMarker(this);
    evacuator_ = new ParallelEvacuator(this);
    incrementalMarker_ = new IncrementalMarker(this);
}

void Heap::Destroy()
{
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
    if (stwYoungGC_ != nullptr) {
        delete stwYoungGC_;
        stwYoungGC_ = nullptr;
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
    if (semiGCMarker_ != nullptr) {
        delete semiGCMarker_;
        semiGCMarker_ = nullptr;
    }
    if (compressGCMarker_ != nullptr) {
        delete compressGCMarker_;
        compressGCMarker_ = nullptr;
    }
    if (evacuator_ != nullptr) {
        delete evacuator_;
        evacuator_ = nullptr;
    }
}

void Heap::Prepare()
{
    MEM_ALLOCATE_AND_GC_TRACE(GetEcmaVM(), HeapPrepare);
    WaitRunningTaskFinished();
    sweeper_->EnsureAllTaskFinished();
    WaitClearTaskFinished();
}

void Heap::Resume(TriggerGCType gcType)
{
    if (mode_ != HeapMode::SPAWN &&
        activeSemiSpace_->AdjustCapacity(inactiveSemiSpace_->GetAllocatedSizeSinceGC())) {
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

    activeSemiSpace_->SetWaterLine();
    PrepareRecordRegionsForReclaim();
    hugeObjectSpace_->ReclaimHugeRegion();
    if (parallelGC_) {
        clearTaskFinished_ = false;
        Taskpool::GetCurrentTaskpool()->PostTask(
            std::make_unique<AsyncClearTask>(GetJSThread()->GetThreadId(), this, gcType));
    } else {
        ReclaimRegions(gcType);
    }
}

void Heap::ResumeForAppSpawn()
{
    sweeper_->WaitAllTaskFinished();
    hugeObjectSpace_->ReclaimHugeRegion();
    inactiveSemiSpace_->ReclaimRegions();
    oldSpace_->Reset();
    auto cb = [] (Region *region) {
        region->ClearMarkGCBitset();
    };
    nonMovableSpace_->EnumerateRegions(cb);
    machineCodeSpace_->EnumerateRegions(cb);
    hugeObjectSpace_->EnumerateRegions(cb);
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
    stwYoungGC_->ConfigParallelGC(false);
    sweeper_->ConfigConcurrentSweep(false);
    concurrentMarker_->ConfigConcurrentMark(false);
    Taskpool::GetCurrentTaskpool()->Destroy(GetJSThread()->GetThreadId());
}

void Heap::EnableParallelGC()
{
    Taskpool::GetCurrentTaskpool()->Initialize();
    parallelGC_ = ecmaVm_->GetJSOptions().EnableParallelGC();
    maxEvacuateTaskCount_ = Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    if (auto totalThreadNum = workManager_->GetTotalThreadNum();
        totalThreadNum != maxEvacuateTaskCount_ + 1) {
        LOG_ECMA_MEM(WARN) << "TheadNum mismatch, totalThreadNum(workerManager): " << totalThreadNum << ", "
                           << "totalThreadNum(taskpool): " << (maxEvacuateTaskCount_ + 1);
        delete workManager_;
        workManager_ = new WorkManager(this, maxEvacuateTaskCount_ + 1);
    }
    maxMarkTaskCount_ = std::min<size_t>(ecmaVm_->GetJSOptions().GetGcThreadNum(),
                                         maxEvacuateTaskCount_ - 1);
    bool concurrentMarkerEnabled = ecmaVm_->GetJSOptions().EnableConcurrentMark();
#if ECMASCRIPT_DISABLE_CONCURRENT_MARKING
    concurrentMarkerEnabled = false;
#endif
    stwYoungGC_->ConfigParallelGC(parallelGC_);
    sweeper_->ConfigConcurrentSweep(ecmaVm_->GetJSOptions().EnableConcurrentSweep());
    concurrentMarker_->ConfigConcurrentMark(concurrentMarkerEnabled);
}

TriggerGCType Heap::SelectGCType() const
{
    // If concurrent mark is enabled, the TryTriggerConcurrentMarking decide which GC to choose.
    if (concurrentMarker_->IsEnabled() && !thread_->IsReadyToMark()) {
        return YOUNG_GC;
    }
    if (!OldSpaceExceedLimit() && !OldSpaceExceedCapacity(activeSemiSpace_->GetCommittedSize()) &&
        GetHeapObjectSize() <= globalSpaceAllocLimit_  + oldSpace_->GetOvershootSize() &&
        !GlobalNativeSizeLargerThanLimit()) {
        return YOUNG_GC;
    }
    return OLD_GC;
}

void Heap::CollectGarbage(TriggerGCType gcType, GCReason reason)
{
    if (thread_->IsCrossThreadExecutionEnable()) {
        return;
    }
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    [[maybe_unused]] GcStateScope scope(thread_);
#endif
    CHECK_NO_GC

#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    LOG_ECMA(DEBUG) << "Enable heap verify";
    isVerifying_ = true;
    // pre gc heap verify
    sweeper_->EnsureAllTaskFinished();
    auto failCount = Verification(this).VerifyAll();
    if (failCount > 0) {
        LOG_GC(FATAL) << "Before gc heap corrupted and " << failCount << " corruptions";
    }
    isVerifying_ = false;
#endif

#if ECMASCRIPT_SWITCH_GC_MODE_TO_FULL_GC
    gcType = TriggerGCType::FULL_GC;
#endif
    if (fullGCRequested_ && thread_->IsReadyToMark() && gcType != TriggerGCType::FULL_GC) {
        gcType = TriggerGCType::FULL_GC;
    }
    if (oldGCRequested_ && gcType != TriggerGCType::FULL_GC) {
        gcType = TriggerGCType::OLD_GC;
    }
    oldGCRequested_ = false;
    oldSpace_->AdjustOvershootSize();

    size_t originalNewSpaceSize = activeSemiSpace_->GetHeapObjectSize();
    memController_->StartCalculationBeforeGC();
    StatisticHeapObject(gcType);
    if (!GetJSThread()->IsReadyToMark() && markType_ == MarkType::MARK_FULL) {
        ecmaVm_->GetEcmaGCStats()->SetGCReason(reason);
    } else {
        ecmaVm_->GetEcmaGCStats()->RecordStatisticBeforeGC(gcType, reason);
    }
    gcType_ = gcType;
    switch (gcType) {
        case TriggerGCType::YOUNG_GC:
            // Use partial GC for young generation.
            if (!concurrentMarker_->IsEnabled() && !incrementalMarker_->IsTriggeredIncrementalMark()) {
                SetMarkType(MarkType::MARK_YOUNG);
            }
            if (concurrentMarker_->IsEnabled() && markType_ == MarkType::MARK_FULL) {
                // gcType_ must be sure. Functions ProcessNativeReferences need to use it.
                gcType_ = TriggerGCType::OLD_GC;
            }
            partialGC_->RunPhases();
            break;
        case TriggerGCType::OLD_GC: {
            bool fullConcurrentMarkRequested = false;
            // Check whether it's needed to trigger full concurrent mark instead of trigger old gc
            if (concurrentMarker_->IsEnabled() && (thread_->IsReadyToMark() || markType_ == MarkType::MARK_YOUNG) &&
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
            if (fullConcurrentMarkRequested) {
                LOG_ECMA(INFO) << "Trigger old gc here may cost long time, trigger full concurrent mark instead";
                oldSpace_->SetOvershootSize(GetEcmaVM()->GetEcmaParamConfiguration().GetOldSpaceOvershootSize());
                TriggerConcurrentMarking();
                oldGCRequested_ = true;
                return;
            }
            partialGC_->RunPhases();
            CheckNonMovableSpaceOOM();
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
        default:
            LOG_ECMA(FATAL) << "this branch is unreachable";
            UNREACHABLE();
            break;
    }

    // OOMError object is not allowed to be allocated during gc process, so throw OOMError after gc
    if (shouldThrowOOMError_) {
        ThrowOutOfMemoryError(oldSpace_->GetMergeSize(), " OldSpace::Merge");
        oldSpace_->ResetMergeSize();
        shouldThrowOOMError_ = false;
    }

    ClearIdleTask();
    // Adjust the old space capacity and global limit for the first partial GC with full mark.
    // Trigger the full mark next time if the current survival rate is much less than half the average survival rates.
    AdjustBySurvivalRate(originalNewSpaceSize);
    memController_->StopCalculationAfterGC(gcType);
    if (gcType == TriggerGCType::FULL_GC || IsFullMark()) {
        // Only when the gc type is not semiGC and after the old space sweeping has been finished,
        // the limits of old space and global space can be recomputed.
        RecomputeLimits();
        OPTIONAL_LOG(ecmaVm_, INFO) << " GC after: is full mark" << IsFullMark()
                                     << " global object size " << GetHeapObjectSize()
                                     << " global committed size " << GetCommittedSize()
                                     << " global limit " << globalSpaceAllocLimit_;
        markType_ = MarkType::MARK_YOUNG;
    }
    if (concurrentMarker_->IsRequestDisabled()) {
        concurrentMarker_->EnableConcurrentMarking(EnableConcurrentMarkType::DISABLE);
    }
    // GC log
    ecmaVm_->GetEcmaGCStats()->RecordStatisticAfterGC();
    ecmaVm_->GetEcmaGCStats()->PrintGCStatistic();
    // weak node nativeFinalizeCallback may execute JS and change the weakNodeList status,
    // even lead to another GC, so this have to invoke after this GC process.
    InvokeWeakNodeNativeFinalizeCallback();

#if ECMASCRIPT_ENABLE_HEAP_VERIFY
    // post gc heap verify
    isVerifying_ = true;
    sweeper_->EnsureAllTaskFinished();
    failCount = Verification(this).VerifyAll();
    if (failCount > 0) {
        LOG_GC(FATAL) << "After gc heap corrupted and " << failCount << " corruptions";
    }
    isVerifying_ = false;
#endif
    JSFinalizationRegistry::CheckAndCall(thread_);
}

void Heap::ThrowOutOfMemoryError(size_t size, std::string functionName, bool NonMovableObjNearOOM)
{
    GetEcmaVM()->GetEcmaGCStats()->PrintGCMemoryStatistic();
    std::ostringstream oss;
    if (NonMovableObjNearOOM) {
        oss << "OutOfMemory when nonmovable live obj size: " << size << " bytes"
            << " function name: " << functionName.c_str();
    } else {
        oss << "OutOfMemory when trying to allocate " << size << " bytes" << " function name: " << functionName.c_str();
    }
    LOG_ECMA_MEM(ERROR) << oss.str().c_str();
    THROW_OOM_ERROR(thread_, oss.str().c_str());
}

void Heap::FatalOutOfMemoryError(size_t size, std::string functionName)
{
    GetEcmaVM()->GetEcmaGCStats()->PrintGCMemoryStatistic();
    LOG_ECMA_MEM(FATAL) << "OOM fatal when trying to allocate " << size << " bytes"
                        << " function name: " << functionName.c_str();
}

void Heap::AdjustBySurvivalRate(size_t originalNewSpaceSize)
{
    if (originalNewSpaceSize <= 0) {
        return;
    }
    semiSpaceCopiedSize_ = activeSemiSpace_->GetHeapObjectSize();
    double copiedRate = semiSpaceCopiedSize_ * 1.0 / originalNewSpaceSize;
    promotedSize_ = GetEvacuator()->GetPromotedSize();
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
            fullMarkRequested_ = true;
            OPTIONAL_LOG(ecmaVm_, INFO) << " Current survival rate: " << survivalRate
                << " is less than half the average survival rates: " << averageSurvivalRate
                << ". Trigger full mark next time.";
            // Survival rate of full mark is precise. Reset recorded survival rates.
            memController_->ResetRecordedSurvivalRates();
        }
        memController_->AddSurvivalRate(survivalRate);
    }
}

size_t Heap::VerifyHeapObjects() const
{
    size_t failCount = 0;
    {
        VerifyObjectVisitor verifier(this, &failCount);
        activeSemiSpace_->IterateOverObjects(verifier);
    }

    {
        VerifyObjectVisitor verifier(this, &failCount);
        oldSpace_->IterateOverObjects(verifier);
    }

    {
        VerifyObjectVisitor verifier(this, &failCount);
        appSpawnSpace_->IterateOverMarkedObjects(verifier);
    }

    {
        VerifyObjectVisitor verifier(this, &failCount);
        nonMovableSpace_->IterateOverObjects(verifier);
    }

    {
        VerifyObjectVisitor verifier(this, &failCount);
        hugeObjectSpace_->IterateOverObjects(verifier);
    }
    {
        VerifyObjectVisitor verifier(this, &failCount);
        machineCodeSpace_->IterateOverObjects(verifier);
    }
    {
        VerifyObjectVisitor verifier(this, &failCount);
        snapshotSpace_->IterateOverObjects(verifier);
    }
    return failCount;
}

size_t Heap::VerifyOldToNewRSet() const
{
    size_t failCount = 0;
    VerifyObjectVisitor verifier(this, &failCount);
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
    // temporarily regard the heap limit is the same as the native limit.
    globalSpaceNativeLimit_ = globalSpaceAllocLimit_;
    OPTIONAL_LOG(ecmaVm_, INFO) << "AdjustOldSpaceLimit oldSpaceAllocLimit_: " << oldSpaceAllocLimit
        << " globalSpaceAllocLimit_: " << globalSpaceAllocLimit_;
}

void Heap::OnAllocateEvent([[maybe_unused]] TaggedObject* address, [[maybe_unused]] size_t size)
{
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    HeapProfilerInterface *profiler = GetEcmaVM()->GetHeapProfile();
    if (profiler != nullptr) {
        BlockHookScope blockScope;
        profiler->AllocationEvent(address, size);
    }
#endif
}

void Heap::OnMoveEvent([[maybe_unused]] uintptr_t address, [[maybe_unused]] TaggedObject* forwardAddress,
                       [[maybe_unused]] size_t size)
{
#if defined(ECMASCRIPT_SUPPORT_HEAPPROFILER)
    HeapProfilerInterface *profiler = GetEcmaVM()->GetHeapProfile();
    if (profiler != nullptr) {
        BlockHookScope blockScope;
        profiler->MoveEvent(address, forwardAddress, size);
    }
#endif
}

void Heap::AddToKeptObjects(JSHandle<JSTaggedValue> value) const
{
    JSHandle<GlobalEnv> env = ecmaVm_->GetGlobalEnv();
    JSHandle<LinkedHashSet> linkedSet;
    if (env->GetWeakRefKeepObjects()->IsUndefined()) {
        linkedSet = LinkedHashSet::Create(thread_);
    } else {
        linkedSet =
            JSHandle<LinkedHashSet>(thread_, LinkedHashSet::Cast(env->GetWeakRefKeepObjects()->GetTaggedObject()));
    }
    linkedSet = LinkedHashSet::Add(thread_, linkedSet, value);
    env->SetWeakRefKeepObjects(thread_, linkedSet);
}

void Heap::AdjustSpaceSizeForAppSpawn()
{
    SetHeapMode(HeapMode::SPAWN);
    auto &config = ecmaVm_->GetEcmaParamConfiguration();
    size_t minSemiSpaceCapacity = config.GetMinSemiSpaceSize();
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
}

void Heap::ClearAllocationInspectorFromAllSpaces()
{
    activeSemiSpace_->ClearAllocationInspector();
    oldSpace_->ClearAllocationInspector();
    nonMovableSpace_->ClearAllocationInspector();
    machineCodeSpace_->ClearAllocationInspector();
    hugeObjectSpace_->ClearAllocationInspector();
}

void Heap::ClearKeptObjects() const
{
    ecmaVm_->GetGlobalEnv()->SetWeakRefKeepObjects(thread_, JSTaggedValue::Undefined());
}

void Heap::RecomputeLimits()
{
    double gcSpeed = memController_->CalculateMarkCompactSpeedPerMS();
    double mutatorSpeed = memController_->GetCurrentOldSpaceAllocationThroughputPerMS();
    size_t oldSpaceSize = oldSpace_->GetHeapObjectSize() + hugeObjectSpace_->GetHeapObjectSize();
    size_t newSpaceCapacity = activeSemiSpace_->GetInitialCapacity();

    double growingFactor = memController_->CalculateGrowingFactor(gcSpeed, mutatorSpeed);
    size_t maxOldSpaceCapacity = oldSpace_->GetMaximumCapacity() - newSpaceCapacity;
    size_t newOldSpaceLimit = memController_->CalculateAllocLimit(oldSpaceSize, MIN_OLD_SPACE_LIMIT,
        maxOldSpaceCapacity, newSpaceCapacity, growingFactor);
    size_t maxGlobalSize = ecmaVm_->GetEcmaParamConfiguration().GetMaxHeapSize() - newSpaceCapacity;
    size_t newGlobalSpaceLimit = memController_->CalculateAllocLimit(GetHeapObjectSize(), MIN_HEAP_SIZE,
                                                                     maxGlobalSize, newSpaceCapacity, growingFactor);
    globalSpaceAllocLimit_ = newGlobalSpaceLimit;
    oldSpace_->SetInitialCapacity(newOldSpaceLimit);
    globalSpaceNativeLimit_ = memController_->CalculateAllocLimit(GetGlobalNativeSize(), MIN_HEAP_SIZE,
                                                                  maxGlobalSize, newSpaceCapacity, growingFactor);
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
    bool isFullMarking = IsFullMark() && GetJSThread()->IsMarking();
    bool isNativeSizeLargeTrigger = isFullMarking ? false : GlobalNativeSizeLargerThanLimit();
    if (isFullMarking && oldSpace_->GetOvershootSize() == 0) {
        oldSpace_->SetOvershootSize(GetEcmaVM()->GetEcmaParamConfiguration().GetOldSpaceOvershootSize());
    }
    if (isNativeSizeLargeTrigger || OldSpaceExceedLimit() || OldSpaceExceedCapacity(size) ||
        GetHeapObjectSize() > globalSpaceAllocLimit_ + oldSpace_->GetOvershootSize()) {
        CollectGarbage(TriggerGCType::OLD_GC, GCReason::ALLOCATION_LIMIT);
        if (!oldGCRequested_) {
            return true;
        }
    }
    return false;
}

bool Heap::CheckOngoingConcurrentMarking()
{
    if (concurrentMarker_->IsEnabled() && !thread_->IsReadyToMark() &&
        concurrentMarker_->IsTriggeredConcurrentMark()) {
        TRACE_GC(GCStats::Scope::ScopeId::WaitConcurrentMarkFinished, GetEcmaVM()->GetEcmaGCStats());
        if (thread_->IsMarking()) {
            ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "Heap::CheckOngoingConcurrentMarking");
            MEM_ALLOCATE_AND_GC_TRACE(GetEcmaVM(), WaitConcurrentMarkingFinished);
            GetNonMovableMarker()->ProcessMarkStack(MAIN_THREAD_INDEX);
            WaitConcurrentMarkingFinished();
        } else {
            WaitRunningTaskFinished();
        }
        memController_->RecordAfterConcurrentMark(IsFullMark(), concurrentMarker_);
        return true;
    }
    return false;
}

void Heap::ClearIdleTask()
{
    SetIdleTask(IdleTaskType::NO_TASK);
    idleTaskFinishTime_ = incrementalMarker_->GetCurrentTimeInMs();
}

void Heap::TryTriggerIdleCollection()
{
    if (idleTask_ != IdleTaskType::NO_TASK || !GetJSThread()->IsReadyToMark() || !enableIdleGC_) {
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
    double newSpaceAllocToLimitDuration =
        (activeSemiSpace_->GetInitialCapacity() - activeSemiSpace_->GetCommittedSize()) / newSpaceAllocSpeed;
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
    // update reference duration
    idlePredictDuration_ = 0.0f;
    size_t updateReferenceSpeed = markType_ == MarkType::MARK_YOUNG ?
        ecmaVm_->GetEcmaGCStats()->GetGCSpeed(SpeedData::YOUNG_UPDATE_REFERENCE_SPEED) :
        ecmaVm_->GetEcmaGCStats()->GetGCSpeed(SpeedData::UPDATE_REFERENCE_SPEED);
    if (updateReferenceSpeed != 0) {
        idlePredictDuration_ += (float)GetHeapObjectSize() / updateReferenceSpeed;
    }

    // clear native object duration
    size_t clearNativeObjSpeed = 0;
    if (markType_ == MarkType::MARK_YOUNG) {
        clearNativeObjSpeed = ecmaVm_->GetEcmaGCStats()->GetGCSpeed(SpeedData::YOUNG_CLEAR_NATIVE_OBJ_SPEED);
    } else if (markType_ == MarkType::MARK_FULL) {
        clearNativeObjSpeed = ecmaVm_->GetEcmaGCStats()->GetGCSpeed(SpeedData::OLD_CLEAR_NATIVE_OBJ_SPEED);
    }

    if (clearNativeObjSpeed != 0) {
        idlePredictDuration_ += (float)GetEcmaVM()->GetNativePointerListSize() / clearNativeObjSpeed;
    }

    // sweep and evacuate duration
    size_t youngEvacuateSpeed = ecmaVm_->GetEcmaGCStats()->GetGCSpeed(SpeedData::YOUNG_EVACUATE_SPACE_SPEED);
    size_t sweepSpeed = ecmaVm_->GetEcmaGCStats()->GetGCSpeed(SpeedData::SWEEP_SPEED);
    size_t oldEvacuateSpeed = ecmaVm_->GetEcmaGCStats()->GetGCSpeed(SpeedData::OLD_EVACUATE_SPACE_SPEED);
    double survivalRate = ecmaVm_->GetEcmaGCStats()->GetAvgSurvivalRate();
    if (markType_ == MarkType::MARK_YOUNG && youngEvacuateSpeed != 0) {
        idlePredictDuration_ += survivalRate * activeSemiSpace_->GetHeapObjectSize() / youngEvacuateSpeed;
    } else if (markType_ == MarkType::MARK_FULL) {
        if (sweepSpeed != 0) {
            idlePredictDuration_ += (float)GetHeapObjectSize() / sweepSpeed;
        }
        if (oldEvacuateSpeed != 0) {
            size_t collectRegionSetSize = GetEcmaVM()->GetEcmaGCStats()->GetRecordData(
                RecordData::COLLECT_REGION_SET_SIZE);
            idlePredictDuration_ += (survivalRate * activeSemiSpace_->GetHeapObjectSize() + collectRegionSetSize) /
                                    oldEvacuateSpeed;
        }
    }

    // Idle YoungGC mark duration
    size_t markSpeed = ecmaVm_->GetEcmaGCStats()->GetGCSpeed(SpeedData::MARK_SPEED);
    if (idleTask_ == IdleTaskType::YOUNG_GC && markSpeed != 0) {
        idlePredictDuration_ += (float)activeSemiSpace_->GetHeapObjectSize() / markSpeed;
    }
    OPTIONAL_LOG(ecmaVm_, INFO) << "Predict idle gc pause: " << idlePredictDuration_ << "ms";
}

void Heap::TryTriggerIncrementalMarking()
{
    if (!GetJSThread()->IsReadyToMark() || idleTask_ != IdleTaskType::NO_TASK || !enableIdleGC_) {
        return;
    }
    size_t oldSpaceAllocLimit = oldSpace_->GetInitialCapacity();
    size_t oldSpaceHeapObjectSize = oldSpace_->GetHeapObjectSize() + hugeObjectSpace_->GetHeapObjectSize();
    double oldSpaceAllocSpeed = memController_->GetOldSpaceAllocationThroughputPerMS();
    double oldSpaceIncrementalMarkSpeed = incrementalMarker_->GetAverageIncrementalMarkingSpeed();
    double oldSpaceAllocToLimitDuration = (oldSpaceAllocLimit - oldSpaceHeapObjectSize) / oldSpaceAllocSpeed;
    double oldSpaceMarkDuration = GetHeapObjectSize() / oldSpaceIncrementalMarkSpeed;

    double oldSpaceRemainSize = (oldSpaceAllocToLimitDuration - oldSpaceMarkDuration) * oldSpaceAllocSpeed;
    // mark finished before allocate limit
    if ((oldSpaceRemainSize > 0 && oldSpaceRemainSize < DEFAULT_REGION_SIZE) ||
        GetHeapObjectSize() >= globalSpaceAllocLimit_) {
        // The object allocated in incremental marking should lower than limit,
        // otherwise select trigger concurrent mark.
        size_t allocateSize = oldSpaceAllocSpeed * oldSpaceMarkDuration;
        if (allocateSize < ALLOCATE_SIZE_LIMIT) {
            EnableNotifyIdle();
            SetIdleTask(IdleTaskType::INCREMENTAL_MARK);
        }
    }
}

void Heap::TryTriggerConcurrentMarking()
{
    // When concurrent marking is enabled, concurrent marking will be attempted to trigger.
    // When the size of old space or global space reaches the limit, isFullMarkNeeded will be set to true.
    // If the predicted duration of current full mark may not result in the new and old spaces reaching their limit,
    // full mark will be triggered.
    // In the same way, if the size of the new space reaches the capacity, and the predicted duration of current
    // young mark may not result in the new space reaching its limit, young mark can be triggered.
    // If it spends much time in full mark, the compress full GC will be requested when the spaces reach the limit.
    // If the global space is larger than half max heap size, we will turn to use full mark and trigger partial GC.
    if (!concurrentMarker_->IsEnabled() || !thread_->IsReadyToMark() ||
        incrementalMarker_->IsTriggeredIncrementalMark() ||
        !(idleTask_ == IdleTaskType::NO_TASK || idleTask_ == IdleTaskType::YOUNG_GC)) {
        return;
    }
    if (fullMarkRequested_) {
        markType_ = MarkType::MARK_FULL;
        OPTIONAL_LOG(ecmaVm_, INFO) << " fullMarkRequested, trigger full mark.";
        TriggerConcurrentMarking();
        return;
    }
    double oldSpaceMarkDuration = 0, newSpaceMarkDuration = 0, newSpaceRemainSize = 0, newSpaceAllocToLimitDuration = 0,
           oldSpaceAllocToLimitDuration = 0;
    double oldSpaceAllocSpeed = memController_->GetOldSpaceAllocationThroughputPerMS();
    double oldSpaceConcurrentMarkSpeed = memController_->GetFullSpaceConcurrentMarkSpeedPerMS();
    size_t oldSpaceHeapObjectSize = oldSpace_->GetHeapObjectSize() + hugeObjectSpace_->GetHeapObjectSize();
    size_t globalHeapObjectSize = GetHeapObjectSize();
    size_t oldSpaceAllocLimit = oldSpace_->GetInitialCapacity();
    if (oldSpaceConcurrentMarkSpeed == 0 || oldSpaceAllocSpeed == 0) {
        if (oldSpaceHeapObjectSize >= oldSpaceAllocLimit || globalHeapObjectSize >= globalSpaceAllocLimit_ ||
            GlobalNativeSizeLargerThanLimit()) {
            markType_ = MarkType::MARK_FULL;
            OPTIONAL_LOG(ecmaVm_, INFO) << "Trigger the first full mark";
            TriggerConcurrentMarking();
            return;
        }
    } else {
        if (oldSpaceHeapObjectSize >= oldSpaceAllocLimit || globalHeapObjectSize >= globalSpaceAllocLimit_ ||
            GlobalNativeSizeLargerThanLimit()) {
            markType_ = MarkType::MARK_FULL;
            TriggerConcurrentMarking();
            OPTIONAL_LOG(ecmaVm_, INFO) << "Trigger full mark";
            return;
        }
        oldSpaceAllocToLimitDuration = (oldSpaceAllocLimit - oldSpaceHeapObjectSize) / oldSpaceAllocSpeed;
        oldSpaceMarkDuration = GetHeapObjectSize() / oldSpaceConcurrentMarkSpeed;
        // oldSpaceRemainSize means the predicted size which can be allocated after the full concurrent mark.
        double oldSpaceRemainSize = (oldSpaceAllocToLimitDuration - oldSpaceMarkDuration) * oldSpaceAllocSpeed;
        if (oldSpaceRemainSize > 0 && oldSpaceRemainSize < DEFAULT_REGION_SIZE) {
            markType_ = MarkType::MARK_FULL;
            TriggerConcurrentMarking();
            OPTIONAL_LOG(ecmaVm_, INFO) << "Trigger full mark";
            return;
        }
    }

    double newSpaceAllocSpeed = memController_->GetNewSpaceAllocationThroughputPerMS();
    double newSpaceConcurrentMarkSpeed = memController_->GetNewSpaceConcurrentMarkSpeedPerMS();
    if (newSpaceConcurrentMarkSpeed == 0 || newSpaceAllocSpeed == 0) {
        auto &config = ecmaVm_->GetEcmaParamConfiguration();
        if (activeSemiSpace_->GetCommittedSize() >= config.GetSemiSpaceTriggerConcurrentMark()) {
            markType_ = MarkType::MARK_YOUNG;
            TriggerConcurrentMarking();
            OPTIONAL_LOG(ecmaVm_, INFO) << "Trigger the first semi mark" << fullGCRequested_;
        }
        return;
    }
    newSpaceAllocToLimitDuration = (activeSemiSpace_->GetInitialCapacity() - activeSemiSpace_->GetCommittedSize()) /
        newSpaceAllocSpeed;
    newSpaceMarkDuration = activeSemiSpace_->GetHeapObjectSize() / newSpaceConcurrentMarkSpeed;
    // newSpaceRemainSize means the predicted size which can be allocated after the semi concurrent mark.
    newSpaceRemainSize = (newSpaceAllocToLimitDuration - newSpaceMarkDuration) * newSpaceAllocSpeed;

    if (newSpaceRemainSize < DEFAULT_REGION_SIZE) {
        markType_ = MarkType::MARK_YOUNG;
        TriggerConcurrentMarking();
        OPTIONAL_LOG(ecmaVm_, INFO) << "Trigger semi mark";
    }
}

void Heap::IncreaseNativeBindingSize(JSNativePointer *object)
{
    size_t size = object->GetBindingSize();
    if (size == 0) {
        return;
    }
    nativeBindingSize_ += size;
}

void Heap::IncreaseNativeBindingSize(size_t size)
{
    if (size == 0) {
        return;
    }
    nativeBindingSize_ += size;
}

void Heap::PrepareRecordRegionsForReclaim()
{
    activeSemiSpace_->SetRecordRegion();
    oldSpace_->SetRecordRegion();
    snapshotSpace_->SetRecordRegion();
    nonMovableSpace_->SetRecordRegion();
    hugeObjectSpace_->SetRecordRegion();
    machineCodeSpace_->SetRecordRegion();
}

void Heap::TriggerConcurrentMarking()
{
    if (idleTask_ == IdleTaskType::YOUNG_GC && IsFullMark()) {
        ClearIdleTask();
        DisableNotifyIdle();
    }
    if (concurrentMarker_->IsEnabled() && !fullGCRequested_ && ConcurrentMarker::TryIncreaseTaskCounts()) {
        concurrentMarker_->Mark();
    }
}

void Heap::WaitRunningTaskFinished()
{
    os::memory::LockHolder holder(waitTaskFinishedMutex_);
    while (runningTaskCount_ > 0) {
        waitTaskFinishedCV_.Wait(&waitTaskFinishedMutex_);
    }
}

void Heap::WaitClearTaskFinished()
{
    os::memory::LockHolder holder(waitClearTaskFinishedMutex_);
    while (!clearTaskFinished_) {
        waitClearTaskFinishedCV_.Wait(&waitClearTaskFinishedMutex_);
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
    Taskpool::GetCurrentTaskpool()->PostTask(
        std::make_unique<ParallelGCTask>(GetJSThread()->GetThreadId(), this, gcTask));
}

void Heap::IncreaseTaskCount()
{
    os::memory::LockHolder holder(waitTaskFinishedMutex_);
    runningTaskCount_++;
}

void Heap::ChangeGCParams(bool inBackground)
{
    inBackground_ = inBackground;
    if (inBackground) {
        LOG_GC(INFO) << "app is inBackground";
        if (GetHeapObjectSize() - heapAliveSizeAfterGC_ > BACKGROUND_GROW_LIMIT) {
            CollectGarbage(TriggerGCType::FULL_GC, GCReason::SWITCH_BACKGROUND);
        }
        if (GetMemGrowingType() != MemGrowingType::PRESSURE) {
            SetMemGrowingType(MemGrowingType::CONSERVATIVE);
            LOG_GC(INFO) << "Heap Growing Type CONSERVATIVE";
        }
        concurrentMarker_->EnableConcurrentMarking(EnableConcurrentMarkType::DISABLE);
        sweeper_->EnableConcurrentSweep(EnableConcurrentSweepType::DISABLE);
        maxMarkTaskCount_ = 1;
        maxEvacuateTaskCount_ = 1;
    } else {
        LOG_GC(INFO) << "app is not inBackground";
        if (GetMemGrowingType() != MemGrowingType::PRESSURE) {
            SetMemGrowingType(MemGrowingType::HIGH_THROUGHPUT);
            LOG_GC(INFO) << "Heap Growing Type HIGH_THROUGHPUT";
        }
        concurrentMarker_->EnableConcurrentMarking(EnableConcurrentMarkType::ENABLE);
        sweeper_->EnableConcurrentSweep(EnableConcurrentSweepType::ENABLE);
        maxMarkTaskCount_ = std::min<size_t>(ecmaVm_->GetJSOptions().GetGcThreadNum(),
            Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() - 1);
        maxEvacuateTaskCount_ = Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    }
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
        default:
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

bool Heap::CheckCanDistributeTask()
{
    os::memory::LockHolder holder(waitTaskFinishedMutex_);
    return runningTaskCount_ < maxMarkTaskCount_;
}

void Heap::ReduceTaskCount()
{
    os::memory::LockHolder holder(waitTaskFinishedMutex_);
    runningTaskCount_--;
    if (runningTaskCount_ == 0) {
        waitTaskFinishedCV_.SignalAll();
    }
}

bool Heap::ParallelGCTask::Run(uint32_t threadIndex)
{
    // Synchronizes-with. Ensure that WorkManager::Initialize must be seen by MarkerThreads.
    while (!heap_->GetWorkManager()->HasInitialized());
    switch (taskPhase_) {
        case ParallelGCTaskPhase::SEMI_HANDLE_THREAD_ROOTS_TASK:
            heap_->GetSemiGCMarker()->MarkRoots(threadIndex);
            heap_->GetSemiGCMarker()->ProcessMarkStack(threadIndex);
            break;
        case ParallelGCTaskPhase::SEMI_HANDLE_SNAPSHOT_TASK:
            heap_->GetSemiGCMarker()->ProcessSnapshotRSet(threadIndex);
            break;
        case ParallelGCTaskPhase::SEMI_HANDLE_GLOBAL_POOL_TASK:
            heap_->GetSemiGCMarker()->ProcessMarkStack(threadIndex);
            break;
        case ParallelGCTaskPhase::OLD_HANDLE_GLOBAL_POOL_TASK:
            heap_->GetNonMovableMarker()->ProcessMarkStack(threadIndex);
            break;
        case ParallelGCTaskPhase::COMPRESS_HANDLE_GLOBAL_POOL_TASK:
            heap_->GetCompressGCMarker()->ProcessMarkStack(threadIndex);
            break;
        case ParallelGCTaskPhase::CONCURRENT_HANDLE_GLOBAL_POOL_TASK:
            heap_->GetNonMovableMarker()->ProcessMarkStack(threadIndex);
            break;
        case ParallelGCTaskPhase::CONCURRENT_HANDLE_OLD_TO_NEW_TASK:
            heap_->GetNonMovableMarker()->ProcessOldToNew(threadIndex);
            break;
        default:
            break;
    }
    heap_->ReduceTaskCount();
    return true;
}

bool Heap::AsyncClearTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    heap_->ReclaimRegions(gcType_);
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

bool Heap::IsAlive(TaggedObject *object) const
{
    if (!ContainObject(object)) {
        LOG_GC(ERROR) << "The region is already free";
        return false;
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

bool Heap::ContainObject(TaggedObject *object) const
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

void Heap::InvokeWeakNodeNativeFinalizeCallback()
{
    // the second callback may lead to another GC, if this, return directly;
    if (runningNativeFinalizeCallbacks_) {
        return;
    }
    runningNativeFinalizeCallbacks_ = true;
    auto weakNodeNativeFinalizeCallBacks = thread_->GetWeakNodeNativeFinalizeCallbacks();
    while (!weakNodeNativeFinalizeCallBacks->empty()) {
        auto callbackPair = weakNodeNativeFinalizeCallBacks->back();
        weakNodeNativeFinalizeCallBacks->pop_back();
        ASSERT(callbackPair.first != nullptr && callbackPair.second != nullptr);
        auto callback = callbackPair.first;
        (*callback)(callbackPair.second);
    }
    runningNativeFinalizeCallbacks_ = false;
}

void Heap::PrintHeapInfo(TriggerGCType gcType) const
{
    OPTIONAL_LOG(ecmaVm_, INFO) << "-----------------------Statistic Heap Object------------------------";
    OPTIONAL_LOG(ecmaVm_, INFO) << "Heap::CollectGarbage, gcType(" << gcType << "), Concurrent Mark("
                                << concurrentMarker_->IsEnabled() << "), Full Mark(" << IsFullMark() << ")";
    OPTIONAL_LOG(ecmaVm_, INFO) << "ActiveSemi(" << activeSemiSpace_->GetHeapObjectSize()
                   << "/" << activeSemiSpace_->GetInitialCapacity() << "), NonMovable("
                   << nonMovableSpace_->GetHeapObjectSize() << "/" << nonMovableSpace_->GetCommittedSize()
                   << "/" << nonMovableSpace_->GetInitialCapacity() << "), Old("
                   << oldSpace_->GetHeapObjectSize() << "/" << oldSpace_->GetCommittedSize()
                   << "/" << oldSpace_->GetInitialCapacity() << "), HugeObject("
                   << hugeObjectSpace_->GetHeapObjectSize() << "/" << hugeObjectSpace_->GetCommittedSize()
                   << "/" << hugeObjectSpace_->GetInitialCapacity() << "), ReadOnlySpace("
                   << readOnlySpace_->GetCommittedSize() << "/" << readOnlySpace_->GetInitialCapacity()
                   << "), AppspawnSpace(" << appSpawnSpace_->GetHeapObjectSize() << "/"
                   << appSpawnSpace_->GetCommittedSize() << "/" << appSpawnSpace_->GetInitialCapacity()
                   << "), GlobalLimitSize(" << globalSpaceAllocLimit_ << ").";
}

void Heap::StatisticHeapObject(TriggerGCType gcType) const
{
    PrintHeapInfo(gcType);
#if ECMASCRIPT_ENABLE_HEAP_DETAIL_STATISTICS
    static const int JS_TYPE_LAST = static_cast<int>(JSType::TYPE_LAST);
    int typeCount[JS_TYPE_LAST] = { 0 };
    static const int MIN_COUNT_THRESHOLD = 1000;

    nonMovableSpace_->IterateOverObjects([&typeCount] (TaggedObject *object) {
        typeCount[static_cast<int>(object->GetClass()->GetObjectType())]++;
    });
    for (int i = 0; i < JS_TYPE_LAST; i++) {
        if (typeCount[i] > MIN_COUNT_THRESHOLD) {
            LOG_ECMA(INFO) << "NonMovable space type " << JSHClass::DumpJSType(JSType(i))
                           << " count:" << typeCount[i];
        }
        typeCount[i] = 0;
    }

    oldSpace_->IterateOverObjects([&typeCount] (TaggedObject *object) {
        typeCount[static_cast<int>(object->GetClass()->GetObjectType())]++;
    });
    for (int i = 0; i < JS_TYPE_LAST; i++) {
        if (typeCount[i] > MIN_COUNT_THRESHOLD) {
            LOG_ECMA(INFO) << "Old space type " << JSHClass::DumpJSType(JSType(i))
                           << " count:" << typeCount[i];
        }
        typeCount[i] = 0;
    }

    activeSemiSpace_->IterateOverObjects([&typeCount] (TaggedObject *object) {
        typeCount[static_cast<int>(object->GetClass()->GetObjectType())]++;
    });
    for (int i = 0; i < JS_TYPE_LAST; i++) {
        if (typeCount[i] > MIN_COUNT_THRESHOLD) {
            LOG_ECMA(INFO) << "Active semi space type " << JSHClass::DumpJSType(JSType(i))
                           << " count:" << typeCount[i];
        }
        typeCount[i] = 0;
    }
#endif
}
}  // namespace panda::ecmascript
