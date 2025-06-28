/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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
#include "common_components/heap/w_collector/trace_barrier.h"

#include "common_components/mutator/mutator.h"
#if defined(COMMON_TSAN_SUPPORT)
#include "common_components/sanitizer/sanitizer_interface.h"
#endif

namespace common {
// Because gc thread will also have impact on tagged pointer in enum and trace phase,
// so we don't expect reading barrier have the ability to modify the referent field.
BaseObject* TraceBarrier::ReadRefField(BaseObject* obj, RefField<false>& field) const
{
    RefField<> tmpField(field);
    return (BaseObject*)tmpField.GetFieldValue();
}

BaseObject* TraceBarrier::ReadStaticRef(RefField<false>& field) const { return ReadRefField(nullptr, field); }

void TraceBarrier::ReadStruct(HeapAddress dst, BaseObject* obj, HeapAddress src, size_t size) const
{
    CHECK_CC(memcpy_s(reinterpret_cast<void*>(dst), size, reinterpret_cast<void*>(src), size) == EOK);
}

void TraceBarrier::WriteRefField(BaseObject* obj, RefField<false>& field, BaseObject* ref) const
{
    UpdateRememberSet(obj, ref);
    RefField<> tmpField(field);
    BaseObject* rememberedObject = nullptr;
    rememberedObject = tmpField.GetTargetObject();
    Mutator* mutator = Mutator::GetMutator();
    if (rememberedObject != nullptr) {
        mutator->RememberObjectInSatbBuffer(rememberedObject);
    }
    DLOG(BARRIER, "write obj %p ref-field@%p: %#zx -> %p", obj, &field, rememberedObject, ref);
    std::atomic_thread_fence(std::memory_order_seq_cst);
    RefField<> newField(ref);
    field.SetFieldValue(newField.GetFieldValue());
}
#ifdef ARK_USE_SATB_BARRIER
void TraceBarrier::WriteBarrier(BaseObject* obj, RefField<false>& field, BaseObject* ref) const
{
    RefField<> tmpField(field);
    BaseObject* rememberedObject = nullptr;
    rememberedObject = tmpField.GetTargetObject();
    if (!Heap::IsTaggedObject(field.GetFieldValue())) {
        return;
    }
    UpdateRememberSet(obj, ref);
    Mutator* mutator = Mutator::GetMutator();
    if (rememberedObject != nullptr) {
        mutator->RememberObjectInSatbBuffer(rememberedObject);
    }
    if (ref != nullptr) {
        if (!Heap::IsTaggedObject((HeapAddress)ref)) {
            return;
        }
        ref = (BaseObject*)((uintptr_t)ref & ~(TAG_WEAK));
        mutator->RememberObjectInSatbBuffer(ref);
    }

    DLOG(BARRIER, "write obj %p ref-field@%p: %#zx -> %p", obj, &field, rememberedObject, ref);
}
#else
void TraceBarrier::WriteBarrier(BaseObject* obj, RefField<false>& field, BaseObject* ref) const
{
    if (!Heap::IsTaggedObject((HeapAddress)ref)) {
        return;
    }
    UpdateRememberSet(obj, ref);
    ref = (BaseObject*)((uintptr_t)ref & ~(TAG_WEAK));
    Mutator* mutator = Mutator::GetMutator();
    mutator->RememberObjectInSatbBuffer(ref);
    DLOG(BARRIER, "write obj %p ref-field@%p: -> %p", obj, &field, ref);
}
#endif

void TraceBarrier::WriteStaticRef(RefField<false>& field, BaseObject* ref) const
{
    std::atomic_thread_fence(std::memory_order_seq_cst);
    RefField<> newField(ref);
    field.SetFieldValue(newField.GetFieldValue());
}

void TraceBarrier::WriteStruct(BaseObject* obj, HeapAddress dst, size_t dstLen, HeapAddress src, size_t srcLen) const
{
    CHECK_CC(obj != nullptr);
    if (obj != nullptr) {
        ASSERT_LOGF(dst > reinterpret_cast<HeapAddress>(obj), "WriteStruct struct addr is less than obj!");
        Mutator* mutator = Mutator::GetMutator();
        if (mutator != nullptr) {
            obj->ForEachRefInStruct(
                [=](RefField<>& refField) {
                    mutator->RememberObjectInSatbBuffer(ReadRefField(obj, refField));
                },
                dst, dst + dstLen);
        }
    }
    std::atomic_thread_fence(std::memory_order_seq_cst);
    CHECK_CC(memcpy_s(reinterpret_cast<void*>(dst), dstLen, reinterpret_cast<void*>(src), srcLen) == EOK);

#if defined(COMMON_TSAN_SUPPORT)
    Sanitizer::TsanWriteMemoryRange(reinterpret_cast<void*>(dst), dstLen);
    Sanitizer::TsanReadMemoryRange(reinterpret_cast<void*>(src), srcLen);
#endif
}

BaseObject* TraceBarrier::AtomicReadRefField(BaseObject* obj, RefField<true>& field, MemoryOrder order) const
{
    BaseObject* target = nullptr;
    RefField<false> oldField(field.GetFieldValue(order));
    target = (BaseObject*)(oldField.GetFieldValue());
    DLOG(TBARRIER, "katomic read obj %p ref@%p: %#zx -> %p", obj, &field, oldField.GetFieldValue(), target);
    return target;
}

void TraceBarrier::AtomicWriteRefField(BaseObject* obj, RefField<true>& field, BaseObject* newRef,
                                       MemoryOrder order) const
{
    RefField<> oldField(field.GetFieldValue(order));
    HeapAddress oldValue = oldField.GetFieldValue();
    (void)oldValue;
    BaseObject* oldRef = oldField.GetTargetObject();
    RefField<> newField(newRef);
    field.SetFieldValue(newField.GetFieldValue(), order);
    Mutator* mutator = Mutator::GetMutator();
    mutator->RememberObjectInSatbBuffer(oldRef);
    if (obj != nullptr) {
        DLOG(TBARRIER, "atomic write obj %p<%p>(%zu) ref@%p: %#zx -> %#zx", obj, obj->GetTypeInfo(), obj->GetSize(),
             &field, oldValue, newField.GetFieldValue());
    } else {
        DLOG(TBARRIER, "atomic write static ref@%p: %#zx -> %#zx", &field, oldValue, newField.GetFieldValue());
    }
}

BaseObject* TraceBarrier::AtomicSwapRefField(BaseObject* obj, RefField<true>& field, BaseObject* newRef,
                                             MemoryOrder order) const
{
    RefField<> newField(newRef);
    HeapAddress oldValue = field.Exchange(newField.GetFieldValue(), order);
    RefField<> oldField(oldValue);
    BaseObject* oldRef = oldField.GetTargetObject();
    Mutator* mutator = Mutator::GetMutator();
    mutator->RememberObjectInSatbBuffer(oldRef);
    DLOG(TRACE, "atomic swap obj %p<%p>(%zu) ref-field@%p: old %#zx(%p), new %#zx(%p)", obj, obj->GetTypeInfo(),
         obj->GetSize(), &field, oldValue, oldRef, field.GetFieldValue(), newRef);
    return oldRef;
}

bool TraceBarrier::CompareAndSwapRefField(BaseObject* obj, RefField<true>& field, BaseObject* oldRef,
                                          BaseObject* newRef, MemoryOrder succOrder, MemoryOrder failOrder) const
{
    HeapAddress oldFieldValue = field.GetFieldValue(std::memory_order_seq_cst);
    RefField<false> oldField(oldFieldValue);
    BaseObject* oldVersion = oldField.GetTargetObject();
    RefField<> newField(newRef);

    while (oldVersion == oldRef) {
        if (field.CompareExchange(oldFieldValue, newField.GetFieldValue(), succOrder, failOrder)) {
            Mutator* mutator = Mutator::GetMutator();
            mutator->RememberObjectInSatbBuffer(oldRef);
            return true;
        }
        oldFieldValue = field.GetFieldValue(std::memory_order_seq_cst);
        RefField<false> tmp(oldFieldValue);
        oldVersion = tmp.GetTargetObject();
    }

    return false;
}

void TraceBarrier::CopyStructArray(BaseObject* dstObj, HeapAddress dstField, MIndex dstSize, BaseObject* srcObj,
                                   HeapAddress srcField, MIndex srcSize) const
{
    LOG_COMMON(FATAL) << "Unresolved fatal";
    UNREACHABLE_CC();
}

} // namespace common
