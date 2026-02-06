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

#include "ecmascript/mem/cms_mem/sweep_gc.h"

#include "common_components/taskpool/taskpool.h"
#include "ecmascript/js_weak_container.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/cms_mem/sweep_gc_visitor-inl.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/parallel_evacuator.h"
#include "ecmascript/mem/parallel_marker.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/runtime_call_id.h"

namespace panda::ecmascript {

SweepGC::SweepGC(Heap *heap) : heap_(heap), workManager_(heap->GetWorkManager())
{
}

void SweepGC::RunPhases()
{
    ASSERT("SweepGC should be disabled" && !g_isEnableCMCGC);
    ASSERT("SweepGC should be disabled" && G_USE_CMS_GC);
    GCStats *gcStats = heap_->GetEcmaVM()->GetEcmaGCStats();
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK,
        ("SweepGC::RunPhases" + std::to_string(heap_->IsConcurrentFullMark())
        + ";GCReason" + std::to_string(static_cast<int>(gcStats->GetGCReason()))
        + ";MarkReason" + std::to_string(static_cast<int>(gcStats->GetMarkReason()))
        + ";Sensitive" + std::to_string(static_cast<int>(heap_->GetSensitiveStatus()))
        + ";IsInBackground" + std::to_string(Runtime::GetInstance()->IsInBackground())
        + ";Startup" + std::to_string(static_cast<int>(heap_->GetStartupStatus()))
        + ";ConMark" + std::to_string(static_cast<int>(heap_->GetJSThread()->GetMarkStatus()))
        + ";Slot" + std::to_string(heap_->GetSlotSpace()->GetCommittedSize())
        + ";Huge" + std::to_string(heap_->GetHugeObjectSpace()->GetCommittedSize())
        + ";TotalCommit" + std::to_string(heap_->GetCommittedSize())
        + ";NativeBindingSize" + std::to_string(heap_->GetNativeBindingSize())
        + ";NativeLimitSize" + std::to_string(heap_->GetGlobalSpaceNativeLimit())
        + ";ObjSizeBeforeSensitive" + std::to_string(heap_->GetRecordHeapObjectSizeBeforeSensitive())).c_str(), "");
    TRACE_GC(GCStats::Scope::ScopeId::TotalGC, gcStats);
    MEM_ALLOCATE_AND_GC_TRACE(heap_->GetEcmaVM(), SweepGC_RunPhases);
    markingInProgress_ = heap_->CheckOngoingConcurrentMarking();
    LOG_GC(DEBUG) << "markingInProgress_" << markingInProgress_;
    Initialize();

    Mark();
    if (UNLIKELY(heap_->ShouldVerifyHeap())) {
        Verification::VerifyMark(heap_);
    }

    ProcessSharedGCRSetWorkList();

    Sweep();
    ClearDeadReferences();

    if (UNLIKELY(heap_->ShouldVerifyHeap())) {
        Verification::VerifyEvacuate(heap_);
    }
    Finish();
}

void SweepGC::Initialize()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SweepGC::Initialize", "");
    TRACE_GC(GCStats::Scope::ScopeId::Initialize, heap_->GetEcmaVM()->GetEcmaGCStats());
    if (!markingInProgress_) {
        LOG_GC(DEBUG) << "No ongoing Concurrent marking. Initializing...";
        heap_->Prepare();
        heap_->GetAppSpawnSpace()->EnumerateRegions([](Region *current) {
            current->ClearMarkGCBitset();
        });
        heap_->EnumerateRegions([](Region *current) {
            current->ResetAliveObject();
        });
        workManager_->Initialize(TriggerGCType::CMS_GC, ParallelGCTaskPhase::HANDLE_GLOBAL_POOL_TASK);
    }
}

void SweepGC::Finish()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SweepGC::Finish", "");
    TRACE_GC(GCStats::Scope::ScopeId::Finish, heap_->GetEcmaVM()->GetEcmaGCStats());
    heap_->Resume(TriggerGCType::CMS_GC);
    if (markingInProgress_) {
        auto marker = heap_->GetConcurrentMarker();
        marker->Reset(false);
    } else {
        workManager_->Finish();
    }
    // fixme: check if need?
    if (heap_->IsNearGCInSensitive()) {
        heap_->SetNearGCInSensitive(false);
    }
}

void SweepGC::MarkRoots()
{
    // fixme: support sticky gc
    SweepGCMarkRootVisitor sweepGCMarkRootVisitor(workManager_->GetWorkNodeHolder(MAIN_THREAD_INDEX));
    heap_->GetNonMovableMarker()->MarkRoots(sweepGCMarkRootVisitor);
}

void SweepGC::Mark()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SweepGC::Mark", "");
    TRACE_GC(GCStats::Scope::ScopeId::Mark, heap_->GetEcmaVM()->GetEcmaGCStats());
    if (markingInProgress_) {
        heap_->GetConcurrentMarker()->ReMark();
        return;
    }
    MarkRoots();
    workManager_->GetWorkNodeHolder(MAIN_THREAD_INDEX)->FlushAll();
    heap_->GetNonMovableMarker()->ProcessMarkStack(MAIN_THREAD_INDEX);
    heap_->WaitRunningMarkTaskFinished();
    // MarkJitCodeMap must be call after other mark work finish to make sure which jserror object js alive.
    heap_->GetNonMovableMarker()->MarkJitCodeMap(MAIN_THREAD_INDEX);
}

void SweepGC::Sweep()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SweepGC::Sweep", "");
    ProcessNativeDelete();
    TRACE_GC(GCStats::Scope::ScopeId::Sweep, heap_->GetEcmaVM()->GetEcmaGCStats());
    heap_->GetSweeper()->Sweep(TriggerGCType::CMS_GC);
    heap_->GetSweeper()->PostTask(TriggerGCType::CMS_GC);
}

void SweepGC::ProcessNativeDelete()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SweepGC::ProcessNativeDelete", "");
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

void SweepGC::ClearDeadReferences()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SweepGC::ClearDeadReferences", "");
    TRACE_GC(GCStats::Scope::ScopeId::ClearDeadReferences, heap_->GetEcmaVM()->GetEcmaGCStats());
    uint32_t totalThreadCount = common::Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() + 1;
    // fixme: in parallel?
    for (uint32_t i = 0; i < totalThreadCount; ++i) {
        UpdateRecordWeakReference(i);
        UpdateRecordWeakLinkedHashMap(i);
    }

    WeakRootVisitor gcClearDeadWeak = [](TaggedObject *header) -> TaggedObject* {
        Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(header));
        ASSERT(objectRegion != nullptr);
        if (objectRegion->InSharedHeap()) {
            return header;
        }
        if (objectRegion->Test(header)) {
            return header;
        }
        return nullptr;
    };
    heap_->GetEcmaVM()->GetJSThread()->IterateWeakEcmaGlobalStorage(gcClearDeadWeak);
    heap_->GetEcmaVM()->ProcessReferences(gcClearDeadWeak);
    heap_->GetEcmaVM()->ProcessSnapShotEnv(gcClearDeadWeak);
    heap_->GetEcmaVM()->GetJSThread()->UpdateJitCodeMapReference(gcClearDeadWeak);
}

void SweepGC::UpdateRecordWeakReference(uint32_t threadId)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SweepGC::UpdateRecordWeakReference", "");
    ProcessQueue *queue = heap_->GetWorkManager()->GetWorkNodeHolder(threadId)->GetWeakReferenceQueue();
    while (true) {
        JSTaggedType *obj = queue->PopBack();
        if (UNLIKELY(obj == nullptr)) {
            break;
        }
        ObjectSlot slot(ToUintPtr(obj));
        JSTaggedType value = slot.GetTaggedType();
        if (JSTaggedValue(value).IsWeak()) {
            Region *objectRegion = Region::ObjectAddressToRange(value);
            if (!objectRegion->InSharedHeap() && !objectRegion->Test(value)) {
                slot.Clear();
            }
        }
    }
}

void SweepGC::UpdateRecordWeakLinkedHashMap(uint32_t threadId)
{
    auto visitor = [](JSTaggedValue key) {
        ASSERT(!key.IsHole());
        if (key.IsUndefined()) {    // Dead key, and set to undefined by GC
            return true;
        }
        ASSERT(key.IsHeapObject() && key.IsWeak());
        uintptr_t addr = ToUintPtr(key.GetTaggedWeakRef());
        Region *keyRegion = Region::ObjectAddressToRange(addr);

        if (keyRegion->InSharedHeap()) {
            return false;
        }
        return !keyRegion->Test(addr);
    };

    JSThread *thread = heap_->GetJSThread();
    WorkManager *workManager = heap_->GetWorkManager();
    WeakLinkedHashMapProcessQueue *queue = workManager->GetWorkNodeHolder(threadId)->GetWeakLinkedHashMapQueue();
    while (true) {
        TaggedObject *obj = queue->PopBack();
        if (UNLIKELY(obj == nullptr)) {
            break;
        }
        WeakLinkedHashMap *map = WeakLinkedHashMap::Cast(obj);
        ASSERT(map->VerifyLayout());

        int entries = map->NumberOfAllUsedElements();
        for (int i = 0; i < entries; ++i) {
            JSTaggedValue maybeKey = map->GetKey(thread, i);
            if (maybeKey.IsHole()) {
                // is a deleted element
                continue;
            }
            bool dead = visitor(maybeKey);
            if (dead) {
                map->RemoveEntryFromGCThread(i);
            }
        }
    }
}

void SweepGC::ProcessSharedGCRSetWorkList()
{
    TRACE_GC(GCStats::Scope::ScopeId::ProcessSharedGCRSetWorkList, heap_->GetEcmaVM()->GetEcmaGCStats());
    heap_->ProcessSharedGCRSetWorkList();
}
}  // namespace panda::ecmascript
