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

#include "ecmascript/mem/parallel_evacuator.h"

namespace panda::ecmascript {
template<TriggerGCType gcType>
void NewToOldEvacuationVisitor<gcType>::operator()(void *mem)
{
    auto object = reinterpret_cast<TaggedObject *>(mem);
    JSHClass *klass = object->GetClass();
    ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, klass, SlotUpdateRangeVisitor);
    if (pgoEnabled_) {
        UpdateTrackInfo(object, klass);
    }
}

template<TriggerGCType gcType>
void NewToOldEvacuationVisitor<gcType>::SlotUpdateRangeVisitor(TaggedObject *root, ObjectSlot start,
                                                               ObjectSlot end, VisitObjectArea area)
{
    auto updateSlot = [root](ObjectSlot slot) {
        ParallelEvacuator::UpdateNewObjectSlot<gcType, false>(slot);
        JSTaggedValue value(slot.GetTaggedType());
        if (!value.IsHeapObject()) {
            return;
        }
        Region *valueRegion = Region::ObjectAddressToRange(slot.GetTaggedObject());
        if (valueRegion->InYoungSpace()) {
            Region *rootRegion = Region::ObjectAddressToRange(root);
            rootRegion->InsertOldToNewRSet(slot.SlotAddress());
        }
    };
    if (area == VisitObjectArea::IN_OBJECT &&
        ParallelEvacuator::VisitBodyInObj(root, start, end, updateSlot)) {
        return;
    }
    for (ObjectSlot slot = start; slot < end; slot++) {
        updateSlot(slot);
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