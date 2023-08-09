/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/visitor.h"

namespace panda::ecmascript {
Marker::Marker(Heap *heap) : heap_(heap), objXRay_(heap->GetEcmaVM()), workManager_(heap->GetWorkManager()) {}

void Marker::MarkRoots(uint32_t threadId)
{
    TRACE_GC(GCStats::Scope::ScopeId::MarkRoots, heap_->GetEcmaVM()->GetEcmaGCStats());
    objXRay_.VisitVMRoots(
        std::bind(&Marker::HandleRoots, this, threadId, std::placeholders::_1, std::placeholders::_2),
        std::bind(&Marker::HandleRangeRoots, this, threadId, std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3),
        std::bind(&Marker::HandleDerivedRoots, this, std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3, std::placeholders::_4));
    workManager_->PushWorkNodeToGlobal(threadId, false);
}

void Marker::ProcessOldToNew(uint32_t threadId)
{
    heap_->EnumerateOldSpaceRegions(std::bind(&Marker::HandleOldToNewRSet, this, threadId, std::placeholders::_1));
    ProcessMarkStack(threadId);
}

void Marker::ProcessOldToNewNoMarkStack(uint32_t threadId)
{
    heap_->EnumerateOldSpaceRegions(std::bind(&Marker::HandleOldToNewRSet, this, threadId, std::placeholders::_1));
}

void Marker::ProcessOldToNew(uint32_t threadId, Region *region)
{
    heap_->EnumerateOldSpaceRegions(std::bind(&Marker::HandleOldToNewRSet, this, threadId, std::placeholders::_1),
                                    region);
    ProcessMarkStack(threadId);
}

void Marker::ProcessSnapshotRSet(uint32_t threadId)
{
    heap_->EnumerateSnapshotSpaceRegions(std::bind(&Marker::HandleOldToNewRSet, this, threadId, std::placeholders::_1));
    ProcessMarkStack(threadId);
}

void Marker::ProcessSnapshotRSetNoMarkStack(uint32_t threadId)
{
    heap_->EnumerateSnapshotSpaceRegions(std::bind(&Marker::HandleOldToNewRSet, this, threadId, std::placeholders::_1));
}

void NonMovableMarker::ProcessMarkStack(uint32_t threadId)
{
    TRACE_GC(GCStats::Scope::ScopeId::ProcessMarkStack, heap_->GetEcmaVM()->GetEcmaGCStats());
    bool isFullMark = heap_->IsFullMark();
    auto visitor = [this, threadId, isFullMark](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                                VisitObjectArea area) {
        Region *rootRegion = Region::ObjectAddressToRange(root);
        bool needBarrier = isFullMark && !rootRegion->InYoungSpaceOrCSet();
        if (area == VisitObjectArea::IN_OBJECT) {
            if (VisitBodyInObj(root, start, end,
                [&](ObjectSlot slot) { MarkValue(threadId, slot, rootRegion, needBarrier); })) {
                return;
            }
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            MarkValue(threadId, slot, rootRegion, needBarrier);
        }
    };
    TaggedObject *obj = nullptr;
    while (true) {
        obj = nullptr;
        if (!workManager_->Pop(threadId, &obj)) {
            break;
        }

        JSHClass *jsHclass = obj->SynchronizedGetClass();
        MarkObject(threadId, jsHclass);
        objXRay_.VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHclass, visitor);
    }
}

void NonMovableMarker::ProcessIncrementalMarkStack(uint32_t threadId, uint32_t markStepSize)
{
    TRACE_GC(GCStats::Scope::ScopeId::ProcessMarkStack, heap_->GetEcmaVM()->GetEcmaGCStats());
    bool isFullMark = heap_->IsFullMark();
    uint32_t visitAddrNum = 0;
    auto visitor = [this, threadId, isFullMark, &visitAddrNum](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                                VisitObjectArea area) {
        Region *rootRegion = Region::ObjectAddressToRange(root);
        visitAddrNum += end.SlotAddress() - start.SlotAddress();
        bool needBarrier = isFullMark && !rootRegion->InYoungSpaceOrCSet();
        if (area == VisitObjectArea::IN_OBJECT) {
            if (VisitBodyInObj(root, start, end,
                [&](ObjectSlot slot) { MarkValue(threadId, slot, rootRegion, needBarrier); })) {
                return;
            }
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            MarkValue(threadId, slot, rootRegion, needBarrier);
        }
    };
    TaggedObject *obj = nullptr;
    double startTime = heap_->GetIncrementalMarker()->GetCurrentTimeInMs();
    double costTime = startTime;
    while (true) {
        obj = nullptr;
        if (!workManager_->Pop(threadId, &obj)) {
            if (heap_->GetJSThread()->IsMarking() && heap_->GetIncrementalMarker()->IsTriggeredIncrementalMark()) {
                costTime = heap_->GetIncrementalMarker()->GetCurrentTimeInMs() - startTime;
                heap_->GetIncrementalMarker()->UpdateMarkingSpeed(visitAddrNum, costTime);
                heap_->GetIncrementalMarker()->SetMarkingFinished(true);
            }
            break;
        }

        JSHClass *jsHclass = obj->GetClass();
        MarkObject(threadId, jsHclass);
        objXRay_.VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHclass, visitor);
        if (heap_->GetIncrementalMarker()->IsTriggeredIncrementalMark() && visitAddrNum >= markStepSize) {
            costTime = heap_->GetIncrementalMarker()->GetCurrentTimeInMs() - startTime;
            heap_->GetIncrementalMarker()->UpdateMarkingSpeed(visitAddrNum, costTime);
            break;
        }
    }
}

void SemiGCMarker::Initialize()
{
    waterLine_ = heap_->GetNewSpace()->GetWaterLine();
}

void SemiGCMarker::ProcessMarkStack(uint32_t threadId)
{
    TRACE_GC(GCStats::Scope::ScopeId::ProcessMarkStack, heap_->GetEcmaVM()->GetEcmaGCStats());
    auto visitor = [this, threadId](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                    VisitObjectArea area) {
        if (area == VisitObjectArea::IN_OBJECT) {
            if (VisitBodyInObj(root, start, end, [&](ObjectSlot slot) { MarkValue(threadId, root, slot); })) {
                return;
            }
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            MarkValue(threadId, root, slot);
        }
    };
    TaggedObject *obj = nullptr;
    while (true) {
        obj = nullptr;
        if (!workManager_->Pop(threadId, &obj)) {
            break;
        }

        auto jsHclass = obj->GetClass();
        objXRay_.VisitObjectBody<VisitType::SEMI_GC_VISIT>(obj, jsHclass, visitor);
    }
}

void CompressGCMarker::ProcessMarkStack(uint32_t threadId)
{
    TRACE_GC(GCStats::Scope::ScopeId::ProcessMarkStack, heap_->GetEcmaVM()->GetEcmaGCStats());
    auto visitor = [this, threadId](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                       VisitObjectArea area) {
        if (area == VisitObjectArea::IN_OBJECT) {
            if (VisitBodyInObj(root, start, end, [&](ObjectSlot slot) { MarkValue(threadId, slot); })) {
                return;
            }
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            MarkValue(threadId, slot);
        }
    };
    TaggedObject *obj = nullptr;
    while (true) {
        obj = nullptr;
        if (!workManager_->Pop(threadId, &obj)) {
            break;
        }

        auto jsHClass = obj->GetClass();
        ObjectSlot objectSlot(ToUintPtr(obj));
        MarkObject(threadId, jsHClass, objectSlot);
        objXRay_.VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHClass, visitor);
    }
}

uintptr_t CompressGCMarker::AllocateForwardAddress(uint32_t threadId, size_t size, JSHClass *hclass,
                                                   TaggedObject *object)
{
    if (!isAppSpawn_) {
        bool isPromoted = true;
        return AllocateDstSpace(threadId, size, isPromoted);
    }
    if (Heap::ShouldMoveToRoSpace(hclass, object)) {
        return AllocateReadOnlySpace(size);
    } else {
        return AllocateAppSpawnSpace(size);
    }
}
}  // namespace panda::ecmascript
