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

#ifndef ECMASCRIPT_STRING_H
#define ECMASCRIPT_STRING_H

#include <cstddef>
#include <cstdint>
#include <cstring>

#include "ecmascript/base/utf_helper.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_tagged_value.h"
#include "ecmascript/mem/barriers.h"
#include "ecmascript/mem/space.h"
#include "ecmascript/mem/tagged_object.h"

#include "libpandabase/macros.h"
#include "securec.h"
#include "unicode/locid.h"

namespace panda {
namespace ecmascript {
template<typename T>
class JSHandle;
class EcmaVM;
class EcmaString : public TaggedObject {
public:
    friend class EcmaStringAccessor;

    CAST_CHECK(EcmaString, IsString);

    static constexpr uint32_t STRING_COMPRESSED_BIT = 0x1;
    static constexpr uint32_t STRING_INTERN_BIT = 0x2;

    static constexpr size_t MIX_LENGTH_OFFSET = TaggedObjectSize();
    // In last bit of mix_length we store if this string is compressed or not.
    ACCESSORS_PRIMITIVE_FIELD(MixLength, uint32_t, MIX_LENGTH_OFFSET, HASHCODE_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(RawHashcode, uint32_t, HASHCODE_OFFSET, SIZE)
    // DATA_OFFSET: the string data stored after the string header.
    // Data can be stored in utf8 or utf16 form according to compressed bit.
    static constexpr size_t DATA_OFFSET = SIZE;  // DATA_OFFSET equal to Empty String size

    enum CompressedStatus {
        STRING_COMPRESSED,
        STRING_UNCOMPRESSED,
    };

    enum TrimMode : uint8_t {
        TRIM,
        TRIM_START,
        TRIM_END,
    };

private:
    static EcmaString *CreateEmptyString(const EcmaVM *vm);
    static EcmaString *CreateFromUtf8(const EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                                      MemSpaceType type = MemSpaceType::SEMI_SPACE);
    static EcmaString *CreateFromUtf16(const EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len,
                                       bool canBeCompress, MemSpaceType type = MemSpaceType::SEMI_SPACE);
    static EcmaString *Concat(const EcmaVM *vm,
        const JSHandle<EcmaString> &str1Handle, const JSHandle<EcmaString> &str2Handle);
    static EcmaString *FastSubString(const EcmaVM *vm,
        const JSHandle<EcmaString> &src, uint32_t start, uint32_t length);

    template<bool verify = true>
    uint16_t At(int32_t index) const;

    static int32_t Compare(EcmaString *lhs, EcmaString *rhs);

    bool IsUtf16() const
    {
        return (GetMixLength() & STRING_COMPRESSED_BIT) == STRING_UNCOMPRESSED;
    }

    bool IsUtf8() const
    {
        return (GetMixLength() & STRING_COMPRESSED_BIT) == STRING_COMPRESSED;
    }

    static size_t ComputeDataSizeUtf16(uint32_t length)
    {
        return length * sizeof(uint16_t);
    }

    /**
     * Methods for uncompressed strings (UTF16):
     */
    static size_t ComputeSizeUtf16(uint32_t utf16Len)
    {
        return DATA_OFFSET + ComputeDataSizeUtf16(utf16Len);
    }

    inline uint16_t *GetData() const
    {
        return reinterpret_cast<uint16_t *>(ToUintPtr(this) + DATA_OFFSET);
    }

    const uint16_t *GetDataUtf16() const
    {
        LOG_ECMA_IF(!IsUtf16(), FATAL) << "EcmaString: Read data as utf16 for utf8 string";
        return GetData();
    }

    /**
     * Methods for compresses strings (UTF8 or LATIN1):
     */
    static size_t ComputeSizeUtf8(uint32_t utf8Len)
    {
        return DATA_OFFSET + utf8Len;
    }

    /**
     * It's Utf8 format, but without 0 in the end.
     */
    const uint8_t *GetDataUtf8() const
    {
        ASSERT_PRINT(IsUtf8(), "EcmaString: Read data as utf8 for utf16 string");
        return reinterpret_cast<uint8_t *>(GetData());
    }

    size_t GetUtf8Length(bool modify = true) const
    {
        if (!IsUtf16()) {
            return GetLength() + 1;  // add place for zero in the end
        }
        return base::utf_helper::Utf16ToUtf8Size(GetData(), GetLength(), modify);
    }

    size_t GetUtf16Length() const
    {
        return GetLength();
    }

    inline size_t CopyDataUtf8(uint8_t *buf, size_t maxLength, bool modify = true) const
    {
        if (maxLength == 0) {
            return 1; // maxLength was -1 at napi
        }
        size_t length = GetLength();
        if (length > maxLength) {
            return 0;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        buf[maxLength - 1] = '\0';
        // Put comparison here so that internal usage and napi can use the same CopyDataRegionUtf8
        return CopyDataRegionUtf8(buf, 0, length, maxLength, modify) + 1;  // add place for zero in the end
    }

    // It allows user to copy into buffer even if maxLength < length
    inline size_t WriteUtf8(uint8_t *buf, size_t maxLength) const
    {
        if (maxLength == 0) {
            return 1; // maxLength was -1 at napi
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        buf[maxLength - 1] = '\0';
        return CopyDataRegionUtf8(buf, 0, GetLength(), maxLength) + 1;  // add place for zero in the end
    }

    size_t CopyDataRegionUtf8(uint8_t *buf, size_t start, size_t length, size_t maxLength, bool modify = true) const
    {
        uint32_t len = GetLength();
        if (start + length > len) {
            return 0;
        }
        if (!IsUtf16()) {
            if (length > std::numeric_limits<size_t>::max() / 2 - 1) {  // 2: half
                LOG_FULL(FATAL) << " length is higher than half of size_t::max";
                UNREACHABLE();
            }
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            // Only memcpy_s maxLength number of chars into buffer if length > maxLength
            if (length > maxLength) {
                if (memcpy_s(buf, maxLength, GetDataUtf8() + start, maxLength) != EOK) {
                    LOG_FULL(FATAL) << "memcpy_s failed when length > maxlength";
                    UNREACHABLE();
                }
                return maxLength;
            }
            if (memcpy_s(buf, maxLength, GetDataUtf8() + start, length) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed when length <= maxlength";
                UNREACHABLE();
            }
            return length;
        }
        if (length > maxLength) {
            return base::utf_helper::ConvertRegionUtf16ToUtf8(GetDataUtf16(), buf, maxLength, maxLength, start, modify);
        }
        return base::utf_helper::ConvertRegionUtf16ToUtf8(GetDataUtf16(), buf, length, maxLength, start, modify);
    }

    inline uint32_t CopyDataUtf16(uint16_t *buf, uint32_t maxLength) const
    {
        return CopyDataRegionUtf16(buf, 0, GetLength(), maxLength);
    }

    uint32_t CopyDataRegionUtf16(uint16_t *buf, uint32_t start, uint32_t length, uint32_t maxLength) const
    {
        if (length > maxLength) {
            return 0;
        }
        uint32_t len = GetLength();
        if (start + length > len) {
            return 0;
        }
        if (IsUtf16()) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (memcpy_s(buf, ComputeDataSizeUtf16(maxLength), GetDataUtf16() + start, ComputeDataSizeUtf16(length)) !=
                EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            return length;
        }
        return base::utf_helper::ConvertRegionUtf8ToUtf16(GetDataUtf8(), buf, len, maxLength, start);
    }

    std::u16string ToU16String(uint32_t len = 0);

    std::unique_ptr<uint8_t[]> ToOneByteDataForced()
    {
        uint8_t *buf = nullptr;
        auto length = GetLength();
        if (IsUtf16()) {
            auto size = ComputeDataSizeUtf16(length);
            buf = new uint8_t[size]();
            CopyDataUtf16(reinterpret_cast<uint16_t *>(buf), length);
        } else {
            buf = new uint8_t[length + 1]();
            CopyDataUtf8(buf, length + 1);
        }
        return std::unique_ptr<uint8_t[]>(buf);
    }

    Span<const uint8_t> ToUtf8Span([[maybe_unused]] CVector<uint8_t> &buf, bool modify = true)
    {
        Span<const uint8_t> str;
        uint32_t strLen = GetLength();
        if (UNLIKELY(IsUtf16())) {
            size_t len = base::utf_helper::Utf16ToUtf8Size(GetDataUtf16(), strLen, modify) - 1;
            buf.reserve(len);
            len = base::utf_helper::ConvertRegionUtf16ToUtf8(GetDataUtf16(), buf.data(), strLen, len, 0, modify);
            str = Span<const uint8_t>(buf.data(), len);
        } else {
            str = Span<const uint8_t>(GetDataUtf8(), strLen);
        }
        return str;
    }

    void WriteData(EcmaString *src, uint32_t start, uint32_t destSize, uint32_t length)
    {
        if (IsUtf8()) {
            ASSERT(src->IsUtf8());
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (length != 0 && memcpy_s(GetDataUtf8Writable() + start, destSize, src->GetDataUtf8(), length) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
        } else if (src->IsUtf8()) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            Span<uint16_t> to(GetDataUtf16Writable() + start, length);
            Span<const uint8_t> from(src->GetDataUtf8(), length);
            for (uint32_t i = 0; i < length; i++) {
                to[i] = from[i];
            }
        } else {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (length != 0 && memcpy_s(GetDataUtf16Writable() + start,
                ComputeDataSizeUtf16(destSize), src->GetDataUtf16(), ComputeDataSizeUtf16(length)) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
        }
    }

    inline void WriteData(uint16_t src, uint32_t start)
    {
        if (IsUtf8()) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *(GetDataUtf8Writable() + start) = static_cast<uint8_t>(src);
        } else {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *(GetDataUtf16Writable() + start) = src;
        }
    }

    uint32_t GetLength() const
    {
        return GetMixLength() >> 2U;
    }

    void SetIsInternString()
    {
        SetMixLength(GetMixLength() | STRING_INTERN_BIT);
    }

    bool IsInternString() const
    {
        return (GetMixLength() & STRING_INTERN_BIT) != 0;
    }

    void ClearInternStringFlag()
    {
        SetMixLength(GetMixLength() & ~STRING_INTERN_BIT);
    }

    size_t ObjectSize() const
    {
        uint32_t length = GetLength();
        return IsUtf16() ? ComputeSizeUtf16(length) : ComputeSizeUtf8(length);
    }

    uint32_t PUBLIC_API GetHashcode()
    {
        uint32_t hashcode = GetRawHashcode();
        // GetLength() == 0 means it's an empty array.No need to computeHashCode again when hashseed is 0.
        if (hashcode == 0 && GetLength() != 0) {
            hashcode = ComputeHashcode(0);
            SetRawHashcode(hashcode);
        }
        return hashcode;
    }

    uint32_t PUBLIC_API ComputeHashcode(uint32_t hashSeed) const;

    static int32_t IndexOf(EcmaString *lhs, EcmaString *rhs, int pos = 0);

    static int32_t LastIndexOf(EcmaString *lhs, EcmaString *rhs, int pos = 0);

    static constexpr uint32_t GetStringCompressionMask()
    {
        return STRING_COMPRESSED_BIT;
    }

    /**
     * Compares string1 + string2 by bytes, It doesn't check canonical unicode equivalence.
     */
    bool EqualToSplicedString(const EcmaString *str1, const EcmaString *str2);
    /**
     * Compares strings by bytes, It doesn't check canonical unicode equivalence.
     */
    static bool StringsAreEqual(EcmaString *str1, EcmaString *str2);
    /**
     * Two strings have the same type of utf encoding format.
     */
    static bool StringsAreEqualSameUtfEncoding(EcmaString *str1, EcmaString *str2);
    /**
     * Compares strings by bytes, It doesn't check canonical unicode equivalence.
     */
    static bool StringsAreEqualUtf8(const EcmaString *str1, const uint8_t *utf8Data, uint32_t utf8Len,
                                    bool canBeCompress);
    /**
     * Compares strings by bytes, It doesn't check canonical unicode equivalence.
     */
    static bool StringsAreEqualUtf16(const EcmaString *str1, const uint16_t *utf16Data, uint32_t utf16Len);
    static uint32_t ComputeHashcodeUtf8(const uint8_t *utf8Data, size_t utf8Len, bool canBeCompress);
    static uint32_t ComputeHashcodeUtf16(const uint16_t *utf16Data, uint32_t length);

    static EcmaString *AllocStringObject(const EcmaVM *vm, size_t length, bool compressed);
    static EcmaString *AllocStringObjectWithSpaceType(const EcmaVM *vm, size_t length, bool compressed,
                                                      MemSpaceType type);

    static bool CanBeCompressed(const uint8_t *utf8Data, uint32_t utf8Len);
    static bool CanBeCompressed(const uint16_t *utf16Data, uint32_t utf16Len);
    static bool CanBeCompressed(const EcmaString *string);

    static inline EcmaString *FastSubUtf8String(const EcmaVM *vm, const JSHandle<EcmaString> &src, uint32_t start,
                                                uint32_t length);
    static inline EcmaString *FastSubUtf16String(const EcmaVM *vm, const JSHandle<EcmaString> &src, uint32_t start,
                                                 uint32_t length);

    bool ToElementIndex(uint32_t *index);

    bool ToTypedArrayIndex(uint32_t *index);

    static EcmaString *ToLower(const EcmaVM *vm, const JSHandle<EcmaString> &src);

    static EcmaString *ToUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src);

    static EcmaString *ToLocaleLower(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale);

    static EcmaString *ToLocaleUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale);

    template<typename T>
    static EcmaString *TrimBody(const JSThread *thread, const JSHandle<EcmaString> &src, Span<T> &data, TrimMode mode);

    static EcmaString *Trim(const JSThread *thread, const JSHandle<EcmaString> &src, TrimMode mode = TrimMode::TRIM);

    void SetLength(uint32_t length, bool compressed = false)
    {
        ASSERT(length < 0x40000000U);
        // Use 0u for compressed/utf8 expression
        SetMixLength((length << 2U) | (compressed ? STRING_COMPRESSED : STRING_UNCOMPRESSED));
    }

    uint16_t *GetDataUtf16Writable()
    {
        LOG_ECMA_IF(!IsUtf16(), FATAL) << "EcmaString: Read data as utf16 for utf8 string";
        return GetData();
    }

    uint8_t *GetDataUtf8Writable()
    {
        ASSERT_PRINT(IsUtf8(), "EcmaString: Read data as utf8 for utf16 string");
        return reinterpret_cast<uint8_t *>(GetData());
    }

    static void CopyUtf16AsUtf8(const uint16_t *utf16From, uint8_t *utf8To, uint32_t utf16Len);

    static bool IsASCIICharacter(uint16_t data)
    {
        // \0 is not considered ASCII in Ecma-Modified-UTF8 [only modify '\u0000']
        return data - 1U < base::utf_helper::UTF8_1B_MAX;
    }

    /**
     * str1 should have the same length as utf16_data.
     * Converts utf8Data to utf16 and compare it with given utf16_data.
     */
    static bool IsUtf8EqualsUtf16(const uint8_t *utf8Data, size_t utf8Len, const uint16_t *utf16Data,
                                  uint32_t utf16Len);

    template<typename T>
    /**
     * Check that two spans are equal. Should have the same length.
     */
    static bool StringsAreEquals(Span<const T> &str1, Span<const T> &str2);

    template<typename T>
    /**
     * Copy String from src to dst
     * */
    static bool StringCopy(Span<T> &dst, size_t dstMax, Span<const T> &src, size_t count);

    template<typename T1, typename T2>
    static int32_t IndexOf(Span<const T1> &lhsSp, Span<const T2> &rhsSp, int32_t pos, int32_t max);

    template<typename T1, typename T2>
    static int32_t LastIndexOf(Span<const T1> &lhsSp, Span<const T2> &rhsSp, int32_t pos);
};

static_assert((EcmaString::DATA_OFFSET % static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT)) == 0);

// if you want to use functions of EcmaString, please not use directly,
// and use functions of EcmaStringAccessor alternatively.
// eg: EcmaString *str = ***; str->GetLength() ----->  EcmaStringAccessor(str).GetLength()
class PUBLIC_API EcmaStringAccessor {
public:
    explicit EcmaStringAccessor(EcmaString *string);

    explicit EcmaStringAccessor(TaggedObject *obj);

    explicit EcmaStringAccessor(JSTaggedValue value);

    explicit EcmaStringAccessor(const JSHandle<EcmaString> &strHandle);

    static EcmaString *AllocStringObject(const EcmaVM *vm, size_t length, bool compressed)
    {
        return EcmaString::AllocStringObject(vm, length, compressed);
    }

    static EcmaString *CreateEmptyString(const EcmaVM *vm)
    {
        return EcmaString::CreateEmptyString(vm);
    }

    static EcmaString *CreateFromUtf8(const EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                                      MemSpaceType type = MemSpaceType::SEMI_SPACE)
    {
        return EcmaString::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress, type);
    }

    static EcmaString *CreateFromUtf16(const EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len,
                                       bool canBeCompress, MemSpaceType type = MemSpaceType::SEMI_SPACE)
    {
        return EcmaString::CreateFromUtf16(vm, utf16Data, utf16Len, canBeCompress, type);
    }

    static EcmaString *Concat(const EcmaVM *vm,
        const JSHandle<EcmaString> &str1Handle, const JSHandle<EcmaString> &str2Handle)
    {
        return EcmaString::Concat(vm, str1Handle, str2Handle);
    }

    static EcmaString *FastSubString(const EcmaVM *vm,
        const JSHandle<EcmaString> &src, uint32_t start, uint32_t length)
    {
        return EcmaString::FastSubString(vm, src, start, length);
    }

    bool IsUtf8() const
    {
        return string_->IsUtf8();
    }

    bool IsUtf16() const
    {
        return string_->IsUtf16();
    }

    uint32_t GetLength() const
    {
        return string_->GetLength();
    }

    size_t GetUtf8Length() const
    {
        return string_->GetUtf8Length();
    }

    size_t ObjectSize() const
    {
        return string_->ObjectSize();
    }

    bool IsInternString() const
    {
        return string_->IsInternString();
    }

    void SetInternString()
    {
        string_->SetIsInternString();
    }

    void ClearInternString()
    {
        string_->ClearInternStringFlag();
    }

    const uint8_t *GetDataUtf8()
    {
        return string_->GetDataUtf8();
    }

    const uint16_t *GetDataUtf16()
    {
        return string_->GetDataUtf16();
    }

    std::u16string ToU16String(uint32_t len = 0)
    {
        return string_->ToU16String(len);
    }

    std::unique_ptr<uint8_t[]> ToOneByteDataForced()
    {
        return string_->ToOneByteDataForced();
    }

    Span<const uint8_t> ToUtf8Span([[maybe_unused]] CVector<uint8_t> &buf)
    {
        return string_->ToUtf8Span(buf);
    }

    std::string ToStdString(StringConvertedUsage usage = StringConvertedUsage::PRINT);

    CString ToCString(StringConvertedUsage usage = StringConvertedUsage::LOGICOPERATION);

    uint32_t WriteToFlatUtf8(uint8_t *buf, uint32_t maxLength)
    {
        return string_->WriteUtf8(buf, maxLength);
    }

    uint32_t WriteToFlatUtf16(uint16_t *buf, uint32_t maxLength) const
    {
        return string_->CopyDataUtf16(buf, maxLength);
    }

    static void ReadData(EcmaString * dst, EcmaString *src, uint32_t start, uint32_t destSize, uint32_t length)
    {
        dst->WriteData(src, start, destSize, length);
    }

    template<bool verify = true>
    uint16_t Get(uint32_t index) const
    {
        return string_->At<verify>(index);
    }

    void Set(uint32_t index, uint16_t src)
    {
        return string_->WriteData(src, index);
    }

    uint32_t GetHashcode()
    {
        return string_->GetHashcode();
    }

    uint32_t ComputeHashcode(uint32_t hashSeed)
    {
        return string_->ComputeHashcode(hashSeed);
    }

    static uint32_t ComputeHashcodeUtf8(const uint8_t *utf8Data, size_t utf8Len, bool canBeCompress)
    {
        return EcmaString::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
    }

    static uint32_t ComputeHashcodeUtf16(const uint16_t *utf16Data, uint32_t length)
    {
        return EcmaString::ComputeHashcodeUtf16(utf16Data, length);
    }

    static int32_t IndexOf(EcmaString *lhs, EcmaString *rhs, int pos = 0)
    {
        return EcmaString::IndexOf(lhs, rhs, pos);
    }

    static int32_t LastIndexOf(EcmaString *lhs, EcmaString *rhs, int pos = 0)
    {
        return EcmaString::LastIndexOf(lhs, rhs, pos);
    }

    static int32_t Compare(EcmaString *lhs, EcmaString *rhs)
    {
        return EcmaString::Compare(lhs, rhs);
    }

    static bool StringsAreEqual(EcmaString *str1, EcmaString *str2)
    {
        return EcmaString::StringsAreEqual(str1, str2);
    }

    static bool StringsAreEqualSameUtfEncoding(EcmaString *str1, EcmaString *str2)
    {
        return EcmaString::StringsAreEqualSameUtfEncoding(str1, str2);
    }

    static bool StringsAreEqualUtf8(const EcmaString *str1, const uint8_t *utf8Data, uint32_t utf8Len,
                                    bool canBeCompress)
    {
        return EcmaString::StringsAreEqualUtf8(str1, utf8Data, utf8Len, canBeCompress);
    }

    static bool StringsAreEqualUtf16(const EcmaString *str1, const uint16_t *utf16Data, uint32_t utf16Len)
    {
        return EcmaString::StringsAreEqualUtf16(str1, utf16Data, utf16Len);
    }

    bool EqualToSplicedString(const EcmaString *str1, const EcmaString *str2)
    {
        return string_->EqualToSplicedString(str1, str2);
    }

    static bool CanBeCompressed(const uint8_t *utf8Data, uint32_t utf8Len)
    {
        return EcmaString::CanBeCompressed(utf8Data, utf8Len);
    }

    static bool CanBeCompressed(const uint16_t *utf16Data, uint32_t utf16Len)
    {
        return EcmaString::CanBeCompressed(utf16Data, utf16Len);
    }

    static bool CanBeCompressed(const EcmaString *string)
    {
        return EcmaString::CanBeCompressed(string);
    }

    bool ToElementIndex(uint32_t *index)
    {
        return string_->ToElementIndex(index);
    }

    bool ToTypedArrayIndex(uint32_t *index)
    {
        return string_->ToTypedArrayIndex(index);
    }

    static EcmaString *ToLower(const EcmaVM *vm, const JSHandle<EcmaString> &src)
    {
        return EcmaString::ToLower(vm, src);
    }

    static EcmaString *ToUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src)
    {
        return EcmaString::ToUpper(vm, src);
    }

    static EcmaString *ToLocaleLower(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale)
    {
        return EcmaString::ToLocaleLower(vm, src, locale);
    }

    static EcmaString *ToLocaleUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale)
    {
        return EcmaString::ToLocaleUpper(vm, src, locale);
    }

    static EcmaString *Trim(const JSThread *thread,
        const JSHandle<EcmaString> &src, EcmaString::TrimMode mode = EcmaString::TrimMode::TRIM)
    {
        return EcmaString::Trim(thread, src, mode);
    }

private:
    EcmaString *string_ {nullptr};
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_STRING_H