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
ParallelEvacuator::ParallelEvacuator(Heap *heap) : heap_(heap), updateRootVisitor_(this),
    setObjectFieldRSetVisitor_(this) {}

// Move regions with a survival rate of more than 75% to new space
// Move regions when young space overshoot size is larger than max capacity.
RegionEvacuateType ParallelEvacuator::SelectRegionEvacuateType(Region *region)
{
    double aliveRate = static_cast<double>(region->AliveObject()) / region->GetSize();
    if (UNLIKELY(region->HasAgeMark())) {
        return RegionEvacuateType::OBJECT_EVACUATE;
    } else if (region->BelowAgeMark()) {
        if (aliveRate >= MIN_OBJECT_SURVIVAL_RATE) {
            return RegionEvacuateType::REGION_NEW_TO_OLD;
        }
        return RegionEvacuateType::OBJECT_EVACUATE;
    }
    if (aliveRate >= MIN_OBJECT_SURVIVAL_RATE || heap_->GetFromSpaceDuringEvacuation()->CommittedSizeIsLarge()) {
        return RegionEvacuateType::REGION_NEW_TO_NEW;
    }
    return RegionEvacuateType::OBJECT_EVACUATE;
}

void ParallelEvacuator::CompensateOvershootSizeIfHighAliveRate(Region* region)
{
    double aliveRate = static_cast<double>(region->AliveObject()) / region->GetSize();
    if (region->IsFreshRegion() || aliveRate >= STRICT_OBJECT_SURVIVAL_RATE) {
        size_t compensateSize = static_cast<size_t>(region->GetCapacity() * (1.0 - HPPGC_NEWSPACE_SIZE_RATIO));
        heap_->GetNewSpace()->AddOverShootSize(compensateSize);
    }
}

bool ParallelEvacuator::TryWholeRegionEvacuate(Region *region, RegionEvacuateType type)
{
    switch (type) {
        case RegionEvacuateType::REGION_NEW_TO_NEW:
            CompensateOvershootSizeIfHighAliveRate(region);
            return heap_->MoveYoungRegion(region);
        case RegionEvacuateType::REGION_NEW_TO_OLD:
            return heap_->MoveYoungRegionToOld(region);
        default:
            return false;
    }
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

bool ParallelEvacuator::UpdateOldToNewObjectSlot(ObjectSlot &slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return false;
    }
    TaggedObject *object = value.GetHeapObject();
    Region *valueRegion = Region::ObjectAddressToRange(object);
    // It is only update old to new object when iterate OldToNewRSet
    if (valueRegion->InYoungSpace()) {
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
    } else if (valueRegion->InNewToOldSet()) {
        if (value.IsWeakForHeapObject() && !valueRegion->Test(object)) {
            slot.Clear();
        }
    }
    return false;
}

ParallelEvacuator::UpdateRootVisitor::UpdateRootVisitor(ParallelEvacuator *evacuator) : evacuator_(evacuator) {}

void ParallelEvacuator::UpdateRootVisitor::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    evacuator_->UpdateObjectSlot(slot);
}

void ParallelEvacuator::UpdateRootVisitor::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        evacuator_->UpdateObjectSlot(slot);
    }
}

void ParallelEvacuator::UpdateRootVisitor::VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base,
                                                                   ObjectSlot derived, uintptr_t baseOldObject)
{
    if (JSTaggedValue(base.GetTaggedType()).IsHeapObject()) {
        derived.Update(base.GetTaggedType() + derived.GetTaggedType() - baseOldObject);
    }
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

template<TriggerGCType gcType, bool needUpdateLocalToShare>
void ParallelEvacuator::UpdateNewObjectSlot(ObjectSlot &slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        Region *objectRegion = Region::ObjectAddressToRange(value.GetRawData());
        ASSERT(objectRegion != nullptr);
        if constexpr (needUpdateLocalToShare == true) {
            if (objectRegion->InSharedSweepableSpace()) {
                Region *rootRegion = Region::ObjectAddressToRange(slot.SlotAddress());
                rootRegion->InsertLocalToShareRSet(slot.SlotAddress());
                return;
            }
        }
        if constexpr (gcType == TriggerGCType::YOUNG_GC) {
            if (!objectRegion->InYoungSpace()) {
                if (value.IsWeakForHeapObject() && objectRegion->InNewToOldSet() &&
                    !objectRegion->Test(value.GetRawData())) {
                    slot.Clear();
                }
                return;
            }
        } else if constexpr (gcType == TriggerGCType::OLD_GC) {
            if (!objectRegion->InYoungSpaceOrCSet()) {
                if (value.IsWeakForHeapObject() && !objectRegion->InSharedHeap() &&
                        (objectRegion->GetMarkGCBitset() == nullptr || !objectRegion->Test(value.GetRawData()))) {
                    slot.Clear();
                }
                return;
            }
        } else {
            LOG_GC(FATAL) << "UpdateNewObjectSlot: not support gcType yet";
            UNREACHABLE();
        }
        if (objectRegion->InNewToNewSet()) {
            if (value.IsWeakForHeapObject() && !objectRegion->Test(value.GetRawData())) {
                slot.Clear();
            }
            return;
        }
        UpdateObjectSlotValue(value, slot);
    }
}

void ParallelEvacuator::UpdateCrossRegionObjectSlot(ObjectSlot &slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        Region *objectRegion = Region::ObjectAddressToRange(value.GetRawData());
        ASSERT(objectRegion != nullptr);
        if (objectRegion->InCollectSet()) {
            UpdateObjectSlotValue(value, slot);
        }
    }
}

void ParallelEvacuator::UpdateObjectSlotValue(JSTaggedValue value, ObjectSlot &slot)
{
    if (value.IsWeakForHeapObject()) {
        MarkWord markWord(value.GetWeakReferent());
        if (markWord.IsForwardingAddress()) {
            auto dst = static_cast<JSTaggedType>(ToUintPtr(markWord.ToForwardingAddress()));
            slot.Update(JSTaggedValue(dst).CreateAndGetWeakRef().GetRawData());
        } else {
            slot.Clear();
        }
    } else {
        MarkWord markWord(value.GetTaggedObject());
        if (markWord.IsForwardingAddress()) {
            auto dst = reinterpret_cast<JSTaggedType>(markWord.ToForwardingAddress());
            slot.Update(dst);
        }
    }
}

ParallelEvacuator::SetObjectFieldRSetVisitor::SetObjectFieldRSetVisitor(ParallelEvacuator *evacuator)
    : evacuator_(evacuator) {}

void ParallelEvacuator::SetObjectFieldRSetVisitor::VisitObjectRangeImpl(BaseObject *root, uintptr_t startAddr,
    uintptr_t endAddr, VisitObjectArea area)
{
    Region *rootRegion = Region::ObjectAddressToRange(root);
    ObjectSlot start(startAddr);
    ObjectSlot end(endAddr);
    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        JSHClass *hclass = TaggedObject::Cast(root)->GetClass();
        ASSERT(!hclass->IsAllTaggedProp());
        int index = 0;
        TaggedObject *dst = hclass->GetLayout().GetTaggedObject();
        LayoutInfo *layout = LayoutInfo::UncheckCast(dst);
        ObjectSlot realEnd = start;
        realEnd += layout->GetPropertiesCapacity();
        end = end > realEnd ? realEnd : end;
        for (ObjectSlot slot = start; slot < end; slot++) {
            auto attr = layout->GetAttr(index++);
            if (attr.IsTaggedRep()) {
                evacuator_->SetObjectRSet(slot, rootRegion);
            }
        }
        return;
    }
    for (ObjectSlot slot = start; slot < end; slot++) {
        evacuator_->SetObjectRSet(slot, rootRegion);
    };
}

void ParallelEvacuator::SetObjectFieldRSet(TaggedObject *object, JSHClass *cls)
{
    ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, cls, setObjectFieldRSetVisitor_);
}

void ParallelEvacuator::SetObjectRSet(ObjectSlot slot, Region *region)
{
    JSTaggedType value = slot.GetTaggedType();
    if (!JSTaggedValue(value).IsHeapObject()) {
        return;
    }
    Region *valueRegion = Region::ObjectAddressToRange(value);
    if (valueRegion->InYoungSpace()) {
        region->InsertOldToNewRSet(slot.SlotAddress());
    } else if (valueRegion->InNewToOldSet()) {
        if (JSTaggedValue(value).IsWeakForHeapObject() && !valueRegion->Test(value)) {
            slot.Clear();
        }
    } else if (valueRegion->InSharedSweepableSpace()) {
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
    if (objectRegion->InYoungSpaceOrCSet()) {
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
    } else if (objectRegion->InNewToOldSet() && !objectRegion->Test(oldAddress)) {
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
