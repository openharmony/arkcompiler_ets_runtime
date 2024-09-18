/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_MEM_PARALLEL_EVACUATOR_INL_H
#define ECMASCRIPT_MEM_PARALLEL_EVACUATOR_INL_H

#include "ecmascript/mem/parallel_evacuator.h"

#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/taskpool/taskpool.h"

namespace panda::ecmascript {
// Move regions with a survival rate of more than 75% to new space
// Move regions when young space overshoot size is larger than max capacity.
bool ParallelEvacuator::IsWholeRegionEvacuate(Region *region)
{
    if ((static_cast<double>(region->AliveObject()) / region->GetSize()) > MIN_OBJECT_SURVIVAL_RATE &&
        !region->HasAgeMark()) {
        return true;
    }
    if (heap_->GetFromSpaceDuringEvacuation()->CommittedSizeIsLarge() && !region->HasAgeMark()) {
        return true;
    }
    return false;
}

bool ParallelEvacuator::WholeRegionEvacuate(Region *region)
{
    if (region->IsFreshRegion()) {
        ASSERT(region->InYoungSpace());
        return heap_->MoveYoungRegionSync(region);
    }
    bool isInYoung = region->InYoungSpace();
    bool isBelowAgeMark = region->BelowAgeMark();
    if (isInYoung && !isBelowAgeMark && IsWholeRegionEvacuate(region) && heap_->MoveYoungRegionSync(region)) {
        return true;
    }
    return false;
}

template <typename Callback>
bool ParallelEvacuator::VisitBodyInObj(
    TaggedObject *root, ObjectSlot start, ObjectSlot end, Callback callback)
{
    auto hclass = root->GetClass();
    ASSERT(!hclass->IsAllTaggedProp());
    int index = 0;
    TaggedObject *dst = hclass->GetLayout().GetTaggedObject();
    auto layout = LayoutInfo::UncheckCast(dst);
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

bool ParallelEvacuator::UpdateNewToEdenObjectSlot(ObjectSlot &slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return false;
    }
    TaggedObject *object = value.GetHeapObject();
    Region *valueRegion = Region::ObjectAddressToRange(object);

    // It is only update edenSpace object when iterate NewToEdenRSet
    if (!valueRegion->InEdenSpace()) {
        return false;
    }
    MarkWord markWord(object);
    if (markWord.IsForwardingAddress()) {
        TaggedObject *dst = markWord.ToForwardingAddress();
        if (value.IsWeakForHeapObject()) {
            dst = JSTaggedValue(dst).CreateAndGetWeakRef().GetRawTaggedObject();
        }
        slot.Update(dst);
    } else {
        if (value.IsWeakForHeapObject()) {
            slot.Clear();
        }
    }
    return false;
}

bool ParallelEvacuator::UpdateForwardedOldToNewObjectSlot(TaggedObject *object, ObjectSlot &slot, bool isWeak)
{
    MarkWord markWord(object);
    if (markWord.IsForwardingAddress()) {
        TaggedObject *dst = markWord.ToForwardingAddress();
        if (isWeak) {
            dst = JSTaggedValue(dst).CreateAndGetWeakRef().GetRawTaggedObject();
        }
        slot.Update(dst);
        Region *dstRegion = Region::ObjectAddressToRange(dst);
        // Keep oldToNewRSet when object is YoungSpace
        if (dstRegion->InYoungSpace()) {
            return true;
        }
    } else if (isWeak) {
        slot.Clear();
    }
    return false;
}

template<bool IsEdenGC>
bool ParallelEvacuator::UpdateOldToNewObjectSlot(ObjectSlot &slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return false;
    }
    TaggedObject *object = value.GetHeapObject();
    Region *valueRegion = Region::ObjectAddressToRange(object);
    if constexpr (IsEdenGC) {
        // only object in EdenSpace will be collect in EdenGC
        if (valueRegion->InEdenSpace()) {
            return UpdateForwardedOldToNewObjectSlot(object, slot, value.IsWeakForHeapObject());
        } else {
            // Keep oldToNewRSet when object is YoungSpace
            return valueRegion->InYoungSpace();
        }
    } else {
        // It is only update old to new object when iterate OldToNewRSet
        if (valueRegion->InGeneralNewSpace()) {
            if (!valueRegion->InNewToNewSet()) {
                return UpdateForwardedOldToNewObjectSlot(object, slot, value.IsWeakForHeapObject());
            }
            // move region from fromspace to tospace
            if (valueRegion->Test(object)) {
                return true;
            }
            if (value.IsWeakForHeapObject()) {
                slot.Clear();
            }
        }
    }
    return false;
}

void ParallelEvacuator::UpdateObjectSlot(ObjectSlot &slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        if (value.IsInSharedHeap()) {
            return;
        }
        if (value.IsWeakForHeapObject()) {
            return UpdateWeakObjectSlot(value.GetTaggedWeakRef(), slot);
        }
        TaggedObject *object = value.GetTaggedObject();
        MarkWord markWord(object);
        if (markWord.IsForwardingAddress()) {
            TaggedObject *dst = markWord.ToForwardingAddress();
            slot.Update(dst);
        }
    }
}

void ParallelEvacuator::UpdateWeakObjectSlot(TaggedObject *value, ObjectSlot &slot)
{
    Region *objectRegion = Region::ObjectAddressToRange(value);
    if (objectRegion->InSharedHeap()) {
        return;
    }

    TaggedObject *dst = UpdateAddressAfterEvacation(value);
    if (dst == value) {
        return;
    }
    if (dst == nullptr) {
        slot.Clear();
        return;
    }
    auto weakRef = JSTaggedValue(dst).CreateAndGetWeakRef().GetRawTaggedObject();
    slot.Update(weakRef);
}

template<TriggerGCType gcType>
void ParallelEvacuator::UpdateObjectSlotOpt(ObjectSlot &slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        if (UpdateWeakObjectSlotOpt<gcType>(value, slot)) {
            return;
        }
        MarkWord markWord(value.GetTaggedObject());
        if (markWord.IsForwardingAddress()) {
            auto dst = reinterpret_cast<JSTaggedType>(markWord.ToForwardingAddress());
            slot.Update(dst);
        }
    }
}

template<TriggerGCType gcType>
bool ParallelEvacuator::UpdateWeakObjectSlotOpt(JSTaggedValue value, ObjectSlot &slot)
{
    // if need to update slot as non-weak then return FALSE, else return TRUE
    Region *objectRegion = Region::ObjectAddressToRange(value.GetRawData());
    ASSERT(objectRegion != nullptr);
    if constexpr (gcType == TriggerGCType::YOUNG_GC) {
        if (!objectRegion->InGeneralNewSpace()) {
            return true;
        }
    } else if constexpr (gcType == TriggerGCType::OLD_GC) {
        if (!objectRegion->InGeneralNewSpaceOrCSet()) {
            if (value.IsWeakForHeapObject() && !objectRegion->InSharedHeap() &&
                    (objectRegion->GetMarkGCBitset() == nullptr || !objectRegion->Test(value.GetRawData()))) {
                slot.Clear();
            }
            return true;
        }
    } else {
        LOG_GC(FATAL) << "UpdateWeakObjectSlotOpt: not support gcType yet";
        UNREACHABLE();
    }
    if (objectRegion->InNewToNewSet()) {
        if (value.IsWeakForHeapObject() && !objectRegion->Test(value.GetRawData())) {
            slot.Clear();
        }
        return true;
    }
    if (value.IsWeakForHeapObject()) {
        MarkWord markWord(value.GetWeakReferent());
        if (markWord.IsForwardingAddress()) {
            auto dst = static_cast<JSTaggedType>(ToUintPtr(markWord.ToForwardingAddress()));
            slot.Update(JSTaggedValue(dst).CreateAndGetWeakRef().GetRawData());
        } else {
            slot.Clear();
        }
        return true;
    }
    return false;
}

void ParallelEvacuator::UpdateLocalToShareRSet(TaggedObject *object, JSHClass *cls)
{
    Region *region = Region::ObjectAddressToRange(object);
    ASSERT(!region->InSharedHeap());
    auto callbackWithCSet = [this, region](TaggedObject *root, ObjectSlot start, ObjectSlot end, VisitObjectArea area) {
        if (area == VisitObjectArea::IN_OBJECT) {
            if (VisitBodyInObj(root, start, end, [&](ObjectSlot slot) { SetLocalToShareRSet(slot, region); })) {
                return;
            };
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            SetLocalToShareRSet(slot, region);
        }
    };
    ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, cls, callbackWithCSet);
}

void ParallelEvacuator::SetLocalToShareRSet(ObjectSlot slot, Region *region)
{
    ASSERT(!region->InSharedHeap());
    JSTaggedType value = slot.GetTaggedType();
    if (!JSTaggedValue(value).IsHeapObject()) {
        return;
    }
    Region *valueRegion = Region::ObjectAddressToRange(value);
    if (valueRegion->InSharedSweepableSpace()) {
        region->AtomicInsertLocalToShareRSet(slot.SlotAddress());
    }
}

template<bool SetEdenObject>
void ParallelEvacuator::SetObjectFieldRSet(TaggedObject *object, JSHClass *cls)
{
    Region *region = Region::ObjectAddressToRange(object);
    auto callbackWithCSet = [this, region](TaggedObject *root, ObjectSlot start, ObjectSlot end, VisitObjectArea area) {
        if (area == VisitObjectArea::IN_OBJECT) {
            if (VisitBodyInObj(root, start, end,
                               [&](ObjectSlot slot) { SetObjectRSet<SetEdenObject>(slot, region); })) {
                return;
            };
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            SetObjectRSet<SetEdenObject>(slot, region);
        }
    };
    ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, cls, callbackWithCSet);
}

template<bool SetEdenObject>
void ParallelEvacuator::SetObjectRSet(ObjectSlot slot, Region *region)
{
    JSTaggedType value = slot.GetTaggedType();
    if (!JSTaggedValue(value).IsHeapObject()) {
        return;
    }
    Region *valueRegion = Region::ObjectAddressToRange(value);
    if constexpr (SetEdenObject) {
        if (region->InYoungSpace() && valueRegion->InEdenSpace()) {
            region->AtomicInsertNewToEdenRSet(slot.SlotAddress());
        } else if (valueRegion->InSharedSweepableSpace()) {
            region->AtomicInsertLocalToShareRSet(slot.SlotAddress());
        }
    } else {
        if (valueRegion->InGeneralNewSpace()) {
            region->InsertOldToNewRSet(slot.SlotAddress());
        }  else if (valueRegion->InSharedSweepableSpace()) {
            region->InsertLocalToShareRSet(slot.SlotAddress());
        } else if (valueRegion->InCollectSet()) {
            region->InsertCrossRegionRSet(slot.SlotAddress());
        } else if (JSTaggedValue(value).IsWeakForHeapObject()) {
            if (heap_->IsConcurrentFullMark() && !valueRegion->InSharedHeap() &&
                    (valueRegion->GetMarkGCBitset() == nullptr || !valueRegion->Test(value))) {
                slot.Clear();
            }
        }
    }
}

bool ParallelEvacuator::AcquireItem::TryAcquire()
{
    return acquire_.exchange(true, std::memory_order_relaxed) == false;
}

void ParallelEvacuator::WorkloadSet::Add(std::unique_ptr<Workload> workload)
{
    workloads_.emplace_back(AcquireItem{}, std::move(workload));
}

bool ParallelEvacuator::WorkloadSet::HasRemaningWorkload() const
{
    return remainingWorkloadNum_.load(std::memory_order_relaxed) > 0;
}

bool ParallelEvacuator::WorkloadSet::FetchSubAndCheckWorkloadCount(size_t finishedCount)
{
    return remainingWorkloadNum_.fetch_sub(finishedCount, std::memory_order_relaxed) == finishedCount;
}

TaggedObject* ParallelEvacuator::UpdateAddressAfterEvacation(TaggedObject *oldAddress)
{
    Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(oldAddress));
    if (!objectRegion) {
        return nullptr;
    }
    if (heap_->IsEdenMark()) {
        if (!objectRegion->InEdenSpace()) {
            return oldAddress;
        }
        MarkWord markWord(oldAddress);
        if (markWord.IsForwardingAddress()) {
            return markWord.ToForwardingAddress();
        }
        return nullptr;
    }
    if (objectRegion->InGeneralNewSpaceOrCSet()) {
        if (objectRegion->InNewToNewSet()) {
            if (objectRegion->Test(oldAddress)) {
                return oldAddress;
            }
        } else {
            MarkWord markWord(oldAddress);
            if (markWord.IsForwardingAddress()) {
                return markWord.ToForwardingAddress();
            }
        }
        return nullptr;
    }
    if (heap_->IsConcurrentFullMark()) {
        if (objectRegion->GetMarkGCBitset() == nullptr || !objectRegion->Test(oldAddress)) {
            return nullptr;
        }
    }
    return oldAddress;
}

int ParallelEvacuator::CalculateEvacuationThreadNum()
{
    uint32_t count = evacuateWorkloadSet_.GetWorkloadCount();
    uint32_t regionPerThread = 8;
    uint32_t maxThreadNum = std::min(heap_->GetMaxEvacuateTaskCount(),
        Taskpool::GetCurrentTaskpool()->GetTotalThreadNum());
    return static_cast<int>(std::min(std::max(1U, count / regionPerThread), maxThreadNum));
}

int ParallelEvacuator::CalculateUpdateThreadNum()
{
    uint32_t count = updateWorkloadSet_.GetWorkloadCount();
    double regionPerThread = 1.0 / 4;
    count = std::pow(count, regionPerThread);
    uint32_t maxThreadNum = std::min(heap_->GetMaxEvacuateTaskCount(),
        Taskpool::GetCurrentTaskpool()->GetTotalThreadNum());
    return static_cast<int>(std::min(std::max(1U, count), maxThreadNum));
}

size_t ParallelEvacuator::WorkloadSet::GetWorkloadCount() const
{
    return workloads_.size();
}

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_PARALLEL_EVACUATOR_INL_H
