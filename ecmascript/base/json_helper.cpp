/*
 * Copyright (c) 2023-2025 Huawei Device Co., Ltd.
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */
#include <sstream>
#include <string>

#if defined(PANDA_TARGET_AMD64)
#include <emmintrin.h>
#endif

#if defined(PANDA_TARGET_ARM64) && !defined(PANDA_TARGET_MACOS)
#include <arm_neon.h>
#endif

#include "ecmascript/base/json_helper.h"
#include "ecmascript/base/json_parser.h"
#include "ecmascript/base/json_stringifier.h"
#include "ecmascript/ecma_string-inl.h"
#include "ecmascript/js_tagged_value-inl.h"
#include "common_components/base/utf_helper.h"
#include "libpandabase/utils/span.h"

namespace panda::ecmascript::base {

#if ENABLE_V70_OPTIMIZATION
namespace {
constexpr uint64_t K_OBJECT_KEY_CACHE_HASH_MUL = 11400714819323198485ULL;
constexpr size_t K_OBJECT_KEY_CACHE_PROBE_COUNT = 2;
constexpr uint32_t K_BYTE_BIT_WIDTH = 8U;
constexpr uint32_t K_UTF16_CODE_UNIT_BIT_WIDTH = 16U;
constexpr uint32_t K_PACKED_KEY_UTF8_HALF_BYTES = sizeof(uint64_t) / sizeof(uint8_t);
constexpr uint32_t K_PACKED_KEY_UTF16_HALF_CODE_UNITS = sizeof(uint64_t) / sizeof(uint16_t);
constexpr uint32_t K_PACKED_KEY_UTF8_MAX_BYTES = K_PACKED_KEY_UTF8_HALF_BYTES * 2U;
constexpr uint32_t K_PACKED_KEY_UTF16_MAX_CODE_UNITS = K_PACKED_KEY_UTF16_HALF_CODE_UNITS * 2U;
}

PackedKey128 PackUtf8ObjectKeyBytes(const uint8_t *utf8Data, uint32_t utf8Len)
{
    ASSERT(utf8Len <= K_PACKED_KEY_UTF8_MAX_BYTES);
    PackedKey128 packed;
    for (uint32_t i = 0; i < utf8Len; i++) {
        uint64_t value = static_cast<uint64_t>(utf8Data[i]);
        if (i < K_PACKED_KEY_UTF8_HALF_BYTES) {
            packed.lo |= value << (i * K_BYTE_BIT_WIDTH);
        } else {
            packed.hi |= value << ((i - K_PACKED_KEY_UTF8_HALF_BYTES) * K_BYTE_BIT_WIDTH);
        }
    }
    return packed;
}

PackedKey128 PackUtf16ObjectKeyBytes(const uint8_t *utf8Data, uint32_t utf16Len)
{
    ASSERT(utf16Len <= K_PACKED_KEY_UTF16_MAX_CODE_UNITS);
    PackedKey128 packed;
    for (uint32_t i = 0; i < utf16Len; i++) {
        uint64_t value = static_cast<uint64_t>(utf8Data[i]);
        if (i < K_PACKED_KEY_UTF16_HALF_CODE_UNITS) {
            packed.lo |= value << (i * K_UTF16_CODE_UNIT_BIT_WIDTH);
        } else {
            packed.hi |= value << ((i - K_PACKED_KEY_UTF16_HALF_CODE_UNITS) * K_UTF16_CODE_UNIT_BIT_WIDTH);
        }
    }
    return packed;
}

size_t Utf8ObjectKeyCacheIndex(const PackedKey128 &packed, uint32_t utf8Len)
{
    static_assert((OBJECT_KEY_CACHE_SIZE & (OBJECT_KEY_CACHE_SIZE - 1)) == 0);
    uint64_t hash = packed.lo ^ (packed.hi * K_OBJECT_KEY_CACHE_HASH_MUL) ^
        (static_cast<uint64_t>(utf8Len) * K_OBJECT_KEY_CACHE_HASH_MUL);
    return static_cast<size_t>(hash) & (OBJECT_KEY_CACHE_SIZE - 1);
}

PackedKey128 PackUtf16ObjectKeyCodeUnits(const uint16_t *utf16Data, uint32_t utf16Len)
{
    ASSERT(utf16Len <= K_PACKED_KEY_UTF16_MAX_CODE_UNITS);
    PackedKey128 packed;
    for (uint32_t i = 0; i < utf16Len; i++) {
        uint64_t value = static_cast<uint64_t>(utf16Data[i]);
        if (i < K_PACKED_KEY_UTF16_HALF_CODE_UNITS) {
            packed.lo |= value << (i * K_UTF16_CODE_UNIT_BIT_WIDTH);
        } else {
            packed.hi |= value << ((i - K_PACKED_KEY_UTF16_HALF_CODE_UNITS) * K_UTF16_CODE_UNIT_BIT_WIDTH);
        }
    }
    return packed;
}

PackedKey128 PackUtf16ObjectKeyFromAccessor(EcmaStringAccessor &accessor, uint32_t utf16Len)
{
    if (accessor.IsUtf8()) {
        return PackUtf16ObjectKeyBytes(accessor.GetDataUtf8(), utf16Len);
    }
    return PackUtf16ObjectKeyCodeUnits(accessor.GetDataUtf16(), utf16Len);
}

size_t Utf16ObjectKeyCacheIndex(const PackedKey128 &packed, uint32_t utf16Len)
{
    static_assert((OBJECT_KEY_CACHE_SIZE & (OBJECT_KEY_CACHE_SIZE - 1)) == 0);
    uint64_t hash = packed.lo ^ (packed.hi * K_OBJECT_KEY_CACHE_HASH_MUL) ^
        (static_cast<uint64_t>(utf16Len) * K_OBJECT_KEY_CACHE_HASH_MUL);
    return static_cast<size_t>(hash) & (OBJECT_KEY_CACHE_SIZE - 1);
}

void ResetObjectKeyCacheEntries(std::array<ObjectKeyCacheEntry, OBJECT_KEY_CACHE_SIZE> &objectKeyCache)
{
    for (auto &entry : objectKeyCache) {
        entry.key_ = JSHandle<EcmaString>();
        entry.packedLo_ = 0;
        entry.packedHi_ = 0;
        entry.length_ = 0;
        entry.valid_ = false;
    }
}

JSHandle<EcmaString> LookupObjectKeyCacheEntry(
    const std::array<ObjectKeyCacheEntry, OBJECT_KEY_CACHE_SIZE> &objectKeyCache, size_t index,
    const PackedKey128 &packed, uint32_t keyLength)
{
    for (size_t probe = 0; probe < K_OBJECT_KEY_CACHE_PROBE_COUNT; probe++) {
        const auto &entry = objectKeyCache[index ^ probe];
        if (entry.valid_ && entry.length_ == keyLength &&
            entry.packedLo_ == packed.lo && entry.packedHi_ == packed.hi) {
            return entry.key_;
        }
    }
    return JSHandle<EcmaString>();
}

void UpdateObjectKeyCacheEntry(std::array<ObjectKeyCacheEntry, OBJECT_KEY_CACHE_SIZE> &objectKeyCache,
                               size_t index, const PackedKey128 &packed, uint32_t keyLength,
                               const JSHandle<EcmaString> &key)
{
    for (size_t probe = 0; probe < K_OBJECT_KEY_CACHE_PROBE_COUNT; probe++) {
        auto &entry = objectKeyCache[index ^ probe];
        if (entry.valid_ && entry.length_ == keyLength &&
            entry.packedLo_ == packed.lo && entry.packedHi_ == packed.hi) {
            entry.key_ = key;
            return;
        }
        if (!entry.valid_) {
            entry.key_ = key;
            entry.packedLo_ = packed.lo;
            entry.packedHi_ = packed.hi;
            entry.length_ = static_cast<uint8_t>(keyLength);
            entry.valid_ = true;
            return;
        }
    }

    auto &entry = objectKeyCache[index];
    entry.key_ = key;
    entry.packedLo_ = packed.lo;
    entry.packedHi_ = packed.hi;
    entry.length_ = static_cast<uint8_t>(keyLength);
    entry.valid_ = true;
}
#endif

std::pair<std::string, std::uint32_t> JsonHelper::AnonymizeJsonString(JSThread *thread, JSHandle<JSTaggedValue> str,
                                                                      uint32_t position, uint32_t width)
{
    ecmascript::EcmaStringAccessor acc = ecmascript::EcmaStringAccessor(str.GetTaggedValue());
    if (acc.IsUtf8()) {
        ecmascript::CVector<uint8_t> inputBuf;
        const uint8_t *data = ecmascript::EcmaStringAccessor::GetUtf8DataFlat(thread,
            EcmaString::Cast(str.GetTaggedValue()), inputBuf);
        uint32_t length = acc.GetLength();
        uint32_t index = 0;
        uint32_t startIndex = (position > width) ? (position - width) : 0;
        uint32_t endIndex = (length > position + width) ? (position + width) : length;
        std::string resStr = "";
        for (uint32_t i = startIndex; i < endIndex; i++) {
            index++;
            if (!IsJsonFormatCharacter(data[i])) {
                resStr.push_back(index % 2 != 0 ? data[i] : '*'); // 2:Replace even positions with *
            } else {
                index = 0;
                resStr.push_back(data[i]);
            }
        }
        uint32_t retPos = position > startIndex ? position - startIndex : 0;
        return {resStr, retPos};
    } else {
        ecmascript::CVector<uint16_t> inputBuf;
        const uint16_t *data = ecmascript::EcmaStringAccessor::GetUtf16DataFlat(thread,
            EcmaString::Cast(str.GetTaggedValue()), inputBuf);
        uint32_t length = acc.GetLength();
        return AnonymizeJsonStringUtf16(data, length, position, width);
    }
}
std::pair<std::string, std::uint32_t> JsonHelper::AnonymizeJsonStringUtf16(const uint16_t *utf16Data, size_t len,
                                                                           uint32_t position, uint32_t width)
{
    if (common::utf_helper::IsUTF16LowSurrogate(utf16Data[position]) && position > 0 &&
        common::utf_helper::IsUTF16HighSurrogate(utf16Data[position -1])) {
        position--;
    }
    std::deque<uint32_t> dq;
    // position is the index of utf16Data, always less than 2^30
    int32_t suffixIndex = static_cast<int32_t>(position);
    int32_t prefixIndex = static_cast<int32_t>(position) - 1;
    uint32_t retPos = 0;
    for (uint32_t i = 0; i < width; i++) {
        if (static_cast<size_t>(suffixIndex + 1) < len &&
            common::utf_helper::IsUTF16HighSurrogate(utf16Data[suffixIndex]) &&
            common::utf_helper::IsUTF16LowSurrogate(utf16Data[suffixIndex + 1])) {
            dq.push_back(common::utf_helper::UTF16Decode(utf16Data[suffixIndex], utf16Data[suffixIndex + 1]));
            suffixIndex += 2; // 2:Consume a surrogate pair
        } else if (static_cast<size_t>(suffixIndex) < len) {
            dq.push_back(utf16Data[suffixIndex]);
            suffixIndex++;
        }
        if (prefixIndex > 0 && common::utf_helper::IsUTF16LowSurrogate(utf16Data[prefixIndex]) &&
            common::utf_helper::IsUTF16HighSurrogate(utf16Data[prefixIndex -1])) {
            dq.push_front(common::utf_helper::UTF16Decode(utf16Data[prefixIndex - 1], utf16Data[prefixIndex]));
            prefixIndex -= 2; // 2:Consume a surrogate pair
            retPos++;
        } else if (prefixIndex >= 0) {
            dq.push_front(utf16Data[prefixIndex]);
            prefixIndex--;
            retPos++;
        }
    }
    uint32_t utf8Length = 0;
    uint32_t index = 0;
    for (uint32_t &codePoint : dq) {
        if (IsJsonFormatCharacter(codePoint)) {
            utf8Length += common::utf_helper::UTF8Length(codePoint);
            index = 0;
        } else {
            index++;
            if (index % 2 == 0) { // 2:Replace even positions with *
                codePoint='*';
                utf8Length++;
            } else {
                utf8Length += common::utf_helper::UTF8Length(codePoint);
            }
        }
    }
    std::string utf8Str(utf8Length, '\0');
    uint32_t utf8Index = 0;
    for (const uint32_t &codePoint : dq) {
        uint32_t size = common::utf_helper::UTF8Length(codePoint);
        common::utf_helper::EncodeUTF8(codePoint, reinterpret_cast<uint8_t*>(utf8Str.data()), utf8Index, size);
        utf8Index += size;
    }
    return {utf8Str, retPos};
}

#if ENABLE_LATEST_OPTIMIZATION
constexpr int K_JSON_ESCAPE_TABLE_ENTRY_SIZE = 8;
constexpr uint8_t K_JSON_ESCAPE_CONTROL_LIMIT = 0x20;
constexpr uint8_t K_JSON_ESCAPE_QUOTE = 0x22;
constexpr uint8_t K_JSON_ESCAPE_BACKSLASH = 0x5C;
constexpr uint16_t K_JSON_ESCAPE_SURROGATE_START = 0xD800;
constexpr uint16_t K_JSON_ESCAPE_SURROGATE_END = 0xDFFF;
constexpr uint16_t K_JSON_ESCAPE_SURROGATE_PREV = K_JSON_ESCAPE_SURROGATE_START - 1;
constexpr uint16_t K_JSON_ESCAPE_SURROGATE_NEXT = K_JSON_ESCAPE_SURROGATE_END + 1;
constexpr uint32_t K_JSON_ESCAPE_PACKED_CONTROL_LIMIT = 0x20202020U;
constexpr uint32_t K_JSON_ESCAPE_PACKED_QUOTE = 0x22222222U;
constexpr uint32_t K_JSON_ESCAPE_PACKED_BACKSLASH = 0x5C5C5C5CU;
constexpr uint32_t K_JSON_ESCAPE_PACKED_ONES = 0x01010101U;
constexpr uint32_t K_JSON_ESCAPE_PACKED_MSB = 0x80808080U;

// Table for escaping Latin1 characters.
// Table entries start at a multiple of 8 with the first byte indicating length.
constexpr const char* const JSON_ESCAPE_TABLE =
    "\\u0000\0 \\u0001\0 \\u0002\0 \\u0003\0 \\u0004\0 \\u0005\0 \\u0006\0 \\u0007\0 "
    "\\b\0     \\t\0     \\n\0     \\u000b\0 \\f\0     \\r\0     \\u000e\0 \\u000f\0 "
    "\\u0010\0 \\u0011\0 \\u0012\0 \\u0013\0 \\u0014\0 \\u0015\0 \\u0016\0 \\u0017\0 "
    "\\u0018\0 \\u0019\0 \\u001a\0 \\u001b\0 \\u001c\0 \\u001d\0 \\u001e\0 \\u001f\0 "
    " \0      !\0      \\\"\0     #\0      $\0      %\0      &\0      '\0      "
    "(\0      )\0      *\0      +\0      ,\0      -\0      .\0      /\0      "
    "0\0      1\0      2\0      3\0      4\0      5\0      6\0      7\0      "
    "8\0      9\0      :\0      ;\0      <\0      =\0      >\0      ?\0      "
    "@\0      A\0      B\0      C\0      D\0      E\0      F\0      G\0      "
    "H\0      I\0      J\0      K\0      L\0      M\0      N\0      O\0      "
    "P\0      Q\0      R\0      S\0      T\0      U\0      V\0      W\0      "
    "X\0      Y\0      Z\0      [\0      \\\\\0     ]\0      ^\0      _\0      ";
 
constexpr uint8_t JSON_ESCAPE_LENGTH_TABLE[] = {
    6, 6, 6, 6, 6, 6, 6, 6, 2, 2, 2, 6, 2, 2, 6, 6,
    6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6, 6,
    1, 1, 2, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 2, 1, 1, 1,
};

constexpr bool JSON_DO_NOT_ESCAPE_FLAG_TABLE[] = {
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    1, 1, 0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

constexpr bool DoNotEscape(uint8_t c)
{
    return JSON_DO_NOT_ESCAPE_FLAG_TABLE[c];
}

bool DoNotEscape(uint16_t c)
{
    return (c >= K_JSON_ESCAPE_CONTROL_LIMIT && c <= (K_JSON_ESCAPE_QUOTE - 1)) ||
           (c >= (K_JSON_ESCAPE_QUOTE + 1) && c != K_JSON_ESCAPE_BACKSLASH &&
            (c < K_JSON_ESCAPE_SURROGATE_START || c > K_JSON_ESCAPE_SURROGATE_END));
}

constexpr bool NeedsEscape(uint32_t input)
{
    const uint32_t hasLt0x20 = input - K_JSON_ESCAPE_PACKED_CONTROL_LIMIT;
    const uint32_t has0x22 = (input ^ K_JSON_ESCAPE_PACKED_QUOTE) - K_JSON_ESCAPE_PACKED_ONES;
    const uint32_t has0x5C = (input ^ K_JSON_ESCAPE_PACKED_BACKSLASH) - K_JSON_ESCAPE_PACKED_ONES;
    const uint32_t resultMask = ~input & K_JSON_ESCAPE_PACKED_MSB;
    return ((hasLt0x20 | has0x22 | has0x5C) & resultMask) != 0;
}

size_t FindFirstEscapeScalar(const uint8_t *data, size_t start, size_t length)
{
    for (size_t i = start; i < length; ++i) {
        if (!DoNotEscape(data[i])) {
            return i;
        }
    }
    return length;
}

size_t FindFirstEscapeSWAR(const uint8_t *data, size_t start, size_t length)
{
    using PackedT = uint32_t;
    static constexpr size_t STRIDE = sizeof(PackedT);
    static constexpr size_t BYTE_BITS = 8;

    size_t i = start;
    for (; i + (STRIDE - 1) < length; i += STRIDE) {
        const PackedT packed = static_cast<PackedT>(data[i]) |
            (static_cast<PackedT>(data[i + 1]) << BYTE_BITS) |
            (static_cast<PackedT>(data[i + 2]) << (BYTE_BITS * 2)) |
            (static_cast<PackedT>(data[i + 3]) << (BYTE_BITS * 3));
        if (UNLIKELY(NeedsEscape(packed))) {
            break;
        }
    }
    return FindFirstEscapeScalar(data, i, length);
}

template <typename T, size_t N>
size_t FindFirstSetLane(const T (&lanes)[N])
{
    for (size_t lane = 0; lane < N; ++lane) {
        if (lanes[lane] != 0) {
            return lane;
        }
    }
    return N;
}

#if defined(PANDA_TARGET_AMD64)
struct JsonEscapeSimdConstants {
    static constexpr size_t SIMD_ALIGNMENT = 16;
    static constexpr size_t UTF8_STRIDE = 16;
    static constexpr size_t UTF16_STRIDE = 8;
    static constexpr size_t UTF8_SIMD_THRESHOLD = 32;
    static constexpr size_t UTF16_SIMD_THRESHOLD = 16;
    static constexpr size_t UTF16_BYTE_SHIFT = 1;
    static constexpr int16_t SIGNED_COMPARE_BIAS = static_cast<int16_t>(0x8000);
    static constexpr int16_t ALL_BITS_16 = static_cast<int16_t>(0xFFFF);
    static constexpr char ALL_BITS_8 = static_cast<char>(0xFF);

    JsonEscapeSimdConstants()
        : zero(_mm_setzero_si128()),
          allBits8(_mm_set1_epi8(ALL_BITS_8)),
          allBits16(_mm_set1_epi16(ALL_BITS_16)),
          utf8ControlLimit(_mm_set1_epi8(K_JSON_ESCAPE_CONTROL_LIMIT)),
          utf8Quote(_mm_set1_epi8(K_JSON_ESCAPE_QUOTE)),
          utf8Backslash(_mm_set1_epi8(K_JSON_ESCAPE_BACKSLASH)),
          utf16ControlLimit(_mm_set1_epi16(K_JSON_ESCAPE_CONTROL_LIMIT)),
          utf16Quote(_mm_set1_epi16(K_JSON_ESCAPE_QUOTE)),
          utf16Backslash(_mm_set1_epi16(K_JSON_ESCAPE_BACKSLASH)),
          utf16Bias(_mm_set1_epi16(SIGNED_COMPARE_BIAS)),
          utf16LowSurrogateBias(_mm_set1_epi16(static_cast<int16_t>(K_JSON_ESCAPE_SURROGATE_PREV ^
                                                                    SIGNED_COMPARE_BIAS))),
          utf16HighSurrogateBias(_mm_set1_epi16(static_cast<int16_t>(K_JSON_ESCAPE_SURROGATE_NEXT ^
                                                                     SIGNED_COMPARE_BIAS)))
    {
    }

    const __m128i zero;
    const __m128i allBits8;
    const __m128i allBits16;
    const __m128i utf8ControlLimit;
    const __m128i utf8Quote;
    const __m128i utf8Backslash;
    const __m128i utf16ControlLimit;
    const __m128i utf16Quote;
    const __m128i utf16Backslash;
    const __m128i utf16Bias;
    const __m128i utf16LowSurrogateBias;
    const __m128i utf16HighSurrogateBias;
};

const JsonEscapeSimdConstants &GetJsonEscapeSimdConstants()
{
    static const JsonEscapeSimdConstants constants;
    return constants;
}

size_t FindFirstEscapeSIMD(const uint8_t *data, size_t length)
{
    const auto &constants = GetJsonEscapeSimdConstants();

    size_t i = 0;
    for (; i + (JsonEscapeSimdConstants::UTF8_STRIDE - 1) < length; i += JsonEscapeSimdConstants::UTF8_STRIDE) {
        const __m128i input = _mm_loadu_si128(reinterpret_cast<const __m128i *>(data + i));
        const __m128i lowerThan0x20 = _mm_xor_si128(_mm_cmpeq_epi8(
            _mm_subs_epu8(constants.utf8ControlLimit, input), constants.zero), constants.allBits8);
        const __m128i isQuote = _mm_cmpeq_epi8(input, constants.utf8Quote);
        const __m128i isBackslash = _mm_cmpeq_epi8(input, constants.utf8Backslash);
        const __m128i result = _mm_or_si128(_mm_or_si128(lowerThan0x20, isQuote), isBackslash);
        const uint32_t bitMask = static_cast<uint32_t>(_mm_movemask_epi8(result));
        if (UNLIKELY(bitMask != 0)) {
            return i + static_cast<size_t>(__builtin_ctz(bitMask));
        }
    }
    return FindFirstEscapeSWAR(data, i, length);
}

#elif defined(PANDA_TARGET_ARM64) && !defined(PANDA_TARGET_MACOS)
struct JsonEscapeSimdConstants {
    static constexpr size_t SIMD_ALIGNMENT = 16;
    static constexpr size_t UTF8_STRIDE = 16;
    static constexpr size_t UTF16_STRIDE = 8;
    static constexpr size_t UTF8_SIMD_THRESHOLD = 32;
    static constexpr size_t UTF16_SIMD_THRESHOLD = 16;

    JsonEscapeSimdConstants()
        : utf8ControlLimit(vdupq_n_u8(K_JSON_ESCAPE_CONTROL_LIMIT)),
          utf8Quote(vdupq_n_u8(K_JSON_ESCAPE_QUOTE)),
          utf8Backslash(vdupq_n_u8(K_JSON_ESCAPE_BACKSLASH)),
          utf16ControlLimit(vdupq_n_u16(K_JSON_ESCAPE_CONTROL_LIMIT)),
          utf16Quote(vdupq_n_u16(K_JSON_ESCAPE_QUOTE)),
          utf16Backslash(vdupq_n_u16(K_JSON_ESCAPE_BACKSLASH)),
          utf16LowSurrogate(vdupq_n_u16(K_JSON_ESCAPE_SURROGATE_START)),
          utf16HighSurrogate(vdupq_n_u16(K_JSON_ESCAPE_SURROGATE_END))
    {
    }

    const uint8x16_t utf8ControlLimit;
    const uint8x16_t utf8Quote;
    const uint8x16_t utf8Backslash;
    const uint16x8_t utf16ControlLimit;
    const uint16x8_t utf16Quote;
    const uint16x8_t utf16Backslash;
    const uint16x8_t utf16LowSurrogate;
    const uint16x8_t utf16HighSurrogate;
};

const JsonEscapeSimdConstants &GetJsonEscapeSimdConstants()
{
    static const JsonEscapeSimdConstants constants;
    return constants;
}

size_t FindFirstEscapeSIMD(const uint8_t *data, size_t length)
{
    const auto &constants = GetJsonEscapeSimdConstants();

    size_t i = 0;
    for (; i + (JsonEscapeSimdConstants::UTF8_STRIDE - 1) < length; i += JsonEscapeSimdConstants::UTF8_STRIDE) {
        const uint8x16_t input = vld1q_u8(data + i);
        const uint8x16_t lowerThan0x20 = vcltq_u8(input, constants.utf8ControlLimit);
        const uint8x16_t isQuote = vceqq_u8(input, constants.utf8Quote);
        const uint8x16_t isBackslash = vceqq_u8(input, constants.utf8Backslash);
        const uint8x16_t result = vorrq_u8(vorrq_u8(lowerThan0x20, isQuote), isBackslash);
        const uint64x2_t result64 = vreinterpretq_u64_u8(result);
        if (UNLIKELY((vgetq_lane_u64(result64, 0) | vgetq_lane_u64(result64, 1)) != 0)) {
            alignas(JsonEscapeSimdConstants::SIMD_ALIGNMENT) uint8_t maskLanes[JsonEscapeSimdConstants::UTF8_STRIDE];
            vst1q_u8(maskLanes, result);
            return i + FindFirstSetLane(maskLanes);
        }
    }
    return FindFirstEscapeSWAR(data, i, length);
}
#endif

size_t FindFirstEscape(const uint8_t *data, size_t length)
{
#if defined(PANDA_TARGET_AMD64) || (defined(PANDA_TARGET_ARM64) && !defined(PANDA_TARGET_MACOS))
    if (length >= JsonEscapeSimdConstants::UTF8_SIMD_THRESHOLD) {
        return FindFirstEscapeSIMD(data, length);
    }
#endif
    return FindFirstEscapeSWAR(data, 0, length);
}

inline bool NeedsEscapeUtf16Scalar(uint16_t ch)
{
    return !DoNotEscape(ch);
}

size_t FindFirstEscapeScalar(const uint16_t *data, size_t start, size_t length)
{
    for (size_t i = start; i < length; ++i) {
        if (NeedsEscapeUtf16Scalar(data[i])) {
            return i;
        }
    }
    return length;
}

size_t FindFirstEscapeSWAR(const uint16_t *data, size_t start, size_t length)
{
    static constexpr size_t STRIDE = 4;
    static constexpr size_t UTF16_LANE_BITS = 16;

    size_t i = start;
    for (; i + (STRIDE - 1) < length; i += STRIDE) {
        const uint64_t packed = static_cast<uint64_t>(data[i]) |
            (static_cast<uint64_t>(data[i + 1]) << UTF16_LANE_BITS) |
            (static_cast<uint64_t>(data[i + 2]) << (UTF16_LANE_BITS * 2)) |
            (static_cast<uint64_t>(data[i + 3]) << (UTF16_LANE_BITS * 3));
        const uint16_t lane0 = static_cast<uint16_t>(packed);
        const uint16_t lane1 = static_cast<uint16_t>(packed >> UTF16_LANE_BITS);
        const uint16_t lane2 = static_cast<uint16_t>(packed >> (UTF16_LANE_BITS * 2));
        const uint16_t lane3 = static_cast<uint16_t>(packed >> (UTF16_LANE_BITS * 3));
        if (UNLIKELY(NeedsEscapeUtf16Scalar(lane0) || NeedsEscapeUtf16Scalar(lane1) ||
                     NeedsEscapeUtf16Scalar(lane2) || NeedsEscapeUtf16Scalar(lane3))) {
            break;
        }
    }
    return FindFirstEscapeScalar(data, i, length);
}

#if defined(PANDA_TARGET_AMD64)
size_t FindFirstEscapeSIMD(const uint16_t *data, size_t length)
{
    const auto &constants = GetJsonEscapeSimdConstants();

    size_t i = 0;
    for (; i + (JsonEscapeSimdConstants::UTF16_STRIDE - 1) < length; i += JsonEscapeSimdConstants::UTF16_STRIDE) {
        const __m128i input = _mm_loadu_si128(reinterpret_cast<const __m128i *>(data + i));
        const __m128i lowerThan0x20 = _mm_xor_si128(_mm_cmpeq_epi16(
            _mm_subs_epu16(constants.utf16ControlLimit, input), constants.zero), constants.allBits16);
        const __m128i isQuote = _mm_cmpeq_epi16(input, constants.utf16Quote);
        const __m128i isBackslash = _mm_cmpeq_epi16(input, constants.utf16Backslash);
        const __m128i biasedInput = _mm_xor_si128(input, constants.utf16Bias);
        const __m128i greaterThanLowSurrogate = _mm_cmpgt_epi16(biasedInput, constants.utf16LowSurrogateBias);
        const __m128i lessThanHighSurrogate = _mm_cmpgt_epi16(constants.utf16HighSurrogateBias, biasedInput);
        const __m128i isSurrogate = _mm_and_si128(greaterThanLowSurrogate, lessThanHighSurrogate);
        const __m128i result = _mm_or_si128(_mm_or_si128(lowerThan0x20, isQuote),
                                            _mm_or_si128(isBackslash, isSurrogate));
        const uint32_t bitMask = static_cast<uint32_t>(_mm_movemask_epi8(result));
        if (UNLIKELY(bitMask != 0)) {
            return i + (static_cast<size_t>(__builtin_ctz(bitMask)) >> JsonEscapeSimdConstants::UTF16_BYTE_SHIFT);
        }
    }
    return FindFirstEscapeSWAR(data, i, length);
}

#elif defined(PANDA_TARGET_ARM64) && !defined(PANDA_TARGET_MACOS)
size_t FindFirstEscapeSIMD(const uint16_t *data, size_t length)
{
    const auto &constants = GetJsonEscapeSimdConstants();

    size_t i = 0;
    for (; i + (JsonEscapeSimdConstants::UTF16_STRIDE - 1) < length; i += JsonEscapeSimdConstants::UTF16_STRIDE) {
        const uint16x8_t input = vld1q_u16(data + i);
        const uint16x8_t lowerThan0x20 = vcltq_u16(input, constants.utf16ControlLimit);
        const uint16x8_t isQuote = vceqq_u16(input, constants.utf16Quote);
        const uint16x8_t isBackslash = vceqq_u16(input, constants.utf16Backslash);
        const uint16x8_t greaterThanLowSurrogate = vcgeq_u16(input, constants.utf16LowSurrogate);
        const uint16x8_t lessThanHighSurrogate = vcleq_u16(input, constants.utf16HighSurrogate);
        const uint16x8_t isSurrogate = vandq_u16(greaterThanLowSurrogate, lessThanHighSurrogate);
        const uint16x8_t result = vorrq_u16(vorrq_u16(lowerThan0x20, isQuote),
                                            vorrq_u16(isBackslash, isSurrogate));
        const uint64x2_t result64 = vreinterpretq_u64_u16(result);
        if (UNLIKELY((vgetq_lane_u64(result64, 0) | vgetq_lane_u64(result64, 1)) != 0)) {
            alignas(JsonEscapeSimdConstants::SIMD_ALIGNMENT) uint16_t maskLanes[JsonEscapeSimdConstants::UTF16_STRIDE];
            vst1q_u16(maskLanes, result);
            return i + FindFirstSetLane(maskLanes);
        }
    }
    return FindFirstEscapeSWAR(data, i, length);
}
#endif

size_t FindFirstEscape(const uint16_t *data, size_t length)
{
#if defined(PANDA_TARGET_AMD64) || (defined(PANDA_TARGET_ARM64) && !defined(PANDA_TARGET_MACOS))
    if (length >= JsonEscapeSimdConstants::UTF16_SIMD_THRESHOLD) {
        return FindFirstEscapeSIMD(data, length);
    }
#endif
    return FindFirstEscapeSWAR(data, 0, length);
}

bool JsonHelper::IsFastValueToQuotedString(const common::Span<const uint8_t> &sp)
{
    return FindFirstEscape(sp.data(), sp.size()) == sp.size();
}
#else
bool JsonHelper::IsFastValueToQuotedString(const CString& str)
{
    for (const auto item : str) {
        const auto ch = static_cast<uint8_t>(item);
        switch (ch) {
            case '\"':
            case '\\':
            case '\b':
            case '\f':
            case '\n':
            case '\r':
            case '\t':
            case ZERO_FIRST:
            case ALONE_SURROGATE_3B_FIRST:
                return false;
            default:
                if (ch < CODE_SPACE) {
                    return false;
                }
                break;
        }
    }
    return true;
}
#endif

#if ENABLE_LATEST_OPTIMIZATION
template <typename T>
bool AppendValueToQuotedStringUtf8(const common::Span<const uint8_t> &sp, T &output)
{
    size_t firstEscape = FindFirstEscape(reinterpret_cast<const uint8_t *>(sp.data()), sp.size());
    if (firstEscape == sp.size()) {
        output.AppendString(reinterpret_cast<const char *>(sp.data()), sp.size());
        return true;
    }

    if (firstEscape > 0) {
        output.AppendString(sp.data(), firstEscape);
    }

    size_t lastNonEscapePos = firstEscape + 1;
    const auto firstCh = sp[firstEscape];
    const char *firstEscapeStr = &JSON_ESCAPE_TABLE[firstCh * K_JSON_ESCAPE_TABLE_ENTRY_SIZE];
    output.AppendString(firstEscapeStr, JSON_ESCAPE_LENGTH_TABLE[firstCh]);

    while (lastNonEscapePos < sp.size()) {
        size_t nextEscape = FindFirstEscape(reinterpret_cast<const uint8_t *>(sp.data()) + lastNonEscapePos,
                                            sp.size() - lastNonEscapePos);
        if (nextEscape == sp.size() - lastNonEscapePos) {
            output.AppendString(sp.data() + lastNonEscapePos, sp.size() - lastNonEscapePos);
            return false;
        }

        size_t escapeIndex = lastNonEscapePos + nextEscape;
        if (escapeIndex > lastNonEscapePos) {
            output.AppendString(sp.data() + lastNonEscapePos, escapeIndex - lastNonEscapePos);
        }

        const auto ch = sp[escapeIndex];
        const char *escapeStr = &JSON_ESCAPE_TABLE[ch * K_JSON_ESCAPE_TABLE_ENTRY_SIZE];
        output.AppendString(escapeStr, JSON_ESCAPE_LENGTH_TABLE[ch]);
        lastNonEscapePos = escapeIndex + 1;
    }

    return false;
}

template <typename T>
void AppendValueToQuotedStringUtf16(const common::Span<const uint16_t> &sp, T &output)
{
    size_t lastNonEscapePos = 0;

    while (lastNonEscapePos < sp.size()) {
        size_t nextEscape = FindFirstEscape(reinterpret_cast<const uint16_t *>(sp.data()) + lastNonEscapePos,
                                            sp.size() - lastNonEscapePos);
        if (nextEscape == sp.size() - lastNonEscapePos) {
            output.AppendString(sp.data() + lastNonEscapePos, sp.size() - lastNonEscapePos);
            return;
        }

        size_t escapeIndex = lastNonEscapePos + nextEscape;
        if (LIKELY(escapeIndex > lastNonEscapePos)) {
            output.AppendString(sp.data() + lastNonEscapePos, escapeIndex - lastNonEscapePos);
        }

        auto utf16Ch = sp[escapeIndex];
        if (common::utf_helper::IsUTF16Surrogate(utf16Ch)) {
            if (utf16Ch <= common::utf_helper::DECODE_LEAD_HIGH && escapeIndex + 1 < sp.size() &&
                common::utf_helper::IsUTF16LowSurrogate(sp[escapeIndex + 1])) {
                output.Append(utf16Ch);
                output.Append(sp[escapeIndex + 1]);
                lastNonEscapePos = escapeIndex + 2; // 2: surrogate pair
                continue;
            }
            JsonHelper::AppendUnicodeEscape(static_cast<uint32_t>(utf16Ch), output);
        } else {
            ASSERT(utf16Ch < 0x60);
            const char *escapeStr = &JSON_ESCAPE_TABLE[utf16Ch * K_JSON_ESCAPE_TABLE_ENTRY_SIZE];
            output.AppendString(escapeStr, JSON_ESCAPE_LENGTH_TABLE[utf16Ch]);
        }
        lastNonEscapePos = escapeIndex + 1;
    }
}

template <typename SrcType, typename T>
bool JsonHelper::AppendValueToQuotedString(const common::Span<const SrcType> &sp, T &output)
{
    using CharType [[maybe_unused]] = typename T::value_type;
    ASSERT(sizeof(CharType) >= sizeof(SrcType));
    output.Append('"');
    if constexpr (sizeof(SrcType) == 1) {
        bool isFast = AppendValueToQuotedStringUtf8(sp, output);
        output.Append('"');
        return isFast;
    } else {
        AppendValueToQuotedStringUtf16(sp, output);
    }

    output.Append('"');
    return false;
}

template bool JsonHelper::AppendValueToQuotedString<uint8_t, JsonStringifier::FastStringBuilder<uint8_t>>(
    const common::Span<const uint8_t> &sp, JsonStringifier::FastStringBuilder<uint8_t> &output);
template bool JsonHelper::AppendValueToQuotedString<uint8_t, JsonStringifier::FastStringBuilder<uint16_t>>(
    const common::Span<const uint8_t> &sp, JsonStringifier::FastStringBuilder<uint16_t> &output);
template bool JsonHelper::AppendValueToQuotedString<uint16_t, JsonStringifier::FastStringBuilder<uint8_t>>(
    const common::Span<const uint16_t> &sp, JsonStringifier::FastStringBuilder<uint8_t> &output);
template bool JsonHelper::AppendValueToQuotedString<uint16_t, JsonStringifier::FastStringBuilder<uint16_t>>(
    const common::Span<const uint16_t> &sp, JsonStringifier::FastStringBuilder<uint16_t> &output);

#else
void JsonHelper::AppendValueToQuotedString(const CString& str, CString& output)
{
    output += "\"";
    bool isFast = IsFastValueToQuotedString(str); // fast mode
    if (isFast) {
        output += str;
        output += "\"";
        return;
    }
    for (uint32_t i = 0; i < str.size(); ++i) {
        const auto ch = static_cast<uint8_t>(str[i]);
        switch (ch) {
            case '\"':
                output += "\\\"";
                break;
            case '\\':
                output += "\\\\";
                break;
            case '\b':
                output += "\\b";
                break;
            case '\f':
                output += "\\f";
                break;
            case '\n':
                output += "\\n";
                break;
            case '\r':
                output += "\\r";
                break;
            case '\t':
                output += "\\t";
                break;
            case ZERO_FIRST:
                output += "\\u0000";
                ++i;
                break;
            case ALONE_SURROGATE_3B_FIRST:
                if (i + 2 < str.size() && // 2: Check 2 more characters
                    static_cast<uint8_t>(str[i + 1]) >= ALONE_SURROGATE_3B_SECOND_START && // 1: 1th character after ch
                    static_cast<uint8_t>(str[i + 1]) <= ALONE_SURROGATE_3B_SECOND_END && // 1: 1th character after ch
                    static_cast<uint8_t>(str[i + 2]) >= ALONE_SURROGATE_3B_THIRD_START && // 2: 2nd character after ch
                    static_cast<uint8_t>(str[i + 2]) <= ALONE_SURROGATE_3B_THIRD_END) {   // 2: 2nd character after ch
                    auto unicodeRes = common::utf_helper::ConvertUtf8ToUnicodeChar(
                        reinterpret_cast<const uint8_t*>(str.c_str() + i), 3); // 3: Parse 3 characters
                    ASSERT(unicodeRes.first != common::utf_helper::INVALID_UTF8);
                    AppendUnicodeEscape(static_cast<uint32_t>(unicodeRes.first), output);
                    i += 2; // 2 : Skip 2 characters
                    break;
                }
                [[fallthrough]];
            default:
                if (ch < CODE_SPACE) {
                    AppendUnicodeEscape(static_cast<uint32_t>(ch), output);
                } else {
                    output += ch;
                }
        }
    }
    output += "\"";
}
#endif
} // namespace panda::ecmascript::base
