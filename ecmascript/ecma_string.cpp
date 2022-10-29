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
static constexpr int SMALL_STRING_SIZE = 128;

EcmaString *EcmaString::Concat(const EcmaVM *vm,
    const JSHandle<EcmaString> &str1Handle, const JSHandle<EcmaString> &str2Handle)
{
    // allocator may trig gc and move src, need to hold it
    EcmaString *string1 = *str1Handle;
    EcmaString *string2 = *str2Handle;

    uint32_t length1 = string1->GetLength();

    uint32_t length2 = string2->GetLength();
    uint32_t newLength = length1 + length2;
    if (newLength == 0) {
        return vm->GetFactory()->GetEmptyString().GetObject<EcmaString>();
    } else if (length1 == 0) {
        return string2;
    } else if (length2 == 0) {
        return string1;
    }
    bool compressed = (!string1->IsUtf16() && !string2->IsUtf16());
    auto newString = AllocStringObject(vm, newLength, compressed);

    // retrieve strings after gc
    string1 = *str1Handle;
    string2 = *str2Handle;
    if (compressed) {
        Span<uint8_t> sp(newString->GetDataUtf8Writable(), newLength);
        Span<const uint8_t> src1(string1->GetDataUtf8(), length1);
        EcmaString::StringCopy(sp, newLength, src1, length1);

        sp = sp.SubSpan(length1);
        Span<const uint8_t> src2(string2->GetDataUtf8(), length2);
        EcmaString::StringCopy(sp, newLength - length1, src2, length2);
    } else {
        Span<uint16_t> sp(newString->GetDataUtf16Writable(), newLength);
        if (!string1->IsUtf16()) {
            for (uint32_t i = 0; i < length1; ++i) {
                sp[i] = string1->At<false>(i);
            }
        } else {
            Span<const uint16_t> src1(string1->GetDataUtf16(), length1);
            EcmaString::StringCopy(sp, newLength << 1U, src1, length1 << 1U);
        }
        sp = sp.SubSpan(length1);
        if (!string2->IsUtf16()) {
            for (uint32_t i = 0; i < length2; ++i) {
                sp[i] = string2->At<false>(i);
            }
        } else {
            uint32_t length = length2 << 1U;
            Span<const uint16_t> src2(string2->GetDataUtf16(), length2);
            EcmaString::StringCopy(sp, length, src2, length);
        }
    }

    ASSERT_PRINT(compressed == CanBeCompressed(newString), "compressed does not match the real value!");
    return newString;
}

/* static */
EcmaString *EcmaString::FastSubString(const EcmaVM *vm,
    const JSHandle<EcmaString> &src, uint32_t start, uint32_t utf16Len)
{
    if (src->IsUtf8()) {
        return FastSubUtf8String(vm, src, start, utf16Len);
    }
    return FastSubUtf16String(vm, src, start, utf16Len);
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

int32_t EcmaString::Compare(EcmaString *lhs, EcmaString *rhs)
{
    if (lhs == rhs) {
        return 0;
    }
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
    int32_t i;
    for (i = pos; i <= max; i++) {
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

int32_t EcmaString::IndexOf(EcmaString *lhs, EcmaString *rhs, int pos)
{
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

int32_t EcmaString::LastIndexOf(EcmaString *lhs, EcmaString *rhs, int pos)
{
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
        result = base::StringHelper::Utf16ToU16String(GetDataUtf16(), length);
    } else {
        result = base::StringHelper::Utf8ToU16String(GetDataUtf8(), length);
    }
    return result;
}

// static
bool EcmaString::CanBeCompressed(const EcmaString *string)
{
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

/* static */
void EcmaString::CopyUtf16AsUtf8(const uint16_t *utf16From, uint8_t *utf8To, uint32_t utf16Len)
{
    Span<const uint16_t> from(utf16From, utf16Len);
    Span<uint8_t> to(utf8To, utf16Len);
    for (uint32_t i = 0; i < utf16Len; i++) {
        to[i] = from[i];
    }
}

bool EcmaString::EqualToSplicedString(const EcmaString *str1, const EcmaString *str2)
{
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
        Span<const uint16_t> data1(str1->GetDataUtf16(), str1->GetLength());
        Span<const uint16_t> data2(str2->GetDataUtf16(), str1->GetLength());
        return EcmaString::StringsAreEquals(data1, data2);
    } else {  // NOLINT(readability-else-after-return)
        Span<const uint8_t> data1(str1->GetDataUtf8(), str1->GetLength());
        Span<const uint8_t> data2(str2->GetDataUtf8(), str1->GetLength());
        return EcmaString::StringsAreEquals(data1, data2);
    }
}

/* static */
bool EcmaString::StringsAreEqual(EcmaString *str1, EcmaString *str2)
{
    if ((str1->IsUtf16() != str2->IsUtf16()) || (str1->GetLength() != str2->GetLength()) ||
        (str1->GetHashcode() != str2->GetHashcode())) {
        return false;
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
        Span<const uint8_t> data1(str1->GetDataUtf8(), utf8Len);
        Span<const uint8_t> data2(utf8Data, utf8Len);
        return EcmaString::StringsAreEquals(data1, data2);
    }
    return IsUtf8EqualsUtf16(utf8Data, utf8Len, str1->GetDataUtf16(), str1->GetLength());
}

/* static */
bool EcmaString::StringsAreEqualUtf16(const EcmaString *str1, const uint16_t *utf16Data, uint32_t utf16Len)
{
    bool result = false;
    if (str1->GetLength() != utf16Len) {
        result = false;
    } else if (!str1->IsUtf16()) {
        result = IsUtf8EqualsUtf16(str1->GetDataUtf8(), str1->GetLength(), utf16Data, utf16Len);
    } else {
        Span<const uint16_t> data1(str1->GetDataUtf16(), str1->GetLength());
        Span<const uint16_t> data2(utf16Data, utf16Len);
        result = EcmaString::StringsAreEquals(data1, data2);
    }
    return result;
}

/* static */
template<typename T>
bool EcmaString::StringsAreEquals(Span<const T> &str1, Span<const T> &str2)
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
    return !memcmp(str1.data(), str2.data(), size);
}

template<typename T>
bool EcmaString::StringCopy(Span<T> &dst, size_t dstMax, Span<const T> &src, size_t count)
{
    ASSERT(dstMax >= count);
    ASSERT(dst.Size() >= src.Size());
    if (memcpy_s(dst.data(), dstMax, src.data(), count) != EOK) {
        LOG_FULL(FATAL) << "memcpy_s failed";
        UNREACHABLE();
    }
    return true;
}

template<class T>
static int32_t ComputeHashForData(const T *data, size_t size, uint32_t hashSeed)
{
    uint32_t hash = hashSeed;
    Span<const T> sp(data, size);
    for (auto c : sp) {
        constexpr size_t SHIFT = 5;
        hash = (hash << SHIFT) - hash + c;
    }
    return static_cast<int32_t>(hash);
}

static int32_t ComputeHashForUtf8(const uint8_t *utf8Data, uint32_t utf8DataLength)
{
    if (utf8Data == nullptr) {
        return 0;
    }
    uint32_t hash = 0;
    const uint8_t *end = utf8Data + utf8DataLength;
    while (utf8Data < end) { // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
        constexpr size_t SHIFT = 5;
        hash = (hash << SHIFT) - hash + *utf8Data++;  // NOLINT(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    }
    return static_cast<int32_t>(hash);
}

uint32_t EcmaString::ComputeHashcode(uint32_t hashSeed) const
{
    int32_t hash;
    if (!IsUtf16()) {
        hash = ComputeHashForData(GetDataUtf8(), GetLength(), hashSeed);
    } else {
        hash = ComputeHashForData(GetDataUtf16(), GetLength(), hashSeed);
    }
    return static_cast<uint32_t>(hash);
}

/* static */
uint32_t EcmaString::ComputeHashcodeUtf8(const uint8_t *utf8Data, size_t utf8Len, bool canBeCompress)
{
    int32_t hash;
    if (canBeCompress) {
        hash = ComputeHashForUtf8(utf8Data, utf8Len);
    } else {
        auto utf16Len = base::utf_helper::Utf8ToUtf16Size(utf8Data, utf8Len);
        CVector<uint16_t> tmpBuffer(utf16Len);
        [[maybe_unused]] auto len = base::utf_helper::ConvertRegionUtf8ToUtf16(utf8Data, tmpBuffer.data(), utf8Len,
                                                                               utf16Len, 0);
        ASSERT(len == utf16Len);
        hash = ComputeHashForData(tmpBuffer.data(), utf16Len, 0);
    }
    return static_cast<uint32_t>(hash);
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
    // length is one more than compared utf16Data, don't need convert all utf8Data to utf16Data
    uint32_t utf8ConvertLength = utf16Len + 1;
    CVector<uint16_t> tmpBuffer(utf8ConvertLength);
    auto len = base::utf_helper::ConvertRegionUtf8ToUtf16(utf8Data, tmpBuffer.data(), utf8Len, utf8ConvertLength, 0);
    if (len != utf16Len) {
        return false;
    }

    Span<const uint16_t> data1(tmpBuffer.data(), len);
    Span<const uint16_t> data2(utf16Data, utf16Len);
    return EcmaString::StringsAreEquals(data1, data2);
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

    uint32_t c = GetDataUtf8()[0];
    uint64_t n = 0;
    if (c == '0') {
        *index = 0;
        return len == 1;
    }
    if (c > '0' && c <= '9') {
        n = c - '0';
        for (uint32_t i = 1; i < len; i++) {
            c = GetDataUtf8()[i];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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

    uint32_t c = GetDataUtf8()[0];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
    uint64_t n  = 0;
    if (c == '0') {
        *index = 0;
        return len == 1;
    }
    if (c > '0' && c <= '9') {
        n = c - '0';
        for (uint32_t i = 1; i < len; i++) {
            c = GetDataUtf8()[i];  // NOLINTNEXTLINE(cppcoreguidelines-pro-bounds-pointer-arithmetic)
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
    uint32_t start = 0;
    uint32_t end = srcLen - 1;

    if (mode == TrimMode::TRIM || mode == TrimMode::TRIM_START) {
        start = base::StringHelper::GetStart(data, srcLen);
    }
    if (mode == TrimMode::TRIM || mode == TrimMode::TRIM_END) {
        end = base::StringHelper::GetEnd(data, start, srcLen);
    }
    EcmaString *res = FastSubString(thread->GetEcmaVM(), src, start, end - start + 1);
    return res;
}

/* static */
EcmaString *EcmaString::ToLower(const EcmaVM *vm, const JSHandle<EcmaString> &src)
{
    uint32_t srcLength = src->GetLength();
    auto factory = vm->GetFactory();
    if (src->IsUtf16()) {
        std::u16string u16str = base::StringHelper::Utf16ToU16String(src->GetDataUtf16(), srcLength);
        std::string res = base::StringHelper::ToLower(u16str);
        return *(factory->NewFromStdString(res));
    } else {
        const char start = 'A';
        const char end = 'Z';
        auto newString = AllocStringObject(vm, srcLength, true);
        Span<uint8_t> data(src->GetDataUtf8Writable(), srcLength);
        auto newStringPtr = newString->GetDataUtf8Writable();
        for (uint32_t index = 0; index < srcLength; ++index) {
            if (base::StringHelper::Utf8CharInRange(data[index], start, end)) {
                *(newStringPtr + index) = data[index] ^ (1 << 5);   // 1 and 5 means lower to upper or upper to lower
            } else {
                *(newStringPtr + index) = data[index];
            }
        }
        return newString;
    }
}

/* static */
EcmaString *EcmaString::ToUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src)
{
    uint32_t srcLength = src->GetLength();
    auto factory = vm->GetFactory();
    if (src->IsUtf16()) {
        std::u16string u16str = base::StringHelper::Utf16ToU16String(src->GetDataUtf16(), srcLength);
        std::string res = base::StringHelper::ToUpper(u16str);
        return *(factory->NewFromStdString(res));
    } else {
        const char start = 'a';
        const char end = 'z';
        auto newString = AllocStringObject(vm, srcLength, true);
        Span<uint8_t> data(src->GetDataUtf8Writable(), srcLength);
        auto newStringPtr = newString->GetDataUtf8Writable();
        for (uint32_t index = 0; index < srcLength; ++index) {
            if (base::StringHelper::Utf8CharInRange(data[index], start, end)) {
                *(newStringPtr + index) = data[index] ^ (1 << 5);   // 1 and 5 means lower to upper or upper to lower
            } else {
                *(newStringPtr + index) = data[index];
            }
        }
        return newString;
    }
}

/* static */
EcmaString *EcmaString::ToLocaleLower(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale)
{
    auto factory = vm->GetFactory();
    std::u16string utf16 = src->ToU16String();
    std::string res = base::StringHelper::ToLocaleLower(utf16, locale);
    return *(factory->NewFromStdString(res));
}

/* static */
EcmaString *EcmaString::ToLocaleUpper(const EcmaVM *vm, const JSHandle<EcmaString> &src, const icu::Locale &locale)
{
    auto factory = vm->GetFactory();
    std::u16string utf16 = src->ToU16String();
    std::string res = base::StringHelper::ToLocaleUpper(utf16, locale);
    return *(factory->NewFromStdString(res));
}

EcmaString *EcmaString::Trim(const JSThread *thread, const JSHandle<EcmaString> &src, TrimMode mode)
{
    uint32_t srcLen = src->GetLength();
    if (UNLIKELY(srcLen == 0)) {
        return EcmaString::Cast(thread->GlobalConstants()->GetEmptyString().GetTaggedObject());
    }
    if (src->IsUtf8()) {
        Span<const uint8_t> data(src->GetDataUtf8(), srcLen);
        return TrimBody(thread, src, data, mode);
    } else {
        Span<const uint16_t> data(src->GetDataUtf16(), srcLen);
        return TrimBody(thread, src, data, mode);
    }
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
    [[maybe_unused]] CVector<uint8_t> buf;
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
    [[maybe_unused]] CVector<uint8_t> buf;
    Span<const uint8_t> sp = string_->ToUtf8Span(buf, modify);
    CString res;
    res.reserve(sp.size());
    for (const auto &c : sp) {
        res.push_back(c);
    }
    return res;
}
}  // namespace panda::ecmascript
