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

    LOG_GC(INFO) << "heap initialize: heap size = " << (maxHeapSize / 1_MB) << "MB"
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
    idleData_ = new IdleData();
    enableIdleGC_ = ecmaVm_->GetJSOptions().EnableIdleGC();
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
    if (idleData_ != nullptr) {
        delete idleData_;
        idleData_ = nullptr;
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
        oldSpace_->SetMaximumCapacity(oldSpaceMaxLimit);
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
        GetHeapObjectSize() <= globalSpaceAllocLimit_) {
        return YOUNG_GC;
    }
    return OLD_GC;
}

void Heap::CollectGarbage(TriggerGCType gcType)
{
#if defined(ECMASCRIPT_SUPPORT_CPUPROFILER)
    [[maybe_unused]] GcStateScope scope(thread_);
#endif
    CHECK_NO_GC
#if ECMASCRIPT_ENABLE_HEAP_VERIFY
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
    size_t originalNewSpaceSize = activeSemiSpace_->GetHeapObjectSize();
    size_t originalNewSpaceNativeSize = activeSemiSpace_->GetNativeBindingSize();
    memController_->StartCalculationBeforeGC();
    StatisticHeapObject(gcType);
    switch (gcType) {
        case TriggerGCType::YOUNG_GC:
            // Use partial GC for young generation.
            if (!concurrentMarker_->IsEnabled()) {
                SetMarkType(MarkType::MARK_YOUNG);
            }
            partialGC_->RunPhases();
            break;
        case TriggerGCType::OLD_GC:
            if (concurrentMarker_->IsEnabled() && markType_ == MarkType::MARK_YOUNG) {
                // Wait for existing concurrent marking tasks to be finished (if any),
                // and reset concurrent marker's status for full mark.
                bool concurrentMark = CheckOngoingConcurrentMarking();
                if (concurrentMark) {
                    concurrentMarker_->Reset();
                }
            }
            SetMarkType(MarkType::MARK_FULL);
            partialGC_->RunPhases();
            break;
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
            UNREACHABLE();
            break;
    }

    // OOMError object is not allowed to be allocated during gc process, so throw OOMError after gc
    if (shouldThrowOOMError_) {
        ThrowOutOfMemoryError(oldSpace_->GetMergeSize(), " OldSpace::Merge");
        oldSpace_->ResetMergeSize();
        shouldThrowOOMError_ = false;
    }

    // Adjust the old space capacity and global limit for the first partial GC with full mark.
    // Trigger the full mark next time if the current survival rate is much less than half the average survival rates.
    AdjustBySurvivalRate(originalNewSpaceSize);
    activeSemiSpace_->AdjustNativeLimit(originalNewSpaceNativeSize);
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
    ecmaVm_->GetEcmaGCStats()->CheckIfLongTimePause();
# if ECMASCRIPT_ENABLE_GC_LOG
    ecmaVm_->GetEcmaGCStats()->PrintStatisticResult();
#endif
    // weak node secondPassCallback may execute JS and change the weakNodeList status,
    // even lead to another GC, so this have to invoke after this GC process.
    InvokeWeakNodeSecondPassCallback();

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

void Heap::ThrowOutOfMemoryError(size_t size, std::string functionName)
{
    GetEcmaVM()->GetEcmaGCStats()->PrintHeapStatisticResult(true);
    std::ostringstream oss;
    oss << "OutOfMemory when trying to allocate " << size << " bytes" << " function name: " << functionName.c_str();
    LOG_ECMA_MEM(ERROR) << oss.str().c_str();
    THROW_OOM_ERROR(thread_, oss.str().c_str());
}

void Heap::FatalOutOfMemoryError(size_t size, std::string functionName)
{
    GetEcmaVM()->GetEcmaGCStats()->PrintHeapStatisticResult(true);
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
    size_t globalSpaceNativeSize = activeSemiSpace_->GetNativeBindingSize() + nonNewSpaceNativeBindingSize_
                                   + nativeAreaAllocator_->GetNativeMemoryUsage();
    globalSpaceNativeLimit_ = memController_->CalculateAllocLimit(globalSpaceNativeSize, MIN_HEAP_SIZE,
                                                                  maxGlobalSize, newSpaceCapacity, growingFactor);
    OPTIONAL_LOG(ecmaVm_, INFO) << "RecomputeLimits oldSpaceAllocLimit_: " << newOldSpaceLimit
        << " globalSpaceAllocLimit_: " << globalSpaceAllocLimit_
        << " globalSpaceNativeLimit_:" << globalSpaceNativeLimit_;
    if ((oldSpace_->GetHeapObjectSize() * 1.0 / SHRINK_OBJECT_SURVIVAL_RATE) < oldSpace_->GetCommittedSize()
         && (oldSpace_->GetCommittedSize() / 2) > newOldSpaceLimit) {
        OPTIONAL_LOG(ecmaVm_, INFO) << " Old space heap object size is too much lower than committed size"
                                    << " heapObjectSize: "<< oldSpace_->GetHeapObjectSize()
                                    << " Committed Size: " << oldSpace_->GetCommittedSize();
        SetFullMarkRequestedState(true);
    }
}

void Heap::CheckAndTriggerOldGC(size_t size)
{
    if (OldSpaceExceedLimit() || OldSpaceExceedCapacity(size) || GetHeapObjectSize() > globalSpaceAllocLimit_) {
        CollectGarbage(TriggerGCType::OLD_GC);
    }
}

bool Heap::CheckOngoingConcurrentMarking()
{
    if (concurrentMarker_->IsEnabled() && !thread_->IsReadyToMark()) {
        if (thread_->IsMarking()) {
            [[maybe_unused]] ClockScope clockScope;
            ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "Heap::CheckOngoingConcurrentMarking");
            MEM_ALLOCATE_AND_GC_TRACE(GetEcmaVM(), WaitConcurrentMarkingFinished);
            GetNonMovableMarker()->ProcessMarkStack(MAIN_THREAD_INDEX);
            WaitConcurrentMarkingFinished();
            ecmaVm_->GetEcmaGCStats()->StatisticConcurrentMarkWait(clockScope.GetPauseTime());
            LOG_GC(DEBUG) << "wait concurrent marking finish pause time " << clockScope.TotalSpentTime();
        } else {
            WaitRunningTaskFinished();
        }
        memController_->RecordAfterConcurrentMark(IsFullMark(), concurrentMarker_);
        return true;
    }
    return false;
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
    if (!concurrentMarker_->IsEnabled() || !thread_->IsReadyToMark()) {
        return;
    }
    if (fullMarkRequested_) {
        markType_ = MarkType::MARK_FULL;
        OPTIONAL_LOG(ecmaVm_, INFO) << " fullMarkRequested, trigger full mark.";
        TriggerConcurrentMarking();
        return;
    }
    bool isFullMarkNeeded = false;
    double oldSpaceMarkDuration = 0, newSpaceMarkDuration = 0, newSpaceRemainSize = 0, newSpaceAllocToLimitDuration = 0,
           oldSpaceAllocToLimitDuration = 0;
    double oldSpaceAllocSpeed = memController_->GetOldSpaceAllocationThroughputPerMS();
    double oldSpaceConcurrentMarkSpeed = memController_->GetFullSpaceConcurrentMarkSpeedPerMS();
    size_t oldSpaceHeapObjectSize = oldSpace_->GetHeapObjectSize() + hugeObjectSpace_->GetHeapObjectSize();
    size_t globalHeapObjectSize = GetHeapObjectSize();
    size_t oldSpaceAllocLimit = oldSpace_->GetInitialCapacity();
    size_t globalSpaceNativeSize = activeSemiSpace_->GetNativeBindingSize() + nonNewSpaceNativeBindingSize_
                                   + nativeAreaAllocator_->GetNativeMemoryUsage();
    if (oldSpaceConcurrentMarkSpeed == 0 || oldSpaceAllocSpeed == 0) {
        if (oldSpaceHeapObjectSize >= oldSpaceAllocLimit ||  globalHeapObjectSize >= globalSpaceAllocLimit_
            || globalSpaceNativeSize >= globalSpaceNativeLimit_) {
            markType_ = MarkType::MARK_FULL;
            OPTIONAL_LOG(ecmaVm_, INFO) << "Trigger the first full mark";
            TriggerConcurrentMarking();
            return;
        }
    } else {
        if (oldSpaceHeapObjectSize >= oldSpaceAllocLimit || globalHeapObjectSize >= globalSpaceAllocLimit_
            || globalSpaceNativeSize >= globalSpaceNativeLimit_) {
            isFullMarkNeeded = true;
        }
        oldSpaceAllocToLimitDuration = (oldSpaceAllocLimit - oldSpaceHeapObjectSize) / oldSpaceAllocSpeed;
        oldSpaceMarkDuration = GetHeapObjectSize() / oldSpaceConcurrentMarkSpeed;
        // oldSpaceRemainSize means the predicted size which can be allocated after the full concurrent mark.
        double oldSpaceRemainSize = (oldSpaceAllocToLimitDuration - oldSpaceMarkDuration) * oldSpaceAllocSpeed;
        if (oldSpaceRemainSize > 0 && oldSpaceRemainSize < DEFAULT_REGION_SIZE) {
            isFullMarkNeeded = true;
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
    newSpaceAllocToLimitDuration = (activeSemiSpace_->GetInitialCapacity() - activeSemiSpace_->GetCommittedSize())
        / newSpaceAllocSpeed;
    newSpaceMarkDuration = activeSemiSpace_->GetHeapObjectSize() / newSpaceConcurrentMarkSpeed;
    // newSpaceRemainSize means the predicted size which can be allocated after the semi concurrent mark.
    newSpaceRemainSize = (newSpaceAllocToLimitDuration - newSpaceMarkDuration) * newSpaceAllocSpeed;

    if (isFullMarkNeeded) {
        if (oldSpaceMarkDuration < newSpaceAllocToLimitDuration
            && oldSpaceMarkDuration < oldSpaceAllocToLimitDuration) {
            markType_ = MarkType::MARK_FULL;
            TriggerConcurrentMarking();
            OPTIONAL_LOG(ecmaVm_, INFO) << "Trigger full mark by speed";
        } else {
            if (oldSpaceHeapObjectSize >= oldSpaceAllocLimit || globalHeapObjectSize >= globalSpaceAllocLimit_
                || globalSpaceNativeSize >= globalSpaceNativeLimit_) {
                markType_ = MarkType::MARK_FULL;
                TriggerConcurrentMarking();
                OPTIONAL_LOG(ecmaVm_, INFO) << "Trigger full mark by limit";
            }
        }
    } else if (newSpaceRemainSize < DEFAULT_REGION_SIZE || activeSemiSpace_->NativeBindingSizeLargerThanLimit()) {
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
    Region *region = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(object));
    if (region->InYoungSpace()) {
        activeSemiSpace_->IncreaseNativeBindingSize(size);
    } else {
        nonNewSpaceNativeBindingSize_ += size;
    }
}

void Heap::IncreaseNativeBindingSize(bool nonMovable, size_t size)
{
    if (size == 0) {
        return;
    }
    if (!nonMovable) {
        activeSemiSpace_->IncreaseNativeBindingSize(size);
    } else {
        nonNewSpaceNativeBindingSize_ += size;
    }
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
    if (concurrentMarker_->IsEnabled() && thread_->IsMarking()) {
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
    if (inBackground) {
        LOG_GC(INFO) << "app is inBackground";
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

void Heap::TriggerIdleCollection([[maybe_unused]] int idleMicroSec)
{
    if (!enableIdleGC_) {
        return;
    }
    int64_t curTime = 0;
    if (waitForStartUp_) {
        curTime = static_cast<int64_t>(JSDate::Now().GetDouble());
        if (idleTime_ == 0) {
            idleTime_ = curTime;
        }
        if (curTime - idleTime_ > WAIT_FOR_APP_START_UP) {
            waitForStartUp_ = false;
        }
        return;
    }

    if (idleMicroSec >= IDLE_TIME_REMARK && thread_->IsMarkFinished()) {
        concurrentMarker_->HandleMarkingFinished();
        return;
    }

    if (idleMicroSec >= IDLE_TIME_LIMIT) {
        curTime = static_cast<int64_t>(JSDate::Now().GetDouble());
        size_t oldCommitSize = oldSpace_->GetCommittedSize();
        // rest
        if (curTime - idleTime_ > MIN_OLD_GC_LIMIT) {
            size_t heapObjectSize = GetHeapObjectSize();
            idleData_->SetNextValue(heapObjectSize);
            idleTime_ = curTime;
            if (idleData_->CheckIsRest() && heapObjectSize > triggerRestIdleSize_) {
                CollectGarbage(TriggerGCType::FULL_GC);
                couldIdleGC_ = false;
                triggerRestIdleSize_ = GetHeapObjectSize() + REST_HEAP_GROWTH_LIMIT;
                return;
            }
            couldIdleGC_ = true;
            idleHeapObjectSize_ = GetHeapObjectSize();
        }

        // sparse space over limit
        if (couldIdleGC_ && oldCommitSize + nonMovableSpace_->GetCommittedSize() > idleOldSpace_) {
            CollectGarbage(TriggerGCType::OLD_GC);
            idleTime_ = curTime;
            couldIdleGC_ = false;
            idleOldSpace_ = oldSpace_->GetInitialCapacity();
            return;
        }

        if (activeSemiSpace_->GetHeapObjectSize() > IDLE_GC_YOUNG_SPACE) {
            CollectGarbage(TriggerGCType::YOUNG_GC);
            return;
        }
    }
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

void Heap::InvokeWeakNodeSecondPassCallback()
{
    // the second callback may lead to another GC, if this, return directly;
    if (runningSecondPassCallbacks_) {
        return;
    }
    runningSecondPassCallbacks_ = true;
    auto weakNodesSecondCallbacks = thread_->GetWeakNodeSecondPassCallbacks();
    while (!weakNodesSecondCallbacks->empty()) {
        auto callbackPair = weakNodesSecondCallbacks->back();
        weakNodesSecondCallbacks->pop_back();
        ASSERT(callbackPair.first != nullptr && callbackPair.second != nullptr);
        auto callback = callbackPair.first;
        (*callback)(callbackPair.second);
    }
    runningSecondPassCallbacks_ = false;
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
