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

#include "ecmascript/mem/cms_mem/sticky_sweep_gc.h"

#include "common_components/taskpool/taskpool.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/parallel_evacuator.h"
#include "ecmascript/mem/parallel_marker.h"
#include "ecmascript/mem/cms_mem/sweep_gc_visitor-inl.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/mem/verification.h"

namespace panda::ecmascript {

StickySweepGC::StickySweepGC(Heap *heap) : SweepGC(heap)
{
}

void StickySweepGC::RunPhases()
{
    ASSERT("StickySweepGC should be disabled" && G_USE_STICKY_CMS_GC);
    LOG_GC(INFO) << "StickySweepGC triggered, mark status " << static_cast<int>(heap_->GetJSThread()->GetMarkStatus());
    GCStats *gcStats = heap_->GetEcmaVM()->GetEcmaGCStats();
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
        ("StickySweepGC::RunPhases" + std::to_string(heap_->IsConcurrentFullMark())
        + ";GCReason" + std::to_string(static_cast<int>(gcStats->GetGCReason()))
        + ";MarkReason" + std::to_string(static_cast<int>(gcStats->GetMarkReason()))
        + ";Sensitive" + std::to_string(static_cast<int>(heap_->GetSensitiveStatus()))
        + ";IsInBackground" + std::to_string(Runtime::GetInstance()->IsInBackground())
        + ";Startup" + std::to_string(static_cast<int>(heap_->GetStartupStatus()))
        + ";ConMark" + std::to_string(static_cast<int>(heap_->GetJSThread()->GetMarkStatus()))
        + ";Regular" + std::to_string(heap_->GetSlotSpace()->GetCommittedSize())
        + ";Huge" + std::to_string(heap_->GetHugeObjectSpace()->GetCommittedSize())
        + ";TotalCommit" + std::to_string(heap_->GetCommittedSize())
        + ";NativeBindingSize" + std::to_string(heap_->GetNativeBindingSize())
        + ";NativeLimitSize" + std::to_string(heap_->GetGlobalSpaceNativeLimit())
        + ";ObjSizeBeforeSensitive" + std::to_string(heap_->GetRecordHeapObjectSizeBeforeSensitive())).c_str(), "");
    TRACE_GC(GCStats::Scope::ScopeId::TotalGC, gcStats);
    // fixme: support report statistic
    markingInProgress_ = heap_->CheckOngoingConcurrentMarking();
    LOG_GC(DEBUG) << "markingInProgress_" << markingInProgress_;
    Initialize();

    Mark();
    // fixme: support sticky verify mark

    ProcessSharedGCRSetWorkList();

    Sweep();
    ClearDeadReferences();

    // fixme: support sticky verify evacuate
    Finish();
}

void StickySweepGC::Initialize()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "StickySweepGC::Initialize", "");
    TRACE_GC(GCStats::Scope::ScopeId::Initialize, heap_->GetEcmaVM()->GetEcmaGCStats());
    if (!markingInProgress_) {
        LOG_GC(DEBUG) << "No ongoing Concurrent marking. Initializing...";
        heap_->Prepare();
        workManager_->Initialize(TriggerGCType::STICKY_CMS_GC, ParallelGCTaskPhase::HANDLE_GLOBAL_POOL_TASK);
    }
}

void StickySweepGC::MarkRoots()
{
    SweepGCMarkRootVisitor sweepGCMarkRootVisitor(workManager_->GetWorkNodeHolder(MAIN_THREAD_INDEX));
    heap_->GetNonMovableMarker()->MarkRoots(sweepGCMarkRootVisitor, GlobalVisitType::YOUNG_GLOBAL_VISIT);
}

void StickySweepGC::Mark()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "StickySweepGC::Mark", "");
    TRACE_GC(GCStats::Scope::ScopeId::Mark, heap_->GetEcmaVM()->GetEcmaGCStats());
    if (markingInProgress_) {
        heap_->GetConcurrentMarker()->ReMark();
        return;
    }
    MarkRoots();
    workManager_->GetWorkNodeHolder(MAIN_THREAD_INDEX)->FlushAll();
    NonMovableMarker *marker = static_cast<NonMovableMarker*>(heap_->GetNonMovableMarker());
    {
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "StickySweepGC::ProcessOldToNew", "");
        marker->ProcessOldToNewForSticky(MAIN_THREAD_INDEX);
    }
    marker->ProcessSnapshotRSetForSticky(MAIN_THREAD_INDEX);
    heap_->GetNonMovableMarker()->ProcessMarkStack(MAIN_THREAD_INDEX);
    heap_->WaitRunningMarkTaskFinished();
    // MarkJitCodeMap must be call after other mark work finish to make sure which jserror object js alive.
    heap_->GetNonMovableMarker()->MarkJitCodeMap(MAIN_THREAD_INDEX);
}

void StickySweepGC::ProcessNativeDelete()
{
    // Todo fixme, Sticky not handle nativepointer
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "StickySweepGC::ProcessNativeDelete", "");
    TRACE_GC(GCStats::Scope::ScopeId::ClearNativeObject, heap_->GetEcmaVM()->GetEcmaGCStats());
    WeakRootVisitor gcUpdateWeak = [this](TaggedObject *header) -> TaggedObject* {
        Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(header));
        ASSERT(!objectRegion->InSharedHeap());
        if (!objectRegion->Test(header)) {
            return nullptr;
        }
        return header;
    };
    heap_->GetEcmaVM()->ProcessNativeDelete(gcUpdateWeak);
}

void StickySweepGC::Sweep()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "StickySweepGC::Sweep", "");
    ProcessNativeDelete();
    heap_->EnumerateRegions([](Region *current) {
        current->ClearOldToNewRSet();
    });
    TRACE_GC(GCStats::Scope::ScopeId::Sweep, heap_->GetEcmaVM()->GetEcmaGCStats());
    heap_->GetSweeper()->Sweep(TriggerGCType::STICKY_CMS_GC);
    heap_->GetSweeper()->PostTask(TriggerGCType::STICKY_CMS_GC);
    heap_->GetJSThread()->ClearYoungGlobalList();
    heap_->GetJSThread()->ClearToBeDeletedNodes();
}
}  // namespace panda::ecmascript
