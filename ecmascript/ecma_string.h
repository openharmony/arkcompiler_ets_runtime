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
#include "ecmascript/common.h"
#include "ecmascript/ecma_macros.h"
#include "ecmascript/js_hclass.h"
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
class JSPandaFile;
class EcmaVM;
class LineEcmaString;
class ConstantString;
class TreeEcmaString;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ECMA_STRING_CHECK_LENGTH_AND_TRHOW(vm, length)                                        \
    if ((length) >= MAX_STRING_LENGTH) {                                                      \
        THROW_RANGE_ERROR_AND_RETURN((vm)->GetJSThread(), "Invalid string length", nullptr);  \
    }

class EcmaString : public TaggedObject {
public:
    CAST_CHECK(EcmaString, IsString);

    static constexpr uint32_t STRING_COMPRESSED_BIT = 0x1;
    static constexpr uint32_t STRING_INTERN_BIT = 0x2;
    static constexpr size_t MAX_STRING_LENGTH = 0x40000000U; // 30 bits for string length, 2 bits for special meaning

    static constexpr size_t MIX_LENGTH_OFFSET = TaggedObjectSize();
    // In last bit of mix_length we store if this string is compressed or not.
    ACCESSORS_PRIMITIVE_FIELD(MixLength, uint32_t, MIX_LENGTH_OFFSET, HASHCODE_OFFSET)
    ACCESSORS_PRIMITIVE_FIELD(RawHashcode, uint32_t, HASHCODE_OFFSET, SIZE)

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
    friend class EcmaStringAccessor;
    friend class LineEcmaString;
    friend class ConstantString;
    friend class TreeEcmaString;
    friend class NameDictionary;

    static constexpr int SMALL_STRING_SIZE = 128;

    static EcmaString *CreateEmptyString(const EcmaVM *vm);
    static EcmaString *CreateFromUtf8(const EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
        bool canBeCompress, MemSpaceType type = MemSpaceType::SEMI_SPACE, bool isConstantString = false,
        uint32_t idOffset = 0);
    static EcmaString *CreateFromUtf16(const EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len,
        bool canBeCompress, MemSpaceType type = MemSpaceType::SEMI_SPACE);
    static EcmaString *CreateLineString(const EcmaVM *vm, size_t length, bool compressed);
    static EcmaString *CreateLineStringNoGC(const EcmaVM *vm, size_t length, bool compressed);
    static EcmaString *CreateLineStringWithSpaceType(const EcmaVM *vm,
        size_t length, bool compressed, MemSpaceType type);
    static EcmaString *CreateTreeString(const EcmaVM *vm,
        const JSHandle<EcmaString> &left, const JSHandle<EcmaString> &right, uint32_t length, bool compressed);
    static EcmaString *CreateConstantString(const EcmaVM *vm, const uint8_t *utf8Data,
        size_t length, bool compressed, MemSpaceType type = MemSpaceType::SEMI_SPACE, uint32_t idOffset = 0);
    static EcmaString *Concat(const EcmaVM *vm, const JSHandle<EcmaString> &left,
        const JSHandle<EcmaString> &right, MemSpaceType type = MemSpaceType::SEMI_SPACE);
    static EcmaString *CopyStringToOldSpace(const EcmaVM *vm, const JSHandle<EcmaString> &original,
        uint32_t length, bool compressed);
    static EcmaString *FastSubString(const EcmaVM *vm,
        const JSHandle<EcmaString> &src, uint32_t start, uint32_t length);
    // require src is LineString
    // not change src data structure
    static inline EcmaString *FastSubUtf8String(const EcmaVM *vm,
        const JSHandle<EcmaString> &src, uint32_t start, uint32_t length);
    // require src is LineString
    // not change src data structure
    static inline EcmaString *FastSubUtf16String(const EcmaVM *vm,
        const JSHandle<EcmaString> &src, uint32_t start, uint32_t length);

    bool IsUtf8() const
    {
        return (GetMixLength() & STRING_COMPRESSED_BIT) == STRING_COMPRESSED;
    }

    bool IsUtf16() const
    {
        return (GetMixLength() & STRING_COMPRESSED_BIT) == STRING_UNCOMPRESSED;
    }

    // require is LineString
    inline uint16_t *GetData() const;
    inline const uint8_t *GetDataUtf8() const;
    inline const uint16_t *GetDataUtf16() const;

    // require is LineString
    inline uint8_t *GetDataUtf8Writable();
    inline uint16_t *GetDataUtf16Writable();

    uint32_t GetLength() const
    {
        return GetMixLength() >> 2U;
    }

    void SetLength(uint32_t length, bool compressed = false)
    {
        ASSERT(length < MAX_STRING_LENGTH);
        // Use 0u for compressed/utf8 expression
        SetMixLength((length << 2U) | (compressed ? STRING_COMPRESSED : STRING_UNCOMPRESSED));
    }

    inline size_t GetUtf8Length(bool modify = true) const;

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

    bool TryGetHashCode(uint32_t *hash)
    {
        uint32_t hashcode = GetRawHashcode();
        if (hashcode == 0 && GetLength() != 0) {
            return false;
        }
        *hash = hashcode;
        return true;
    }

    // not change this data structure.
    // if string is not flat, this func has low efficiency.
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

    // not change this data structure.
    // if string is not flat, this func has low efficiency.
    uint32_t PUBLIC_API ComputeHashcode(uint32_t hashSeed) const;

    static uint32_t ComputeHashcodeUtf8(const uint8_t *utf8Data, size_t utf8Len, bool canBeCompress);
    static uint32_t ComputeHashcodeUtf16(const uint16_t *utf16Data, uint32_t length);

    template<bool verify = true>
    uint16_t At(int32_t index) const;

    // require is LineString
    void WriteData(uint32_t index, uint16_t src);

    // can change left and right data structure
    static int32_t Compare(const EcmaVM *vm, const JSHandle<EcmaString> &left, const JSHandle<EcmaString> &right);

    // Check that two spans are equal. Should have the same length.
    /* static */
    template<typename T>
    static bool StringsAreEquals(Span<const T> &str1, Span<const T> &str2)
    {
        ASSERT(str1.Size() <= str2.Size());
        size_t size = str1.Size();
        if (size < SMALL_STRING_SIZE) {
            for (size_t i = 0; i < size; i++) {
                if (str1[i] != str2[i]) {
                    return false;
                }
            }
            return true;
        }
        return memcmp(str1.data(), str2.data(), size * sizeof(T)) == 0;
    }

    // Converts utf8Data to utf16 and compare it with given utf16_data.
    static bool IsUtf8EqualsUtf16(const uint8_t *utf8Data, size_t utf8Len, const uint16_t *utf16Data,
                                  uint32_t utf16Len);
    // Compares string1 + string2 by bytes, It doesn't check canonical unicode equivalence.
    bool EqualToSplicedString(const EcmaString *str1, const EcmaString *str2);
    // Compares strings by bytes, It doesn't check canonical unicode equivalence.
    static bool StringsAreEqual(const EcmaVM *vm, const JSHandle<EcmaString> &str1, const JSHandle<EcmaString> &str2);
    // Compares strings by bytes, It doesn't check canonical unicode equivalence.
    static bool StringsAreEqual(EcmaString *str1, EcmaString *str2);
    // Two strings have the same type of utf encoding format.
    static bool StringsAreEqualSameUtfEncoding(EcmaString *str1, EcmaString *str2);
    // Compares strings by bytes, It doesn't check canonical unicode equivalence.
    // not change str1 data structure.
    // if str1 is not flat, this func has low efficiency.
    static bool StringsAreEqualUtf8(const EcmaString *str1, const uint8_t *utf8Data, uint32_t utf8Len,
                                    bool canBeCompress);
    // Compares strings by bytes, It doesn't check canonical unicode equivalence.
    // not change str1 data structure.
    // if str1 is not flat, this func has low efficiency.
    static bool StringsAreEqualUtf16(const EcmaString *str1, const uint16_t *utf16Data, uint32_t utf16Len);

    // can change receiver and search data structure
    static int32_t IndexOf(const EcmaVM *vm,
        const JSHandle<EcmaString> &receiver, const JSHandle<EcmaString> &search, int pos = 0);

    // can change receiver and search data structure
    static int32_t LastIndexOf(const EcmaVM *vm,
        const JSHandle<EcmaString> &receiver, const JSHandle<EcmaString> &search, int pos = 0);

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
    inline size_t WriteUtf8(uint8_t *buf, size_t maxLength, bool isWriteBuffer = false) const
    {
        if (maxLength == 0) {
            return 1; // maxLength was -1 at napi
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        buf[maxLength - 1] = '\0';
        return CopyDataRegionUtf8(buf, 0, GetLength(), maxLength, true, isWriteBuffer) + 1;
    }

    size_t CopyDataToUtf16(uint16_t *buf, uint32_t length, uint32_t bufLength) const
    {
        if (IsUtf16()) {
            CVector<uint16_t> tmpBuf;
            const uint16_t *data = EcmaString::GetUtf16DataFlat(this, tmpBuf);
            if (length > bufLength) {
                if (memcpy_s(buf, bufLength * sizeof(uint16_t), data, bufLength * sizeof(uint16_t)) != EOK) {
                    LOG_FULL(FATAL) << "memcpy_s failed when length > bufLength";
                    UNREACHABLE();
                }
                return bufLength;
            }
            if (memcpy_s(buf, bufLength * sizeof(uint16_t), data, length * sizeof(uint16_t)) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            return length;
        }
        CVector<uint8_t> tmpBuf;
        const uint8_t *data = EcmaString::GetUtf8DataFlat(this, tmpBuf);
        if (length > bufLength) {
            return base::utf_helper::ConvertRegionUtf8ToUtf16(data, buf, bufLength, bufLength, 0);
        }
        return base::utf_helper::ConvertRegionUtf8ToUtf16(data, buf, length, bufLength, 0);
    }

    // It allows user to copy into buffer even if maxLength < length
    inline size_t WriteUtf16(uint16_t *buf, uint32_t targetLength, uint32_t bufLength) const
    {
        if (bufLength == 0) {
            return 0;
        }
        // Returns a number representing a valid backrest length.
        return CopyDataToUtf16(buf, targetLength, bufLength);
    }

    size_t WriteOneByte(uint8_t *buf, size_t maxLength) const
    {
        if (maxLength == 0) {
            return 0;
        }
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        buf[maxLength - 1] = '\0';
        uint32_t length = GetLength();
        if (!IsUtf16()) {
            CVector<uint8_t> tmpBuf;
            const uint8_t *data = GetUtf8DataFlat(this, tmpBuf);
            if (length > maxLength) {
                length = maxLength;
            }
            if (memcpy_s(buf, maxLength, data, length) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed when write one byte";
                UNREACHABLE();
            }
            return length;
        }
        
        CVector<uint16_t> tmpBuf;
        const uint16_t *data = GetUtf16DataFlat(this, tmpBuf);
        if (length > maxLength) {
            return base::utf_helper::ConvertRegionUtf16ToLatin1(data, buf, maxLength, maxLength);
        }
        return base::utf_helper::ConvertRegionUtf16ToLatin1(data, buf, length, maxLength);
    }

    size_t CopyDataRegionUtf8(uint8_t *buf, size_t start, size_t length, size_t maxLength,
                              bool modify = true, bool isWriteBuffer = false) const
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
            CVector<uint8_t> tmpBuf;
            const uint8_t *data = GetUtf8DataFlat(this, tmpBuf);
            if (length > maxLength) {
                if (memcpy_s(buf, maxLength, data + start, maxLength) != EOK) {
                    LOG_FULL(FATAL) << "memcpy_s failed when length > maxlength";
                    UNREACHABLE();
                }
                return maxLength;
            }
            if (memcpy_s(buf, maxLength, data + start, length) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed when length <= maxlength";
                UNREACHABLE();
            }
            return length;
        }
        CVector<uint16_t> tmpBuf;
        const uint16_t *data = GetUtf16DataFlat(this, tmpBuf);
        if (length > maxLength) {
            return base::utf_helper::ConvertRegionUtf16ToUtf8(data, buf, maxLength, maxLength, start,
                                                              modify, isWriteBuffer);
        }
        return base::utf_helper::ConvertRegionUtf16ToUtf8(data, buf, length, maxLength, start,
                                                          modify, isWriteBuffer);
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
            CVector<uint16_t> tmpBuf;
            const uint16_t *data = EcmaString::GetUtf16DataFlat(this, tmpBuf);
            if (memcpy_s(buf, maxLength * sizeof(uint16_t), data + start, length * sizeof(uint16_t)) != EOK) {
                LOG_FULL(FATAL) << "memcpy_s failed";
                UNREACHABLE();
            }
            return length;
        }
        CVector<uint8_t> tmpBuf;
        const uint8_t *data = EcmaString::GetUtf8DataFlat(this, tmpBuf);
        return base::utf_helper::ConvertRegionUtf8ToUtf16(data, buf, len, maxLength, start);
    }

    std::u16string ToU16String(uint32_t len = 0);

    std::unique_ptr<uint8_t[]> ToOneByteDataForced()
    {
        uint8_t *buf = nullptr;
        auto length = GetLength();
        if (IsUtf16()) {
            auto size = length * sizeof(uint16_t);
            buf = new uint8_t[size]();
            CopyDataUtf16(reinterpret_cast<uint16_t *>(buf), length);
        } else {
            buf = new uint8_t[length + 1]();
            CopyDataUtf8(buf, length + 1);
        }
        return std::unique_ptr<uint8_t[]>(buf);
    }

    Span<const uint8_t> ToUtf8Span(CVector<uint8_t> &buf, bool modify = true)
    {
        Span<const uint8_t> str;
        uint32_t strLen = GetLength();
        if (UNLIKELY(IsUtf16())) {
            CVector<uint16_t> tmpBuf;
            const uint16_t *data = EcmaString::GetUtf16DataFlat(this, tmpBuf);
            size_t len = base::utf_helper::Utf16ToUtf8Size(data, strLen, modify) - 1;
            buf.reserve(len);
            len = base::utf_helper::ConvertRegionUtf16ToUtf8(data, buf.data(), strLen, len, 0, modify);
            str = Span<const uint8_t>(buf.data(), len);
        } else {
            const uint8_t *data = EcmaString::GetUtf8DataFlat(this, buf);
            str = Span<const uint8_t>(data, strLen);
        }
        return str;
    }

    void WriteData(EcmaString *src, uint32_t start, uint32_t destSize, uint32_t length);

    static bool CanBeCompressed(const uint8_t *utf8Data, uint32_t utf8Len);
    static bool CanBeCompressed(const uint16_t *utf16Data, uint32_t utf16Len);
    static bool CanBeCompressed(const EcmaString *string);

    bool ToElementIndex(uint32_t *index);

    bool ToTypedArrayIndex(uint32_t *index);

    template<bool isLower>
    static EcmaString *ConvertCase(const EcmaVM *vm, const JSHandle<EcmaString> &src);

    template<bool isLower>
    static EcmaString *LocaleConvertCase(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale);

    template<typename T>
    static EcmaString *TrimBody(const JSThread *thread, const JSHandle<EcmaString> &src, Span<T> &data, TrimMode mode);

    static EcmaString *Trim(const JSThread *thread, const JSHandle<EcmaString> &src, TrimMode mode = TrimMode::TRIM);

    // single char copy for loop
    template<typename DstType, typename SrcType>
    static void CopyChars(DstType *dst, SrcType *src, uint32_t count)
    {
        Span<SrcType> srcSp(src, count);
        Span<DstType> dstSp(dst, count);
        for (uint32_t i = 0; i < count; i++) {
            dstSp[i] = srcSp[i];
        }
    }

    // memory block copy
    template<typename T>
    static bool MemCopyChars(Span<T> &dst, size_t dstMax, Span<const T> &src, size_t count);

    template<typename T>
    static uint32_t ComputeHashForData(const T *data, size_t size, uint32_t hashSeed)
    {
        uint32_t hash = hashSeed;
        Span<const T> sp(data, size);
        for (auto c : sp) {
            constexpr size_t SHIFT = 5;
            hash = (hash << SHIFT) - hash + c;
        }
        return hash;
    }

    static bool IsASCIICharacter(uint16_t data)
    {
        // \0 is not considered ASCII in Ecma-Modified-UTF8 [only modify '\u0000']
        return data - 1U < base::utf_helper::UTF8_1B_MAX;
    }

    template<typename T1, typename T2>
    static int32_t IndexOf(Span<const T1> &lhsSp, Span<const T2> &rhsSp, int32_t pos, int32_t max);

    template<typename T1, typename T2>
    static int32_t LastIndexOf(Span<const T1> &lhsSp, Span<const T2> &rhsSp, int32_t pos);

    bool IsFlat() const;

    bool IsLineString() const
    {
        return GetClass()->IsLineString();
    }
    bool IsConstantString() const
    {
        return GetClass()->IsConstantString();
    }
    bool IsTreeString() const
    {
        return GetClass()->IsTreeString();
    }
    bool IsLineOrConstantString() const
    {
        auto hclass = GetClass();
        return hclass->IsLineString() || hclass->IsConstantString();
    }

    JSType GetStringType() const
    {
        JSType type = GetClass()->GetObjectType();
        ASSERT(type >= JSType::STRING_FIRST && type <= JSType::STRING_LAST);
        return type;
    }

    template <typename Char>
    static void WriteToFlat(EcmaString *src, Char *buf, uint32_t maxLength);

    static const uint8_t *GetUtf8DataFlat(const EcmaString *src, CVector<uint8_t> &buf);

    static const uint16_t *GetUtf16DataFlat(const EcmaString *src, CVector<uint16_t> &buf);

    // string must be not flat
    static EcmaString *SlowFlatten(const EcmaVM *vm, const JSHandle<TreeEcmaString> &string, MemSpaceType type);

    static EcmaString *Flatten(const EcmaVM *vm, const JSHandle<EcmaString> &string,
                               MemSpaceType type = MemSpaceType::SEMI_SPACE);

    static EcmaString *FlattenNoGC(const EcmaVM *vm, EcmaString *string);

    static EcmaString *ToLower(const EcmaVM *vm, const JSHandle<EcmaString> &src);

    static EcmaString *ToUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src);

    static EcmaString *ToLocaleLower(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale);

    static EcmaString *ToLocaleUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale);

    static EcmaString *TryToLower(const EcmaVM *vm, const JSHandle<EcmaString> &src);

    static EcmaString *ConvertUtf8ToLowerOrUpper(const EcmaVM *vm, const JSHandle<EcmaString> &srcFlat,
                                                 bool toLower, uint32_t startIndex = 0);
};

// The LineEcmaString abstract class captures sequential string values, only LineEcmaString can store chars data
class LineEcmaString : public EcmaString {
public:
    // DATA_OFFSET: the string data stored after the string header.
    // Data can be stored in utf8 or utf16 form according to compressed bit.
    static constexpr size_t DATA_OFFSET = EcmaString::SIZE;  // DATA_OFFSET equal to Empty String size

    CAST_CHECK(LineEcmaString, IsLineString);

    static LineEcmaString *Cast(EcmaString *str)
    {
        return static_cast<LineEcmaString *>(str);
    }

    static LineEcmaString *Cast(const EcmaString *str)
    {
        return LineEcmaString::Cast(const_cast<EcmaString *>(str));
    }

    static size_t ComputeSizeUtf8(uint32_t utf8Len)
    {
        return DATA_OFFSET + utf8Len;
    }

    static size_t ComputeSizeUtf16(uint32_t utf16Len)
    {
        return DATA_OFFSET + utf16Len * sizeof(uint16_t);
    }

    static size_t ObjectSize(EcmaString *str)
    {
        uint32_t length = str->GetLength();
        return str->IsUtf16() ? ComputeSizeUtf16(length) : ComputeSizeUtf8(length);
    }

    uint16_t *GetData() const
    {
        return reinterpret_cast<uint16_t *>(ToUintPtr(this) + DATA_OFFSET);
    }

    template<bool verify = true>
    uint16_t Get(int32_t index) const
    {
        int32_t length = static_cast<int32_t>(GetLength());
        if (verify) {
            if ((index < 0) || (index >= length)) {
                return 0;
            }
        }
        if (!IsUtf16()) {
            Span<const uint8_t> sp(GetDataUtf8(), length);
            return sp[index];
        }
        Span<const uint16_t> sp(GetDataUtf16(), length);
        return sp[index];
    }

    void Set(uint32_t index, uint16_t src)
    {
        ASSERT(index < GetLength());
        if (IsUtf8()) {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *(reinterpret_cast<uint8_t *>(GetData()) + index) = static_cast<uint8_t>(src);
        } else {
            // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            *(GetData() + index) = src;
        }
    }
};
static_assert((LineEcmaString::DATA_OFFSET % static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT)) == 0);

class ConstantString : public EcmaString {
public:
    static constexpr size_t ENTITY_ID_OFFSET = EcmaString::SIZE;
    // ConstantData is the pointer of const string in the pandafile.
    // String in pandafile is encoded by the utf8 format.
    ACCESSORS_PRIMITIVE_FIELD(EntityId, uint32_t, ENTITY_ID_OFFSET, CONSTANT_DATA_OFFSET);
    ACCESSORS_NATIVE_FIELD(ConstantData, uint8_t, CONSTANT_DATA_OFFSET, SIZE);

    CAST_CHECK(ConstantString, IsConstantString);

    static ConstantString *Cast(EcmaString *str)
    {
        return static_cast<ConstantString *>(str);
    }

    static ConstantString *Cast(const EcmaString *str)
    {
        return ConstantString::Cast(const_cast<EcmaString *>(str));
    }

    static size_t ObjectSize()
    {
        return ConstantString::SIZE;
    }

    template<bool verify = true>
    uint16_t Get(int32_t index) const
    {
        int32_t length = static_cast<int32_t>(GetLength());
        if (verify) {
            if ((index < 0) || (index >= length)) {
                return 0;
            }
        }
        ASSERT(IsUtf8());
        Span<const uint8_t> sp(GetConstantData(), length);
        return sp[index];
    }
};

class TreeEcmaString : public EcmaString {
public:
    // Minimum length for a tree string
    static constexpr uint32_t MIN_TREE_ECMASTRING_LENGTH = 13;

    static constexpr size_t FIRST_OFFSET = EcmaString::SIZE;
    ACCESSORS(First, FIRST_OFFSET, SECOND_OFFSET);
    ACCESSORS(Second, SECOND_OFFSET, SIZE);

    DECL_VISIT_OBJECT(FIRST_OFFSET, SIZE);

    CAST_CHECK(TreeEcmaString, IsTreeString);

    static TreeEcmaString *Cast(EcmaString *str)
    {
        return static_cast<TreeEcmaString *>(str);
    }

    static TreeEcmaString *Cast(const EcmaString *str)
    {
        return TreeEcmaString::Cast(const_cast<EcmaString *>(str));
    }

    bool IsFlat() const
    {
        auto strSecond = EcmaString::Cast(GetSecond());
        return strSecond->GetLength() == 0;
    }

    template<bool verify = true>
    uint16_t Get(int32_t index) const
    {
        int32_t length = static_cast<int32_t>(GetLength());
        if (verify) {
            if ((index < 0) || (index >= length)) {
                return 0;
            }
        }

        if (IsFlat()) {
            EcmaString *first = EcmaString::Cast(GetFirst());
            return first->At<verify>(index);
        }
        EcmaString *string = const_cast<TreeEcmaString *>(this);
        while (true) {
            if (string->IsTreeString()) {
                EcmaString *first = EcmaString::Cast(TreeEcmaString::Cast(string)->GetFirst());
                if (static_cast<int32_t>(first->GetLength()) > index) {
                    string = first;
                } else {
                    index -= static_cast<int32_t>(first->GetLength());
                    string = EcmaString::Cast(TreeEcmaString::Cast(string)->GetSecond());
                }
            } else {
                return string->At<verify>(index);
            }
        }
        UNREACHABLE();
    }
};

// if you want to use functions of EcmaString, please not use directly,
// and use functions of EcmaStringAccessor alternatively.
// eg: EcmaString *str = ***; str->GetLength() ----->  EcmaStringAccessor(str).GetLength()
class PUBLIC_API EcmaStringAccessor {
public:
    explicit EcmaStringAccessor(EcmaString *string);

    explicit EcmaStringAccessor(TaggedObject *obj);

    explicit EcmaStringAccessor(JSTaggedValue value);

    explicit EcmaStringAccessor(const JSHandle<EcmaString> &strHandle);

    static EcmaString *CreateLineString(const EcmaVM *vm, size_t length, bool compressed)
    {
        return EcmaString::CreateLineString(vm, length, compressed);
    }

    static EcmaString *CreateEmptyString(const EcmaVM *vm)
    {
        return EcmaString::CreateEmptyString(vm);
    }

    static EcmaString *CreateFromUtf8(const EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                                      MemSpaceType type = MemSpaceType::SEMI_SPACE, bool isConstantString = false,
                                      uint32_t idOffset = 0)
    {
        return EcmaString::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress, type, isConstantString, idOffset);
    }

    static EcmaString *CreateConstantString(const EcmaVM *vm, const uint8_t *utf8Data, size_t length,
        bool compressed, MemSpaceType type = MemSpaceType::SEMI_SPACE, uint32_t idOffset = 0)
    {
        return EcmaString::CreateConstantString(vm, utf8Data, length, compressed, type, idOffset);
    }

    static EcmaString *CreateFromUtf16(const EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len,
                                       bool canBeCompress, MemSpaceType type = MemSpaceType::SEMI_SPACE)
    {
        return EcmaString::CreateFromUtf16(vm, utf16Data, utf16Len, canBeCompress, type);
    }

    static EcmaString *Concat(const EcmaVM *vm, const JSHandle<EcmaString> &str1Handle,
        const JSHandle<EcmaString> &str2Handle, MemSpaceType type = MemSpaceType::SEMI_SPACE)
    {
        return EcmaString::Concat(vm, str1Handle, str2Handle, type);
    }

    static EcmaString *CopyStringToOldSpace(const EcmaVM *vm, const JSHandle<EcmaString> &original,
        uint32_t length, bool compressed)
    {
        return EcmaString::CopyStringToOldSpace(vm, original, length, compressed);
    }

    // can change src data structure
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

    // require is LineString
    inline size_t GetUtf8Length() const;

    size_t ObjectSize() const
    {
        if (string_->IsLineString()) {
            return LineEcmaString::ObjectSize(string_);
        } if (string_->IsConstantString()) {
            return ConstantString::ObjectSize();
        } else {
            return TreeEcmaString::SIZE;
        }
    }

    // For TreeString, the calculation result is size of LineString correspondingly.
    size_t GetFlatStringSize() const
    {
        if (string_->IsConstantString()) {
            return ConstantString::ObjectSize();
        }
        return LineEcmaString::ObjectSize(string_);
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

    // require is LineString
    // It's Utf8 format, but without 0 in the end.
    inline const uint8_t *GetDataUtf8();

    // require is LineString
    inline const uint16_t *GetDataUtf16();

    // not change string data structure.
    // if string is not flat, this func has low efficiency.
    std::u16string ToU16String(uint32_t len = 0)
    {
        return string_->ToU16String(len);
    }

    // not change string data structure.
    // if string is not flat, this func has low efficiency.
    std::unique_ptr<uint8_t[]> ToOneByteDataForced()
    {
        return string_->ToOneByteDataForced();
    }

    // not change string data structure.
    // if string is not flat, this func has low efficiency.
    Span<const uint8_t> ToUtf8Span(CVector<uint8_t> &buf)
    {
        return string_->ToUtf8Span(buf);
    }

    // not change string data structure.
    // if string is not flat, this func has low efficiency.
    std::string ToStdString(StringConvertedUsage usage = StringConvertedUsage::PRINT);

    // not change string data structure.
    // if string is not flat, this func has low efficiency.
    CString ToCString(StringConvertedUsage usage = StringConvertedUsage::LOGICOPERATION);

    // not change string data structure.
    // if string is not flat, this func has low efficiency.
    uint32_t WriteToFlatUtf8(uint8_t *buf, uint32_t maxLength, bool isWriteBuffer = false)
    {
        return string_->WriteUtf8(buf, maxLength, isWriteBuffer);
    }

    uint32_t WriteToUtf16(uint16_t *buf, uint32_t bufLength)
    {
        return string_->WriteUtf16(buf, GetLength(), bufLength);
    }

    uint32_t WriteToOneByte(uint8_t *buf, uint32_t maxLength)
    {
        return string_->WriteOneByte(buf, maxLength);
    }

    // not change string data structure.
    // if string is not flat, this func has low efficiency.
    uint32_t WriteToFlatUtf16(uint16_t *buf, uint32_t maxLength) const
    {
        return string_->CopyDataUtf16(buf, maxLength);
    }

    // require dst is LineString
    // not change src data structure.
    // if src is not flat, this func has low efficiency.
    inline static void ReadData(EcmaString * dst, EcmaString *src, uint32_t start, uint32_t destSize, uint32_t length);

    // not change src data structure.
    // if src is not flat, this func has low efficiency.
    template<bool verify = true>
    uint16_t Get(uint32_t index) const
    {
        return string_->At<verify>(index);
    }

    // require string is LineString.
    void Set(uint32_t index, uint16_t src)
    {
        return string_->WriteData(index, src);
    }

    // not change src data structure.
    // if src is not flat, this func has low efficiency.
    uint32_t GetHashcode()
    {
        return string_->GetHashcode();
    }

    // not change src data structure.
    // if src is not flat, this func has low efficiency.
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

    // can change receiver and search data structure
    static int32_t IndexOf(const EcmaVM *vm,
        const JSHandle<EcmaString> &receiver, const JSHandle<EcmaString> &search, int pos = 0)
    {
        return EcmaString::IndexOf(vm, receiver, search, pos);
    }

    // can change receiver and search data structure
    static int32_t LastIndexOf(const EcmaVM *vm,
        const JSHandle<EcmaString> &receiver, const JSHandle<EcmaString> &search, int pos = 0)
    {
        return EcmaString::LastIndexOf(vm, receiver, search, pos);
    }

    // can change receiver and search data structure
    static int32_t Compare(const EcmaVM *vm, const JSHandle<EcmaString>& left, const JSHandle<EcmaString>& right)
    {
        return EcmaString::Compare(vm, left, right);
    }

    // can change str1 and str2 data structure
    static bool StringsAreEqual(const EcmaVM *vm, const JSHandle<EcmaString> &str1, const JSHandle<EcmaString> &str2)
    {
        return EcmaString::StringsAreEqual(vm, str1, str2);
    }

    // not change str1 and str2 data structure.
    // if str1 or str2 is not flat, this func has low efficiency.
    static bool StringsAreEqual(EcmaString *str1, EcmaString *str2)
    {
        return EcmaString::StringsAreEqual(str1, str2);
    }

    // not change str1 and str2 data structure.
    // if str1 or str2 is not flat, this func has low efficiency.
    static bool StringsAreEqualSameUtfEncoding(EcmaString *str1, EcmaString *str2)
    {
        return EcmaString::StringsAreEqualSameUtfEncoding(str1, str2);
    }

    // not change str1 data structure.
    // if str1 is not flat, this func has low efficiency.
    static bool StringsAreEqualUtf8(const EcmaString *str1, const uint8_t *utf8Data, uint32_t utf8Len,
                                    bool canBeCompress)
    {
        return EcmaString::StringsAreEqualUtf8(str1, utf8Data, utf8Len, canBeCompress);
    }

    // not change str1 data structure.
    // if str1 is not flat, this func has low efficiency.
    static bool StringsAreEqualUtf16(const EcmaString *str1, const uint16_t *utf16Data, uint32_t utf16Len)
    {
        return EcmaString::StringsAreEqualUtf16(str1, utf16Data, utf16Len);
    }

    // require str1 and str2 are LineString.
    // not change string data structure.
    // if string is not flat, this func has low efficiency.
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

    // require string is LineString
    static bool CanBeCompressed(const EcmaString *string)
    {
        return EcmaString::CanBeCompressed(string);
    }

    // not change string data structure.
    // if string is not flat, this func has low efficiency.
    bool ToElementIndex(uint32_t *index)
    {
        return string_->ToElementIndex(index);
    }

    // not change string data structure.
    // if string is not flat, this func has low efficiency.
    bool ToTypedArrayIndex(uint32_t *index)
    {
        return string_->ToTypedArrayIndex(index);
    }

    static EcmaString *ToLower(const EcmaVM *vm, const JSHandle<EcmaString> &src)
    {
        return EcmaString::ToLower(vm, src);
    }

    static EcmaString *TryToLower(const EcmaVM *vm, const JSHandle<EcmaString> &src)
    {
        return EcmaString::TryToLower(vm, src);
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

    bool IsFlat() const
    {
        return string_->IsFlat();
    }

    bool IsLineString() const
    {
        return string_->IsLineString();
    }

    bool IsConstantString() const
    {
        return string_->IsConstantString();
    }

    bool IsLineOrConstantString() const
    {
        return string_->IsLineOrConstantString();
    }

    bool IsTreeString() const
    {
        return string_->IsTreeString();
    }

    static EcmaString *Flatten(const EcmaVM *vm, const JSHandle<EcmaString> &string,
        MemSpaceType type = MemSpaceType::SEMI_SPACE)
    {
        return EcmaString::Flatten(vm, string, type);
    }

    static EcmaString *SlowFlatten(const EcmaVM *vm, const JSHandle<TreeEcmaString> &string,
        MemSpaceType type = MemSpaceType::SEMI_SPACE)
    {
        return EcmaString::SlowFlatten(vm, string, type);
    }

    static EcmaString *FlattenNoGC(const EcmaVM *vm, EcmaString *string)
    {
        return EcmaString::FlattenNoGC(vm, string);
    }

    static const uint8_t *GetUtf8DataFlat(const EcmaString *src, CVector<uint8_t> &buf)
    {
        return EcmaString::GetUtf8DataFlat(src, buf);
    }

    static const uint16_t *GetUtf16DataFlat(const EcmaString *src, CVector<uint16_t> &buf)
    {
        return EcmaString::GetUtf16DataFlat(src, buf);
    }

private:
    EcmaString *string_ {nullptr};
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_STRING_H