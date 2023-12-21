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

#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/clock_scope.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/parallel_evacuator.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/ecma_string_table.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
PartialGC::PartialGC(Heap *heap) : heap_(heap), workManager_(heap->GetWorkManager()) {}

void PartialGC::RunPhases()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PartialGC::RunPhases" + std::to_string(heap_->IsFullMark()));
    TRACE_GC(GCStats::Scope::ScopeId::TotalGC, heap_->GetEcmaVM()->GetEcmaGCStats());
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), PartialGC_RunPhases);

    markingInProgress_ = heap_->CheckOngoingConcurrentMarking();

    LOG_GC(DEBUG) << "markingInProgress_" << markingInProgress_;
    Initialize();
    Mark();
    Sweep();
    Evacuate();
    if (heap_->IsFullMark()) {
        heap_->GetSweeper()->PostTask();
    }
    Finish();
    if (heap_->IsFullMark()) {
        heap_->NotifyHeapAliveSizeAfterGC(heap_->GetHeapObjectSize());
    }
}

void PartialGC::Initialize()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PartialGC::Initialize");
    TRACE_GC(GCStats::Scope::ScopeId::Initialize, heap_->GetEcmaVM()->GetEcmaGCStats());
    if (!markingInProgress_ && !heap_->GetIncrementalMarker()->IsTriggeredIncrementalMark()) {
        LOG_GC(DEBUG) << "No ongoing Concurrent marking. Initializing...";
        heap_->Prepare();
        if (heap_->IsFullMark()) {
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
    if (heap_->IsFullMark()) {
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
    if (heap_->IsFullMark()) {
        heap_->GetNonMovableMarker()->ProcessMarkStack(MAIN_THREAD_INDEX);
    } else {
        heap_->GetNonMovableMarker()->ProcessOldToNew(MAIN_THREAD_INDEX);
        heap_->GetNonMovableMarker()->ProcessSnapshotRSet(MAIN_THREAD_INDEX);
    }
    heap_->WaitRunningTaskFinished();
}

void PartialGC::Sweep()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "PartialGC::Sweep");
    ProcessNativeDelete();
    if (heap_->IsFullMark()) {
        heap_->GetOldSpace()->EnumerateRegions([](Region *current) {
            current->SetRegionAliveSize();
        });
        TRACE_GC(GCStats::Scope::ScopeId::Sweep, heap_->GetEcmaVM()->GetEcmaGCStats());
        heap_->GetSweeper()->Sweep();
    }
}

void PartialGC::ProcessNativeDelete()
{
    TRACE_GC(GCStats::Scope::ScopeId::ClearNativeObject, heap_->GetEcmaVM()->GetEcmaGCStats());
    WeakRootVisitor gcUpdateWeak = [this](TaggedObject *header) {
        Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(header));
        if (!objectRegion->InYoungSpaceOrCSet() && !heap_->IsFullMark()) {
            return header;
        }
        if (!objectRegion->Test(header)) {
            return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
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
}  // namespace panda::ecmascript
