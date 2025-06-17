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

#include "ecmascript/ecma_string-inl.h"

#include "common_interfaces/objects/base_string.h"
#include "ecmascript/base/json_helper.h"

namespace panda::ecmascript {

constexpr size_t LOW_3BITS = 0x7;
constexpr size_t LOW_4BITS = 0xF;
constexpr size_t LOW_5BITS = 0x1F;
constexpr size_t LOW_6BITS = 0x3F;
constexpr size_t L_SURROGATE_START = 0xDC00;
constexpr size_t H_SURROGATE_START = 0xD800;
constexpr size_t SURROGATE_RAIR_START = 0x10000;
constexpr size_t OFFSET_18POS = 18;
constexpr size_t OFFSET_12POS = 12;
constexpr size_t OFFSET_10POS = 10;
constexpr size_t OFFSET_6POS = 6;

using NumberHelper = base::NumberHelper;

EcmaString *EcmaString::Concat(const EcmaVM *vm,
    const JSHandle<EcmaString> &left, const JSHandle<EcmaString> &right, MemSpaceType type)
{
    ASSERT(IsSMemSpace(type));
    // allocator may trig gc and move src, need to hold it
    EcmaString *strLeft = *left;
    EcmaString *strRight = *right;
    uint32_t leftLength = strLeft->GetLength();
    uint32_t rightLength = strRight->GetLength();
    uint32_t newLength = leftLength + rightLength;
    if (newLength == 0) {
        return vm->GetFactory()->GetEmptyString().GetObject<EcmaString>();
    }

    if (leftLength == 0) {
        return strRight;
    }
    if (rightLength == 0) {
        return strLeft;
    }
    // if the result string is small, make a LineString
    bool compressed = (strLeft->IsUtf8() && strRight->IsUtf8());
    if (newLength < TreeString::MIN_TREE_STRING_LENGTH) {
        ASSERT(strLeft->IsLineString());
        ASSERT(strRight->IsLineString());
        auto newString = CreateLineStringWithSpaceType(vm, newLength, compressed, type);
        // retrieve strings after gc
        strLeft = *left;
        strRight = *right;
        if (compressed) {
            // copy left part
            Span<uint8_t> sp(newString->GetDataUtf8Writable(), newLength);
            Span<const uint8_t> srcLeft(strLeft->GetDataUtf8(), leftLength);
            EcmaString::MemCopyChars(sp, newLength, srcLeft, leftLength);
            // copy right part
            sp = sp.SubSpan(leftLength);
            Span<const uint8_t> srcRight(strRight->GetDataUtf8(), rightLength);
            EcmaString::MemCopyChars(sp, rightLength, srcRight, rightLength);
        } else {
            // copy left part
            Span<uint16_t> sp(newString->GetDataUtf16Writable(), newLength);
            if (strLeft->IsUtf8()) {
                BaseString::CopyChars(sp.data(), strLeft->GetDataUtf8(), leftLength);
            } else {
                Span<const uint16_t> srcLeft(strLeft->GetDataUtf16(), leftLength);
                EcmaString::MemCopyChars(sp, newLength << 1U, srcLeft, leftLength << 1U);
            }
            // copy right part
            sp = sp.SubSpan(leftLength);
            if (strRight->IsUtf8()) {
                BaseString::CopyChars(sp.data(), strRight->GetDataUtf8(), rightLength);
            } else {
                Span<const uint16_t> srcRight(strRight->GetDataUtf16(), rightLength);
                EcmaString::MemCopyChars(sp, rightLength << 1U, srcRight, rightLength << 1U);
            }
        }
        ASSERT_PRINT(compressed == CanBeCompressed(newString), "compressed does not match the real value!");
        return newString;
    }
    return CreateTreeString(vm, left, right, newLength, compressed);
}

/* static */
EcmaString *EcmaString::CopyStringToOldSpace(const EcmaVM *vm, const JSHandle<EcmaString> &original,
    uint32_t length, bool compressed)
{
    JSHandle<EcmaString> newString(vm->GetJSThread(),
        CreateLineStringWithSpaceType(vm, length, compressed, MemSpaceType::OLD_SPACE));
    auto strOrigin = FlattenAllString(vm, original);
    if (compressed) {
        // copy
        Span<uint8_t> sp(newString->GetDataUtf8Writable(), length);
        Span<const uint8_t> srcSp(strOrigin.GetDataUtf8(), length);
        EcmaString::MemCopyChars(sp, length, srcSp, length);
    } else {
        // copy left part
        Span<uint16_t> sp(newString->GetDataUtf16Writable(), length);
        if (strOrigin.IsUtf8()) {
            BaseString::CopyChars(sp.data(), strOrigin.GetDataUtf8(), length);
        } else {
            Span<const uint16_t> srcSp(strOrigin.GetDataUtf16(), length);
            EcmaString::MemCopyChars(sp, length << 1U, srcSp, length << 1U);
        }
    }
    ASSERT_PRINT(compressed == CanBeCompressed(*newString), "compressed does not match the real value!");
    return *newString;
}

/* static */
EcmaString *EcmaString::FastSubString(const EcmaVM *vm,
    const JSHandle<EcmaString> &src, uint32_t start, uint32_t length)
{
    ASSERT((start + length) <= src->GetLength());
    if (length == 0) {
        return *vm->GetFactory()->GetEmptyString();
    }
    if (start == 0 && length == src->GetLength()) {
        return *src;
    }
    if (src->IsUtf8()) {
        return FastSubUtf8String(vm, src, start, length);
    }
    return FastSubUtf16String(vm, src, start, length);
}

/* static */
EcmaString *EcmaString::GetSlicedString(const EcmaVM *vm,
    const JSHandle<EcmaString> &src, uint32_t start, uint32_t length)
{
    ASSERT((start + length) <= src->GetLength());
    FlatStringInfo srcFlat = FlattenAllString(vm, src);
    JSHandle<EcmaString> flatString(vm->GetJSThread(), srcFlat.GetString());
    SlicedEcmaString *slicedString = CreateSlicedString(vm, flatString);
    slicedString->InitLengthAndFlags(length, flatString->IsUtf8());
    slicedString->SetStartIndex(start + srcFlat.GetStartIndex());
    return slicedString;
}

/* static */
EcmaString *EcmaString::GetSubString(const EcmaVM *vm,
    const JSHandle<EcmaString> &src, uint32_t start, uint32_t length)
{
    ASSERT((start + length) <= src->GetLength());
    if (length == 1) {
        JSThread *thread = vm->GetJSThread();
        uint16_t res = EcmaStringAccessor(src).Get<false>(thread, start);
        if (EcmaStringAccessor::CanBeCompressed(&res, 1)) {
            JSHandle<SingleCharTable> singleCharTable(thread, thread->GetSingleCharTable());
            return EcmaString::Cast(singleCharTable->GetStringFromSingleCharTable(thread, res).GetTaggedObject());
        }
    }
    if (static_cast<uint32_t>(length) >= SlicedString::MIN_SLICED_STRING_LENGTH) {
        if (start == 0 && length == src->GetLength()) {
            return *src;
        }
        if (src->IsUtf16()) {
            FlatStringInfo srcFlat = FlattenAllString(vm, src);
            bool canBeCompressed = CanBeCompressed(srcFlat.GetDataUtf16() + start, length);
            if (canBeCompressed) {
                JSHandle<EcmaString> string(vm->GetJSThread(), CreateLineString(vm, length, canBeCompressed));
                srcFlat = FlattenAllString(vm, src);
                BaseString::CopyChars(string->GetDataUtf8Writable(), srcFlat.GetDataUtf16() + start, length);
                return *string;
            }
        }
        return GetSlicedString(vm, src, start, length);
    }
    return FastSubString(vm, src, start, length);
}

bool EcmaString::SubStringIsUtf8(const EcmaVM *vm,
    const JSHandle<EcmaString> &src, uint32_t start, uint32_t length)
{
    ASSERT((start + length) <= src->GetLength());
    if (length == 0) {
        return true;
    }
    if (src->IsUtf8()) {
        return true;
    }
    FlatStringInfo srcFlat = FlattenAllString(vm, src);
    return CanBeCompressed(srcFlat.GetDataUtf16() + start, length);
}

int32_t EcmaString::Compare(const EcmaVM *vm, const JSHandle<EcmaString> &left, const JSHandle<EcmaString> &right)
{
    if (*left == *right) {
        return 0;
    }
    FlatStringInfo lhs = FlattenAllString(vm, left);
    JSHandle<EcmaString> string(vm->GetJSThread(), lhs.GetString());
    FlatStringInfo rhs = FlattenAllString(vm, right);
    lhs.SetString(*string);
    int32_t lhsCount = static_cast<int32_t>(lhs.GetLength());
    int32_t rhsCount = static_cast<int32_t>(rhs.GetLength());
    int32_t countDiff = lhsCount - rhsCount;
    int32_t minCount = (countDiff < 0) ? lhsCount : rhsCount;
    if (!lhs.IsUtf16() && !rhs.IsUtf16()) {
        Span<const uint8_t> lhsSp(lhs.GetDataUtf8(), lhsCount);
        Span<const uint8_t> rhsSp(rhs.GetDataUtf8(), rhsCount);
        int32_t charDiff = common::CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    } else if (!lhs.IsUtf16()) {
        Span<const uint8_t> lhsSp(lhs.GetDataUtf8(), lhsCount);
        Span<const uint16_t> rhsSp(rhs.GetDataUtf16(), rhsCount);
        int32_t charDiff = common::CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    } else if (!rhs.IsUtf16()) {
        Span<const uint16_t> lhsSp(lhs.GetDataUtf16(), rhsCount);
        Span<const uint8_t> rhsSp(rhs.GetDataUtf8(), lhsCount);
        int32_t charDiff = common::CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    } else {
        Span<const uint16_t> lhsSp(lhs.GetDataUtf16(), lhsCount);
        Span<const uint16_t> rhsSp(rhs.GetDataUtf16(), rhsCount);
        int32_t charDiff = common::CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    }
    return countDiff;
}

/**
 * left: text string
 * right: pattern string
 * example 1: IsSubStringAt("IsSubStringAt", "Is", 0) return true
 * example 2: IsSubStringAt("IsSubStringAt", "It", 0) return false
*/
bool EcmaString::IsSubStringAt(const EcmaVM *vm, const JSHandle<EcmaString>& left,
    const JSHandle<EcmaString>& right, uint32_t offset)
{
    FlatStringInfo lhs = FlattenAllString(vm, left);
    JSHandle<EcmaString> string(vm->GetJSThread(), lhs.GetString());
    FlatStringInfo rhs = FlattenAllString(vm, right);
    lhs.SetString(*string);
    int32_t lhsCount = static_cast<int32_t>(lhs.GetLength());
    int32_t rhsCount = static_cast<int32_t>(rhs.GetLength());
    if (!lhs.IsUtf16() && !rhs.IsUtf16()) {
        Span<const uint8_t> lhsSp(lhs.GetDataUtf8(), lhsCount);
        Span<const uint8_t> rhsSp(rhs.GetDataUtf8(), rhsCount);
        return common::IsSubStringAtSpan(lhsSp, rhsSp, offset);
    } else if (!lhs.IsUtf16()) {
        Span<const uint8_t> lhsSp(lhs.GetDataUtf8(), lhsCount);
        Span<const uint16_t> rhsSp(rhs.GetDataUtf16(), rhsCount);
        return common::IsSubStringAtSpan(lhsSp, rhsSp, offset);
    } else if (!rhs.IsUtf16()) {
        Span<const uint16_t> lhsSp(lhs.GetDataUtf16(), lhsCount);
        Span<const uint8_t> rhsSp(rhs.GetDataUtf8(), rhsCount);
        return common::IsSubStringAtSpan(lhsSp, rhsSp, offset);
    } else {
        Span<const uint16_t> lhsSp(lhs.GetDataUtf16(), lhsCount);
        Span<const uint16_t> rhsSp(rhs.GetDataUtf16(), rhsCount);
        return common::IsSubStringAtSpan(lhsSp, rhsSp, offset);
    }
    return false;
}

/* static */
template<typename T1, typename T2>
int32_t EcmaString::IndexOf(Span<const T1> &lhsSp, Span<const T2> &rhsSp, int32_t pos, int32_t max)
{
   return BaseString::IndexOf(lhsSp, rhsSp, pos, max);
}

template<typename T1, typename T2>
int32_t EcmaString::LastIndexOf(Span<const T1> &lhsSp, Span<const T2> &rhsSp, int32_t pos)
{
    return BaseString::LastIndexOf(lhsSp, rhsSp, pos);
}

int32_t EcmaString::IndexOf(const EcmaVM *vm,
    const JSHandle<EcmaString> &receiver, const JSHandle<EcmaString> &search, int pos)
{
    EcmaString *lhstring = *receiver;
    EcmaString *rhstring = *search;
    if (lhstring == nullptr || rhstring == nullptr) {
        return -1;
    }
    int32_t lhsCount = static_cast<int32_t>(lhstring->GetLength());
    int32_t rhsCount = static_cast<int32_t>(rhstring->GetLength());

    if (pos > lhsCount) {
        return -1;
    }

    if (rhsCount == 0) {
        return pos;
    }

    if (pos < 0) {
        pos = 0;
    }

    int32_t max = lhsCount - rhsCount;
    if (max < 0) {
        return -1;
    }

    if (pos + rhsCount > lhsCount) {
        return -1;
    }

    FlatStringInfo lhs = FlattenAllString(vm, receiver);
    JSHandle<EcmaString> string(vm->GetJSThread(), lhs.GetString());
    FlatStringInfo rhs = FlattenAllString(vm, search);
    lhs.SetString(*string);

    if (rhs.IsUtf8() && lhs.IsUtf8()) {
        Span<const uint8_t> lhsSp(lhs.GetDataUtf8(), lhsCount);
        Span<const uint8_t> rhsSp(rhs.GetDataUtf8(), rhsCount);
        return EcmaString::IndexOf(lhsSp, rhsSp, pos, max);
    } else if (rhs.IsUtf16() && lhs.IsUtf16()) {  // NOLINT(readability-else-after-return)
        Span<const uint16_t> lhsSp(lhs.GetDataUtf16(), lhsCount);
        Span<const uint16_t> rhsSp(rhs.GetDataUtf16(), rhsCount);
        return EcmaString::IndexOf(lhsSp, rhsSp, pos, max);
    } else if (rhs.IsUtf16()) {
        return -1;
    } else {  // NOLINT(readability-else-after-return)
        Span<const uint16_t> lhsSp(lhs.GetDataUtf16(), lhsCount);
        Span<const uint8_t> rhsSp(rhs.GetDataUtf8(), rhsCount);
        return EcmaString::IndexOf(lhsSp, rhsSp, pos, max);
    }
}

int32_t EcmaString::LastIndexOf(const EcmaVM *vm,
    const JSHandle<EcmaString> &receiver, const JSHandle<EcmaString> &search, int pos)
{
    EcmaString *lhstring = *receiver;
    EcmaString *rhstring = *search;
    if (lhstring == nullptr || rhstring == nullptr) {
        return -1;
    }

    int32_t lhsCount = static_cast<int32_t>(lhstring->GetLength());
    int32_t rhsCount = static_cast<int32_t>(rhstring->GetLength());
    if (lhsCount < rhsCount) {
        return -1;
    }

    if (pos < 0) {
        pos = 0;
    }

    if (pos > lhsCount) {
        pos = lhsCount;
    }

    if (pos + rhsCount > lhsCount) {
        pos = lhsCount - rhsCount;
    }

    if (rhsCount == 0) {
        return pos;
    }

    FlatStringInfo lhs = FlattenAllString(vm, receiver);
    JSHandle<EcmaString> string(vm->GetJSThread(), lhs.GetString());
    FlatStringInfo rhs = FlattenAllString(vm, search);
    lhs.SetString(*string);
    if (rhs.IsUtf8() && lhs.IsUtf8()) {
        Span<const uint8_t> lhsSp(lhs.GetDataUtf8(), lhsCount);
        Span<const uint8_t> rhsSp(rhs.GetDataUtf8(), rhsCount);
        return EcmaString::LastIndexOf(lhsSp, rhsSp, pos);
    } else if (rhs.IsUtf16() && lhs.IsUtf16()) {  // NOLINT(readability-else-after-return)
        Span<const uint16_t> lhsSp(lhs.GetDataUtf16(), lhsCount);
        Span<const uint16_t> rhsSp(rhs.GetDataUtf16(), rhsCount);
        return EcmaString::LastIndexOf(lhsSp, rhsSp, pos);
    } else if (rhs.IsUtf16()) {
        return -1;
    } else {  // NOLINT(readability-else-after-return)
        Span<const uint16_t> lhsSp(lhs.GetDataUtf16(), lhsCount);
        Span<const uint8_t> rhsSp(rhs.GetDataUtf8(), rhsCount);
        return EcmaString::LastIndexOf(lhsSp, rhsSp, pos);
    }
}

std::u16string EcmaString::ToU16String(const JSThread *thread, uint32_t len)
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return ToBaseString()->ToU16String(std::move(readBarrier), len);
}

// static
#if ENABLE_NEXT_OPTIMIZATION
template<typename T1, typename T2>
uint32_t EcmaString::CalculateDataConcatHashCode(const T1 *dataFirst, size_t sizeFirst,
                                                 const T2 *dataSecond, size_t sizeSecond)
{
    return BaseString::CalculateDataConcatHashCode(dataFirst, sizeFirst, dataSecond, sizeSecond);
}
#else
template<typename T1, typename T2>
uint32_t EcmaString::CalculateDataConcatHashCode(const T1 *dataFirst, size_t sizeFirst,
                                                 const T2 *dataSecond, size_t sizeSecond)
{
    return BaseString::CalculateDataConcatHashCode(dataFirst, sizeFirst, dataSecond, sizeSecond);
}
#endif

// static
uint32_t EcmaString::CalculateConcatHashCode(const JSThread *thread, const JSHandle<EcmaString> &firstString,
                                             const JSHandle<EcmaString> &secondString)
{
    bool isFirstStringUtf8 = EcmaStringAccessor(firstString).IsUtf8();
    bool isSecondStringUtf8 = EcmaStringAccessor(secondString).IsUtf8();
    EcmaString *firstStr = *firstString;
    EcmaString *secondStr = *secondString;
    CVector<uint8_t> bufFirstUint8;
    CVector<uint8_t> bufSecondUint8;
    CVector<uint16_t> bufFirstUint16;
    CVector<uint16_t> bufSecondUint16;
    if (isFirstStringUtf8 && isSecondStringUtf8) {
        const uint8_t *dataFirst = EcmaString::GetUtf8DataFlat(thread, firstStr, bufFirstUint8);
        const uint8_t *dataSecond = EcmaString::GetUtf8DataFlat(thread, secondStr, bufSecondUint8);
        return CalculateDataConcatHashCode(dataFirst, firstStr->GetLength(),
                                           dataSecond, secondStr->GetLength());
    }
    if (!isFirstStringUtf8 && isSecondStringUtf8) {
        const uint16_t *dataFirst = EcmaString::GetUtf16DataFlat(thread, firstStr, bufFirstUint16);
        const uint8_t *dataSecond = EcmaString::GetUtf8DataFlat(thread, secondStr, bufSecondUint8);
        return CalculateDataConcatHashCode(dataFirst, firstStr->GetLength(),
                                           dataSecond, secondStr->GetLength());
    }
    if (isFirstStringUtf8 && !isSecondStringUtf8) {
        const uint8_t *dataFirst = EcmaString::GetUtf8DataFlat(thread, firstStr, bufFirstUint8);
        const uint16_t *dataSecond = EcmaString::GetUtf16DataFlat(thread, secondStr, bufSecondUint16);
        return CalculateDataConcatHashCode(dataFirst, firstStr->GetLength(),
                                           dataSecond, secondStr->GetLength());
    }
    {
        const uint16_t *dataFirst = EcmaString::GetUtf16DataFlat(thread, firstStr, bufFirstUint16);
        const uint16_t *dataSecond = EcmaString::GetUtf16DataFlat(thread, secondStr, bufSecondUint16);
        return  CalculateDataConcatHashCode(dataFirst, firstStr->GetLength(),
                                            dataSecond, secondStr->GetLength());
    }
}

// static
bool EcmaString::CanBeCompressed(const EcmaString *string)
{
    return BaseString::CanBeCompressed(string->ToBaseString());
}

// static
bool EcmaString::CanBeCompressed(const uint8_t *utf8Data, uint32_t utf8Len)
{
    return BaseString::CanBeCompressed(utf8Data, utf8Len);
}

/* static */
bool EcmaString::CanBeCompressed(const uint16_t *utf16Data, uint32_t utf16Len)
{
    return BaseString::CanBeCompressed(utf16Data, utf16Len);
}

bool EcmaString::EqualToSplicedString(const JSThread *thread, const EcmaString *str1, const EcmaString *str2)
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return ToBaseString()->EqualToSplicedString(std::move(readBarrier), str1->ToBaseString(), str2->ToBaseString());
}

/* static */
bool EcmaString::StringsAreEqualDiffUtfEncoding(const JSThread *thread, EcmaString *left, EcmaString *right)
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return BaseString::StringsAreEqualDiffUtfEncoding(std::move(readBarrier), left->ToBaseString(),
                                                      right->ToBaseString());
}

/* static */
bool EcmaString::StringsAreEqualDiffUtfEncoding(const FlatStringInfo &left, const FlatStringInfo &right)
{
    int32_t lhsCount = static_cast<int32_t>(left.GetLength());
    int32_t rhsCount = static_cast<int32_t>(right.GetLength());
    if (!left.IsUtf16() && !right.IsUtf16()) {
        Span<const uint8_t> lhsSp(left.GetDataUtf8(), lhsCount);
        Span<const uint8_t> rhsSp(right.GetDataUtf8(), rhsCount);
        return EcmaString::StringsAreEquals(lhsSp, rhsSp);
    } else if (!left.IsUtf16()) {
        Span<const uint8_t> lhsSp(left.GetDataUtf8(), lhsCount);
        Span<const uint16_t> rhsSp(right.GetDataUtf16(), rhsCount);
        return EcmaString::StringsAreEquals(lhsSp, rhsSp);
    } else if (!right.IsUtf16()) {
        Span<const uint16_t> lhsSp(left.GetDataUtf16(), rhsCount);
        Span<const uint8_t> rhsSp(right.GetDataUtf8(), lhsCount);
        return EcmaString::StringsAreEquals(lhsSp, rhsSp);
    } else {
        Span<const uint16_t> lhsSp(left.GetDataUtf16(), lhsCount);
        Span<const uint16_t> rhsSp(right.GetDataUtf16(), rhsCount);
        return EcmaString::StringsAreEquals(lhsSp, rhsSp);
    }
}

bool EcmaString::StringsAreEqual(const EcmaVM *vm, const JSHandle<EcmaString> &str1, const JSHandle<EcmaString> &str2)
{
    if (str1 == str2) {
        return true;
    }
    if (str1->IsInternString() && str2->IsInternString()) {
        return false;
    }
    uint32_t str1Len = str1->GetLength();
    if (str1Len != str2->GetLength()) {
        return false;
    }
    if (str1Len == 0) {
        return true;
    }

    uint32_t str1Hash;
    uint32_t str2Hash;
    if (str1->TryGetHashCode(&str1Hash) && str2->TryGetHashCode(&str2Hash)) {
        if (str1Hash != str2Hash) {
            return false;
        }
    }
    FlatStringInfo str1Flat = FlattenAllString(vm, str1);
    JSHandle<EcmaString> string(vm->GetJSThread(), str1Flat.GetString());
    FlatStringInfo str2Flat = FlattenAllString(vm, str2);
    str1Flat.SetString(*string);
    return StringsAreEqualDiffUtfEncoding(str1Flat, str2Flat);
}

/* static */
bool EcmaString::StringIsEqualUint8Data(const JSThread *thread, const EcmaString *str1, const uint8_t *dataAddr,
                                        uint32_t dataLen, bool canBeCompressToUtf8)
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return BaseString::StringIsEqualUint8Data(std::move(readBarrier), str1->ToBaseString(), dataAddr, dataLen,
                                              canBeCompressToUtf8);
}

/* static */
bool EcmaString::StringsAreEqualUtf16(const JSThread *thread, const EcmaString *str1, const uint16_t *utf16Data,
                                      uint32_t utf16Len)
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return BaseString::StringsAreEqualUtf16(std::move(readBarrier), str1->ToBaseString(), utf16Data, utf16Len);
}

template<typename T>
bool EcmaString::MemCopyChars(Span<T> &dst, size_t dstMax, Span<const T> &src, size_t count)
{
    ASSERT(dstMax >= count);
    ASSERT(dst.Size() >= src.Size());
    if (memcpy_s(dst.data(), dstMax, src.data(), count) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    return true;
}

// hashSeed only be used when computing two separate strings merged hashcode.
uint32_t EcmaString::ComputeRawHashcode(const JSThread *thread) const
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return ToBaseString()->ComputeRawHashcode(std::move(readBarrier));
}

/* static */
uint32_t EcmaString::ComputeHashcodeUtf8(const uint8_t *utf8Data, size_t utf8Len, bool canBeCompress)
{
    return BaseString::ComputeHashcodeUtf8(utf8Data, utf8Len, canBeCompress);
}

/* static */
uint32_t EcmaString::ComputeHashcodeUtf16(const uint16_t *utf16Data, uint32_t length)
{
    return BaseString::ComputeHashcodeUtf16(utf16Data, length);
}

bool EcmaString::ToElementIndex(const JSThread *thread, uint32_t *index)
{
    uint32_t len = GetLength();
    if (UNLIKELY(len == 0 || len > MAX_ELEMENT_INDEX_LEN)) {  // NOLINTNEXTLINEreadability-magic-numbers)
        return false;
    }
    if (UNLIKELY(IsUtf16())) {
        return false;
    }

    CVector<uint8_t> buf;
    const uint8_t *data = EcmaString::GetUtf8DataFlat(thread, this, buf);
    constexpr uint64_t maxValue = std::numeric_limits<uint32_t>::max() - 1;
    if (NumberHelper::StringToUint<uint32_t, uint8_t>(std::basic_string_view(data, GetLength()), *index, maxValue)) {
        return true;
    }
    return false;
}

bool EcmaString::ToInt(const JSThread *thread, int32_t *index, bool *negative)
{
    uint32_t len = GetLength();
    if (UNLIKELY(len == 0 || len > MAX_ELEMENT_INDEX_LEN)) {  // NOLINTNEXTLINEreadability-magic-numbers)
        return false;
    }
    if (UNLIKELY(IsUtf16())) {
        return false;
    }
    CVector<uint8_t> buf;
    const uint8_t *data = EcmaString::GetUtf8DataFlat(thread, this, buf);
    uint32_t c = data[0];
    uint32_t loopStart = 0;
    uint64_t n = 0;
    if (c == '0') {
        *index = 0;
        return len == 1;
    }
    if (c == '-' && len > 1) {
        *negative = true;
        loopStart = 1;
    }

    if (ToUInt64FromLoopStart(&n, loopStart, data) && n <= std::numeric_limits<int32_t>::max()) {
        *index = *negative ? -n : n;
        return true;
    }
    return false;
}

bool EcmaString::ToUInt64FromLoopStart(uint64_t *index, uint32_t loopStart, const uint8_t *data)
{
    uint64_t n = 0;
    uint32_t len = GetLength();
    if (UNLIKELY(loopStart >= len)) {
        return false;
    }
    for (uint32_t i = loopStart; i < len; i++) {
        uint32_t c = data[i];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (c < '0' || c > '9') {
            return false;
        }
        // NOLINTNEXTLINE(readability-magic-numbers)
        n = n * 10 + (c - '0');  // 10: decimal factor
    }
    *index = n;
    return true;
}

bool EcmaString::ToTypedArrayIndex(const JSThread *thread, uint32_t *index)
{
    uint32_t len = GetLength();
    if (UNLIKELY(len == 0 || len > MAX_ELEMENT_INDEX_LEN)) {
        return false;
    }
    if (UNLIKELY(IsUtf16())) {
        return false;
    }

    CVector<uint8_t> buf;
    const uint8_t *data = EcmaString::GetUtf8DataFlat(thread, this, buf);
    uint32_t c = data[0];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    uint64_t n = 0;
    if (c == '0') {
        *index = 0;
        return len == 1;
    }
    if (c > '0' && c <= '9') {
        n = c - '0';
        for (uint32_t i = 1; i < len; i++) {
            c = data[i];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (c >= '0' && c <= '9') {
                // NOLINTNEXTLINE(readability-magic-numbers)
                n = n * 10 + (c - '0');  // 10: decimal factor
            } else if (c == '.') {
                n = JSObject::MAX_ELEMENT_INDEX;
                break;
            } else {
                return false;
            }
        }
        if (n < JSObject::MAX_ELEMENT_INDEX) {
            *index = n;
            return true;
        } else {
            *index = JSObject::MAX_ELEMENT_INDEX;
            return true;
        }
    } else if (c == '-') {
        *index = JSObject::MAX_ELEMENT_INDEX;
        return true;
    }
    return false;
}

template<typename T>
EcmaString *EcmaString::TrimBody(const JSThread *thread, const JSHandle<EcmaString> &src, Span<T> &data, TrimMode mode)
{
    uint32_t srcLen = src->GetLength();
    int32_t start = 0;
    int32_t end = static_cast<int32_t>(srcLen) - 1;

    if (mode == TrimMode::TRIM || mode == TrimMode::TRIM_START) {
        start = static_cast<int32_t>(base::StringHelper::GetStart(data, srcLen));
    }
    if (mode == TrimMode::TRIM || mode == TrimMode::TRIM_END) {
        end = base::StringHelper::GetEnd(data, start, srcLen);
    }
    EcmaString *res = FastSubString(thread->GetEcmaVM(), src, start, static_cast<uint32_t>(end - start + 1));
    return res;
}

/* static */
EcmaString *EcmaString::ToLower(const EcmaVM *vm, const JSHandle<EcmaString> &src)
{
    auto srcFlat = FlattenAllString(vm, src);
    uint32_t srcLength = srcFlat.GetLength();
    auto factory = vm->GetFactory();
    if (srcFlat.IsUtf16()) {
        std::u16string u16str = base::StringHelper::Utf16ToU16String(srcFlat.GetDataUtf16(), srcLength);
        std::string res = base::StringHelper::ToLower(u16str);
        return *(factory->NewFromStdString(res));
    } else {
        return ConvertUtf8ToLowerOrUpper(vm, src, true);
    }
}

/* static */
EcmaString *EcmaString::TryToLower(const EcmaVM *vm, const JSHandle<EcmaString> &src)
{
    auto srcFlat = FlattenAllString(vm, src);
    uint32_t srcLength = srcFlat.GetLength();
    const char start = 'A';
    const char end = 'Z';
    uint32_t upperIndex = srcLength;
    Span<uint8_t> data(srcFlat.GetDataUtf8Writable(), srcLength);
    for (uint32_t index = 0; index < srcLength; ++index) {
        if (base::StringHelper::Utf8CharInRange(data[index], start, end)) {
            upperIndex = index;
            break;
        }
    }
    if (upperIndex == srcLength) {
        return *src;
    }
    return ConvertUtf8ToLowerOrUpper(vm, src, true, upperIndex);
}

/* static */
EcmaString *EcmaString::TryToUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src)
{
    auto srcFlat = FlattenAllString(vm, src);
    uint32_t srcLength = srcFlat.GetLength();
    const char start = 'a';
    const char end = 'z';
    uint32_t lowerIndex = srcLength;
    Span<uint8_t> data(srcFlat.GetDataUtf8Writable(), srcLength);
    for (uint32_t index = 0; index < srcLength; ++index) {
        if (base::StringHelper::Utf8CharInRange(data[index], start, end)) {
            lowerIndex = index;
            break;
        }
    }
    if (lowerIndex == srcLength) {
        return *src;
    }
    return ConvertUtf8ToLowerOrUpper(vm, src, false, lowerIndex);
}

/* static */
EcmaString *EcmaString::ConvertUtf8ToLowerOrUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src,
                                                  bool toLower, uint32_t startIndex)
{
    const char start = toLower ? 'A' : 'a';
    const char end = toLower ? 'Z' : 'z';
    uint32_t srcLength = src->GetLength();
    JSHandle<EcmaString> newString(vm->GetJSThread(), CreateLineString(vm, srcLength, true));
    auto srcFlat = FlattenAllString(vm, src);
    Span<uint8_t> data(srcFlat.GetDataUtf8Writable(), srcLength);
    auto newStringPtr = newString->GetDataUtf8Writable();
    if (startIndex > 0) {
        if (memcpy_s(newStringPtr, startIndex * sizeof(uint8_t), data.data(), startIndex * sizeof(uint8_t)) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
    for (uint32_t index = startIndex; index < srcLength; ++index) {
        if (base::StringHelper::Utf8CharInRange(data[index], start, end)) {
            *(newStringPtr + index) = data[index] ^ (1 << 5);   // 1 and 5 means lower to upper or upper to lower
        } else {
            *(newStringPtr + index) = data[index];
        }
    }
    return *newString;
}

/* static */
EcmaString *EcmaString::ToUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src)
{
    FlatStringInfo srcFlat = FlattenAllString(vm, src);
    uint32_t srcLength = srcFlat.GetLength();
    auto factory = vm->GetFactory();
    if (srcFlat.IsUtf16()) {
        std::u16string u16str = base::StringHelper::Utf16ToU16String(srcFlat.GetDataUtf16(), srcLength);
        std::string res = base::StringHelper::ToUpper(u16str);
        return *(factory->NewFromStdString(res));
    } else {
        return ConvertUtf8ToLowerOrUpper(vm, src, false);
    }
}

/* static */
EcmaString *EcmaString::ToLocaleLower(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale)
{
    auto factory = vm->GetFactory();
    FlatStringInfo srcFlat = FlattenAllString(vm, src);
    std::u16string utf16 = srcFlat.ToU16String();
    std::string res = base::StringHelper::ToLocaleLower(utf16, locale);
    return *(factory->NewFromStdString(res));
}

/* static */
EcmaString *EcmaString::ToLocaleUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale)
{
    auto factory = vm->GetFactory();
    FlatStringInfo srcFlat = FlattenAllString(vm, src);
    std::u16string utf16 = srcFlat.ToU16String();
    std::string res = base::StringHelper::ToLocaleUpper(utf16, locale);
    return *(factory->NewFromStdString(res));
}

EcmaString *EcmaString::Trim(const JSThread *thread, const JSHandle<EcmaString> &src, TrimMode mode)
{
    FlatStringInfo srcFlat = FlattenAllString(thread->GetEcmaVM(), src);
    uint32_t srcLen = srcFlat.GetLength();
    if (UNLIKELY(srcLen == 0)) {
        return EcmaString::Cast(thread->GlobalConstants()->GetEmptyString().GetTaggedObject());
    }
    if (srcFlat.IsUtf8()) {
        Span<const uint8_t> data(srcFlat.GetDataUtf8(), srcLen);
        return TrimBody(thread, src, data, mode);
    } else {
        Span<const uint16_t> data(srcFlat.GetDataUtf16(), srcLen);
        return TrimBody(thread, src, data, mode);
    }
}

EcmaString *EcmaString::SlowFlatten(const EcmaVM *vm, const JSHandle<EcmaString> &string, MemSpaceType type)
{
    ASSERT(string->IsTreeString() || string->IsSlicedString());
    ASSERT(IsSMemSpace(type));
    auto thread = vm->GetJSThread();
    uint32_t length = string->GetLength();
    EcmaString *result = nullptr;
    if (string->IsUtf8()) {
        result = CreateLineStringWithSpaceType(vm, length, true, type);
        WriteToFlat<uint8_t>(thread, *string, result->GetDataUtf8Writable(), length);
    } else {
        result = CreateLineStringWithSpaceType(vm, length, false, type);
        WriteToFlat<uint16_t>(thread, *string, result->GetDataUtf16Writable(), length);
    }
    if (string->IsTreeString()) {
        JSHandle<TreeEcmaString> tree(string);
        ASSERT(EcmaString::Cast(tree->GetSecond(thread))->GetLength() != 0);
        tree->SetFirst(thread, JSTaggedValue(result));
        tree->SetSecond(thread, JSTaggedValue(*vm->GetFactory()->GetEmptyString()));
    }
    return result;
}

EcmaString *EcmaString::Flatten(const EcmaVM *vm, const JSHandle<EcmaString> &string, MemSpaceType type)
{
    EcmaString *s = *string;
    if (!s->IsTreeString()) {
        return s;
    }
    JSThread *thread = vm->GetJSThread();
    JSHandle<TreeEcmaString> tree = JSHandle<TreeEcmaString>::Cast(string);
    if (!tree->IsFlat(thread)) {
        return SlowFlatten(vm, string, type);
    }
    return EcmaString::Cast(tree->GetFirst(thread));
}

FlatStringInfo EcmaString::FlattenAllString(const EcmaVM *vm, const JSHandle<EcmaString> &string, MemSpaceType type)
{
    ASSERT(IsSMemSpace(type));
    EcmaString *s = *string;
    uint32_t startIndex = 0;
    if (s->IsLineString()) {
        return FlatStringInfo(s, startIndex, s->GetLength());
    }
    JSThread *thread = vm->GetJSThread();
    if (string->IsTreeString()) {
        JSHandle<TreeEcmaString> tree = JSHandle<TreeEcmaString>::Cast(string);
        if (!tree->IsFlat(thread)) {
            s = SlowFlatten(vm, string, type);
        } else {
            s = EcmaString::Cast(tree->GetFirst(thread));
        }
    } else if (string->IsSlicedString()) {
        s = EcmaString::Cast(SlicedEcmaString::Cast(*string)->GetParent(thread));
        startIndex = SlicedEcmaString::Cast(*string)->GetStartIndex();
    }
    return FlatStringInfo(s, startIndex, string->GetLength());
}

EcmaString *EcmaString::FlattenNoGCForSnapshot(const EcmaVM *vm, EcmaString *string)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (string->IsLineString()) {
        return string;
    }
    if (string->IsTreeString()) {
        TreeEcmaString *tree = TreeEcmaString::Cast(string);
        JSThread *thread = vm->GetJSThread();
        if (tree->IsFlat(thread)) {
            string = EcmaString::Cast(tree->GetFirst(thread));
        } else {
            uint32_t length = tree->GetLength();
            EcmaString *result = nullptr;
            if (tree->IsUtf8()) {
                result = CreateLineStringNoGC(vm, length, true);
                WriteToFlat<uint8_t>(thread, tree, result->GetDataUtf8Writable(), length);
            } else {
                result = CreateLineStringNoGC(vm, length, false);
                WriteToFlat<uint16_t>(thread, tree, result->GetDataUtf16Writable(), length);
            }
            tree->SetFirst(vm->GetJSThread(), JSTaggedValue(result));
            tree->SetSecond(vm->GetJSThread(), JSTaggedValue(*vm->GetFactory()->GetEmptyString()));
            return result;
        }
    } else if (string->IsSlicedString()) {
        SlicedEcmaString *str = SlicedEcmaString::Cast(string);
        uint32_t length = str->GetLength();
        JSThread *thread = vm->GetJSThread();
        EcmaString *result = nullptr;
        if (str->IsUtf8()) {
            result = CreateLineStringNoGC(vm, length, true);
            WriteToFlat<uint8_t>(thread, str, result->GetDataUtf8Writable(), length);
        } else {
            result = CreateLineStringNoGC(vm, length, false);
            WriteToFlat<uint16_t>(thread, str, result->GetDataUtf16Writable(), length);
        }
        return result;
    }
    return string;
}

const uint8_t *EcmaString::GetUtf8DataFlat(const JSThread *thread, const EcmaString *src, CVector<uint8_t> &buf)
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return BaseString::GetUtf8DataFlat(std::move(readBarrier), src->ToBaseString(), buf);
}

const uint8_t *EcmaString::GetNonTreeUtf8Data(const JSThread *thread, const EcmaString *src)
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return BaseString::GetNonTreeUtf8Data(std::move(readBarrier), src->ToBaseString());
}

const uint16_t *EcmaString::GetUtf16DataFlat(const JSThread *thread, const EcmaString *src, CVector<uint16_t> &buf)
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return BaseString::GetUtf16DataFlat(std::move(readBarrier), src->ToBaseString(), buf);
}

const uint16_t *EcmaString::GetNonTreeUtf16Data(const JSThread *thread, const EcmaString *src)
{
    auto readBarrier = [thread](const void *obj, size_t offset) -> TaggedObject * {
        return Barriers::GetTaggedObject(thread, obj, offset);
    };
    return BaseString::GetNonTreeUtf16Data(std::move(readBarrier), src->ToBaseString());
}

std::u16string FlatStringInfo::ToU16String(uint32_t len)
{
    uint32_t length = len > 0 ? len : GetLength();
    std::u16string result;
    if (IsUtf16()) {
        const uint16_t *data = this->GetDataUtf16();
        result = base::StringHelper::Utf16ToU16String(data, length);
    } else {
        const uint8_t *data = this->GetDataUtf8();
        result = base::StringHelper::Utf8ToU16String(data, length);
    }
    return result;
}

EcmaStringAccessor::EcmaStringAccessor(TaggedObject *obj)
{
    ASSERT(obj != nullptr);
    string_ = EcmaString::Cast(obj);
}

EcmaStringAccessor::EcmaStringAccessor(JSTaggedValue value)
{
    ASSERT(value.IsString());
    string_ = EcmaString::Cast(value.GetTaggedObject());
}

EcmaStringAccessor::EcmaStringAccessor(const JSHandle<EcmaString> &strHandle)
    : string_(*strHandle)
{
}

std::string EcmaStringAccessor::ToStdString(const JSThread *thread, StringConvertedUsage usage)
{
    if (string_ == nullptr) {
        return "";
    }
    bool modify = (usage != StringConvertedUsage::PRINT);
    CVector<uint8_t> buf;
    Span<const uint8_t> sp = string_->ToUtf8Span(thread, buf, modify);
#if ENABLE_NEXT_OPTIMIZATION
    return std::string(reinterpret_cast<const char*>(sp.data()), sp.size());
#else
    std::string res;
    res.reserve(sp.size());
    for (const auto &c : sp) {
        res.push_back(c);
    }
    return res;
#endif
}

CString EcmaStringAccessor::Utf8ConvertToString(const JSThread *thread)
{
    if (string_ == nullptr) {
        return CString("");
    }
    if (IsUtf8()) {
        std::string stdStr;
        if (IsLineString()) {
            return base::StringHelper::Utf8ToCString(GetDataUtf8(), GetLength());
        }
        CVector<uint8_t> buf;
        const uint8_t *data = EcmaString::GetUtf8DataFlat(thread, string_, buf);
        return base::StringHelper::Utf8ToCString(data, GetLength());
    } else {
        return ToCString(thread);
    }
}

std::string EcmaStringAccessor::DebuggerToStdString(const JSThread *thread, StringConvertedUsage usage)
{
    if (string_ == nullptr) {
        return "";
    }

    bool modify = (usage != StringConvertedUsage::PRINT);
    CVector<uint8_t> buf;
    Span<const uint8_t> sp = string_->DebuggerToUtf8Span(thread, buf, modify);
#if ENABLE_NEXT_OPTIMIZATION
    return std::string(reinterpret_cast<const char*>(sp.data()), sp.size());
#else
    std::string res;
    res.reserve(sp.size());
    for (const auto &c : sp) {
        res.push_back(c);
    }
    return res;
#endif
}

CString EcmaStringAccessor::ToCString(const JSThread *thread, StringConvertedUsage usage, bool cesu8)
{
    if (string_ == nullptr) {
        return "";
    }
    bool modify = (usage != StringConvertedUsage::PRINT);
    CVector<uint8_t> buf;
    Span<const uint8_t> sp = string_->ToUtf8Span(thread, buf, modify, cesu8);
#if ENABLE_NEXT_OPTIMIZATION
    return CString(reinterpret_cast<const char*>(sp.data()), sp.size());
#else
    CString res;
    res.reserve(sp.size());
    for (const auto &c : sp) {
        res.push_back(c);
    }
    return res;
#endif
}

#if ENABLE_NEXT_OPTIMIZATION
void EcmaStringAccessor::AppendToCString(const JSThread *thread, CString &str)
{
    if (string_ == nullptr) {
        return;
    }

    size_t strLen = GetLength();
    CVector<uint8_t> buf;
    const uint8_t *data = EcmaString::GetUtf8DataFlat(thread, string_, buf);
    str.append(reinterpret_cast<const char *>(data), strLen);
}

void EcmaStringAccessor::AppendToC16String(const JSThread *thread, C16String &str)
{
    if (string_ == nullptr) {
        return;
    }
    // used to append utf8 space to utf16 gap by stringify
    // In real world, space is usually utf8.
    if LIKELY(string_->IsUtf8()) {
        CVector<uint8_t> buf;
        const uint8_t *data = EcmaString::GetUtf8DataFlat(thread, string_, buf);
        // only ascii codes, no need to convert to UTF-16, just append.
        AppendString(str, reinterpret_cast<const char*>(data), GetLength());
    } else {
        CVector<uint16_t> buf;
        const uint16_t *data = EcmaString::GetUtf16DataFlat(thread, string_, buf);
        str.append(reinterpret_cast<const char16_t *>(data), GetLength());
    }
}
#endif
// static
EcmaString *EcmaStringAccessor::CreateLineString(const EcmaVM *vm, size_t length, bool compressed)
{
    return EcmaString::CreateLineString(vm, length, compressed);
}
}  // namespace panda::ecmascript
