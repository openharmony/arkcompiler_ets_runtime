/*
 * Copyright (c) 2024 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/shared_heap/shared_full_gc.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_marker.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/mem/shared_heap/shared_gc_marker-inl.h"

namespace panda::ecmascript {
void SharedFullGC::RunPhases()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedFullGC::RunPhases;Reason"
        + std::to_string(static_cast<int>(sHeap_->GetEcmaGCStats()->GetGCReason()))
        + ";Sensitive" + std::to_string(static_cast<int>(sHeap_->GetSensitiveStatus()))
        + ";IsInBackground" + std::to_string(sHeap_->IsInBackground())
        + ";Startup" + std::to_string(static_cast<int>(sHeap_->GetStartupStatus()))
        + ";Old" + std::to_string(sHeap_->GetOldSpace()->GetCommittedSize())
        + ";huge" + std::to_string(sHeap_->GetHugeObjectSpace()->GetCommittedSize())
        + ";NonMov" + std::to_string(sHeap_->GetNonMovableSpace()->GetCommittedSize())
        + ";TotCommit" + std::to_string(sHeap_->GetCommittedSize())
        + ";NativeBindingSize" + std::to_string(sHeap_->GetNativeSizeAfterLastGC())
        + ";NativeLimitGC" + std::to_string(sHeap_->GetNativeSizeTriggerSharedGC())
        + ";NativeLimitCM" + std::to_string(sHeap_->GetNativeSizeTriggerSharedCM()));
    TRACE_GC(GCStats::Scope::ScopeId::TotalGC, sHeap_->GetEcmaGCStats());
    Initialize();
    Mark();
    Sweep();
    Finish();
}

void SharedFullGC::Initialize()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedFullGC::Initialize");
    TRACE_GC(GCStats::Scope::ScopeId::Initialize, sHeap_->GetEcmaGCStats());
    sHeap_->Prepare(true);
    if (UNLIKELY(sHeap_->CheckOngoingConcurrentMarking())) {
        // Concurrent shared mark should always trigger shared gc without moving.
        sHeap_->GetConcurrentMarker()->Reset(true);
    }
    sHeap_->GetAppSpawnSpace()->EnumerateRegions([](Region *current) {
        current->ClearMarkGCBitset();
    });
    sHeap_->EnumerateOldSpaceRegions([](Region *current) {
        ASSERT(current->InSharedSweepableSpace());
        current->ResetAliveObject();
    });
    sWorkManager_->Initialize(TriggerGCType::SHARED_FULL_GC, SharedParallelMarkPhase::SHARED_COMPRESS_TASK);
}

void SharedFullGC::Mark()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedFullGC::Mark");
    TRACE_GC(GCStats::Scope::ScopeId::Mark, sHeap_->GetEcmaGCStats());
    SharedGCMovableMarker *marker = sHeap_->GetSharedGCMovableMarker();

    marker->MarkRoots(DAEMON_THREAD_INDEX, SharedMarkType::NOT_CONCURRENT_MARK, VMRootVisitType::UPDATE_ROOT);
    marker->DoMark<SharedMarkType::NOT_CONCURRENT_MARK>(DAEMON_THREAD_INDEX);
    marker->MergeBackAndResetRSetWorkListHandler();
    sHeap_->WaitRunningTaskFinished();
}

void SharedFullGC::Sweep()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedFullGC::Sweep");
    TRACE_GC(GCStats::Scope::ScopeId::Sweep, sHeap_->GetEcmaGCStats());
    UpdateRecordWeakReference();
    WeakRootVisitor gcUpdateWeak = [](TaggedObject *header) {
        Region *objectRegion = Region::ObjectAddressToRange(header);
        if (!objectRegion) {
            LOG_GC(ERROR) << "SharedFullGC updateWeakReference: region is nullptr, header is " << header;
            return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
        }
        if (objectRegion->InSharedOldSpace()) {
            MarkWord markWord(header);
            if (markWord.IsForwardingAddress()) {
                return markWord.ToForwardingAddress();
            }
            return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
        }
        if (!objectRegion->InSharedSweepableSpace() || objectRegion->Test(header)) {
            return header;
        }
        return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
    };
    auto stringTableCleaner = Runtime::GetInstance()->GetEcmaStringTable()->GetCleaner();
    stringTableCleaner->PostSweepWeakRefTask(gcUpdateWeak);
    Runtime::GetInstance()->ProcessNativeDeleteInSharedGC(gcUpdateWeak);
    Runtime::GetInstance()->ProcessSharedNativeDelete(gcUpdateWeak);

    Runtime::GetInstance()->GCIterateThreadList([&](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        thread->IterateWeakEcmaGlobalStorage(gcUpdateWeak, GCKind::SHARED_GC);
        const_cast<Heap*>(thread->GetEcmaVM()->GetHeap())->ResetTlab();
        thread->ClearContextCachedConstantPool();
    });

    stringTableCleaner->JoinAndWaitSweepWeakRefTask(gcUpdateWeak);
    sHeap_->GetSweeper()->Sweep(true);
    sHeap_->GetSweeper()->PostTask(true);
}

void SharedFullGC::Finish()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedFullGC::Finish");
    TRACE_GC(GCStats::Scope::ScopeId::Finish, sHeap_->GetEcmaGCStats());
    sHeap_->SwapOldSpace();
    sWorkManager_->Finish();
    if (!isAppspawn_) {
        sHeap_->Reclaim(TriggerGCType::SHARED_FULL_GC);
    } else {
        sHeap_->ReclaimForAppSpawn();
    }
    
    sHeap_->GetSweeper()->TryFillSweptRegion();
}

void SharedFullGC::UpdateRecordWeakReference()
{
    auto totalThreadCount = Taskpool::GetCurrentTaskpool()->GetTotalThreadNum() + 1;
    for (uint32_t i = 0; i < totalThreadCount; i++) {
        ProcessQueue *queue = sHeap_->GetWorkManager()->GetWeakReferenceQueue(i);

        while (true) {
            auto obj = queue->PopBack();
            if (UNLIKELY(obj == nullptr)) {
                break;
            }
            ObjectSlot slot(ToUintPtr(obj));
            JSTaggedValue value(slot.GetTaggedType());
            ASSERT(value.IsWeak());
            auto header = value.GetTaggedWeakRef();
            Region *objectRegion = Region::ObjectAddressToRange(header);
            if (!objectRegion->InSharedOldSpace()) {
                if (!objectRegion->Test(header)) {
                    slot.Clear();
                }
            } else {
                MarkWord markWord(header);
                if (markWord.IsForwardingAddress()) {
                    TaggedObject *dst = markWord.ToForwardingAddress();
                    auto weakRef = JSTaggedValue(JSTaggedValue(dst).CreateAndGetWeakRef()).GetRawTaggedObject();
                    slot.Update(weakRef);
                } else {
                    slot.Clear();
                }
            }
        }
    }
}

bool SharedFullGC::HasEvacuated(Region *region)
{
    auto marker = reinterpret_cast<SharedGCMovableMarker *>(sHeap_->GetSharedGCMovableMarker());
    return marker->NeedEvacuate(region);
}

void SharedFullGC::ResetWorkManager(SharedGCWorkManager *sWorkManager)
{
    sWorkManager_ = sWorkManager;
}
}  // namespace panda::ecmascript
