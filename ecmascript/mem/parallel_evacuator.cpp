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

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/concurrent_sweeper.h"
#include "ecmascript/mem/gc_bitset.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/mem/tlab_allocator-inl.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/ecma_string_table.h"
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
    if (heap_->IsEdenMark()) {
        heap_->GetEdenSpace()->EnumerateRegions([this] (Region *current) {
            AddWorkload(std::make_unique<EvacuateWorkload>(this, current));
        });
    } else if (heap_->IsConcurrentFullMark() || heap_->IsYoungMark()) {
        heap_->GetEdenSpace()->EnumerateRegions([this] (Region *current) {
            AddWorkload(std::make_unique<EvacuateWorkload>(this, current));
        });
        heap_->GetFromSpaceDuringEvacuation()->EnumerateRegions([this] (Region *current) {
            AddWorkload(std::make_unique<EvacuateWorkload>(this, current));
        });
        heap_->GetOldSpace()->EnumerateCollectRegionSet([this](Region *current) {
            AddWorkload(std::make_unique<EvacuateWorkload>(this, current));
        });
    }
    if (heap_->IsParallelGCEnabled()) {
        LockHolder holder(mutex_);
        parallel_ = CalculateEvacuationThreadNum();
        for (int i = 0; i < parallel_; i++) {
            Taskpool::GetCurrentTaskpool()->PostTask(
                std::make_unique<EvacuationTask>(heap_->GetJSThread()->GetThreadId(), this));
        }
    }
    {
        GCStats::Scope sp2(GCStats::Scope::ScopeId::EvacuateRegion, heap_->GetEcmaVM()->GetEcmaGCStats());
        EvacuateSpace(allocator_, MAIN_THREAD_INDEX, true);
    }

    {
        GCStats::Scope sp2(GCStats::Scope::ScopeId::WaitFinish, heap_->GetEcmaVM()->GetEcmaGCStats());
        WaitFinished();
    }

    if (heap_->GetJSThread()->IsPGOProfilerEnable()) {
        UpdateTrackInfo();
    }
}

bool ParallelEvacuator::EvacuateSpace(TlabAllocator *allocator, uint32_t threadIndex, bool isMain)
{
    std::unique_ptr<Workload> region = GetWorkloadSafe();
    auto &arrayTrackInfoSet = ArrayTrackInfoSet(threadIndex);
    while (region != nullptr) {
        EvacuateRegion(allocator, region->GetRegion(), arrayTrackInfoSet);
        region = GetWorkloadSafe();
    }
    allocator->Finalize();
    if (!isMain) {
        LockHolder holder(mutex_);
        if (--parallel_ <= 0) {
            condition_.SignalAll();
        }
    }
    return true;
}

void ParallelEvacuator::EvacuateRegion(TlabAllocator *allocator, Region *region,
                                       std::unordered_set<JSTaggedType> &trackSet)
{
    bool isInEden = region->InEdenSpace();
    bool isInOldGen = region->InOldSpace();
    bool isBelowAgeMark = region->BelowAgeMark();
    bool pgoEnabled = heap_->GetJSThread()->IsPGOProfilerEnable();
    size_t promotedSize = 0;
    size_t edenToYoungSize = 0;
    if (WholeRegionEvacuate(region)) {
        return;
    }
    region->IterateAllMarkedBits([this, &region, &isInOldGen, &isBelowAgeMark, isInEden, &pgoEnabled,
                                  &promotedSize, &allocator, &trackSet, &edenToYoungSize](void *mem) {
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

        if (memcpy_s(ToVoidPtr(address), size, ToVoidPtr(ToUintPtr(mem)), size) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
        }
        heap_->OnMoveEvent(reinterpret_cast<uintptr_t>(mem), reinterpret_cast<TaggedObject *>(address), size);
        if (pgoEnabled) {
            if (actualPromoted && klass->IsJSArray()) {
                auto trackInfo = JSArray::Cast(header)->GetTrackInfo();
                trackSet.emplace(trackInfo.GetRawData());
            }
        }
        Barriers::SetPrimitive(header, 0, MarkWord::FromForwardingAddress(address));

        if (UNLIKELY(heap_->ShouldVerifyHeap())) {
            VerifyHeapObject(reinterpret_cast<TaggedObject *>(address));
        }
        if (actualPromoted) {
            SetObjectFieldRSet<false>(reinterpret_cast<TaggedObject *>(address), klass);
        } else if (isInEden) {
            SetObjectFieldRSet<true>(reinterpret_cast<TaggedObject *>(address), klass);
        }
        if (region->HasLocalToShareRememberedSet()) {
            UpdateLocalToShareRSet(reinterpret_cast<TaggedObject *>(address), klass);
        }
    });
    promotedSize_.fetch_add(promotedSize);
    edenToYoungSize_.fetch_add(edenToYoungSize);
}

void ParallelEvacuator::VerifyHeapObject(TaggedObject *object)
{
    auto klass = object->GetClass();
    ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, klass,
        [&](TaggedObject *root, ObjectSlot start, ObjectSlot end, VisitObjectArea area) {
            if (area == VisitObjectArea::IN_OBJECT) {
                if (VisitBodyInObj(root, start, end, [&](ObjectSlot slot) { VerifyValue(object, slot); })) {
                    return;
                };
            }
            for (ObjectSlot slot = start; slot < end; slot++) {
                VerifyValue(object, slot);
            }
        });
}

void ParallelEvacuator::VerifyValue(TaggedObject *object, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        if (value.IsWeakForHeapObject()) {
            return;
        }
        Region *objectRegion = Region::ObjectAddressToRange(value.GetTaggedObject());
        if (objectRegion->InSharedHeap()) {
            return;
        }
        if (!heap_->IsConcurrentFullMark() && objectRegion->InGeneralOldSpace()) {
            return;
        }

        if (heap_->IsEdenMark() && !objectRegion->InEdenSpace()) {
            return;
        }

        if (!objectRegion->Test(value.GetTaggedObject()) && !objectRegion->InAppSpawnSpace()) {
            LOG_GC(FATAL) << "Miss mark value: " << value.GetTaggedObject()
                                << ", body address:" << slot.SlotAddress()
                                << ", header address:" << object;
        }
    }
}

void ParallelEvacuator::UpdateReference()
{
    TRACE_GC(GCStats::Scope::ScopeId::UpdateReference, heap_->GetEcmaVM()->GetEcmaGCStats());
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), ParallelUpdateReference);
    // Update reference pointers
    uint32_t youngeRegionMoveCount = 0;
    uint32_t youngeRegionCopyCount = 0;
    uint32_t oldRegionCount = 0;
    if (heap_->IsEdenMark()) {
        heap_->GetNewSpace()->EnumerateRegions([&] ([[maybe_unused]] Region *current) {
            AddWorkload(std::make_unique<UpdateNewToEdenRSetWorkload>(this, current));
        });
    } else {
        heap_->GetNewSpace()->EnumerateRegions([&] (Region *current) {
            if (current->InNewToNewSet()) {
                AddWorkload(std::make_unique<UpdateAndSweepNewRegionWorkload>(this, current));
                youngeRegionMoveCount++;
            } else {
                AddWorkload(std::make_unique<UpdateNewRegionWorkload>(this, current));
                youngeRegionCopyCount++;
            }
        });
    }
    heap_->EnumerateOldSpaceRegions([this, &oldRegionCount] (Region *current) {
        if (current->InCollectSet()) {
            return;
        }
        AddWorkload(std::make_unique<UpdateRSetWorkload>(this, current, heap_->IsEdenMark()));
        oldRegionCount++;
    });
    heap_->EnumerateSnapshotSpaceRegions([this] (Region *current) {
        AddWorkload(std::make_unique<UpdateRSetWorkload>(this, current, heap_->IsEdenMark()));
    });
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
        UpdateWeakReference();
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

    ObjectXRay::VisitVMRoots(heap_->GetEcmaVM(), gcUpdateYoung, gcUpdateRangeYoung, gcUpdateDerived);
}

void ParallelEvacuator::UpdateRecordWeakReference()
{
    auto totalThreadCount = Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() + 1;
    for (uint32_t i = 0; i < totalThreadCount; i++) {
        ProcessQueue *queue = heap_->GetWorkManager()->GetWeakReferenceQueue(i);

        while (true) {
            auto obj = queue->PopBack();
            if (UNLIKELY(obj == nullptr)) {
                break;
            }
            ObjectSlot slot(ToUintPtr(obj));
            JSTaggedValue value(slot.GetTaggedType());
            if (value.IsWeak()) {
                UpdateWeakObjectSlot(value.GetTaggedWeakRef(), slot);
            }
        }
    }
}

void ParallelEvacuator::UpdateWeakReference()
{
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), UpdateWeakReference);
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::UpdateWeakReference");
    UpdateRecordWeakReference();
    bool isFullMark = heap_->IsConcurrentFullMark();
    bool isEdenMark = heap_->IsEdenMark();
    WeakRootVisitor gcUpdateWeak = [isFullMark, isEdenMark](TaggedObject *header) -> TaggedObject* {
        Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(header));
        if (UNLIKELY(objectRegion == nullptr)) {
            LOG_GC(ERROR) << "PartialGC updateWeakReference: region is nullptr, header is " << header;
            return nullptr;
        }
        // The weak object in shared heap is always alive during partialGC.
        if (objectRegion->InSharedHeap()) {
            return header;
        }
        if (isEdenMark) {
            if (!objectRegion->InEdenSpace()) {
                return header;
            }
            MarkWord markWord(header);
            if (markWord.IsForwardingAddress()) {
                return markWord.ToForwardingAddress();
            }
            return nullptr;
        }
        if (objectRegion->InGeneralNewSpaceOrCSet()) {
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
        }
        if (isFullMark) {
            if (objectRegion->GetMarkGCBitset() == nullptr || !objectRegion->Test(header)) {
                return nullptr;
            }
        }
        return header;
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
    region->IterateAllCrossRegionBits([this](void *mem) {
        ObjectSlot slot(ToUintPtr(mem));
        UpdateObjectSlot(slot);
    });
    region->ClearCrossRegionRSet();
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
            UpdateNewObjectField(obj, klass);
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

void ParallelEvacuator::UpdateAndSweepNewRegionReference(Region *region)
{
    uintptr_t freeStart = region->GetBegin();
    uintptr_t freeEnd = freeStart + region->GetAllocatedBytes();
    region->IterateAllMarkedBits([&](void *mem) {
        ASSERT(region->InRange(ToUintPtr(mem)));
        auto header = reinterpret_cast<TaggedObject *>(mem);
        JSHClass *klass = header->GetClass();
        UpdateNewObjectField(header, klass);

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

void ParallelEvacuator::UpdateNewObjectField(TaggedObject *object, JSHClass *cls)
{
    ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, cls,
        [this](TaggedObject *root, ObjectSlot start, ObjectSlot end, VisitObjectArea area) {
            if (area == VisitObjectArea::IN_OBJECT) {
                if (VisitBodyInObj(root, start, end, [&](ObjectSlot slot) { UpdateObjectSlot(slot); })) {
                    return;
                };
            }
            for (ObjectSlot slot = start; slot < end; slot++) {
                UpdateObjectSlot(slot);
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
    std::unique_ptr<Workload> region = GetWorkloadSafe();
    while (region != nullptr) {
        region->Process(isMain);
        region = GetWorkloadSafe();
    }
    if (!isMain) {
        LockHolder holder(mutex_);
        if (--parallel_ <= 0) {
            condition_.SignalAll();
        }
    }
    return true;
}

ParallelEvacuator::EvacuationTask::EvacuationTask(int32_t id, ParallelEvacuator *evacuator)
    : Task(id), evacuator_(evacuator)
{
    allocator_ = new TlabAllocator(evacuator->heap_);
}

ParallelEvacuator::EvacuationTask::~EvacuationTask()
{
    delete allocator_;
}

bool ParallelEvacuator::EvacuationTask::Run(uint32_t threadIndex)
{
    return evacuator_->EvacuateSpace(allocator_, threadIndex);
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
    GetEvacuator()->UpdateNewRegionReference(GetRegion());
    return true;
}

bool ParallelEvacuator::UpdateAndSweepNewRegionWorkload::Process([[maybe_unused]] bool isMain)
{
    GetEvacuator()->UpdateAndSweepNewRegionReference(GetRegion());
    return true;
}
}  // namespace panda::ecmascript
