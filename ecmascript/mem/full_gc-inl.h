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

FullGCRunner::FullGCRunner(Heap *heap, WorkNodeHolder *workNodeHolder, bool isAppSpawn)
    : heap_(heap), workNodeHolder_(workNodeHolder), isAppSpawn_(isAppSpawn), markRootVisitor_(this),
      markObjectVisitor_(this), updateLocalToShareRSetVisitor_(this) {}

FullGCMarkRootVisitor &FullGCRunner::GetMarkRootVisitor()
{
    return markRootVisitor_;
}

FullGCMarkObjectVisitor &FullGCRunner::GetMarkObjectVisitor()
{
    return markObjectVisitor_;
}

void FullGCRunner::HandleMarkingSlot(ObjectSlot slot)
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

template <class Callback>
void FullGCRunner::VisitBodyInObj(BaseObject *root, uintptr_t start, uintptr_t endAddr, Callback &&cb)
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

void FullGCRunner::MarkJitCodeVec(JitCodeVector *vec)
{
    for (auto &jitCodeMap : *vec) {
        auto &jitCode = std::get<0>(jitCodeMap);
        auto obj = static_cast<TaggedObject *>(jitCode);
        // jitcode is MachineCode, and MachineCode is in the MachineCode space, will not be evacute.
        HandleMarkingSlotObject(ObjectSlot(reinterpret_cast<uintptr_t>(&jitCode)), obj);
    }
}

void FullGCRunner::HandleMarkingSlotObject(ObjectSlot slot, TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    if (objectRegion->InSharedHeap()) {
        return;
    }
    if (!NeedEvacuate(objectRegion)) {
        if (objectRegion->AtomicMark(object)) {
            size_t size = object->GetSize();
            objectRegion->IncreaseAliveObject(size);
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

bool FullGCRunner::HandleWeakAggregate(WeakAggregate weakAggregate)
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

bool FullGCRunner::NeedEvacuate(Region *region)
{
    ASSERT(!region->InSharedHeap());
    if (isAppSpawn_) {
        return !region->InHugeObjectSpace()  && !region->InReadOnlySpace() && !region->InNonMovableSpace();
    }
    // fixme: refactor?
    if constexpr (G_USE_CMS_GC) {
        return region->InSlotSpace();
    }
    return region->InYoungOrOldSpace();
}

bool FullGCRunner::IsAlive(TaggedObject *object)
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

void FullGCRunner::EvacuateObject(ObjectSlot slot, TaggedObject *object, const MarkWord &markWord)
{
    JSHClass *klass = markWord.GetJSHClass();
    size_t size = klass->SizeFromJSHClass(object);
    uintptr_t forwardAddress = AllocateForwardAddress(size);
    RawCopyObject(ToUintPtr(object), forwardAddress, size, markWord);

    MarkWordType oldValue = markWord.GetValue();
    MarkWordType result = Barriers::AtomicSetPrimitive(object, 0, oldValue,
                                                       MarkWord::FromForwardingAddress(forwardAddress));
    if (result == oldValue) {
        TaggedObject *toObject = reinterpret_cast<TaggedObject*>(forwardAddress);
        UpdateForwardAddressIfSuccess(slot, object, klass, size, toObject);
        Region *region = Region::ObjectAddressToRange(object);
        if (region->HasLocalToShareRememberedSet()) {
            ObjectXRay::VisitObjectBody<VisitType::OLD_GC_VISIT>(toObject, klass, updateLocalToShareRSetVisitor_);
        }
        return;
    }
    ASSERT(MarkWord::IsForwardingAddress(result));
    UpdateForwardAddressIfFailed(slot, size, forwardAddress, MarkWord::ToForwardingAddress(result));
}

uintptr_t FullGCRunner::AllocateForwardAddress(size_t size)
{
    if (!isAppSpawn_) {
        return AllocateDstSpace(size);
    }
    return AllocateAppSpawnSpace(size);
}

uintptr_t FullGCRunner::AllocateDstSpace(size_t size)
{
    uintptr_t forwardAddress;
    // fixme: refactor?
    if constexpr (G_USE_CMS_GC) {
        forwardAddress = workNodeHolder_->GetSlotGCAllocator()->Allocate(size);
    } else {
        forwardAddress = workNodeHolder_->GetTlabAllocator()->Allocate(size, MemSpaceType::COMPRESS_SPACE);
    }
    if (UNLIKELY(forwardAddress == 0)) {
        LOG_ECMA_MEM(FATAL) << "EvacuateObject alloc failed: " << " size: " << size;
        UNREACHABLE();
    }
    return forwardAddress;
}

uintptr_t FullGCRunner::AllocateAppSpawnSpace(size_t size)
{
    uintptr_t forwardAddress = heap_->GetAppSpawnSpace()->AllocateSync(size);
    if (UNLIKELY(forwardAddress == 0)) {
        LOG_ECMA_MEM(FATAL) << "Evacuate AppSpawn Object: alloc failed: "
                            << " size: " << size;
        UNREACHABLE();
    }
    return forwardAddress;
}

void FullGCRunner::RawCopyObject(uintptr_t fromAddress, uintptr_t toAddress, size_t size, const MarkWord &markWord)
{
    size_t copySize = size - HEAD_SIZE;
    errno_t res = memcpy_s(ToVoidPtr(toAddress + HEAD_SIZE), copySize, ToVoidPtr(fromAddress + HEAD_SIZE), copySize);
    if (res != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed " << res;
    }
    *reinterpret_cast<MarkWordType *>(toAddress) = markWord.GetValue();
}

void FullGCRunner::UpdateForwardAddressIfSuccess(ObjectSlot slot, TaggedObject *object, JSHClass *klass, size_t size,
    TaggedObject *toAddress)
{
    workNodeHolder_->IncreaseAliveSize(size);

    heap_->OnMoveEvent(reinterpret_cast<uintptr_t>(object), toAddress, size);
    if (klass->HasReferenceField()) {
        PushObject(toAddress);
    }
    slot.Update(toAddress);
}

void FullGCRunner::UpdateForwardAddressIfFailed(ObjectSlot slot, size_t size, uintptr_t toAddress, TaggedObject *dst)
{
    FreeObject::FillFreeObject(heap_, toAddress, size);
    slot.Update(dst);
}

void FullGCRunner::PushObject(TaggedObject *object)
{
    workNodeHolder_->Push(object);
}

void FullGCRunner::RecordWeakReference(JSTaggedType *weak)
{
    workNodeHolder_->PushWeakReference(weak);
}

void FullGCRunner::RecordWeakLinkedHashMap(TaggedObject *object)
{
    ASSERT(JSTaggedValue(object).IsWeakLinkedHashMap());
    workNodeHolder_->PushWeakLinkedHashMap(object);
}

void FullGCRunner::RecordFreshWeakAggregate(WeakAggregate weakAggregate)
{
    workNodeHolder_->PushFreshWeakAggregate(weakAggregate);
}

void FullGCRunner::RecordPendingWeakAggregate(WeakAggregate weakAggregate)
{
    workNodeHolder_->PushPendingWeakAggregate(weakAggregate);
}

FullGCMarkRootVisitor::FullGCMarkRootVisitor(FullGCRunner *runner) : runner_(runner) {}

void FullGCMarkRootVisitor::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsHeapObject()) {
        ASSERT(!value.IsWeak());
        runner_->HandleMarkingSlotObject(slot, value.GetTaggedObject());
    }
}

void FullGCMarkRootVisitor::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsHeapObject()) {
            ASSERT(!value.IsWeak());
            runner_->HandleMarkingSlotObject(slot, value.GetTaggedObject());
        }
    }
}

void FullGCMarkRootVisitor::VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base, ObjectSlot derived,
                                                    uintptr_t baseOldObject)
{
    if (JSTaggedValue(base.GetTaggedType()).IsHeapObject()) {
        derived.Update(base.GetTaggedType() + derived.GetTaggedType() - baseOldObject);
    }
}

FullGCMarkObjectVisitor::FullGCMarkObjectVisitor(FullGCRunner *runner) : runner_(runner) {}

void FullGCMarkObjectVisitor::VisitObjectRangeImpl(BaseObject *root, uintptr_t start, uintptr_t end,
                                                   VisitObjectArea area)
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

void FullGCMarkObjectVisitor::VisitWeakLinkedHashMapImpl(BaseObject *rootObject)
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

void FullGCMarkObjectVisitor::VisitHClassSlot(ObjectSlot slot, TaggedObject *hclass)
{
    ASSERT(hclass->GetClass()->IsHClass());
    runner_->HandleMarkingSlotObject(slot, hclass);
}

FullGCUpdateLocalToShareRSetVisitor::FullGCUpdateLocalToShareRSetVisitor(FullGCRunner *runner) : runner_(runner) {}

void FullGCUpdateLocalToShareRSetVisitor::VisitObjectRangeImpl(BaseObject *root, uintptr_t start, uintptr_t end,
                                                               VisitObjectArea area)
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

void FullGCUpdateLocalToShareRSetVisitor::SetLocalToShareRSet(ObjectSlot slot, Region *rootRegion)
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
