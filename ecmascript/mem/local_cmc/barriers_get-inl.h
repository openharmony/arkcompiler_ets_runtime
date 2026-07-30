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

#ifndef ECMASCRIPT_MEM_LOCAL_CMC_BARRIERS_GET_INL_H
#define ECMASCRIPT_MEM_LOCAL_CMC_BARRIERS_GET_INL_H

#include "ecmascript/js_tagged_value_wrapper.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/barriers.h"

namespace panda::ecmascript {
PUBLIC_API JSTaggedType ReadBarrierImpl(const JSThread *thread, uintptr_t slotAddress, JSTaggedValue value);
PUBLIC_API TemporaryJSTaggedValue ReadBarrierForCompressedImpl(const JSThread *thread, uintptr_t slotAddress,
    TemporaryJSTaggedValue temporaryValue);
PUBLIC_API JSTaggedType ReadBarrierForStringTableSlotImpl(JSTaggedType value, const JSThread *thread);
static ARK_INLINE JSTaggedType ReadBarrier(const JSThread *thread, const void *obj, size_t offset,
                                           const JSTaggedValue &value)
{
    // If sticky CMS-GC is enabled, hclass value with object state may not be regareded as heap object and
    // will skip ReadBarrierImpl. This is fine because hclass must be allocated in non-movable space which
    // does not participate in concurrent copying, therefore ReadBarrierImpl is not required. Moreover,
    // ReadBarrier should only be called by js thread and loading hclass value should use specific getter.
    if (value.IsHeapObject()) {
        return ReadBarrierImpl(thread, ToUintPtr(obj) + offset, value);
    }
    return value.GetRawData();
}

static ARK_INLINE TemporaryJSTaggedValue ReadBarrierForCompressed(const JSThread *thread,
    JSTaggedValue obj, size_t offset, TemporaryJSTaggedValue temporaryValue)
{
    // If sticky CMS-GC is enabled, hclass value with object state may not be regareded as heap object and
    // will skip ReadBarrierImpl. This is fine because hclass must be allocated in non-movable space which
    // does not participate in concurrent copying, therefore ReadBarrierImpl is not required. Moreover,
    // ReadBarrier should only be called by js thread and loading hclass value should use specific getter.
    if (temporaryValue.IsHeapObject()) {
        return ReadBarrierForCompressedImpl(thread, ToUintPtr(obj.GetTaggedObject()) + offset, temporaryValue);
    }
    return temporaryValue;
}

static ARK_INLINE JSTaggedType ReadBarrier(const JSThread *thread, uintptr_t slotAddress, const JSTaggedValue &value)
{
    if (value.IsHeapObject()) {
        return ReadBarrierImpl(thread, slotAddress, value);
    }
    return value.GetRawData();
}

static ARK_INLINE TemporaryJSTaggedValue ReadBarrierForCompressed(const JSThread *thread, uintptr_t slotAddress,
    TemporaryJSTaggedValue temporaryValue)
{
    if (temporaryValue.IsHeapObject()) {
        return ReadBarrierForCompressedImpl(thread, slotAddress, temporaryValue);
    }
    return temporaryValue;
}

static ARK_INLINE JSTaggedType AtomicReadBarrier(const JSThread *thread, const void *obj, size_t offset,
                                                 const JSTaggedValue &value)
{
    if (value.IsHeapObject()) {
        return ReadBarrierImpl(thread, ToUintPtr(obj) + offset, value);
    }
    return value.GetRawData();
}

inline ARK_INLINE JSTaggedType Barriers::ReadBarrierForObject(const JSThread *thread, uintptr_t slotAddress,
                                                               JSTaggedValue value)
{
    return ReadBarrierImpl(thread, slotAddress, value);
}

inline ARK_INLINE TemporaryJSTaggedValue Barriers::ReadBarrierForCompressedForObject(const JSThread *thread,
    uintptr_t slotAddress, TemporaryJSTaggedValue temporaryValue)
{
    return ReadBarrierForCompressedImpl(thread, slotAddress, temporaryValue);
}

inline ARK_INLINE JSTaggedType Barriers::ReadBarrierForStringTableSlot(JSTaggedType value, const JSThread *thread)
{
    return ReadBarrierForStringTableSlotImpl(value, thread);
}

inline ARK_INLINE JSTaggedType Barriers::GetTaggedValue(const JSThread *thread, const void *obj, size_t offset)
{
    JSTaggedValue value = *reinterpret_cast<JSTaggedValue *>(ToUintPtr(obj) + offset);
#ifndef NDEBUG
    if constexpr (G_USE_STICKY_CMS_GC) {
        CheckObjectForCMS(thread, const_cast<void *>(obj), offset, value);
    }
#endif
    ASSERT(thread != nullptr);
    if (UNLIKELY(thread->NeedReadBarrier())) {
        return ReadBarrier(thread, obj, offset, value);
    }
    return value.GetRawData();
}

inline ARK_INLINE TemporaryJSTaggedValue Barriers::GetFromCompressedTaggedValue(
    const JSThread *thread, JSTaggedValue obj, size_t offset)
{
    TemporaryJSTaggedValue temporaryValue =
        CompressedJSTaggedValue(*reinterpret_cast<CompressedJSTaggedType *>(ToUintPtr(obj.GetTaggedObject()) + offset))
        .Decompress(obj);
    ASSERT(thread != nullptr);
    if (UNLIKELY(thread->NeedReadBarrier())) {
        return ReadBarrierForCompressed(thread, obj, offset, temporaryValue);
    }
    return temporaryValue;
}

inline ARK_INLINE JSTaggedType Barriers::GetTaggedValue(const JSThread *thread, uintptr_t slotAddress)
{
    JSTaggedValue value = *reinterpret_cast<JSTaggedValue *>(slotAddress);
#ifndef NDEBUG
    if constexpr (G_USE_STICKY_CMS_GC) {
        CheckValueForCMS(thread, value);
    }
#endif
    ASSERT(thread != nullptr);
    if (UNLIKELY(thread->NeedReadBarrier())) {
        return ReadBarrier(thread, slotAddress, value);
    }
    return value.GetRawData();
}

inline ARK_INLINE TemporaryJSTaggedValue Barriers::GetFromCompressedTaggedValue(
    const JSThread *thread, uintptr_t slotAddress)
{
    TemporaryJSTaggedValue temporaryValue =
        CompressedJSTaggedValue(*reinterpret_cast<CompressedJSTaggedType *>(slotAddress)).Decompress(slotAddress);
    ASSERT(thread != nullptr);
    if (UNLIKELY(thread->NeedReadBarrier())) {
        return ReadBarrierForCompressed(thread, slotAddress, temporaryValue);
    }
    return temporaryValue;
}

inline ARK_INLINE JSTaggedType Barriers::GetTaggedValueAtomic(const JSThread *thread, const void *obj, size_t offset)
{
    JSTaggedValue value =  reinterpret_cast<volatile std::atomic<JSTaggedValue> *>(ToUintPtr(obj) +
        offset)->load(std::memory_order_acquire);
#ifndef NDEBUG
    if constexpr (G_USE_STICKY_CMS_GC) {
        CheckObjectForCMS(thread, const_cast<void *>(obj), offset, value);
    }
#endif
    ASSERT(thread != nullptr);
    if (UNLIKELY(thread->NeedReadBarrier())) {
        return AtomicReadBarrier(thread, obj, offset, value);
    }
    return value.GetRawData();
}

template <RBMode mode>
inline ARK_INLINE JSTaggedType Barriers::GetTaggedValue(const JSThread *thread, const void *obj, size_t offset)
{
    JSTaggedValue value = *reinterpret_cast<JSTaggedValue *>(ToUintPtr(obj) + offset);
#ifndef NDEBUG
    if (G_USE_STICKY_CMS_GC && thread != nullptr) {
        CheckObjectForCMS(thread, const_cast<void *>(obj), offset, value);
    }
#endif
    if constexpr (mode == RBMode::DEFAULT_RB) {
        ASSERT(thread != nullptr);
        if (UNLIKELY(thread->NeedReadBarrier())) {
            return ReadBarrier(thread, obj, offset, value);
        }
    } else if constexpr (mode == RBMode::FAST_CMC_RB) {
        return ReadBarrier(thread, obj, offset, value);
    }

    return value.GetRawData();
}

template <RBMode mode>
inline ARK_INLINE TemporaryJSTaggedValue Barriers::GetFromCompressedTaggedValue(
    const JSThread *thread, JSTaggedValue obj, size_t offset)
{
    TemporaryJSTaggedValue temporaryValue =
        CompressedJSTaggedValue(*reinterpret_cast<CompressedJSTaggedType *>(ToUintPtr(obj.GetTaggedObject()) + offset))
        .Decompress(obj);
    if constexpr (mode == RBMode::DEFAULT_RB) {
        ASSERT(thread != nullptr);
        if (UNLIKELY(thread->NeedReadBarrier())) {
            return ReadBarrierForCompressed(thread, obj, offset, temporaryValue);
        }
    } else if constexpr (mode == RBMode::FAST_CMC_RB) {
        return ReadBarrierForCompressed(thread, obj, offset, temporaryValue);
    }

    return temporaryValue;
}

template <RBMode mode>
inline ARK_INLINE JSTaggedType Barriers::GetTaggedValue(const JSThread *thread, uintptr_t slotAddress)
{
    JSTaggedValue value = *reinterpret_cast<JSTaggedValue *>(slotAddress);
#ifndef NDEBUG
    if (G_USE_STICKY_CMS_GC && thread != nullptr) {
        CheckValueForCMS(thread, value);
    }
#endif
    if constexpr (mode == RBMode::DEFAULT_RB) {
        ASSERT(thread != nullptr);
        if (UNLIKELY(thread->NeedReadBarrier())) {
            return ReadBarrier(thread, slotAddress, value);
        }
    } else if constexpr (mode == RBMode::FAST_CMC_RB) {
        return ReadBarrier(thread, slotAddress, value);
    }

    return value.GetRawData();
}

template <RBMode mode>
inline ARK_INLINE JSTaggedType Barriers::GetTaggedValueAtomic(const JSThread *thread, const void *obj, size_t offset)
{
    JSTaggedValue value =  reinterpret_cast<volatile std::atomic<JSTaggedValue> *>(ToUintPtr(obj) +
        offset)->load(std::memory_order_acquire);
#ifndef NDEBUG
    if (G_USE_STICKY_CMS_GC && thread != nullptr) {
        CheckObjectForCMS(thread, const_cast<void *>(obj), offset, value);
    }
#endif
    if constexpr (mode == RBMode::DEFAULT_RB) {
        ASSERT(thread != nullptr);
        if (UNLIKELY(thread->NeedReadBarrier())) {
            return AtomicReadBarrier(thread, obj, offset, value);
        }
    } else if constexpr (mode == RBMode::FAST_CMC_RB) {
        return AtomicReadBarrier(thread, obj, offset, value);
    }

    return value.GetRawData();
}

inline ARK_INLINE TaggedObject* Barriers::GetTaggedObject(const JSThread *thread, const void* obj, size_t offset)
{
    return JSTaggedValue(GetTaggedValue(thread, obj, offset)).GetTaggedObject();
}

template <RBMode mode>
inline ARK_INLINE TaggedObject* Barriers::GetTaggedObject(const JSThread *thread, const void *obj, size_t offset)
{
    return JSTaggedValue(GetTaggedValue<mode>(thread, obj, offset)).GetTaggedObject();
}

inline ARK_INLINE JSTaggedType Barriers::UpdateSlot(const JSThread *thread, void *obj, size_t offset)
{
    JSTaggedType value = GetTaggedValue(thread, obj, offset);
    *reinterpret_cast<JSTaggedType *>(ToUintPtr(obj) + offset) = value;
    return value;
}

inline ARK_INLINE JSTaggedType Barriers::UpdateSlot(const JSThread *thread, uintptr_t slotAddress)
{
    JSTaggedType value = GetTaggedValue(thread, slotAddress);
    *reinterpret_cast<JSTaggedType *>(slotAddress) = value;
    return value;
}
} // namespace panda::ecmascript

#endif // ECMASCRIPT_MEM_LOCAL_CMC_BARRIERS_GET_INL_H
