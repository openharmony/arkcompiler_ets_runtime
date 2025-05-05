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

#ifndef ECMASCRIPT_MEM_PARALLEL_EVACUATOR_VISITOR_INL_H
#define ECMASCRIPT_MEM_PARALLEL_EVACUATOR_VISITOR_INL_H

#include "ecmascript/mem/parallel_evacuator_visitor.h"

namespace panda::ecmascript {

template<TriggerGCType gcType>
SlotUpdateRangeVisitor<gcType>::SlotUpdateRangeVisitor(ParallelEvacuator *evacuator) : evacuator_(evacuator) {}

template<TriggerGCType gcType>
void SlotUpdateRangeVisitor<gcType>::VisitObjectRangeImpl(BaseObject *root, uintptr_t startAddr, uintptr_t endAddr,
                                                          VisitObjectArea area)
{
    Region *rootRegion = Region::ObjectAddressToRange(root);
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
                UpdateSlot(slot, rootRegion);
            }
        }
        return;
    }
    for (ObjectSlot slot = start; slot < end; slot++) {
        UpdateSlot(slot, rootRegion);
    }
}

template<TriggerGCType gcType>
void SlotUpdateRangeVisitor<gcType>::UpdateSlot(ObjectSlot slot, Region *rootRegion)
{
    evacuator_->UpdateNewObjectSlot<gcType, false>(slot);
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return;
    }
    Region *valueRegion = Region::ObjectAddressToRange(slot.GetTaggedObject());
    if (valueRegion->InYoungSpace()) {
        rootRegion->InsertOldToNewRSet(slot.SlotAddress());
    }
}

template<TriggerGCType gcType>
NewToOldEvacuationVisitor<gcType>::NewToOldEvacuationVisitor(Heap *heap, std::unordered_set<JSTaggedType> *set,
    ParallelEvacuator *evacuator) : pgoEnabled_(heap->GetJSThread()->IsPGOProfilerEnable()),
    pgoProfiler_(heap->GetEcmaVM()->GetPGOProfiler()), trackSet_(set), slotUpdateRangeVisitor_(evacuator) {}

template<TriggerGCType gcType>
void NewToOldEvacuationVisitor<gcType>::operator()(void *mem)
{
    auto object = reinterpret_cast<TaggedObject *>(mem);
    JSHClass *klass = object->GetClass();
    ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, klass, slotUpdateRangeVisitor_);
    if (pgoEnabled_) {
        UpdateTrackInfo(object, klass);
    }
}

template<TriggerGCType gcType>
void NewToOldEvacuationVisitor<gcType>::UpdateTrackInfo(TaggedObject *header, JSHClass *klass)
{
    if (klass->IsJSArray()) {
        auto trackInfo = JSArray::Cast(header)->GetTrackInfo();
        trackSet_->emplace(trackInfo.GetRawData());
    }
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_PARALLEL_EVACUATOR_VISITOR_INL_H
