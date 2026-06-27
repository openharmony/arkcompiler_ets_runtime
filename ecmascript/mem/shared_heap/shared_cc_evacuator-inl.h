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

#ifndef ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CC_EVACUATOR_INL_H
#define ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CC_EVACUATOR_INL_H

#include "ecmascript/mem/shared_heap/shared_cc_evacuator.h"

#include "ecmascript/free_object.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/barriers-inl.h"
#include "ecmascript/mem/heap.h"
#include "ecmascript/mem/region-inl.h"
#include "ecmascript/mem/slots.h"
#include "ecmascript/mem/tlab_allocator-inl.h"
#include "ecmascript/mem/work_manager.h"
#include "securec.h"

namespace panda::ecmascript {
inline SharedCCEvacuator::SharedCCEvacuator(SharedHeap *heap)
    : sHeap_(heap), tlab_(new SharedTlabAllocator(heap)), ownsAllocator_(true) {}

inline TaggedObject* SharedCCEvacuator::Copy(TaggedObject *fromObj, const MarkWord &markWord)
{
    JSHClass *klass = markWord.GetJSHClass();
    size_t size = klass->SizeFromJSHClass(fromObj);

    uintptr_t forwardAddr = tlab_->Allocate(size, SHARED_COMPRESS_SPACE);
    if (forwardAddr == 0) {
        LOG_GC(FATAL) << "SharedCC: Allocate failed in Copy";
        UNREACHABLE();
    }

    RawCopyObject(ToUintPtr(fromObj), forwardAddr, size, markWord);

    // Ensure all non-atomic writes from memcpy are globally visible before publishing the forwarding
    // address. Without this fence, a concurrent reader (JS thread via read barrier) may observe the
    // forwarding address via relaxed load but see a partially-copied object body (ARM Store-Store
    // reordering). This matches the pattern in LocalCC CCEvacuator::Copy (cc_evacuator.cpp:37).
    std::atomic_thread_fence(std::memory_order_seq_cst);

    if (SetForwardingAddress(fromObj, markWord, forwardAddr)) {
        return reinterpret_cast<TaggedObject*>(forwardAddr);
    }
    // Another thread won the race, free the allocated memory and use their forwarding address
    // Follow the same pattern as LocalCC (cc_evacuator.cpp) and SharedFullGC (shared_full_gc-inl.h)
    FreeObject::FillFreeObject(sHeap_, forwardAddr, size);
    MarkWord newMarkWord(fromObj, RELAXED_LOAD);
    return newMarkWord.ToForwardingAddress();
}

inline void SharedCCEvacuator::RawCopyObject(uintptr_t from, uintptr_t to, size_t size, const MarkWord &markWord)
{
    if (memcpy_s(reinterpret_cast<void*>(to), size, reinterpret_cast<void*>(from), size) != EOK) {
        LOG_GC(FATAL) << "SharedCC: memcpy_s failed in RawCopyObject";
        return;
    }

    TaggedObject *toObject = reinterpret_cast<TaggedObject*>(to);
    JSHClass *klass = markWord.GetJSHClass();
    toObject->SetClassWithoutBarrier(klass);
}

inline bool SharedCCEvacuator::SetForwardingAddress(TaggedObject *object, const MarkWord &markWord,
                                                    uintptr_t forwardAddr)
{
    auto oldValue = markWord.GetValue();
    auto newValue = MarkWord::FromForwardingAddress(forwardAddr);
    auto result = Barriers::AtomicSetPrimitive(object, 0, oldValue, newValue);
    return result == oldValue;
}

inline void ProcessSharedCCWeakReferenceSlot(ObjectSlot &slot, SharedCCEvacuator &evacuator)
{
    JSTaggedValue value(slot.GetTaggedType());
    if (!value.IsWeak()) {
        return;
    }
    auto header = value.GetTaggedWeakRef();
    Region *objectRegion = Region::ObjectAddressToRange(header);
    if (objectRegion->IsFromRegion()) {
        if (!objectRegion->Test(header)) {
            slot.Clear();
            return;
        }
        MarkWord markWord(header, RELAXED_LOAD);
        if (markWord.IsForwardingAddress()) {
            TaggedObject *dst = markWord.ToForwardingAddress();
            auto weakRef = JSTaggedValue(JSTaggedValue(dst).CreateAndGetWeakRef()).GetRawTaggedObject();
            slot.Update(weakRef);
        } else {
            TaggedObject *dst = evacuator.Copy(header, markWord);
            auto weakRef = JSTaggedValue(JSTaggedValue(dst).CreateAndGetWeakRef()).GetRawTaggedObject();
            slot.Update(weakRef);
        }
        return;
    }
    if (!objectRegion->InSharedSweepableSpace() || objectRegion->Test(header)) {
        return;
    }
    slot.Clear();
}

inline void UpdateSharedCCWeakReferences(ProcessQueue *queue, SharedCCEvacuator &evacuator)
{
    auto *obj = queue->PopBack();
    while (obj != nullptr) {
        ObjectSlot slot(ToUintPtr(obj));
        ProcessSharedCCWeakReferenceSlot(slot, evacuator);
        obj = queue->PopBack();
    }
}

}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SHARED_HEAP_SHARED_CC_EVACUATOR_INL_H
