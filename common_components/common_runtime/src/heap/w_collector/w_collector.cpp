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
#include "common_components/common_runtime/src/heap/w_collector/w_collector.h"

#include "common_components/common_runtime/src/mutator/mutator_manager.h"
#include "common_interfaces/heap/heap_visitor.h"
#include "common_components/log/log.h"
#include "common_interfaces/objects/ref_field.h"
#include "verification.h"

namespace panda {
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

bool WCollector::MarkObject(BaseObject* obj) const
{
    bool marked = RegionSpace::MarkObject(obj);
    if (!marked) {
        reinterpret_cast<RegionSpace&>(theAllocator_).CountLiveObject(obj);
        RegionDesc* region = RegionDesc::GetRegionDescAt(reinterpret_cast<HeapAddress>(obj));
        (void)region;

        if (region->IsGarbageRegion()) {
            LOG_COMMON(FATAL) << "Unresolved fatal";
            UNREACHABLE_CC();
        }

        DLOG(TRACE, "mark obj %p<%p>(%zu) in region %p(%u)@%#zx, live %u", obj, obj->GetTypeInfo(), obj->GetSize(),
             region, region->GetRegionType(), region->GetRegionStart(), region->GetLiveByteCount());
    }
    return marked;
}

bool WCollector::ResurrectObject(BaseObject* obj)
{
    bool resurrected = RegionSpace::ResurrentObject(obj);
    if (!resurrected) {
        reinterpret_cast<RegionSpace&>(theAllocator_).CountLiveObject(obj);
        RegionDesc* region = RegionDesc::GetRegionDescAt(reinterpret_cast<HeapAddress>(obj));
        (void)region;
        DLOG(TRACE, "resurrect region %p@%#zx obj %p<%p>(%zu), live bytes %u", region, region->GetRegionStart(), obj,
             obj->GetTypeInfo(), obj->GetSize(), region->GetLiveByteCount());
    }
    return resurrected;
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
    auto obj = field.GetTargetObject();
    ASSERT_LOGF(Heap::IsTaggedObject(value), "EnumRefFieldRoot failed: Invalid root");

    // need fix or clean
    rootSet.push_back(field.GetTargetObject());
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

void WCollector::EnumAndTagRawRoot(ObjectRef& ref, RootSet& rootSet) const
{
    RefField<>& refField = reinterpret_cast<RefField<>&>(ref);
    RefField<> oldField(refField);
    LOGF_CHECK(!IsOldPointer(oldField)) << "EnumAndTagRawRoot failed: Invalid root: " << oldField.GetFieldValue();
    if (IsCurrentPointer(oldField)) {
        rootSet.push_back(oldField.GetTargetObject());
        return;
    }
    BaseObject* root = oldField.GetTargetObject();
    if (Heap::IsHeapAddress(root)) {
        CHECK_CC(root->IsValidObject());
        RefField<> newField = GetAndTryTagRefField(root);
        if (oldField.GetFieldValue() == newField.GetFieldValue()) {
            DLOG(ENUM, "enum raw root @%p: %p(%zu)", &ref, root, root->GetSize());
        } else if (refField.CompareExchange(oldField.GetFieldValue(), newField.GetFieldValue())) {
            DLOG(ENUM, "enum static ref@%p: %#zx=>%#zx -> %p<%p>(%zu)", &refField, oldField.GetFieldValue(),
                 newField.GetFieldValue(), root, root->GetTypeInfo(), root->GetSize());
        } else {
            DLOG(ENUM, "enum static ref@%p: %#zx -> %p<%p>(%zu)", &refField, oldField.GetFieldValue(), root,
                 root->GetTypeInfo(), root->GetSize());
        }
        rootSet.push_back(root);
    }
}

// note each ref-field will not be traced twice, so each old pointer the tracer meets must come from previous gc.
void WCollector::TraceRefField(BaseObject* obj, RefField<>& field, WorkStack& workStack, WeakStack& weakStack) const
{
    if (!Heap::IsTaggedObject(field.GetFieldValue())) {
        return;
    }
    BaseObject* targetObj = field.GetTargetObject();
    // field is tagged object, should be in heap
    DCHECK_CC(Heap::IsHeapAddress(targetObj));

    DLOG(TRACE, "trace obj %p ref@%p: %p<%p>(%zu)", obj, &field, targetObj, targetObj->GetTypeInfo(),
         targetObj->GetSize());
    if (IsMarkedObject(targetObj)) {
        return;
    }
    if (RegionSpace::IsNewObjectSinceTrace(targetObj)) {
        DLOG(TRACE, "trace: skip new obj %p<%p>(%zu)", targetObj, targetObj->GetTypeInfo(), targetObj->GetSize());
        return;
    }
    if (field.IsWeak()) {
        weakStack.push_back(&field);
        return;
    }
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

    if (!Heap::IsTaggedObject(field.GetFieldValue())) {
        return;
    }

    // target object could be null or non-heap for some static variable.
    if (!Heap::IsHeapAddress(targetObj)) {
        return;
    }

    BaseObject* latest = FindToVersion(targetObj);
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
        RefField<> newField(toVersion);
        // CAS failure means some mutator or gc thread writes a new ref (must be a to-object), no need to retry.
        if (refField.CompareExchange(oldField.GetFieldValue(), newField.GetFieldValue())) {
            DLOG(FIX, "fix raw-ref @%p: %p -> %p", &root, oldObj, toVersion);
            return toVersion;
        }
    }

    return oldObj;
}

using ForwardUpdateRawRefHookType = void (*)(WCollector *collector, uint64_t *obj);
using ForwardUpdateWeakRawRefHookType = bool (*)(WCollector *collector, uint64_t *obj);
using PreforwardStaticRootsHookType =
    void (*)(WCollector *collector, ForwardUpdateRawRefHookType hook, ForwardUpdateWeakRawRefHookType weakHook);
PreforwardStaticRootsHookType g_preforwardStaticRootsHook = nullptr;

extern "C" PUBLIC_API void RegisterPreforwardStaticRootsHook(PreforwardStaticRootsHookType hook) {
    g_preforwardStaticRootsHook = hook;
}

void WCollector::PreforwardStaticRoots()
{
    panda::RefFieldVisitor visitor = [this](RefField<>& refField) {
        RefField<> oldField(refField);
        BaseObject* oldObj = oldField.GetTargetObject();
        DLOG(FIX, "visit raw-ref @%p: %p", &refField, oldObj);
        if (IsFromObject(oldObj)) {
            BaseObject* toVersion = TryForwardObject(oldObj);
            CHECK_CC(toVersion != nullptr);
            RefField<> newField(toVersion);
            // CAS failure means some mutator or gc thread writes a new ref (must be a to-object), no need to retry.
            if (refField.CompareExchange(oldField.GetFieldValue(), newField.GetFieldValue())) {
                DLOG(FIX, "fix raw-ref @%p: %p -> %p", &refField, oldObj, toVersion);
            }
        }
    };

    panda::WeakRefFieldVisitor weakVisitor = [this](RefField<> &refField) -> bool {
        RefField<> oldField(refField);
        BaseObject *oldObj = oldField.GetTargetObject();
        if (!IsMarkedObject(oldObj) && !RegionSpace::IsNewObjectSinceTrace(oldObj)) {
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

    VisitRoots(visitor, false);
    VisitWeakRoots(weakVisitor);

    AllocationBuffer* allocBuffer = AllocationBuffer::GetAllocBuffer();
    if (LIKELY_CC(allocBuffer != nullptr)) {
        allocBuffer->ClearRegion();
    }
}

void WCollector::PreforwardFinalizerProcessorRoots()
{
    RootVisitor visitor = [this](ObjectRef& root) { ForwardUpdateRawRef(root); };
    collectorResources_.GetFinalizerProcessor().VisitRawPointers(visitor);
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

    ARK_COMMON_PHASE_TIMER("enum roots & update old pointers within");
    TransitionToGCPhase(GCPhase::GC_PHASE_ENUM, true);
    EnumerateAllRoots(workStack);
}

void WCollector::TraceHeap(WorkStack& workStack)
{
    ARK_COMMON_PHASE_TIMER("trace live objects");
    markedObjectCount_.store(0, std::memory_order_relaxed);
    TransitionToGCPhase(GCPhase::GC_PHASE_MARK, true);

    TraceRoots(workStack);
    ProcessFinalizers();
}

void WCollector::PostTrace()
{
    ARK_COMMON_PHASE_TIMER("PostTrace");
    TransitionToGCPhase(GC_PHASE_POST_MARK, true);

    // clear satb buffer when gc finish tracing.
    SatbBuffer::Instance().ClearBuffer();
    ExemptFromSpace();

    WVerify::VerifyAfterMark(*this);
}
void WCollector::Preforward()
{
    OHOS_HITRACE("ARK_RT_GC_PREFORWARD");
    ARK_COMMON_PHASE_TIMER("Preforward");
    TransitionToGCPhase(GCPhase::GC_PHASE_PRECOPY, true);

    GCThreadPool* threadPool = GetThreadPool();
    ASSERT_LOGF(threadPool != nullptr, "thread pool is null");

    // copy and fix finalizer roots.
    threadPool->AddWork(new (std::nothrow) LambdaWork([this](size_t) { PreforwardStaticRoots(); }));

    threadPool->Start();
    threadPool->WaitFinish();
}

void WCollector::FixHeap()
{
    WVerify::VerifyAfterForward(*this);

    ARK_COMMON_PHASE_TIMER("FixHeap");
    TransitionToGCPhase(GCPhase::GC_PHASE_FIX, true);
    reinterpret_cast<RegionSpace&>(theAllocator_).FixHeap();

    WVerify::VerifyAfterFix(*this);
}

void WCollector::DoGarbageCollection()
{
    if (shouldSTW_ == 2) { // 2: stw-gc
        ScopedStopTheWorld stw("stw-gc");
        WorkStack workStack = NewWorkStack();
        EnumRoots(workStack);
        TraceHeap(workStack);
        PostTrace();

        Preforward();
        // reclaim large objects should after preforward(may process weak ref) and
        // before fix heap(may clear live bit)
        CollectLargeGarbage();

        CopyFromSpace();
        FixHeap();

        TransitionToGCPhase(GCPhase::GC_PHASE_IDLE, true);
        ClearAllGCInfo();
        CollectSmallSpace();

        return;
    } else if (shouldSTW_ == 1) {
        WorkStack workStack = NewWorkStack();
        {
            ScopedStopTheWorld stw("wgc-enumroot");
            EnumRoots(workStack);
        }
        TraceHeap(workStack);
        PostTrace();

        ScopedStopTheWorld stw("wgc-preforward");
        reinterpret_cast<RegionSpace&>(theAllocator_).PrepareForward();
        Preforward();
        // reclaim large objects should after preforward(may process weak ref) and
        // before fix heap(may clear live bit)
        CollectLargeGarbage();

        CopyFromSpace();
        FixHeap();

        TransitionToGCPhase(GCPhase::GC_PHASE_IDLE, true);
        ClearAllGCInfo();
        CollectSmallSpace();
        return;
    }

    WorkStack workStack = NewWorkStack();
    {
        ScopedStopTheWorld stw("wgc-enumroot");
        EnumRoots(workStack);
    }
    TraceHeap(workStack);
    PostTrace();

    {
        ScopedStopTheWorld stw("wgc-preforward");
        reinterpret_cast<RegionSpace&>(theAllocator_).PrepareForward();
        Preforward();
    }
    // reclaim large objects should after preforward(may process weak ref)
    // and before fix heap(may clear live bit)
    CollectLargeGarbage();

    CopyFromSpace();
    FixHeap();

    TransitionToGCPhase(GCPhase::GC_PHASE_IDLE, true);
    ClearAllGCInfo();
    CollectSmallSpace();
}

void WCollector::MarkNewObject(BaseObject* obj)
{
    GCPhase mutatorPhase = Mutator::GetMutator()->GetMutatorPhase();
    if (UNLIKELY_CC(mutatorPhase == GCPhase::GC_PHASE_ENUM) || UNLIKELY_CC(mutatorPhase == GCPhase::GC_PHASE_MARK) ||
        UNLIKELY_CC(mutatorPhase == GCPhase::GC_PHASE_REMARK_SATB)) {
        MarkObject(obj);
    }
}

void WCollector::ProcessWeakReferences()
{
    while (!globalWeakStack_.empty()) {
        RefField<>& field = reinterpret_cast<RefField<>&>(*globalWeakStack_.back());
        globalWeakStack_.pop_back();
        RefField<> oldField(field);
        BaseObject* targetObj = oldField.GetTargetObject();
        if (!Heap::IsHeapAddress(targetObj) || IsMarkedObject(targetObj) ||
            RegionSpace::IsNewObjectSinceTrace(targetObj)) {
            continue;
        }
        if (field.ClearRef(oldField.GetFieldValue())) {
            // fix log
            // DLOG(FIX, "fix weak obj %p+%zu ref@%p: %#zx => %p<%p>(%zu)", obj, obj->GetSize(), &field,
            //     oldField.GetFieldValue(), latest, latest->GetTypeInfo(), latest->GetSize())
        }
    }
#ifndef ARK_USE_SATB_BARRIER
    panda::WeakRefFieldVisitor weakVisitor = [this](RefField<> &refField) -> bool {
        RefField<> oldField(refField);
        BaseObject *oldObj = oldField.GetTargetObject();
        if (!IsMarkedObject(oldObj) && !RegionSpace::IsNewObjectSinceTrace(oldObj)) {
            return false;
        }
        return true;
    };
    VisitWeakRoots(weakVisitor);
#endif
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
                 GetGCPhase() == GCPhase::GC_PHASE_FIX);
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
        if (obj->TryLockObject(oldWord)) {
            return CopyObjectExclusive(obj);
        }
    } while (true);
    LOG_COMMON(FATAL) << "forwardObject exit in wrong path";
    UNREACHABLE_CC();
    return nullptr;
}

BaseObject* WCollector::CopyObjectExclusive(BaseObject* obj)
{
    size_t size = RegionSpace::GetAllocSize(*obj);
    BaseObject* toObj = fwdTable_.RouteObject(obj, size);
    if (toObj == nullptr) {
        ASSERT_LOGF(0, "OOM");
        // ConcurrentGC
        obj->UnlockObject(panda::BaseStateWord::ForwardState::NORMAL);
        return toObj;
    }
    DLOG(COPY, "copy obj %p<%p>(%zu) to %p", obj, obj->GetTypeInfo(), size, toObj);
    CopyObject(*obj, *toObj, size);
    if (IsToObject(toObj)) {
        toObj->SetForwardState(panda::BaseStateWord::ForwardState::NORMAL);
    } else {
        // if this object is not in to-space, we label it as to-object explicitly.
        toObj->SetForwardState(panda::BaseStateWord::ForwardState::TO_VERSION);
    }
    std::atomic_thread_fence(std::memory_order_release);
    obj->SetSizeForwarded(size);
    obj->SetForwardingPointerExclusive(toObj);
    return toObj;
}

void WCollector::ClearAllGCInfo()
{
    ARK_COMMON_PHASE_TIMER("ClearAllGCInfo");
    RegionSpace& space = reinterpret_cast<RegionSpace&>(theAllocator_);
    space.ClearAllGCInfo();
}

void WCollector::CollectSmallSpace()
{
    GCStats& stats = GetGCStats();
    RegionSpace& space = reinterpret_cast<RegionSpace&>(theAllocator_);
    {
        ARK_COMMON_PHASE_TIMER("CollectFromSpaceGarbage");
        stats.collectedBytes += stats.smallGarbageSize;
        space.CollectFromSpaceGarbage();
    }

    size_t candidateBytes = stats.fromSpaceSize + stats.pinnedSpaceSize + stats.largeSpaceSize;
    stats.garbageRatio = (candidateBytes > 0) ? static_cast<float>(stats.collectedBytes) / candidateBytes : 0;

    stats.liveBytesAfterGC = space.GetAllocatedBytes();

    VLOG(REPORT,
         "collect %zu B: old small %zu - %zu B, old pinned %zu - %zu B, old large %zu - %zu B. garbage ratio %.2f%%",
         stats.collectedBytes, stats.fromSpaceSize, stats.smallGarbageSize, stats.pinnedSpaceSize,
         stats.pinnedGarbageSize, stats.largeSpaceSize, stats.largeGarbageSize,
         stats.garbageRatio * 100); // The base of the percentage is 100

    VLOG(REPORT, "start to release heap garbage memory");
    collectorResources_.GetFinalizerProcessor().NotifyToReclaimGarbage();
}

bool WCollector::ShouldIgnoreRequest(GCRequest& request) { return request.ShouldBeIgnored(); }
} // namespace panda
