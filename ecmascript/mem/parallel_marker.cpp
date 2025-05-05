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

#include "ecmascript/mem/parallel_marker.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/object_xray.h"

#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/mem/incremental_gc_visitor-inl.h"
#include "ecmascript/mem/old_gc_visitor-inl.h"
#include "ecmascript/mem/young_gc_visitor-inl.h"
#include "ecmascript/mem/full_gc-inl.h"

namespace panda::ecmascript {
Marker::Marker(Heap *heap) : heap_(heap), workManager_(heap->GetWorkManager()) {}

void Marker::MarkRoots(RootVisitor &rootVisitor, VMRootVisitType type)
{
    TRACE_GC(GCStats::Scope::ScopeId::MarkRoots, heap_->GetEcmaVM()->GetEcmaGCStats());
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::MarkRoots");
    ObjectXRay::VisitVMRoots(heap_->GetEcmaVM(), rootVisitor, type);
}

void NonMovableMarker::ProcessOldToNew(uint32_t threadId)
{
    ASSERT(heap_->IsYoungMark());
    YoungGCMarkOldToNewRSetVisitor youngGCMarkOldToNewRSetVisitor(workManager_->GetWorkNodeHolder(threadId));
    heap_->EnumerateOldSpaceRegions(youngGCMarkOldToNewRSetVisitor);
    ProcessMarkStack(threadId);
}

void NonMovableMarker::ProcessOldToNewNoMarkStack(uint32_t threadId)
{
    ASSERT(heap_->IsYoungMark());
    YoungGCMarkOldToNewRSetVisitor youngGCMarkOldToNewRSetVisitor(workManager_->GetWorkNodeHolder(threadId));
    heap_->EnumerateOldSpaceRegions(youngGCMarkOldToNewRSetVisitor);
}

void NonMovableMarker::ProcessSnapshotRSet(uint32_t threadId)
{
    ASSERT(heap_->IsYoungMark());
    YoungGCMarkOldToNewRSetVisitor youngGCMarkOldToNewRSetVisitor(workManager_->GetWorkNodeHolder(threadId));
    heap_->EnumerateSnapshotSpaceRegions(youngGCMarkOldToNewRSetVisitor);
    ProcessMarkStack(threadId);
}

void NonMovableMarker::ProcessSnapshotRSetNoMarkStack(uint32_t threadId)
{
    ASSERT(heap_->IsYoungMark());
    YoungGCMarkOldToNewRSetVisitor youngGCMarkOldToNewRSetVisitor(workManager_->GetWorkNodeHolder(threadId));
    heap_->EnumerateSnapshotSpaceRegions(youngGCMarkOldToNewRSetVisitor);
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
    OldGCMarkObjectVisitor objectVisitor(workManager_->GetWorkNodeHolder(threadId));
    JitCodeMapVisitor visitor = [&objectVisitor](std::map<JSTaggedType, JitCodeVector *> &jitCodeMaps) {
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
                objectVisitor.HandleObject(jitCode, Region::ObjectAddressToRange(jitCode));
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
    if (heap_->IsYoungMark()) {
        ProcessYoungGCMarkStack(threadId);
    } else {
        ProcessOldGCMarkStack(threadId);
    }
}

void NonMovableMarker::ProcessYoungGCMarkStack(uint32_t threadId)
{
    WorkNodeHolder *workNodeHolder = workManager_->GetWorkNodeHolder(threadId);
    YoungGCMarkObjectVisitor youngGCMarkObjectVisitor(workNodeHolder);
    SemiSpace *newSpace = heap_->GetNewSpace();
    TaggedObject *obj = nullptr;
    while (workNodeHolder->Pop(&obj)) {
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
        region->IncreaseAliveObject(size);

        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHclass, youngGCMarkObjectVisitor);
    }
}

void NonMovableMarker::ProcessOldGCMarkStack(uint32_t threadId)
{
    WorkNodeHolder *workNodeHolder = workManager_->GetWorkNodeHolder(threadId);
    OldGCMarkObjectVisitor oldGCMarkObjectVisitor(workNodeHolder);
    SemiSpace *newSpace = heap_->GetNewSpace();
    TaggedObject *obj = nullptr;
    while (workNodeHolder->Pop(&obj)) {
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
        region->IncreaseAliveObject(size);

        oldGCMarkObjectVisitor.VisitHClass(jsHclass);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHclass, oldGCMarkObjectVisitor);
    }
}

void NonMovableMarker::ProcessIncrementalMarkStack(uint32_t threadId, uint32_t markStepSize)
{
    ASSERT(heap_->IsConcurrentFullMark());
    WorkNodeHolder *workNodeHolder = workManager_->GetWorkNodeHolder(threadId);
    uint32_t visitAddrNum = 0;
    IncrementalOldGCMarkObjectVisitor incrementalOldGCMarkObjectVisitor(workNodeHolder, visitAddrNum);
    TaggedObject *obj = nullptr;
    double startTime = heap_->GetIncrementalMarker()->GetCurrentTimeInMs();
    double costTime = startTime;
    while (workNodeHolder->Pop(&obj)) {
        JSHClass *jsHclass = obj->GetClass();
        Region *region = Region::ObjectAddressToRange(obj);
        auto size = jsHclass->SizeFromJSHClass(obj);
        region->IncreaseAliveObject(size);
        incrementalOldGCMarkObjectVisitor.VisitHClass(jsHclass);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHclass, incrementalOldGCMarkObjectVisitor);
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

void CompressGCMarker::ProcessMarkStack(uint32_t threadId)
{
    WorkNodeHolder *workNodeHolder = workManager_->GetWorkNodeHolder(threadId);
    FullGCRunner fullGCRunner(heap_, workNodeHolder, isAppSpawn_);
    FullGCMarkObjectVisitor &fullGCMarkObjectVisitor = fullGCRunner.GetMarkObjectVisitor();
    TaggedObject *obj = nullptr;
    while (workNodeHolder->Pop(&obj)) {
        auto jsHClass = obj->GetClass();
        ObjectSlot hClassSlot(ToUintPtr(obj));
        fullGCMarkObjectVisitor.VisitHClassSlot(hClassSlot, jsHClass);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHClass, fullGCMarkObjectVisitor);
    }
}

void CompressGCMarker::MarkJitCodeMap(uint32_t threadId)
{
    // To keep MachineCode objects alive (for dump) before JsError object be free, we have to know which JsError is
    // alive first. So this method must be call after all other mark work finish.
    TRACE_GC(GCStats::Scope::ScopeId::MarkRoots, heap_->GetEcmaVM()->GetEcmaGCStats());
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::MarkJitCodeMap");
    FullGCRunner fullGCRunner(heap_, workManager_->GetWorkNodeHolder(threadId), isAppSpawn_);
    JitCodeMapVisitor visitor = [&fullGCRunner] (std::map<JSTaggedType, JitCodeVector *> &jitCodeMaps) {
        auto it = jitCodeMaps.begin();
        while (it != jitCodeMaps.end()) {
            JSTaggedType jsError = it->first;
            auto jsErrorObj = reinterpret_cast<TaggedObject *>(jsError);
            Region *objectRegion = Region::ObjectAddressToRange(jsErrorObj);
            auto jitCodeVec = it->second;
            if (!fullGCRunner.NeedEvacuate(objectRegion)) {
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
                fullGCRunner.HandleMarkingSlotObject(ObjectSlot(reinterpret_cast<uintptr_t>(&jitCode)), obj);
            }
            ++it;
        }
    };
    ObjectXRay::VisitJitCodeMap(heap_->GetEcmaVM(), visitor);
    ProcessMarkStack(threadId);
    heap_->WaitRunningTaskFinished();
}
}  // namespace panda::ecmascript
