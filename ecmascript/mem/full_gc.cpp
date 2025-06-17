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

#include "ecmascript/mem/full_gc-inl.h"

#include "common_components/taskpool/taskpool.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/incremental_marker.h"
#include "ecmascript/mem/parallel_marker.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {
FullGC::FullGC(Heap *heap) : heap_(heap), workManager_(heap->GetWorkManager()) {}

void FullGC::RunPhases()
{
    ASSERT("FullGC should be disabled" && !g_isEnableCMCGC);
    GCStats *gcStats = heap_->GetEcmaVM()->GetEcmaGCStats();
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, ("FullGC::RunPhases;GCReason"
        + std::to_string(static_cast<int>(gcStats->GetGCReason()))
        + ";Sensitive" + std::to_string(static_cast<int>(heap_->GetSensitiveStatus()))
        + ";IsInBackground" + std::to_string(heap_->IsInBackground())
        + ";Startup" + std::to_string(static_cast<int>(heap_->GetStartupStatus()))
        + ";Young" + std::to_string(heap_->GetNewSpace()->GetCommittedSize())
        + ";Old" + std::to_string(heap_->GetOldSpace()->GetCommittedSize())
        + ";huge" + std::to_string(heap_->GetHugeObjectSpace()->GetCommittedSize())
        + ";NonMov" + std::to_string(heap_->GetNonMovableSpace()->GetCommittedSize())
        + ";TotCommit" + std::to_string(heap_->GetCommittedSize())
        + ";ObjSizeBeforeSensitive"
        + std::to_string(heap_->GetRecordHeapObjectSizeBeforeSensitive())).c_str(), "");
    TRACE_GC(GCStats::Scope::ScopeId::TotalGC, gcStats);
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), FullGC_RunPhases);

    if (heap_->CheckOngoingConcurrentMarking()) {
        LOG_GC(DEBUG) << "FullGC after ConcurrentMarking";
        heap_->GetConcurrentMarker()->Reset();  // HPPGC use mark result to move TaggedObject.
    }

    if (heap_->GetIncrementalMarker()->IsTriggeredIncrementalMark()) {
        LOG_GC(DEBUG) << "FullGC after IncrementalMarking";
        heap_->ClearIdleTask();
        heap_->DisableNotifyIdle();
        heap_->GetIncrementalMarker()->Reset();
    }
    ProcessSharedGCRSetWorkList();
    Initialize();
    Mark();
    Sweep();
    Finish();
    if (UNLIKELY(heap_->ShouldVerifyHeap())) {
        // verify mark
        LOG_ECMA(DEBUG) << "start verify post fullgc";
        Verification(heap_, VerifyKind::VERIFY_SHARED_RSET_POST_FULL_GC).VerifyAll();
    }
}

void FullGC::RunPhasesForAppSpawn()
{
    auto marker = reinterpret_cast<CompressGCMarker*>(heap_->GetCompressGCMarker());
    marker->SetAppSpawn(true);
    RunPhases();
    marker->SetAppSpawn(false);
}

void FullGC::Initialize()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "FullGC::Initialize", "");
    TRACE_GC(GCStats::Scope::ScopeId::Initialize, heap_->GetEcmaVM()->GetEcmaGCStats());
    heap_->Prepare();
    auto callback = [](Region *current) {
        current->ResetAliveObject();
        current->ClearOldToNewRSet();
    };
    heap_->EnumerateNonMovableRegions(callback);
    heap_->GetAppSpawnSpace()->EnumerateRegions([](Region *current) {
        current->ClearMarkGCBitset();
        current->ClearCrossRegionRSet();
    });
    youngSpaceCommitSize_ = heap_->GetNewSpace()->GetCommittedSize();
    heap_->SwapNewSpace();
    workManager_->Initialize(TriggerGCType::FULL_GC, ParallelGCTaskPhase::COMPRESS_HANDLE_GLOBAL_POOL_TASK);
    heap_->GetCompressGCMarker()->Initialize();

    youngAndOldAliveSize_ = 0;
    nonMoveSpaceFreeSize_ = 0;
    oldSpaceCommitSize_ = heap_->GetOldSpace()->GetCommittedSize();
    nonMoveSpaceCommitSize_ = heap_->GetNonMovableSpace()->GetCommittedSize();
}

void FullGC::MarkRoots()
{
    CompressGCMarker *marker = static_cast<CompressGCMarker*>(heap_->GetCompressGCMarker());
    FullGCRunner fullGCRunner(heap_, workManager_->GetWorkNodeHolder(MAIN_THREAD_INDEX), forAppSpawn_);
    FullGCMarkRootVisitor &fullGCMarkRootVisitor = fullGCRunner.GetMarkRootVisitor();
    marker->MarkRoots(fullGCMarkRootVisitor, VMRootVisitType::UPDATE_ROOT);
}

void FullGC::Mark()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "FullGC::Mark", "");
    TRACE_GC(GCStats::Scope::ScopeId::Mark, heap_->GetEcmaVM()->GetEcmaGCStats());
    MarkRoots();
    heap_->GetCompressGCMarker()->ProcessMarkStack(MAIN_THREAD_INDEX);
    heap_->WaitRunningTaskFinished();
    // MarkJitCodeMap must be call after other mark work finish to make sure which jserror object js alive.
    heap_->GetCompressGCMarker()->MarkJitCodeMap(MAIN_THREAD_INDEX);
}

void FullGC::Sweep()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "FullGC::Sweep", "");
    TRACE_GC(GCStats::Scope::ScopeId::Sweep, heap_->GetEcmaVM()->GetEcmaGCStats());
    // process weak reference
    uint32_t totalThreadCount = 1; // 1 : mainthread
    if (heap_->IsParallelGCEnabled()) {
        totalThreadCount += common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum();
    }
    for (uint32_t i = 0; i < totalThreadCount; i++) {
        ProcessQueue *queue = workManager_->GetWorkNodeHolder(i)->GetWeakReferenceQueue();

        while (true) {
            auto obj = queue->PopBack();
            if (UNLIKELY(obj == nullptr)) {
                break;
            }
            ObjectSlot slot(ToUintPtr(obj));
            JSTaggedValue value(slot.GetTaggedType());
            auto header = value.GetTaggedWeakRef();

            Region *objectRegion = Region::ObjectAddressToRange(header);
            if (!HasEvacuated(objectRegion)) {
                if (!objectRegion->InSharedHeap() && !objectRegion->Test(header)) {
                    slot.Clear();
                }
            } else {
                MarkWord markWord(header);
                if (markWord.IsForwardingAddress()) {
                    TaggedObject *dst = markWord.ToForwardingAddress();
                    auto weakRef = JSTaggedValue(JSTaggedValue(dst).CreateAndGetWeakRef()).GetRawTaggedObject();
                    slot.Update(weakRef);
                } else {
                    slot.Update(static_cast<JSTaggedType>(JSTaggedValue::Undefined().GetRawData()));
                }
            }
        }
    }

    WeakRootVisitor gcUpdateWeak = [this](TaggedObject *header) -> TaggedObject* {
        Region *objectRegion = Region::ObjectAddressToRange(header);
        if (UNLIKELY(objectRegion == nullptr)) {
            LOG_GC(ERROR) << "FullGC updateWeakReference: region is nullptr, header is " << header;
            return nullptr;
        }
        if (!HasEvacuated(objectRegion)) {
            // The weak object in shared heap is always alive during fullGC.
            if (objectRegion->InSharedHeap() || objectRegion->Test(header)) {
                return header;
            }
            return nullptr;
        }

        MarkWord markWord(header);
        if (markWord.IsForwardingAddress()) {
            return markWord.ToForwardingAddress();
        }
        return nullptr;
    };
    heap_->GetEcmaVM()->GetJSThread()->IterateWeakEcmaGlobalStorage(gcUpdateWeak);
    heap_->GetEcmaVM()->ProcessReferences(gcUpdateWeak);

    heap_->GetSweeper()->Sweep(true);
    heap_->GetSweeper()->PostTask(true);
}

void FullGC::Finish()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "FullGC::Finish", "");
    TRACE_GC(GCStats::Scope::ScopeId::Finish, heap_->GetEcmaVM()->GetEcmaGCStats());
    if (!forAppSpawn_) {
        heap_->SwapOldSpace();
    }
    youngAndOldAliveSize_ = workManager_->Finish();
    if (forAppSpawn_) {
        heap_->ResumeForAppSpawn();
    } else {
        heap_->Resume(FULL_GC);
    }
    heap_->GetSweeper()->TryFillSweptRegion();
    heap_->SetFullMarkRequestedState(false);
}

bool FullGC::HasEvacuated(Region *region)
{
    if (forAppSpawn_) {
        return !region->InHugeObjectSpace()  && !region->InReadOnlySpace() && !region->InNonMovableSpace() &&
               !region->InSharedHeap();
    }
    return region->InYoungOrOldSpace();
}

void FullGC::SetForAppSpawn(bool flag)
{
    forAppSpawn_ = flag;
}

ARK_INLINE void FullGC::ProcessSharedGCRSetWorkList()
{
    TRACE_GC(GCStats::Scope::ScopeId::ProcessSharedGCRSetWorkList, heap_->GetEcmaVM()->GetEcmaGCStats());
    heap_->ProcessSharedGCRSetWorkList();
}
}  // namespace panda::ecmascript
