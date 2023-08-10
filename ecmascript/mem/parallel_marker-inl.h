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

#ifndef ECMASCRIPT_MEM_PARALLEL_MARKER_INL_H
#define ECMASCRIPT_MEM_PARALLEL_MARKER_INL_H

#include "ecmascript/mem/parallel_marker.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/gc_bitset.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/tlab_allocator-inl.h"

namespace panda::ecmascript {
constexpr size_t HEAD_SIZE = TaggedObject::TaggedObjectSize();

template <typename Callback>
inline bool NonMovableMarker::VisitBodyInObj(TaggedObject *root, ObjectSlot start, ObjectSlot end, Callback callback)
{
    auto hclass = root->SynchronizedGetClass();
    if (hclass->IsAllTaggedProp()) {
        return false;
    }
    int index = 0;
    for (ObjectSlot slot = start; slot < end; slot++) {
        auto layout = LayoutInfo::Cast(hclass->GetLayout().GetTaggedObject());
        auto attr = layout->GetAttr(index++);
        if (attr.IsTaggedRep()) {
            callback(slot);
        }
    }
    return true;
}

inline void NonMovableMarker::MarkValue(uint32_t threadId, ObjectSlot &slot, Region *rootRegion, bool needBarrier)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        TaggedObject *obj = nullptr;
        if (!value.IsWeakForHeapObject()) {
            obj = value.GetTaggedObject();
            MarkObject(threadId, obj);
        } else {
            RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(slot.SlotAddress()), rootRegion);
            obj = value.GetWeakReferentUnChecked();
        }
        if (needBarrier) {
            Region *valueRegion = Region::ObjectAddressToRange(obj);
            if (valueRegion->InCollectSet()) {
                rootRegion->AtomicInsertCrossRegionRSet(slot.SlotAddress());
            }
        }
    }
}

inline void NonMovableMarker::MarkObject(uint32_t threadId, TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);

    if (!heap_->IsFullMark() && !objectRegion->InYoungSpace()) {
        return;
    }

    if (objectRegion->AtomicMark(object)) {
        workManager_->Push(threadId, object, objectRegion);
    }
}

inline void NonMovableMarker::HandleRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        MarkObject(threadId, value.GetTaggedObject());
    }
}

inline void NonMovableMarker::HandleRangeRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot start,
    ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            if (value.IsWeakForHeapObject()) {
                LOG_ECMA_MEM(FATAL) << "Weak Reference in NonMovableMarker roots";
            }
            MarkObject(threadId, value.GetTaggedObject());
        }
    }
}

inline void NonMovableMarker::HandleDerivedRoots([[maybe_unused]] Root type, [[maybe_unused]] ObjectSlot base,
                                                 [[maybe_unused]] ObjectSlot derived,
                                                 [[maybe_unused]] uintptr_t baseOldObject)
{
    // It is only used to update the derived value. The mark of partial GC does not need to update slot
}

inline void NonMovableMarker::HandleOldToNewRSet(uint32_t threadId, Region *region)
{
    region->IterateAllOldToNewBits([this, threadId, &region](void *mem) -> bool {
        ObjectSlot slot(ToUintPtr(mem));
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            if (value.IsWeakForHeapObject()) {
                RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(mem), region);
            } else {
                MarkObject(threadId, value.GetTaggedObject());
            }
        }
        return true;
    });
}

inline void NonMovableMarker::RecordWeakReference(uint32_t threadId, JSTaggedType *ref, Region *objectRegion)
{
    auto value = JSTaggedValue(*ref);
    Region *valueRegion = Region::ObjectAddressToRange(value.GetTaggedWeakRef());
    if (!objectRegion->InYoungSpaceOrCSet() && !valueRegion->InYoungSpaceOrCSet()) {
        workManager_->PushWeakReference(threadId, ref);
    }
}

template <typename Callback>
inline bool MovableMarker::VisitBodyInObj(TaggedObject *root, ObjectSlot start, ObjectSlot end, Callback callback)
{
    auto hclass = root->GetClass();
    if (hclass->IsAllTaggedProp()) {
        return false;
    }
    int index = 0;
    for (ObjectSlot slot = start; slot < end; slot++) {
        TaggedObject *dst = hclass->GetLayout().GetTaggedObject();
        MarkWord markWord(dst);
        if (markWord.IsForwardingAddress()) {
            dst = markWord.ToForwardingAddress();
        }
        auto layout = LayoutInfo::Cast(dst);
        auto attr = layout->GetAttr(index++);
        if (attr.IsTaggedRep()) {
            callback(slot);
        }
    }
    return true;
}

inline void MovableMarker::HandleRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        MarkObject(threadId, value.GetTaggedObject(), slot);
    }
}

inline void MovableMarker::HandleRangeRoots(uint32_t threadId, [[maybe_unused]] Root type, ObjectSlot start,
    ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            if (value.IsWeakForHeapObject()) {
                Region *objectRegion = Region::ObjectAddressToRange(start.SlotAddress());
                RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(slot.SlotAddress()), objectRegion);
            } else {
                MarkObject(threadId, value.GetTaggedObject(), slot);
            }
        }
    }
}

inline void MovableMarker::HandleDerivedRoots([[maybe_unused]] Root type, ObjectSlot base,
                                              ObjectSlot derived, uintptr_t baseOldObject)
{
    if (JSTaggedValue(base.GetTaggedType()).IsHeapObject()) {
        derived.Update(base.GetTaggedType() + derived.GetTaggedType() - baseOldObject);
    }
}

inline void MovableMarker::HandleOldToNewRSet(uint32_t threadId, Region *region)
{
    region->IterateAllOldToNewBits([this, threadId, &region](void *mem) -> bool {
        ObjectSlot slot(ToUintPtr(mem));
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            if (value.IsWeakForHeapObject()) {
                RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(mem), region);
                return true;
            }
            auto slotStatus = MarkObject(threadId, value.GetTaggedObject(), slot);
            if (slotStatus == SlotStatus::CLEAR_SLOT) {
                return false;
            }
        }
        return true;
    });
}

inline uintptr_t MovableMarker::AllocateDstSpace(uint32_t threadId, size_t size, bool &shouldPromote)
{
    uintptr_t forwardAddress = 0;
    if (shouldPromote) {
        forwardAddress = workManager_->GetTlabAllocator(threadId)->Allocate(size, COMPRESS_SPACE);
        if (UNLIKELY(forwardAddress == 0)) {
            LOG_ECMA_MEM(FATAL) << "EvacuateObject alloc failed: "
                                << " size: " << size;
            UNREACHABLE();
        }
    } else {
        forwardAddress = workManager_->GetTlabAllocator(threadId)->Allocate(size, SEMI_SPACE);
        if (UNLIKELY(forwardAddress == 0)) {
            forwardAddress = workManager_->GetTlabAllocator(threadId)->Allocate(size, COMPRESS_SPACE);
            if (UNLIKELY(forwardAddress == 0)) {
                LOG_ECMA_MEM(FATAL) << "EvacuateObject alloc failed: "
                                    << " size: " << size;
                UNREACHABLE();
            }
            shouldPromote = true;
        }
    }
    return forwardAddress;
}

inline void MovableMarker::UpdateForwardAddressIfSuccess(uint32_t threadId, TaggedObject *object, JSHClass *klass,
    uintptr_t toAddress, size_t size, const MarkWord &markWord, ObjectSlot slot, bool isPromoted)
{
    if (memcpy_s(ToVoidPtr(toAddress + HEAD_SIZE), size - HEAD_SIZE, ToVoidPtr(ToUintPtr(object) + HEAD_SIZE),
        size - HEAD_SIZE) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
    }
    workManager_->IncreaseAliveSize(threadId, size);
    if (isPromoted) {
        workManager_->IncreasePromotedSize(threadId, size);
    }

    *reinterpret_cast<MarkWordType *>(toAddress) = markWord.GetValue();
    heap_->OnMoveEvent(reinterpret_cast<intptr_t>(object), reinterpret_cast<TaggedObject *>(toAddress), size);
    if (klass->HasReferenceField()) {
        workManager_->Push(threadId, reinterpret_cast<TaggedObject *>(toAddress));
    }
    slot.Update(reinterpret_cast<TaggedObject *>(toAddress));
}

inline bool MovableMarker::UpdateForwardAddressIfFailed(TaggedObject *object, uintptr_t toAddress, size_t size,
    ObjectSlot slot)
{
    FreeObject::FillFreeObject(heap_->GetEcmaVM(), toAddress, size);
    TaggedObject *dst = MarkWord(object).ToForwardingAddress();
    slot.Update(dst);
    return Region::ObjectAddressToRange(dst)->InYoungSpace();
}

inline void SemiGCMarker::MarkValue(uint32_t threadId, TaggedObject *root, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        Region *rootRegion = Region::ObjectAddressToRange(root);
        if (value.IsWeakForHeapObject()) {
            RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(slot.SlotAddress()), rootRegion);
            return;
        }
        auto slotStatus = MarkObject(threadId, value.GetTaggedObject(), slot);
        if (!rootRegion->InYoungSpace() && slotStatus == SlotStatus::KEEP_SLOT) {
            SlotNeedUpdate waitUpdate(reinterpret_cast<TaggedObject *>(root), slot);
            workManager_->PushSlotNeedUpdate(threadId, waitUpdate);
        }
    }
}

inline SlotStatus SemiGCMarker::MarkObject(uint32_t threadId, TaggedObject *object, ObjectSlot slot)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (!objectRegion->InYoungSpace()) {
        return SlotStatus::CLEAR_SLOT;
    }

    MarkWord markWord(object);
    if (markWord.IsForwardingAddress()) {
        TaggedObject *dst = markWord.ToForwardingAddress();
        slot.Update(dst);
        Region *valueRegion = Region::ObjectAddressToRange(dst);
        return valueRegion->InYoungSpace() ? SlotStatus::KEEP_SLOT : SlotStatus::CLEAR_SLOT;
    }
    return EvacuateObject(threadId, object, markWord, slot);
}

inline SlotStatus SemiGCMarker::EvacuateObject(uint32_t threadId, TaggedObject *object, const MarkWord &markWord,
    ObjectSlot slot)
{
    JSHClass *klass = markWord.GetJSHClass();
    size_t size = klass->SizeFromJSHClass(object);
    bool isPromoted = ShouldBePromoted(object);

    uintptr_t forwardAddress = AllocateDstSpace(threadId, size, isPromoted);
    bool result = Barriers::AtomicSetPrimitive(object, 0, markWord.GetValue(),
                                               MarkWord::FromForwardingAddress(forwardAddress));
    if (result) {
        UpdateForwardAddressIfSuccess(threadId, object, klass, forwardAddress, size, markWord, slot, isPromoted);
        return isPromoted ? SlotStatus::CLEAR_SLOT : SlotStatus::KEEP_SLOT;
    }
    bool keepSlot = UpdateForwardAddressIfFailed(object, forwardAddress, size, slot);
    return keepSlot ? SlotStatus::KEEP_SLOT : SlotStatus::CLEAR_SLOT;
}

inline bool SemiGCMarker::ShouldBePromoted(TaggedObject *object)
{
    Region *region = Region::ObjectAddressToRange(object);
    return (region->BelowAgeMark() || (region->HasAgeMark() && ToUintPtr(object) < waterLine_));
}

inline void SemiGCMarker::RecordWeakReference(uint32_t threadId, JSTaggedType *ref,
                                              [[maybe_unused]] Region *objectRegion)
{
    auto value = JSTaggedValue(*ref);
    Region *valueRegion = Region::ObjectAddressToRange(value.GetTaggedWeakRef());
    if (valueRegion->InYoungSpace()) {
        workManager_->PushWeakReference(threadId, ref);
    }
}

inline void CompressGCMarker::MarkValue(uint32_t threadId, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        if (value.IsWeakForHeapObject()) {
            // It is unnecessary to use region pointer in compressGCMarker.
            RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(slot.SlotAddress()));
            return;
        }
        MarkObject(threadId, value.GetTaggedObject(), slot);
    }
}

inline SlotStatus CompressGCMarker::MarkObject(uint32_t threadId, TaggedObject *object, ObjectSlot slot)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (!NeedEvacuate(objectRegion)) {
        if (objectRegion->AtomicMark(object)) {
            workManager_->Push(threadId, object);
        }
        return SlotStatus::CLEAR_SLOT;
    }

    MarkWord markWord(object);
    if (markWord.IsForwardingAddress()) {
        TaggedObject *dst = markWord.ToForwardingAddress();
        slot.Update(dst);
        return SlotStatus::CLEAR_SLOT;
    }
    return EvacuateObject(threadId, object, markWord, slot);
}

inline uintptr_t CompressGCMarker::AllocateReadOnlySpace(size_t size)
{
    os::memory::LockHolder lock(mutex_);
    uintptr_t forwardAddress = heap_->GetReadOnlySpace()->Allocate(size);
    if (UNLIKELY(forwardAddress == 0)) {
        LOG_ECMA_MEM(FATAL) << "Evacuate Read only Object: alloc failed: "
                            << " size: " << size;
        UNREACHABLE();
    }
    return forwardAddress;
}

inline uintptr_t CompressGCMarker::AllocateAppSpawnSpace(size_t size)
{
    os::memory::LockHolder lock(mutex_);
    uintptr_t forwardAddress = heap_->GetAppSpawnSpace()->Allocate(size);
    if (UNLIKELY(forwardAddress == 0)) {
        LOG_ECMA_MEM(FATAL) << "Evacuate AppSpawn Object: alloc failed: "
                            << " size: " << size;
        UNREACHABLE();
    }
    return forwardAddress;
}

inline SlotStatus CompressGCMarker::EvacuateObject(uint32_t threadId, TaggedObject *object, const MarkWord &markWord,
    ObjectSlot slot)
{
    JSHClass *klass = markWord.GetJSHClass();
    size_t size = klass->SizeFromJSHClass(object);
    uintptr_t forwardAddress = AllocateForwardAddress(threadId, size, klass, object);
    bool result = Barriers::AtomicSetPrimitive(object, 0, markWord.GetValue(),
                                               MarkWord::FromForwardingAddress(forwardAddress));
    if (result) {
        UpdateForwardAddressIfSuccess(threadId, object, klass, forwardAddress, size, markWord, slot);
        if (isAppSpawn_ && klass->IsString()) {
            // calculate and set hashcode for read-only ecmastring in advance
            EcmaStringAccessor(reinterpret_cast<TaggedObject *>(forwardAddress)).GetHashcode();
        }
        return SlotStatus::CLEAR_SLOT;
    }
    UpdateForwardAddressIfFailed(object, forwardAddress, size, slot);
    return SlotStatus::CLEAR_SLOT;
}

inline void CompressGCMarker::RecordWeakReference(uint32_t threadId, JSTaggedType *ref,
                                                  [[maybe_unused]] Region *objectRegion)
{
    workManager_->PushWeakReference(threadId, ref);
}

inline bool CompressGCMarker::NeedEvacuate(Region *region)
{
    if (isAppSpawn_) {
        return !region->InHugeObjectSpace()  && !region->InReadOnlySpace() && !region->InNonMovableSpace();
    }
    return region->InYoungOrOldSpace();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_PARALLEL_MARKER_INL_H
