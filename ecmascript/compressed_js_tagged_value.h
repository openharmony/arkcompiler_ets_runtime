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

#ifndef ECMASCRIPT_COMPRESSED_JS_TAGGED_VALUE_H
#define ECMASCRIPT_COMPRESSED_JS_TAGGED_VALUE_H

#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/tagged_state_word.h"

namespace panda::ecmascript {
class CompressedJSTaggedValue;
// A temporary JSTaggedValue for decompressing CompressedJSTaggedValue
class TemporaryJSTaggedValue : public JSTaggedValueInternals {
public:
    static_assert(std::is_unsigned<JSTaggedType>::value);
    static_assert(std::is_unsigned<CompressedJSTaggedType>::value);
    static_assert(sizeof(JSTaggedType) >= sizeof(CompressedJSTaggedType));

    ARK_INLINE explicit constexpr TemporaryJSTaggedValue(JSTaggedType v) : value_{v}
    {
    }

    static ARK_INLINE constexpr TemporaryJSTaggedValue Hole();

    ARK_INLINE bool IsHole() const;

    ARK_INLINE bool IsHeapObject() const;

    ARK_INLINE bool IsInt() const
    {
        // fixme: compressed pointer : support int for compressed value
        return false;
    }

    ARK_INLINE bool IsWeakForHeapObject() const
    {
        ASSERT(IsHeapObject());
        // fixme: compressed pointer : support weak for compressed value
        return ConvertHeapObjectToJSTaggedValue().IsWeakForHeapObject();
    }
    
    ARK_INLINE bool IsInSharedSweepableSpace() const;

    // never use `GetRawDataUnsafe()` or anything else to get the raw `value` outside, only if you know the conversion
    // between `JSTaggedValue` and `CompressedJSTaggedValue`
    ARK_INLINE JSTaggedType GetRawDataUnsafe() const
    {
        return value_;
    }

    ARK_INLINE int GetInt() const
    {
        LOG_ECMA(FATAL) << "not support";
        UNREACHABLE();
    }

    ARK_INLINE JSTaggedValue ConvertHeapObjectToJSTaggedValue() const
    {
        JSTaggedValue res(value_);
        ASSERT(IsHeapObject());
        ASSERT(res.IsHeapObject());
        return res;
    }

    ARK_INLINE JSTaggedValue ConvertHoleToJSTaggedValue() const
    {
        ASSERT(IsHole());
        return JSTaggedValue::Hole();
    }

    ARK_INLINE JSTaggedValue ConvertIntToJSTaggedValue() const
    {
        // fixme: compressed pointer : support int
        LOG_ECMA(FATAL) << "not support";
        UNREACHABLE();
    }

    ARK_INLINE JSTaggedValue ConvertToJSTaggedValue() const
    {
        if (IsHeapObject()) {
            return ConvertHeapObjectToJSTaggedValue();
        }
        if (IsHole()) {
            return ConvertHoleToJSTaggedValue();
        }
        ASSERT(IsInt());
        return ConvertIntToJSTaggedValue();
    }

    ARK_INLINE TaggedObject *GetTaggedObject() const
    {
        ASSERT(IsHeapObject());
        return ConvertHeapObjectToJSTaggedValue().GetTaggedObject();
    }

    ARK_INLINE TaggedObject *GetHeapObject() const
    {
        ASSERT(IsHeapObject());
        // fixme: compressed pointer : support compressed weak
        ASSERT(!IsWeakForHeapObject());
        return GetTaggedObject();
    }

    //  This function returns the heap object pointer which may have the weak tag.
    ARK_INLINE TaggedObject *GetRawHeapObject() const
    {
        ASSERT(IsHeapObject());
        return ConvertHeapObjectToJSTaggedValue().GetRawHeapObject();
    }

    bool IsAOTLiteralInfo() const;
private:
    // never use `GetRawDataUnsafe()` or anything else to get the raw `value` outside, only if you know the conversion
    // between `JSTaggedValue` and `CompressedJSTaggedValue`
    JSTaggedType value_;

    friend class CompressedJSTaggedValue;
};

class CompressedJSTaggedValue : public CompressedJSTaggedValueInternals {
public:
    static_assert(std::is_unsigned<JSTaggedType>::value);
    static_assert(std::is_unsigned<CompressedJSTaggedType>::value);
    static_assert(sizeof(JSTaggedType) >= sizeof(CompressedJSTaggedType));

    static constexpr JSTaggedType RESERVED_LOW_BIT_MASK =
        static_cast<JSTaggedType>(~static_cast<CompressedJSTaggedType>(0ULL));
    static constexpr JSTaggedType COMPRESSED_HIGH_BIT_MASK =
        (~static_cast<JSTaggedType>(0ULL)) ^ RESERVED_LOW_BIT_MASK;

    static ARK_INLINE constexpr size_t CompressedTaggedTypeSize()
    {
        return sizeof(CompressedJSTaggedType);
    }

    static ARK_INLINE constexpr size_t CompressFactorToJSTaggedValue()
    {
        static_assert(JSTaggedValue::TaggedTypeSize() % CompressedTaggedTypeSize() == 0);
        return JSTaggedValue::TaggedTypeSize() / CompressedTaggedTypeSize();
    }

    static ARK_INLINE constexpr CompressedJSTaggedValue Hole()
    {
        return CompressedJSTaggedValue(COMPRESSED_VALUE_HOLE);
    }

    static ARK_INLINE size_t AlignUpSizeToJSTaggedValue(size_t size)
    {
        ASSERT(size % CompressedTaggedTypeSize() == 0);
        return AlignUp(size, JSTaggedValue::TaggedTypeSize());
    }

    ARK_INLINE explicit constexpr CompressedJSTaggedValue(CompressedJSTaggedType v) : value_(v)
    {
    }

    ARK_INLINE CompressedJSTaggedType GetCompressedRawData() const
    {
        return value_;
    }

    ARK_INLINE bool IsCompressedHeapObject() const
    {
        return ((value_ & TAG_COMPRESSED_HEAPOBJECT_MASK) == 0U);
    }

    ARK_INLINE bool IsCompressedHole() const
    {
        return value_ == COMPRESSED_VALUE_HOLE;
    }

    static ARK_INLINE CompressedJSTaggedValue Compress(JSTaggedValue value)
    {
        // fixme: compressed pointer : support int for compressed value
        ASSERT(value.IsHeapObject() || value.IsHole());
        JSTaggedType raw = value.GetRawData();
        ASSERT(!value.IsHeapObject() || TaggedStateWord::BASE_ADDRESS == 0 ||
               (raw & COMPRESSED_HIGH_BIT_MASK) == TaggedStateWord::BASE_ADDRESS);
        JSTaggedType lowRaw = raw & RESERVED_LOW_BIT_MASK;
        ASSERT((lowRaw & COMPRESSED_HIGH_BIT_MASK) == 0);
        return CompressedJSTaggedValue(static_cast<CompressedJSTaggedType>(lowRaw));
    }

    static ARK_INLINE CompressedJSTaggedValue Compress(TemporaryJSTaggedValue temporaryValue)
    {
        // fixme: compressed pointer : support int for compressed value
        ASSERT(temporaryValue.IsHeapObject() || temporaryValue.IsHole());
        // never use `GetRawDataUnsafe` or anything else to get the raw `value` in `TemporaryJSTaggedValue`,
        // only if you know the conversion between `JSTaggedValue` and `CompressedJSTaggedValue`
        JSTaggedType raw = temporaryValue.GetRawDataUnsafe();
        ASSERT(!temporaryValue.IsHeapObject() || TaggedStateWord::BASE_ADDRESS == 0 ||
               (raw & COMPRESSED_HIGH_BIT_MASK) == TaggedStateWord::BASE_ADDRESS);
        JSTaggedType lowRaw = raw & RESERVED_LOW_BIT_MASK;
        ASSERT((lowRaw & COMPRESSED_HIGH_BIT_MASK) == 0);
        return CompressedJSTaggedValue(static_cast<CompressedJSTaggedType>(lowRaw));
    }

    // `Hole` will be decompress to a object-like address, but not an object, `IsHeapObject()` returns false.
    // Do more processing after checking `IsHeapObject()`.
    ARK_INLINE TemporaryJSTaggedValue Decompress(JSTaggedValue helper)
    {
        return Decompress(static_cast<uintptr_t>(helper.GetRawData()));
    }

    // `Hole` will be decompress to a object-like address, but not an object, `IsHeapObject()` returns false.
    // Do more processing after checking `IsHeapObject()`.
    ARK_INLINE TemporaryJSTaggedValue Decompress(uintptr_t helper)
    {
        JSTaggedType highBit = static_cast<JSTaggedType>(helper) & COMPRESSED_HIGH_BIT_MASK;
        JSTaggedType lowBit = static_cast<JSTaggedType>(value_);
        ASSERT((lowBit & COMPRESSED_HIGH_BIT_MASK) == 0);
        return TemporaryJSTaggedValue(highBit | lowBit);
    }

private:
    CompressedJSTaggedType value_;
};

ARK_INLINE constexpr TemporaryJSTaggedValue TemporaryJSTaggedValue::Hole()
{
    return TemporaryJSTaggedValue(
        static_cast<JSTaggedType>(CompressedJSTaggedValue::Hole().GetCompressedRawData()));
}

inline ARK_INLINE bool TemporaryJSTaggedValue::IsHole() const
{
    return CompressedJSTaggedValue(static_cast<CompressedJSTaggedType>(value_)).IsCompressedHole();
}

inline ARK_INLINE bool TemporaryJSTaggedValue::IsHeapObject() const
{
    return CompressedJSTaggedValue(static_cast<CompressedJSTaggedType>(value_)).IsCompressedHeapObject();
}
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_COMPRESSED_JS_TAGGED_VALUE_H
