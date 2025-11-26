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

#ifndef ECMASCRIPT_MEM_LOCAL_CMC_CC_EVACUATOR_INL_H
#define ECMASCRIPT_MEM_LOCAL_CMC_CC_EVACUATOR_INL_H

#include "ecmascript/mem/local_cmc/cc_evacuator.h"

#include "ecmascript/mem/object_xray.h"
namespace panda::ecmascript {
void CCUpdateRootVisitor::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    HandleSlot(slot);
}

void CCUpdateRootVisitor::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        HandleSlot(slot);
    }
}

void CCUpdateRootVisitor::VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base, ObjectSlot derived,
                                                  uintptr_t baseOldObject)
{
    if (JSTaggedValue(base.GetTaggedType()).IsHeapObject()) {
        derived.Update(base.GetTaggedType() + derived.GetTaggedType() - baseOldObject);
    }
}

void CCUpdateRootVisitor::HandleSlot(ObjectSlot slot)
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
    if (objectRegion->IsFromRegion()) {
        MarkWord markWord(object);
        if (markWord.IsForwardingAddress()) {
            TaggedObject *dst = markWord.ToForwardingAddress();
            slot.Update(dst);
            return;
        }
        TaggedObject *ref = evacuator_->Copy(object, markWord);
        slot.Update(ref);
    }
}

TaggedObject* CCUpdateRootVisitor::operator()(TaggedObject *header)
{
    Region *objectRegion = Region::ObjectAddressToRange(header);
    if (UNLIKELY(objectRegion == nullptr)) {
        return nullptr;
    }
    if (objectRegion->IsToRegion() || objectRegion->InSharedHeap()) {
        return header;
    } else if (!objectRegion->Test(header)) {
        return nullptr;
    } else if (objectRegion->IsFromRegion()) {
        MarkWord markWord(header);
        if (markWord.IsForwardingAddress()) {
            return markWord.ToForwardingAddress();
        } else {
            TaggedObject *ref = evacuator_->Copy(header, markWord);
            return ref;
        }
    } else {
        return header;
    }
}

void CCUpdateVisitor::VisitObjectRangeImpl(BaseObject *root, uintptr_t start, uintptr_t end,
                                           VisitObjectArea area)
{
    ObjectSlot startSlot(start);
    ObjectSlot endSlot(end);
    auto rootObject = TaggedObject::Cast(root);
    Region *rootRegion = Region::ObjectAddressToRange(rootObject);
    ASSERT(!rootRegion->InSharedHeap());
    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        JSHClass *hclass = rootObject->SynchronizedGetClass();
        ASSERT(!hclass->IsAllTaggedProp());
        int index = 0;
        LayoutInfo *layout = LayoutInfo::UncheckCast(hclass->GetLayout(thread_).GetTaggedObject());
        ObjectSlot realEnd(start);
        realEnd += layout->GetPropertiesCapacity();
        endSlot = endSlot > realEnd ? realEnd : endSlot;
        for (ObjectSlot slot = startSlot; slot < endSlot; slot++) {
            PropertyAttributes attr = layout->GetAttr(thread_, index++);
            if (attr.IsTaggedRep()) {
                HandleSlot(slot, rootRegion);
            }
        }
        return;
    }
    for (ObjectSlot slot = startSlot; slot < endSlot; slot++) {
        HandleSlot(slot, rootRegion);
    }
}

void CCUpdateVisitor::HandleSlot(ObjectSlot slot, Region *rootRegion)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return;
    }
    Region *objectRegion = Region::ObjectAddressToRange(value.GetRawHeapObject());
    if (objectRegion->InSharedSweepableSpace()) {
        if (needBarrier_) {
            rootRegion->InsertLocalToShareRSetForCC(slot.SlotAddress());
        }
        return;
    }
    if (objectRegion->IsFromRegion()) {
        TaggedObject *rawObject = value.GetRawHeapObject();
        TaggedObject *object = value.GetHeapObject();
        MarkWord markWord(object);
        ASSERT(markWord.IsForwardingAddress());
        TaggedObject *dst = markWord.ToForwardingAddress();
        if (value.IsWeakForHeapObject()) {
            JSTaggedValue dstValue = JSTaggedValue(dst);
            dstValue.CreateWeakRef();
            dst = dstValue.GetRawHeapObject();
            slot.CASUpdate(rawObject, dst);
        } else {
            slot.CASUpdate(rawObject, dst);
        }
    }
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_LOCAL_CMC_CC_EVACUATOR_INL_H
