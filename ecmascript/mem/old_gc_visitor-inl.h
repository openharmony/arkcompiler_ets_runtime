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

#ifndef ECMASCRIPT_MEM_OLD_GC_VISITOR_INL_H
#define ECMASCRIPT_MEM_OLD_GC_VISITOR_INL_H

#include "ecmascript/mem/old_gc_visitor.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/work_manager-inl.h"

namespace panda::ecmascript {
OldGCMarkRootVisitor::OldGCMarkRootVisitor(WorkNodeHolder *workNodeHolder) : workNodeHolder_(workNodeHolder) {}

void OldGCMarkRootVisitor::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    HandleSlot(slot);
}

void OldGCMarkRootVisitor::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        HandleSlot(slot);
    }
}

void OldGCMarkRootVisitor::VisitBaseAndDerivedRoot([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
                                                   [[maybe_unused]] ObjectSlot derived,
                                                   [[maybe_unused]] uintptr_t baseOldObject)
{
    // It is only used to update the derived value. The mark of OldGC does not need to update slot
}

void OldGCMarkRootVisitor::HandleSlot(ObjectSlot slot)
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

OldGCMarkObjectVisitor::OldGCMarkObjectVisitor(WorkNodeHolder *workNodeHolder) : workNodeHolder_(workNodeHolder) {}

void OldGCMarkObjectVisitor::VisitObjectRangeImpl(BaseObject *rootObject, uintptr_t start, uintptr_t end,
                                                  VisitObjectArea area)
{
    ObjectSlot startSlot(start);
    ObjectSlot endSlot(end);
    auto root = TaggedObject::Cast(rootObject);
    JSThread *thread = workNodeHolder_->GetJSThread();
    Region *rootRegion = Region::ObjectAddressToRange(root);
    bool rootNeedEvacuate = rootRegion->InYoungSpaceOrCSet();
    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        JSHClass *hclass = root->SynchronizedGetClass();
        ASSERT(!hclass->IsAllTaggedProp());
        int index = 0;
        LayoutInfo *layout = LayoutInfo::UncheckCast(hclass->GetLayout(thread).GetTaggedObject());
        ObjectSlot realEnd(start);
        realEnd += layout->GetPropertiesCapacity();
        endSlot = endSlot > realEnd ? realEnd : endSlot;
        for (ObjectSlot slot = startSlot; slot < endSlot; slot++) {
            PropertyAttributes attr = layout->GetAttr(thread, index++);
            if (attr.IsTaggedRep()) {
                HandleSlot(slot, rootRegion, rootNeedEvacuate);
            }
        }
        return;
    }
    for (ObjectSlot slot = startSlot; slot < endSlot; slot++) {
        HandleSlot(slot, rootRegion, rootNeedEvacuate);
    }
}

void OldGCMarkObjectVisitor::VisitJSWeakMapImpl(BaseObject *rootObject)
{
    TaggedObject *obj = TaggedObject::Cast(rootObject);
    ASSERT(JSTaggedValue(obj).IsJSWeakMap());
    workNodeHolder_->PushJSWeakMap(obj);
}

void OldGCMarkObjectVisitor::VisitObjectHClassImpl(BaseObject *hclassObject)
{
    auto hclass = reinterpret_cast<TaggedObject *>(hclassObject);
    ASSERT(hclass->GetClass()->IsHClass());
    Region *hclassRegion = Region::ObjectAddressToRange(hclass);
    if (!hclassRegion->InSharedHeap()) {
        ASSERT(hclassRegion->InNonMovableSpace() || hclassRegion->InReadOnlySpace());
        MarkAndPush(hclass, hclassRegion);
    }
}

void OldGCMarkObjectVisitor::HandleSlot(ObjectSlot slot, Region *rootRegion, bool rootNeedEvacuate)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return;
    }

    // Region is correct no matter value is weak or not.
    Region *objectRegion = Region::ObjectAddressToRange(value.GetRawHeapObject());

    if (!value.IsWeakForHeapObject()) {
        TaggedObject *object = value.GetTaggedObject();
        HandleObject(object, objectRegion);
    } else {
        bool objectNeedEvacuate = objectRegion->InYoungSpaceOrCSet();
        if (!rootNeedEvacuate && !objectNeedEvacuate) {
            // If `rootNeedEvacuate`, every slot will be updated when/after it has been evacuated.
            // Otherwise if `objectNeedEvacuate`, weak reference will be updated in UpdateReference by visiting RSet.
            RecordWeakReference(reinterpret_cast<JSTaggedType*>(slot.SlotAddress()));
        }
    }

    bool objectInCSet = objectRegion->InCollectSet();
    if (!rootNeedEvacuate && objectInCSet) {
        rootRegion->AtomicInsertCrossRegionRSet(slot.SlotAddress());
    }
}

void OldGCMarkObjectVisitor::HandleObject(TaggedObject *object, Region *objectRegion)
{
    if (!objectRegion->InSharedHeap() && !objectRegion->IsFreshRegion()) {
        MarkAndPush(object, objectRegion);
    }
}

void OldGCMarkObjectVisitor::MarkAndPush(TaggedObject *object, Region *objectRegion)
{
    if (objectRegion->AtomicMark(object)) {
        workNodeHolder_->Push(object);
    }
}

void OldGCMarkObjectVisitor::RecordWeakReference(JSTaggedType *weak)
{
    workNodeHolder_->PushWeakReference(weak);
}

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_OLD_GC_VISITOR_INL_H
