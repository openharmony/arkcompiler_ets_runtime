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

#include "ecmascript/mem/cms_mem/sweep_gc_visitor-inl.h"
#include "ecmascript/mem/local_cmc/cc_gc_visitor-inl.h"
#include "ecmascript/mem/old_gc_visitor-inl.h"
#include "ecmascript/mem/young_gc_visitor-inl.h"
#include "ecmascript/mem/full_gc-inl.h"

namespace panda::ecmascript {
Marker::Marker(Heap *heap) : heap_(heap), workManager_(heap->GetWorkManager()) {}

void Marker::MarkRoots(RootVisitor &rootVisitor)
{
    TRACE_GC(GCStats::Scope::ScopeId::MarkRoots, heap_->GetEcmaVM()->GetEcmaGCStats());
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "GC::MarkRoots", "");
    ObjectXRay::VisitVMRoots(heap_->GetEcmaVM(), rootVisitor);
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
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "GC::MarkJitCodeMap", "");
    if (heap_->IsYoungMark()) {
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
    // fixme: refactor?
    if constexpr (G_USE_CMS_GC) {
        ProcessCMSGCMarkStack(threadId);
        return;
    }
    if (heap_->IsYoungMark()) {
        ProcessYoungGCMarkStack(threadId);
    } else if (heap_->IsFullMark()) {
        ProcessOldGCMarkStack(threadId);
    } else {
        ASSERT(heap_->IsCCMark());
        ProcessCCGCMarkStack(threadId);
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

void NonMovableMarker::ProcessCMSGCMarkStack(uint32_t threadId)
{
    WorkNodeHolder *workNodeHolder = workManager_->GetWorkNodeHolder(threadId);
    SweepGCMarkObjectVisitor sweepGCMarkObjectVisitor(workNodeHolder);
    SlotSpace *slotSpace = heap_->GetSlotSpace();
    TaggedObject *obj = nullptr;
    while (workNodeHolder->Pop(&obj)) {
        Region *region = Region::ObjectAddressToRange(obj);

        JSHClass *jsHclass = obj->SynchronizedGetClass();
        size_t size = jsHclass->SizeFromJSHClass(obj);
        region->IncreaseAliveObject(size);

        sweepGCMarkObjectVisitor.VisitHClass(jsHclass);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHclass, sweepGCMarkObjectVisitor);
    }
}

void NonMovableMarker::ProcessCCGCMarkStack(uint32_t threadId)
{
    WorkNodeHolder *workNodeHolder = workManager_->GetWorkNodeHolder(threadId);
    CCMarkObjectVisitor ccMarkObjectVisitor(workNodeHolder);
    SemiSpace *newSpace = heap_->GetNewSpace();
    TaggedObject *obj = nullptr;
    while (workNodeHolder->Pop(&obj)) {
        Region *region = Region::ObjectAddressToRange(obj);

        auto jsHClass = obj->GetClass();
        auto size = jsHClass->SizeFromJSHClass(obj);
        region->IncreaseAliveObject(size);
        ccMarkObjectVisitor.VisitObjectHClassImpl(jsHClass);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHClass, ccMarkObjectVisitor);
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
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "GC::MarkJitCodeMap", "");
    FullGCRunner fullGCRunner(heap_, workManager_->GetWorkNodeHolder(threadId), isAppSpawn_);
    JitCodeMapVisitor visitor = [&fullGCRunner] (std::map<JSTaggedType, JitCodeVector *> &jitCodeMaps) {
        std::map<JSTaggedType, JitCodeVector *> tempVec;
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
                } else {
                    fullGCRunner.MarkJitCodeVec(jitCodeVec);
                    ++it;
                }
            } else {
                MarkWord markWord(jsErrorObj);
                if (markWord.IsForwardingAddress()) {
                    TaggedObject *dst = markWord.ToForwardingAddress();
                    tempVec.emplace(JSTaggedValue(dst).GetRawData(), it->second);
                    it = jitCodeMaps.erase(it);
                    fullGCRunner.MarkJitCodeVec(jitCodeVec);
                } else {
                    delete it->second;
                    it = jitCodeMaps.erase(it);
                }
            }
        }
        jitCodeMaps.insert(tempVec.begin(), tempVec.end());
    };
    ObjectXRay::VisitJitCodeMap(heap_->GetEcmaVM(), visitor);
    ProcessMarkStack(threadId);
    heap_->WaitRunningTaskFinished();
}
}  // namespace panda::ecmascript
