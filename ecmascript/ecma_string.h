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

#include "common_components/base/utf_helper.h"
#include "common_interfaces/objects/base_string.h"
#include "common_interfaces/objects/string/line_string.h"
#include "common_interfaces/objects/string/sliced_string.h"
#include "common_interfaces/objects/string/tree_string.h"
#include "common_interfaces/objects/string/base_string-inl1.h"
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
namespace test {
    class EcmaStringEqualsTest;
    class EcmaStringHashTest;
}
namespace ecmascript {
template<typename T>
class JSHandle;
class JSPandaFile;
class EcmaVM;
class LineEcmaString;
class TreeEcmaString;
class SlicedEcmaString;
class FlatStringInfo;

using ::common::BaseString;
using ::common::LineString;
using ::common::SlicedString;
using ::common::TreeString;

// NOLINTNEXTLINE(cppcoreguidelines-macro-usage)
#define ECMA_STRING_CHECK_LENGTH_AND_TRHOW(vm, length)                                        \
    if ((length) >= BaseString::MAX_STRING_LENGTH) {                                                      \
        THROW_RANGE_ERROR_AND_RETURN((vm)->GetJSThread(), "Invalid string length", nullptr);  \
    }

class EcmaString : public TaggedObject {
private:
    using TaggedObject::SIZE;

public:
    CAST_CHECK(EcmaString, IsString);

    BaseString* ToBaseString()
    {
        return BaseString::Cast(this);
    }

    const BaseString* ToBaseString() const
    {
        return BaseString::ConstCast(this);
    }

    static EcmaString* FromBaseString(BaseString* str)
    {
        return reinterpret_cast<EcmaString*>(str);
    }

    enum TrimMode : uint8_t {
        TRIM,
        TRIM_START,
        TRIM_END,
    };

    void SetRawHashcode(uint32_t rawHashCode)
    {
        return ToBaseString()->SetRawHashcode(rawHashCode);
    }

private:

    friend class EcmaStringAccessor;
    friend class LineEcmaString;
    friend class TreeEcmaString;
    friend class SlicedEcmaString;
    friend class FlatStringInfo;
    friend class NameDictionary;
    friend class panda::test::EcmaStringEqualsTest;
    friend class panda::test::EcmaStringHashTest;

    static constexpr size_t ALIGNMENT_8_BYTES = 8;
    static EcmaString *CreateEmptyString(const EcmaVM *vm);
    static EcmaString *CreateFromUtf8(const EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len,
        bool canBeCompress, MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE);
    static EcmaString *CreateFromUtf8CompressedSubString(const EcmaVM *vm, const JSHandle<EcmaString> &string,
        uint32_t offset, uint32_t utf8Len, MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE);
    static EcmaString *CreateUtf16StringFromUtf8(const EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len,
        MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE);
    static EcmaString *CreateFromUtf16(const EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len,
        bool canBeCompress, MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE);
    static SlicedEcmaString* CreateSlicedString(const EcmaVM* vm, JSHandle<EcmaString> parent,
                                                MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE);
    static EcmaString *CreateLineString(const EcmaVM *vm, size_t length, bool compressed);
    static EcmaString *CreateLineStringNoGC(const EcmaVM *vm, size_t length, bool compressed);
    static EcmaString *AllocLineString(const EcmaVM* vm, size_t size, MemSpaceType type);
    static EcmaString *CreateLineStringWithSpaceType(const EcmaVM *vm,
                                                     size_t length, bool compressed, MemSpaceType type);
    static EcmaString *CreateTreeString(const EcmaVM *vm,
        const JSHandle<EcmaString> &left, const JSHandle<EcmaString> &right, uint32_t length, bool compressed);
    static EcmaString *Concat(const EcmaVM *vm, const JSHandle<EcmaString> &left,
        const JSHandle<EcmaString> &right, MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE);
    template<typename T1, typename T2>
    static uint32_t CalculateDataConcatHashCode(const T1 *dataFirst, size_t sizeFirst,
                                                const T2 *dataSecond, size_t sizeSecond);
    static uint32_t CalculateConcatHashCode(const JSHandle<EcmaString> &firstString,
                                            const JSHandle<EcmaString> &secondString);
    static EcmaString *CopyStringToOldSpace(const EcmaVM *vm, const JSHandle<EcmaString> &original,
        uint32_t length, bool compressed);
    static EcmaString *FastSubString(const EcmaVM *vm,
        const JSHandle<EcmaString> &src, uint32_t start, uint32_t length);
    static bool SubStringIsUtf8(const EcmaVM *vm,
        const JSHandle<EcmaString> &src, uint32_t start, uint32_t length);
    static EcmaString *GetSlicedString(const EcmaVM *vm,
        const JSHandle<EcmaString> &src, uint32_t start, uint32_t length);
    static EcmaString *GetSubString(const EcmaVM *vm,
        const JSHandle<EcmaString> &src, uint32_t start, uint32_t length);
    // require src is LineString
    // not change src data structure
    static inline EcmaString *FastSubUtf8String(const EcmaVM *vm,
        const JSHandle<EcmaString> &src, uint32_t start, uint32_t length);
    // require src is LineString
    // not change src data structure
    static inline EcmaString *FastSubUtf16String(const EcmaVM *vm,
        const JSHandle<EcmaString> &src, uint32_t start, uint32_t length);
    inline void TrimLineString(const JSThread *thread, uint32_t newLength);

    inline bool IsUtf8() const
    {
        return ToBaseString()->IsUtf8();
    }

    inline bool IsUtf16() const
    {
        return ToBaseString()->IsUtf16();
    }

    inline const uint8_t *GetDataUtf8() const;
    inline const uint16_t *GetDataUtf16() const;

    inline Span<const uint8_t> FastToUtf8Span() const;

    // require is LineString
    inline uint8_t *GetDataUtf8Writable();
    inline uint16_t *GetDataUtf16Writable();

    inline uint32_t GetLength() const
    {
        return ToBaseString()->GetLength();
    }

    inline void InitLengthAndFlags(uint32_t length, bool compressed = false, bool isIntern = false)
    {
        ToBaseString()->InitLengthAndFlags(length, compressed, isIntern);
    }

    inline size_t GetUtf8Length(bool modify = true, bool isGetBufferSize = false) const;

    inline void SetIsInternString()
    {
        ToBaseString()->SetIsInternString();
    }

    inline bool IsInternString() const
    {
        return ToBaseString()->IsInternString();
    }

    inline void ClearInternStringFlag()
    {
        ToBaseString()->ClearInternStringFlag();
    }

    inline bool TryGetHashCode(uint32_t *hash)
    {
        return ToBaseString()->TryGetHashCode(hash);
    }

    // not change this data structure.
    // if string is not flat, this func has low efficiency.
    uint32_t PUBLIC_API GetHashcode()
    {
        uint32_t hashcode = ToBaseString()->GetRawHashcode();
        // GetLength() == 0 means it's an empty array.No need to computeHashCode again when hashseed is 0.
        if (hashcode == 0 && GetLength() != 0) {
            hashcode = ComputeRawHashcode();
            SetRawHashcode(hashcode);
        }
        return hashcode;
    }

    uint32_t PUBLIC_API ComputeRawHashcode() const;

    static uint32_t ComputeHashcodeUtf8(const uint8_t *utf8Data, size_t utf8Len, bool canBeCompress);
    static uint32_t ComputeHashcodeUtf16(const uint16_t *utf16Data, uint32_t length);

    template<bool verify = true>
    uint16_t At(int32_t index) const;

    // require is LineString
    void WriteData(uint32_t index, uint16_t src);

    // can change left and right data structure
    static int32_t Compare(const EcmaVM *vm, const JSHandle<EcmaString> &left, const JSHandle<EcmaString> &right);

    static bool IsSubStringAt(const EcmaVM *vm, const JSHandle<EcmaString>& left,
        const JSHandle<EcmaString>& right, uint32_t offset);

    // Check that two spans are equal. Should have the same length.
    /* static */
    template<typename T, typename T1>
    static bool StringsAreEquals(Span<const T> &str1, Span<const T1> &str2)
    {
        return BaseString::StringsAreEquals(str1, str2);
    }

    // Compares string1 + string2 by bytes, It doesn't check canonical unicode equivalence.
    bool EqualToSplicedString(const EcmaString *str1, const EcmaString *str2);
    // Compares strings by bytes, It doesn't check canonical unicode equivalence.
    static PUBLIC_API bool StringsAreEqual(const EcmaVM *vm, const JSHandle<EcmaString> &str1,
        const JSHandle<EcmaString> &str2);
    // Compares strings by bytes, It doesn't check canonical unicode equivalence.
    template <RBMode mode = RBMode::DEFAULT_RB>
    static PUBLIC_API bool StringsAreEqual(EcmaString* str1, EcmaString* str2)
    {
        auto readBarrier = [](const void* obj, size_t offset)-> TaggedObject* {
            return Barriers::GetTaggedObject<mode>(obj, offset);
        };
        return BaseString::StringsAreEqual(std::move(readBarrier), str1->ToBaseString(), str2->ToBaseString());
    }

    // Two strings have the same type of utf encoding format.
    static bool StringsAreEqualDiffUtfEncoding(EcmaString *str1, EcmaString *str2);
    static bool StringsAreEqualDiffUtfEncoding(const FlatStringInfo &str1, const FlatStringInfo &str2);
    // Compares strings by bytes, It doesn't check canonical unicode equivalence.
    // not change str1 data structure.
    // if str1 is not flat, this func has low efficiency.
    static bool StringIsEqualUint8Data(const EcmaString *str1, const uint8_t *dataAddr, uint32_t dataLen,
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

    // It allows user to copy into buffer even if maxLength < length
    size_t WriteUtf8(uint8_t *buf, size_t maxLength, bool isWriteBuffer = false) const;

    // It allows user to copy into buffer even if maxLength < length
    size_t WriteUtf16(uint16_t *buf, uint32_t targetLength, uint32_t bufLength) const;

    size_t WriteOneByte(uint8_t *buf, size_t maxLength) const;

    uint32_t CopyDataUtf16(uint16_t *buf, uint32_t maxLength) const;

    std::u16string ToU16String(uint32_t len = 0);

    Span<const uint8_t> ToUtf8Span(CVector<uint8_t> &buf, bool modify = true, bool cesu8 = false);

    Span<const uint8_t> DebuggerToUtf8Span(CVector<uint8_t> &buf, bool modify = true);

    Span<const uint16_t> ToUtf16Span(CVector<uint16_t> &buf)
    {
        Span<const uint16_t> str;
        uint32_t strLen = GetLength();
        if (UNLIKELY(IsUtf16())) {
            const uint16_t *data = EcmaString::GetUtf16DataFlat(this, buf);
            str = Span<const uint16_t>(data, strLen);
        } else {
            CVector<uint8_t> tmpBuf;
            const uint8_t *data = EcmaString::GetUtf8DataFlat(this, tmpBuf);
            buf.reserve(strLen);
            auto utf16Len = common::utf_helper::ConvertRegionUtf8ToUtf16(data, buf.data(), strLen, strLen);
#if !defined(NDEBUG)
            auto calculatedLen = common::utf_helper::Utf8ToUtf16Size(data, strLen);
            ASSERT_PRINT(utf16Len == calculatedLen, "Bad utf8 to utf16 conversion!");
#endif
            str = Span<const uint16_t>(buf.data(), utf16Len);
        }
        return str;
    }

    template <RBMode mode = RBMode::DEFAULT_RB>
    void WriteData(EcmaString *src, uint32_t start, uint32_t destSize, uint32_t length)
    {
        auto readBarrier = [](const void* obj, size_t offset)-> TaggedObject* {
            return Barriers::GetTaggedObject<mode>(obj, offset);
        };
        ToBaseString()->WriteData(std::move(readBarrier), src->ToBaseString(), start, destSize, length);
    }

    static bool CanBeCompressed(const uint8_t *utf8Data, uint32_t utf8Len);
    static bool CanBeCompressed(const uint16_t *utf16Data, uint32_t utf16Len);
    static bool CanBeCompressed(const EcmaString *string);

    bool PUBLIC_API ToElementIndex(uint32_t *index);

    bool ToInt(int32_t *index, bool *negative);

    bool ToUInt64FromLoopStart(uint64_t *index, uint32_t loopStart, const uint8_t *data);

    bool PUBLIC_API ToTypedArrayIndex(uint32_t *index);

    template<typename T>
    static EcmaString *TrimBody(const JSThread *thread, const JSHandle<EcmaString> &src, Span<T> &data, TrimMode mode);

    static EcmaString *Trim(const JSThread *thread, const JSHandle<EcmaString> &src, TrimMode mode = TrimMode::TRIM);

    // memory block copy
    template<typename T>
    static bool MemCopyChars(Span<T> &dst, size_t dstMax, Span<const T> &src, size_t count);

    template<typename T1, typename T2>
    static int32_t IndexOf(Span<const T1> &lhsSp, Span<const T2> &rhsSp, int32_t pos, int32_t max);

    template<typename T1, typename T2>
    static int32_t LastIndexOf(Span<const T1> &lhsSp, Span<const T2> &rhsSp, int32_t pos);

    bool IsFlat() const;

    bool IsLineString() const
    {
        return ToBaseString()->IsLineString();
    }
    bool IsSlicedString() const
    {
        return ToBaseString()->IsSlicedString();
    }
    bool IsTreeString() const
    {
        return ToBaseString()->IsTreeString();
    }

    bool NotTreeString() const
    {
        return !IsTreeString();
    }

    template <typename Char>
    static void WriteToFlat(EcmaString *src, Char *buf, uint32_t maxLength);

    template <typename Char>
    static void WriteToFlatWithPos(EcmaString *src, Char *buf, uint32_t length, uint32_t pos);

    static const uint8_t *PUBLIC_API GetUtf8DataFlat(const EcmaString *src, CVector<uint8_t> &buf);

    static const uint8_t *PUBLIC_API GetNonTreeUtf8Data(const EcmaString *src);

    static const uint16_t *PUBLIC_API GetUtf16DataFlat(const EcmaString *src, CVector<uint16_t> &buf);

    static const uint16_t *PUBLIC_API GetNonTreeUtf16Data(const EcmaString *src);

    // string must be not flat
    static EcmaString *SlowFlatten(const EcmaVM *vm, const JSHandle<EcmaString> &string, MemSpaceType type);

    PUBLIC_API static EcmaString *Flatten(const EcmaVM *vm, const JSHandle<EcmaString> &string,
                               MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE);

    static FlatStringInfo FlattenAllString(const EcmaVM *vm, const JSHandle<EcmaString> &string,
                                            MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE);

    static EcmaString *FlattenNoGCForSnapshot(const EcmaVM *vm, EcmaString *string);

    static EcmaString *ToLower(const EcmaVM *vm, const JSHandle<EcmaString> &src);

    static EcmaString *ToUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src);

    static EcmaString *ToLocaleLower(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale);

    static EcmaString *ToLocaleUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale);

    static EcmaString *TryToLower(const EcmaVM *vm, const JSHandle<EcmaString> &src);

    static EcmaString *TryToUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src);

    static EcmaString *ConvertUtf8ToLowerOrUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src,
                                                 bool toLower, uint32_t startIndex = 0);
};

// The LineEcmaString abstract class captures sequential string values, only LineEcmaString can store chars data
class LineEcmaString : public EcmaString {
private:
    using TaggedObject::SIZE;
public:
    LineString* ToLineString()
    {
        return LineString::Cast(this);
    }

    const LineString* ToLineString() const
    {
        return LineString::ConstCast(this);
    }

    CAST_CHECK(LineEcmaString, IsLineString);

    DECL_VISIT_ARRAY(LineString::DATA_OFFSET, 0, GetPointerLength());

    static LineEcmaString* Cast(EcmaString* str)
    {
        return static_cast<LineEcmaString *>(str);
    }

    static LineEcmaString *Cast(const EcmaString *str)
    {
        return LineEcmaString::Cast(const_cast<EcmaString *>(str));
    }

    static size_t ComputeSizeUtf8(uint32_t utf8Len)
    {
        return LineString::ComputeSizeUtf8(utf8Len);
    }

    static size_t ComputeSizeUtf16(uint32_t utf16Len)
    {
        return LineString::ComputeSizeUtf16(utf16Len);
    }

    static size_t ObjectSize(EcmaString *str)
    {
        return LineString::ObjectSize(str->ToBaseString());
    }

    static size_t DataSize(EcmaString *str)
    {
        return LineString::DataSize(str->ToBaseString());
    }

    size_t GetPointerLength()
    {
        size_t byteSize = DataSize(this);
        return AlignUp(byteSize, static_cast<size_t>(MemAlignment::MEM_ALIGN_OBJECT)) / sizeof(JSTaggedType);
    }

    template<bool verify = true>
    uint16_t Get(int32_t index) const
    {
        return ToLineString()->Get<verify>(index);
    }

    void Set(uint32_t index, uint16_t src)
    {
        return ToLineString()->Set(index, src);
    }
};
// static_assert((LineString::DATA_OFFSET % static_cast<uint8_t>(MemAlignment::MEM_ALIGN_OBJECT)) == 0);

// The substrings of another string use SlicedString to describe.
class SlicedEcmaString : public EcmaString {
private:
    using TaggedObject::SIZE;
public:
    DECL_VISIT_OBJECT(SlicedString::PARENT_OFFSET, SlicedString::STARTINDEX_AND_FLAGS_OFFSET);

    CAST_CHECK(SlicedEcmaString, IsSlicedString);

    SlicedString* ToSlicedString()
    {
        return SlicedString::Cast(this);
    }

    const SlicedString* ToSlicedString() const
    {
        return SlicedString::ConstCast(this);
    }

    static SlicedEcmaString* FromBaseString(SlicedString* str)
    {
        return reinterpret_cast<SlicedEcmaString*>(str);
    }

    uint32_t GetStartIndex() const
    {
        return ToSlicedString()->GetStartIndex();
    }

    void SetStartIndex(uint32_t startIndex)
    {
        ToSlicedString()->SetStartIndex(startIndex);
    }

    bool GetHasBackingStore() const
    {
        return ToSlicedString()->GetHasBackingStore();
    }

    void SetHasBackingStore(bool hasBackingStore)
    {
        return ToSlicedString()->SetHasBackingStore(hasBackingStore);
    }

    JSTaggedValue GetParent() const
    {
        auto readBarrier = [](const void* obj, size_t offset)-> TaggedObject* {
            return Barriers::GetTaggedObject(obj, offset);
        };
        return JSTaggedValue(ToSlicedString()->GetParent<TaggedObject*>(std::move(readBarrier)));
    }

    template <typename T>
    void SetParent(const JSThread* thread, JSHandle<T> value, BarrierMode mode = WRITE_BARRIER)
    {
        auto writeBarrier = [thread, mode](void* obj, size_t offset, BaseObject* str) {
            if (mode == WRITE_BARRIER) {
                Barriers::SetObject<true>(thread, obj, offset, reinterpret_cast<JSTaggedType>(str));
            } else { Barriers::SetPrimitive<JSTaggedType>(obj, offset, reinterpret_cast<JSTaggedType>(str)); }
        };
        ToSlicedString()->SetParent(std::move(writeBarrier), value.GetTaggedValue().GetTaggedObject());
    }

    void SetParent(const JSThread* thread, JSTaggedValue value, BarrierMode mode = WRITE_BARRIER)
    {
        auto writeBarrier = [thread, mode](void* obj, size_t offset, BaseObject* str) {
            if (mode == WRITE_BARRIER) {
                Barriers::SetObject<true>(thread, obj, offset, reinterpret_cast<JSTaggedType>(str));
            } else { Barriers::SetPrimitive<JSTaggedType>(obj, offset, reinterpret_cast<JSTaggedType>(str)); }
        };
        ToSlicedString()->SetParent(std::move(writeBarrier), value.GetTaggedObject());
    };
private:
    friend class EcmaString;
    static SlicedEcmaString *Cast(EcmaString *str)
    {
        return static_cast<SlicedEcmaString *>(str);
    }

    static SlicedEcmaString *Cast(const EcmaString *str)
    {
        return SlicedEcmaString::Cast(const_cast<EcmaString *>(str));
    }

    static size_t ObjectSize()
    {
        return SlicedString::SIZE;
    }

    // Minimum length for a sliced string
    template<bool verify = true>
    uint16_t Get(int32_t index) const
    {
        auto readBarrier = [](const void* obj, size_t offset)-> TaggedObject* {
            return Barriers::GetTaggedObject(obj, offset);
        };
        return ToSlicedString()->Get<verify>(std::move(readBarrier), index);
    }
};

class TreeEcmaString : public EcmaString {
private:
    using TaggedObject::SIZE;
public:
    DECL_VISIT_OBJECT(TreeString::FIRST_OFFSET, TreeString::SIZE);

    CAST_CHECK(TreeEcmaString, IsTreeString);

    static TreeEcmaString* Cast(EcmaString* str)
    {
        return static_cast<TreeEcmaString*>(str);
    }

    static TreeEcmaString* Cast(const EcmaString* str)
    {
        return TreeEcmaString::Cast(const_cast<EcmaString*>(str));
    }

    TreeString* ToTreeString()
    {
        return TreeString::Cast(this);
    }

    const TreeString* ToTreeString() const
    {
        return TreeString::ConstCast(this);
    }

    static TreeEcmaString* FromBaseString(TreeString* str)
    {
        return reinterpret_cast<TreeEcmaString*>(str);
    }

    JSTaggedValue GetFirst() const
    {
        auto readBarrier = [](const void* obj, size_t offset)-> TaggedObject* {
            return Barriers::GetTaggedObject(obj, offset);
        };
        return JSTaggedValue(ToTreeString()->GetFirst<TaggedObject*>(std::move(readBarrier)));
    }

    template <typename T>
    void SetFirst(const JSThread* thread, JSHandle<T> value, BarrierMode mode = WRITE_BARRIER)
    {
        auto writeBarrier = [thread, mode](void* obj, size_t offset, BaseObject* str) {
            if (mode == WRITE_BARRIER) {
                Barriers::SetObject<true>(thread, obj, offset, reinterpret_cast<JSTaggedType>(str));
            } else { Barriers::SetPrimitive<JSTaggedType>(obj, offset, reinterpret_cast<JSTaggedType>(str)); }
        };
        ToTreeString()->SetFirst(std::move(writeBarrier), value->GetTaggedObject());
    }

    void SetFirst(const JSThread* thread, JSTaggedValue value, BarrierMode mode = WRITE_BARRIER)
    {
        auto writeBarrier = [thread, mode](void* obj, size_t offset, BaseObject* str) {
            if (mode == WRITE_BARRIER) {
                Barriers::SetObject<true>(thread, obj, offset, reinterpret_cast<JSTaggedType>(str));
            } else { Barriers::SetPrimitive<JSTaggedType>(obj, offset, reinterpret_cast<JSTaggedType>(str)); }
        };
        ToTreeString()->SetFirst(std::move(writeBarrier), value.GetTaggedObject());
    };

    JSTaggedValue GetSecond() const
    {
        auto readBarrier = [](const void* obj, size_t offset)-> TaggedObject* {
            return Barriers::GetTaggedObject(obj, offset);
        };
        return JSTaggedValue(ToTreeString()->GetSecond<TaggedObject*>(std::move(readBarrier)));
    }

    template <typename T>
    void SetSecond(const JSThread* thread, JSHandle<T> value, BarrierMode mode = WRITE_BARRIER)
    {
        auto writeBarrier = [thread, mode](void* obj, size_t offset, BaseObject* str) {
            if (mode == WRITE_BARRIER) {
                Barriers::SetObject<true>(thread, obj, offset, reinterpret_cast<JSTaggedType>(str));
            } else { Barriers::SetPrimitive<JSTaggedType>(obj, offset, reinterpret_cast<JSTaggedType>(str)); }
        };
        ToTreeString()->SetSecond(std::move(writeBarrier), value->GetTaggedObject());
    }

    void SetSecond(const JSThread* thread, JSTaggedValue value, BarrierMode mode = WRITE_BARRIER)
    {
        auto writeBarrier = [thread, mode](void* obj, size_t offset, BaseObject* str) {
            if (mode == WRITE_BARRIER) {
                Barriers::SetObject<true>(thread, obj, offset, reinterpret_cast<JSTaggedType>(str));
            } else { Barriers::SetPrimitive<JSTaggedType>(obj, offset, reinterpret_cast<JSTaggedType>(str)); }
        };
        ToTreeString()->SetSecond(std::move(writeBarrier), value.GetTaggedObject());
    };

    bool IsFlat() const
    {
        auto readBarrier = [](const void* obj, size_t offset)-> TaggedObject* {
            return Barriers::GetTaggedObject(obj, offset);
        };
        return ToTreeString()->IsFlat(std::move(readBarrier));
    }

    template<bool verify = true>
    uint16_t Get(int32_t index) const
    {
        auto readBarrier = [](const void* obj, size_t offset)-> TaggedObject* {
            return Barriers::GetTaggedObject(obj, offset);
        };
        return ToTreeString()->Get<verify>(std::move(readBarrier), index);
    }
};

// FlatStringInfo holds an EcmaString* instead of a JSHandle. If a GC occurs during its usage period,
// it may cause the pointer to become invalid, necessitating the pointer to be reset.
class FlatStringInfo {
public:
    FlatStringInfo(EcmaString *string, uint32_t startIndex, uint32_t length) : string_(string),
                                                                               startIndex_(startIndex),
                                                                               length_(length) {}
    bool IsUtf8() const
    {
        return string_->IsUtf8();
    }

    bool IsUtf16() const
    {
        return string_->IsUtf16();
    }

    EcmaString *GetString() const
    {
        return string_;
    }

    void SetString(EcmaString *string)
    {
        string_ = string;
    }

    uint32_t GetStartIndex() const
    {
        return startIndex_;
    }

    void SetStartIndex(uint32_t index)
    {
        startIndex_ = index;
    }

    uint32_t GetLength() const
    {
        return length_;
    }

    const uint8_t *GetDataUtf8() const;
    const uint16_t *GetDataUtf16() const;
    uint8_t *GetDataUtf8Writable() const;
    uint16_t *GetDataUtf16Writable() const;
    std::u16string ToU16String(uint32_t len = 0);
private:
    EcmaString *string_ {nullptr};
    uint32_t startIndex_ {0};
    uint32_t length_ {0};
};

// if you want to use functions of EcmaString, please not use directly,
// and use functions of EcmaStringAccessor alternatively.
// eg: EcmaString *str = ***; str->GetLength() ----->  EcmaStringAccessor(str).GetLength()
class PUBLIC_API EcmaStringAccessor {
public:
    explicit inline EcmaStringAccessor(EcmaString *string)
    {
        ASSERT(string != nullptr);
        string_ = string;
    }

    explicit EcmaStringAccessor(TaggedObject *obj);

    explicit EcmaStringAccessor(JSTaggedValue value);

    explicit EcmaStringAccessor(const JSHandle<EcmaString> &strHandle);

    static uint32_t CalculateAllConcatHashCode(const JSHandle<EcmaString> &firstString,
                                               const JSHandle<EcmaString> &secondString)
    {
        return EcmaString::CalculateConcatHashCode(firstString, secondString);
    }

    static EcmaString *CreateLineString(const EcmaVM *vm, size_t length, bool compressed);

    static EcmaString *CreateEmptyString(const EcmaVM *vm)
    {
        return EcmaString::CreateEmptyString(vm);
    }

    static EcmaString *CreateFromUtf8(const EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf8Len, bool canBeCompress,
                                      MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE)
    {
        return EcmaString::CreateFromUtf8(vm, utf8Data, utf8Len, canBeCompress, type);
    }

    static EcmaString *CreateFromUtf8CompressedSubString(const EcmaVM *vm, const JSHandle<EcmaString> &string,
                                                         uint32_t offset, uint32_t utf8Len,
                                                         MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE)
    {
        return EcmaString::CreateFromUtf8CompressedSubString(vm, string, offset, utf8Len, type);
    }

    static EcmaString *CreateUtf16StringFromUtf8(const EcmaVM *vm, const uint8_t *utf8Data, uint32_t utf16Len,
        MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE)
    {
        return EcmaString::CreateUtf16StringFromUtf8(vm, utf8Data, utf16Len, type);
    }

    static EcmaString *CreateFromUtf16(const EcmaVM *vm, const uint16_t *utf16Data, uint32_t utf16Len,
                                       bool canBeCompress, MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE)
    {
        return EcmaString::CreateFromUtf16(vm, utf16Data, utf16Len, canBeCompress, type);
    }

    static EcmaString *Concat(const EcmaVM *vm, const JSHandle<EcmaString> &str1Handle,
        const JSHandle<EcmaString> &str2Handle, MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE)
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
    static bool SubStringIsUtf8(const EcmaVM *vm,
        const JSHandle<EcmaString> &src, uint32_t start, uint32_t length)
    {
        return EcmaString::SubStringIsUtf8(vm, src, start, length);
    }
    // get
    static EcmaString *GetSubString(const EcmaVM *vm,
        const JSHandle<EcmaString> &src, uint32_t start, uint32_t length)
    {
        return EcmaString::GetSubString(vm, src, start, length);
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
    inline size_t GetUtf8Length(bool isGetBufferSize = false) const;

    size_t ObjectSize() const
    {
        if (string_->IsLineString()) {
            return LineEcmaString::ObjectSize(string_);
        } else {
            return TreeString::SIZE;
        }
    }

    // For TreeString, the calculation result is size of LineString correspondingly.
    size_t GetFlatStringSize() const
    {
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
    Span<const uint8_t> ToUtf8Span(CVector<uint8_t> &buf)
    {
        return string_->ToUtf8Span(buf);
    }

    // not change string data structure.
    // if string is not flat, this func has low efficiency.
    std::string ToStdString(StringConvertedUsage usage = StringConvertedUsage::PRINT);

    // this function convert for Utf8
    CString Utf8ConvertToString();

    std::string DebuggerToStdString(StringConvertedUsage usage = StringConvertedUsage::PRINT);
    // not change string data structure.
    // if string is not flat, this func has low efficiency.
    CString ToCString(StringConvertedUsage usage = StringConvertedUsage::LOGICOPERATION, bool cesu8 = false);

#if ENABLE_NEXT_OPTIMIZATION
    void AppendToCString(CString &str);
    void AppendToC16String(C16String &str);
#endif

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

    template <typename Char>
    static void WriteToFlatWithPos(EcmaString *src, Char *buf, uint32_t length, uint32_t pos)
    {
        src->WriteToFlatWithPos(src, buf, length, pos);
    }

    template <typename Char>
    static void WriteToFlat(EcmaString *src, Char *buf, uint32_t maxLength)
    {
        src->WriteToFlat(src, buf, maxLength);
    }

    // require dst is LineString
    // not change src data structure.
    // if src is not flat, this func has low efficiency.
    template <RBMode mode = RBMode::DEFAULT_RB>
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

    uint32_t ComputeHashcode()
    {
        return string_->ComputeRawHashcode();
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


    // can change receiver and search data structure
    static bool IsSubStringAt(const EcmaVM *vm, const JSHandle<EcmaString>& left,
        const JSHandle<EcmaString>& right, uint32_t offset = 0)
    {
        return EcmaString::IsSubStringAt(vm, left, right, offset);
    }

    // can change str1 and str2 data structure
    static bool StringsAreEqual(const EcmaVM *vm, const JSHandle<EcmaString> &str1, const JSHandle<EcmaString> &str2)
    {
        return EcmaString::StringsAreEqual(vm, str1, str2);
    }

    // not change str1 and str2 data structure.
    // if str1 or str2 is not flat, this func has low efficiency.
    template <RBMode mode = RBMode::DEFAULT_RB>
    static bool StringsAreEqual(EcmaString *str1, EcmaString *str2)
    {
        return EcmaString::StringsAreEqual<mode>(str1, str2);
    }

    // not change str1 and str2 data structure.
    // if str1 or str2 is not flat, this func has low efficiency.
    static bool StringsAreEqualDiffUtfEncoding(EcmaString *str1, EcmaString *str2)
    {
        return EcmaString::StringsAreEqualDiffUtfEncoding(str1, str2);
    }

    // not change str1 data structure.
    // if str1 is not flat, this func has low efficiency.
    static bool StringIsEqualUint8Data(const EcmaString *str1, const uint8_t *dataAddr, uint32_t dataLen,
                                       bool canBeCompress)
    {
        return EcmaString::StringIsEqualUint8Data(str1, dataAddr, dataLen, canBeCompress);
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
    bool ToInt(int32_t *index, bool *negative)
    {
        return string_->ToInt(index, negative);
    }

    // not change string data structure.
    // if string is not flat, this func has low efficiency.
    bool PUBLIC_API ToTypedArrayIndex(uint32_t *index)
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

    static EcmaString *TryToUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src)
    {
        return EcmaString::TryToUpper(vm, src);
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

    static bool IsASCIICharacter(uint16_t data)
    {
        if (data == 0) {
            return false;
        }
        // \0 is not considered ASCII in Ecma-Modified-UTF8 [only modify '\u0000']
        return data <= common::utf_helper::UTF8_1B_MAX;
    }

    bool IsFlat() const
    {
        return string_->IsFlat();
    }

    bool IsLineString() const
    {
        return string_->IsLineString();
    }

    bool IsSlicedString() const
    {
        return string_->IsSlicedString();
    }

    bool IsTreeString() const
    {
        return string_->IsTreeString();
    }

    bool NotTreeString() const
    {
        return string_->NotTreeString();
    }

    inline Span<const uint8_t> FastToUtf8Span() const;

    // the returned string may be a linestring or slicestring!!
    PUBLIC_API static EcmaString *Flatten(const EcmaVM *vm, const JSHandle<EcmaString> &string,
        MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE)
    {
        return EcmaString::Flatten(vm, string, type);
    }

    static FlatStringInfo FlattenAllString(const EcmaVM *vm, const JSHandle<EcmaString> &string,
        MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE)
    {
        return EcmaString::FlattenAllString(vm, string, type);
    }

    static EcmaString *SlowFlatten(const EcmaVM *vm, const JSHandle<EcmaString> &string,
        MemSpaceType type = MemSpaceType::SHARED_OLD_SPACE)
    {
        return EcmaString::SlowFlatten(vm, string, type);
    }

    static EcmaString *FlattenNoGCForSnapshot(const EcmaVM *vm, EcmaString *string)
    {
        return EcmaString::FlattenNoGCForSnapshot(vm, string);
    }

    static const uint8_t *GetUtf8DataFlat(const EcmaString *src, CVector<uint8_t> &buf)
    {
        return EcmaString::GetUtf8DataFlat(src, buf);
    }

    static const uint8_t *GetNonTreeUtf8Data(const EcmaString *src)
    {
        return EcmaString::GetNonTreeUtf8Data(src);
    }

    static const uint16_t *GetUtf16DataFlat(const EcmaString *src, CVector<uint16_t> &buf)
    {
        return EcmaString::GetUtf16DataFlat(src, buf);
    }

    static const uint16_t *GetNonTreeUtf16Data(const EcmaString *src)
    {
        return EcmaString::GetNonTreeUtf16Data(src);
    }

    static JSTaggedValue StringToList(JSThread *thread, JSHandle<JSTaggedValue> &str);

private:
    EcmaString *string_ {nullptr};
};
}  // namespace ecmascript
}  // namespace panda
#endif  // ECMASCRIPT_STRING_H
