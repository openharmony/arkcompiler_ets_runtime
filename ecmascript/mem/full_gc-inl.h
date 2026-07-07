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

#ifndef ECMASCRIPT_MEM_FULL_GC_INL_H
#define ECMASCRIPT_MEM_FULL_GC_INL_H

#include "ecmascript/mem/full_gc.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/linked_hash_table.h"
#include "ecmascript/mem/object_xray.h"
#include "ecmascript/mem/heap-inl.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/work_manager-inl.h"

namespace panda::ecmascript {

template <bool evacuateNonMovableSpace>
FullGCRunner<evacuateNonMovableSpace>::FullGCRunner(Heap *heap, WorkNodeHolder *workNodeHolder, bool isAppSpawn)
    : heap_(heap), workNodeHolder_(workNodeHolder), isAppSpawn_(isAppSpawn), markRootVisitor_(this),
      markObjectVisitor_(this), updateLocalToShareRSetVisitor_(this) {}

template <bool evacuateNonMovableSpace>
FullGCMarkRootVisitor<evacuateNonMovableSpace> &FullGCRunner<evacuateNonMovableSpace>::GetMarkRootVisitor()
{
    return markRootVisitor_;
}

template <bool evacuateNonMovableSpace>
FullGCMarkObjectVisitor<evacuateNonMovableSpace> &FullGCRunner<evacuateNonMovableSpace>::GetMarkObjectVisitor()
{
    return markObjectVisitor_;
}

template <bool evacuateNonMovableSpace>
void FullGCRunner<evacuateNonMovableSpace>::HandleMarkingSlot(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsHeapObject()) {
        return;
    }

    if (!value.IsWeakForHeapObject()) {
        TaggedObject *object = value.GetTaggedObject();
        HandleMarkingSlotObject(slot, object);
    } else {
        RecordWeakReference(reinterpret_cast<JSTaggedType *>(slot.SlotAddress()));
    }
}

template <bool evacuateNonMovableSpace>
template <class Callback>
void FullGCRunner<evacuateNonMovableSpace>::VisitBodyInObj(BaseObject *root, uintptr_t start, uintptr_t endAddr,
                                                           Callback &&cb)
{
    JSThread *thread = heap_->GetJSThread();
    JSHClass *hclass = TaggedObject::Cast(root)->SynchronizedGetClass();
    ASSERT(!hclass->IsAllTaggedProp());
    int index = 0;
    LayoutInfo *layout = LayoutInfo::UncheckCast(hclass->GetLayout(thread).GetTaggedObject());
    ObjectSlot realEnd(start);
    realEnd += layout->GetPropertiesCapacity();
    ObjectSlot end(endAddr);
    end = end > realEnd ? realEnd : end;
    for (ObjectSlot slot(start); slot < end; slot++) {
        PropertyAttributes attr = layout->GetAttr(thread, index++);
        if (attr.IsTaggedRep()) {
            cb(slot);
        }
    }
}

template <bool evacuateNonMovableSpace>
void FullGCRunner<evacuateNonMovableSpace>::MarkJitCodeVec(JitCodeVector *vec)
{
    for (auto &jitCodeMap : *vec) {
        auto &jitCode = std::get<0>(jitCodeMap);
        auto obj = static_cast<TaggedObject *>(jitCode);
        // jitcode is MachineCode, and MachineCode is in the MachineCode space, will not be evacute.
        HandleMarkingSlotObject(ObjectSlot(reinterpret_cast<uintptr_t>(&jitCode)), obj);
    }
}

template <bool evacuateNonMovableSpace>
void FullGCRunner<evacuateNonMovableSpace>::HandleMarkingSlotObject(ObjectSlot slot, TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (objectRegion->InSharedHeap()) {
        return;
    }
    if (!NeedEvacuate(objectRegion)) {
        if (objectRegion->AtomicMark(object)) {
            // fixme: refactor?
            if constexpr (!G_USE_CMS_GC) {
                size_t size = object->GetSize();
                objectRegion->IncreaseAliveObject(size);
            } else if constexpr (G_USE_STICKY_CMS_GC) {
                object->SetObjectState(ObjectState::OLD);
            }
            PushObject(object);
        }
        return;
    }

    MarkWord markWord(object, ACQUIRE_LOAD);
    if (markWord.IsForwardingAddress()) {
        TaggedObject *dst = markWord.ToForwardingAddress();
        slot.Update(dst);
        return;
    }
    return EvacuateObject(slot, object, markWord);
}

template <bool evacuateNonMovableSpace>
bool FullGCRunner<evacuateNonMovableSpace>::HandleWeakAggregate(WeakAggregate weakAggregate)
{
    ObjectSlot keySlot = weakAggregate.keySlot;
    ObjectSlot valueSlot = weakAggregate.valueSlot;

    ASSERT(keySlot.GetTaggedValue().IsWeak());
    ASSERT(valueSlot.GetTaggedValue().IsHeapObject());
    TaggedObject *key = keySlot.GetTaggedValue().GetTaggedWeakRef();
    TaggedObject *value = valueSlot.GetTaggedValue().GetHeapObject();
    if (IsAlive(key) || IsAlive(value)) {
        HandleMarkingSlot(valueSlot);
        return true;
    }

    return false;
}

template <bool evacuateNonMovableSpace>
bool FullGCRunner<evacuateNonMovableSpace>::NeedEvacuate(Region *region)
{
    ASSERT(!region->InSharedHeap());
    if constexpr (evacuateNonMovableSpace) {
        static_assert(!G_USE_CMS_GC);
        ASSERT(!isAppSpawn_);
        return region->InYoungOrOldSpace() || region->InNonMovableSpace();
    }
    if (isAppSpawn_) {
        return !region->InHugeObjectSpace()  && !region->InReadOnlySpace() && !region->InNonMovableSpace();
    }
    // fixme: refactor?
    if constexpr (G_USE_CMS_GC) {
        static_assert(!evacuateNonMovableSpace);
        return region->InSlotSpace();
    }
    return region->InYoungOrOldSpace();
}

template <bool evacuateNonMovableSpace>
bool FullGCRunner<evacuateNonMovableSpace>::IsAlive(TaggedObject *object)
{
    Region *region = Region::ObjectAddressToRange(object);
    if (region->InSharedHeap()) {
        return true;
    }
    if (NeedEvacuate(region)) {
        MarkWord markWord(object, RELAXED_LOAD);
        return markWord.IsForwardingAddress();
    }
    return region->Test(ToUintPtr(object));
}

template <bool evacuateNonMovableSpace>
void FullGCRunner<evacuateNonMovableSpace>::EvacuateObject(ObjectSlot slot, TaggedObject *object,
                                                           const MarkWord &markWord)
{
    JSHClass *klass = markWord.GetJSHClass();
    size_t size = klass->SizeFromJSHClass(object);
    Region *objectRegion = Region::ObjectAddressToRange(object);
    // NonMovable objects must be evacuated into NonMovableSpace (e.g. HClasses), never into
    // CompressSpace/AppSpawnSpace.
    MemSpaceType space = objectRegion->InNonMovableSpace() ? NON_MOVABLE : COMPRESS_SPACE;
    uintptr_t forwardAddress = AllocateForwardAddress(size, space);
    RawCopyObject(ToUintPtr(object), forwardAddress, size, markWord);

    MarkWordType oldValue = markWord.GetValue();
    MarkWordType result = Barriers::AtomicSetPrimitive(object, 0, oldValue,
                                                       MarkWord::FromForwardingAddress(forwardAddress));
    if (result == oldValue) {
        TaggedObject *toObject = reinterpret_cast<TaggedObject*>(forwardAddress);
        UpdateForwardAddressIfSuccess(slot, object, klass, size, toObject);
        if (objectRegion->HasLocalToShareRememberedSet()) {
            ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(toObject, klass, updateLocalToShareRSetVisitor_);
        }
#if USE_STICKY_CMS_GC
        // Full GC should set mark bit and object state to make sticky CMS-GC correct.
        auto toRegion = Region::ObjectAddressToRange(toObject);
        toRegion->AtomicMark(toObject);
        toObject->SetObjectState(ObjectState::OLD);
        toRegion->IncreaseGCAliveSize(size);
#endif
        return;
    }
    ASSERT(MarkWord::IsForwardingAddress(result));
    UpdateForwardAddressIfFailed(slot, size, forwardAddress, MarkWord::ToForwardingAddress(result));
}

template <bool evacuateNonMovableSpace>
uintptr_t FullGCRunner<evacuateNonMovableSpace>::AllocateForwardAddress(size_t size, MemSpaceType space)
{
    if constexpr (evacuateNonMovableSpace) {
        ASSERT(!isAppSpawn_);
        return AllocateDstSpace(size, space);
    } else {
        if (!isAppSpawn_) {
            return AllocateDstSpace(size, space);
        }
        return AllocateAppSpawnSpace(size);
    }
}

template <bool evacuateNonMovableSpace>
uintptr_t FullGCRunner<evacuateNonMovableSpace>::AllocateDstSpace(size_t size, MemSpaceType space)
{
    uintptr_t forwardAddress;
    // fixme: refactor?
    if constexpr (G_USE_CMS_GC) {
        static_assert(!evacuateNonMovableSpace);
        forwardAddress = workNodeHolder_->GetSlotGCAllocator()->Allocate(size);
    } else {
        forwardAddress = workNodeHolder_->GetTlabAllocator()->Allocate(size, space);
    }
    if (UNLIKELY(forwardAddress == 0)) {
        LOG_ECMA_MEM(FATAL) << "EvacuateObject alloc failed: " << " size: " << size;
        UNREACHABLE();
    }
    return forwardAddress;
}

template <bool evacuateNonMovableSpace>
uintptr_t FullGCRunner<evacuateNonMovableSpace>::AllocateAppSpawnSpace(size_t size)
{
    uintptr_t forwardAddress = heap_->GetAppSpawnSpace()->AllocateSync(size);
    if (UNLIKELY(forwardAddress == 0)) {
        LOG_ECMA_MEM(FATAL) << "Evacuate AppSpawn Object: alloc failed: "
                            << " size: " << size;
        UNREACHABLE();
    }
    return forwardAddress;
}

template <bool evacuateNonMovableSpace>
void FullGCRunner<evacuateNonMovableSpace>::RawCopyObject(uintptr_t fromAddress, uintptr_t toAddress, size_t size,
                                                          const MarkWord &markWord)
{
    size_t copySize = size - HEAD_SIZE;
    errno_t res = memcpy_s(ToVoidPtr(toAddress + HEAD_SIZE), copySize, ToVoidPtr(fromAddress + HEAD_SIZE), copySize);
    if (res != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed " << res;
    }
    *reinterpret_cast<MarkWordType *>(toAddress) = markWord.GetValue();
}

template <bool evacuateNonMovableSpace>
void FullGCRunner<evacuateNonMovableSpace>::UpdateForwardAddressIfSuccess(ObjectSlot slot, TaggedObject *object,
    JSHClass *klass, size_t size, TaggedObject *toAddress)
{
    workNodeHolder_->IncreaseAliveSize(size);

    heap_->OnMoveEvent(reinterpret_cast<uintptr_t>(object), toAddress, size);
    if (klass->HasReferenceField()) {
        PushObject(toAddress);
    }
    slot.Update(toAddress);
}

template <bool evacuateNonMovableSpace>
void FullGCRunner<evacuateNonMovableSpace>::UpdateForwardAddressIfFailed(ObjectSlot slot, size_t size,
    uintptr_t toAddress, TaggedObject *dst)
{
    FreeObject::FillFreeObject(heap_, toAddress, size);
    slot.Update(dst);
}

template <bool evacuateNonMovableSpace>
void FullGCRunner<evacuateNonMovableSpace>::PushObject(TaggedObject *object)
{
    workNodeHolder_->Push(object);
}

template <bool evacuateNonMovableSpace>
void FullGCRunner<evacuateNonMovableSpace>::RecordWeakReference(JSTaggedType *weak)
{
    workNodeHolder_->PushWeakReference(weak);
}

template <bool evacuateNonMovableSpace>
void FullGCRunner<evacuateNonMovableSpace>::RecordWeakLinkedHashMap(TaggedObject *object)
{
    ASSERT(JSTaggedValue(object).IsWeakLinkedHashMap());
    workNodeHolder_->PushWeakLinkedHashMap(object);
}

template <bool evacuateNonMovableSpace>
void FullGCRunner<evacuateNonMovableSpace>::RecordFreshWeakAggregate(WeakAggregate weakAggregate)
{
    workNodeHolder_->PushFreshWeakAggregate(weakAggregate);
}

template <bool evacuateNonMovableSpace>
void FullGCRunner<evacuateNonMovableSpace>::RecordPendingWeakAggregate(WeakAggregate weakAggregate)
{
    workNodeHolder_->PushPendingWeakAggregate(weakAggregate);
}

template <bool evacuateNonMovableSpace>
FullGCMarkRootVisitor<evacuateNonMovableSpace>::FullGCMarkRootVisitor(FullGCRunner<evacuateNonMovableSpace> *runner)
    : runner_(runner) {}

template <bool evacuateNonMovableSpace>
void FullGCMarkRootVisitor<evacuateNonMovableSpace>::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        ASSERT(!value.IsWeak());
        runner_->HandleMarkingSlotObject(slot, value.GetTaggedObject());
    }
}

template <bool evacuateNonMovableSpace>
void FullGCMarkRootVisitor<evacuateNonMovableSpace>::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start,
                                                                    ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            ASSERT(!value.IsWeak());
            runner_->HandleMarkingSlotObject(slot, value.GetTaggedObject());
        }
    }
}

template <bool evacuateNonMovableSpace>
void FullGCMarkRootVisitor<evacuateNonMovableSpace>::VisitBaseAndDerivedRoot([[maybe_unused]] Root type,
    ObjectSlot base, ObjectSlot derived, uintptr_t baseOldObject)
{
    if (JSTaggedValue(base.GetTaggedType()).IsHeapObject()) {
        derived.Update(base.GetTaggedType() + derived.GetTaggedType() - baseOldObject);
    }
}

template <bool evacuateNonMovableSpace>
FullGCMarkObjectVisitor<evacuateNonMovableSpace>::FullGCMarkObjectVisitor(FullGCRunner<evacuateNonMovableSpace> *runner)
    : runner_(runner) {}

template <bool evacuateNonMovableSpace>
void FullGCMarkObjectVisitor<evacuateNonMovableSpace>::VisitObjectRangeImpl(BaseObject *root, uintptr_t start,
    uintptr_t end, VisitObjectArea area)
{
    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        runner_->VisitBodyInObj(root, start, end, [this](ObjectSlot slot) {
            runner_->HandleMarkingSlot(slot);
        });
        return;
    }
    ObjectSlot startSlot(start);
    ObjectSlot endSlot(end);
    for (ObjectSlot slot = startSlot; slot < endSlot; slot++) {
        runner_->HandleMarkingSlot(slot);
    }
}

template <bool evacuateNonMovableSpace>
void FullGCMarkObjectVisitor<evacuateNonMovableSpace>::VisitObjectHClassImpl(BaseObject *root, BaseObject *hclass)
{
    ObjectSlot slot(ToUintPtr(root));
    runner_->HandleMarkingSlotObject(slot, reinterpret_cast<TaggedObject *>(hclass));
}

template <bool evacuateNonMovableSpace>
void FullGCMarkObjectVisitor<evacuateNonMovableSpace>::VisitWeakLinkedHashMapImpl(BaseObject *rootObject)
{
    TaggedObject *obj = TaggedObject::Cast(rootObject);
    ASSERT(JSTaggedValue(obj).IsWeakLinkedHashMap());
    ASSERT(!Region::ObjectAddressToRange(obj)->InSharedHeap());
    runner_->RecordWeakLinkedHashMap(obj);

    WeakLinkedHashMap *map = WeakLinkedHashMap::Cast(obj);
    ASSERT(map->VerifyLayout());
    // pay attention to this unusual access
    JSTaggedType *data = map->GetData();
    ObjectSlot nextTableSlot(ToUintPtr(&data[WeakLinkedHashMap::NEXT_TABLE_INDEX]));
    runner_->HandleMarkingSlot(nextTableSlot);

    int entries = map->NumberOfAllUsedElements();
    for (int i = 0; i < entries; ++i) {
        int keyIdx = map->GetKeyIndex(i);
        ObjectSlot keySlot(ToUintPtr(&data[keyIdx]));
        if (!keySlot.GetTaggedValue().IsHeapObject()) {
            continue;
        }

        ASSERT(keySlot.GetTaggedValue().IsWeak());
        runner_->RecordWeakReference(reinterpret_cast<JSTaggedType *>(keySlot.SlotAddress()));

        int valueIdx = map->GetValueIndex(i);
        ObjectSlot valueSlot(ToUintPtr(&data[valueIdx]));
        if (!valueSlot.GetTaggedValue().IsHeapObject()) {
            continue;
        }

        WeakAggregate weakAggregate = {keySlot, valueSlot};
        if (!runner_->HandleWeakAggregate(weakAggregate)) {
            runner_->RecordFreshWeakAggregate({keySlot, valueSlot});
        }
    }
}

template <bool evacuateNonMovableSpace>
FullGCUpdateLocalToShareRSetVisitor<evacuateNonMovableSpace>::FullGCUpdateLocalToShareRSetVisitor(
    FullGCRunner<evacuateNonMovableSpace> *runner) : runner_(runner) {}

template <bool evacuateNonMovableSpace>
void FullGCUpdateLocalToShareRSetVisitor<evacuateNonMovableSpace>::VisitObjectRangeImpl(BaseObject *root,
    uintptr_t start, uintptr_t end, VisitObjectArea area)
{
    Region *rootRegion = Region::ObjectAddressToRange(root);
    ASSERT(!rootRegion->InSharedHeap());
    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        runner_->VisitBodyInObj(root, start, end, [this, rootRegion](ObjectSlot slot) {
            SetLocalToShareRSet(slot, rootRegion);
        });
        return;
    }
    ObjectSlot startSlot(start);
    ObjectSlot endSlot(end);
    for (ObjectSlot slot = startSlot; slot < endSlot; slot++) {
        SetLocalToShareRSet(slot, rootRegion);
    }
}

template <bool evacuateNonMovableSpace>
void FullGCUpdateLocalToShareRSetVisitor<evacuateNonMovableSpace>::SetLocalToShareRSet(ObjectSlot slot,
                                                                                       Region *rootRegion)
{
    ASSERT(!rootRegion->InSharedHeap());
    JSTaggedType value = slot.GetTaggedType();
    if (!JSTaggedValue(value).IsHeapObject()) {
        return;
    }

    Region *valueRegion = Region::ObjectAddressToRange(value);
    if (valueRegion->InSharedSweepableSpace()) {
        rootRegion->AtomicInsertLocalToShareRSet(slot.SlotAddress());
    }
}

}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_FULL_GC_INL_H
