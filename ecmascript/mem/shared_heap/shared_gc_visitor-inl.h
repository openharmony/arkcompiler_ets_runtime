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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_VISITOR_INL_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_VISITOR_INL_H

#include "ecmascript/mem/shared_heap/shared_gc_visitor.h"

#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/work_manager-inl.h"

namespace panda::ecmascript {
SharedGCMarkRootVisitor::SharedGCMarkRootVisitor(SharedGCWorkManager *sWorkManager, uint32_t threadId)
    : sWorkManager_(sWorkManager), threadId_(threadId) {}

void SharedGCMarkRootVisitor::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        ASSERT(!value.IsWeak());
        MarkObject(value.GetTaggedObject());
    }
}

void SharedGCMarkRootVisitor::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            ASSERT(!value.IsWeak());
            MarkObject(value.GetTaggedObject());
        }
    }
}

void SharedGCMarkRootVisitor::VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
                                                      [[maybe_unused]] ObjectSlot derived,
                                                      [[maybe_unused]] uintptr_t baseOldObject)
{
    // It is only used to update the derived value. The mark of SharedGC does not need to update slot
}

void SharedGCMarkRootVisitor::MarkObject(TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (!objectRegion->InSharedSweepableSpace()) {
        return;
    }
    if (objectRegion->AtomicMark(object)) {
        sWorkManager_->Push(threadId_, object);
    }
}

SharedGCMarkObjectVisitor::SharedGCMarkObjectVisitor(SharedGCWorkManager *sWorkManager, uint32_t threadId)
    : sWorkManager_(sWorkManager), threadId_(threadId) {}

void SharedGCMarkObjectVisitor::VisitObjectRangeImpl(TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                                     VisitObjectArea area)
{
    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        JSHClass *hclass = root->SynchronizedGetClass();
        ASSERT(!hclass->IsAllTaggedProp());
        int index = 0;
        LayoutInfo *layout = LayoutInfo::UncheckCast(hclass->GetLayout().GetTaggedObject());
        ObjectSlot realEnd = start;
        realEnd += layout->GetPropertiesCapacity();
        end = end > realEnd ? realEnd : end;
        for (ObjectSlot slot = start; slot < end; slot++) {
            PropertyAttributes attr = layout->GetAttr(index++);
            if (attr.IsTaggedRep()) {
                HandleSlot(slot);
            }
        }
        return;
    }
    for (ObjectSlot slot = start; slot < end; slot++) {
        HandleSlot(slot);
    }
}

void SharedGCMarkObjectVisitor::VisitObjectHClassImpl(TaggedObject *hclass)
{
    ASSERT(hclass->GetClass()->IsHClass());
    Region *hclassRegion = Region::ObjectAddressToRange(hclass);
    if (hclassRegion->InSharedSweepableSpace()) {
        ASSERT(hclassRegion->InSharedNonMovableSpace());
        MarkAndPush(hclass, hclassRegion);
    }
}

void SharedGCMarkObjectVisitor::HandleSlot(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return;
    }

    Region *objectRegion = Region::ObjectAddressToRange(value.GetRawHeapObject());
    ASSERT(objectRegion->InSharedHeap());

    if (!objectRegion->InSharedSweepableSpace()) {
        return;
    }

    if (!value.IsWeakForHeapObject()) {
        TaggedObject *object = value.GetTaggedObject();
        MarkAndPush(object, objectRegion);
    } else {
        RecordWeakReference(reinterpret_cast<JSTaggedType*>(slot.SlotAddress()));
    }
}

void SharedGCMarkObjectVisitor::MarkAndPush(TaggedObject *object, Region *objectRegion)
{
    if (objectRegion->AtomicMark(object)) {
        sWorkManager_->Push(threadId_, object);
    }
}

void SharedGCMarkObjectVisitor::RecordWeakReference(JSTaggedType *weak)
{
    sWorkManager_->PushWeakReference(threadId_, weak);
}

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_GC_VISITOR_INL_H
