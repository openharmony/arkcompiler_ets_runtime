/*
 * Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_PLATFORM_JSON_HELPER_INTERNAL_X64_H
#define ECMASCRIPT_PLATFORM_JSON_HELPER_INTERNAL_X64_H

#include <cstddef>
#include <cstdint>
#include <emmintrin.h>

#include "macros.h"

#if defined(_MSC_VER)
#include <intrin.h>
#endif

namespace panda::ecmascript::base {
class JsonHelperInternal {
    friend class JsonPlatformHelper;
private:
    enum class ParseStepResult : uint8_t {
        CONTINUE,
        RETURN_TRUE,
        RETURN_FALSE,
    };

    static constexpr uint8_t QUOTE_CHAR = '"';
    static constexpr uint8_t BACKSLASH_CHAR = '\\';
    static constexpr uint8_t SPACE_CHAR = ' ';
    static constexpr size_t CHUNK_SIZE = 16;
    static constexpr uint8_t ASCII_END = 0x7F;

    static constexpr uint16_t UTF16_QUOTE_CHAR = 0x0022;
    static constexpr uint16_t UTF16_BACKSLASH_CHAR = 0x005C;
    static constexpr uint16_t UTF16_SPACE_CHAR = 0x0020;
    static constexpr uint16_t UTF16_ASCII_END = 0x007F;
    static constexpr size_t UTF16_CHUNK_SIZE = 8;

    static inline uint32_t CountTrailingZeroes(uint32_t x)
    {
#if defined(_MSC_VER)
        unsigned long index = 0;
        _BitScanForward(&index, x);
        return static_cast<uint32_t>(index);
#else
        return static_cast<uint32_t>(__builtin_ctz(x));
#endif
    }

    #include "ecmascript/platform/x64/json_helper_internal_helpers.inl"

    static bool ReadJsonStringRangeForUtf8(bool &isFastString, const uint8_t *&current,
                                           const uint8_t *range, const uint8_t *&end)
    {
        const uint8_t *cur = current;

        const __m128i quoteVector = _mm_set1_epi8(static_cast<char>(QUOTE_CHAR));
        const __m128i backslashVector = _mm_set1_epi8(static_cast<char>(BACKSLASH_CHAR));
        const __m128i spaceVector = _mm_set1_epi8(static_cast<char>(SPACE_CHAR));
        const __m128i signBitFlipVector = _mm_set1_epi8(static_cast<char>(0x80));
        const __m128i biasedSpaceVector = _mm_xor_si128(spaceVector, signBitFlipVector);

        for (; cur + CHUNK_SIZE <= range; cur += CHUNK_SIZE) {
            const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(cur));
            const __m128i quoteMaskVector = _mm_cmpeq_epi8(chunk, quoteVector);
            const __m128i backslashMaskVector = _mm_cmpeq_epi8(chunk, backslashVector);
            const __m128i biasedChunk = _mm_xor_si128(chunk, signBitFlipVector);
            const __m128i controlMaskVector = _mm_cmpgt_epi8(biasedSpaceVector, biasedChunk);

            const uint32_t quoteMask = static_cast<uint32_t>(_mm_movemask_epi8(quoteMaskVector));
            const uint32_t backslashMask = static_cast<uint32_t>(_mm_movemask_epi8(backslashMaskVector));
            const uint32_t controlMask = static_cast<uint32_t>(_mm_movemask_epi8(controlMaskVector));
            const uint32_t combinedMask = quoteMask | backslashMask | controlMask;

            if (combinedMask != 0) {
                const uint32_t firstIndex = CountTrailingZeroes(combinedMask);
                const uint32_t firstBit = static_cast<uint32_t>(1U) << firstIndex;

                if ((quoteMask & firstBit) != 0) {
                    end = cur + firstIndex;
                    return true;
                }
                if (UNLIKELY((backslashMask & firstBit) != 0)) {
                    isFastString = false;
                    return true;
                }
                if (UNLIKELY((controlMask & firstBit) != 0)) {
                    current = cur + firstIndex;
                    return false;
                }
            }
        }
        current = cur;
        return ScanJsonStringRangeForUtf8Scalar(isFastString, current, range, end);
    }

    static bool ReadJsonStringRangeForUtf16(bool &isFastString, bool &isAscii,
                                            const uint16_t *&current,
                                            const uint16_t *range,
                                            const uint16_t *&end)
    {
        const uint16_t *cur = current;

        const __m128i quoteVector = _mm_set1_epi16(static_cast<int16_t>(UTF16_QUOTE_CHAR));
        const __m128i backslashVector = _mm_set1_epi16(static_cast<int16_t>(UTF16_BACKSLASH_CHAR));
        const __m128i spaceVector = _mm_set1_epi16(static_cast<int16_t>(UTF16_SPACE_CHAR));
        const __m128i asciiEndVector = _mm_set1_epi16(static_cast<int16_t>(UTF16_ASCII_END));
        const __m128i signBitFlipVector = _mm_set1_epi16(static_cast<int16_t>(0x8000));
        const __m128i biasedSpaceVector = _mm_xor_si128(spaceVector, signBitFlipVector);
        const __m128i biasedAsciiEndVector = _mm_xor_si128(asciiEndVector, signBitFlipVector);

        for (; cur + UTF16_CHUNK_SIZE <= range; cur += UTF16_CHUNK_SIZE) {
            const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(cur));

            const __m128i quoteMaskVector = _mm_cmpeq_epi16(chunk, quoteVector);
            const __m128i backslashMaskVector = _mm_cmpeq_epi16(chunk, backslashVector);

            const __m128i biasedChunk = _mm_xor_si128(chunk, signBitFlipVector);
            const __m128i controlMaskVector = _mm_cmpgt_epi16(biasedSpaceVector, biasedChunk);
            const __m128i nonAsciiMaskVector = _mm_cmpgt_epi16(biasedChunk, biasedAsciiEndVector);

            const uint32_t quoteMask = static_cast<uint32_t>(_mm_movemask_epi8(quoteMaskVector));
            const uint32_t backslashMask = static_cast<uint32_t>(_mm_movemask_epi8(backslashMaskVector));
            const uint32_t controlMask = static_cast<uint32_t>(_mm_movemask_epi8(controlMaskVector));
            const uint32_t nonAsciiMask = static_cast<uint32_t>(_mm_movemask_epi8(nonAsciiMaskVector));
            uint32_t combinedMask = quoteMask | backslashMask | controlMask | nonAsciiMask;

            ParseStepResult result = HandleUtf16StringRangeChunk(isFastString, isAscii, cur, combinedMask,
                                                                 quoteMask, backslashMask, controlMask,
                                                                 nonAsciiMask, current, end);
            if (result == ParseStepResult::RETURN_TRUE) {
                return true;
            }
            if (result == ParseStepResult::RETURN_FALSE) {
                return false;
            }
        }

        current = cur;
        return ScanJsonStringRangeForUtf16Scalar(isFastString, isAscii, current, range, end);
    }

    template <typename CheckBackslashFunc>
    static bool ParseStringLengthForUtf8(size_t &length, bool &isAscii, bool inObjOrArrOrMap,
                                         const uint8_t *&current,
                                         const uint8_t *range,
                                         const uint8_t *&end,
                                         CheckBackslashFunc checkBackslash)
    {
        const uint8_t *cur = current;
        const uint8_t *last = inObjOrArrOrMap ? range : end;

        const __m128i quoteVector = _mm_set1_epi8(static_cast<char>(QUOTE_CHAR));
        const __m128i backslashVector = _mm_set1_epi8(static_cast<char>(BACKSLASH_CHAR));
        const __m128i spaceVector = _mm_set1_epi8(static_cast<char>(SPACE_CHAR));
        const __m128i signBitFlipVector = _mm_set1_epi8(static_cast<char>(0x80));
        const __m128i biasedSpaceVector = _mm_xor_si128(spaceVector, signBitFlipVector);

        while (cur + CHUNK_SIZE <= last) {
            const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(cur));

            const __m128i backslashMaskVector = _mm_cmpeq_epi8(chunk, backslashVector);

            const __m128i biasedChunk = _mm_xor_si128(chunk, signBitFlipVector);
            const __m128i controlMaskVector = _mm_cmpgt_epi8(biasedSpaceVector, biasedChunk);

            const uint32_t quoteMask = inObjOrArrOrMap
                ? static_cast<uint32_t>(_mm_movemask_epi8(_mm_cmpeq_epi8(chunk, quoteVector)))
                : 0;

            const uint32_t backslashMask = static_cast<uint32_t>(_mm_movemask_epi8(backslashMaskVector));
            const uint32_t controlMask = static_cast<uint32_t>(_mm_movemask_epi8(controlMaskVector));

            // For UTF-8 bytes, high bit set means byte > 0x7F.
            // Non-ASCII does not stop parsing; it only flips isAscii.
            const uint32_t nonAsciiMask = static_cast<uint32_t>(_mm_movemask_epi8(chunk));

            const uint32_t combinedMask = quoteMask | backslashMask | controlMask | nonAsciiMask;

            if (combinedMask == 0) {
                cur += CHUNK_SIZE;
                length += CHUNK_SIZE;
                continue;
            }

            ParseStepResult result = HandleUtf8LengthChunk(length, isAscii, cur, last, current, end,
                                                           quoteMask, backslashMask, controlMask, nonAsciiMask,
                                                           checkBackslash);
            if (result == ParseStepResult::RETURN_TRUE) {
                return true;
            }
            if (result == ParseStepResult::RETURN_FALSE) {
                return false;
            }
        }

        current = cur;
        return ParseStringLengthScalarForUtf8(length, isAscii, inObjOrArrOrMap, current, last, end,
                                              checkBackslash);
    }

    template <typename CheckBackslashFunc>
    static bool ParseStringLengthForUtf16(size_t &length, bool &isAscii, bool inObjOrArrOrMap,
                                          const uint16_t *&current,
                                          const uint16_t *range,
                                          const uint16_t *&end,
                                          CheckBackslashFunc checkBackslash)
    {
        const uint16_t *cur = current;
        const uint16_t *last = inObjOrArrOrMap ? range : end;

        const __m128i quoteVector = _mm_set1_epi16(static_cast<int16_t>(UTF16_QUOTE_CHAR));
        const __m128i backslashVector = _mm_set1_epi16(static_cast<int16_t>(UTF16_BACKSLASH_CHAR));
        const __m128i spaceVector = _mm_set1_epi16(static_cast<int16_t>(UTF16_SPACE_CHAR));
        const __m128i asciiEndVector = _mm_set1_epi16(static_cast<int16_t>(UTF16_ASCII_END));

        const __m128i signBitFlipVector = _mm_set1_epi16(static_cast<int16_t>(0x8000));
        const __m128i biasedSpaceVector = _mm_xor_si128(spaceVector, signBitFlipVector);
        const __m128i biasedAsciiEndVector = _mm_xor_si128(asciiEndVector, signBitFlipVector);

        while (cur + UTF16_CHUNK_SIZE <= last) {
            const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(cur));

            const __m128i quoteMaskVector = inObjOrArrOrMap
                ? _mm_cmpeq_epi16(chunk, quoteVector)
                : _mm_setzero_si128();

            const __m128i backslashMaskVector = _mm_cmpeq_epi16(chunk, backslashVector);

            const __m128i biasedChunk = _mm_xor_si128(chunk, signBitFlipVector);
            const __m128i controlMaskVector = _mm_cmpgt_epi16(biasedSpaceVector, biasedChunk);

            const __m128i nonAsciiMaskVector = isAscii
                ? _mm_cmpgt_epi16(biasedChunk, biasedAsciiEndVector)
                : _mm_setzero_si128();

            const uint32_t quoteMask = static_cast<uint32_t>(_mm_movemask_epi8(quoteMaskVector));
            const uint32_t backslashMask = static_cast<uint32_t>(_mm_movemask_epi8(backslashMaskVector));
            const uint32_t controlMask = static_cast<uint32_t>(_mm_movemask_epi8(controlMaskVector));
            const uint32_t nonAsciiMask = static_cast<uint32_t>(_mm_movemask_epi8(nonAsciiMaskVector));

            const uint32_t combinedMask = quoteMask | backslashMask | controlMask | nonAsciiMask;

            if (combinedMask == 0) {
                cur += UTF16_CHUNK_SIZE;
                length += UTF16_CHUNK_SIZE;
                continue;
            }

            ParseStepResult result = HandleUtf16LengthChunk(length, isAscii, cur, last, current, end,
                                                            quoteMask, backslashMask, controlMask, nonAsciiMask,
                                                            checkBackslash);
            if (result == ParseStepResult::RETURN_TRUE) {
                return true;
            }
            if (result == ParseStepResult::RETURN_FALSE) {
                return false;
            }
        }

        current = cur;
        return ParseStringLengthScalarForUtf16(length, isAscii, inObjOrArrOrMap, current, last, end,
                                               checkBackslash);
    }

    template <typename Char, typename ParseBackslashFunc>
    static void CopyCharWithBackslashForUtf8(Char *&p,
                                             const uint8_t *&current,
                                             const uint8_t *end,
                                             ParseBackslashFunc parseBackslash)
    {
        const uint8_t *cur = current;
        const uint8_t *last = end + 1;

        const __m128i backslashVector = _mm_set1_epi8(static_cast<char>(BACKSLASH_CHAR));

        while (cur + CHUNK_SIZE <= last) {
            const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(cur));
            const __m128i backslashMaskVector = _mm_cmpeq_epi8(chunk, backslashVector);
            const uint32_t backslashMask = static_cast<uint32_t>(_mm_movemask_epi8(backslashMaskVector));
            if (backslashMask == 0) {
                CopyPlainUtf8Chars(p, cur, CHUNK_SIZE);
                cur += CHUNK_SIZE;
                continue;
            }

            const uint32_t firstIndex = CountTrailingZeroes(backslashMask);

            CopyPlainUtf8Chars(p, cur, firstIndex);
            cur += firstIndex;

            current = cur;
            parseBackslash(p);

            ++current;
            cur = current;
        }

        current = cur;

        while (current <= end) {
            uint8_t c = *current;
            if (c == BACKSLASH_CHAR) {
                parseBackslash(p);
            } else {
                *p++ = static_cast<Char>(c);
            }
            ++current;
        }
    }

    template <typename Char, typename ParseBackslashFunc>
    static void CopyCharWithBackslashForUtf16(Char *&p,
                                              const uint16_t *&current,
                                              const uint16_t *end,
                                              ParseBackslashFunc parseBackslash)
    {
        const uint16_t *cur = current;
        const uint16_t *last = end + 1;

        const __m128i backslashVector = _mm_set1_epi16(static_cast<int16_t>(UTF16_BACKSLASH_CHAR));

        while (cur + UTF16_CHUNK_SIZE <= last) {
            const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(cur));
            const __m128i backslashMaskVector = _mm_cmpeq_epi16(chunk, backslashVector);
            const uint32_t backslashMask = static_cast<uint32_t>(_mm_movemask_epi8(backslashMaskVector));
            if (backslashMask == 0) {
                CopyPlainUtf16Chars(p, cur, UTF16_CHUNK_SIZE);
                cur += UTF16_CHUNK_SIZE;
                continue;
            }

            // movemask is per byte; each UTF-16 lane contributes two bits.
            const uint32_t firstIndex = CountTrailingZeroes(backslashMask) >> 1;

            CopyPlainUtf16Chars(p, cur, firstIndex);
            cur += firstIndex;

            current = cur;
            parseBackslash(p);

            ++current;
            cur = current;
        }

        current = cur;

        while (current <= end) {
            uint16_t c = *current;
            if (c == UTF16_BACKSLASH_CHAR) {
                parseBackslash(p);
            } else {
                *p++ = static_cast<Char>(c);
            }
            ++current;
        }
    }
};
}  // namespace panda::ecmascript::base
#endif  // ECMASCRIPT_PLATFORM_JSON_HELPER_INTERNAL_X64_H
