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
#include "common_components/heap/w_collector/w_collector.h"

#include "common_components/base_runtime/hooks.h"
#include "common_components/log/log.h"
#include "common_components/mutator/mutator_manager-inl.h"
#include "common_components/heap/verification.h"
#include "common_interfaces/heap/heap_visitor.h"
#include "common_interfaces/objects/ref_field.h"
#include "common_interfaces/profiler/heap_profiler_listener.h"
#include "common_components/objects/string_table_internal.h"

namespace common {
bool WCollector::IsUnmovableFromObject(BaseObject* obj) const
{
    // filter const string object.
    if (!Heap::IsHeapAddress(obj)) {
        return false;
    }

    RegionDesc* regionInfo = nullptr;
    regionInfo = RegionDesc::GetRegionDescAt(reinterpret_cast<uintptr_t>(obj));
    return regionInfo->IsUnmovableFromRegion();
}

bool WCollector::MarkObject(BaseObject* obj, size_t cellCount) const
{
    bool marked = RegionSpace::MarkObject(obj);
    if (!marked) {
        RegionDesc* region = RegionDesc::GetRegionDescAt(reinterpret_cast<HeapAddress>(obj));
        (void)region;

        if (region->IsGarbageRegion()) {
            LOG_COMMON(FATAL) << "Unresolved fatal";
            UNREACHABLE_CC();
        }
        size_t size = cellCount == 0 ? obj->GetSize() : (cellCount + 1) * sizeof(uint64_t);
        region->AddLiveByteCount(size);
        DLOG(TRACE, "mark obj %p<%p>(%zu) in region %p(%u)@%#zx, live %u", obj, obj->GetTypeInfo(), obj->GetSize(),
             region, region->GetRegionType(), region->GetRegionStart(), region->GetLiveByteCount());
    }
    return marked;
}

// this api updates current pointer as well as old pointer, caller should take care of this.
template<bool copy>
bool WCollector::TryUpdateRefFieldImpl(BaseObject* obj, RefField<>& field, BaseObject*& fromObj,
                                       BaseObject*& toObj) const
{
    RefField<> oldRef(field);
    fromObj = oldRef.GetTargetObject();
    if (IsFromObject(fromObj)) {
        if (copy) {
            toObj = const_cast<WCollector*>(this)->TryForwardObject(fromObj);
            if (toObj != nullptr) {
                HeapProfilerListener::GetInstance().OnMoveEvent(reinterpret_cast<uintptr_t>(fromObj),
                                                                reinterpret_cast<uintptr_t>(toObj),
                                                                toObj->GetSize());
            }
        } else {
            toObj = FindToVersion(fromObj);
        }
        if (toObj == nullptr) {
            return false;
        }
        RefField<> tmpField(toObj);
        if (field.CompareExchange(oldRef.GetFieldValue(), tmpField.GetFieldValue())) {
            if (obj != nullptr) {
                DLOG(TRACE, "update obj %p<%p>(%zu)+%zu ref-field@%p: %#zx -> %#zx", obj, obj->GetTypeInfo(),
                    obj->GetSize(), BaseObject::FieldOffset(obj, &field), &field, oldRef.GetFieldValue(),
                    tmpField.GetFieldValue());
            } else {
                DLOG(TRACE, "update ref@%p: 0x%zx -> %p", &field, oldRef.GetFieldValue(), toObj);
            }
            return true;
        } else {
            if (obj != nullptr) {
                DLOG(TRACE,
                    "update obj %p<%p>(%zu)+%zu but cas failed ref-field@%p: %#zx(%#zx) -> %#zx but cas failed ",
                     obj, obj->GetTypeInfo(), obj->GetSize(), BaseObject::FieldOffset(obj, &field), &field,
                     oldRef.GetFieldValue(), field.GetFieldValue(), tmpField.GetFieldValue());
            } else {
                DLOG(TRACE, "update but cas failed ref@%p: 0x%zx(%zx) -> %p", &field, oldRef.GetFieldValue(),
                     field.GetFieldValue(), toObj);
            }
            return true;
        }
    }

    return false;
}

bool WCollector::TryUpdateRefField(BaseObject* obj, RefField<>& field, BaseObject*& newRef) const
{
    BaseObject* oldRef = nullptr;
    return TryUpdateRefFieldImpl<false>(obj, field, oldRef, newRef);
}

bool WCollector::TryForwardRefField(BaseObject* obj, RefField<>& field, BaseObject*& newRef) const
{
    BaseObject* oldRef = nullptr;
    return TryUpdateRefFieldImpl<true>(obj, field, oldRef, newRef);
}

// this api untags current pointer as well as old pointer, caller should take care of this.
bool WCollector::TryUntagRefField(BaseObject* obj, RefField<>& field, BaseObject*& target) const
{
    for (;;) {
        RefField<> oldRef(field);
        if (oldRef.IsTagged()) {
            target = oldRef.GetTargetObject();
            RefField<> newRef(target);
            if (field.CompareExchange(oldRef.GetFieldValue(), newRef.GetFieldValue())) {
                if (obj != nullptr) {
                    DLOG(FIX, "untag obj %p<%p>(%zu) ref-field@%p: %#zx -> %#zx", obj, obj->GetTypeInfo(),
                         obj->GetSize(), &field, oldRef.GetFieldValue(), newRef.GetFieldValue());
                } else {
                    DLOG(FIX, "untag ref@%p: %#zx -> %#zx", &field, oldRef.GetFieldValue(), newRef.GetFieldValue());
                }

                return true;
            }
        } else {
            return false;
        }
    }

    return false;
}

// RefFieldRoot is root in tagged pointer format.
void WCollector::EnumRefFieldRoot(RefField<>& field, RootSet& rootSet) const
{
    auto value = field.GetFieldValue();
    ASSERT_LOGF(Heap::IsTaggedObject(value), "EnumRefFieldRoot failed: Invalid root");

    // need fix or clean
    BaseObject* obj = field.GetTargetObject();
    RegionDesc* objRegion = RegionDesc::GetRegionDescAt(reinterpret_cast<MAddress>(obj));
    if (Heap::GetHeap().GetGCReason() == GC_REASON_YOUNG && objRegion->IsInOldSpace()) {
        DLOG(ENUM, "enum: skip old object %p<%p>(%zu)", obj, obj->GetTypeInfo(), obj->GetSize());
        return;
    }
    rootSet.push_back(obj);
    return;

    // consider remove below
    RefField<> oldField(field);
    // if field is already tagged currently, it is also already enumerated.
    if (IsCurrentPointer(oldField)) {
        rootSet.push_back(oldField.GetTargetObject());
        return;
    }

    BaseObject* latest = nullptr;
    if (IsOldPointer(oldField)) {
        BaseObject* targetObj = oldField.GetTargetObject();
        latest = FindLatestVersion(targetObj);
    } else {
        latest = field.GetTargetObject();
    }

    // target object could be null or non-heap for some static variable.
    if (!Heap::IsHeapAddress(latest)) {
        return;
    }
    CHECK_CC(latest->IsValidObject());

    RefField<> newField = GetAndTryTagRefField(latest);
    if (oldField.GetFieldValue() == newField.GetFieldValue()) {
        DLOG(ENUM, "enum static ref@%p: %#zx -> %p<%p>(%zu)", &field, oldField.GetFieldValue(), latest,
             latest->GetTypeInfo(), latest->GetSize());
    } else if (field.CompareExchange(oldField.GetFieldValue(), newField.GetFieldValue())) {
        DLOG(ENUM, "enum static ref@%p: %#zx=>%#zx -> %p<%p>(%zu)", &field, oldField.GetFieldValue(),
             newField.GetFieldValue(), latest, latest->GetTypeInfo(), latest->GetSize());
    } else {
        DLOG(ENUM, "enum static ref@%p: %#zx -> %p<%p>(%zu)", &field, oldField.GetFieldValue(), latest,
             latest->GetTypeInfo(), latest->GetSize());
    }
    rootSet.push_back(latest);
}

// note each ref-field will not be traced twice, so each old pointer the tracer meets must come from previous gc.
void WCollector::TraceRefField(BaseObject* obj, RefField<>& field, WorkStack& workStack, WeakStack& weakStack) const
{
    RefField<> oldField(field);
    BaseObject* targetObj = oldField.GetTargetObject();

    if (!Heap::IsTaggedObject(oldField.GetFieldValue())) {
        return;
    }
    // field is tagged object, should be in heap
    DCHECK_CC(Heap::IsHeapAddress(targetObj));

    auto gcReason = Heap::GetHeap().GetGCReason();
    auto targetRegion = RegionDesc::GetRegionDescAt(reinterpret_cast<MAddress>((void*)targetObj));

    if (gcReason != GC_REASON_YOUNG && oldField.IsWeak()) {
        DLOG(TRACE, "trace: skip weak obj when full gc, object: %p@%p, targetObj: %p", obj, &field, targetObj);
        weakStack.push_back(&field);
        return;
    }

    if (gcReason == GC_REASON_YOUNG && targetRegion->IsInOldSpace()) {
        DLOG(TRACE, "trace: skip old object %p@%p, target object: %p<%p>(%zu)",
            obj, &field, targetObj, targetObj->GetTypeInfo(), targetObj->GetSize());
        return;
    }

    if (targetRegion->IsNewObjectSinceTrace(targetObj)) {
        DLOG(TRACE, "trace: skip new obj %p<%p>(%zu)", targetObj, targetObj->GetTypeInfo(), targetObj->GetSize());
        return;
    }

    if (targetRegion->MarkObject(targetObj)) {
        DLOG(TRACE, "trace: obj has been marked %p", targetObj);
        return;
    }

    DLOG(TRACE, "trace obj %p ref@%p: %p<%p>(%zu)",
        obj, &field, targetObj, targetObj->GetTypeInfo(), targetObj->GetSize());
    workStack.push_back(targetObj);
}

void WCollector::TraceObjectRefFields(BaseObject* obj, WorkStack& workStack, WeakStack& weakStack)
{
    auto refFunc = [this, obj, &workStack, &weakStack] (RefField<>& field) {
        TraceRefField(obj, field, workStack, weakStack);
    };

    obj->ForEachRefField(refFunc);
}

void WCollector::FixRefField(BaseObject* obj, RefField<>& field) const
{
    RefField<> oldField(field);
    BaseObject* targetObj = oldField.GetTargetObject();
    if (!Heap::IsTaggedObject(oldField.GetFieldValue())) {
        return;
    }
    // target object could be null or non-heap for some static variable.
    if (!Heap::IsHeapAddress(targetObj)) {
        return;
    }

    BaseObject* latest = FindToVersion(targetObj);

    // update remember set
    BaseObject* toObj = latest == nullptr ? targetObj : latest;
    RegionDesc* objRegion = RegionDesc::GetRegionDescAt(reinterpret_cast<MAddress>((void*)obj));
    RegionDesc* refRegion = RegionDesc::GetRegionDescAt(reinterpret_cast<MAddress>((void*)toObj));
    if (!objRegion->IsInRecentSpace() && refRegion->IsInRecentSpace()) {
        if (objRegion->MarkRSetCardTable(obj)) {
            DLOG(TRACE, "fix phase update point-out remember set of region %p, obj %p, ref: %p<%p>",
                objRegion, obj, toObj, toObj->GetTypeInfo());
        }
    }
    if (latest == nullptr) { return; }

    CHECK_CC(latest->IsValidObject());
    RefField<> newField(latest, oldField.IsWeak());
    if (field.CompareExchange(oldField.GetFieldValue(), newField.GetFieldValue())) {
        DLOG(FIX, "fix obj %p+%zu ref@%p: %#zx => %p<%p>(%zu)", obj, obj->GetSize(), &field,
             oldField.GetFieldValue(), latest, latest->GetTypeInfo(), latest->GetSize());
    }
}

void WCollector::FixObjectRefFields(BaseObject* obj) const
{
    DLOG(FIX, "fix obj %p<%p>(%zu)", obj, obj->GetTypeInfo(), obj->GetSize());
    auto refFunc = [this, obj](RefField<>& field) { FixRefField(obj, field); };
    obj->ForEachRefField(refFunc);
}

BaseObject* WCollector::ForwardUpdateRawRef(ObjectRef& root)
{
    auto& refField = reinterpret_cast<RefField<>&>(root);
    RefField<> oldField(refField);
    BaseObject* oldObj = oldField.GetTargetObject();
    DLOG(FIX, "try fix raw-ref @%p: %p", &root, oldObj);
    if (IsFromObject(oldObj)) {
        BaseObject* toVersion = TryForwardObject(oldObj);
        CHECK_CC(toVersion != nullptr);
        HeapProfilerListener::GetInstance().OnMoveEvent(reinterpret_cast<uintptr_t>(oldObj),
                                                        reinterpret_cast<uintptr_t>(toVersion),
                                                        toVersion->GetSize());
        RefField<> newField(toVersion);
        // CAS failure means some mutator or gc thread writes a new ref (must be a to-object), no need to retry.
        if (refField.CompareExchange(oldField.GetFieldValue(), newField.GetFieldValue())) {
            DLOG(FIX, "fix raw-ref @%p: %p -> %p", &root, oldObj, toVersion);
            return toVersion;
        }
    }

    return oldObj;
}

void WCollector::RemarkAndPreforwardStaticRoots(WorkStack& workStack)
{
    const auto markObject = [&workStack, this](BaseObject *temp) {
        if (!this->MarkObject(temp)) {
            workStack.push_back(temp);
        }
    };

    const auto markToObject = [&workStack, this](BaseObject *oldVersion, BaseObject *toVersion) {
        if (!this->MarkObject(toVersion)) {
            // Therefore, we must still attempt to mark the old object to prevent
            // it from being pushed into the mark stack during subsequent
            // traversals.
            this->MarkObject(oldVersion);
            // The reference in toSpace needs to be fixed up. Therefore, even if
            // the oldVersion has been marked, it must still be pushed into the
            // stack. This will be optimized later.
            workStack.push_back(toVersion);
        }
    };
    RefFieldVisitor visitor = [this, &markObject, &markToObject](RefField<>& refField) {
        RefField<> oldField(refField);
        BaseObject* oldObj = oldField.GetTargetObject();
        DLOG(FIX, "visit raw-ref @%p: %p", &refField, oldObj);
        if (IsFromObject(oldObj)) {
            BaseObject* toVersion = TryForwardObject(oldObj);
            CHECK_CC(toVersion != nullptr);
            HeapProfilerListener::GetInstance().OnMoveEvent(reinterpret_cast<uintptr_t>(oldObj),
                                                            reinterpret_cast<uintptr_t>(toVersion),
                                                            toVersion->GetSize());
            RefField<> newField(toVersion);
            // CAS failure means some mutator or gc thread writes a new ref (must be a to-object), no need to retry.
            if (refField.CompareExchange(oldField.GetFieldValue(), newField.GetFieldValue())) {
                DLOG(FIX, "fix raw-ref @%p: %p -> %p", &refField, oldObj, toVersion);
            }
            markToObject(oldObj, toVersion);
        } else {
            markObject(oldObj);
        }
    };
    VisitRoots(visitor);
    // inline MergeAllocBufferRoots
    MutatorManager &mutatorManager = MutatorManager::Instance();
    bool worldStopped = mutatorManager.WorldStopped();
    worldStopped ? ((void)0) : mutatorManager.MutatorManagementWLock();
    theAllocator_.VisitAllocBuffers([&markObject](AllocationBuffer &buffer) { buffer.MarkStack(markObject); });
    worldStopped ? ((void)0) : mutatorManager.MutatorManagementWUnlock();
}

void WCollector::PreforwardStaticWeakRoots()
{
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::PreforwardStaticRoots", "");

    WeakRefFieldVisitor weakVisitor = GetWeakRefFieldVisitor();
    VisitWeakRoots(weakVisitor);
    MutatorManager::Instance().VisitAllMutators([](Mutator& mutator) {
        // Request finalize callback in each vm-thread when gc finished.
        mutator.SetFinalizeRequest();
    });

    AllocationBuffer* allocBuffer = AllocationBuffer::GetAllocBuffer();
    if (LIKELY_CC(allocBuffer != nullptr)) {
        allocBuffer->ClearRegion();
    }
}

void WCollector::PreforwardConcurrencyModelRoots()
{
    LOG_COMMON(FATAL) << "Unresolved fatal";
    UNREACHABLE_CC();
}

void WCollector::EnumRoots(WorkStack& workStack)
{
    // assemble garbage candidates.
    reinterpret_cast<RegionSpace&>(theAllocator_).AssembleGarbageCandidates();
    reinterpret_cast<RegionSpace&>(theAllocator_).PrepareTrace();

    COMMON_PHASE_TIMER("enum roots & update old pointers within");
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::EnumRoots", "");
    TransitionToGCPhase(GCPhase::GC_PHASE_ENUM, true);
    EnumerateAllRoots(workStack);
}

void WCollector::TraceHeap(WorkStack& workStack)
{
    COMMON_PHASE_TIMER("trace live objects");
    markedObjectCount_.store(0, std::memory_order_relaxed);
    TransitionToGCPhase(GCPhase::GC_PHASE_MARK, true);

    TraceRoots(workStack);
    ProcessFinalizers();
    ExemptFromSpace();
}

void WCollector::PostTrace()
{
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::PostTrace", "");
    COMMON_PHASE_TIMER("PostTrace");
    TransitionToGCPhase(GC_PHASE_POST_MARK, true);

    // clear satb buffer when gc finish tracing.
    SatbBuffer::Instance().ClearBuffer();

    WVerify::VerifyAfterMark(*this);
}

WeakRefFieldVisitor WCollector::GetWeakRefFieldVisitor()
{
    return [this](RefField<> &refField) -> bool {
        RefField<> oldField(refField);
        BaseObject *oldObj = oldField.GetTargetObject();
        auto gcReason = Heap::GetHeap().GetGCReason();
        if (gcReason == GC_REASON_YOUNG) {
            if (RegionSpace::IsYoungSpaceObject(oldObj) && !IsMarkedObject(oldObj) &&
                !RegionSpace::IsNewObjectSinceTrace(oldObj)) {
                return false;
            }
        } else {
            if (!IsMarkedObject(oldObj) && !RegionSpace::IsNewObjectSinceTrace(oldObj)) {
                return false;
            }
        }

        DLOG(FIX, "visit weak raw-ref @%p: %p", &refField, oldObj);
        if (IsFromObject(oldObj)) {
            BaseObject *toVersion = TryForwardObject(oldObj);
            CHECK_CC(toVersion != nullptr);
            HeapProfilerListener::GetInstance().OnMoveEvent(reinterpret_cast<uintptr_t>(oldObj),
                                                            reinterpret_cast<uintptr_t>(toVersion),
                                                            toVersion->GetSize());
            RefField<> newField(toVersion);
            // CAS failure means some mutator or gc thread writes a new ref (must be
            // a to-object), no need to retry.
            if (refField.CompareExchange(oldField.GetFieldValue(), newField.GetFieldValue())) {
                DLOG(FIX, "fix weak raw-ref @%p: %p -> %p", &refField, oldObj, toVersion);
            }
        }
        return true;
    };
}

void WCollector::PreforwardFlip(WorkStack& workStack)
{
    auto remarkAndForwardGlobalRoot = [this, &workStack]() {
        OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::PreforwardFlip[STW]", "");
        ASSERT_LOGF(GetThreadPool() != nullptr, "thread pool is null");
        TransitionToGCPhase(GCPhase::GC_PHASE_FINAL_MARK, true);
        Remark(workStack);
        PostTrace();
        reinterpret_cast<RegionSpace&>(theAllocator_).PrepareForward();

        TransitionToGCPhase(GCPhase::GC_PHASE_PRECOPY, true);
        WeakRefFieldVisitor weakVisitor = GetWeakRefFieldVisitor();
        VisitWeakGlobalRoots(weakVisitor);
    };
    FlipFunction forwardMutatorRoot = [this](Mutator &mutator) {
        WeakRefFieldVisitor weakVisitor = GetWeakRefFieldVisitor();
        VisitWeakMutatorRoot(weakVisitor, mutator);
        // Request finalize callback in each vm-thread when gc finished.
        mutator.SetFinalizeRequest();
    };
    MutatorManager::Instance().FlipMutators("final-mark", remarkAndForwardGlobalRoot, &forwardMutatorRoot);

    AllocationBuffer* allocBuffer = AllocationBuffer::GetAllocBuffer();
    if (LIKELY_CC(allocBuffer != nullptr)) {
        allocBuffer->ClearRegion();
    }
}

void WCollector::Preforward()
{
    COMMON_PHASE_TIMER("Preforward");
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::Preforward[STW]", "");
    TransitionToGCPhase(GCPhase::GC_PHASE_PRECOPY, true);

    [[maybe_unused]] Taskpool *threadPool = GetThreadPool();
    ASSERT_LOGF(threadPool != nullptr, "thread pool is null");

    // copy and fix finalizer roots.
    // Only one root task, no need to post task.
    PreforwardStaticWeakRoots();
}

void WCollector::PrepareFix()
{
    // make sure all objects before fixline is initialized
    COMMON_PHASE_TIMER("PrepareFix");
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::PrepareFix[STW]", "");
    reinterpret_cast<RegionSpace&>(theAllocator_).PrepareFix();
    reinterpret_cast<RegionSpace&>(theAllocator_).PrepareFixForPin();
#ifndef GC_STW_STRINGTABLE
    auto *baseRuntime = BaseRuntime::GetInstance();
    auto& stringTable = reinterpret_cast<BaseStringTableImpl&>(baseRuntime->GetStringTable());
    stringTable.GetInternalTable()->GetCleaner()->CleanUp();
#endif
    TransitionToGCPhase(GCPhase::GC_PHASE_FIX, true);
}

void WCollector::FixHeap()
{
    COMMON_PHASE_TIMER("FixHeap");
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::FixHeap", "");
    reinterpret_cast<RegionSpace&>(theAllocator_).FixHeap();

    WVerify::VerifyAfterFix(*this);
}

void WCollector::DoGarbageCollection()
{
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::DoGarbageCollection", "");
    if (gcMode_ == GCMode::STW) { // 2: stw-gc
#ifdef ENABLE_CMC_RB_DFX
        WVerify::DisableReadBarrierDFX(*this);
#endif

        ScopedStopTheWorld stw("stw-gc");
        WorkStack workStack = NewWorkStack();
        EnumRoots(workStack);
        TraceHeap(workStack);
        Remark(workStack);
        PostTrace();

        Preforward();
        // reclaim large objects should after preforward(may process weak ref) and
        // before fix heap(may clear live bit)
        if (Heap::GetHeap().GetGCReason() != GC_REASON_YOUNG) {
            CollectLargeGarbage();
        }
        SweepThreadLocalJitFort();

        CopyFromSpace();
        WVerify::VerifyAfterForward(*this);

        PrepareFix();
        FixHeap();
        if (Heap::GetHeap().GetGCReason() != GC_REASON_YOUNG) {
            CollectPinnedGarbage();
        }

        TransitionToGCPhase(GCPhase::GC_PHASE_IDLE, true);

        ClearAllGCInfo();
        CollectSmallSpace();

#if defined(ENABLE_CMC_RB_DFX)
        WVerify::EnableReadBarrierDFX(*this);
#endif
        return;
    } else if (gcMode_ == GCMode::CONCURRENT_MARK) { // 1: concurrent-mark
        WorkStack workStack = NewWorkStack();
        {
            ScopedStopTheWorld stw("wgc-enumroot");
            EnumRoots(workStack);
        }
        TraceHeap(workStack);
    {
        ScopedStopTheWorld stw("final-mark", true, GCPhase::GC_PHASE_FINAL_MARK);
        Remark(workStack);
        PostTrace();
        reinterpret_cast<RegionSpace&>(theAllocator_).PrepareForward();
        Preforward();
    }
        // reclaim large objects should after preforward(may process weak ref) and
        // before fix heap(may clear live bit)
        if (Heap::GetHeap().GetGCReason() != GC_REASON_YOUNG) {
            CollectLargeGarbage();
        }
        SweepThreadLocalJitFort();

        CopyFromSpace();
        WVerify::VerifyAfterForward(*this);

        PrepareFix();
        FixHeap();
        if (Heap::GetHeap().GetGCReason() != GC_REASON_YOUNG) {
            CollectPinnedGarbage();
        }

        TransitionToGCPhase(GCPhase::GC_PHASE_IDLE, true);
        ClearAllGCInfo();
        CollectSmallSpace();
        return;
    }

    WorkStack workStack = NewWorkStack();
    EnumRootsFlip(workStack);
    TraceHeap(workStack);
    PreforwardFlip(workStack);
    // reclaim large objects should after preforward(may process weak ref)
    // and before fix heap(may clear live bit)
    if (Heap::GetHeap().GetGCReason() != GC_REASON_YOUNG) {
        CollectLargeGarbage();
    }
    SweepThreadLocalJitFort();

    CopyFromSpace();
    WVerify::VerifyAfterForward(*this);

    {
        ScopedStopTheWorld stw("wgc-preparefix");
        PrepareFix();
    }
    FixHeap();
    if (Heap::GetHeap().GetGCReason() != GC_REASON_YOUNG) {
        CollectPinnedGarbage();
    }

    TransitionToGCPhase(GCPhase::GC_PHASE_IDLE, true);
    ClearAllGCInfo();
    CollectSmallSpace();
}

void WCollector::EnumRootsFlip(WorkStack& rootSet)
{
    std::mutex stackMutex;
    auto enumGlobalRoot = [this, &rootSet]() {
        // assemble garbage candidates.
        reinterpret_cast<RegionSpace&>(theAllocator_).AssembleGarbageCandidates();
        reinterpret_cast<RegionSpace&>(theAllocator_).PrepareTrace();

        COMMON_PHASE_TIMER("enum roots & update old pointers within");
        OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::EnumRootsFlip[STW]", "");
        TransitionToGCPhase(GCPhase::GC_PHASE_ENUM, true);
        const RefFieldVisitor& visitor =
            [this, &rootSet](RefField<>& root) { EnumRefFieldRoot(root, rootSet); };
        VisitGlobalRoots(visitor);
        MergeAllocBufferRoots(rootSet);
    };
    FlipFunction enumMutatorRoot = [this, &rootSet, &stackMutex](Mutator &mutator) {
        RootSet tmpSet;
        const RefFieldVisitor& visitor =
            [this, &tmpSet](RefField<>& root) { EnumRefFieldRoot(root, tmpSet); };
        VisitMutatorRoot(visitor, mutator);
        std::lock_guard<std::mutex> lockGuard(stackMutex);
        rootSet.insert(tmpSet);
    };
    MutatorManager::Instance().FlipMutators("wgc-enumroot", enumGlobalRoot, &enumMutatorRoot);
}

void WCollector::ProcessStringTable()
{
#ifdef GC_STW_STRINGTABLE
    return;
#endif
    WeakRefFieldVisitor weakVisitor = [this](RefField<> &refField) -> bool {
        auto isSurvivor = [this](BaseObject* oldObj) {
            RegionDesc* region = RegionDesc::GetRegionDescAt(reinterpret_cast<uintptr_t>(oldObj));
            auto gcReason = Heap::GetHeap().GetGCReason();
            return (gcReason == GC_REASON_YOUNG && !region->IsInYoungSpace())
                || region->IsMarkedObject(oldObj)
                || region->IsNewObjectSinceTrace(oldObj)
                || region->IsToRegion();
        };

        RefField<> oldField(refField);
        BaseObject *oldObj = oldField.GetTargetObject();
        if (oldObj == nullptr) {
            return false;
        }
        if (!isSurvivor(oldObj)) {
            // CAS failure means some mutator or gc thread writes a new ref (must be
            // a to-object), no need to retry.
            RefField<> newField(nullptr);
            if (refField.CompareExchange(oldField.GetFieldValue(), newField.GetFieldValue())) {
                DLOG(FIX, "fix weak raw-ref @%p: %p -> %p", &refField, oldObj, nullptr);
            }
            return false;
        }
        DLOG(FIX, "visit weak raw-ref @%p: %p", &refField, oldObj);
        if (IsFromObject(oldObj)) {
            BaseObject *toVersion = TryForwardObject(oldObj);
            CHECK_CC(toVersion != nullptr);
            RefField<> newField(toVersion);
            // CAS failure means some mutator or gc thread writes a new ref (must be
            // a to-object), no need to retry.
            if (refField.CompareExchange(oldField.GetFieldValue(), newField.GetFieldValue())) {
                DLOG(FIX, "fix weak raw-ref @%p: %p -> %p", &refField, oldObj, toVersion);
            }
        }
        return true;
    };
    auto* baseRuntime = BaseRuntime::GetInstance();
    auto& stringTable = reinterpret_cast<BaseStringTableImpl&>(baseRuntime->GetStringTable());
    auto stringTableCleaner = stringTable.GetInternalTable()->GetCleaner();
    stringTableCleaner->PostSweepWeakRefTask(weakVisitor);
    stringTableCleaner->JoinAndWaitSweepWeakRefTask(weakVisitor);
}

void WCollector::ProcessWeakReferences()
{
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::ProcessWeakReferences", "");
    {
        OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::ProcessGlobalWeakStack", "");
        while (!globalWeakStack_.empty()) {
            RefField<>& field = reinterpret_cast<RefField<>&>(*globalWeakStack_.back());
            globalWeakStack_.pop_back();
            RefField<> oldField(field);
            BaseObject* targetObj = oldField.GetTargetObject();
            if (Heap::GetHeap().GetGCReason() == GC_REASON_YOUNG) {
                if (!Heap::IsHeapAddress(targetObj) || IsMarkedObject(targetObj) ||
                    RegionSpace::IsNewObjectSinceTrace(targetObj) || !RegionSpace::IsYoungSpaceObject(targetObj)) {
                    continue;
                }
            } else {
                if (!Heap::IsHeapAddress(targetObj) || IsMarkedObject(targetObj) ||
                    RegionSpace::IsNewObjectSinceTrace(targetObj)) {
                    continue;
                }
                field.ClearRef(oldField.GetFieldValue());
            }
        }
    }
}

void WCollector::ProcessFinalizers()
{
    std::function<bool(BaseObject*)> finalizable = [this](BaseObject* obj) { return !IsMarkedObject(obj); };
    FinalizerProcessor& fp = collectorResources_.GetFinalizerProcessor();
    fp.EnqueueFinalizables(finalizable, snapshotFinalizerNum_);
    fp.Notify();
}

BaseObject* WCollector::ForwardObject(BaseObject* obj)
{
    BaseObject* to = TryForwardObject(obj);
    if (to != nullptr) {
        HeapProfilerListener::GetInstance().OnMoveEvent(reinterpret_cast<uintptr_t>(obj),
                                                        reinterpret_cast<uintptr_t>(to),
                                                        to->GetSize());
    }
    return (to != nullptr) ? to : obj;
}

BaseObject* WCollector::TryForwardObject(BaseObject* obj)
{
    return CopyObjectImpl(obj);
}

// ConcurrentGC
BaseObject* WCollector::CopyObjectImpl(BaseObject* obj)
{
    // reconsider phase difference between mutator and GC thread during transition.
    if (IsGcThread()) {
        CHECK_CC(GetGCPhase() == GCPhase::GC_PHASE_PRECOPY || GetGCPhase() == GCPhase::GC_PHASE_COPY ||
                 GetGCPhase() == GCPhase::GC_PHASE_FIX || GetGCPhase() == GCPhase::GC_PHASE_FINAL_MARK);
    } else {
        auto phase = Mutator::GetMutator()->GetMutatorPhase();
        CHECK_CC(phase == GCPhase::GC_PHASE_PRECOPY || phase == GCPhase::GC_PHASE_COPY ||
                 phase == GCPhase::GC_PHASE_FIX);
    }

    do {
        BaseStateWord oldWord = obj->GetBaseStateWord();

        // 1. object has already been forwarded
        if (obj->IsForwarded()) {
            auto toObj = GetForwardingPointer(obj);
            DLOG(COPY, "skip forwarded obj %p -> %p<%p>(%zu)", obj, toObj, toObj->GetTypeInfo(), toObj->GetSize());
            return toObj;
        }

        // ConcurrentGC
        // 2. object is being forwarded, spin until it is forwarded (or gets its own forwarded address)
        if (oldWord.IsForwarding()) {
            sched_yield();
            continue;
        }

        // 3. hope we can copy this object
        if (obj->TryLockExclusive(oldWord)) {
            return CopyObjectAfterExclusive(obj);
        }
    } while (true);
    LOG_COMMON(FATAL) << "forwardObject exit in wrong path";
    UNREACHABLE_CC();
    return nullptr;
}

BaseObject* WCollector::CopyObjectAfterExclusive(BaseObject* obj)
{
    size_t size = RegionSpace::GetAllocSize(*obj);
    // 8: size of free object, but free object can not be copied.
    if (size == 8) {
        LOG_COMMON(FATAL) << "forward free obj: " << obj <<
            "is survived: " << (IsSurvivedObject(obj) ? "true" : "false");
    }
    BaseObject* toObj = fwdTable_.RouteObject(obj, size);
    if (toObj == nullptr) {
        ASSERT_LOGF(0, "OOM");
        // ConcurrentGC
        obj->UnlockExclusive(BaseStateWord::ForwardState::NORMAL);
        return toObj;
    }
    DLOG(COPY, "copy obj %p<%p>(%zu) to %p", obj, obj->GetTypeInfo(), size, toObj);
    CopyObject(*obj, *toObj, size);
    if (IsToObject(toObj)) {
        toObj->SetForwardState(BaseStateWord::ForwardState::NORMAL);
    } else {
        // if this object is not in to-space, we label it as to-object explicitly.
        toObj->SetForwardState(BaseStateWord::ForwardState::TO_VERSION);
    }
    std::atomic_thread_fence(std::memory_order_release);
    obj->SetSizeForwarded(size);
    // Avoid seeing the fwd pointer before observing the size modification
    // when calling GetSize during the CopyPhase.
    std::atomic_thread_fence(std::memory_order_release);
    obj->SetForwardingPointerAfterExclusive(toObj);
    return toObj;
}

void WCollector::ClearAllGCInfo()
{
    COMMON_PHASE_TIMER("ClearAllGCInfo");
    RegionSpace& space = reinterpret_cast<RegionSpace&>(theAllocator_);
    space.ClearAllGCInfo();
}

void WCollector::CollectSmallSpace()
{
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::CollectSmallSpace", "");
    GCStats& stats = GetGCStats();
    RegionSpace& space = reinterpret_cast<RegionSpace&>(theAllocator_);
    {
        COMMON_PHASE_TIMER("CollectFromSpaceGarbage");
        stats.collectedBytes += stats.smallGarbageSize;
        if (gcReason_ == GC_REASON_APPSPAWN) {
            VLOG(DEBUG, "APPSPAWN GC Collect");
            space.CollectAppSpawnSpaceGarbage();
        } else {
            space.CollectFromSpaceGarbage();
            space.HandlePromotion();
        }
    }

    size_t candidateBytes = stats.fromSpaceSize + stats.pinnedSpaceSize + stats.largeSpaceSize;
    stats.garbageRatio = (candidateBytes > 0) ? static_cast<float>(stats.collectedBytes) / candidateBytes : 0;

    stats.liveBytesAfterGC = space.GetAllocatedBytes();

    VLOG(INFO,
         "collect %zu B: old small %zu - %zu B, old pinned %zu - %zu B, old large %zu - %zu B. garbage ratio %.2f%%",
         stats.collectedBytes, stats.fromSpaceSize, stats.smallGarbageSize, stats.pinnedSpaceSize,
         stats.pinnedGarbageSize, stats.largeSpaceSize, stats.largeGarbageSize,
         stats.garbageRatio * 100); // The base of the percentage is 100
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::CollectSmallSpace END", (
                    "collect:" + std::to_string(stats.collectedBytes) +
                    "B;old small:" + std::to_string(stats.fromSpaceSize) +
                    "-" + std::to_string(stats.smallGarbageSize) +
                    "B;old pinned:" + std::to_string(stats.pinnedSpaceSize) +
                    "-" + std::to_string(stats.pinnedGarbageSize) +
                    "B;old large:" + std::to_string(stats.largeSpaceSize) +
                    "-" + std::to_string(stats.largeGarbageSize) +
                    "B;garbage ratio:" + std::to_string(stats.garbageRatio)
                ).c_str());

    collectorResources_.GetFinalizerProcessor().NotifyToReclaimGarbage();
}

bool WCollector::ShouldIgnoreRequest(GCRequest& request) { return request.ShouldBeIgnored(); }
} // namespace common
