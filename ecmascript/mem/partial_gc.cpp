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

#include "ecmascript/mem/partial_gc.h"

#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/mem/parallel_evacuator.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/runtime_call_id.h"
#include "ecmascript/mem/verification.h"

namespace panda::ecmascript {
PartialGC::PartialGC(Heap *heap) : heap_(heap), workManager_(heap->GetWorkManager()) {}

void PartialGC::RunPhases()
{
    GCStats *gcStats = heap_->GetEcmaVM()->GetEcmaGCStats();
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PartialGC::RunPhases" + std::to_string(heap_->IsConcurrentFullMark())
        + ";Reason" + std::to_string(static_cast<int>(gcStats->GetGCReason()))
        + ";Sensitive" + std::to_string(static_cast<int>(heap_->GetSensitiveStatus()))
        + ";IsInBackground" + std::to_string(heap_->IsInBackground())
        + ";Startup" + std::to_string(heap_->OnStartupEvent())
        + ";ConMark" + std::to_string(static_cast<int>(heap_->GetJSThread()->GetMarkStatus()))
        + ";Young" + std::to_string(heap_->GetNewSpace()->GetCommittedSize())
        + ";Old" + std::to_string(heap_->GetOldSpace()->GetCommittedSize())
        + ";TotalCommit" + std::to_string(heap_->GetCommittedSize()));
    TRACE_GC(GCStats::Scope::ScopeId::TotalGC, gcStats);
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), PartialGC_RunPhases);
    bool mainThreadInForeground = heap_->GetJSThread()->IsMainThreadFast() && !heap_->IsInBackground();
    bool needAjustGCThreadPrio = heap_->GetGCType() == TriggerGCType::OLD_GC ||
        heap_->GetNewSpace()->GetCommittedSize() >= heap_->GetNewSpace()->GetMaximumCapacity();
    if (mainThreadInForeground && needAjustGCThreadPrio) {
        Taskpool::GetCurrentTaskpool()->SetThreadPriority(PriorityMode::STW);
    }
    markingInProgress_ = heap_->CheckOngoingConcurrentMarking();
    LOG_GC(DEBUG) << "markingInProgress_" << markingInProgress_;
    Initialize();
    Mark();
    if (UNLIKELY(heap_->ShouldVerifyHeap())) {
        Verification::VerifyMark(heap_);
    }
    ProcessSharedGCRSetWorkList();
    Sweep();
    Evacuate();
    if (heap_->IsConcurrentFullMark()) {
        heap_->GetSweeper()->PostTask();
    }
    if (UNLIKELY(heap_->ShouldVerifyHeap())) {
        Verification::VerifyEvacuate(heap_);
    }
    Finish();
    if (mainThreadInForeground && needAjustGCThreadPrio) {
        Taskpool::GetCurrentTaskpool()->SetThreadPriority(PriorityMode::FOREGROUND);
    }
}

void PartialGC::Initialize()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PartialGC::Initialize");
    TRACE_GC(GCStats::Scope::ScopeId::Initialize, heap_->GetEcmaVM()->GetEcmaGCStats());
    if (!markingInProgress_ && !heap_->GetIncrementalMarker()->IsTriggeredIncrementalMark()) {
        LOG_GC(DEBUG) << "No ongoing Concurrent marking. Initializing...";
        heap_->Prepare();
        if (heap_->IsConcurrentFullMark()) {
            heap_->GetOldSpace()->SelectCSet();
            heap_->GetAppSpawnSpace()->EnumerateRegions([](Region *current) {
                current->ClearMarkGCBitset();
                current->ClearCrossRegionRSet();
            });
            heap_->EnumerateNonNewSpaceRegions([](Region *current) {
                current->ResetAliveObject();
            });
        }
        workManager_->Initialize(TriggerGCType::OLD_GC, ParallelGCTaskPhase::OLD_HANDLE_GLOBAL_POOL_TASK);

        freeSize_ = 0;
        hugeSpaceFreeSize_ = 0;
        oldSpaceCommitSize_ = heap_->GetOldSpace()->GetCommittedSize();
        nonMoveSpaceCommitSize_ = heap_->GetNonMovableSpace()->GetCommittedSize();
    }
}

void PartialGC::Finish()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PartialGC::Finish");
    TRACE_GC(GCStats::Scope::ScopeId::Finish, heap_->GetEcmaVM()->GetEcmaGCStats());
    heap_->Resume(OLD_GC);
    if (heap_->GetIncrementalMarker()->IsTriggeredIncrementalMark()) {
        heap_->GetIncrementalMarker()->Reset();
    } else if (markingInProgress_) {
        auto marker = heap_->GetConcurrentMarker();
        marker->Reset(false);
    } else {
        workManager_->Finish();
    }
    if (heap_->IsConcurrentFullMark()) {
        heap_->GetSweeper()->TryFillSweptRegion();
    }
}

void PartialGC::Mark()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PartialGC::Mark");
    TRACE_GC(GCStats::Scope::ScopeId::Mark, heap_->GetEcmaVM()->GetEcmaGCStats());
    if (markingInProgress_) {
        heap_->GetConcurrentMarker()->ReMark();
        return;
    }
    heap_->GetNonMovableMarker()->MarkRoots(MAIN_THREAD_INDEX);
    if (heap_->IsConcurrentFullMark()) {
        heap_->GetNonMovableMarker()->ProcessMarkStack(MAIN_THREAD_INDEX);
    } else if (heap_->IsEdenMark()) {
        {
            ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::ProcessOldToNew");
            heap_->GetNonMovableMarker()->ProcessOldToNew(MAIN_THREAD_INDEX);
        }
        {
            ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::ProcessNewToEden");
            heap_->GetNonMovableMarker()->ProcessNewToEden(MAIN_THREAD_INDEX);
        }
        heap_->GetNonMovableMarker()->ProcessSnapshotRSet(MAIN_THREAD_INDEX);
    } else if (heap_->IsYoungMark()) {
        {
            ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::ProcessOldToNew");
            heap_->GetNonMovableMarker()->ProcessOldToNew(MAIN_THREAD_INDEX);
        }
        heap_->GetNonMovableMarker()->ProcessSnapshotRSet(MAIN_THREAD_INDEX);
    }
    heap_->WaitRunningTaskFinished();
    // MarkJitCodeMap must be call after other mark work finish to make sure which jserror object js alive.
    heap_->GetNonMovableMarker()->MarkJitCodeMap(MAIN_THREAD_INDEX);
}

void PartialGC::Sweep()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PartialGC::Sweep");
    ProcessNativeDelete();
    if (heap_->IsConcurrentFullMark()) {
        heap_->GetOldSpace()->EnumerateRegions([](Region *current) {
            current->SetRegionAliveSize();
        });
        TRACE_GC(GCStats::Scope::ScopeId::Sweep, heap_->GetEcmaVM()->GetEcmaGCStats());
        heap_->GetSweeper()->Sweep();
    }
}

void PartialGC::ProcessNativeDelete()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "GC::ProcessNativeDelete");
    TRACE_GC(GCStats::Scope::ScopeId::ClearNativeObject, heap_->GetEcmaVM()->GetEcmaGCStats());
    WeakRootVisitor gcUpdateWeak = [this](TaggedObject *header) -> TaggedObject* {
        Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(header));
        ASSERT(!objectRegion->InSharedHeap());
        if (heap_->IsEdenMark() && !objectRegion->InEdenSpace()) {
            return header;
        }
        if (!objectRegion->InGeneralNewSpaceOrCSet() && heap_->IsYoungMark()) {
            return header;
        }
        if (!objectRegion->Test(header)) {
            return nullptr;
        }
        return header;
    };
    heap_->GetEcmaVM()->ProcessNativeDelete(gcUpdateWeak);
}

void PartialGC::Evacuate()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PartialGC::Evacuate");
    TRACE_GC(GCStats::Scope::ScopeId::Evacuate, heap_->GetEcmaVM()->GetEcmaGCStats());
    heap_->GetEvacuator()->Evacuate();
}

ARK_INLINE void PartialGC::ProcessSharedGCRSetWorkList()
{
    TRACE_GC(GCStats::Scope::ScopeId::ProcessSharedGCRSetWorkList, heap_->GetEcmaVM()->GetEcmaGCStats());
    heap_->ProcessSharedGCRSetWorkList();
}
}  // namespace panda::ecmascript
