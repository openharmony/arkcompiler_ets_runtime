/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#include "ecmascript/mem/shared_heap/global_gc.h"

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/shared_heap/global_gc_marker.h"
#include "ecmascript/mem/gc_stats.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/shared_heap/shared_full_gc.h"
#include "ecmascript/mem/work_manager-inl.h"
#include "ecmascript/mem/region-inl.h"

namespace panda::ecmascript {

GlobalGC::GlobalGC(SharedHeap *sHeap)
    : sHeap_(sHeap), workManager_(sHeap->GetGlobalGCWorkManager()), cleaner_(sHeap) {}

GlobalGC::~GlobalGC() = default;

void GlobalGC::RunPhases()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "GlobalGC::RunPhases", "");
    TRACE_GC(GCStats::Scope::ScopeId::GlobalGC, sHeap_->GetEcmaGCStats());

    Initialize();
    Mark();
    Clean();
    Finish();
    TriggerSharedGC();

    LOG_GC(INFO) << "GlobalGC: finished in " << sp.TotalSpentTime() << "ms";
}

void GlobalGC::Initialize()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "GlobalGC::Initialize", "");
    workManager_->Initialize();
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *iteratedThread) {
        Heap *heap = iteratedThread->GetEcmaVM()->GetHeap();
        heap->Prepare();
        heap->GetConcurrentMarker()->Reset(true);
        heap->GetAppSpawnSpace()->EnumerateRegions([](Region *current) {
            current->ClearMarkGCBitset();
        });
    });
}

void GlobalGC::Mark()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "GlobalGC::Mark", "");
    TRACE_GC(GCStats::Scope::ScopeId::GlobalMark, sHeap_->GetEcmaGCStats());
    auto *marker = sHeap_->GetGlobalGCMarker();
    GlobalGCWorkNodeHolder *holder = workManager_->GetHolder(DAEMON_THREAD_INDEX);
    Runtime::GetInstance()->GCIterateThreadList([&](JSThread *iteratedThread) {
        Heap *heap = iteratedThread->GetEcmaVM()->GetHeap();
        heap->SetGCType(TriggerGCType::GLOBAL_GC);
        marker->MarkRoots(heap, holder);
        holder->PushWorkNodeToGlobal();
    });
    marker->ProcessMarkStack(workManager_, DAEMON_THREAD_INDEX);
    sHeap_->WaitGlobalGCMarkTaskFinished();
    marker->MarkJitCodeMaps(workManager_, DAEMON_THREAD_INDEX);
    sHeap_->WaitGlobalGCMarkTaskFinished();
    LOG_GC(INFO) << "GlobalGC: mark " << sp.TotalSpentTime() << "ms";
}

void GlobalGC::Clean()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "GlobalGC::Clean", "");
    TRACE_GC(GCStats::Scope::ScopeId::GlobalClean, sHeap_->GetEcmaGCStats());
    cleaner_.Clean();
    RebuildFreeList();
    LOG_GC(INFO) << "GlobalGC: clean " << sp.TotalSpentTime() << "ms";
}

void GlobalGC::Finish()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "GlobalGC::Finish", "");
    workManager_->Finish();
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *iteratedThread) {
        Heap *heap = iteratedThread->GetEcmaVM()->GetHeap();
        heap->EnumerateRegions([](Region *region) {
            if (!region->InAppSpawnSpace()) {
                region->ClearMarkGCBitset();
            }
        });
    });
}

void GlobalGC::RebuildFreeList()
{
    Runtime::GetInstance()->GCIterateThreadList([](JSThread *iteratedThread) {
        Heap *heap = iteratedThread->GetEcmaVM()->GetHeap();
        heap->GetOldSpace()->RebuildFreeList();
        heap->GetNonMovableSpace()->RebuildFreeList();
    });
}

void GlobalGC::TriggerSharedGC()
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "GlobalGC::TriggerSharedGC", "");
    TRACE_GC(GCStats::Scope::ScopeId::GlobalSharedGC, sHeap_->GetEcmaGCStats());
    size_t sizeBefore = sHeap_->GetHeapObjectSize();
    sHeap_->GetSharedFullGC()->RunPhases();
    size_t sizeAfter = sHeap_->GetHeapObjectSize();
    freedSize_ = sizeBefore > sizeAfter ? sizeBefore - sizeAfter : 0;
    LOG_GC(INFO) << "GlobalGC: shared gc " << sp.TotalSpentTime() << "ms, freed "
                 << (freedSize_ / 1_KB) << "KB";
}

}  // namespace panda::ecmascript
