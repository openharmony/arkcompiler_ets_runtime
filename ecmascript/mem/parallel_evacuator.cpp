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

#include "ecmascript/mem/parallel_evacuator-inl.h"

#include "ecmascript/mem/tlab_allocator-inl.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
void ParallelEvacuator::Initialize()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ParallelEvacuatorInitialize);
    waterLine_ = heap_->GetNewSpace()->GetWaterLine();
    if (heap_->IsEdenMark()) {
        heap_->ReleaseEdenAllocator();
    } else {
        ASSERT(heap_->IsYoungMark() || heap_->IsFullMark());
        heap_->SwapNewSpace();
    }
    allocator_ = new TlabAllocator(heap_);
    promotedSize_ = 0;
    edenToYoungSize_ = 0;
}

void ParallelEvacuator::Finalize()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ParallelEvacuatorFinalize);
    delete allocator_;
    evacuateWorkloadSet_.Clear();
    updateWorkloadSet_.Clear();
}

void ParallelEvacuator::Evacuate()
{
    Initialize();
    EvacuateSpace();
    UpdateReference();
    Finalize();
}

void ParallelEvacuator::UpdateTrackInfo()
{
    for (uint32_t i = 0; i <= MAX_TASKPOOL_THREAD_NUM; i++) {
        auto &trackInfoSet = ArrayTrackInfoSet(i);
        for (auto &each : trackInfoSet) {
            auto trackInfoVal = JSTaggedValue(each);
            if (!trackInfoVal.IsHeapObject() || !trackInfoVal.IsWeak()) {
                continue;
            }
            auto trackInfo = trackInfoVal.GetWeakReferentUnChecked();
            trackInfo = UpdateAddressAfterEvacation(trackInfo);
            if (trackInfo) {
                heap_->GetEcmaVM()->GetPGOProfiler()->UpdateTrackSpaceFlag(trackInfo, RegionSpaceFlag::IN_OLD_SPACE);
            }
        }
        trackInfoSet.clear();
    }
}

void ParallelEvacuator::EvacuateSpace()
{
    TRACE_GC(GCStats::Scope::ScopeId::EvacuateSpace, heap_->GetEcmaVM()->GetEcmaGCStats());
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::EvacuateSpace");
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ParallelEvacuator);
    auto &workloadSet = evacuateWorkloadSet_;
    if (heap_->IsEdenMark()) {
        heap_->GetEdenSpace()->EnumerateRegions([this, &workloadSet](Region *current) {
            workloadSet.Add(std::make_unique<EvacuateWorkload>(this, current));
        });
    } else if (heap_->IsConcurrentFullMark() || heap_->IsYoungMark()) {
        heap_->GetEdenSpace()->EnumerateRegions([this, &workloadSet](Region *current) {
            workloadSet.Add(std::make_unique<EvacuateWorkload>(this, current));
        });
        heap_->GetFromSpaceDuringEvacuation()->EnumerateRegions([this, &workloadSet](Region *current) {
            workloadSet.Add(std::make_unique<EvacuateWorkload>(this, current));
        });
        heap_->GetOldSpace()->EnumerateCollectRegionSet([this, &workloadSet](Region *current) {
            workloadSet.Add(std::make_unique<EvacuateWorkload>(this, current));
        });
    }
    workloadSet.PrepareWorkloads();
    if (heap_->IsParallelGCEnabled()) {
        LockHolder holder(mutex_);
        parallel_ = CalculateEvacuationThreadNum();
        ASSERT(parallel_ >= 0);
        evacuateTaskNum_ = static_cast<uint32_t>(parallel_);
        for (uint32_t i = 1; i <= evacuateTaskNum_; i++) {
            Taskpool::GetCurrentTaskpool()->PostTask(
                std::make_unique<EvacuationTask>(heap_->GetJSThread()->GetThreadId(), i, this));
        }
    } else {
        evacuateTaskNum_ = 0;
    }
    {
        GCStats::Scope sp2(GCStats::Scope::ScopeId::EvacuateRegion, heap_->GetEcmaVM()->GetEcmaGCStats());
        EvacuateSpace(allocator_, MAIN_THREAD_INDEX, 0, true);
    }

    {
        GCStats::Scope sp2(GCStats::Scope::ScopeId::WaitFinish, heap_->GetEcmaVM()->GetEcmaGCStats());
        WaitFinished();
    }

    if (heap_->GetJSThread()->IsPGOProfilerEnable()) {
        UpdateTrackInfo();
    }
}

bool ParallelEvacuator::EvacuateSpace(TlabAllocator *allocator, uint32_t threadIndex, uint32_t idOrder, bool isMain)
{
    UpdateRecordWeakReferenceInParallel(idOrder);

    auto &arrayTrackInfoSet = ArrayTrackInfoSet(threadIndex);
    DrainWorkloads(evacuateWorkloadSet_, [&](std::unique_ptr<Workload> &region) {
        EvacuateRegion(allocator, region->GetRegion(), arrayTrackInfoSet);
    });
    allocator->Finalize();
    if (!isMain) {
        LockHolder holder(mutex_);
        if (--parallel_ <= 0) {
            condition_.SignalAll();
        }
    }
    return true;
}

void ParallelEvacuator::UpdateRecordWeakReferenceInParallel(uint32_t idOrder)
{
    auto totalThreadCount = Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() + 1;
    for (uint32_t i = idOrder; i < totalThreadCount; i += (evacuateTaskNum_ + 1)) {
        ProcessQueue *queue = heap_->GetWorkManager()->GetWeakReferenceQueue(i);
        while (true) {
            auto obj = queue->PopBack();
            if (UNLIKELY(obj == nullptr)) {
                break;
            }
            ObjectSlot slot(ToUintPtr(obj));
            JSTaggedType value = slot.GetTaggedType();
            if (JSTaggedValue(value).IsWeak()) {
                ASSERT(heap_->IsConcurrentFullMark());
                Region *objectRegion = Region::ObjectAddressToRange(value);
                if (!objectRegion->InGeneralNewSpaceOrCSet() && !objectRegion->InSharedHeap() &&
                        (objectRegion->GetMarkGCBitset() == nullptr || !objectRegion->Test(value))) {
                    slot.Clear();
                }
            }
        }
    }
}

void ParallelEvacuator::EvacuateRegion(TlabAllocator *allocator, Region *region,
                                       std::unordered_set<JSTaggedType> &trackSet)
{
    bool isInEden = region->InEdenSpace();
    bool isInOldGen = region->InOldSpace();
    bool isBelowAgeMark = region->BelowAgeMark();
    bool pgoEnabled = heap_->GetJSThread()->IsPGOProfilerEnable();
    bool inHeapProfiler = heap_->InHeapProfiler();
    size_t promotedSize = 0;
    size_t edenToYoungSize = 0;
    if (WholeRegionEvacuate(region)) {
        return;
    }
    region->IterateAllMarkedBits([this, &region, &isInOldGen, &isBelowAgeMark, isInEden, &pgoEnabled,
                                  &promotedSize, &allocator, &trackSet, &edenToYoungSize, inHeapProfiler](void *mem) {
        ASSERT(region->InRange(ToUintPtr(mem)));
        auto header = reinterpret_cast<TaggedObject *>(mem);
        auto klass = header->GetClass();
        auto size = klass->SizeFromJSHClass(header);

        uintptr_t address = 0;
        bool actualPromoted = false;
        bool hasAgeMark = isBelowAgeMark || (region->HasAgeMark() && ToUintPtr(mem) < waterLine_);
        if (hasAgeMark) {
            address = allocator->Allocate(size, OLD_SPACE);
            actualPromoted = true;
            promotedSize += size;
        } else if (isInOldGen) {
            address = allocator->Allocate(size, OLD_SPACE);
            actualPromoted = true;
        } else {
            address = allocator->Allocate(size, SEMI_SPACE);
            if (address == 0) {
                address = allocator->Allocate(size, OLD_SPACE);
                actualPromoted = true;
                promotedSize += size;
            } else if (isInEden) {
                edenToYoungSize += size;
            }
        }
        LOG_ECMA_IF(address == 0, FATAL) << "Evacuate object failed:" << size;

        if (memcpy_s(ToVoidPtr(address), size, ToVoidPtr(ToUintPtr(mem)), size) != EOK) { // LOCV_EXCL_BR_LINE
            LOG_FULL(FATAL) << "memcpy_s failed";
        }
        if (inHeapProfiler) {
            heap_->OnMoveEvent(reinterpret_cast<uintptr_t>(mem), reinterpret_cast<TaggedObject *>(address), size);
        }
        if (pgoEnabled) {
            if (actualPromoted && klass->IsJSArray()) {
                auto trackInfo = JSArray::Cast(header)->GetTrackInfo();
                trackSet.emplace(trackInfo.GetRawData());
            }
        }
        Barriers::SetPrimitive(header, 0, MarkWord::FromForwardingAddress(address));

        if (actualPromoted) {
            SetObjectFieldRSet<false>(reinterpret_cast<TaggedObject *>(address), klass);
        } else if (isInEden) {
            SetObjectFieldRSet<true>(reinterpret_cast<TaggedObject *>(address), klass);
        } else if (region->HasLocalToShareRememberedSet()) {
            UpdateLocalToShareRSet(reinterpret_cast<TaggedObject *>(address), klass);
        }
    });
    promotedSize_.fetch_add(promotedSize);
    edenToYoungSize_.fetch_add(edenToYoungSize);
}

void ParallelEvacuator::UpdateReference()
{
    TRACE_GC(GCStats::Scope::ScopeId::UpdateReference, heap_->GetEcmaVM()->GetEcmaGCStats());
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ParallelUpdateReference);
    // Update reference pointers
    uint32_t youngeRegionMoveCount = 0;
    uint32_t youngeRegionCopyCount = 0;
    uint32_t oldRegionCount = 0;
    auto &workloadSet = updateWorkloadSet_;
    if (heap_->IsEdenMark()) {
        heap_->GetNewSpace()->EnumerateRegions([&]([[maybe_unused]] Region *current) {
            workloadSet.Add(
                std::make_unique<UpdateNewToEdenRSetWorkload>(this, current));
        });
    } else {
        heap_->GetNewSpace()->EnumerateRegions([&](Region *current) {
            if (current->InNewToNewSet()) {
                workloadSet.Add(
                    std::make_unique<UpdateAndSweepNewRegionWorkload>(
                        this, current, heap_->IsYoungMark()));
                youngeRegionMoveCount++;
            } else {
                workloadSet.Add(
                    std::make_unique<UpdateNewRegionWorkload>(this, current, heap_->IsYoungMark()));
                youngeRegionCopyCount++;
            }
        });
    }
    heap_->EnumerateOldSpaceRegions([this, &oldRegionCount, &workloadSet](Region *current) {
        if (current->InCollectSet()) {
            return;
        }
        workloadSet.Add(std::make_unique<UpdateRSetWorkload>(this, current, heap_->IsEdenMark()));
        oldRegionCount++;
    });
    heap_->EnumerateSnapshotSpaceRegions([this, &workloadSet](Region *current) {
        workloadSet.Add(std::make_unique<UpdateRSetWorkload>(this, current, heap_->IsEdenMark()));
    });
    workloadSet.PrepareWorkloads();
    LOG_GC(DEBUG) << "UpdatePointers statistic: younge space region compact moving count:"
                        << youngeRegionMoveCount
                        << "younge space region compact coping count:" << youngeRegionCopyCount
                        << "old space region count:" << oldRegionCount;

    if (heap_->IsParallelGCEnabled()) {
        LockHolder holder(mutex_);
        parallel_ = CalculateUpdateThreadNum();
        for (int i = 0; i < parallel_; i++) {
            Taskpool::GetCurrentTaskpool()->PostTask(
                std::make_unique<UpdateReferenceTask>(heap_->GetJSThread()->GetThreadId(), this));
        }
    }
    {
        GCStats::Scope sp2(GCStats::Scope::ScopeId::UpdateRoot, heap_->GetEcmaVM()->GetEcmaGCStats());
        UpdateRoot();
    }

    {
        GCStats::Scope sp2(GCStats::Scope::ScopeId::UpdateWeekRef, heap_->GetEcmaVM()->GetEcmaGCStats());
        if (heap_->IsEdenMark()) {
            UpdateWeakReferenceOpt<TriggerGCType::EDEN_GC>();
        } else if (heap_->IsYoungMark()) {
            UpdateWeakReferenceOpt<TriggerGCType::YOUNG_GC>();
        } else {
            UpdateWeakReferenceOpt<TriggerGCType::OLD_GC>();
        }
    }
    {
        GCStats::Scope sp2(GCStats::Scope::ScopeId::ProceeWorkload, heap_->GetEcmaVM()->GetEcmaGCStats());\
        ProcessWorkloads(true);
    }
    WaitFinished();
}

void ParallelEvacuator::UpdateRoot()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), UpdateRoot);
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::UpdateRoot");
    RootVisitor gcUpdateYoung = [this]([[maybe_unused]] Root type, ObjectSlot slot) {
        UpdateObjectSlot(slot);
    };
    RootRangeVisitor gcUpdateRangeYoung = [this]([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end) {
        for (ObjectSlot slot = start; slot < end; slot++) {
            UpdateObjectSlot(slot);
        }
    };
    RootBaseAndDerivedVisitor gcUpdateDerived =
        []([[maybe_unused]] Root type, ObjectSlot base, ObjectSlot derived, uintptr_t baseOldObject) {
        if (JSTaggedValue(base.GetTaggedType()).IsHeapObject()) {
            derived.Update(base.GetTaggedType() + derived.GetTaggedType() - baseOldObject);
        }
    };

    ObjectXRay::VisitVMRoots(heap_->GetEcmaVM(), gcUpdateYoung, gcUpdateRangeYoung, gcUpdateDerived,
                             VMRootVisitType::UPDATE_ROOT);
}

template<TriggerGCType gcType>
void ParallelEvacuator::UpdateWeakReferenceOpt()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), UpdateWeakReferenceOpt);
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::UpdateWeakReferenceOpt");
    WeakRootVisitor gcUpdateWeak = [](TaggedObject *header) -> TaggedObject* {
        Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(header));
        ASSERT(objectRegion != nullptr);
        if constexpr (gcType == TriggerGCType::EDEN_GC) {
            if (!objectRegion->InEdenSpace()) {
                return header;
            }
            MarkWord markWord(header);
            if (markWord.IsForwardingAddress()) {
                return markWord.ToForwardingAddress();
            }
            return nullptr;
        } else if constexpr (gcType == TriggerGCType::YOUNG_GC) {
            if (!objectRegion->InGeneralNewSpace()) {
                return header;
            }
        } else if constexpr (gcType == TriggerGCType::OLD_GC) {
            if (!objectRegion->InGeneralNewSpaceOrCSet()) {
                if (!objectRegion->InSharedHeap() && (objectRegion->GetMarkGCBitset() == nullptr ||
                                              !objectRegion->Test(header))) {
                    return nullptr;
                }
                return header;
            }
        } else { // LOCV_EXCL_BR_LINE
            LOG_GC(FATAL) << "WeakRootVisitor: not support gcType yet";
            UNREACHABLE();
        }
        if (objectRegion->InNewToNewSet()) {
            if (objectRegion->Test(header)) {
                return header;
            }
        } else {
            MarkWord markWord(header);
            if (markWord.IsForwardingAddress()) {
                return markWord.ToForwardingAddress();
            }
        }
        return nullptr;
    };

    heap_->GetEcmaVM()->GetJSThread()->IterateWeakEcmaGlobalStorage(gcUpdateWeak);
    heap_->GetEcmaVM()->ProcessReferences(gcUpdateWeak);
    heap_->GetEcmaVM()->GetJSThread()->UpdateJitCodeMapReference(gcUpdateWeak);
}

template<bool IsEdenGC>
void ParallelEvacuator::UpdateRSet(Region *region)
{
    auto cb = [this](void *mem) -> bool {
        ObjectSlot slot(ToUintPtr(mem));
        return UpdateOldToNewObjectSlot<IsEdenGC>(slot);
    };

    if (heap_->GetSweeper()->IsSweeping()) {
        if (region->IsGCFlagSet(RegionGCFlags::HAS_BEEN_SWEPT)) {
            // Region is safe while update remember set
            region->MergeOldToNewRSetForCS();
            region->MergeLocalToShareRSetForCS();
        } else {
            region->AtomicIterateAllSweepingRSetBits(cb);
        }
    }
    region->IterateAllOldToNewBits(cb);
    if (heap_->IsYoungMark()) {
        return;
    }
    if constexpr (IsEdenGC) {
        region->IterateAllCrossRegionBits([this](void *mem) {
            ObjectSlot slot(ToUintPtr(mem));
            UpdateObjectSlot(slot);
        });
    } else {
        region->IterateAllCrossRegionBits([this](void *mem) {
            ObjectSlot slot(ToUintPtr(mem));
            JSTaggedType value = slot.GetTaggedType();
            if (JSTaggedValue(value).IsHeapObject() && Region::ObjectAddressToRange(value)->InCollectSet()) {
                UpdateObjectSlotOpt<TriggerGCType::OLD_GC>(slot);
            }
        });
    }
    region->DeleteCrossRegionRSet();
}

void ParallelEvacuator::UpdateNewToEdenRSetReference(Region *region)
{
    auto cb = [this](void *mem) -> bool {
        ObjectSlot slot(ToUintPtr(mem));
        return UpdateNewToEdenObjectSlot(slot);
    };
    region->IterateAllNewToEdenBits(cb);
    region->ClearNewToEdenRSet();
}

template<TriggerGCType gcType>
void ParallelEvacuator::UpdateNewRegionReference(Region *region)
{
    Region *current = heap_->GetNewSpace()->GetCurrentRegion();
    auto curPtr = region->GetBegin();
    uintptr_t endPtr = 0;
    if (region == current) {
        auto top = heap_->GetNewSpace()->GetTop();
        endPtr = curPtr + region->GetAllocatedBytes(top);
    } else {
        endPtr = curPtr + region->GetAllocatedBytes();
    }

    size_t objSize = 0;
    while (curPtr < endPtr) {
        auto freeObject = FreeObject::Cast(curPtr);
        // If curPtr is freeObject, It must to mark unpoison first.
        ASAN_UNPOISON_MEMORY_REGION(reinterpret_cast<void *>(freeObject), TaggedObject::TaggedObjectSize());
        if (!freeObject->IsFreeObject()) {
            auto obj = reinterpret_cast<TaggedObject *>(curPtr);
            auto klass = obj->GetClass();
            UpdateNewObjectField<gcType>(obj, klass);
            objSize = klass->SizeFromJSHClass(obj);
        } else {
            freeObject->AsanUnPoisonFreeObject();
            objSize = freeObject->Available();
            freeObject->AsanPoisonFreeObject();
        }
        curPtr += objSize;
        CHECK_OBJECT_SIZE(objSize);
    }
    CHECK_REGION_END(curPtr, endPtr);
}

template<TriggerGCType gcType>
void ParallelEvacuator::UpdateAndSweepNewRegionReference(Region *region)
{
    uintptr_t freeStart = region->GetBegin();
    uintptr_t freeEnd = freeStart + region->GetAllocatedBytes();
    region->IterateAllMarkedBits([&](void *mem) {
        ASSERT(region->InRange(ToUintPtr(mem)));
        auto header = reinterpret_cast<TaggedObject *>(mem);
        JSHClass *klass = header->GetClass();
        UpdateNewObjectField<gcType>(header, klass);

        uintptr_t freeEnd = ToUintPtr(mem);
        if (freeStart != freeEnd) {
            size_t freeSize = freeEnd - freeStart;
            FreeObject::FillFreeObject(heap_, freeStart, freeSize);
            region->ClearLocalToShareRSetInRange(freeStart, freeEnd);
        }

        freeStart = freeEnd + klass->SizeFromJSHClass(header);
    });
    CHECK_REGION_END(freeStart, freeEnd);
    if (freeStart < freeEnd) {
        FreeObject::FillFreeObject(heap_, freeStart, freeEnd - freeStart);
        region->ClearLocalToShareRSetInRange(freeStart, freeEnd);
    }
}

template<TriggerGCType gcType>
void ParallelEvacuator::UpdateNewObjectField(TaggedObject *object, JSHClass *cls)
{
    ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, cls,
        [this](TaggedObject *root, ObjectSlot start, ObjectSlot end, VisitObjectArea area) {
            if (area == VisitObjectArea::IN_OBJECT) {
                if (VisitBodyInObj(root, start, end,
                                    [&](ObjectSlot slot) { UpdateObjectSlotOpt<gcType>(slot); })) {
                    return;
                };
            }
            for (ObjectSlot slot = start; slot < end; slot++) {
                UpdateObjectSlotOpt<gcType>(slot);
            }
        });
}

void ParallelEvacuator::WaitFinished()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), WaitUpdateFinished);
    if (parallel_ > 0) {
        LockHolder holder(mutex_);
        while (parallel_ > 0) {
            condition_.Wait(&mutex_);
        }
    }
}

bool ParallelEvacuator::ProcessWorkloads(bool isMain)
{
    DrainWorkloads(updateWorkloadSet_, [&](std::unique_ptr<Workload> &region) {
        region->Process(isMain);
        });
    if (!isMain) {
        LockHolder holder(mutex_);
        if (--parallel_ <= 0) {
            condition_.SignalAll();
        }
    }
    return true;
}

template <typename WorkloadCallback>
void ParallelEvacuator::DrainWorkloads(WorkloadSet &workloadSet, WorkloadCallback callback)
{
    std::unique_ptr<Workload> region;
    while (workloadSet.HasRemaningWorkload()) {
        std::optional<size_t> index = workloadSet.GetNextIndex();
        if (!index.has_value()) {
            return;
        }
        size_t count = workloadSet.GetWorkloadCount();
        size_t finishedCount = 0;
        for (size_t i = index.value(); i < count; i++) {
            region = workloadSet.TryGetWorkload(i);
            if (region == nullptr) {
                break;
            }
            callback(region);
            finishedCount++;
        }
        if (finishedCount && workloadSet.FetchSubAndCheckWorkloadCount(finishedCount)) {
            return;
        }
    }
}

void ParallelEvacuator::WorkloadSet::PrepareWorkloads()
{
    size_t size = workloads_.size();
    remainingWorkloadNum_.store(size, std::memory_order_relaxed);
    /*
    Construct indexList_ containing starting indices for multi-threaded acquire workload.
    The construction method starts with the interval [0, size] and recursively
    selects midpoints as starting indices for subintervals.
    The first starting index is 0 to ensure no workloads are missed.
    */
    indexList_.reserve(size);
    indexList_.emplace_back(0);
    std::vector<std::pair<size_t, size_t>> pairList{{0, size}};
    pairList.reserve(size);
    while (!pairList.empty()) {
        auto [start, end] = pairList.back();
        pairList.pop_back();
        size_t mid = (start + end) >> 1;
        indexList_.emplace_back(mid);
        if (end - mid > 1U) {
            pairList.emplace_back(mid, end);
        }
        if (mid - start > 1U) {
            pairList.emplace_back(start, mid);
        }
    }
}

std::optional<size_t> ParallelEvacuator::WorkloadSet::GetNextIndex()
{
    size_t cursor = indexCursor_.fetch_add(1, std::memory_order_relaxed);
    if (cursor >= indexList_.size()) {
        return std::nullopt;
    }
    return indexList_[cursor];
}

std::unique_ptr<ParallelEvacuator::Workload> ParallelEvacuator::WorkloadSet::TryGetWorkload(size_t index)
{
    std::unique_ptr<Workload> workload;
    if (workloads_.at(index).first.TryAcquire()) {
        workload = std::move(workloads_[index].second);
    }
    return workload;
}

void ParallelEvacuator::WorkloadSet::Clear()
{
    workloads_.clear();
    indexList_.clear();
    indexCursor_.store(0, std::memory_order_relaxed);
    remainingWorkloadNum_.store(0, std::memory_order_relaxed);
}

ParallelEvacuator::EvacuationTask::EvacuationTask(int32_t id, uint32_t idOrder, ParallelEvacuator *evacuator)
    : Task(id), idOrder_(idOrder), evacuator_(evacuator)
{
    allocator_ = new TlabAllocator(evacuator->heap_);
}

ParallelEvacuator::EvacuationTask::~EvacuationTask()
{
    delete allocator_;
}

bool ParallelEvacuator::EvacuationTask::Run(uint32_t threadIndex)
{
    return evacuator_->EvacuateSpace(allocator_, threadIndex, idOrder_);
}

bool ParallelEvacuator::UpdateReferenceTask::Run([[maybe_unused]] uint32_t threadIndex)
{
    evacuator_->ProcessWorkloads(false);
    return true;
}

bool ParallelEvacuator::EvacuateWorkload::Process([[maybe_unused]] bool isMain)
{
    return true;
}

bool ParallelEvacuator::UpdateRSetWorkload::Process([[maybe_unused]] bool isMain)
{
    if (isEdenGC_) {
        GetEvacuator()->UpdateRSet<true>(GetRegion());
    } else {
        GetEvacuator()->UpdateRSet<false>(GetRegion());
    }
    return true;
}

bool ParallelEvacuator::UpdateNewToEdenRSetWorkload::Process([[maybe_unused]] bool isMain)
{
    GetEvacuator()->UpdateNewToEdenRSetReference(GetRegion());
    return true;
}


bool ParallelEvacuator::UpdateNewRegionWorkload::Process([[maybe_unused]] bool isMain)
{
    if (isYoungGC_) {
        GetEvacuator()->UpdateNewRegionReference<TriggerGCType::YOUNG_GC>(GetRegion());
    } else {
        GetEvacuator()->UpdateNewRegionReference<TriggerGCType::OLD_GC>(GetRegion());
    }
    return true;
}

bool ParallelEvacuator::UpdateAndSweepNewRegionWorkload::Process([[maybe_unused]] bool isMain)
{
    if (isYoungGC_) {
        GetEvacuator()->UpdateAndSweepNewRegionReference<TriggerGCType::YOUNG_GC>(GetRegion());
    } else {
        GetEvacuator()->UpdateAndSweepNewRegionReference<TriggerGCType::OLD_GC>(GetRegion());
    }
    return true;
}
}  // namespace panda::ecmascript
