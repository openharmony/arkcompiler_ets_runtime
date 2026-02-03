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

#include "ecmascript/mem/shared_heap/shared_gc_marker-inl.h"

#include "ecmascript/mem/local_cmc/concurrent_copy_gc.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/shared_heap/shared_full_gc.h"
#include "ecmascript/mem/shared_heap/shared_gc_visitor-inl.h"
#include "ecmascript/mem/shared_heap/shared_full_gc-inl.h"

namespace panda::ecmascript {
void SharedGCMarkerBase::MarkRoots(RootVisitor &visitor, SharedMarkType markType)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkRoots", "");
    MarkGlobalRoots(visitor);
    MarkAllLocalRoots(visitor, markType);
}

void SharedGCMarkerBase::MarkGlobalRoots(RootVisitor &visitor)
{
    MarkSendableGlobalStorage(visitor);
    MarkSerializeRoots(visitor);
    MarkSharedModule(visitor);
    MarkStringCache(visitor);
}

void SharedGCMarkerBase::MarkAllLocalRoots(RootVisitor &visitor, SharedMarkType markType)
{
    Runtime *runtime = Runtime::GetInstance();
    if (markType != SharedMarkType::CONCURRENT_MARK_REMARK) {
        PrepareCollectLocalVMRSet();
    }
    runtime->GCIterateThreadList([&](JSThread *thread) {
        ASSERT(thread->IsSuspended() || thread->HasLaunchedSuspendAll());
        auto vm = thread->GetEcmaVM();
        ASSERT(!thread->IsConcurrentCopying());
        MarkLocalVMRoots(visitor, vm, markType);
        if (markType != SharedMarkType::CONCURRENT_MARK_REMARK) {
            CollectLocalVMRSet(vm, SourceOfMarkingLocalVMRoot::STW_FROM_DAEMON);
        }
    });
}

void SharedGCMarkerBase::MarkLocalVMRoots(RootVisitor &visitor, EcmaVM *localVm, SharedMarkType markType)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkLocalVMRoots", "");
    Heap *heap = const_cast<Heap*>(localVm->GetHeap());
    if (markType != SharedMarkType::CONCURRENT_MARK_REMARK) {
        heap->GetSweeper()->EnsureAllTaskFinished();
    }
    ObjectXRay::VisitVMRoots(localVm, visitor);
    heap->ProcessSharedGCMarkingLocalBuffer();
}

void SharedGCMarkerBase::PrepareCollectLocalVMRSet()
{
    rSetHandlers_.reserve(Runtime::GetInstance()->GetThreadListSize());
    ASSERT(rSetHandlers_.empty());
}

void SharedGCMarkerBase::CollectLocalVMRSet(EcmaVM *localVm, SourceOfMarkingLocalVMRoot markSource)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedGCMarkerBase::CollectLocalVMRSet", "");
    Heap *heap = const_cast<Heap*>(localVm->GetHeap());
    RSetWorkListHandler *handler = new RSetWorkListHandler(heap, localVm->GetJSThreadNoCheck());
    heap->SetRSetWorkListHandler(handler);
    NotifyThreadProcessRsetStart(handler->GetOwnerThreadUnsafe());
    if (markSource == SourceOfMarkingLocalVMRoot::FLIP_FROM_JS_THREAD) {
        LockHolder holder(mutex_);
        rSetHandlers_.emplace_back(handler);
    } else {
        ASSERT(markSource == SourceOfMarkingLocalVMRoot::STW_FROM_DAEMON);
        rSetHandlers_.emplace_back(handler);
    }
}

void SharedGCMarkerBase::MarkSerializeRoots(RootVisitor &visitor)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkSerializeRoots", "");
    Runtime::GetInstance()->IterateSerializeRoot(visitor);
}

void SharedGCMarkerBase::MarkStringCache(RootVisitor &visitor)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkStringCache", "");
    Runtime::GetInstance()->IterateCachedStringRoot(visitor);
}

void SharedGCMarkerBase::MarkSendableGlobalStorage(RootVisitor &visitor)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkSendableGlobalStorage", "");
    Runtime::GetInstance()->IterateSendableGlobalStorage(visitor);
}

void SharedGCMarkerBase::MarkSharedModule(RootVisitor &visitor)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkSharedModule", "");
    SharedModuleManager::GetInstance()->Iterate(visitor);
}

void SharedGCMarkerBase::ProcessThenMergeBackRSetFromBoundJSThread(RSetWorkListHandler *handler)
{
    ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedGCMarker::ProcessRSet", "");
    ASSERT(JSThread::GetCurrent() == handler->GetHeap()->GetEcmaVM()->GetJSThread());
    ASSERT(JSThread::GetCurrent()->IsInRunningState());
    MarkWorkNode *&localBuffer = handler->GetHeap()->GetMarkingObjectLocalBuffer();
    auto visitor = [this, &localBuffer](void *mem) -> bool {
        ObjectSlot slot(ToUintPtr(mem));
        JSTaggedValue value(slot.GetTaggedType());

        if (value.IsInSharedSweepableSpace()) {
            // For now if record weak references from local to share in marking root, the slots
            // may be invalid due to LocalGC, so just mark them as strong-reference.
            MarkObjectFromJSThread(localBuffer, value.GetHeapObject());
            return true;
        }

        // clear bit.
        return false;
    };
    handler->ProcessAll(visitor);
    handler->WaitFinishedThenMergeBack();
}

void SharedGCMarker::ProcessMarkStack(uint32_t threadId)
{
    SharedGCWorkNodeHolder *sWorkNodeHolder = sWorkManager_->GetSharedGCWorkNodeHolder(threadId);
    SharedGCMarkObjectVisitor sharedGCMarkObjectVisitor(sWorkNodeHolder);
    TaggedObject *obj = nullptr;
    while (sWorkNodeHolder->Pop(&obj)) {
        JSHClass *hclass = obj->SynchronizedGetClass();
        auto size = hclass->SizeFromJSHClass(obj);
        Region *region = Region::ObjectAddressToRange(obj);
        ASSERT(region->InSharedSweepableSpace());
        region->IncreaseAliveObject(size);

        sharedGCMarkObjectVisitor.VisitHClass(hclass);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, hclass, sharedGCMarkObjectVisitor);
    }
}

void SharedGCMovableMarker::ProcessMarkStack(uint32_t threadId)
{
    SharedGCWorkNodeHolder *sWorkNodeHolder = sWorkManager_->GetSharedGCWorkNodeHolder(threadId);
    SharedFullGCRunner sRunner(SharedHeap::GetInstance(), sWorkNodeHolder);
    SharedFullGCMarkObjectVisitor &objectVisitor = sRunner.GetMarkObjectVisitor();
    TaggedObject *obj = nullptr;
    while (sWorkNodeHolder->Pop(&obj)) {
        JSHClass *hclass = obj->GetClass();
        ObjectSlot hClassSlot(ToUintPtr(obj));

        objectVisitor.VisitHClassSlot(hClassSlot, hclass);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, hclass, objectVisitor);
    }
}

void SharedGCMarkerBase::MergeBackAndResetRSetWorkListHandler()
{
    for (RSetWorkListHandler *handler : rSetHandlers_) {
        handler->MergeBack();
        delete handler;
    }
    rSetHandlers_.clear();
}

void SharedGCMarkerBase::ResetWorkManager(SharedGCWorkManager *workManager)
{
    sWorkManager_ = workManager;
}

SharedGCMarker::SharedGCMarker(SharedGCWorkManager *sWorkManger)
    : SharedGCMarkerBase(sWorkManger) {}

SharedGCMovableMarker::SharedGCMovableMarker(SharedGCWorkManager *sWorkManger)
    : SharedGCMarkerBase(sWorkManger) {}

}  // namespace panda::ecmascript
