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

#ifndef ECMASCRIPT_MEM_YOUNG_GC_VISITOR_INL_H
#define ECMASCRIPT_MEM_YOUNG_GC_VISITOR_INL_H

#include "ecmascript/mem/young_gc_visitor.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/work_manager-inl.h"

namespace panda::ecmascript {
YoungGCMarkRootVisitor::YoungGCMarkRootVisitor(WorkNodeHolder *workNodeHolder) : workNodeHolder_(workNodeHolder) {}

void YoungGCMarkRootVisitor::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    HandleSlot(slot);
}

void YoungGCMarkRootVisitor::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        HandleSlot(slot);
    }
}

void YoungGCMarkRootVisitor::VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
                                                     [[maybe_unused]] ObjectSlot derived,
                                                     [[maybe_unused]] uintptr_t baseOldObject)
{
    // It is only used to update the derived value. The mark of YoungGC does not need to update slot
}

void YoungGCMarkRootVisitor::HandleSlot(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return;
    }

    ASSERT(!value.IsWeak());
    TaggedObject *object = value.GetTaggedObject();
    Region *objectRegion = Region::ObjectAddressToRange(object);

    if (!objectRegion->InYoungSpace()) {
        return;
    }

    if (objectRegion->IsFreshRegion()) {
        // This should only happen in MarkRoot from js thread.
        ASSERT(JSThread::GetCurrent() != nullptr);
        objectRegion->NonAtomicMark(object);
    } else if (objectRegion->AtomicMark(object)) {
        workNodeHolder_->Push(object);
    }
}

YoungGCMarkObjectVisitor::YoungGCMarkObjectVisitor(WorkNodeHolder *workNodeHolder) : workNodeHolder_(workNodeHolder) {}

void YoungGCMarkObjectVisitor::VisitObjectRangeImpl(BaseObject *root, uintptr_t startAddr, uintptr_t endAddr,
                                                    VisitObjectArea area)
{
    ObjectSlot start(startAddr);
    ObjectSlot end(endAddr);
    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        JSHClass *hclass = TaggedObject::Cast(root)->SynchronizedGetClass();
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

void YoungGCMarkObjectVisitor::HandleSlot(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject() || value.IsWeakForHeapObject()) {
        return;
    }

    TaggedObject *object = value.GetTaggedObject();
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (!objectRegion->InYoungSpace() || objectRegion->IsFreshRegion()) {
        return;
    }

    if (objectRegion->AtomicMark(object)) {
        workNodeHolder_->Push(object);
    }
}

YoungGCMarkOldToNewRSetVisitor::YoungGCMarkOldToNewRSetVisitor(WorkNodeHolder *workNodeHolder)
    : workNodeHolder_(workNodeHolder) {}

void YoungGCMarkOldToNewRSetVisitor::operator()(Region *region) const
{
    region->IterateAllOldToNewBits([this](void *mem) -> bool {
        ObjectSlot slot(ToUintPtr(mem));
        return HandleSlot(slot);
    });
}

bool YoungGCMarkOldToNewRSetVisitor::HandleSlot(ObjectSlot slot) const
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return false;
    }

    Region *region = Region::ObjectAddressToRange(value.GetRawHeapObject());
    if (!region->InYoungSpace()) {
        return false;
    }

    // In initial mark, all regions are non-fresh.
    ASSERT(!region->IsFreshRegion());

    if (value.IsWeakForHeapObject()) {
        // Keep OldToNew to update weak reference.
        return true;
    }

    TaggedObject *object = value.GetTaggedObject();
    if (region->AtomicMark(object)) {
        workNodeHolder_->Push(object);
    }
    return true;
}

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_YOUNG_GC_VISITOR_INL_H
