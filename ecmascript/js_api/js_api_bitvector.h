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

#ifndef ECMASCRIPT_JS_API_JS_API_BITVECTOR_H
#define ECMASCRIPT_JS_API_JS_API_BITVECTOR_H

#include "ecmascript/js_object.h"
#include "ecmascript/js_tagged_value-inl.h"

namespace panda::ecmascript {
class JSAPIBitVector : public JSObject {
public:
    static constexpr int32_t BIT_SET_LENGTH = 64;
    static constexpr int32_t TAGGED_VALUE_BIT_SIZE = 6;
    static constexpr uint64_t TAGGED_VALUE_BIT_OFFSET = 0x3FULL;
    static JSAPIBitVector* Cast(TaggedObject* object)
    {
        ASSERT(JSTaggedValue(object).IsJSAPIBitVector());
        return static_cast<JSAPIBitVector*>(object);
    }

    static bool Push(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector, const JSHandle<JSTaggedValue>& value);
    static JSTaggedValue Pop(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector);

    static JSTaggedValue GetIteratorObj(JSThread* thread, const JSHandle<JSAPIBitVector>& obj);

    static JSTaggedValue SetBitsByRange(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector,
        const JSHandle<JSTaggedValue>& value, const JSHandle<JSTaggedValue>& value1,
        const JSHandle<JSTaggedValue>& value2);
    static JSTaggedValue GetBitsByRange(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector,
        const JSHandle<JSTaggedValue>& value1, const JSHandle<JSTaggedValue>& value2);
    static JSTaggedValue SetAllBits(
        JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector, const JSHandle<JSTaggedValue>& value);
    static void Resize(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector, int capacity);
    static JSTaggedValue GetBitCountByRange(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector,
        const JSHandle<JSTaggedValue>& value, const JSHandle<JSTaggedValue>& value1,
        const JSHandle<JSTaggedValue>& value2);
    static int GetIndexOf(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector,
        const JSHandle<JSTaggedValue>& value, const JSHandle<JSTaggedValue>& value1,
        const JSHandle<JSTaggedValue>& value2);
    static int GetLastIndexOf(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector,
        const JSHandle<JSTaggedValue>& value, const JSHandle<JSTaggedValue>& value1,
        const JSHandle<JSTaggedValue>& value2);
    static JSTaggedValue FlipBitByIndex(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector, const int index);
    static JSTaggedValue FlipBitsByRange(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector,
        const JSHandle<JSTaggedValue>& value1, const JSHandle<JSTaggedValue>& value2);
    JSTaggedValue PUBLIC_API Set(JSThread* thread, const uint32_t index, JSTaggedValue value);
    JSTaggedValue Get(JSThread* thread, const uint32_t index);
    bool Has(const JSTaggedValue& value) const;
    static bool Has(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector, const JSHandle<JSTaggedValue>& value,
        const JSHandle<JSTaggedValue>& value1, const JSHandle<JSTaggedValue>& value2);
    static bool Include(JSThread* thread, const JSHandle<JSAPIBitVector>& bitVector,
        const JSHandle<JSTaggedValue>& value, const JSHandle<JSTaggedValue>& value1,
        const JSHandle<JSTaggedValue>& value2);

    static JSHandle<TaggedArray> OwnKeys(JSThread* thread, const JSHandle<JSAPIBitVector>& obj);
    static JSHandle<TaggedArray> OwnEnumKeys(JSThread* thread, const JSHandle<JSAPIBitVector>& obj);
    static bool GetOwnProperty(
        JSThread* thread, const JSHandle<JSAPIBitVector>& obj, const JSHandle<JSTaggedValue>& key);
    static OperationResult GetProperty(
        JSThread* thread, const JSHandle<JSAPIBitVector>& obj, const JSHandle<JSTaggedValue>& key);
    static bool SetProperty(JSThread* thread, const JSHandle<JSAPIBitVector>& obj, const JSHandle<JSTaggedValue>& key,
        const JSHandle<JSTaggedValue>& value);
    inline uint32_t GetSize() const
    {
        return GetLength();
    }

    static constexpr size_t NATIVE_POINTER_OFFSET = JSObject::SIZE;
    ACCESSORS(NativePointer, NATIVE_POINTER_OFFSET, LENGTH_OFFSET);
    ACCESSORS_PRIMITIVE_FIELD(Length, int32_t, LENGTH_OFFSET, MOD_RECORD_OFFSET);
    ACCESSORS_SYNCHRONIZED_PRIMITIVE_FIELD(ModRecord, uint32_t, MOD_RECORD_OFFSET, LAST_OFFSET);
    DEFINE_ALIGN_SIZE(LAST_OFFSET);
    
    static constexpr uint32_t MAX_INLINE = PropertyAttributes::MAX_FAST_PROPS_CAPACITY -
        SIZE / JSTaggedValue::TaggedTypeSize() + 1;
    DECL_VISIT_OBJECT_FOR_JS_OBJECT(JSObject, NATIVE_POINTER_OFFSET, LENGTH_OFFSET);
    DECL_DUMP()

private:
    inline static uint32_t ComputeCapacity(uint32_t oldCapacity)
    {
        uint32_t newCapacity = oldCapacity + (oldCapacity >> 1U);
        return newCapacity > 0 ? newCapacity : 0;
    }
    inline static std::pair<uint32_t, uint32_t> ComputeElementIdAndBitId(uint32_t index)
    {
        uint32_t elementId = index >> TAGGED_VALUE_BIT_SIZE;
        uint32_t bitId = index & TAGGED_VALUE_BIT_OFFSET;
        return std::make_pair(elementId, bitId);
    }
    inline static void SetBit(std::vector<std::bitset<BIT_SET_LENGTH>> *elements, uint32_t index,
                              const JSTaggedValue &value)
    {
        std::pair<uint32_t, uint32_t> pair = ComputeElementIdAndBitId(index);
        uint32_t elementId = pair.first;
        uint32_t bitId = pair.second;
        if (value.IsZero()) {
            (*elements)[elementId].reset(bitId);
        } else {
            (*elements)[elementId].set(bitId);
        }
        return;
    }
    inline static JSTaggedValue GetBit(std::vector<std::bitset<BIT_SET_LENGTH>> *elements, uint32_t index)
    {
        std::pair<uint32_t, uint32_t> pair = ComputeElementIdAndBitId(index);
        uint32_t elementId = pair.first;
        uint32_t bitId = pair.second;
        int32_t bit = (*elements)[elementId].test(bitId);
        return JSTaggedValue(bit);
    }
};
} // namespace panda::ecmascript

#endif // ECMASCRIPT_JS_API_JS_API_BITVECTOR_H
