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

#ifndef ECMASCRIPT_MEM_SLOTS_H
#define ECMASCRIPT_MEM_SLOTS_H

#include "ecmascript/js_tagged_value_wrapper.h"
#include "ecmascript/mem/mem.h"

namespace panda::ecmascript {
enum class SlotStatus : bool {
    KEEP_SLOT,
    CLEAR_SLOT,
};

template <ReferenceType refType>
class ObjectSlotBase {
public:
    static constexpr bool IS_COMPRESSED_SLOT = ReferenceIsCompressed<refType>;
    static constexpr ReferenceType REFERENCE_TYPE = refType;
    using SlotType = ObjectSlotBase<refType>;
    using T = RawDataType<refType>;

    explicit ObjectSlotBase(uintptr_t slotAddr) : slotAddress_(slotAddr) {}
    ObjectSlotBase() : ObjectSlotBase(0) {}
    ~ObjectSlotBase() = default;

    DEFAULT_COPY_SEMANTIC(ObjectSlotBase);
    DEFAULT_MOVE_SEMANTIC(ObjectSlotBase);

    void Update(TaggedObject *header)
    {
        if constexpr (IS_COMPRESSED_SLOT) {
            Update(CompressedJSTaggedValue::Compress(JSTaggedValue(header)).GetCompressedRawData());
        } else {
            Update(static_cast<JSTaggedType>(ToUintPtr(header)));
        };
    }

    uintptr_t* GetRefFieldAddr()
    {
        return reinterpret_cast<uintptr_t*>(slotAddress_);
    }

    void Update(T value)
    {
        // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
        *reinterpret_cast<T *>(slotAddress_) = value;
    }

    void CASUpdate(TaggedObject *oldObject, TaggedObject *toRef)
    {
        auto convertValue = [](TaggedObject *object) {
            JSTaggedValue fullValue = JSTaggedValue(object);
            if constexpr (IS_COMPRESSED_SLOT) {
                return CompressedJSTaggedValue::Compress(fullValue).GetCompressedRawData();
            } else {
                return fullValue.GetRawData();
            }
        };
        T oldVal = convertValue(oldObject);
        T newVal = convertValue(toRef);
        std::atomic_compare_exchange_strong_explicit(
            reinterpret_cast<volatile std::atomic<T> *>(slotAddress_),
            &oldVal, newVal, std::memory_order_relaxed, std::memory_order_relaxed);
    }

    // compressed value do not support weak now
    template <typename Dummy = void,
              typename = std::enable_if_t<!IS_COMPRESSED_SLOT, Dummy>>
    void UpdateWeak(uintptr_t value)
    {
        // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
        *reinterpret_cast<JSTaggedType *>(slotAddress_) = value | JSTaggedValue::TAG_WEAK;
    }

    // compressed value do not support weak now
    template <typename Dummy = void,
              typename = std::enable_if_t<!IS_COMPRESSED_SLOT, Dummy>>
    void CASUpdateWeak(TaggedObject *oldObject, TaggedObject *toRef)
    {
        TaggedObject *dst = JSTaggedValue::Cast(toRef).CreateAndGetWeakRef().GetRawHeapObject();
        std::atomic_compare_exchange_strong_explicit(
            reinterpret_cast<volatile std::atomic<TaggedObject*> *>(slotAddress_),
            &oldObject, dst,
            std::memory_order_relaxed, std::memory_order_relaxed);
    }

    // compressed value do not support weak now
    template <typename Dummy = void,
              typename = std::enable_if_t<!IS_COMPRESSED_SLOT, Dummy>>
    void Clear()
    {
        *reinterpret_cast<JSTaggedType *>(slotAddress_) = JSTaggedValue::VALUE_UNDEFINED;
    }

    template <typename Dummy = void,
              typename = std::enable_if_t<!IS_COMPRESSED_SLOT, Dummy>>
    JSTaggedType GetTaggedType() const
    {
        // NOLINTNEXTLINE(clang-analyzer-core.NullDereference)
        return *reinterpret_cast<JSTaggedType *>(slotAddress_);
    }

    TaggedValueType<refType> GetTaggedValue() const
    {
        if constexpr (IS_COMPRESSED_SLOT) {
            CompressedJSTaggedType rawData = *reinterpret_cast<CompressedJSTaggedType *>(slotAddress_);
            return CompressedJSTaggedValue(rawData).Decompress(slotAddress_);
        } else {
            return JSTaggedValue(GetTaggedType());
        }
    }

    T GetRawValueForTest() const
    {
        return *reinterpret_cast<T *>(slotAddress_);
    }

    SlotType &operator++()
    {
        slotAddress_ += sizeof(T);
        return *this;
    }

    // NOLINTNEXTLINE(cert-dcl21-cpp)
    SlotType operator++(int)
    {
        SlotType ret = *this;
        slotAddress_ += sizeof(T);
        return ret;
    }

    SlotType operator+=(size_t length)
    {
        SlotType ret = *this;
        slotAddress_ += sizeof(T) * length;
        return ret;
    }

    uintptr_t SlotAddress() const
    {
        return slotAddress_;
    }

    bool operator<(const SlotType &other) const
    {
        return slotAddress_ < other.slotAddress_;
    }
    bool operator<=(const SlotType &other) const
    {
        return slotAddress_ <= other.slotAddress_;
    }
    bool operator>(const SlotType &other) const
    {
        return slotAddress_ > other.slotAddress_;
    }
    bool operator>=(const SlotType &other) const
    {
        return slotAddress_ >= other.slotAddress_;
    }
    bool operator==(const SlotType &other) const
    {
        return slotAddress_ == other.slotAddress_;
    }
    bool operator!=(const SlotType &other) const
    {
        return slotAddress_ != other.slotAddress_;
    }

private:
    uintptr_t slotAddress_;
};

using ObjectSlot = ObjectSlotBase<ReferenceType::NORMAL>;
using CompressedObjectSlot = ObjectSlotBase<ReferenceType::COMPRESSED>;
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_SLOTS_H
