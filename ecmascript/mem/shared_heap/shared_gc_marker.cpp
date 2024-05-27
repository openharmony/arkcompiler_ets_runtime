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
#include "ecmascript/mem/visitor.h"
#include "ecmascript/module/js_shared_module_manager.h"
#include "ecmascript/runtime.h"

namespace panda::ecmascript {
void SharedGCMarker::MarkRoots(uint32_t threadId, SharedMarkType markType)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarker::MarkRoots");
    MarkSerializeRoots(threadId);
    MarkSharedModule(threadId);
    MarkStringCache(threadId);
    Runtime::GetInstance()->GCIterateThreadList([&](JSThread *thread) {
        ASSERT(!thread->IsInRunningState());
        auto vm = thread->GetEcmaVM();
        MarkLocalVMRoots(threadId, vm, markType);
    });
}

void SharedGCMarker::MarkLocalVMRoots(uint32_t threadId, EcmaVM *localVm, SharedMarkType markType)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarker::MarkLocalRoots");
    Heap *heap = const_cast<Heap*>(localVm->GetHeap());
    heap->GetSweeper()->EnsureAllTaskFinished();
    heap->WaitClearTaskFinished();
    ObjectXRay::VisitVMRoots(
        localVm,
        std::bind(&SharedGCMarker::HandleLocalRoots, this, threadId, std::placeholders::_1, std::placeholders::_2),
        std::bind(&SharedGCMarker::HandleLocalRangeRoots, this, threadId, std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3),
        std::bind(&SharedGCMarker::HandleLocalDerivedRoots, this, std::placeholders::_1, std::placeholders::_2,
                  std::placeholders::_3, std::placeholders::_4));
    WorkNode *&localBuffer = heap->GetMarkingObjectLocalBuffer();
    if (localBuffer != nullptr) {
        sWorkManager_->PushLocalBufferToGlobal(localBuffer);
    }
    ProcessLocalToShareNoMarkStack(threadId, const_cast<Heap*>(localVm->GetHeap()), markType);
}

void SharedGCMarker::MarkSerializeRoots(uint32_t threadId)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarker::MarkSerializeRoots");
    auto callback =
        std::bind(&SharedGCMarker::HandleRoots, this, threadId, std::placeholders::_1, std::placeholders::_2);
    Runtime::GetInstance()->IterateSerializeRoot(callback);
}

void SharedGCMarker::MarkStringCache(uint32_t threadId)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarker::MarkStringCache");
    auto cacheStringCallback =
        std::bind(&SharedGCMarker::HandleLocalRangeRoots, this, threadId, std::placeholders::_1, std::placeholders::_2,
            std::placeholders::_3);
    Runtime::GetInstance()->IterateCachedStringRoot(cacheStringCallback);
}

void SharedGCMarker::MarkSharedModule(uint32_t threadId)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarker::MarkSharedModule");
    auto visitor = std::bind(&SharedGCMarker::HandleRoots, this, threadId, std::placeholders::_1,
                             std::placeholders::_2);
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
    auto visitor = [this, threadId, cb](TaggedObject *root, ObjectSlot start, ObjectSlot end,
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
        MarkObject(threadId, hclass);
        ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(obj, hclass, visitor);
    }
}

void SharedGCMarker::ResetWorkManager(SharedGCWorkManager *workManager)
{
    sWorkManager_ = workManager;
}
}  // namespace panda::ecmascript