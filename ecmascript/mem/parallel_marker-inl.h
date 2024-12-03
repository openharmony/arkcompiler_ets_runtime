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

template <typename Callback>
ARK_INLINE bool NonMovableMarker::VisitBodyInObj(TaggedObject *root, ObjectSlot start, ObjectSlot end,
                                                 bool needBarrier, Callback callback)
{
    auto hclass = root->SynchronizedGetClass();
    Region *rootRegion = Region::ObjectAddressToRange(root);
    int index = 0;
    auto layout = LayoutInfo::UncheckCast(hclass->GetLayout().GetTaggedObject());
    ObjectSlot realEnd = start;
    realEnd += layout->GetPropertiesCapacity();
    end = end > realEnd ? realEnd : end;
    for (ObjectSlot slot = start; slot < end; slot++) {
        auto attr = layout->GetAttr(index++);
        if (attr.IsTaggedRep()) {
            callback(slot, rootRegion, needBarrier);
        }
    }
    return true;
}

inline void NonMovableMarker::MarkValue(uint32_t threadId, ObjectSlot &slot, Region *rootRegion, bool needBarrier)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        ASSERT(!value.IsHole()); // check that value is not zero
        TaggedObject *obj = nullptr;
        if (!value.IsWeakForHeapObject()) {
            obj = value.GetTaggedObject();
            Region *objRegion = Region::ObjectAddressToRange(obj);
            if (objRegion->IsFreshRegion()) {
                // Object in fresh region should only mark from JS Thread in barrier, or MarkObject in MarkRoots.
                ASSERT(objRegion->InYoungSpace());
                return;
            }
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

    if (objectRegion->InSharedHeap()) {
        return;
    }

    if (heap_->IsYoungMark() && objectRegion->InGeneralOldSpace()) {
        return;
    }
    
    if (heap_->IsEdenMark() && !objectRegion->InEdenSpace()) {
        return;
    }

    if (objectRegion->IsFreshRegion()) {
        // This should only happen in MarkRoot from js thread.
        ASSERT(JSThread::GetCurrent() != nullptr);
        ASSERT(objectRegion->InYoungSpace());
        objectRegion->NonAtomicMark(object);
    } else if (objectRegion->AtomicMark(object)) {
        workManager_->Push(threadId, object);
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

inline void NonMovableMarker::HandleNewToEdenRSet(uint32_t threadId, Region *region)
{
    ASSERT(region->InYoungSpace());
    region->IterateAllNewToEdenBits([this, threadId, region](void *mem) -> bool {
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

inline void NonMovableMarker::HandleOldToNewRSet(uint32_t threadId, Region *region)
{
    bool isEdenMark = heap_->IsEdenMark();
    region->IterateAllOldToNewBits([this, threadId, &region, isEdenMark](void *mem) -> bool {
        ObjectSlot slot(ToUintPtr(mem));
        JSTaggedValue value(slot.GetTaggedType());
        if (!value.IsHeapObject()) {
            return true;
        }
        if (value.IsWeakForHeapObject()) {
            RecordWeakReference(threadId, reinterpret_cast<JSTaggedType *>(mem), region);
        } else {
            auto object = value.GetTaggedObject();
            Region *objectRegion = Region::ObjectAddressToRange(object);
            if (isEdenMark) {
                if (objectRegion->InEdenSpace()) {
                    MarkObject(threadId, value.GetTaggedObject());
                }
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
    if (heap_->IsEdenMark()) {
        // only record object may be sweep, but no object will be sweep in EdenGC
        return;
    }
    if (!objectRegion->InGeneralNewSpaceOrCSet() && !valueRegion->InGeneralNewSpaceOrCSet()) {
        workManager_->PushWeakReference(threadId, ref);
    }
}

template <typename Callback>
ARK_INLINE bool MovableMarker::VisitBodyInObj(TaggedObject *root, ObjectSlot start, ObjectSlot end, Callback callback)
{
    auto hclass = root->GetClass();
    int index = 0;
    TaggedObject *dst = hclass->GetLayout().GetTaggedObject();
    auto layout = LayoutInfo::UncheckCast(dst);
    ObjectSlot realEnd = start;
    realEnd += layout->GetPropertiesCapacity();
    end = end > realEnd ? realEnd : end;
    for (ObjectSlot slot = start; slot < end; slot++) {
        auto attr = layout->GetAttr(index++);
        if (attr.IsTaggedRep()) {
            callback(slot, root);
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

inline void MovableMarker::HandleNewToEdenRSet(uint32_t threadId, Region *region)
{
    region->IterateAllNewToEdenBits([this, threadId, &region](void *mem) -> bool {
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
        ASSERT(forwardAddress != 0);
    } else {
        forwardAddress = workManager_->GetTlabAllocator(threadId)->Allocate(size, SEMI_SPACE);
        if (UNLIKELY(forwardAddress == 0)) {
            forwardAddress = workManager_->GetTlabAllocator(threadId)->Allocate(size, COMPRESS_SPACE);
            ASSERT(forwardAddress != 0);
            shouldPromote = true;
        }
    }
    return forwardAddress;
}

inline void MovableMarker::UpdateForwardAddressIfSuccess(uint32_t threadId, TaggedObject *object, JSHClass *klass,
    uintptr_t toAddress, size_t size, ObjectSlot slot, bool isPromoted)
{
    workManager_->IncreaseAliveSize(threadId, size);
    if (isPromoted) {
        workManager_->IncreasePromotedSize(threadId, size);
    }

    heap_->OnMoveEvent(reinterpret_cast<intptr_t>(object), reinterpret_cast<TaggedObject *>(toAddress), size);
    if (klass->HasReferenceField()) {
        workManager_->Push(threadId, reinterpret_cast<TaggedObject *>(toAddress));
    }
    slot.Update(reinterpret_cast<TaggedObject *>(toAddress));
}

inline bool MovableMarker::UpdateForwardAddressIfFailed(TaggedObject *object, uintptr_t toAddress, size_t size,
    ObjectSlot slot)
{
    FreeObject::FillFreeObject(heap_, toAddress, size);
    TaggedObject *dst = MarkWord(object).ToForwardingAddress();
    slot.Update(dst);
    return Region::ObjectAddressToRange(dst)->InYoungSpace();
}

inline void MovableMarker::RawCopyObject(uintptr_t fromAddress, uintptr_t toAddress, size_t size,
    const MarkWord &markWord)
{
    if (memcpy_s(ToVoidPtr(toAddress + HEAD_SIZE), size - HEAD_SIZE, ToVoidPtr(fromAddress + HEAD_SIZE),
        size - HEAD_SIZE) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
    }
    *reinterpret_cast<MarkWordType *>(toAddress) = markWord.GetValue();
}

void MovableMarker::UpdateLocalToShareRSet(TaggedObject *object, JSHClass *cls)
{
    Region *region = Region::ObjectAddressToRange(object);
    ASSERT(!region->InSharedHeap());
    auto callbackWithCSet = [this, region](TaggedObject *root, ObjectSlot start, ObjectSlot end, VisitObjectArea area) {
        if (area == VisitObjectArea::IN_OBJECT) {
            if (VisitBodyInObj(root, start, end,
                               [&](ObjectSlot slot, [[maybe_unused]]TaggedObject *root) {
                                   SetLocalToShareRSet(slot, region);
                               })) {
                return;
            };
        }
        for (ObjectSlot slot = start; slot < end; slot++) {
            SetLocalToShareRSet(slot, region);
        }
    };
    ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(object, cls, callbackWithCSet);
}

void MovableMarker::SetLocalToShareRSet(ObjectSlot slot, Region *region)
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
        if (rootRegion->InGeneralOldSpace() && slotStatus == SlotStatus::KEEP_SLOT) {
            SlotNeedUpdate waitUpdate(reinterpret_cast<TaggedObject *>(root), slot);
            workManager_->PushSlotNeedUpdate(threadId, waitUpdate);
        }
    }
}

inline SlotStatus SemiGCMarker::MarkObject(uint32_t threadId, TaggedObject *object, ObjectSlot slot)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (objectRegion->InGeneralOldSpace()) {
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
    RawCopyObject(ToUintPtr(object), forwardAddress, size, markWord);

    auto oldValue = markWord.GetValue();
    auto result = Barriers::AtomicSetPrimitive(object, 0, oldValue,
                                               MarkWord::FromForwardingAddress(forwardAddress));
    if (result == oldValue) {
        UpdateForwardAddressIfSuccess(threadId, object, klass, forwardAddress, size, slot, isPromoted);
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
        if (!objectRegion->InSharedHeap() && objectRegion->AtomicMark(object)) {
            workManager_->Push(threadId, object);
            auto hclass = object->GetClass();
            auto size = hclass->SizeFromJSHClass(object);
            objectRegion->IncreaseAliveObject(size);
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
    LockHolder lock(mutex_);
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
    LockHolder lock(mutex_);
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
    RawCopyObject(ToUintPtr(object), forwardAddress, size, markWord);

    auto oldValue = markWord.GetValue();
    auto result = Barriers::AtomicSetPrimitive(object, 0, oldValue,
                                               MarkWord::FromForwardingAddress(forwardAddress));
    if (result == oldValue) {
        UpdateForwardAddressIfSuccess(threadId, object, klass, forwardAddress, size, slot);
        Region *region = Region::ObjectAddressToRange(object);
        if (region->HasLocalToShareRememberedSet()) {
            UpdateLocalToShareRSet(reinterpret_cast<TaggedObject *>(forwardAddress), klass);
        }
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
        return !region->InHugeObjectSpace()  && !region->InReadOnlySpace() && !region->InNonMovableSpace() &&
               !region->InSharedHeap();
    }
    return region->InYoungOrOldSpace();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_PARALLEL_MARKER_INL_H
