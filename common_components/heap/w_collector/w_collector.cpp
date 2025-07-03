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

#ifdef ENABLE_RSS
#include "res_sched_client.h"
#endif
#ifdef ENABLE_QOS
#include "qos.h"
#endif

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
        DLOG(TRACE, "mark obj %p<%p>(%zu) in region %p(%u)@%#zx, live %u", obj, obj->GetTypeInfo(), size,
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
    if (IsFromObject(fromObj)) { //LCOV_EXCL_BR_LINE
        if (copy) { //LCOV_EXCL_BR_LINE
            toObj = const_cast<WCollector*>(this)->TryForwardObject(fromObj);
            if (toObj != nullptr) { //LCOV_EXCL_BR_LINE
                HeapProfilerListener::GetInstance().OnMoveEvent(reinterpret_cast<uintptr_t>(fromObj),
                                                                reinterpret_cast<uintptr_t>(toObj),
                                                                toObj->GetSize());
            }
        } else { //LCOV_EXCL_BR_LINE
            toObj = FindToVersion(fromObj);
        }
        if (toObj == nullptr) { //LCOV_EXCL_BR_LINE
            return false;
        }
        RefField<> tmpField(toObj, oldRef.IsWeak());
        if (field.CompareExchange(oldRef.GetFieldValue(), tmpField.GetFieldValue())) {
            if (obj != nullptr) {
                DLOG(TRACE, "update obj %p<%p>(%zu)+%zu ref-field@%p: %#zx -> %#zx", obj, obj->GetTypeInfo(),
                    obj->GetSize(), BaseObject::FieldOffset(obj, &field), &field, oldRef.GetFieldValue(),
                    tmpField.GetFieldValue());
            } else { //LCOV_EXCL_BR_LINE
                DLOG(TRACE, "update ref@%p: 0x%zx -> %p", &field, oldRef.GetFieldValue(), toObj);
            }
            return true;
        } else { //LCOV_EXCL_BR_LINE
            if (obj != nullptr) { //LCOV_EXCL_BR_LINE
                DLOG(TRACE,
                    "update obj %p<%p>(%zu)+%zu but cas failed ref-field@%p: %#zx(%#zx) -> %#zx but cas failed ",
                     obj, obj->GetTypeInfo(), obj->GetSize(), BaseObject::FieldOffset(obj, &field), &field,
                     oldRef.GetFieldValue(), field.GetFieldValue(), tmpField.GetFieldValue());
            } else { //LCOV_EXCL_BR_LINE
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
    for (;;) { //LCOV_EXCL_BR_LINE
        RefField<> oldRef(field);
        if (oldRef.IsTagged()) { //LCOV_EXCL_BR_LINE
            target = oldRef.GetTargetObject();
            RefField<> newRef(target);
            if (field.CompareExchange(oldRef.GetFieldValue(), newRef.GetFieldValue())) { //LCOV_EXCL_BR_LINE
                if (obj != nullptr) {
                    DLOG(FIX, "untag obj %p<%p>(%zu) ref-field@%p: %#zx -> %#zx", obj, obj->GetTypeInfo(),
                         obj->GetSize(), &field, oldRef.GetFieldValue(), newRef.GetFieldValue());
                } else { //LCOV_EXCL_BR_LINE
                    DLOG(FIX, "untag ref@%p: %#zx -> %#zx", &field, oldRef.GetFieldValue(), newRef.GetFieldValue());
                }

                return true;
            }
        } else { //LCOV_EXCL_BR_LINE
            return false;
        }
    }

    return false;
}

static void TraceRefField(BaseObject *obj, BaseObject *targetObj, RefField<> &field,
                          WorkStack &workStack, RegionDesc *targetRegion);
// note each ref-field will not be traced twice, so each old pointer the tracer meets must come from previous gc.
static void TraceRefField(BaseObject *obj, RefField<> &field, WorkStack &workStack,
                          WeakStack &weakStack, const GCReason gcReason)
{
    RefField<> oldField(field);
    BaseObject* targetObj = oldField.GetTargetObject();

    if (!Heap::IsTaggedObject(oldField.GetFieldValue())) {
        return;
    }
    // field is tagged object, should be in heap
    DCHECK_CC(Heap::IsHeapAddress(targetObj));

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
    common::TraceRefField(obj, targetObj, field, workStack, targetRegion);
}

// note each ref-field will not be traced twice, so each old pointer the tracer meets must come from previous gc.
static void TraceRefField(BaseObject *obj, BaseObject *targetObj, RefField<> &field,
                          WorkStack &workStack, RegionDesc *targetRegion)
{
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

TraceCollector::TraceRefFieldVisitor WCollector::CreateTraceObjectRefFieldsVisitor(WorkStack *workStack,
                                                                                   WeakStack *weakStack)
{
    TraceRefFieldVisitor visitor;

    if (gcReason_ == GCReason::GC_REASON_YOUNG) {
        visitor.SetVisitor([obj = visitor.GetClosure(), workStack, weakStack](RefField<> &field) {
            const GCReason gcReason = GCReason::GC_REASON_YOUNG;
            TraceRefField(*obj, field, *workStack, *weakStack, gcReason);
        });
    } else {
        visitor.SetVisitor([obj = visitor.GetClosure(), workStack, weakStack](RefField<> &field) {
            const GCReason gcReason = GCReason::GC_REASON_HEU;
            TraceRefField(*obj, field, *workStack, *weakStack, gcReason);
        });
    }
    return visitor;
}

void WCollector::TraceObjectRefFields(BaseObject *obj, TraceRefFieldVisitor *data)
{
    data->SetTraceRefFieldArgs(obj);
    obj->ForEachRefField(data->GetRefFieldVisitor());
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

class RemarkAndPreforwardVisitor {
public:
    RemarkAndPreforwardVisitor(WorkStack &localStack, WCollector *collector)
        : localStack_(localStack), collector_(collector) {}

    void operator()(RefField<> &refField)
    {
        RefField<> oldField(refField);
        BaseObject* oldObj = oldField.GetTargetObject();
        DLOG(FIX, "visit raw-ref @%p: %p", &refField, oldObj);
        if (collector_->IsFromObject(oldObj)) {
            BaseObject* toVersion = collector_->TryForwardObject(oldObj);
            if (toVersion == nullptr) {
                Heap::throwOOM();
                return;
            }
            HeapProfilerListener::GetInstance().OnMoveEvent(reinterpret_cast<uintptr_t>(oldObj),
                                                            reinterpret_cast<uintptr_t>(toVersion),
                                                            toVersion->GetSize());
            RefField<> newField(toVersion);
            // CAS failure means some mutator or gc thread writes a new ref (must be a to-object), no need to retry.
            if (refField.CompareExchange(oldField.GetFieldValue(), newField.GetFieldValue())) {
                DLOG(FIX, "fix raw-ref @%p: %p -> %p", &refField, oldObj, toVersion);
            }
            MarkToObject(oldObj, toVersion);
        } else {
            MarkObject(oldObj);
        }
    }

private:
    void MarkObject(BaseObject *object)
    {
        if (!collector_->MarkObject(object)) {
            localStack_.push_back(object);
        }
    }

    void MarkToObject(BaseObject *oldVersion, BaseObject *toVersion)
    {
        if (!collector_->MarkObject(toVersion)) {
            // Therefore, we must still attempt to mark the old object to prevent
            // it from being pushed into the mark stack during subsequent
            // traversals.
            collector_->MarkObject(oldVersion);
            // The reference in toSpace needs to be fixed up. Therefore, even if
            // the oldVersion has been marked, it must still be pushed into the
            // stack. This will be optimized later.
            localStack_.push_back(toVersion);
        }
    }

private:
    WorkStack &localStack_;
    WCollector *collector_;
};

class RemarkingAndPreforwardTask : public common::Task {
public:
    RemarkingAndPreforwardTask(WCollector *collector, WorkStack &localStack, TaskPackMonitor &monitor,
                               std::function<Mutator*()>& next)
        : Task(0), visitor_(localStack, collector), monitor_(monitor), getNextMutator_(next)
    {}

    bool Run([[maybe_unused]] uint32_t threadIndex) override
    {
        ThreadLocal::SetThreadType(ThreadType::GC_THREAD);
        Mutator *mutator = getNextMutator_();
        while (mutator != nullptr) {
            VisitMutatorRoot(visitor_, *mutator);
            mutator = getNextMutator_();
        }
        ThreadLocal::SetThreadType(ThreadType::ARK_PROCESSOR);
        ThreadLocal::ClearAllocBufferRegion();
        monitor_.NotifyFinishOne();
        return true;
    }

private:
    RemarkAndPreforwardVisitor visitor_;
    TaskPackMonitor &monitor_;
    std::function<Mutator*()> &getNextMutator_;
};

void WCollector::ParallelRemarkAndPreforward(WorkStack& workStack)
{
    std::vector<Mutator*> taskList;
    MutatorManager &mutatorManager = MutatorManager::Instance();
    mutatorManager.VisitAllMutators([&taskList](Mutator &mutator) {
        taskList.push_back(&mutator);
    });
    std::atomic<int> taskIter = 0;
    std::function<Mutator*()> getNextMutator = [&taskIter, &taskList]() -> Mutator* {
        uint32_t idx = static_cast<uint32_t>(taskIter.fetch_add(1U, std::memory_order_relaxed));
        if (idx < taskList.size()) {
            return taskList[idx];
        }
        return nullptr;
    };

    const uint32_t runningWorkers = std::min<uint32_t>(GetGCThreadCount(true), taskList.size());
    uint32_t parallelCount = runningWorkers + 1; // 1 ：DaemonThread
    TaskPackMonitor monitor(runningWorkers, runningWorkers);
    WorkStack localStack[parallelCount];
    for (uint32_t i = 1; i < parallelCount; ++i) {
        GetThreadPool()->PostTask(std::make_unique<RemarkingAndPreforwardTask>(this, localStack[i], monitor,
                                                                               getNextMutator));
    }
    // Run in daemon thread.
    RemarkAndPreforwardVisitor visitor(localStack[0], this);
    VisitGlobalRoots(visitor);
    Mutator *mutator = getNextMutator();
    while (mutator != nullptr) {
        VisitMutatorRoot(visitor, *mutator);
        mutator = getNextMutator();
    }
    monitor.WaitAllFinished();
    for (uint32_t i = 0; i < parallelCount; ++i) {
        workStack.insert(localStack[i]);
    }
}

void WCollector::RemarkAndPreforwardStaticRoots(WorkStack& workStack)
{
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::RemarkAndPreforwardStaticRoots", "");
    const uint32_t maxWorkers = GetGCThreadCount(true) - 1;
    if (maxWorkers > 0) {
        ParallelRemarkAndPreforward(workStack);
    } else {
        RemarkAndPreforwardVisitor visitor(workStack, this);
        VisitSTWRoots(visitor);
    }
}

void WCollector::PreforwardConcurrentRoots()
{
    RefFieldVisitor visitor = [this](RefField<> &refField) {
        RefField<> oldField(refField);
        BaseObject *oldObj = oldField.GetTargetObject();
        DLOG(FIX, "visit raw-ref @%p: %p", &refField, oldObj);
        if (IsFromObject(oldObj)) {
            BaseObject *toVersion = TryForwardObject(oldObj);
            ASSERT_LOGF(toVersion != nullptr, "TryForwardObject failed");
            HeapProfilerListener::GetInstance().OnMoveEvent(reinterpret_cast<uintptr_t>(oldObj),
                                                            reinterpret_cast<uintptr_t>(toVersion),
                                                            toVersion->GetSize());
            RefField<> newField(toVersion);
            // CAS failure means some mutator or gc thread writes a new ref (must be a to-object), no need to retry.
            if (refField.CompareExchange(oldField.GetFieldValue(), newField.GetFieldValue())) {
                DLOG(FIX, "fix raw-ref @%p: %p -> %p", &refField, oldObj, toVersion);
            }
        }
    };
    VisitConcurrentRoots(visitor);
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
        allocBuffer->ClearRegions();
    }
}

void WCollector::PreforwardConcurrencyModelRoots()
{
    LOG_COMMON(FATAL) << "Unresolved fatal";
    UNREACHABLE_CC();
}

class EnumRootsBuffer {
public:
    EnumRootsBuffer();
    void UpdateBufferSize();
    CArrayList<BaseObject *> *GetBuffer() { return &buffer_; }

private:
    static size_t bufferSize_;
    CArrayList<BaseObject *> buffer_;
};

size_t EnumRootsBuffer::bufferSize_ = 16;
EnumRootsBuffer::EnumRootsBuffer() : buffer_(bufferSize_)
{
    buffer_.clear();  // memset to zero and allocated real memory
}

void EnumRootsBuffer::UpdateBufferSize()
{
    if (buffer_.empty()) {
        return;
    }
    const size_t decreaseBufferThreshold = bufferSize_ >> 2;
    if (buffer_.size() < decreaseBufferThreshold) {
        bufferSize_ = bufferSize_ >> 1;
    } else {
        bufferSize_ = std::max(buffer_.capacity(), bufferSize_);
    }
    if (buffer_.capacity() > UINT16_MAX) {
        LOG_COMMON(INFO) << "too many roots, allocated buffer too large: " << buffer_.size() << ", allocate "
                         << (static_cast<double>(buffer_.capacity()) / MB);
    }
}

template <WCollector::EnumRootsPolicy policy>
CArrayList<BaseObject *> WCollector::EnumRoots()
{
    STWParam stwParam{"wgc-enumroot"};
    EnumRootsBuffer buffer;
    CArrayList<common::BaseObject *> *results = buffer.GetBuffer();
    common::RefFieldVisitor visitor = [&results](RefField<> &filed) { results->push_back(filed.GetTargetObject()); };

    if constexpr (policy == EnumRootsPolicy::NO_STW_AND_NO_FLIP_MUTATOR) {
        EnumRootsImpl<VisitRoots>(visitor);
    } else if constexpr (policy == EnumRootsPolicy::STW_AND_NO_FLIP_MUTATOR) {
        ScopedStopTheWorld stw(stwParam);
        OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL,
                     ("CMCGC::EnumRoots-STW-bufferSize(" + std::to_string(results->capacity()) + ")").c_str(), "");
        EnumRootsImpl<VisitRoots>(visitor);
    } else if constexpr (policy == EnumRootsPolicy::STW_AND_FLIP_MUTATOR) {
        auto rootSet = EnumRootsFlip(stwParam, visitor);
        for (const auto &roots : rootSet) {
            std::copy(roots.begin(), roots.end(), std::back_inserter(*results));
        }
        VisitConcurrentRoots(visitor);
    }
    buffer.UpdateBufferSize();
    GetGCStats().recordSTWTime(stwParam.GetElapsedNs());
    return std::move(*results);
}

void WCollector::TraceHeap(const CArrayList<BaseObject *> &collectedRoots)
{
    COMMON_PHASE_TIMER("trace live objects");
    markedObjectCount_.store(0, std::memory_order_relaxed);
    TransitionToGCPhase(GCPhase::GC_PHASE_MARK, true);

    TraceRoots(collectedRoots);
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
        if (gcReason_ == GC_REASON_YOUNG) {
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

RefFieldVisitor WCollector::GetPrefowardRefFieldVisitor()
{
    return [this](RefField<> &refField) -> void {
        RefField<> oldField(refField);
        BaseObject *oldObj = oldField.GetTargetObject();
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
                DLOG(FIX, "fix raw-ref @%p: %p -> %p", &refField, oldObj, toVersion);
            }
        }
    };
}

void WCollector::PreforwardFlip()
{
    auto remarkAndForwardGlobalRoot = [this]() {
        OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::PreforwardFlip[STW]", "");
        SetGCThreadQosPriority(common::PriorityMode::STW);
        ASSERT_LOGF(GetThreadPool() != nullptr, "thread pool is null");
        TransitionToGCPhase(GCPhase::GC_PHASE_FINAL_MARK, true);
        Remark();
        PostTrace();
        reinterpret_cast<RegionSpace&>(theAllocator_).PrepareForward();

        TransitionToGCPhase(GCPhase::GC_PHASE_PRECOPY, true);
        WeakRefFieldVisitor weakVisitor = GetWeakRefFieldVisitor();
        SetGCThreadQosPriority(common::PriorityMode::FOREGROUND);

        if (Heap::GetHeap().GetGCReason() == GC_REASON_YOUNG) {
            // only visit weak roots that may reference young objects
            VisitDynamicWeakGlobalRoots(weakVisitor);
        } else {
            VisitDynamicWeakGlobalRoots(weakVisitor);
            VisitDynamicWeakGlobalRootsOld(weakVisitor);
        }
    };
    FlipFunction forwardMutatorRoot = [this](Mutator &mutator) {
        WeakRefFieldVisitor weakVisitor = GetWeakRefFieldVisitor();
        VisitWeakMutatorRoot(weakVisitor, mutator);
        RefFieldVisitor visitor = GetPrefowardRefFieldVisitor();
        VisitMutatorPreforwardRoot(visitor, mutator);
        // Request finalize callback in each vm-thread when gc finished.
        mutator.SetFinalizeRequest();
    };
    STWParam stwParam{"final-mark"};
    MutatorManager::Instance().FlipMutators(stwParam, remarkAndForwardGlobalRoot, &forwardMutatorRoot);
    GetGCStats().recordSTWTime(stwParam.GetElapsedNs());
    AllocationBuffer* allocBuffer = AllocationBuffer::GetAllocBuffer();
    if (LIKELY_CC(allocBuffer != nullptr)) {
        allocBuffer->ClearRegions();
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
    RefFieldVisitor visitor = GetPrefowardRefFieldVisitor();
    VisitPreforwardRoots(visitor);
}

void WCollector::ConcurrentPreforward()
{
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::ConcurrentPreforward", "");
    PreforwardConcurrentRoots();
    ProcessStringTable();
}

void WCollector::PrepareFix()
{
    if (Heap::GetHeap().GetGCReason() == GCReason::GC_REASON_YOUNG) {
        // string table objects are always not in young space, skip it
        return;
    }

    COMMON_PHASE_TIMER("PrepareFix");

    // we cannot re-enter STW, check it first
    if (!MutatorManager::Instance().WorldStopped()) {
        STWParam prepareFixStwParam{"wgc-preparefix"};
        ScopedStopTheWorld stw(prepareFixStwParam);
        OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::PrepareFix[STW]", "");

#ifndef GC_STW_STRINGTABLE
        auto *baseRuntime = BaseRuntime::GetInstance();
        auto& stringTable = reinterpret_cast<BaseStringTableImpl&>(baseRuntime->GetStringTable());
        stringTable.GetInternalTable()->GetCleaner()->CleanUp();
#endif

        GetGCStats().recordSTWTime(prepareFixStwParam.GetElapsedNs());
    } else {
#ifndef GC_STW_STRINGTABLE
        auto *baseRuntime = BaseRuntime::GetInstance();
        auto& stringTable = reinterpret_cast<BaseStringTableImpl&>(baseRuntime->GetStringTable());
        stringTable.GetInternalTable()->GetCleaner()->CleanUp();
#endif
    }
}

void WCollector::FixHeap()
{
    TransitionToGCPhase(GCPhase::GC_PHASE_FIX, true);
    COMMON_PHASE_TIMER("FixHeap");
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::FixHeap", "");
    reinterpret_cast<RegionSpace&>(theAllocator_).FixHeap();

    WVerify::VerifyAfterFix(*this);
}

void WCollector::DoGarbageCollection()
{
    const bool isNotYoungGC = gcReason_ != GCReason::GC_REASON_YOUNG;
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::DoGarbageCollection", "");
    if (gcMode_ == GCMode::STW) { // 2: stw-gc
#ifdef ENABLE_CMC_RB_DFX
        WVerify::DisableReadBarrierDFX(*this);
#endif
        STWParam stwParam{"stw-gc"};
    {
        ScopedStopTheWorld stw(stwParam);
        auto collectedRoots = EnumRoots<EnumRootsPolicy::NO_STW_AND_NO_FLIP_MUTATOR>();
        TraceHeap(collectedRoots);
        TransitionToGCPhase(GCPhase::GC_PHASE_FINAL_MARK, true);
        Remark();
        PostTrace();

        Preforward();
        ConcurrentPreforward();
        // reclaim large objects should after preforward(may process weak ref) and
        // before fix heap(may clear live bit)
        if (isNotYoungGC) {
            CollectLargeGarbage();
        }
        SweepThreadLocalJitFort();

        CopyFromSpace();
        WVerify::VerifyAfterForward(*this);

        PrepareFix();
        FixHeap();
        if (isNotYoungGC) {
            CollectPinnedGarbage();
        }

        TransitionToGCPhase(GCPhase::GC_PHASE_IDLE, true);

        ClearAllGCInfo();
        CollectSmallSpace();

#if defined(ENABLE_CMC_RB_DFX)
        WVerify::EnableReadBarrierDFX(*this);
#endif
    }
        GetGCStats().recordSTWTime(stwParam.GetElapsedNs());
        return;
    } else if (gcMode_ == GCMode::CONCURRENT_MARK) { // 1: concurrent-mark
        auto collectedRoots = EnumRoots<EnumRootsPolicy::STW_AND_NO_FLIP_MUTATOR>();
        TraceHeap(collectedRoots);
        STWParam finalMarkStwParam{"final-mark"};
    {
        ScopedStopTheWorld stw(finalMarkStwParam, true, GCPhase::GC_PHASE_FINAL_MARK);
        Remark();
        PostTrace();
        reinterpret_cast<RegionSpace&>(theAllocator_).PrepareForward();
        Preforward();
    }
        GetGCStats().recordSTWTime(finalMarkStwParam.GetElapsedNs());
        ConcurrentPreforward();
        // reclaim large objects should after preforward(may process weak ref) and
        // before fix heap(may clear live bit)
        if (isNotYoungGC) {
            CollectLargeGarbage();
        }
        SweepThreadLocalJitFort();

        CopyFromSpace();
        WVerify::VerifyAfterForward(*this);

        PrepareFix();
        FixHeap();
        if (isNotYoungGC) {
            CollectPinnedGarbage();
        }

        TransitionToGCPhase(GCPhase::GC_PHASE_IDLE, true);
        ClearAllGCInfo();
        CollectSmallSpace();
        return;
    }

    auto collectedRoots = EnumRoots<EnumRootsPolicy::STW_AND_FLIP_MUTATOR>();
    TraceHeap(collectedRoots);
    PreforwardFlip();
    ConcurrentPreforward();
    // reclaim large objects should after preforward(may process weak ref)
    // and before fix heap(may clear live bit)
    if (isNotYoungGC) {
        CollectLargeGarbage();
    }
    SweepThreadLocalJitFort();

    CopyFromSpace();
    WVerify::VerifyAfterForward(*this);

    PrepareFix();
    FixHeap();
    if (isNotYoungGC) {
        CollectPinnedGarbage();
    }

    TransitionToGCPhase(GCPhase::GC_PHASE_IDLE, true);
    ClearAllGCInfo();
    CollectSmallSpace();
}

CArrayList<CArrayList<BaseObject *>> WCollector::EnumRootsFlip(STWParam& param, const common::RefFieldVisitor &visitor)
{
    const auto enumGlobalRoots = [this, &visitor]() {
        SetGCThreadQosPriority(common::PriorityMode::STW);
        EnumRootsImpl<VisitGlobalRoots>(visitor);
        SetGCThreadQosPriority(common::PriorityMode::FOREGROUND);
    };

    std::mutex stackMutex;
    CArrayList<CArrayList<BaseObject *>> rootSet;  // allcate for each mutator
    FlipFunction enumMutatorRoot = [&rootSet, &stackMutex](Mutator &mutator) {
        CArrayList<BaseObject *> roots;
        RefFieldVisitor localVisitor = [&roots](RefField<> &root) { roots.emplace_back(root.GetTargetObject()); };
        VisitMutatorRoot(localVisitor, mutator);
        std::lock_guard<std::mutex> lockGuard(stackMutex);
        rootSet.emplace_back(std::move(roots));
    };
    MutatorManager::Instance().FlipMutators(param, enumGlobalRoots, &enumMutatorRoot);
    return rootSet;
}

void WCollector::ProcessStringTable()
{
#ifdef GC_STW_STRINGTABLE
    return;
#endif
    if (Heap::GetHeap().GetGCReason() == GC_REASON_YOUNG) {
        // no need to fix weak ref in young gc
        return;
    }

    WeakRefFieldVisitor weakVisitor = [this](RefField<> &refField) -> bool {
        auto isSurvivor = [this](BaseObject* oldObj) {
            RegionDesc* region = RegionDesc::GetRegionDescAt(reinterpret_cast<uintptr_t>(oldObj));
            return (gcReason_ == GC_REASON_YOUNG && !region->IsInYoungSpace())
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
        Heap::throwOOM();
        // ConcurrentGC
        obj->UnlockExclusive(BaseStateWord::ForwardState::NORMAL);
        return toObj;
    }
    DLOG(COPY, "copy obj %p<%p>(%zu) to %p", obj, obj->GetTypeInfo(), size, toObj);
    CopyObject(*obj, *toObj, size);

    ASSERT_LOGF(IsToObject(toObj), "Copy object to invalid region");
    toObj->SetForwardState(BaseStateWord::ForwardState::NORMAL);

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

void WCollector::SetGCThreadRssPriority(common::RssPriorityType type)
{
#ifdef ENABLE_RSS
    if (IsPostForked()) {
        LOG_COMMON(DEBUG) << "SetGCThreadRssPriority gettid " << gettid();
        std::unordered_map<std::string, std::string> payLoad = { { "pid", std::to_string(getpid()) },
                                                    { "tid", std::to_string(gettid()) } };
        OHOS::ResourceSchedule::ResSchedClient::GetInstance()
            .ReportData(OHOS::ResourceSchedule::ResType::RES_TYPE_GC_THREAD_QOS_STATUS_CHANGE,
            static_cast<int64_t>(type), payLoad);
        common::Taskpool::GetCurrentTaskpool()->SetThreadRssPriority(type);
    }
#endif
}

void WCollector::SetGCThreadQosPriority(common::PriorityMode mode)
{
#ifdef ENABLE_QOS
    LOG_COMMON(DEBUG) << "SetGCThreadQosPriority gettid " << gettid();
    OHOS_HITRACE(HITRACE_LEVEL_COMMERCIAL, "CMCGC::SetGCThreadQosPriority", "");
    switch (mode) {
        case PriorityMode::STW: {
            OHOS::QOS::SetQosForOtherThread(OHOS::QOS::QosLevel::QOS_USER_INTERACTIVE, gettid());
            break;
        }
        case PriorityMode::FOREGROUND: {
            OHOS::QOS::SetQosForOtherThread(OHOS::QOS::QosLevel::QOS_USER_INITIATED, gettid());
            break;
        }
        case PriorityMode::BACKGROUND: {
            OHOS::QOS::ResetQosForOtherThread(gettid());
            break;
        }
        default:
            UNREACHABLE();
            break;
    }
    common::Taskpool::GetCurrentTaskpool()->SetThreadPriority(mode);
#endif
}

bool WCollector::ShouldIgnoreRequest(GCRequest& request) { return request.ShouldBeIgnored(); }
} // namespace common
