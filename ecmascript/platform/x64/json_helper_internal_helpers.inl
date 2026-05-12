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

    static bool ScanJsonStringRangeForUtf8Scalar(bool &isFastString, const uint8_t *&current,
                                                 const uint8_t *range, const uint8_t *&end)
    {
        for (; current != range; ++current) {
            if (*current == QUOTE_CHAR) {
                end = current;
                return true;
            }
            if (UNLIKELY(*current == BACKSLASH_CHAR)) {
                isFastString = false;
                return true;
            }
            if (UNLIKELY(*current < SPACE_CHAR)) {
                return false;
            }
        }
        current = range;
        return false;
    }

    static ParseStepResult HandleUtf16StringRangeChunk(bool &isFastString, bool &isAscii,
                                                       const uint16_t *cur, uint32_t combinedMask,
                                                       uint32_t quoteMask, uint32_t backslashMask,
                                                       uint32_t controlMask, uint32_t nonAsciiMask,
                                                       const uint16_t *&current, const uint16_t *&end)
    {
        while (combinedMask != 0) {
            const uint32_t firstIndex = CountTrailingZeroes(combinedMask) >> 1;
            const uint32_t firstLaneMask = static_cast<uint32_t>(0x3U) << (firstIndex * 2U);
            if ((quoteMask & firstLaneMask) != 0) {
                end = cur + firstIndex;
                return ParseStepResult::RETURN_TRUE;
            }
            if (UNLIKELY((backslashMask & firstLaneMask) != 0)) {
                isFastString = false;
                return ParseStepResult::RETURN_TRUE;
            }
            if (UNLIKELY((controlMask & firstLaneMask) != 0)) {
                current = cur + firstIndex;
                return ParseStepResult::RETURN_FALSE;
            }
            if (UNLIKELY((nonAsciiMask & firstLaneMask) != 0)) {
                isAscii = false;
                combinedMask &= ~firstLaneMask;
            }
        }
        return ParseStepResult::CONTINUE;
    }

    static bool ScanJsonStringRangeForUtf16Scalar(bool &isFastString, bool &isAscii,
                                                  const uint16_t *&current, const uint16_t *range,
                                                  const uint16_t *&end)
    {
        for (; current != range; ++current) {
            uint16_t c = *current;
            if (c == UTF16_QUOTE_CHAR) {
                end = current;
                return true;
            }
            if (UNLIKELY(c == UTF16_BACKSLASH_CHAR)) {
                isFastString = false;
                return true;
            }
            if (c <= UTF16_ASCII_END) {
                if (UNLIKELY(c < UTF16_SPACE_CHAR)) {
                    return false;
                }
            } else {
                isAscii = false;
            }
        }
        current = range;
        return false;
    }

    template <typename CheckBackslashFunc>
    static ParseStepResult HandleUtf8LengthChunk(size_t &length, bool &isAscii,
                                                 const uint8_t *&cur, const uint8_t *last,
                                                 const uint8_t *&current, const uint8_t *&end,
                                                 uint32_t quoteMask, uint32_t backslashMask,
                                                 uint32_t controlMask, uint32_t nonAsciiMask,
                                                 CheckBackslashFunc checkBackslash)
    {
        const uint32_t firstIndex = CountTrailingZeroes(quoteMask | backslashMask | controlMask | nonAsciiMask);
        const uint32_t firstBit = static_cast<uint32_t>(1U) << firstIndex;
        length += firstIndex;
        cur += firstIndex;
        if ((quoteMask & firstBit) != 0) {
            end = cur;
            return ParseStepResult::RETURN_TRUE;
        }
        if (UNLIKELY((backslashMask & firstBit) != 0)) {
            if (UNLIKELY(!checkBackslash(cur, last, isAscii))) {
                current = cur;
                return ParseStepResult::RETURN_FALSE;
            }
            ++length;
            ++cur;
            return ParseStepResult::CONTINUE;
        }
        if (UNLIKELY((controlMask & firstBit) != 0)) {
            current = cur;
            return ParseStepResult::RETURN_FALSE;
        }
        isAscii = false;
        ++length;
        ++cur;
        return ParseStepResult::CONTINUE;
    }

    template <typename CheckBackslashFunc>
    static bool ParseStringLengthScalarForUtf8(size_t &length, bool &isAscii, bool inObjOrArrOrMap,
                                               const uint8_t *&current, const uint8_t *last,
                                               const uint8_t *&end, CheckBackslashFunc checkBackslash)
    {
        for (; current < last; ++current) {
            uint8_t c = *current;
            if (inObjOrArrOrMap && c == QUOTE_CHAR) {
                end = current;
                return true;
            }
            if (UNLIKELY(c == BACKSLASH_CHAR)) {
                if (UNLIKELY(!checkBackslash(current, last, isAscii))) {
                    return false;
                }
            } else if (UNLIKELY(c < SPACE_CHAR)) {
                return false;
            } else if (c > ASCII_END) {
                isAscii = false;
            }
            ++length;
        }
        return !inObjOrArrOrMap;
    }

    template <typename CheckBackslashFunc>
    static ParseStepResult HandleUtf16LengthChunk(size_t &length, bool &isAscii,
                                                  const uint16_t *&cur, const uint16_t *last,
                                                  const uint16_t *&current, const uint16_t *&end,
                                                  uint32_t quoteMask, uint32_t backslashMask,
                                                  uint32_t controlMask, uint32_t nonAsciiMask,
                                                  CheckBackslashFunc checkBackslash)
    {
        const uint32_t firstIndex = CountTrailingZeroes(quoteMask | backslashMask | controlMask | nonAsciiMask) >> 1;
        const uint32_t firstLaneMask = static_cast<uint32_t>(0x3U) << (firstIndex * 2U);
        cur += firstIndex;
        length += firstIndex;
        if ((quoteMask & firstLaneMask) != 0) {
            end = cur;
            return ParseStepResult::RETURN_TRUE;
        }
        if (UNLIKELY((backslashMask & firstLaneMask) != 0)) {
            if (UNLIKELY(!checkBackslash(cur, last, isAscii))) {
                current = cur;
                return ParseStepResult::RETURN_FALSE;
            }
            ++length;
            ++cur;
            return ParseStepResult::CONTINUE;
        }
        if (UNLIKELY((controlMask & firstLaneMask) != 0)) {
            current = cur;
            return ParseStepResult::RETURN_FALSE;
        }
        isAscii = false;
        ++length;
        ++cur;
        return ParseStepResult::CONTINUE;
    }

    template <typename CheckBackslashFunc>
    static bool ParseStringLengthScalarForUtf16(size_t &length, bool &isAscii, bool inObjOrArrOrMap,
                                                const uint16_t *&current, const uint16_t *last,
                                                const uint16_t *&end, CheckBackslashFunc checkBackslash)
    {
        for (; current < last; ++current) {
            uint16_t c = *current;
            if (inObjOrArrOrMap && c == UTF16_QUOTE_CHAR) {
                end = current;
                return true;
            }
            if (UNLIKELY(c == UTF16_BACKSLASH_CHAR)) {
                if (UNLIKELY(!checkBackslash(current, last, isAscii))) {
                    return false;
                }
            } else if (UNLIKELY(c < UTF16_SPACE_CHAR)) {
                return false;
            } else if (c > UTF16_ASCII_END) {
                isAscii = false;
            }
            ++length;
        }
        return !inObjOrArrOrMap;
    }

    template <typename Char>
    static void CopyPlainUtf8CharsSameWidth(Char *&p, const uint8_t *src, size_t count)
    {
        std::copy(src, src + count, p);
        p += count;
    }

    template <typename Char>
    static void CopyPlainUtf8CharsExpand(Char *&p, const uint8_t *src, size_t count)
    {
        const __m128i zeroVector = _mm_setzero_si128();
        while (count >= CHUNK_SIZE) {
            const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
            const __m128i low = _mm_unpacklo_epi8(chunk, zeroVector);
            const __m128i high = _mm_unpackhi_epi8(chunk, zeroVector);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(p), low);
            _mm_storeu_si128(reinterpret_cast<__m128i *>(p + UTF16_CHUNK_SIZE), high);
            src += CHUNK_SIZE;
            p += CHUNK_SIZE;
            count -= CHUNK_SIZE;
        }
        while (count-- != 0) {
            *p++ = static_cast<Char>(*src++);
        }
    }

    template <typename Char>
    static void CopyPlainUtf16CharsSameWidth(Char *&p, const uint16_t *src, size_t count)
    {
        std::copy(src, src + count, p);
        p += count;
    }

    template <typename Char>
    static void CopyPlainUtf16CharsTruncate(Char *&p, const uint16_t *src, size_t count)
    {
        const __m128i zeroVector = _mm_setzero_si128();
        while (count >= UTF16_CHUNK_SIZE) {
            const __m128i chunk = _mm_loadu_si128(reinterpret_cast<const __m128i *>(src));
            const __m128i packed = _mm_packus_epi16(chunk, zeroVector);
            _mm_storel_epi64(reinterpret_cast<__m128i *>(p), packed);
            src += UTF16_CHUNK_SIZE;
            p += UTF16_CHUNK_SIZE;
            count -= UTF16_CHUNK_SIZE;
        }
        while (count-- != 0) {
            *p++ = static_cast<Char>(*src++);
        }
    }

    template <typename Char>
    static inline void CopyPlainUtf8Chars(Char *&p, const uint8_t *src, size_t count)
    {
        if constexpr (sizeof(Char) == sizeof(uint8_t)) {
            CopyPlainUtf8CharsSameWidth(p, src, count);
        } else {
            CopyPlainUtf8CharsExpand(p, src, count);
        }
    }

    template <typename Char>
    static inline void CopyPlainUtf16Chars(Char *&p, const uint16_t *src, size_t count)
    {
        if constexpr (sizeof(Char) == sizeof(uint16_t)) {
            CopyPlainUtf16CharsSameWidth(p, src, count);
        } else {
            CopyPlainUtf16CharsTruncate(p, src, count);
        }
    }
