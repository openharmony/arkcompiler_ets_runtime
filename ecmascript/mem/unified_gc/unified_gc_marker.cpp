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

#include "ecmascript/checkpoint/thread_state_transition.h"
#include "ecmascript/mem/concurrent_marker.h"
#include "ecmascript/mem/unified_gc/unified_gc_marker-inl.h"
#include "ecmascript/mem/verification.h"

namespace panda::ecmascript {

void UnifiedGCMarker::Initialize()
{
    if (initialized_.load(std::memory_order_acquire)) {
        return;
    }
    LockHolder holder(initializeMutex_);
    if (!initialized_.load(std::memory_order_relaxed)) {
        heap_->UnifiedGCPrepare();
        heap_->GetAppSpawnSpace()->EnumerateRegions([](Region *current) {
            current->ClearMarkGCBitset();
        });
        workManager_->Initialize(TriggerGCType::UNIFIED_GC, ParallelGCTaskPhase::UNIFIED_HANDLE_GLOBAL_POOL_TASK);
        initialized_.store(true, std::memory_order_release);
    }
}

void UnifiedGCMarker::InitialMark(uint32_t threadId)
{
    UnifiedGCMarkRootsScope scope(heap_->GetJSThread());
    UnifiedGCMarkRootVisitor visitor(workManager_->GetWorkNodeHolder(threadId), this);
    MarkRoots(visitor);
}

void UnifiedGCMarker::Finish()
{
    initialized_.store(false, std::memory_order_relaxed);
    workManager_->Finish();
    heap_->Resume(TriggerGCType::UNIFIED_GC);
}

void UnifiedGCMarker::MarkFromObject(TaggedObject *object)
{
    Initialize();
    Region *objectRegion = Region::ObjectAddressToRange(object);

    if (objectRegion->InSharedHeap()) {
        return;
    }

    if (objectRegion->AtomicMark(object)) {
        workManager_->PushObjectToGlobal(object);
    }
}

void UnifiedGCMarker::ProcessMarkStack(uint32_t threadId)
{
    WorkNodeHolder *workNodeHolder = workManager_->GetWorkNodeHolder(threadId);
    UnifiedGCMarkObjectVisitor visitor(workNodeHolder, this);

    TaggedObject *obj = nullptr;
    while (workNodeHolder->Pop(&obj)) {
        JSHClass *jsHclass = obj->SynchronizedGetClass();
        visitor.VisitHClass(jsHclass);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, jsHclass, visitor);
    }
}
}  // namespace panda::ecmascript