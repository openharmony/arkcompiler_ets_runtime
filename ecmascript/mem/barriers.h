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
#include "libpandabase/mem/mem.h"
#ifdef USE_CMC_GC
#include "common_interfaces/base_runtime.h"
#endif

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
    template<bool needWriteBarrier, bool maybeOverlap>
    static void CopyObject(const JSThread *thread, const TaggedObject *dstObj, JSTaggedValue *dstAddr,
                           const JSTaggedValue *srcAddr, size_t count);

    // dstAddr/srcAddr is the address will be copied to/from.
    // It can be a derived pointer point to the middle of an object.
    //
    // Note: dstObj is the object address for dstAddr, it must point to the head of an object.
    template<bool maybeOverlap>
    static void CopyObjectPrimitive(JSTaggedValue* dst, const JSTaggedValue* src, size_t count);
    static void SynchronizedSetObject(const JSThread *thread, void *obj, size_t offset, JSTaggedType value,
                                      bool isPrimitive = false);

    template<class T>
    static inline T GetValue(const void *obj, size_t offset)
    {
        auto *addr = reinterpret_cast<T *>(ToUintPtr(obj) + offset);
        return *addr;
    }

    static inline JSTaggedType GetTaggedValue(const void *obj, size_t offset)
    {
#ifdef USE_READ_BARRIER
        JSTaggedValue value = *reinterpret_cast<JSTaggedValue *>(ToUintPtr(obj) + offset);
        if (value.IsHeapObject()) {
            return reinterpret_cast<JSTaggedType>(BaseRuntime::ReadBarrier(const_cast<void*>(obj),
                                                                           (void*)(ToUintPtr(obj) + offset)));
        }
        return value.GetRawData();
#else
        return *reinterpret_cast<JSTaggedType *>(ToUintPtr(obj) + offset);
#endif
    }

    static inline JSTaggedType GetTaggedValue(uintptr_t slotAddress)
    {
#ifdef USE_READ_BARRIER
        JSTaggedValue value = *reinterpret_cast<JSTaggedValue *>(slotAddress);
        if (value.IsHeapObject()) {
            return reinterpret_cast<JSTaggedType>(BaseRuntime::ReadBarrier((void*)slotAddress));
        }
        return value.GetRawData();
#else
        return *reinterpret_cast<JSTaggedType *>(slotAddress);
#endif
    }

    static inline JSTaggedType GetTaggedValueAtomic(const void *obj, size_t offset)
    {
#ifdef USE_READ_BARRIER
        JSTaggedValue value =  reinterpret_cast<volatile std::atomic<JSTaggedValue> *>(ToUintPtr(obj) +
            offset)->load(std::memory_order_acquire);
        if (value.IsHeapObject()) {
            return reinterpret_cast<JSTaggedType>(
                BaseRuntime::AtomicReadBarrier(const_cast<void*>(obj), (void*)(ToUintPtr(obj) + offset),
                                               std::memory_order_acquire));
        }
        return value.GetRawData();
#else
        return reinterpret_cast<volatile std::atomic<JSTaggedType> *>(ToUintPtr(obj) + \
            offset)->load(std::memory_order_acquire);
#endif
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
#ifdef USE_CMC_GC
    static void PUBLIC_API CMCWriteBarrier(const JSThread *thread, void *obj, size_t offset, JSTaggedType value);
    static void PUBLIC_API CMCArrayCopyWriteBarrier(const JSThread *thread, void* src, void* dst, size_t count);
#endif
};
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_BARRIERS_H
