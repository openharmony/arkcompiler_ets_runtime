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

#include "ecmascript/ecma_string_table.h"
#include "ecmascript/ecma_vm.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/mark_stack.h"
#include "ecmascript/mem/mem.h"
#include "ecmascript/mem/parallel_marker-inl.h"
#include "ecmascript/mem/shared_heap/shared_concurrent_sweeper.h"
#include "ecmascript/mem/shared_heap/shared_gc_marker-inl.h"
#include "ecmascript/mem/space-inl.h"
#include "ecmascript/mem/visitor.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/runtime.h"

namespace panda::ecmascript {
void SharedGC::RunPhases()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGC::RunPhases");
    TRACE_GC(GCStats::Scope::ScopeId::TotalGC, sHeap_->GetEcmaGCStats());
    Initialize();
    Mark();
    Sweep();
    Finish();
}

void SharedGC::Initialize()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGC::Initialize");
    TRACE_GC(GCStats::Scope::ScopeId::Initialize, sHeap_->GetEcmaGCStats());
    sHeap_->EnumerateOldSpaceRegions([](Region *current) {
        ASSERT(current->InSharedSweepableSpace());
        current->ResetAliveObject();
    });
    sWorkManager_->Initialize();
}

void SharedGC::Mark()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGC::Mark");
    TRACE_GC(GCStats::Scope::ScopeId::Mark, sHeap_->GetEcmaGCStats());
    sHeap_->GetSharedGCMarker()->MarkSerializeRoots(MAIN_THREAD_INDEX);
    Runtime::GetInstance()->GCIterateThreadList([&](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        auto vm = thread->GetEcmaVM();
        auto heap = const_cast<Heap*>(vm->GetHeap());
        heap->GetSweeper()->EnsureAllTaskFinished();
        heap->WaitClearTaskFinished();
        sHeap_->GetSharedGCMarker()->MarkRoots(MAIN_THREAD_INDEX, vm);
        sHeap_->GetSharedGCMarker()->ProcessLocalToShare(MAIN_THREAD_INDEX, heap);
    });
    sHeap_->WaitRunningTaskFinished();
}

void SharedGC::Sweep()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGC::Sweep");
    TRACE_GC(GCStats::Scope::ScopeId::Sweep, sHeap_->GetEcmaGCStats());
    UpdateRecordWeakReference();
    WeakRootVisitor gcUpdateWeak = [](TaggedObject *header) {
        Region *objectRegion = Region::ObjectAddressToRange(header);
        if (!objectRegion) {
            LOG_GC(ERROR) << "SharedGC updateWeakReference: region is nullptr, header is " << header;
            return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
        }
        if (!objectRegion->InSharedSweepableSpace() || objectRegion->Test(header)) {
            return header;
        }
        return reinterpret_cast<TaggedObject *>(ToUintPtr(nullptr));
    };
    Runtime::GetInstance()->GetEcmaStringTable()->SweepWeakReference(gcUpdateWeak);

    Runtime::GetInstance()->GCIterateThreadList([&](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        thread->GetCurrentEcmaContext()->ProcessNativeDeleteInSharedGC(gcUpdateWeak);
        thread->IterateWeakEcmaGlobalStorage(gcUpdateWeak, true);
        thread->GetEcmaVM()->ProcessSharedNativeDelete(gcUpdateWeak);
    });

    sHeap_->GetSweeper()->Sweep();
    sHeap_->GetSweeper()->PostTask();
}

void SharedGC::Finish()
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGC::Finish");
    TRACE_GC(GCStats::Scope::ScopeId::Finish, sHeap_->GetEcmaGCStats());
    sHeap_->Reclaim();
    sWorkManager_->Finish();
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

void SharedGC::ResetWorkManager(SharedGCWorkManager *workManager)
{
    sWorkManager_ = workManager;
}
}  // namespace panda::ecmascript
