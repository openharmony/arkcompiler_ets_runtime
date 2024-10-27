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
#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/mem/parallel_marker-inl.h"

namespace panda::ecmascript {
Marker::Marker(Heap *heap) : heap_(heap), workManager_(heap->GetWorkManager()) {}

void Marker::MarkRoots(uint32_t threadId, VMRootVisitType type)
{
    TRACE_GC(GCStats::Scope::ScopeId::MarkRoots, heap_->GetEcmaVM()->GetEcmaGCStats());
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::MarkRoots");
    ObjectXRay::VisitVMRoots(
        heap_->GetEcmaVM(),
        [this, threadId](Root type, ObjectSlot slot) {this->HandleRoots(threadId, type, slot);},
        [this, threadId](Root type, ObjectSlot start, ObjectSlot end) {
            this->HandleRangeRoots(threadId, type, start, end);
        },
        [this](Root type, ObjectSlot base, ObjectSlot derived, uintptr_t baseOldObject) {
            this->HandleDerivedRoots(type, base, derived, baseOldObject);
        }, type);
    workManager_->PushWorkNodeToGlobal(threadId, false);
}

void Marker::ProcessNewToEden(uint32_t threadId)
{
    heap_->EnumerateNewSpaceRegions([this, threadId](Region *region) {this->HandleNewToEdenRSet(threadId, region);});
    ProcessMarkStack(threadId);
}

void Marker::ProcessNewToEdenNoMarkStack(uint32_t threadId)
{
    heap_->EnumerateNewSpaceRegions([this, threadId](Region *region) {this->HandleNewToEdenRSet(threadId, region);});
}

void Marker::ProcessOldToNew(uint32_t threadId)
{
    heap_->EnumerateOldSpaceRegions([this, threadId](Region *region) {
        this->HandleOldToNewRSet(threadId, region);
    });
    ProcessMarkStack(threadId);
}

void Marker::ProcessOldToNewNoMarkStack(uint32_t threadId)
{
    heap_->EnumerateOldSpaceRegions([this, threadId](Region *region) {
        this->HandleOldToNewRSet(threadId, region);
    });
}

void Marker::ProcessOldToNew(uint32_t threadId, Region *region)
{
    heap_->EnumerateOldSpaceRegions([this, threadId](Region *region) {
        this->HandleOldToNewRSet(threadId, region);
        }, region);
    ProcessMarkStack(threadId);
}

void Marker::ProcessSnapshotRSet(uint32_t threadId)
{
    heap_->EnumerateSnapshotSpaceRegions([this, threadId](Region *region) {
        this->HandleOldToNewRSet(threadId, region);
    });
    ProcessMarkStack(threadId);
}

void Marker::ProcessSnapshotRSetNoMarkStack(uint32_t threadId)
{
    heap_->EnumerateSnapshotSpaceRegions([this, threadId](Region *region) {
        this->HandleOldToNewRSet(threadId, region);
    });
}

void NonMovableMarker::MarkJitCodeMap(uint32_t threadId)
{
    // To keep MachineCode objects alive (for dump) before JsError object be free, we have to know which JsError is
    // alive first. So this method must be call after all other mark work finish.
    TRACE_GC(GCStats::Scope::ScopeId::MarkRoots, heap_->GetEcmaVM()->GetEcmaGCStats());
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::MarkJitCodeMap");
    if (!heap_->IsFullMark()) {
        return;
    }
    JitCodeMapVisitor visitor = [this, threadId](std::map<JSTaggedType, JitCodeVector *> &jitCodeMaps) {
        auto it = jitCodeMaps.begin();
        while (it != jitCodeMaps.end()) {
            JSTaggedType jsError = it->first;
            Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(jsError));
            if (!objectRegion->Test(reinterpret_cast<TaggedObject *>(jsError))) {
                ++it;
                continue;
            }
            for (auto &jitCodeMap : *(it->second)) {
                auto &jitCode = std::get<0>(jitCodeMap);
                MarkObject(threadId, jitCode);
            }
            ++it;
        }
    };
    ObjectXRay::VisitJitCodeMap(heap_->GetEcmaVM(), visitor);
    ProcessMarkStack(threadId);
    heap_->WaitRunningTaskFinished();
}

void NonMovableMarker::ProcessMarkStack(uint32_t threadId)
{
    bool isFullMark = heap_->IsConcurrentFullMark();
    auto cb = [&](ObjectSlot s, Region *rootRegion, bool needBarrier) {
        MarkValue(threadId, s, rootRegion, needBarrier);
    };
    EcmaObjectRangeVisitor visitor = [this, threadId, isFullMark, cb](TaggedObject *root, ObjectSlot start,
                                                                      ObjectSlot end, VisitObjectArea area) {
        Region *rootRegion = Region::ObjectAddressToRange(root);
        bool needBarrier = isFullMark && !rootRegion->InGeneralNewSpaceOrCSet();
        if (area == VisitObjectArea::IN_OBJECT) {
            if (VisitBodyInObj(root, start, end, needBarrier, cb)) {
                return;
            }
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            MarkValue(threadId, slot, rootRegion, needBarrier);
        }
    };
    SemiSpace *newSpace = heap_->GetNewSpace();
    TaggedObject *obj = nullptr;
    while (workManager_->Pop(threadId, &obj)) {
        Region *region = Region::ObjectAddressToRange(obj);
        if (region->IsHalfFreshRegion()) {
            ASSERT(region->InYoungSpace());
            if (newSpace->IsFreshObjectInHalfFreshRegion(obj)) {
                // Fresh object do not need to visit body.
                continue;
            }
        }

        JSHClass *jsHclass = obj->SynchronizedGetClass();
        ASSERT(!region->IsFreshRegion());
        auto size = jsHclass->SizeFromJSHClass(obj);
        region->IncreaseAliveObjectSafe(size);
        MarkObject(threadId, jsHclass);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHclass, visitor);
    }
}

void NonMovableMarker::ProcessIncrementalMarkStack(uint32_t threadId, uint32_t markStepSize)
{
    bool isFullMark = heap_->IsConcurrentFullMark();
    uint32_t visitAddrNum = 0;
    auto cb = [&](ObjectSlot s, Region *rootRegion, bool needBarrier) {
        MarkValue(threadId, s, rootRegion, needBarrier);
    };
    EcmaObjectRangeVisitor visitor = [this, threadId, isFullMark, &visitAddrNum, cb](TaggedObject *root,
                                                                                     ObjectSlot start,
                                                                                     ObjectSlot end,
                                                                                     VisitObjectArea area) {
        Region *rootRegion = Region::ObjectAddressToRange(root);
        visitAddrNum += end.SlotAddress() - start.SlotAddress();
        bool needBarrier = isFullMark && !rootRegion->InGeneralNewSpaceOrCSet();
        if (area == VisitObjectArea::IN_OBJECT) {
            if (VisitBodyInObj(root, start, end, needBarrier, cb)) {
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
    while (workManager_->Pop(threadId, &obj)) {
        JSHClass *jsHclass = obj->GetClass();
        Region *region = Region::ObjectAddressToRange(obj);
        auto size = jsHclass->SizeFromJSHClass(obj);
        region->IncreaseAliveObjectSafe(size);
        MarkObject(threadId, jsHclass);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHclass, visitor);
        if (heap_->GetIncrementalMarker()->IsTriggeredIncrementalMark() && visitAddrNum >= markStepSize) {
            costTime = heap_->GetIncrementalMarker()->GetCurrentTimeInMs() - startTime;
            heap_->GetIncrementalMarker()->UpdateMarkingSpeed(visitAddrNum, costTime);
            return;
        }
    }
    if (heap_->GetJSThread()->IsMarking() && heap_->GetIncrementalMarker()->IsTriggeredIncrementalMark()) {
        costTime = heap_->GetIncrementalMarker()->GetCurrentTimeInMs() - startTime;
        heap_->GetIncrementalMarker()->UpdateMarkingSpeed(visitAddrNum, costTime);
        heap_->GetIncrementalMarker()->SetMarkingFinished(true);
    }
}

void SemiGCMarker::Initialize()
{
    waterLine_ = heap_->GetNewSpace()->GetWaterLine();
}

void SemiGCMarker::ProcessMarkStack(uint32_t threadId)
{
    auto cb = [&](ObjectSlot s, TaggedObject *root) { MarkValue(threadId, root, s); };
    EcmaObjectRangeVisitor visitor = [this, threadId, cb](TaggedObject *root, ObjectSlot start,
                                                          ObjectSlot end, VisitObjectArea area) {
        if (area == VisitObjectArea::IN_OBJECT) {
            if (VisitBodyInObj(root, start, end, cb)) {
                return;
            }
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            MarkValue(threadId, root, slot);
        }
    };
    TaggedObject *obj = nullptr;
    while (workManager_->Pop(threadId, &obj)) {
        auto jsHclass = obj->GetClass();
        ObjectXRay::VisitObjectBody<VisitType::SEMI_GC_VISIT>(obj, jsHclass, visitor);
    }
}

void CompressGCMarker::ProcessMarkStack(uint32_t threadId)
{
    auto cb = [&](ObjectSlot s, [[maybe_unused]] TaggedObject *root) { MarkValue(threadId, s); };
    EcmaObjectRangeVisitor visitor = [this, threadId, cb](TaggedObject *root, ObjectSlot start,
                                                          ObjectSlot end, VisitObjectArea area) {
        if (area == VisitObjectArea::IN_OBJECT) {
            if (VisitBodyInObj(root, start, end, cb)) {
                return;
            }
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            MarkValue(threadId, slot);
        }
    };
    TaggedObject *obj = nullptr;
    while (workManager_->Pop(threadId, &obj)) {
        auto jsHClass = obj->GetClass();
        ObjectSlot objectSlot(ToUintPtr(obj));
        MarkObject(threadId, jsHClass, objectSlot);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHClass, visitor);
    }
}

void CompressGCMarker::MarkJitCodeMap(uint32_t threadId)
{
    // To keep MachineCode objects alive (for dump) before JsError object be free, we have to know which JsError is
    // alive first. So this method must be call after all other mark work finish.
    TRACE_GC(GCStats::Scope::ScopeId::MarkRoots, heap_->GetEcmaVM()->GetEcmaGCStats());
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::MarkJitCodeMap");
    ObjectXRay::VisitJitCodeMap(heap_->GetEcmaVM(),
        [this, threadId](std::map<JSTaggedType, JitCodeVector *> &jitCodeMaps) {
            this->HandleVisitJitCodeMap(threadId, jitCodeMaps);
        });
    ProcessMarkStack(threadId);
    heap_->WaitRunningTaskFinished();
}

void CompressGCMarker::HandleVisitJitCodeMap(uint32_t threadId, std::map<JSTaggedType, JitCodeVector *> &jitCodeMaps)
{
    auto it = jitCodeMaps.begin();
    while (it != jitCodeMaps.end()) {
        JSTaggedType jsError = it->first;
        auto jsErrorObj = reinterpret_cast<TaggedObject *>(jsError);
        Region *objectRegion = Region::ObjectAddressToRange(jsErrorObj);
        auto jitCodeVec = it->second;
        if (!NeedEvacuate(objectRegion)) {
            if (!objectRegion->InSharedHeap() && !objectRegion->Test(jsErrorObj)) {
                delete it->second;
                it = jitCodeMaps.erase(it);
                continue;
            }
        } else {
            MarkWord markWord(jsErrorObj);
            if (markWord.IsForwardingAddress()) {
                TaggedObject *dst = markWord.ToForwardingAddress();
                jitCodeMaps.emplace(JSTaggedValue(dst).GetRawData(), it->second);
                it = jitCodeMaps.erase(it);
            } else {
                delete it->second;
                it = jitCodeMaps.erase(it);
                continue;
            }
        }
        for (auto &jitCodeMap : *jitCodeVec) {
            auto &jitCode = std::get<0>(jitCodeMap);
            auto obj = static_cast<TaggedObject *>(jitCode);
            // jitcode is MachineCode, and MachineCode is in the MachineCode space, will not be evacute.
            MarkObject(threadId, obj, ObjectSlot(reinterpret_cast<uintptr_t>(&jitCode)));
        }
        ++it;
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
