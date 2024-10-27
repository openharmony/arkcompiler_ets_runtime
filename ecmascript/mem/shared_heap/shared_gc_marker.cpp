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
#include "ecmascript/runtime.h"

namespace panda::ecmascript {
void SharedGCMarkerBase::MarkRoots(uint32_t threadId, SharedMarkType markType, VMRootVisitType type)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkRoots");
    MarkSerializeRoots(threadId);
    MarkSharedModule(threadId);
    MarkStringCache(threadId);
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
        MarkLocalVMRoots(threadId, vm, markType, type);
        if (markType != SharedMarkType::CONCURRENT_MARK_REMARK) {
            CollectLocalVMRSet(vm);
        }
    });
}

void SharedGCMarkerBase::MarkLocalVMRoots(uint32_t threadId, EcmaVM *localVm, SharedMarkType markType,
                                          VMRootVisitType type)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkLocalVMRoots");
    Heap *heap = const_cast<Heap*>(localVm->GetHeap());
    if (markType != SharedMarkType::CONCURRENT_MARK_REMARK) {
        heap->GetSweeper()->EnsureAllTaskFinished();
    }
    ObjectXRay::VisitVMRoots(
        localVm,
        [this, threadId](Root type, ObjectSlot slot) {this->HandleLocalRoots(threadId, type, slot);},
        [this, threadId](Root type, ObjectSlot start, ObjectSlot end) {
            this->HandleLocalRangeRoots(threadId, type, start, end);
        },
        [this](Root type, ObjectSlot base, ObjectSlot derived, uintptr_t baseOldObject) {
            this->HandleLocalDerivedRoots(type, base, derived, baseOldObject);
        }, type);
    heap->ProcessSharedGCMarkingLocalBuffer();
}

void SharedGCMarkerBase::CollectLocalVMRSet(EcmaVM *localVm)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarkerBase::CollectLocalVMRSet");
    Heap *heap = const_cast<Heap*>(localVm->GetHeap());
    RSetWorkListHandler *handler = new RSetWorkListHandler(heap);
    heap->SetRSetWorkListHandler(handler);
    heap->SetProcessingRset(true);
    rSetHandlers_.emplace_back(handler);
}

void SharedGCMarkerBase::MarkSerializeRoots(uint32_t threadId)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkSerializeRoots");
    auto callback = [this, threadId](Root type, ObjectSlot slot) {this->HandleRoots(threadId, type, slot);};
    Runtime::GetInstance()->IterateSerializeRoot(callback);
}

void SharedGCMarkerBase::MarkStringCache(uint32_t threadId)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkStringCache");
    auto cacheStringCallback = [this, threadId](Root type, ObjectSlot start, ObjectSlot end) {
        this->HandleLocalRangeRoots(threadId, type, start, end);
    };
    Runtime::GetInstance()->IterateCachedStringRoot(cacheStringCallback);
}

void SharedGCMarkerBase::MarkSharedModule(uint32_t threadId)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarkerBase::MarkSharedModule");
    auto visitor = [this, threadId](Root type, ObjectSlot slot) {this->HandleRoots(threadId, type, slot);};
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
    auto cb = [&](ObjectSlot slot) {
        MarkValue(threadId, slot);
    };
    EcmaObjectRangeVisitor visitor = [this, threadId, cb](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                        VisitObjectArea area) {
        if (area == VisitObjectArea::IN_OBJECT) {
            if (VisitBodyInObj(root, start, end, cb)) {
                return;
            }
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            MarkValue(threadId, slot);
        }
    };
    TaggedObject *obj = nullptr;
    while (true) {
        obj = nullptr;
        if (!sWorkManager_->Pop(threadId, &obj)) {
            break;
        }
        JSHClass *hclass = obj->SynchronizedGetClass();
        auto size = hclass->SizeFromJSHClass(obj);
        Region *region = Region::ObjectAddressToRange(obj);
        ASSERT(region->InSharedSweepableSpace());
        region->IncreaseAliveObjectSafe(size);
        ObjectSlot objectSlot(ToUintPtr(obj));
        MarkObject(threadId, hclass, objectSlot);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, hclass, visitor);
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
    auto cb = [&](ObjectSlot slot) {
        MarkValue(threadId, slot);
    };
    EcmaObjectRangeVisitor visitor = [this, threadId, cb](TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                        VisitObjectArea area) {
        if (area == VisitObjectArea::IN_OBJECT) {
            if (VisitBodyInObj(root, start, end, cb)) {
                return;
            }
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            MarkValue(threadId, slot);
        }
    };
    TaggedObject *obj = nullptr;
    while (true) {
        obj = nullptr;
        if (!sWorkManager_->Pop(threadId, &obj)) {
            break;
        }
        JSHClass *hclass = obj->SynchronizedGetClass();
        [[maybe_unused]] Region *region = Region::ObjectAddressToRange(obj);
        ObjectSlot objectSlot(ToUintPtr(obj));
        MarkObject(threadId, hclass, objectSlot);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, hclass, visitor);
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