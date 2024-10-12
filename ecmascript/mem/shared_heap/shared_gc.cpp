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

#include "ecmascript/mem/shared_heap/shared_gc.h"

#include "ecmascript/mem/shared_heap/shared_concurrent_marker.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/mem/shared_heap/shared_gc_marker-inl.h"
#include "ecmascript/mem/verification.h"
#include "ecmascript/runtime.h"

namespace panda::ecmascript {
void SharedGC::RunPhases()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGC::RunPhases"
        + std::to_string(static_cast<int>(sHeap_->GetEcmaGCStats()->GetGCReason()))
        + ";Sensitive" + std::to_string(static_cast<int>(sHeap_->GetSensitiveStatus()))
        + ";IsInBackground" + std::to_string(sHeap_->IsInBackground())
        + ";Startup" + std::to_string(sHeap_->OnStartupEvent())
        + ";Old" + std::to_string(sHeap_->GetOldSpace()->GetCommittedSize())
        + ";huge" + std::to_string(sHeap_->GetHugeObjectSpace()->GetCommittedSize())
        + ";NonMov" + std::to_string(sHeap_->GetNonMovableSpace()->GetCommittedSize())
        + ";TotCommit" + std::to_string(sHeap_->GetCommittedSize()));
    TRACE_GC(GCStats::Scope::ScopeId::TotalGC, sHeap_->GetEcmaGCStats());
    markingInProgress_ = sHeap_->CheckOngoingConcurrentMarking();
    Initialize();
    Mark();
    if (UNLIKELY(sHeap_->ShouldVerifyHeap())) {
        // verify mark
        LOG_ECMA(DEBUG) << "start verify mark";
        SharedHeapVerification(sHeap_, VerifyKind::VERIFY_SHARED_GC_MARK).VerifyMark(markingInProgress_);
    }
    Sweep();
    if (UNLIKELY(sHeap_->ShouldVerifyHeap())) {
        // verify sweep
        LOG_ECMA(DEBUG) << "start verify sweep";
        SharedHeapVerification(sHeap_, VerifyKind::VERIFY_SHARED_GC_SWEEP).VerifySweep(markingInProgress_);
    }
    Finish();
    sHeap_->ResetNativeSizeAfterLastGC();
}

void SharedGC::Initialize()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGC::Initialize");
    TRACE_GC(GCStats::Scope::ScopeId::Initialize, sHeap_->GetEcmaGCStats());
    if (!markingInProgress_) {
        sHeap_->Prepare(true);
        sHeap_->GetAppSpawnSpace()->EnumerateRegions([](Region *current) {
            current->ClearMarkGCBitset();
        });
        sHeap_->EnumerateOldSpaceRegions([](Region *current) {
            ASSERT(current->InSharedSweepableSpace());
            current->ResetAliveObject();
        });
        sWorkManager_->Initialize(TriggerGCType::SHARED_GC, SharedParallelMarkPhase::SHARED_MARK_TASK);
    }
}
void SharedGC::Mark()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGC::Mark");
    TRACE_GC(GCStats::Scope::ScopeId::Mark, sHeap_->GetEcmaGCStats());
    if (markingInProgress_) {
        sHeap_->GetConcurrentMarker()->ReMark();
        return;
    }
    SharedGCMarker *marker = sHeap_->GetSharedGCMarker();
    marker->MarkRoots(DAEMON_THREAD_INDEX, SharedMarkType::NOT_CONCURRENT_MARK);
    marker->DoMark<SharedMarkType::NOT_CONCURRENT_MARK>(DAEMON_THREAD_INDEX);
    marker->MergeBackAndResetRSetWorkListHandler();
    sHeap_->WaitRunningTaskFinished();
}

void SharedGC::Sweep()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGC::Sweep");
    TRACE_GC(GCStats::Scope::ScopeId::Sweep, sHeap_->GetEcmaGCStats());
    UpdateRecordWeakReference();
    WeakRootVisitor gcUpdateWeak = [](TaggedObject *header) -> TaggedObject* {
        Region *objectRegion = Region::ObjectAddressToRange(header);
        if (UNLIKELY(objectRegion == nullptr)) {
            LOG_GC(ERROR) << "SharedGC updateWeakReference: region is nullptr, header is " << header;
            return nullptr;
        }
        if (!objectRegion->InSharedSweepableSpace() || objectRegion->Test(header)) {
            return header;
        }
        return nullptr;
    };
    auto stringTableCleaner = Runtime::GetInstance()->GetEcmaStringTable()->GetCleaner();
    stringTableCleaner->PostSweepWeakRefTask(gcUpdateWeak);
    Runtime::GetInstance()->ProcessNativeDeleteInSharedGC(gcUpdateWeak);
    Runtime::GetInstance()->ProcessSharedNativeDelete(gcUpdateWeak);

    Runtime::GetInstance()->GCIterateThreadList([&](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        thread->IterateWeakEcmaGlobalStorage(gcUpdateWeak, GCKind::SHARED_GC);
        const_cast<Heap*>(thread->GetEcmaVM()->GetHeap())->ResetTlab();
    });

    stringTableCleaner->JoinAndWaitSweepWeakRefTask(gcUpdateWeak);
    sHeap_->GetSweeper()->Sweep(false);
    sHeap_->GetSweeper()->PostTask(false);
}

void SharedGC::Finish()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGC::Finish");
    TRACE_GC(GCStats::Scope::ScopeId::Finish, sHeap_->GetEcmaGCStats());
    sHeap_->Reclaim(TriggerGCType::SHARED_GC);
    if (markingInProgress_) {
        sHeap_->GetConcurrentMarker()->Reset(false);
    } else {
        sWorkManager_->Finish();
    }
    sHeap_->GetSweeper()->TryFillSweptRegion();
}

void SharedGC::UpdateRecordWeakReference()
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
            if (value.IsWeak()) {
                auto header = value.GetTaggedWeakRef();
                Region *objectRegion = Region::ObjectAddressToRange(header);
                if (!objectRegion->Test(header)) {
                    slot.Clear();
                }
            }
        }
    }
}

void SharedGC::ResetWorkManager(SharedGCWorkManager *sWorkManager)
{
    sWorkManager_ = sWorkManager;
}
}  // namespace panda::ecmascript
