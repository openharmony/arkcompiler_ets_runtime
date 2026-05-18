/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_GC_VISITOR_INL_H
#define ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_GC_VISITOR_INL_H

#include "ecmascript/mem/shared_heap/global_gc_visitor.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/region-inl.h"

namespace panda::ecmascript {

void GlobalMarkRootVisitor::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    MarkLocalObject(slot);
}

void GlobalMarkRootVisitor::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        MarkLocalObject(slot);
    }
}

void GlobalMarkRootVisitor::MarkLocalObject(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return;
    }
    ASSERT(!value.IsWeak());
    TaggedObject *object = value.GetTaggedObject();
    Region *region = Region::ObjectAddressToRange(object);
    if (region->InSharedHeap()) {
        return;
    }
    if (region->AtomicMark(object)) {
        holder_->Push(object);
    }
}

void GlobalMarkObjectVisitor::VisitObjectRangeImpl(BaseObject *root, uintptr_t startAddr, uintptr_t endAddr,
                                                   VisitObjectArea area)
{
    ObjectSlot start(startAddr);
    ObjectSlot end(endAddr);
    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        JSHClass *hclass = TaggedObject::Cast(root)->GetClass();
        ASSERT(!hclass->IsAllTaggedProp());
        int index = 0;
        LayoutInfo *layout = LayoutInfo::UncheckCast(
            hclass->GetLayout<RBMode::FAST_NO_RB>(THREAD_ARG_PLACEHOLDER).GetTaggedObject());
        ObjectSlot realEnd = start;
        realEnd += layout->GetPropertiesCapacity();
        end = end > realEnd ? realEnd : end;
        for (ObjectSlot slot = start; slot < end; slot++) {
            PropertyAttributes attr = layout->GetAttr<RBMode::FAST_NO_RB>(THREAD_ARG_PLACEHOLDER, index++);
            if (attr.IsTaggedRep()) {
                MarkLocalRefFromSlot(slot);
            }
        }
        return;
    }
    for (ObjectSlot slot = start; slot < end; slot++) {
        MarkLocalRefFromSlot(slot);
    }
}

void GlobalMarkObjectVisitor::VisitObjectHClassImpl(BaseObject *hclassObject)
{
    auto hclass = reinterpret_cast<TaggedObject *>(hclassObject);
    HandleObject(hclass, Region::ObjectAddressToRange(hclass));
}

void GlobalMarkObjectVisitor::HandleObject(TaggedObject *object, Region *objectRegion)
{
    if (objectRegion->InSharedHeap()) {
        return;
    }
    if (objectRegion->AtomicMark(object)) {
        holder_->Push(object);
    }
}

void GlobalMarkObjectVisitor::MarkLocalRefFromSlot(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return;
    }
    TaggedObject *obj = value.GetHeapObject();
    HandleObject(obj, Region::ObjectAddressToRange(obj));
}

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_GLOBAL_GC_VISITOR_INL_H
