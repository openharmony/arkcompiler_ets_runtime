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

#ifndef ECMASCRIPT_MEM_LOCAL_CMC_CC_GC_VISITOR_INL_H
#define ECMASCRIPT_MEM_LOCAL_CMC_CC_GC_VISITOR_INL_H

#include "ecmascript/mem/local_cmc/cc_gc_visitor.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/work_manager-inl.h"

namespace panda::ecmascript {
void CCMarkRootVisitor::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    HandleSlot(slot);
}

void CCMarkRootVisitor::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        HandleSlot(slot);
    }
}

void CCMarkRootVisitor::HandleSlot(ObjectSlot slot)
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
    if (objectRegion->IsFreshRegion()) {
        // This should only happen in MarkRoot from js thread.
        ASSERT(JSThread::GetCurrent() != nullptr);
        objectRegion->NonAtomicMark(object);
    } else if (objectRegion->AtomicMark(object)) {
        workNodeHolder_->Push(object);
    }
}

void CCMarkObjectVisitor::VisitObjectRangeImpl(BaseObject *rootObject, uintptr_t start, uintptr_t end,
                                               VisitObjectArea area)
{
    ObjectSlot startSlot(start);
    ObjectSlot endSlot(end);
    auto root = TaggedObject::Cast(rootObject);
    ASSERT(!Region::ObjectAddressToRange(rootObject)->InSharedHeap());
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

void CCMarkObjectVisitor::VisitJSWeakMapImpl(BaseObject *rootObject)
{
    TaggedObject *obj = TaggedObject::Cast(rootObject);
    ASSERT(JSTaggedValue(obj).IsJSWeakMap());
    workNodeHolder_->PushJSWeakMap(obj);
}

void CCMarkObjectVisitor::VisitObjectHClassImpl(BaseObject *object)
{
    auto hclass = reinterpret_cast<TaggedObject *>(object);
    ASSERT(hclass->GetClass()->IsHClass());
    Region *hclassRegion = Region::ObjectAddressToRange(hclass);
    if (!hclassRegion->InSharedHeap()) {
        ASSERT(hclassRegion->InNonMovableSpace() || hclassRegion->InReadOnlySpace());
        if (hclassRegion->AtomicMark(object)) {
            workNodeHolder_->Push(hclass);
        }
    }
}

void CCMarkObjectVisitor::HandleSlot(ObjectSlot slot)
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

void CCMarkObjectVisitor::HandleObject(TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (objectRegion->InSharedHeap() || objectRegion->IsFreshRegion()) {
        return;
    }
    if (objectRegion->AtomicMark(object)) {
        workNodeHolder_->Push(object);
    }
}

void CCMarkObjectVisitor::RecordWeakReference(JSTaggedType *weak)
{
    workNodeHolder_->PushWeakReference(weak);
}

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_LOCAL_CMC_CC_GC_VISITOR_INL_H
