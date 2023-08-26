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

#include "ecmascript/js_symbol.h"
#include "ecmascript/mem/c_containers.h"

namespace panda::ecmascript {

EcmaString *EcmaString::Concat(const EcmaVM *vm,
    const JSHandle<EcmaString> &left, const JSHandle<EcmaString> &right, MemSpaceType type)
{
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
        if (type == MemSpaceType::OLD_SPACE) {
            Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(*right));
            if (objectRegion->InYoungSpace()) {
                return CopyStringToOldSpace(vm, right, rightLength, strRight->IsUtf8());
            }
        }
        return strRight;
    }
    if (rightLength == 0) {
        if (type == MemSpaceType::OLD_SPACE) {
            Region *objectRegion = Region::ObjectAddressToRange(reinterpret_cast<TaggedObject *>(*left));
            if (objectRegion->InYoungSpace()) {
                return CopyStringToOldSpace(vm, left, leftLength, strLeft->IsUtf8());
            }
        }
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
    EcmaString *strOrigin = *original;
    ASSERT(strOrigin->IsLineOrConstantString());
    EcmaString *newString = nullptr;
    if (strOrigin->IsLineString()) {
        newString = CreateLineStringWithSpaceType(vm, length, compressed, MemSpaceType::OLD_SPACE);
    } else if (strOrigin->IsConstantString()) {
        return CreateConstantString(vm, strOrigin->GetDataUtf8(), length, MemSpaceType::OLD_SPACE);
    }
    strOrigin = *original;
    if (compressed) {
        // copy
        Span<uint8_t> sp(newString->GetDataUtf8Writable(), length);
        Span<const uint8_t> srcSp(strOrigin->GetDataUtf8(), length);
        EcmaString::MemCopyChars(sp, length, srcSp, length);
    } else {
        // copy left part
        Span<uint16_t> sp(newString->GetDataUtf16Writable(), length);
        if (strOrigin->IsUtf8()) {
            EcmaString::CopyChars(sp.data(), strOrigin->GetDataUtf8(), length);
        } else {
            Span<const uint16_t> srcSp(strOrigin->GetDataUtf16(), length);
            EcmaString::MemCopyChars(sp, length << 1U, srcSp, length << 1U);
        }
    }
    ASSERT_PRINT(compressed == CanBeCompressed(newString), "compressed does not match the real value!");
    return newString;
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
    auto srcFlat = JSHandle<EcmaString>(vm->GetJSThread(), Flatten(vm, src));
    if (srcFlat->IsUtf8()) {
        return FastSubUtf8String(vm, srcFlat, start, length);
    }
    return FastSubUtf16String(vm, srcFlat, start, length);
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
    auto leftFlat = JSHandle<EcmaString>(vm->GetJSThread(), Flatten(vm, left));
    auto rightFlat = JSHandle<EcmaString>(vm->GetJSThread(), Flatten(vm, right));
    EcmaString *lhs = *leftFlat;
    EcmaString *rhs = *rightFlat;
    int32_t lhsCount = static_cast<int32_t>(lhs->GetLength());
    int32_t rhsCount = static_cast<int32_t>(rhs->GetLength());
    int32_t countDiff = lhsCount - rhsCount;
    int32_t minCount = (countDiff < 0) ? lhsCount : rhsCount;
    if (!lhs->IsUtf16() && !rhs->IsUtf16()) {
        Span<const uint8_t> lhsSp(lhs->GetDataUtf8(), lhsCount);
        Span<const uint8_t> rhsSp(rhs->GetDataUtf8(), rhsCount);
        int32_t charDiff = CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    } else if (!lhs->IsUtf16()) {
        Span<const uint8_t> lhsSp(lhs->GetDataUtf8(), lhsCount);
        Span<const uint16_t> rhsSp(rhs->GetDataUtf16(), rhsCount);
        int32_t charDiff = CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    } else if (!rhs->IsUtf16()) {
        Span<const uint16_t> lhsSp(lhs->GetDataUtf16(), rhsCount);
        Span<const uint8_t> rhsSp(rhs->GetDataUtf8(), lhsCount);
        int32_t charDiff = CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    } else {
        Span<const uint16_t> lhsSp(lhs->GetDataUtf16(), lhsCount);
        Span<const uint16_t> rhsSp(rhs->GetDataUtf16(), rhsCount);
        int32_t charDiff = CompareStringSpan(lhsSp, rhsSp, minCount);
        if (charDiff != 0) {
            return charDiff;
        }
    }
    return countDiff;
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
    EcmaString *lhs = *receiver;
    EcmaString *rhs = *search;
    if (lhs == nullptr || rhs == nullptr) {
        return -1;
    }
    int32_t lhsCount = static_cast<int32_t>(lhs->GetLength());
    int32_t rhsCount = static_cast<int32_t>(rhs->GetLength());

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

    auto receiverFlat = JSHandle<EcmaString>(vm->GetJSThread(), Flatten(vm, receiver));
    auto searchFlat = JSHandle<EcmaString>(vm->GetJSThread(), Flatten(vm, search));
    lhs = *receiverFlat;
    rhs = *searchFlat;

    if (rhs->IsUtf8() && lhs->IsUtf8()) {
        Span<const uint8_t> lhsSp(lhs->GetDataUtf8(), lhsCount);
        Span<const uint8_t> rhsSp(rhs->GetDataUtf8(), rhsCount);
        return EcmaString::IndexOf(lhsSp, rhsSp, pos, max);
    } else if (rhs->IsUtf16() && lhs->IsUtf16()) {  // NOLINT(readability-else-after-return)
        Span<const uint16_t> lhsSp(lhs->GetDataUtf16(), lhsCount);
        Span<const uint16_t> rhsSp(rhs->GetDataUtf16(), rhsCount);
        return EcmaString::IndexOf(lhsSp, rhsSp, pos, max);
    } else if (rhs->IsUtf16()) {
        return -1;
    } else {  // NOLINT(readability-else-after-return)
        Span<const uint16_t> lhsSp(lhs->GetDataUtf16(), lhsCount);
        Span<const uint8_t> rhsSp(rhs->GetDataUtf8(), rhsCount);
        return EcmaString::IndexOf(lhsSp, rhsSp, pos, max);
    }
}

int32_t EcmaString::LastIndexOf(const EcmaVM *vm,
    const JSHandle<EcmaString> &receiver, const JSHandle<EcmaString> &search, int pos)
{
    EcmaString *lhs = *receiver;
    EcmaString *rhs = *search;
    if (lhs == nullptr || rhs == nullptr) {
        return -1;
    }

    int32_t lhsCount = static_cast<int32_t>(lhs->GetLength());
    int32_t rhsCount = static_cast<int32_t>(rhs->GetLength());
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

    auto receiverFlat = JSHandle<EcmaString>(vm->GetJSThread(), Flatten(vm, receiver));
    auto searchFlat = JSHandle<EcmaString>(vm->GetJSThread(), Flatten(vm, search));
    lhs = *receiverFlat;
    rhs = *searchFlat;

    if (rhs->IsUtf8() && lhs->IsUtf8()) {
        Span<const uint8_t> lhsSp(lhs->GetDataUtf8(), lhsCount);
        Span<const uint8_t> rhsSp(rhs->GetDataUtf8(), rhsCount);
        return EcmaString::LastIndexOf(lhsSp, rhsSp, pos);
    } else if (rhs->IsUtf16() && lhs->IsUtf16()) {  // NOLINT(readability-else-after-return)
        Span<const uint16_t> lhsSp(lhs->GetDataUtf16(), lhsCount);
        Span<const uint16_t> rhsSp(rhs->GetDataUtf16(), rhsCount);
        return EcmaString::LastIndexOf(lhsSp, rhsSp, pos);
    } else if (rhs->IsUtf16()) {
        return -1;
    } else {  // NOLINT(readability-else-after-return)
        Span<const uint16_t> lhsSp(lhs->GetDataUtf16(), lhsCount);
        Span<const uint8_t> rhsSp(rhs->GetDataUtf8(), rhsCount);
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
    bool isCompressed = true;
    uint32_t index = 0;
    // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    while (index < utf8Len) {
        // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        if (!IsASCIICharacter(utf8Data[index])) {
            isCompressed = false;
            break;
        }
        ++index;
    }
    return isCompressed;
}

/* static */
bool EcmaString::CanBeCompressed(const uint16_t *utf16Data, uint32_t utf16Len)
{
    bool isCompressed = true;
    Span<const uint16_t> data(utf16Data, utf16Len);
    for (uint32_t i = 0; i < utf16Len; i++) {
        if (!IsASCIICharacter(data[i])) {
            isCompressed = false;
            break;
        }
    }
    return isCompressed;
}

bool EcmaString::EqualToSplicedString(const EcmaString *str1, const EcmaString *str2)
{
    ASSERT(IsLineOrConstantString());
    ASSERT(str1->IsLineOrConstantString() && str2->IsLineOrConstantString());
    if (GetLength() != str1->GetLength() + str2->GetLength()) {
        return false;
    }
    if (IsUtf16()) {
        if (str1->IsUtf8() && str2->IsUtf8()) {
            return false;
        }
        if (EcmaString::StringsAreEqualUtf16(str1, GetDataUtf16(), str1->GetLength())) {
            return EcmaString::StringsAreEqualUtf16(str2, GetDataUtf16() + str1->GetLength(), str2->GetLength());
        }
    } else {
        if (str1->IsUtf16() || str2->IsUtf16()) {
            return false;
        }
        Span<const uint8_t> concatData(GetDataUtf8(), str1->GetLength());
        Span<const uint8_t> data1(str1->GetDataUtf8(), str1->GetLength());
        if (EcmaString::StringsAreEquals(concatData, data1)) {
            concatData = Span<const uint8_t>(GetDataUtf8() + str1->GetLength(), str2->GetLength());
            Span<const uint8_t> data2(str2->GetDataUtf8(), str2->GetLength());
            return EcmaString::StringsAreEquals(concatData, data2);
        }
    }
    return false;
}

/* static */
bool EcmaString::StringsAreEqualSameUtfEncoding(EcmaString *str1, EcmaString *str2)
{
    if (str1->IsUtf16()) {
        CVector<uint16_t> buf1;
        CVector<uint16_t> buf2;
        const uint16_t *data1 = EcmaString::GetUtf16DataFlat(str1, buf1);
        const uint16_t *data2 = EcmaString::GetUtf16DataFlat(str2, buf2);
        Span<const uint16_t> sp1(data1, str1->GetLength());
        Span<const uint16_t> sp2(data2, str2->GetLength());
        return EcmaString::StringsAreEquals(sp1, sp2);
    } else {  // NOLINT(readability-else-after-return)
        CVector<uint8_t> buf1;
        CVector<uint8_t> buf2;
        const uint8_t *data1 = EcmaString::GetUtf8DataFlat(str1, buf1);
        const uint8_t *data2 = EcmaString::GetUtf8DataFlat(str2, buf2);
        Span<const uint8_t> sp1(data1, str1->GetLength());
        Span<const uint8_t> sp2(data2, str2->GetLength());
        return EcmaString::StringsAreEquals(sp1, sp2);
    }
}

bool EcmaString::StringsAreEqual(const EcmaVM *vm, const JSHandle<EcmaString> &str1, const JSHandle<EcmaString> &str2)
{
    if (str1 == str2) {
        return true;
    }
    if (str1->IsUtf16() != str2->IsUtf16()) {
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

    auto str1Flat = JSHandle<EcmaString>(vm->GetJSThread(), Flatten(vm, str1));
    auto str2Flat = JSHandle<EcmaString>(vm->GetJSThread(), Flatten(vm, str2));
    return StringsAreEqualSameUtfEncoding(*str1Flat, *str2Flat);
}

/* static */
bool EcmaString::StringsAreEqual(EcmaString *str1, EcmaString *str2)
{
    if (str1 == str2) {
        return true;
    }
    if (str1->IsUtf16() != str2->IsUtf16()) {
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
    return StringsAreEqualSameUtfEncoding(str1, str2);
}

/* static */
bool EcmaString::StringsAreEqualUtf8(const EcmaString *str1, const uint8_t *utf8Data, uint32_t utf8Len,
                                     bool canBeCompress)
{
    if (canBeCompress != str1->IsUtf8()) {
        return false;
    }
    if (canBeCompress && str1->GetLength() != utf8Len) {
        return false;
    }
    if (canBeCompress) {
        CVector<uint8_t> buf;
        Span<const uint8_t> data1(EcmaString::GetUtf8DataFlat(str1, buf), utf8Len);
        Span<const uint8_t> data2(utf8Data, utf8Len);
        return EcmaString::StringsAreEquals(data1, data2);
    }
    CVector<uint16_t> buf;
    uint32_t length = str1->GetLength();
    const uint16_t *data = EcmaString::GetUtf16DataFlat(str1, buf);
    return IsUtf8EqualsUtf16(utf8Data, utf8Len, data, length);
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

uint32_t EcmaString::ComputeHashcode(uint32_t hashSeed) const
{
    uint32_t hash;
    uint32_t length = GetLength();
    if (IsUtf8()) {
        CVector<uint8_t> buf;
        const uint8_t *data = EcmaString::GetUtf8DataFlat(this, buf);
        hash = ComputeHashForData(data, length, hashSeed);
    } else {
        CVector<uint16_t> buf;
        const uint16_t *data = EcmaString::GetUtf16DataFlat(this, buf);
        hash = ComputeHashForData(data, length, hashSeed);
    }
    return hash;
}

/* static */
uint32_t EcmaString::ComputeHashcodeUtf8(const uint8_t *utf8Data, size_t utf8Len, bool canBeCompress)
{
    uint32_t hash = 0;
    if (canBeCompress) {
        hash = ComputeHashForData(utf8Data, utf8Len, 0);
    } else {
        auto utf16Len = base::utf_helper::Utf8ToUtf16Size(utf8Data, utf8Len);
        CVector<uint16_t> tmpBuffer(utf16Len);
        [[maybe_unused]] auto len = base::utf_helper::ConvertRegionUtf8ToUtf16(utf8Data, tmpBuffer.data(), utf8Len,
                                                                               utf16Len, 0);
        ASSERT(len == utf16Len);
        hash = ComputeHashForData(tmpBuffer.data(), utf16Len, 0);
    }
    return hash;
}

/* static */
uint32_t EcmaString::ComputeHashcodeUtf16(const uint16_t *utf16Data, uint32_t length)
{
    return ComputeHashForData(utf16Data, length, 0);
}

/* static */
bool EcmaString::IsUtf8EqualsUtf16(const uint8_t *utf8Data, size_t utf8Len, const uint16_t *utf16Data,
                                   uint32_t utf16Len)
{
    size_t utf8Pos = 0;
    size_t utf16Pos = 0;
    while (utf8Pos < utf8Len) {
        auto [pair, nbytes] = utf::ConvertMUtf8ToUtf16Pair(utf8Data, utf8Len - utf8Pos);
        auto [pHigh, pLow] = utf::SplitUtf16Pair(pair);
        utf8Data += nbytes;
        utf8Pos += nbytes;
        if (pHigh != 0) {
            if (utf16Pos >= utf16Len - 1 || *utf16Data != pHigh) {
                return false;
            }
            ++utf16Pos;
            ++utf16Data;
        }
        if (utf16Pos >= utf16Len || *utf16Data != pLow) {
            return false;
        }
        ++utf16Pos;
        ++utf16Data;
    }
    return true;
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

    CVector<uint8_t> buf;
    const uint8_t *data = EcmaString::GetUtf8DataFlat(this, buf);
    uint32_t c = data[0];
    uint64_t n = 0;
    if (c == '0') {
        *index = 0;
        return len == 1;
    }
    if (c > '0' && c <= '9') {
        n = c - '0';
        for (uint32_t i = 1; i < len; i++) {
            c = data[i];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
            if (c < '0' || c > '9') {
                return false;
            }
            // NOLINTNEXTLINE(readability-magic-numbers)
            n = n * 10 + (c - '0');  // 10: decimal factor
        }
        if (n < JSObject::MAX_ELEMENT_INDEX) {
            *index = n;
            return true;
        }
    }
    return false;
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
    auto srcFlat = JSHandle<EcmaString>(vm->GetJSThread(), Flatten(vm, src));
    uint32_t srcLength = srcFlat->GetLength();
    auto factory = vm->GetFactory();
    if (srcFlat->IsUtf16()) {
        std::u16string u16str = base::StringHelper::Utf16ToU16String(srcFlat->GetDataUtf16(), srcLength);
        std::string res = base::StringHelper::ToLower(u16str);
        return *(factory->NewFromStdString(res));
    } else {
        return ConvertUtf8ToLowerOrUpper(vm, srcFlat, true);
    }
}

/* static */
EcmaString *EcmaString::TryToLower(const EcmaVM *vm, const JSHandle<EcmaString> &src)
{
    auto srcFlat = JSHandle<EcmaString>(vm->GetJSThread(), Flatten(vm, src));
    uint32_t srcLength = srcFlat->GetLength();
    const char start = 'A';
    const char end = 'Z';
    uint32_t upperIndex = srcLength;
    Span<uint8_t> data(srcFlat->GetDataUtf8Writable(), srcLength);
    for (uint32_t index = 0; index < srcLength; ++index) {
        if (base::StringHelper::Utf8CharInRange(data[index], start, end)) {
            upperIndex = index;
            break;
        }
    }
    if (upperIndex == srcLength) {
        return *src;
    }
    return ConvertUtf8ToLowerOrUpper(vm, srcFlat, true, upperIndex);
}

/* static */
EcmaString *EcmaString::ConvertUtf8ToLowerOrUpper(const EcmaVM *vm, const JSHandle<EcmaString> &srcFlat,
                                                  bool toLower, uint32_t startIndex)
{
    const char start = toLower ? 'A' : 'a';
    const char end = toLower ? 'Z' : 'z';
    uint32_t srcLength = srcFlat->GetLength();
    auto newString = CreateLineString(vm, srcLength, true);
    Span<uint8_t> data(srcFlat->GetDataUtf8Writable(), srcLength);
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
    return newString;
}

/* static */
EcmaString *EcmaString::ToUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src)
{
    auto srcFlat = JSHandle<EcmaString>(vm->GetJSThread(), Flatten(vm, src));
    uint32_t srcLength = srcFlat->GetLength();
    auto factory = vm->GetFactory();
    if (srcFlat->IsUtf16()) {
        std::u16string u16str = base::StringHelper::Utf16ToU16String(srcFlat->GetDataUtf16(), srcLength);
        std::string res = base::StringHelper::ToUpper(u16str);
        return *(factory->NewFromStdString(res));
    } else {
        return ConvertUtf8ToLowerOrUpper(vm, srcFlat, false);
    }
}

/* static */
EcmaString *EcmaString::ToLocaleLower(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale)
{
    auto factory = vm->GetFactory();
    auto srcFlat = JSHandle<EcmaString>(vm->GetJSThread(), Flatten(vm, src));
    std::u16string utf16 = srcFlat->ToU16String();
    std::string res = base::StringHelper::ToLocaleLower(utf16, locale);
    return *(factory->NewFromStdString(res));
}

/* static */
EcmaString *EcmaString::ToLocaleUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale)
{
    auto factory = vm->GetFactory();
    auto srcFlat = JSHandle<EcmaString>(vm->GetJSThread(), Flatten(vm, src));
    std::u16string utf16 = srcFlat->ToU16String();
    std::string res = base::StringHelper::ToLocaleUpper(utf16, locale);
    return *(factory->NewFromStdString(res));
}

EcmaString *EcmaString::Trim(const JSThread *thread, const JSHandle<EcmaString> &src, TrimMode mode)
{
    auto srcFlat = JSHandle<EcmaString>(thread, Flatten(thread->GetEcmaVM(), src));
    uint32_t srcLen = srcFlat->GetLength();
    if (UNLIKELY(srcLen == 0)) {
        return EcmaString::Cast(thread->GlobalConstants()->GetEmptyString().GetTaggedObject());
    }
    if (srcFlat->IsUtf8()) {
        Span<const uint8_t> data(srcFlat->GetDataUtf8(), srcLen);
        return TrimBody(thread, srcFlat, data, mode);
    } else {
        Span<const uint16_t> data(srcFlat->GetDataUtf16(), srcLen);
        return TrimBody(thread, srcFlat, data, mode);
    }
}

EcmaString *EcmaString::SlowFlatten(const EcmaVM *vm, const JSHandle<TreeEcmaString> &string, MemSpaceType type)
{
    auto thread = vm->GetJSThread();
    ASSERT(EcmaString::Cast(string->GetSecond())->GetLength() != 0);

    uint32_t length = string->GetLength();
    EcmaString *result = nullptr;
    if (string->IsUtf8()) {
        result = CreateLineStringWithSpaceType(vm, length, true, type);
        WriteToFlat<uint8_t>(*string, result->GetDataUtf8Writable(), length);
    } else {
        result = CreateLineStringWithSpaceType(vm, length, false, type);
        WriteToFlat<uint16_t>(*string, result->GetDataUtf16Writable(), length);
    }
    string->SetFirst(thread, JSTaggedValue(result));
    string->SetSecond(thread, JSTaggedValue(*vm->GetFactory()->GetEmptyString()));
    return result;
}

EcmaString *EcmaString::Flatten(const EcmaVM *vm, const JSHandle<EcmaString> &string, MemSpaceType type)
{
    EcmaString *s = *string;
    if (s->IsLineOrConstantString()) {
        return s;
    }
    if (s->IsTreeString()) {
        JSHandle<TreeEcmaString> tree = JSHandle<TreeEcmaString>::Cast(string);
        if (!tree->IsFlat()) {
            return SlowFlatten(vm, tree, type);
        }
        s = EcmaString::Cast(tree->GetFirst());
    }
    return s;
}

EcmaString *EcmaString::FlattenNoGC(const EcmaVM *vm, EcmaString *string)
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
    }
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
    }
    return string->GetDataUtf16();
}

EcmaStringAccessor::EcmaStringAccessor(EcmaString *string)
{
    ASSERT(string != nullptr);
    string_ = string;
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

CString EcmaStringAccessor::ToCString(StringConvertedUsage usage)
{
    if (string_ == nullptr) {
        return "";
    }
    bool modify = (usage != StringConvertedUsage::PRINT);
    CVector<uint8_t> buf;
    Span<const uint8_t> sp = string_->ToUtf8Span(buf, modify);
    CString res;
    res.reserve(sp.size());
    for (const auto &c : sp) {
        res.push_back(c);
    }
    return res;
}
}  // namespace panda::ecmascript
