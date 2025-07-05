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

#ifndef ECMASCRIPT_MEM_BARRIERS_GET_INL_H
#define ECMASCRIPT_MEM_BARRIERS_GET_INL_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/js_thread.h"
#include "ecmascript/mem/barriers.h"

namespace panda::ecmascript {
static ARK_INLINE JSTaggedType ReadBarrier(const JSThread *thread, const void *obj, size_t offset,
                                           const JSTaggedValue &value)
{
    if (value.IsHeapObject()) {
#ifdef ENABLE_CMC_RB_DFX
        JSTaggedValue valueRB(reinterpret_cast<JSTaggedType>(
            common::BaseRuntime::ReadBarrier(const_cast<void*>(obj), (void*) (ToUintPtr(obj) + offset))));
        valueRB.RemoveReadBarrierDFXTag();
        return valueRB.GetRawData();
#else
        return reinterpret_cast<JSTaggedType>(
            common::BaseRuntime::ReadBarrier(const_cast<void *>(obj), (void *)(ToUintPtr(obj) + offset)));
#endif
    }
    return value.GetRawData();
}

static ARK_INLINE JSTaggedType ReadBarrier(const JSThread *thread, uintptr_t slotAddress, const JSTaggedValue &value)
{
    if (value.IsHeapObject()) {
#ifdef ENABLE_CMC_RB_DFX
        JSTaggedValue valueRB(
            reinterpret_cast<JSTaggedType>(common::BaseRuntime::ReadBarrier((void *)slotAddress)));
        valueRB.RemoveReadBarrierDFXTag();
        return valueRB.GetRawData();
#else
        return reinterpret_cast<JSTaggedType>(common::BaseRuntime::ReadBarrier((void*)slotAddress));
#endif
    }
    return value.GetRawData();
}

static ARK_INLINE JSTaggedType AtomicReadBarrier(const JSThread *thread, const void *obj, size_t offset,
                                                 const JSTaggedValue &value)
{
    if (value.IsHeapObject()) {
#ifdef ENABLE_CMC_RB_DFX
        JSTaggedValue value(reinterpret_cast<JSTaggedType>(common::BaseRuntime::AtomicReadBarrier(
            const_cast<void*>(obj), (void*) (ToUintPtr(obj) + offset), std::memory_order_acquire)));
        value.RemoveReadBarrierDFXTag();
        return value.GetRawData();
#else
        return reinterpret_cast<JSTaggedType>(common::BaseRuntime::AtomicReadBarrier(
            const_cast<void *>(obj), (void *)(ToUintPtr(obj) + offset), std::memory_order_acquire));
#endif
    }
    return value.GetRawData();
}

inline ARK_INLINE JSTaggedType Barriers::GetTaggedValue(const JSThread *thread, const void *obj, size_t offset)
{
    JSTaggedValue value = *reinterpret_cast<JSTaggedValue *>(ToUintPtr(obj) + offset);
    ASSERT(thread != nullptr);
    if (UNLIKELY(thread->NeedReadBarrier())) {
        return ReadBarrier(thread, obj, offset, value);
    }
    return value.GetRawData();
}

inline ARK_INLINE JSTaggedType Barriers::GetTaggedValue(const JSThread *thread, uintptr_t slotAddress)
{
    JSTaggedValue value = *reinterpret_cast<JSTaggedValue *>(slotAddress);
    ASSERT(thread != nullptr);
    if (UNLIKELY(thread->NeedReadBarrier())) {
        return ReadBarrier(thread, slotAddress, value);
    }
    return value.GetRawData();
}

inline ARK_INLINE JSTaggedType Barriers::GetTaggedValueAtomic(const JSThread *thread, const void *obj, size_t offset)
{
    JSTaggedValue value =  reinterpret_cast<volatile std::atomic<JSTaggedValue> *>(ToUintPtr(obj) +
        offset)->load(std::memory_order_acquire);
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
inline ARK_INLINE JSTaggedType Barriers::GetTaggedValue(const JSThread *thread, uintptr_t slotAddress)
{
    JSTaggedValue value = *reinterpret_cast<JSTaggedValue *>(slotAddress);
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
} // namespace panda::ecmascript

#endif // ECMASCRIPT_MEM_BARRIERS_GET_INL_H
