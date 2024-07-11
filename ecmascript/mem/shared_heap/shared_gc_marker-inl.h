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

namespace panda::ecmascript {
inline void SharedGCMarker::MarkObject(uint32_t threadId, TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    ASSERT(objectRegion->InSharedHeap());
    if (!objectRegion->InSharedReadOnlySpace() && objectRegion->AtomicMark(object)) {
        sWorkManager_->Push(threadId, object);
    }
}

inline void SharedGCMarker::MarkObjectFromJSThread(WorkNode *&localBuffer, TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    ASSERT(objectRegion->InSharedHeap());
    if (!objectRegion->InSharedReadOnlySpace() && objectRegion->AtomicMark(object)) {
        sWorkManager_->PushToLocalMarkingBuffer(localBuffer, object);
    }
}

inline void SharedGCMarker::MarkValue(uint32_t threadId, ObjectSlot &slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsInSharedSweepableSpace()) {
        if (!value.IsWeakForHeapObject()) {
            MarkObject(threadId, value.GetTaggedObject());
        } else {
            RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(slot.SlotAddress()));
        }
    }
}

inline void SharedGCMarker::HandleRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsInSharedSweepableSpace()) {
        MarkObject(threadId, value.GetTaggedObject());
    }
}

inline void SharedGCMarker::HandleLocalRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsInSharedSweepableSpace()) {
        MarkObject(threadId, value.GetTaggedObject());
    }
}

inline void SharedGCMarker::HandleLocalRangeRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot start,
    ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsInSharedSweepableSpace()) {
            if (value.IsWeakForHeapObject()) {
                LOG_ECMA_MEM(FATAL) << "Weak Reference in SharedGCMarker roots";
            }
            MarkObject(threadId, value.GetTaggedObject());
        }
    }
}

inline void SharedGCMarker::HandleLocalDerivedRoots([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
                                                    [[maybe_unused]] ObjectSlot derived,
                                                    [[maybe_unused]] uintptr_t baseOldObject)
{
    // It is only used to update the derived value. The mark of share GC does not need to update slot
}

template <typename Callback>
ARK_INLINE bool SharedGCMarker::VisitBodyInObj(TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                               Callback callback)
{
    auto hclass = root->SynchronizedGetClass();
    int index = 0;
    auto layout = LayoutInfo::UncheckCast(hclass->GetLayout().GetTaggedObject());
    ObjectSlot realEnd = start;
    realEnd += layout->GetPropertiesCapacity();
    end = end > realEnd ? realEnd : end;
    for (ObjectSlot slot = start; slot < end; slot++) {
        auto attr = layout->GetAttr(index++);
        if (attr.IsTaggedRep()) {
            callback(slot);
        }
    }
    return true;
}

inline void SharedGCMarker::RecordWeakReference(uint32_t threadId, JSTaggedType *slot)
{
    sWorkManager_->PushWeakReference(threadId, slot);
}

inline void SharedGCMarker::RecordObject(JSTaggedValue value, uint32_t threadId, void *mem)
{
    if (value.IsWeakForHeapObject()) {
        RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(mem));
    } else {
        MarkObject(threadId, value.GetTaggedObject());
    }
}

template<SharedMarkType markType>
inline bool SharedGCMarker::GetVisitor(JSTaggedValue value, uint32_t threadId, void *mem)
{
    if (value.IsInSharedSweepableSpace()) {
        if constexpr (markType == SharedMarkType::CONCURRENT_MARK_INITIAL_MARK) {
            // For now if record weak references from local to share in marking root, the slots
            // may be invalid due to LocalGC, so just mark them as strong-reference.
            MarkObject(threadId, value.GetHeapObject());
        } else {
            static_assert(markType == SharedMarkType::NOT_CONCURRENT_MARK);
            RecordObject(value, threadId, mem);
        }
        return true;
    }
    return false;
}

template<SharedMarkType markType>
inline auto SharedGCMarker::GenerateRSetVisitor(uint32_t threadId)
{
    auto visitor = [this, threadId](void *mem) -> bool {
        ObjectSlot slot(ToUintPtr(mem));
        JSTaggedValue value(slot.GetTaggedType());
        
        return GetVisitor<markType>(value, threadId, mem);
    };
    return visitor;
}

template<SharedMarkType markType>
inline void SharedGCMarker::ProcessVisitorOfDoMark(uint32_t threadId)
{
    auto rSetVisitor = GenerateRSetVisitor<markType>(threadId);
    auto visitor = [rSetVisitor](Region *region, RememberedSet *rSet) {
        rSet->IterateAllMarkedBits(ToUintPtr(region), rSetVisitor);
    };
    for (RSetWorkListHandler *handler : rSetHandlers_) {
        ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarker::ProcessRSet");
        handler->ProcessAll(visitor);
    }
}

template<SharedMarkType markType>
inline void SharedGCMarker::DoMark(uint32_t threadId)
{
    if constexpr (markType != SharedMarkType::CONCURRENT_MARK_REMARK) {
        ProcessVisitorOfDoMark<markType>(threadId);
    }
    ProcessMarkStack(threadId);
}

inline bool SharedGCMarker::MarkObjectOfProcessVisitor(void *mem, WorkNode *&localBuffer)
{
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
}

inline void SharedGCMarker::ProcessVisitor(RSetWorkListHandler *handler)
{
    WorkNode *&localBuffer = handler->GetHeap()->GetMarkingObjectLocalBuffer();
    auto rSetVisitor = [this, &localBuffer](void *mem) -> bool {
        return MarkObjectOfProcessVisitor(mem, localBuffer);
    };
    auto visitor = [rSetVisitor](Region *region, RememberedSet *rSet) {
        rSet->IterateAllMarkedBits(ToUintPtr(region), rSetVisitor);
    };
    handler->ProcessAll(visitor);
}

inline void SharedGCMarker::ProcessThenMergeBackRSetFromBoundJSThread(RSetWorkListHandler *handler)
{
    ECMA_BYTRACE_NAME(HITRACE_TAG_ARK, "SharedGCMarker::ProcessRSet");
    ASSERT(JSThread::GetCurrent() == handler->GetHeap()->GetEcmaVM()->GetJSThread());
    ASSERT(JSThread::GetCurrent()->IsInRunningState());
    ProcessVisitor(handler);
    handler->WaitFinishedThenMergeBack();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_MARKER_INL_H
