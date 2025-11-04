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

#ifndef ECMASCRIPT_MEM_CMS_MEM_SWEEP_GC_VISITOR_INL_H
#define ECMASCRIPT_MEM_CMS_MEM_SWEEP_GC_VISITOR_INL_H

#include "ecmascript/mem/cms_mem/sweep_gc_visitor.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/work_manager-inl.h"

namespace panda::ecmascript {
SweepGCMarkRootVisitor::SweepGCMarkRootVisitor(WorkNodeHolder *workNodeHolder) : workNodeHolder_(workNodeHolder)
{
}

void SweepGCMarkRootVisitor::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    HandleSlot(slot);
}

void SweepGCMarkRootVisitor::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        HandleSlot(slot);
    }
}

void SweepGCMarkRootVisitor::VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
                                                     [[maybe_unused]] ObjectSlot derived,
                                                     [[maybe_unused]] uintptr_t baseOldObject)
{
    // It is only used to update the derived value. The mark of SweepGC does not need to update slot
}

void SweepGCMarkRootVisitor::HandleSlot(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return;
    }

    ASSERT(!value.IsWeak());
    TaggedObject *object = value.GetTaggedObject();
    Region *objectRegion = Region::ObjectAddressToRange(object);

    if (objectRegion->InSharedHeap()) {
        return;
    }

    if (objectRegion->AtomicMark(object)) {
        workNodeHolder_->Push(object);
    }
}

SweepGCMarkObjectVisitor::SweepGCMarkObjectVisitor(WorkNodeHolder *workNodeHolder) : workNodeHolder_(workNodeHolder)
{
}

void SweepGCMarkObjectVisitor::VisitObjectRangeImpl(BaseObject *rootObject, uintptr_t start, uintptr_t end,
                                                    VisitObjectArea area)
{
    ObjectSlot startSlot(start);
    ObjectSlot endSlot(end);
    TaggedObject *root = TaggedObject::Cast(rootObject);
    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        JSHClass *hclass = root->SynchronizedGetClass();
        ASSERT(!hclass->IsAllTaggedProp());
        int index = 0;
        LayoutInfo *layout = LayoutInfo::UncheckCast(hclass->GetLayout<RBMode::FAST_NO_RB>(nullptr).GetTaggedObject());
        ObjectSlot realEnd(start);
        realEnd += layout->GetPropertiesCapacity();
        endSlot = endSlot > realEnd ? realEnd : endSlot;
        for (ObjectSlot slot = startSlot; slot < endSlot; slot++) {
            PropertyAttributes attr = layout->GetAttr<RBMode::FAST_NO_RB>(nullptr, index++);
            if (attr.IsTaggedRep()) {
                HandleSlot(slot);
            }
        }
        return;
    }
    for (ObjectSlot slot = startSlot; slot < endSlot; slot++) {
        HandleSlot(slot);
    }
}

void SweepGCMarkObjectVisitor::VisitJSWeakMapImpl(BaseObject *rootObject)
{
    TaggedObject *obj = TaggedObject::Cast(rootObject);
    ASSERT(JSTaggedValue(obj).IsJSWeakMap());
    ASSERT(!Region::ObjectAddressToRange(obj)->InSharedHeap());
    workNodeHolder_->PushJSWeakMap(obj);
}

void SweepGCMarkObjectVisitor::VisitObjectHClassImpl(BaseObject *hclassObject)
{
    auto hclass = reinterpret_cast<TaggedObject *>(hclassObject);
    ASSERT(hclass->GetClass()->IsHClass());
    Region *hclassRegion = Region::ObjectAddressToRange(hclass);
    if (!hclassRegion->InSharedHeap()) {
        ASSERT(hclassRegion->InNonMovableSpace() || hclassRegion->InReadOnlySpace());
        MarkAndPush(hclass, hclassRegion);
    }
}

void SweepGCMarkObjectVisitor::HandleSlot(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return;
    }

    if (!value.IsWeakForHeapObject()) {
        TaggedObject *object = value.GetTaggedObject();
        HandleObject(object);
    } else {
        RecordWeakReference(reinterpret_cast<JSTaggedType*>(slot.SlotAddress()));
    }
}

void SweepGCMarkObjectVisitor::HandleObject(TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (!objectRegion->InSharedHeap()) {
        MarkAndPush(object, objectRegion);
    }
}

void SweepGCMarkObjectVisitor::MarkAndPush(TaggedObject *object, Region *objectRegion)
{
    if (objectRegion->AtomicMark(object)) {
        workNodeHolder_->Push(object);
    }
}

void SweepGCMarkObjectVisitor::RecordWeakReference(JSTaggedType *weak)
{
    workNodeHolder_->PushWeakReference(weak);
}

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_CMS_MEM_SWEEP_GC_VISITOR_INL_H
