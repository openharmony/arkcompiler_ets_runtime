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

#ifndef ECMASCRIPT_MEM_UNIFIED_GC_UNIFIED_GC_MARKER_INL_H
#define ECMASCRIPT_MEM_UNIFIED_GC_UNIFIED_GC_MARKER_INL_H

#include "ecmascript/mem/unified_gc/unified_gc_marker.h"

namespace panda::ecmascript {

template <typename Callback>
ARK_INLINE bool UnifiedGCMarker::VisitBodyInObj(TaggedObject *root, ObjectSlot start, ObjectSlot end,
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

inline void UnifiedGCMarker::MarkValue(uint32_t threadId, ObjectSlot &slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject() && !value.IsWeakForHeapObject()) {
        ASSERT(!value.IsHole());
        MarkObject(threadId, value.GetTaggedObject());
    }
}

inline void UnifiedGCMarker::MarkObject(uint32_t threadId, TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);

    if (objectRegion->InSharedHeap()) {
        return;
    }

#ifdef PANDA_JS_ETS_HYBRID_MODE
    HandleJSXRefObject(object);
#endif // PANDA_JS_ETS_HYBRID_MODE

    if (objectRegion->AtomicMark(object)) {
        workManager_->Push(threadId, object);
    }
}

#ifdef PANDA_JS_ETS_HYBRID_MODE
inline void UnifiedGCMarker::HandleJSXRefObject(TaggedObject *object)
{
    JSTaggedValue value(object);
    if (value.IsJSXRefObject()) {
        auto stsVMInterface = heap_->GetEcmaVM()->GetCrossVMOperator()->GetSTSVMInterface();
        stsVMInterface->MarkFromObject(JSObject::Cast(object)->GetNativePointerField(0));
    }
}
#endif // PANDA_JS_ETS_HYBRID_MODE

inline void UnifiedGCMarker::HandleRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        MarkObject(threadId, value.GetTaggedObject());
    }
}

inline void UnifiedGCMarker::HandleRangeRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot start,
    ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            MarkObject(threadId, value.GetTaggedObject());
        }
    }
}

inline void UnifiedGCMarker::HandleDerivedRoots([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
                                                [[maybe_unused]] ObjectSlot derived,
                                                [[maybe_unused]] uintptr_t baseOldObject)
{
}

inline void UnifiedGCMarker::HandleNewToEdenRSet([[maybe_unused]] uint32_t threadId, [[maybe_unused]] Region *region)
{
}

inline void UnifiedGCMarker::HandleOldToNewRSet([[maybe_unused]] uint32_t threadId, [[maybe_unused]] Region *region)
{
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_UNIFIED_GC_UNIFIED_GC_MARKER_INL_H