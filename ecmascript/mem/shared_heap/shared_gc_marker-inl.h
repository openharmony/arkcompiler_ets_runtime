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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_MARKER_INL_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_MARKER_INL_H

#include "ecmascript/mem/shared_heap/shared_gc_marker.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/tlab_allocator-inl.h"
#include "ecmascript/mem/work_manager-inl.h"

namespace panda::ecmascript {

inline void SharedGCMarkerBase::MarkObjectFromJSThread(MarkWorkNode *&localBuffer, TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    ASSERT(objectRegion->InSharedHeap());
    if (!objectRegion->InSharedReadOnlySpace() && objectRegion->AtomicMark(object)) {
        sWorkManager_->PushToLocalMarkingBuffer(localBuffer, object);
    }
}

template <typename LocalToShareRSetVisitor>
void SharedGCMarkerBase::ProcessLocalToShareRSet(LocalToShareRSetVisitor &&visitor)
{
    for (RSetWorkListHandler *handler : rSetHandlers_) {
        ECMA_BYTRACE_NAME(HITRACE_LEVEL_COMMERCIAL, HITRACE_TAG_ARK, "SharedGCMarker::ProcessRSet", "");
        handler->ProcessAll(visitor);
        // To ensure the accuracy of the state range, notify finished is executed on js thread and deamon thread.
        // Reentrant does not cause exceptions because all the values are set to false.
        NotifyThreadProcessRsetFinished(handler->GetOwnerThreadUnsafe());
    }
}

inline void SharedGCMarkerBase::NotifyThreadProcessRsetStart(JSThread *localThread)
{
    // This method is called within the GCIterateThreadList method,
    // so the thread lock problem does not need to be considered.
    ASSERT(localThread != nullptr);
    localThread->SetProcessingLocalToSharedRset(true);
}

inline void SharedGCMarkerBase::NotifyThreadProcessRsetFinished(JSThread *localThread)
{
    // The localThread may have been released or reused.
    Runtime::GetInstance()->GCIterateThreadList([localThread](JSThread *thread) {
        if (localThread == thread) {
            thread->SetProcessingLocalToSharedRset(false);
            return;
        }
    });
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_MARKER_INL_H
