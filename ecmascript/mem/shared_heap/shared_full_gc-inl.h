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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_FULL_GC_INL_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_FULL_GC_INL_H

#include "ecmascript/mem/shared_heap/shared_full_gc.h"

#include "ecmascript/js_hclass-inl.h"
#include "ecmascript/mem/work_manager-inl.h"

namespace panda::ecmascript {

SharedFullGCRunner::SharedFullGCRunner(SharedHeap *sHeap, SharedGCWorkNodeHolder *sWorkNodeHolder)
    : sHeap_(sHeap), sWorkNodeHolder_(sWorkNodeHolder), markRootVisitor_(this), markObjectVisitor_(this),
      markLocalToShareRSetVisitor_(this)
{
}

SharedFullGCMarkRootVisitor &SharedFullGCRunner::GetMarkRootVisitor()
{
    return markRootVisitor_;
}

SharedFullGCMarkObjectVisitor &SharedFullGCRunner::GetMarkObjectVisitor()
{
    return markObjectVisitor_;
}

SharedFullGCMarkLocalToShareRSetVisitor &SharedFullGCRunner::GetMarkLocalToShareRSetVisitor()
{
    return markLocalToShareRSetVisitor_;
}

bool SharedFullGCRunner::NeedEvacuate(Region *region) const
{
    return region->InSharedOldSpace();
}

void SharedFullGCRunner::MarkValue(ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsInSharedSweepableSpace()) {
        if (!value.IsWeakForHeapObject()) {
            MarkObject(slot, value.GetTaggedObject());
        } else {
            RecordWeakReference(reinterpret_cast<JSTaggedType *>(slot.SlotAddress()));
        }
    }
}

void SharedFullGCRunner::MarkObject(ObjectSlot slot, TaggedObject *object)
{
    Region *objectRegion = Region::ObjectAddressToRange(object);
    ASSERT(objectRegion->InSharedHeap());
    if (!NeedEvacuate(objectRegion)) {
        if (!objectRegion->InSharedReadOnlySpace() && objectRegion->AtomicMark(object)) {
            auto size = object->GetSize();
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

void SharedFullGCRunner::EvacuateObject(ObjectSlot slot, TaggedObject *object, const MarkWord &markWord)
{
    JSHClass *klass = markWord.GetJSHClass();
    size_t size = klass->SizeFromJSHClass(object);
    uintptr_t forwardAddress = AllocateForwardAddress(size);
    RawCopyObject(ToUintPtr(object), forwardAddress, size, markWord);

    auto oldValue = markWord.GetValue();
    auto result = Barriers::AtomicSetPrimitive(object, 0, oldValue,
                                               MarkWord::FromForwardingAddress(forwardAddress));
    if (result == oldValue) {
        TaggedObject *toObject = reinterpret_cast<TaggedObject *>(forwardAddress);
        UpdateForwardAddressIfSuccess(slot, object, klass, size, toObject);
        return;
    }
    ASSERT(MarkWord::IsForwardingAddress(result));
    UpdateForwardAddressIfFailed(slot, size, forwardAddress, MarkWord::ToForwardingAddress(result));
}

uintptr_t SharedFullGCRunner::AllocateDstSpace(size_t size)
{
    uintptr_t forwardAddress = 0;
    forwardAddress = sWorkNodeHolder_->GetTlabAllocator()->Allocate(size, SHARED_COMPRESS_SPACE);
    if (UNLIKELY(forwardAddress == 0)) {
        LOG_ECMA_MEM(FATAL) << "EvacuateObject alloc failed: "
                            << " size: " << size;
        UNREACHABLE();
    }
    return forwardAddress;
}

void SharedFullGCRunner::RawCopyObject(uintptr_t fromAddress, uintptr_t toAddress, size_t size,
                                       const MarkWord &markWord)
{
    if (memcpy_s(ToVoidPtr(toAddress + HEAD_SIZE), size - HEAD_SIZE, ToVoidPtr(fromAddress + HEAD_SIZE),
        size - HEAD_SIZE) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
    }
    *reinterpret_cast<MarkWordType *>(toAddress) = markWord.GetValue();
}

void SharedFullGCRunner::UpdateForwardAddressIfSuccess(ObjectSlot slot, TaggedObject *object, JSHClass *klass,
                                                       size_t size, TaggedObject *toObject)
{
    sWorkNodeHolder_->IncreaseAliveSize(size);

    if (UNLIKELY(sHeap_->InHeapProfiler())) {
        sHeap_->OnMoveEvent(ToUintPtr(object), toObject, size);
    }
    if (klass->HasReferenceField()) {
        PushObject(toObject);
    }

    slot.Update(toObject);
}

void SharedFullGCRunner::UpdateForwardAddressIfFailed(ObjectSlot slot, size_t size, uintptr_t toAddress,
                                                      TaggedObject *dst)
{
    FreeObject::FillFreeObject(sHeap_, toAddress, size);
    slot.Update(dst);
}

uintptr_t SharedFullGCRunner::AllocateForwardAddress(size_t size)
{
    return AllocateDstSpace(size);
}

void SharedFullGCRunner::PushObject(TaggedObject *object)
{
    sWorkNodeHolder_->Push(object);
}

void SharedFullGCRunner::RecordWeakReference(JSTaggedType *weak)
{
    sWorkNodeHolder_->PushWeakReference(weak);
}

SharedFullGCMarkRootVisitor::SharedFullGCMarkRootVisitor(SharedFullGCRunner *sRunner)
    : sRunner_(sRunner)
{
}

void SharedFullGCMarkRootVisitor::VisitRoot([[maybe_unused]] Root type, ObjectSlot slot)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsInSharedSweepableSpace()) {
        ASSERT(!value.IsWeak());
        sRunner_->MarkObject(slot, value.GetTaggedObject());
    }
}

void SharedFullGCMarkRootVisitor::VisitRangeRoot([[maybe_unused]] Root type, ObjectSlot start, ObjectSlot end)
{
    for (ObjectSlot slot = start; slot < end; slot++) {
        JSTaggedValue value(slot.GetTaggedType());
        if (value.IsInSharedSweepableSpace()) {
            ASSERT(!value.IsWeak());
            sRunner_->MarkObject(slot, value.GetTaggedObject());
        }
    }
}

void SharedFullGCMarkRootVisitor::VisitBaseAndDerivedRoot([[maybe_unused]] Root type, ObjectSlot base,
                                                          ObjectSlot derived, uintptr_t baseOldObject)
{
    if (JSTaggedValue(base.GetTaggedType()).IsHeapObject()) {
        derived.Update(base.GetTaggedType() + derived.GetTaggedType() - baseOldObject);
    }
}

SharedFullGCMarkObjectVisitor::SharedFullGCMarkObjectVisitor(SharedFullGCRunner *sRunner)
    : sRunner_(sRunner)
{
}

void SharedFullGCMarkObjectVisitor::VisitObjectRangeImpl(BaseObject *rootObject, uintptr_t startAddr, uintptr_t endAddr,
                                                         VisitObjectArea area)
{
    ObjectSlot start(startAddr);
    ObjectSlot end(endAddr);
    if (UNLIKELY(area == VisitObjectArea::IN_OBJECT)) {
        auto root = TaggedObject::Cast(rootObject);
        JSHClass *hclass = root->SynchronizedGetClass();
        ASSERT(!hclass->IsAllTaggedProp());
        int index = 0;
        LayoutInfo *layout = LayoutInfo::UncheckCast(hclass->GetLayout(THREAD_ARG_PLACEHOLDER).GetTaggedObject());
        ObjectSlot realEnd = start;
        realEnd += layout->GetPropertiesCapacity();
        end = end > realEnd ? realEnd : end;
        for (ObjectSlot slot = start; slot < end; slot++) {
            PropertyAttributes attr = layout->GetAttr(THREAD_ARG_PLACEHOLDER, index++);
            if (attr.IsTaggedRep()) {
                sRunner_->MarkValue(slot);
            }
        }
        return;
    }
    for (ObjectSlot slot = start; slot < end; slot++) {
        sRunner_->MarkValue(slot);
    }
}

void SharedFullGCMarkObjectVisitor::VisitHClassSlot(ObjectSlot slot, TaggedObject *hclass)
{
    ASSERT(hclass->GetClass()->IsHClass());
    sRunner_->MarkObject(slot, hclass);
}

SharedFullGCMarkLocalToShareRSetVisitor::SharedFullGCMarkLocalToShareRSetVisitor(
    SharedFullGCRunner *sRunner) : sRunner_(sRunner)
{
}

bool SharedFullGCMarkLocalToShareRSetVisitor::operator()(void *mem) const
{
    ObjectSlot slot(ToUintPtr(mem));
    JSTaggedValue value(slot.GetTaggedType());
    if (value.IsInSharedSweepableSpace()) {
        if (!value.IsWeakForHeapObject()) {
            sRunner_->MarkObject(slot, value.GetTaggedObject());
        } else {
            sRunner_->RecordWeakReference(reinterpret_cast<JSTaggedType *>(mem));
        }
        return true;
    }
    return false;
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_FULL_GC_INL_H
