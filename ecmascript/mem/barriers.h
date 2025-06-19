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

#ifndef ECMASCRIPT_MEM_BARRIERS_H
#define ECMASCRIPT_MEM_BARRIERS_H

#include "ecmascript/common.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/mark_word.h"
#include "ecmascript/mem/mem_common.h"
#include "common_interfaces/base_runtime.h"

namespace panda::ecmascript {
class Region;

enum class WriteBarrierType : size_t { NORMAL, DESERIALIZE, AOT_DESERIALIZE };

class Barriers {
public:
    template<class T>
    static inline void SetPrimitive(void *obj, size_t offset, T value)
    {
        auto *addr = reinterpret_cast<T *>(ToUintPtr(obj) + offset);
        // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
        *addr = value;
    }

#ifdef ARK_USE_SATB_BARRIER
    template<>
    inline void SetPrimitive(void *obj, size_t offset, JSTaggedType value)
    {
        // NOT USE IN Concrruent Enum Barrier
        CMCWriteBarrier(nullptr, obj, offset, JSTaggedValue::Hole().GetRawData());
        auto *addr = reinterpret_cast<JSTaggedType *>(ToUintPtr(obj) + offset);
        // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
        *addr = value;
        // thread not be used, and satb barrier don't need newvalue.
    }
#endif

    template<class T>
    static inline T AtomicSetPrimitive(volatile void *obj, size_t offset, T oldValue, T value)
    {
        volatile auto atomicField = reinterpret_cast<volatile std::atomic<T> *>(ToUintPtr(obj) + offset);
        std::atomic_compare_exchange_strong_explicit(atomicField, &oldValue, value, std::memory_order_release,
                                                     std::memory_order_relaxed);
        return oldValue;
    }

    template<bool needWriteBarrier = true>
    static void SetObject(const JSThread *thread, void *obj, size_t offset, JSTaggedType value);

    // dstAddr/srcAddr is the address will be copied to/from.
    // It can be a derived pointer point to the middle of an object.
    //
    // Note: dstObj is the object address for dstAddr, it must point to the head of an object.
    template <bool needWriteBarrier, bool maybeOverlap>
    static void CopyObject(const JSThread *thread, const TaggedObject *dstObj, JSTaggedValue *dstAddr,
                           const JSTaggedValue *srcAddr, size_t count);

    // dstAddr/srcAddr is the address will be copied to/from.
    // It can be a derived pointer point to the middle of an object.
    //
    // Note: dstObj is the object address for dstAddr, it must point to the head of an object.
    template<bool needReadBarrier, bool maybeOverlap>
    static void CopyObjectPrimitive(JSTaggedValue* dst, const JSTaggedValue* src, size_t count);
    static void SynchronizedSetObject(const JSThread *thread, void *obj, size_t offset, JSTaggedType value,
                                      bool isPrimitive = false);

    template<class T>
    static inline T GetValue(const void *obj, size_t offset)
    {
        auto *addr = reinterpret_cast<T *>(ToUintPtr(obj) + offset);
        return *addr;
    }

    static inline ARK_INLINE TaggedObject* GetTaggedObject(const void* obj, size_t offset)
    {
        return JSTaggedValue(GetTaggedValue(obj, offset)).GetTaggedObject();
    }

    static inline JSTaggedType GetTaggedValue(const void *obj, size_t offset)
    {
        JSTaggedValue value = *reinterpret_cast<JSTaggedValue *>(ToUintPtr(obj) + offset);
        if (UNLIKELY(g_isEnableCMCGC)) {
            return GetTaggedValueForRB(obj, offset, value);
        }

        return value.GetRawData();
    }

    static inline JSTaggedType GetTaggedValue(uintptr_t slotAddress)
    {
        JSTaggedValue value = *reinterpret_cast<JSTaggedValue *>(slotAddress);
        if (UNLIKELY(g_isEnableCMCGC)) {
            return GetTaggedValueForRB(slotAddress, value);
        }

        return value.GetRawData();
    }

    static inline JSTaggedType GetTaggedValueAtomic(const void *obj, size_t offset)
    {
        JSTaggedValue value =  reinterpret_cast<volatile std::atomic<JSTaggedValue> *>(ToUintPtr(obj) +
            offset)->load(std::memory_order_acquire);
        if (UNLIKELY(g_isEnableCMCGC)) {
            return GetTaggedValueAtomicForRB(obj, offset, value);
        }

        return value.GetRawData();
    }

    template <RBMode mode = RBMode::DEFAULT_RB>
    static inline ARK_INLINE TaggedObject* GetTaggedObject(const void* obj, size_t offset)
    {
        return JSTaggedValue(GetTaggedValue<mode>(obj, offset)).GetTaggedObject();
    }

    template <RBMode mode>
    static inline JSTaggedType GetTaggedValue(const void *obj, size_t offset)
    {
        JSTaggedValue value = *reinterpret_cast<JSTaggedValue *>(ToUintPtr(obj) + offset);
        if constexpr (mode == RBMode::DEFAULT_RB) {
            if (UNLIKELY(g_isEnableCMCGC)) {
                return GetTaggedValueForRB(obj, offset, value);
            }
        } else if constexpr (mode == RBMode::FAST_CMC_RB) {
            return GetTaggedValueForRB(obj, offset, value);
        }

        return value.GetRawData();
    }

    template <RBMode mode>
    static inline JSTaggedType GetTaggedValue(uintptr_t slotAddress)
    {
        JSTaggedValue value = *reinterpret_cast<JSTaggedValue *>(slotAddress);
        if constexpr (mode == RBMode::DEFAULT_RB) {
            if (UNLIKELY(g_isEnableCMCGC)) {
                return GetTaggedValueForRB(slotAddress, value);
            }
        } else if constexpr (mode == RBMode::FAST_CMC_RB) {
            return GetTaggedValueForRB(slotAddress, value);
        }

        return value.GetRawData();
    }

    template <RBMode mode>
    static inline JSTaggedType GetTaggedValueAtomic(const void *obj, size_t offset)
    {
        JSTaggedValue value =  reinterpret_cast<volatile std::atomic<JSTaggedValue> *>(ToUintPtr(obj) +
            offset)->load(std::memory_order_acquire);
        if constexpr (mode == RBMode::DEFAULT_RB) {
            if (UNLIKELY(g_isEnableCMCGC)) {
                return GetTaggedValueAtomicForRB(obj, offset, value);
            }
        } else if constexpr (mode == RBMode::FAST_CMC_RB) {
            return GetTaggedValueAtomicForRB(obj, offset, value);
        }

        return value.GetRawData();
    }

    static inline JSTaggedType UpdateSlot(void *obj, size_t offset)
    {
        JSTaggedType value = GetTaggedValue(obj, offset);
        *reinterpret_cast<JSTaggedType *>(ToUintPtr(obj) + offset) = value;
        return value;
    }

    static void PUBLIC_API Update(const JSThread *thread, uintptr_t slotAddr, Region *objectRegion,
                                  TaggedObject *value, Region *valueRegion,
                                  WriteBarrierType writeType = WriteBarrierType::NORMAL);

    static void PUBLIC_API UpdateShared(const JSThread *thread, uintptr_t slotAddr, Region *objectRegion,
                                        TaggedObject *value, Region *valueRegion);
    static void PUBLIC_API CMCWriteBarrier(const JSThread *thread, void *obj, size_t offset, JSTaggedType value);
    static void PUBLIC_API CMCArrayCopyWriteBarrier(const JSThread *thread, const TaggedObject *dstObj,
                                                        void* src, void* dst, size_t count);
private:
    static inline JSTaggedType GetTaggedValueForRB(const void *obj, size_t offset, const JSTaggedValue &value)
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

    static inline JSTaggedType GetTaggedValueForRB(uintptr_t slotAddress, const JSTaggedValue &value)
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

    static inline JSTaggedType GetTaggedValueAtomicForRB(const void *obj, size_t offset, const JSTaggedValue &value)
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
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_BARRIERS_H
