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

inline void SharedGCMarker::HandleRangeRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot start,
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

inline void SharedGCMarker::HandleDerivedRoots([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
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

inline void SharedGCMarker::ProcessLocalToShare(uint32_t threadId, Heap *localHeap)
{
    localHeap->EnumerateRegions(
        std::bind(&SharedGCMarker::HandleLocalToShareRSet, this, threadId, std::placeholders::_1));
    ProcessMarkStack(threadId);
}

inline void SharedGCMarker::RecordWeakReference(uint32_t threadId, JSTaggedType *slot)
{
    sWorkManager_->PushWeakReference(threadId, slot);
}

// Don't call this function when mutator thread is running.
inline void SharedGCMarker::HandleLocalToShareRSet(uint32_t threadId, Region *region)
{
    // If the mem does not point to a shared object, the related bit in localToShareRSet will be cleared.
    region->AtomicIterateAllLocalToShareBits([this, threadId](void *mem) -> bool {
        ObjectSlot slot(ToUintPtr(mem));
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsInSharedSweepableSpace()) {
            if (value.IsWeakForHeapObject()) {
                RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(mem));
            } else {
                MarkObject(threadId, value.GetTaggedObject());
            }
            return true;
        } else {
            // clear bit.
            return false;
        }
    });
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_MARKER_INL_H
