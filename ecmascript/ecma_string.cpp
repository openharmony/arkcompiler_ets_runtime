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

#include "ecmascript/ecma_string_table.h"
#include "ecmascript/platform/ecma_string_hash.h"

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
    if (newLength < TreeEcmaString::MIN_TREE_ECMASTRING_LENGTH) {
        ASSERT(strLeft->IsLineOrConstantString());
        ASSERT(strRight->IsLineOrConstantString());
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
                EcmaString::CopyChars(sp.data(), strLeft->GetDataUtf8(), leftLength);
            } else {
                Span<const uint16_t> srcLeft(strLeft->GetDataUtf16(), leftLength);
                EcmaString::MemCopyChars(sp, newLength << 1U, srcLeft, leftLength << 1U);
            }
            // copy right part
            sp = sp.SubSpan(leftLength);
            if (strRight->IsUtf8()) {
                EcmaString::CopyChars(sp.data(), strRight->GetDataUtf8(), rightLength);
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
    if (original->IsConstantString()) {
        return CreateConstantString(vm, original->GetDataUtf8(), length, MemSpaceType::OLD_SPACE);
    }
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
            EcmaString::CopyChars(sp.data(), strOrigin.GetDataUtf8(), length);
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
    JSHandle<SlicedString> slicedString(vm->GetJSThread(), CreateSlicedString(vm));
    FlatStringInfo srcFlat = FlattenAllString(vm, src);
    slicedString->SetLength(length, srcFlat.GetString()->IsUtf8());
    slicedString->SetParent(vm->GetJSThread(), JSTaggedValue(srcFlat.GetString()));
    slicedString->SetStartIndex(start + srcFlat.GetStartIndex());
    return *slicedString;
}

/* static */
EcmaString *EcmaString::GetSubString(const EcmaVM *vm,
    const JSHandle<EcmaString> &src, uint32_t start, uint32_t length)
{
    ASSERT((start + length) <= src->GetLength());
    if (length == 1) {
        JSThread *thread = vm->GetJSThread();
        uint16_t res = EcmaStringAccessor(src).Get<false>(start);
        if (EcmaStringAccessor::CanBeCompressed(&res, 1)) {
            JSHandle<SingleCharTable> singleCharTable(thread, thread->GetSingleCharTable());
            return EcmaString::Cast(singleCharTable->GetStringFromSingleCharTable(res).GetTaggedObject());
        }
    }
    if (static_cast<uint32_t>(length) >= SlicedString::MIN_SLICED_ECMASTRING_LENGTH) {
        if (start == 0 && length == src->GetLength()) {
            return *src;
        }
        if (src->IsUtf16()) {
            FlatStringInfo srcFlat = FlattenAllString(vm, src);
            bool canBeCompressed = CanBeCompressed(srcFlat.GetDataUtf16() + start, length);
            if (canBeCompressed) {
                JSHandle<EcmaString> string(vm->GetJSThread(), CreateLineString(vm, length, canBeCompressed));
                srcFlat = FlattenAllString(vm, src);
                CopyChars(string->GetDataUtf8Writable(), srcFlat.GetDataUtf16() + start, length);
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

void EcmaString::WriteData(EcmaString *src, uint32_t start, uint32_t destSize, uint32_t length)
{
    ASSERT(IsLineString() && !IsConstantString());
    if (IsUtf8()) {
        ASSERT(src->IsUtf8());
        CVector<uint8_t> buf;
        const uint8_t *data = EcmaString::GetUtf8DataFlat(src, buf);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (length != 0 && memcpy_s(GetDataUtf8Writable() + start, destSize, data, length) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    } else if (src->IsUtf8()) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        CVector<uint8_t> buf;
        const uint8_t *data = EcmaString::GetUtf8DataFlat(src, buf);
        Span<uint16_t> to(GetDataUtf16Writable() + start, length);
        Span<const uint8_t> from(data, length);
        for (uint32_t i = 0; i < length; i++) {
            to[i] = from[i];
        }
    } else {
        CVector<uint16_t> buf;
        const uint16_t *data = EcmaString::GetUtf16DataFlat(src, buf);
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (length != 0 && memcpy_s(GetDataUtf16Writable() + start,
            destSize * sizeof(uint16_t), data, length * sizeof(uint16_t)) != EOK) {
            LOG_FULL(FATAL) << "memcpy_s failed";
            UNREACHABLE();
        }
    }
}

template<typename T1, typename T2>
int32_t CompareStringSpan(Span<T1> &lhsSp, Span<T2> &rhsSp, int32_t count)
{
    for (int32_t i = 0; i < count; ++i) {
        auto left = static_cast<int32_t>(lhsSp[i]);
        auto right = static_cast<int32_t>(rhsSp[i]);
        if (left != right) {
            return left - right;
        }
    }
    return 0;
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
        int32_t charDiff = CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    } else if (!lhs.IsUtf16()) {
        Span<const uint8_t> lhsSp(lhs.GetDataUtf8(), lhsCount);
        Span<const uint16_t> rhsSp(rhs.GetDataUtf16(), rhsCount);
        int32_t charDiff = CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    } else if (!rhs.IsUtf16()) {
        Span<const uint16_t> lhsSp(lhs.GetDataUtf16(), rhsCount);
        Span<const uint8_t> rhsSp(rhs.GetDataUtf8(), lhsCount);
        int32_t charDiff = CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    } else {
        Span<const uint16_t> lhsSp(lhs.GetDataUtf16(), lhsCount);
        Span<const uint16_t> rhsSp(rhs.GetDataUtf16(), rhsCount);
        int32_t charDiff = CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    }
    return countDiff;
}

template<typename T1, typename T2>
bool IsSubStringAtSpan(Span<T1> &lhsSp, Span<T2> &rhsSp, uint32_t offset)
{
    int rhsSize = static_cast<int>(rhsSp.size());
    ASSERT(rhsSize + offset <= lhsSp.size());
    for (int i = 0; i < rhsSize; ++i) {
        auto left = static_cast<int32_t>(lhsSp[offset + static_cast<uint32_t>(i)]);
        auto right = static_cast<int32_t>(rhsSp[i]);
        if (left != right) {
            return false;
        }
    }
    return true;
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
        return IsSubStringAtSpan(lhsSp, rhsSp, offset);
    } else if (!lhs.IsUtf16()) {
        Span<const uint8_t> lhsSp(lhs.GetDataUtf8(), lhsCount);
        Span<const uint16_t> rhsSp(rhs.GetDataUtf16(), rhsCount);
        return IsSubStringAtSpan(lhsSp, rhsSp, offset);
    } else if (!rhs.IsUtf16()) {
        Span<const uint16_t> lhsSp(lhs.GetDataUtf16(), lhsCount);
        Span<const uint8_t> rhsSp(rhs.GetDataUtf8(), rhsCount);
        return IsSubStringAtSpan(lhsSp, rhsSp, offset);
    } else {
        Span<const uint16_t> lhsSp(lhs.GetDataUtf16(), lhsCount);
        Span<const uint16_t> rhsSp(rhs.GetDataUtf16(), rhsCount);
        return IsSubStringAtSpan(lhsSp, rhsSp, offset);
    }
    return false;
}

/* static */
template<typename T1, typename T2>
int32_t EcmaString::IndexOf(Span<const T1> &lhsSp, Span<const T2> &rhsSp, int32_t pos, int32_t max)
{
    ASSERT(rhsSp.size() > 0);
    auto first = static_cast<int32_t>(rhsSp[0]);
    for (int32_t i = pos; i <= max; i++) {
        if (static_cast<int32_t>(lhsSp[i]) != first) {
            i++;
            while (i <= max && static_cast<int32_t>(lhsSp[i]) != first) {
                i++;
            }
        }
        /* Found first character, now look at the rest of rhsSp */
        if (i <= max) {
            int j = i + 1;
            int end = j + static_cast<int>(rhsSp.size()) - 1;

            for (int k = 1; j < end && static_cast<int32_t>(lhsSp[j]) == static_cast<int32_t>(rhsSp[k]); j++, k++) {
            }
            if (j == end) {
                /* Found whole string. */
                return i;
            }
        }
    }
    return -1;
}

template<typename T1, typename T2>
int32_t EcmaString::LastIndexOf(Span<const T1> &lhsSp, Span<const T2> &rhsSp, int32_t pos)
{
    int rhsSize = static_cast<int>(rhsSp.size());
    ASSERT(rhsSize > 0);
    auto first = rhsSp[0];
    for (int32_t i = pos; i >= 0; i--) {
        if (lhsSp[i] != first) {
            continue;
        }
        /* Found first character, now look at the rest of rhsSp */
        int j = 1;
        while (j < rhsSize) {
            if (rhsSp[j] != lhsSp[i + j]) {
                break;
            }
            j++;
        }
        if (j == rhsSize) {
            return i;
        }
    }
    return -1;
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

std::u16string EcmaString::ToU16String(uint32_t len)
{
    uint32_t length = len > 0 ? len : GetLength();
    std::u16string result;
    if (IsUtf16()) {
        CVector<uint16_t> buf;
        const uint16_t *data = EcmaString::GetUtf16DataFlat(this, buf);
        result = base::StringHelper::Utf16ToU16String(data, length);
    } else {
        CVector<uint8_t> buf;
        const uint8_t *data = EcmaString::GetUtf8DataFlat(this, buf);
        result = base::StringHelper::Utf8ToU16String(data, length);
    }
    return result;
}

//static
uint32_t EcmaString::CalculateAllConcatHashCode(const JSHandle<EcmaString> &firstString,
                                                const JSHandle<EcmaString> &secondString)
{
    uint32_t hashCode;
    uint32_t firstLength = firstString->GetLength();
    uint32_t secondLength = secondString->GetLength();
    if ((firstLength + secondLength < MAX_ELEMENT_INDEX_LEN) &&
        firstString->IsUtf8() && secondString->IsUtf8() &&
        firstString->IsInteger() && secondString->IsInteger()) {
            firstString->HashIntegerString(firstLength, &hashCode, 0);
            secondString->HashIntegerString(secondLength, &hashCode, hashCode);
            return hashCode;
    }
    hashCode = EcmaString::CalculateConcatHashCode(firstString, secondString);
    hashCode = MixHashcode(hashCode, NOT_INTEGER);
    return hashCode;
}

// static
template<typename T1, typename T2>
uint32_t EcmaString::CalculateDataConcatHashCode(const T1 *dataFirst, size_t sizeFirst,
                                                 const T2 *dataSecond, size_t sizeSecond)
{
    uint32_t totalHash = 0;
    constexpr uint32_t hashShift = static_cast<uint32_t>(EcmaStringHash::HASH_SHIFT);
    constexpr uint32_t blockSize = static_cast<size_t>(EcmaStringHash::BLOCK_SIZE);
    // The concatenated length of the two strings is less than MIN_SIZE_FOR_UNROLLING.
    if (sizeFirst + sizeSecond <= static_cast<size_t>(EcmaStringHash::MIN_SIZE_FOR_UNROLLING)) {
        for (uint32_t i = 0; i < sizeFirst; i++) {
            totalHash = (totalHash << hashShift) - totalHash + dataFirst[i];
        }
        for (uint32_t i = 0; i < sizeSecond; i++) {
            totalHash = (totalHash << hashShift) - totalHash + dataSecond[i];
        }
        return totalHash;
    }
    // Process the entire block of the first string.
    uint32_t hash[blockSize] = {0};
    uint32_t index = 0;
    for (; index + blockSize <= sizeFirst; index += blockSize) {
        hash[0] = (hash[0] << hashShift) - hash[0] + dataFirst[index];
        hash[1] = (hash[1] << hashShift) - hash[1] + dataFirst[index + 1]; // 1: the second element
        hash[2] = (hash[2] << hashShift) - hash[2] + dataFirst[index + 2]; // 2: the third element
        hash[3] = (hash[3] << hashShift) - hash[3] + dataFirst[index + 3]; // 3: the fourth element
    }
    // The remaining total string length is less than a whole block.
    if ((sizeFirst % blockSize) + sizeSecond < blockSize) {
        for (; index < sizeFirst; ++index) {
            hash[0] = (hash[0] << hashShift) - hash[0] + dataFirst[index];
        }
        index = 0;
    } else {
        //Calculate the non-integral block portion at the end of the first string.
        for (; index < sizeFirst; ++index) {
            hash[index % blockSize] = (hash[index % blockSize] << hashShift) -
                                        hash[index % blockSize] + dataFirst[index];
        }
        //Calculate the portion of the second string
        //that starts and aligns with an integral block at the end of the first string.
        uint32_t wholeBlockRemain = (blockSize - sizeFirst % blockSize) % blockSize;
        index = 0;
        for (; index < wholeBlockRemain && index < sizeSecond; ++index) {
            uint32_t nowHashIndex = sizeFirst % blockSize + index;
            hash[nowHashIndex] = (hash[nowHashIndex] << hashShift) - hash[nowHashIndex] + dataSecond[index];
        }
        // Process the entire block of the Second string.
        for (; index + blockSize <= sizeSecond; index += blockSize) {
            hash[0] = (hash[0] << hashShift) - hash[0] + dataSecond[index];
            hash[1] = (hash[1] << hashShift) - hash[1] + dataSecond[index + 1]; // 1: the second element
            hash[2] = (hash[2] << hashShift) - hash[2] + dataSecond[index + 2]; // 2: the third element
            hash[3] = (hash[3] << hashShift) - hash[3] + dataSecond[index + 3]; // 3: the fourth element
        }
    }
    for (; index < sizeSecond; ++index) {
        hash[0] = (hash[0] << hashShift) - hash[0] + dataSecond[index];
    }
    for (uint32_t i = 0; i < blockSize; ++i) {
        totalHash = (totalHash << hashShift) - totalHash + hash[i];
    }
    return totalHash;
}

// static
uint32_t EcmaString::CalculateConcatHashCode(const JSHandle<EcmaString> &firstString,
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
        const uint8_t *dataFirst = EcmaString::GetUtf8DataFlat(firstStr, bufFirstUint8);
        const uint8_t *dataSecond = EcmaString::GetUtf8DataFlat(secondStr, bufSecondUint8);
        return CalculateDataConcatHashCode(dataFirst, firstStr->GetLength(),
                                           dataSecond, secondStr->GetLength());
    }
    if (!isFirstStringUtf8 && isSecondStringUtf8) {
        const uint16_t *dataFirst = EcmaString::GetUtf16DataFlat(firstStr, bufFirstUint16);
        const uint8_t *dataSecond = EcmaString::GetUtf8DataFlat(secondStr, bufSecondUint8);
        return CalculateDataConcatHashCode(dataFirst, firstStr->GetLength(),
                                           dataSecond, secondStr->GetLength());
    }
    if (isFirstStringUtf8 && !isSecondStringUtf8) {
        const uint8_t *dataFirst = EcmaString::GetUtf8DataFlat(firstStr, bufFirstUint8);
        const uint16_t *dataSecond = EcmaString::GetUtf16DataFlat(secondStr, bufSecondUint16);
        return CalculateDataConcatHashCode(dataFirst, firstStr->GetLength(),
                                           dataSecond, secondStr->GetLength());
    }
    {
        const uint16_t *dataFirst = EcmaString::GetUtf16DataFlat(firstStr, bufFirstUint16);
        const uint16_t *dataSecond = EcmaString::GetUtf16DataFlat(secondStr, bufSecondUint16);
        return  CalculateDataConcatHashCode(dataFirst, firstStr->GetLength(),
                                            dataSecond, secondStr->GetLength());
    }
}

// static
bool EcmaString::CanBeCompressed(const EcmaString *string)
{
    ASSERT(string->IsLineOrConstantString());
    if (string->IsUtf8()) {
        return CanBeCompressed(string->GetDataUtf8(), string->GetLength());
    }
    return CanBeCompressed(string->GetDataUtf16(), string->GetLength());
}

// static
bool EcmaString::CanBeCompressed(const uint8_t *utf8Data, uint32_t utf8Len)
{
    uint32_t index = 0;
    for (; index + 4 <= utf8Len; index += 4) { // 4: process the data in chunks of 4 elements to improve speed
        // Check if all four characters in the current block are ASCII characters
        if (!IsASCIICharacter(utf8Data[index]) ||
            !IsASCIICharacter(utf8Data[index + 1]) || // 1: the second element of the block
            !IsASCIICharacter(utf8Data[index + 2]) || // 2: the third element of the block
            !IsASCIICharacter(utf8Data[index + 3])) { // 3: the fourth element of the block
            return false;
        }
    }
    // Check remaining characters if they are ASCII
    for (; index < utf8Len; ++index) {
        if (!IsASCIICharacter(utf8Data[index])) {
            return false;
        }
    }
    return true;
}

/* static */
bool EcmaString::CanBeCompressed(const uint16_t *utf16Data, uint32_t utf16Len)
{
    uint32_t index = 0;
    for (; index + 4 <= utf16Len; index += 4) { // 4: process the data in chunks of 4 elements to improve speed
        // Check if all four characters in the current block are ASCII characters
        if (!IsASCIICharacter(utf16Data[index]) ||
            !IsASCIICharacter(utf16Data[index + 1]) || // 1: the second element of the block
            !IsASCIICharacter(utf16Data[index + 2]) || // 2: the third element of the block
            !IsASCIICharacter(utf16Data[index + 3])) { // 3: the fourth element of the block
            return false;
        }
    }
    // Check remaining characters if they are ASCII
    for (; index < utf16Len; ++index) {
        if (!IsASCIICharacter(utf16Data[index])) {
            return false;
        }
    }
    return true;
}

bool EcmaString::EqualToSplicedString(const EcmaString *str1, const EcmaString *str2)
{
    ASSERT(NotTreeString());
    ASSERT(str1->NotTreeString() && str2->NotTreeString());
    if (GetLength() != str1->GetLength() + str2->GetLength()) {
        return false;
    }
    if (IsUtf16()) {
        CVector<uint16_t> buf;
        const uint16_t *data = EcmaString::GetUtf16DataFlat(this, buf);
        if (EcmaString::StringsAreEqualUtf16(str1, data, str1->GetLength())) {
            return EcmaString::StringsAreEqualUtf16(str2, data + str1->GetLength(), str2->GetLength());
        }
    } else {
        CVector<uint8_t> buf;
        const uint8_t *data = EcmaString::GetUtf8DataFlat(this, buf);
        if (EcmaString::StringIsEqualUint8Data(str1, data, str1->GetLength(), this->IsUtf8())) {
            return EcmaString::StringIsEqualUint8Data(str2, data + str1->GetLength(),
                                                      str2->GetLength(), this->IsUtf8());
        }
    }
    return false;
}

/* static */
bool EcmaString::StringsAreEqualDiffUtfEncoding(EcmaString *left, EcmaString *right)
{
    CVector<uint16_t> bufLeftUft16;
    CVector<uint16_t> bufRightUft16;
    CVector<uint8_t> bufLeftUft8;
    CVector<uint8_t> bufRightUft8;
    int32_t lhsCount = static_cast<int32_t>(left->GetLength());
    int32_t rhsCount = static_cast<int32_t>(right->GetLength());
    if (!left->IsUtf16() && !right->IsUtf16()) {
        const uint8_t *data1 = EcmaString::GetUtf8DataFlat(left, bufLeftUft8);
        const uint8_t *data2 = EcmaString::GetUtf8DataFlat(right, bufRightUft8);
        Span<const uint8_t> lhsSp(data1, lhsCount);
        Span<const uint8_t> rhsSp(data2, rhsCount);
        return EcmaString::StringsAreEquals(lhsSp, rhsSp);
    } else if (!left->IsUtf16()) {
        const uint8_t *data1 = EcmaString::GetUtf8DataFlat(left, bufLeftUft8);
        const uint16_t *data2 = EcmaString::GetUtf16DataFlat(right, bufRightUft16);
        Span<const uint8_t> lhsSp(data1, lhsCount);
        Span<const uint16_t> rhsSp(data2, rhsCount);
        return EcmaString::StringsAreEquals(lhsSp, rhsSp);
    } else if (!right->IsUtf16()) {
        const uint16_t *data1 = EcmaString::GetUtf16DataFlat(left, bufLeftUft16);
        const uint8_t *data2 = EcmaString::GetUtf8DataFlat(right, bufRightUft8);
        Span<const uint16_t> lhsSp(data1, lhsCount);
        Span<const uint8_t> rhsSp(data2, rhsCount);
        return EcmaString::StringsAreEquals(lhsSp, rhsSp);
    } else {
        const uint16_t *data1 = EcmaString::GetUtf16DataFlat(left, bufLeftUft16);
        const uint16_t *data2 = EcmaString::GetUtf16DataFlat(right, bufRightUft16);
        Span<const uint16_t> lhsSp(data1, lhsCount);
        Span<const uint16_t> rhsSp(data2, rhsCount);
        return EcmaString::StringsAreEquals(lhsSp, rhsSp);
    }
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
bool EcmaString::StringsAreEqual(EcmaString *str1, EcmaString *str2)
{
    if (str1 == str2) {
        return true;
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
    return StringsAreEqualDiffUtfEncoding(str1, str2);
}

/* static */
bool EcmaString::StringIsEqualUint8Data(const EcmaString *str1, const uint8_t *dataAddr, uint32_t dataLen,
                                        bool canBeCompressToUtf8)
{
    if (!str1->IsSlicedString() && canBeCompressToUtf8 != str1->IsUtf8()) {
        return false;
    }
    if (canBeCompressToUtf8 && str1->GetLength() != dataLen) {
        return false;
    }
    if (str1->IsUtf8()) {
        CVector<uint8_t> buf;
        Span<const uint8_t> data1(EcmaString::GetUtf8DataFlat(str1, buf), dataLen);
        Span<const uint8_t> data2(dataAddr, dataLen);
        return EcmaString::StringsAreEquals(data1, data2);
    }
    CVector<uint16_t> buf;
    uint32_t length = str1->GetLength();
    const uint16_t *data = EcmaString::GetUtf16DataFlat(str1, buf);
    return IsUtf8EqualsUtf16(dataAddr, dataLen, data, length);
}

/* static */
bool EcmaString::StringsAreEqualUtf16(const EcmaString *str1, const uint16_t *utf16Data, uint32_t utf16Len)
{
    uint32_t length = str1->GetLength();
    if (length != utf16Len) {
        return false;
    }
    if (str1->IsUtf8()) {
        CVector<uint8_t> buf;
        const uint8_t *data = EcmaString::GetUtf8DataFlat(str1, buf);
        return IsUtf8EqualsUtf16(data, length, utf16Data, utf16Len);
    } else {
        CVector<uint16_t> buf;
        Span<const uint16_t> data1(EcmaString::GetUtf16DataFlat(str1, buf), length);
        Span<const uint16_t> data2(utf16Data, utf16Len);
        return EcmaString::StringsAreEquals(data1, data2);
    }
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

bool EcmaString::HashIntegerString(uint32_t length, uint32_t *hash, const uint32_t hashSeed) const
{
    ASSERT(length >= 0);
    Span<const uint8_t> str = FastToUtf8Span();
    return HashIntegerString(str.data(), length, hash, hashSeed);
}

uint32_t EcmaString::ComputeHashcode() const
{
    auto [hash, isInteger] = ComputeRawHashcode();
    return MixHashcode(hash, isInteger);
}

// hashSeed only be used when computing two separate strings merged hashcode.
std::pair<uint32_t, bool> EcmaString::ComputeRawHashcode() const
{
    uint32_t hash = 0;
    uint32_t length = GetLength();
    if (length == 0) {
        return {hash, false};
    }

    if (IsUtf8()) {
        // String using UTF8 encoding, and length smaller than 10, try to compute integer hash.
        if (length < MAX_ELEMENT_INDEX_LEN && this->HashIntegerString(length, &hash, 0)) {
            return {hash, true};
        }
        CVector<uint8_t> buf;
        const uint8_t *data = EcmaString::GetUtf8DataFlat(this, buf);
        // String can not convert to integer number, using normal hashcode computing algorithm.
        hash = this->ComputeHashForData(data, length, 0);
        return {hash, false};
    } else {
        CVector<uint16_t> buf;
        const uint16_t *data = EcmaString::GetUtf16DataFlat(this, buf);
        // If rawSeed has certain value, and second string uses UTF16 encoding,
        // then merged string can not be small integer number.
        hash = this->ComputeHashForData(data, length, 0);
        return {hash, false};
    }
}

// hashSeed only be used when computing two separate strings merged hashcode.
uint32_t EcmaString::ComputeHashcode(uint32_t rawHashSeed, bool isInteger) const
{
    uint32_t hash;
    uint32_t length = GetLength();
    if (length == 0) {
        return MixHashcode(rawHashSeed, isInteger);
    }

    if (IsUtf8()) {
        // String using UTF8 encoding, and length smaller than 10, try to compute integer hash.
        if ((rawHashSeed == 0 || isInteger) &&
             length < MAX_ELEMENT_INDEX_LEN && this->HashIntegerString(length, &hash, rawHashSeed)) {
            return hash;
        }
        CVector<uint8_t> buf;
        const uint8_t *data = EcmaString::GetUtf8DataFlat(this, buf);
        // String can not convert to integer number, using normal hashcode computing algorithm.
        hash = this->ComputeHashForData(data, length, rawHashSeed);
        return MixHashcode(hash, NOT_INTEGER);
    } else {
        CVector<uint16_t> buf;
        const uint16_t *data = EcmaString::GetUtf16DataFlat(this, buf);
        // If rawSeed has certain value, and second string uses UTF16 encoding,
        // then merged string can not be small integer number.
        hash = this->ComputeHashForData(data, length, rawHashSeed);
        return MixHashcode(hash, NOT_INTEGER);
    }
}

/* static */
uint32_t EcmaString::ComputeHashcodeUtf8(const uint8_t *utf8Data, size_t utf8Len, bool canBeCompress)
{
    uint32_t mixHash = 0;
    if (canBeCompress) {
        // String using UTF8 encoding, and length smaller than 10, try to compute integer hash.
        if (utf8Len < MAX_ELEMENT_INDEX_LEN && HashIntegerString(utf8Data, utf8Len, &mixHash, 0)) {
            return mixHash;
        }
        uint32_t hash = ComputeHashForData(utf8Data, utf8Len, 0);
        return MixHashcode(hash, NOT_INTEGER);
    } else {
        auto utf16Len = base::utf_helper::Utf8ToUtf16Size(utf8Data, utf8Len);
        CVector<uint16_t> tmpBuffer(utf16Len);
        [[maybe_unused]] auto len = base::utf_helper::ConvertRegionUtf8ToUtf16(utf8Data, tmpBuffer.data(), utf8Len,
                                                                               utf16Len);
        ASSERT(len == utf16Len);
        uint32_t hash = ComputeHashForData(tmpBuffer.data(), utf16Len, 0);
        return MixHashcode(hash, NOT_INTEGER);
    }
    LOG_ECMA(FATAL) << "this branch is unreachable";
    UNREACHABLE();
}

/* static */
uint32_t EcmaString::ComputeHashcodeUtf16(const uint16_t *utf16Data, uint32_t length)
{
    uint32_t mixHash = 0;
    // String length smaller than 10, try to compute integer hash.
    if (length < MAX_ELEMENT_INDEX_LEN && HashIntegerString(utf16Data, length, &mixHash, 0)) {
        return mixHash;
    }
    uint32_t hash = ComputeHashForData(utf16Data, length, 0);
    return MixHashcode(hash, NOT_INTEGER);
}

// drop the tail bytes if the remain length can't fill the length it represents.
static size_t FixUtf8Len(const uint8_t* utf8, size_t utf8Len)
{
    constexpr size_t TWO_BYTES_LENGTH = 2;
    constexpr size_t THREE_BYTES_LENGTH = 3;
    size_t trimSize = 0;
    if (utf8Len >= 1 && utf8[utf8Len - 1] >= 0xC0) {
        // The last one char claim there are more than 1 byte next to it, it's invalid, so drop the last one.
        trimSize = 1;
    }
    if (utf8Len >= TWO_BYTES_LENGTH && utf8[utf8Len - TWO_BYTES_LENGTH] >= 0xE0) {
        // The second to last char claim there are more than 2 bytes next to it, it's invalid, so drop the last two.
        trimSize = TWO_BYTES_LENGTH;
    }
    if (utf8Len >= THREE_BYTES_LENGTH && utf8[utf8Len - THREE_BYTES_LENGTH] >= 0xF0) {
        // The third to last char claim there are more than 3 bytes next to it, it's invalid, so drop the last three.
        trimSize = THREE_BYTES_LENGTH;
    }
    return utf8Len - trimSize;
}


/* static */
bool EcmaString::IsUtf8EqualsUtf16(const uint8_t *utf8Data, size_t utf8Len,
                                   const uint16_t *utf16Data, uint32_t utf16Len)
{
    size_t safeUtf8Len = FixUtf8Len(utf8Data, utf8Len);
    const uint8_t *utf8End = utf8Data + utf8Len;
    const uint8_t *utf8SafeEnd = utf8Data + safeUtf8Len;
    const uint16_t *utf16End = utf16Data + utf16Len;
    while (utf8Data < utf8SafeEnd && utf16Data < utf16End) {
        uint8_t src = *utf8Data;
        switch (src & 0xF0) {
            case 0xF0: {
                const uint8_t c2 = *(++utf8Data);
                const uint8_t c3 = *(++utf8Data);
                const uint8_t c4 = *(++utf8Data);
                uint32_t codePoint = ((src & LOW_3BITS) << OFFSET_18POS) | ((c2 & LOW_6BITS) << OFFSET_12POS) |
                                     ((c3 & LOW_6BITS) << OFFSET_6POS) | (c4 & LOW_6BITS);
                if (codePoint >= SURROGATE_RAIR_START) {
                    if (utf16Data >= utf16End - 1) {
                        return false;
                    }
                    codePoint -= SURROGATE_RAIR_START;
                    if (*utf16Data++ != static_cast<uint16_t>((codePoint >> OFFSET_10POS) | H_SURROGATE_START)) {
                        return false;
                    } else if (*utf16Data++ != static_cast<uint16_t>((codePoint & 0x3FF) | L_SURROGATE_START)) {
                        return false;
                    }
                } else {
                    if (*utf16Data++ != static_cast<uint16_t>(codePoint)) {
                        return false;
                    }
                }
                utf8Data++;
                break;
            }
            case 0xE0: {
                const uint8_t c2 = *(++utf8Data);
                const uint8_t c3 = *(++utf8Data);
                if (*utf16Data++ != static_cast<uint16_t>(((src & LOW_4BITS) << OFFSET_12POS) |
                    ((c2 & LOW_6BITS) << OFFSET_6POS) | (c3 & LOW_6BITS))) {
                    return false;
                }
                utf8Data++;
                break;
            }
            case 0xD0:
            case 0xC0: {
                const uint8_t c2 = *(++utf8Data);
                if (*utf16Data++ != static_cast<uint16_t>(((src & LOW_5BITS) << OFFSET_6POS) | (c2 & LOW_6BITS))) {
                    return false;
                }
                utf8Data++;
                break;
            }
            default:
                do {
                    if (*utf16Data++ != static_cast<uint16_t>(*utf8Data++)) {
                        return false;
                    }
                } while (utf8Data < utf8SafeEnd && utf16Data < utf16End && *utf8Data < 0x80);
                break;
        }
    }
    // The remain chars should be treated as single byte char.
    while (utf8Data < utf8End && utf16Data < utf16End) {
        if (*utf16Data++ != static_cast<uint16_t>(*utf8Data++)) {
            return false;
        }
    }
    return utf8Data == utf8End && utf16Data == utf16End;
}

bool EcmaString::ToElementIndex(uint32_t *index)
{
    uint32_t len = GetLength();
    if (UNLIKELY(len == 0 || len > MAX_ELEMENT_INDEX_LEN)) {  // NOLINTNEXTLINEreadability-magic-numbers)
        return false;
    }
    if (UNLIKELY(IsUtf16())) {
        return false;
    }

    // fast path: get integer from string's hash value
    if (TryToGetInteger(index)) {
        return true;
    }

    CVector<uint8_t> buf;
    const uint8_t *data = EcmaString::GetUtf8DataFlat(this, buf);
    uint32_t c = data[0];
    uint64_t n = 0;
    if (c == '0') {
        *index = 0;
        return len == 1;
    }
    uint32_t loopStart = 0;
    if (ToUInt64FromLoopStart(&n, loopStart, data) && n < JSObject::MAX_ELEMENT_INDEX) {
        *index = n;
        return true;
    }
    return false;
}

bool EcmaString::ToInt(int32_t *index, bool *negative)
{
    uint32_t len = GetLength();
    if (UNLIKELY(len == 0 || len > MAX_ELEMENT_INDEX_LEN)) {  // NOLINTNEXTLINEreadability-magic-numbers)
        return false;
    }
    if (UNLIKELY(IsUtf16())) {
        return false;
    }
    CVector<uint8_t> buf;
    const uint8_t *data = EcmaString::GetUtf8DataFlat(this, buf);
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

bool EcmaString::ToTypedArrayIndex(uint32_t *index)
{
    uint32_t len = GetLength();
    if (UNLIKELY(len == 0 || len > MAX_ELEMENT_INDEX_LEN)) {
        return false;
    }
    if (UNLIKELY(IsUtf16())) {
        return false;
    }

    CVector<uint8_t> buf;
    const uint8_t *data = EcmaString::GetUtf8DataFlat(this, buf);
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
        WriteToFlat<uint8_t>(*string, result->GetDataUtf8Writable(), length);
    } else {
        result = CreateLineStringWithSpaceType(vm, length, false, type);
        WriteToFlat<uint16_t>(*string, result->GetDataUtf16Writable(), length);
    }
    if (string->IsTreeString()) {
        JSHandle<TreeEcmaString> tree(string);
        ASSERT(EcmaString::Cast(tree->GetSecond())->GetLength() != 0);
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
    JSHandle<TreeEcmaString> tree = JSHandle<TreeEcmaString>::Cast(string);
    if (!tree->IsFlat()) {
        return SlowFlatten(vm, string, type);
    }
    return EcmaString::Cast(tree->GetFirst());
}

FlatStringInfo EcmaString::FlattenAllString(const EcmaVM *vm, const JSHandle<EcmaString> &string, MemSpaceType type)
{
    ASSERT(IsSMemSpace(type));
    EcmaString *s = *string;
    uint32_t startIndex = 0;
    if (s->IsLineOrConstantString()) {
        return FlatStringInfo(s, startIndex, s->GetLength());
    }
    if (string->IsTreeString()) {
        JSHandle<TreeEcmaString> tree = JSHandle<TreeEcmaString>::Cast(string);
        if (!tree->IsFlat()) {
            s = SlowFlatten(vm, string, type);
        } else {
            s = EcmaString::Cast(tree->GetFirst());
        }
    } else if (string->IsSlicedString()) {
        s = EcmaString::Cast(SlicedString::Cast(*string)->GetParent());
        startIndex = SlicedString::Cast(*string)->GetStartIndex();
    }
    return FlatStringInfo(s, startIndex, string->GetLength());
}

EcmaString *EcmaString::FlattenNoGCForSnapshot(const EcmaVM *vm, EcmaString *string)
{
    DISALLOW_GARBAGE_COLLECTION;
    if (string->IsLineOrConstantString()) {
        return string;
    }
    if (string->IsTreeString()) {
        TreeEcmaString *tree = TreeEcmaString::Cast(string);
        if (tree->IsFlat()) {
            string = EcmaString::Cast(tree->GetFirst());
        } else {
            uint32_t length = tree->GetLength();
            EcmaString *result = nullptr;
            if (tree->IsUtf8()) {
                result = CreateLineStringNoGC(vm, length, true);
                WriteToFlat<uint8_t>(tree, result->GetDataUtf8Writable(), length);
            } else {
                result = CreateLineStringNoGC(vm, length, false);
                WriteToFlat<uint16_t>(tree, result->GetDataUtf16Writable(), length);
            }
            tree->SetFirst(vm->GetJSThread(), JSTaggedValue(result));
            tree->SetSecond(vm->GetJSThread(), JSTaggedValue(*vm->GetFactory()->GetEmptyString()));
            return result;
        }
    } else if (string->IsSlicedString()) {
        SlicedString *str = SlicedString::Cast(string);
        uint32_t length = str->GetLength();
        EcmaString *result = nullptr;
        if (str->IsUtf8()) {
            result = CreateLineStringNoGC(vm, length, true);
            WriteToFlat<uint8_t>(str, result->GetDataUtf8Writable(), length);
        } else {
            result = CreateLineStringNoGC(vm, length, false);
            WriteToFlat<uint16_t>(str, result->GetDataUtf16Writable(), length);
        }
        return result;
    }
    return string;
}

const uint8_t *EcmaString::GetUtf8DataFlat(const EcmaString *src, CVector<uint8_t> &buf)
{
    ASSERT(src->IsUtf8());
    uint32_t length = src->GetLength();
    EcmaString *string = const_cast<EcmaString *>(src);
    if (string->IsTreeString()) {
        if (string->IsFlat()) {
            string = EcmaString::Cast(TreeEcmaString::Cast(string)->GetFirst());
        } else {
            buf.reserve(length);
            WriteToFlat(string, buf.data(), length);
            return buf.data();
        }
    } else if (string->IsSlicedString()) {
        SlicedString *str = SlicedString::Cast(string);
        return EcmaString::Cast(str->GetParent())->GetDataUtf8() + str->GetStartIndex();
    }
    return string->GetDataUtf8();
}

const uint8_t *EcmaString::GetNonTreeUtf8Data(const EcmaString *src)
{
    ASSERT(src->IsUtf8());
    ASSERT(!src->IsTreeString());
    EcmaString *string = const_cast<EcmaString *>(src);
    if (string->IsSlicedString()) {
        SlicedString *str = SlicedString::Cast(string);
        return EcmaString::Cast(str->GetParent())->GetDataUtf8() + str->GetStartIndex();
    }
    ASSERT(src->IsLineOrConstantString());
    return string->GetDataUtf8();
}

const uint16_t *EcmaString::GetUtf16DataFlat(const EcmaString *src, CVector<uint16_t> &buf)
{
    ASSERT(src->IsUtf16());
    uint32_t length = src->GetLength();
    EcmaString *string = const_cast<EcmaString *>(src);
    if (string->IsTreeString()) {
        if (string->IsFlat()) {
            string = EcmaString::Cast(TreeEcmaString::Cast(string)->GetFirst());
        } else {
            buf.reserve(length);
            WriteToFlat(string, buf.data(), length);
            return buf.data();
        }
    } else if (string->IsSlicedString()) {
        SlicedString *str = SlicedString::Cast(string);
        return EcmaString::Cast(str->GetParent())->GetDataUtf16() + str->GetStartIndex();
    }
    return string->GetDataUtf16();
}

const uint16_t *EcmaString::GetNonTreeUtf16Data(const EcmaString *src)
{
    ASSERT(src->IsUtf16());
    ASSERT(!src->IsTreeString());
    EcmaString *string = const_cast<EcmaString *>(src);
    if (string->IsSlicedString()) {
        SlicedString *str = SlicedString::Cast(string);
        return EcmaString::Cast(str->GetParent())->GetDataUtf16() + str->GetStartIndex();
    }
    ASSERT(src->IsLineOrConstantString());
    return string->GetDataUtf16();
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

std::string EcmaStringAccessor::ToStdString(StringConvertedUsage usage)
{
    if (string_ == nullptr) {
        return "";
    }
    bool modify = (usage != StringConvertedUsage::PRINT);
    CVector<uint8_t> buf;
    Span<const uint8_t> sp = string_->ToUtf8Span(buf, modify);
    std::string res;
    res.reserve(sp.size());
    for (const auto &c : sp) {
        res.push_back(c);
    }
    return res;
}

CString EcmaStringAccessor::Utf8ConvertToString()
{
    if (string_ == nullptr) {
        return CString("");
    }
    if (IsUtf8()) {
        std::string stdStr;
        if (IsLineString()) {
            return base::StringHelper::Utf8ToString(GetDataUtf8(), GetLength()).c_str();
        }
        CVector<uint8_t> buf;
        const uint8_t *data = EcmaString::GetUtf8DataFlat(string_, buf);
        return base::StringHelper::Utf8ToString(data, GetLength()).c_str();
    } else {
        return ToCString();
    }
}

std::string EcmaStringAccessor::DebuggerToStdString(StringConvertedUsage usage)
{
    if (string_ == nullptr) {
        return "";
    }

    bool modify = (usage != StringConvertedUsage::PRINT);
    CVector<uint8_t> buf;
    Span<const uint8_t> sp = string_->DebuggerToUtf8Span(buf, modify);
    std::string res;
    res.reserve(sp.size());
    for (const auto &c : sp) {
        res.push_back(c);
    }
    return res;
}

CString EcmaStringAccessor::ToCString(StringConvertedUsage usage, bool cesu8)
{
    if (string_ == nullptr) {
        return "";
    }
    bool modify = (usage != StringConvertedUsage::PRINT);
    CVector<uint8_t> buf;
    Span<const uint8_t> sp = string_->ToUtf8Span(buf, modify, cesu8);
    CString res;
    res.reserve(sp.size());
    for (const auto &c : sp) {
        res.push_back(c);
    }
    return res;
}

// static
EcmaString *EcmaStringAccessor::CreateLineString(const EcmaVM *vm, size_t length, bool compressed)
{
    return EcmaString::CreateLineString(vm, length, compressed);
}
}  // namespace panda::ecmascript
