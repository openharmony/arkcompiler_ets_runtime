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

#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/shared_heap/shared_full_gc.h"
#include "ecmascript/mem/shared_heap/shared_gc_visitor-inl.h"
#include "ecmascript/mem/shared_heap/shared_full_gc-inl.h"

namespace panda::ecmascript {
void SharedGCMarkerBase::MarkRoots(RootVisitor &visitor, SharedMarkType markType, VMRootVisitType type)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkRoots");
    MarkSerializeRoots(visitor);
    MarkSharedModule(visitor);
    MarkStringCache(visitor);
    Runtime *runtime = Runtime::GetInstance();
    if (markType != SharedMarkType::CONCURRENT_MARK_REMARK) {
        // The approximate size is enough, because even if some thread creates and registers after here, it will keep
        // waiting in transition to RUNNING state before JSThread::SetReadyForGCIterating.
        rSetHandlers_.reserve(runtime->ApproximateThreadListSize());
        ASSERT(rSetHandlers_.empty());
    }
    runtime->GCIterateThreadList([&](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        auto vm = thread->GetEcmaVM();
        MarkLocalVMRoots(visitor, vm, markType, type);
        if (markType != SharedMarkType::CONCURRENT_MARK_REMARK) {
            CollectLocalVMRSet(vm);
        }
    });
}

void SharedGCMarkerBase::MarkLocalVMRoots(RootVisitor &visitor, EcmaVM *localVm, SharedMarkType markType,
                                          VMRootVisitType type)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkLocalVMRoots");
    Heap *heap = const_cast<Heap*>(localVm->GetHeap());
    if (markType != SharedMarkType::CONCURRENT_MARK_REMARK) {
        heap->GetSweeper()->EnsureAllTaskFinished();
    }
    ObjectXRay::VisitVMRoots(localVm, visitor, type);
    heap->ProcessSharedGCMarkingLocalBuffer();
}

void SharedGCMarkerBase::CollectLocalVMRSet(EcmaVM *localVm)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarkerBase::CollectLocalVMRSet");
    Heap *heap = const_cast<Heap*>(localVm->GetHeap());
    RSetWorkListHandler *handler = new RSetWorkListHandler(heap, localVm->GetJSThreadNoCheck());
    heap->SetRSetWorkListHandler(handler);
    NotifyThreadProcessRsetStart(handler->GetOwnerThreadUnsafe());
    rSetHandlers_.emplace_back(handler);
}

void SharedGCMarkerBase::MarkSerializeRoots(RootVisitor &visitor)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkSerializeRoots");
    Runtime::GetInstance()->IterateSerializeRoot(visitor);
}

void SharedGCMarkerBase::MarkStringCache(RootVisitor &visitor)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkStringCache");
    Runtime::GetInstance()->IterateCachedStringRoot(visitor);
}

void SharedGCMarkerBase::MarkSharedModule(RootVisitor &visitor)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkSharedModule");
    SharedModuleManager::GetInstance()->Iterate(visitor);
}

void SharedGCMarker::ProcessMarkStack(uint32_t threadId)
{
#ifndef NDEBUG
    DaemonThread *dThread = DaemonThread::GetInstance();
    if (UNLIKELY(!dThread->IsRunning())) {
        // This DAEMON_THREAD_INDEX not means in daemon thread, but the daemon thread is terminated, and
        // SharedGC is directly running in the current js thread, this maybe happen only AppSpawn
        // trigger GC after PreFork (which is not expected), and at this time ParallelGC is disabled
        ASSERT(threadId == DAEMON_THREAD_INDEX);
    } else {
        if (os::thread::GetCurrentThreadId() != dThread->GetThreadId()) {
            ASSERT(threadId != 0);
        } else {
            ASSERT(threadId == 0);
        }
    }
#endif
    SharedGCMarkObjectVisitor sharedGCMarkObjectVisitor(sWorkManager_, threadId);
    TaggedObject *obj = nullptr;
    while (sWorkManager_->Pop(threadId, &obj)) {
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
#ifndef NDEBUG
    DaemonThread *dThread = DaemonThread::GetInstance();
    if (UNLIKELY(!dThread->IsRunning())) {
        // This DAEMON_THREAD_INDEX not means in daemon thread, but the daemon thread is terminated, and
        // SharedGC is directly running in the current js thread, this maybe happen only AppSpawn
        // trigger GC after PreFork (which is not expected), and at this time ParallelGC is disabled
        ASSERT(threadId == DAEMON_THREAD_INDEX);
    } else {
        if (os::thread::GetCurrentThreadId() != dThread->GetThreadId()) {
            ASSERT(threadId != 0);
        } else {
            ASSERT(threadId == 0);
        }
    }
#endif
    SharedFullGCMarkObjectVisitor sharedFullGCMarkObjectVisitor(this, threadId);
    TaggedObject *obj = nullptr;
    while (sWorkManager_->Pop(threadId, &obj)) {
        JSHClass *hclass = obj->SynchronizedGetClass();
        ObjectSlot objectSlot(ToUintPtr(obj));
        MarkObject(threadId, hclass, objectSlot);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, hclass, sharedFullGCMarkObjectVisitor);
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

SharedGCMarker::SharedGCMarker(SharedGCWorkManager *workManger)
    : SharedGCMarkerBase(workManger) {}

SharedGCMovableMarker::SharedGCMovableMarker(SharedGCWorkManager *workManger, SharedHeap *sHeap)
    : SharedGCMarkerBase(workManger), sHeap_(sHeap) {}

}  // namespace panda::ecmascript