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

static ParseStepResult ScanJsonStringRangeUtf16Tail(bool &isFastString, bool &isAscii,
                                                    const uint16_t *scan, const uint16_t *chunkEnd,
                                                    const uint16_t *&current, const uint16_t *&end)
{
    for (; scan != chunkEnd; ++scan) {
        uint16_t c = *scan;
        if (c == UTF16_QUOTE_CHAR) {
            end = scan;
            return ParseStepResult::RETURN_TRUE;
        }
        if (UNLIKELY(c == UTF16_BACKSLASH_CHAR)) {
            isFastString = false;
            return ParseStepResult::RETURN_TRUE;
        }
        if (c <= UTF16_ASCII_END) {
            if (UNLIKELY(c < UTF16_SPACE_CHAR)) {
                current = scan;
                return ParseStepResult::RETURN_FALSE;
            }
        } else {
            isAscii = false;
        }
    }
    return ParseStepResult::CONTINUE;
}

static ParseStepResult HandleUtf16StringRangeChunk(bool &isFastString, bool &isAscii,
                                                   const uint16_t *cur, uint16_t minIndex,
                                                   const uint16_t *&current, const uint16_t *&end)
{
    const uint16_t offset = (minIndex & UTF16_INDEX_MASK) >> UTF16_INDEX_SHIFT;
    if ((minIndex & UTF16_QUOTE_MASK) > 0) {
        end = cur + offset;
        return ParseStepResult::RETURN_TRUE;
    }
    if (UNLIKELY((minIndex & UTF16_BACKSLASH_MASK) > 0)) {
        isFastString = false;
        return ParseStepResult::RETURN_TRUE;
    }
    if (UNLIKELY((minIndex & UTF16_CONTROL_MASK) > 0)) {
        current = cur + offset;
        return ParseStepResult::RETURN_FALSE;
    }
    if ((minIndex & UTF16_NON_ASCII_MASK) == 0) {
        return ParseStepResult::CONTINUE;
    }
    isAscii = false;
    return ScanJsonStringRangeUtf16Tail(isFastString, isAscii, cur + offset + 1, cur + UTF16_CHUNK_SIZE,
                                        current, end);
}

template <typename CheckBackslashFunc>
static ParseStepResult HandleUtf8LengthChunk(size_t &length, bool &isAscii,
                                             const uint8_t *&cur, const uint8_t *last,
                                             const uint8_t *&current, const uint8_t *&end,
                                             uint8_t minIndex, CheckBackslashFunc checkBackslash)
{
    const uint8_t offset = (minIndex & INDEX_MASK) >> INDEX_SHIFT;
    cur += offset;
    length += offset;
    if ((minIndex & QUOTE_MASK) > 0) {
        end = cur;
        return ParseStepResult::RETURN_TRUE;
    }
    if (UNLIKELY((minIndex & BACKSLASH_MASK) > 0)) {
        if (UNLIKELY(!checkBackslash(cur, last, isAscii))) {
            current = cur;
            return ParseStepResult::RETURN_FALSE;
        }
        ++length;
        ++cur;
        return ParseStepResult::CONTINUE;
    }
    if (UNLIKELY((minIndex & CONTROL_MASK) > 0)) {
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
static ParseStepResult ProcessUtf8LengthChunk(size_t &length, bool &isAscii, bool inObjOrArrOrMap,
                                              const uint8_t *&cur, const uint8_t *last,
                                              const uint8_t *&current, const uint8_t *&end,
                                              CheckBackslashFunc checkBackslash)
{
    uint8x16_t chunk = vld1q_u8(cur);
    uint8x16_t quote_mask = inObjOrArrOrMap
        ? vandq_u8(QUOTE_INDEX_MASK_DATA, vceqq_u8(chunk, QUOTE_VECTOR))
        : ZERO_VECTOR;
    uint8x16_t backslash_mask = vandq_u8(BACKSLASH_INDEX_MASK_DATA, vceqq_u8(chunk, BACKSLASH_VECTOR));
    uint8x16_t control_mask = vandq_u8(CONTROL_INDEX_MASK_DATA, vcltq_u8(chunk, SPACE_VECTOR));
    uint8x16_t non_ascii_mask = isAscii
        ? vandq_u8(NON_ASCII_INDEX_MASK_DATA, vcgtq_u8(chunk, ASCII_END_VECTOR))
        : ZERO_VECTOR;
    uint8x16_t combined_mask =
        vorrq_u8(vorrq_u8(quote_mask, backslash_mask), vorrq_u8(control_mask, non_ascii_mask));
    uint8x16_t zero_mask = vceqq_u8(combined_mask, ZERO_VECTOR);
    uint8x16_t indices = vbslq_u8(zero_mask, COMPARE_VECTOR, combined_mask);
    uint8_t min_index = vminvq_u8(indices);
    if (min_index == COMPARE_MASK) {
        cur += CHUNK_SIZE;
        length += CHUNK_SIZE;
        return ParseStepResult::CONTINUE;
    }
    return HandleUtf8LengthChunk(length, isAscii, cur, last, current, end,
                                 min_index, checkBackslash);
}

template <typename CheckBackslashFunc>
static ParseStepResult HandleUtf16LengthChunk(size_t &length, bool &isAscii,
                                              const uint16_t *&cur, const uint16_t *last,
                                              const uint16_t *&current, const uint16_t *&end,
                                              uint16_t minIndex, CheckBackslashFunc checkBackslash)
{
    const uint16_t offset = (minIndex & UTF16_INDEX_MASK) >> UTF16_INDEX_SHIFT;
    cur += offset;
    length += offset;
    if ((minIndex & UTF16_QUOTE_MASK) > 0) {
        end = cur;
        return ParseStepResult::RETURN_TRUE;
    }
    if (UNLIKELY((minIndex & UTF16_BACKSLASH_MASK) > 0)) {
        if (UNLIKELY(!checkBackslash(cur, last, isAscii))) {
            current = cur;
            return ParseStepResult::RETURN_FALSE;
        }
        ++length;
        ++cur;
        return ParseStepResult::CONTINUE;
    }
    if (UNLIKELY((minIndex & UTF16_CONTROL_MASK) > 0)) {
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

template <typename CheckBackslashFunc>
static ParseStepResult ProcessUtf16LengthChunk(size_t &length, bool &isAscii, bool inObjOrArrOrMap,
                                               const uint16_t *&cur, const uint16_t *last,
                                               const uint16_t *&current, const uint16_t *&end,
                                               CheckBackslashFunc checkBackslash)
{
    uint16x8_t chunk = vld1q_u16(cur);
    uint16x8_t quote_mask = inObjOrArrOrMap
        ? vandq_u16(UTF16_QUOTE_INDEX_MASK_DATA, vceqq_u16(chunk, UTF16_QUOTE_VECTOR))
        : UTF16_ZERO_VECTOR;
    uint16x8_t backslash_mask =
        vandq_u16(UTF16_BACKSLASH_INDEX_MASK_DATA, vceqq_u16(chunk, UTF16_BACKSLASH_VECTOR));
    uint16x8_t control_mask =
        vandq_u16(UTF16_CONTROL_INDEX_MASK_DATA, vcltq_u16(chunk, UTF16_SPACE_VECTOR));
    uint16x8_t non_ascii_mask = isAscii
        ? vandq_u16(UTF16_NON_ASCII_INDEX_MASK_DATA, vcgtq_u16(chunk, UTF16_ASCII_END_VECTOR))
        : UTF16_ZERO_VECTOR;
    uint16x8_t combined_mask =
        vorrq_u16(vorrq_u16(quote_mask, backslash_mask), vorrq_u16(control_mask, non_ascii_mask));
    uint16x8_t zero_mask = vceqq_u16(combined_mask, UTF16_ZERO_VECTOR);
    uint16x8_t indices = vbslq_u16(zero_mask, UTF16_COMPARE_VECTOR, combined_mask);
    uint16_t min_index = vminvq_u16(indices);
    if (min_index == UTF16_COMPARE_MASK) {
        cur += UTF16_CHUNK_SIZE;
        length += UTF16_CHUNK_SIZE;
        return ParseStepResult::CONTINUE;
    }
    return HandleUtf16LengthChunk(length, isAscii, cur, last, current, end,
                                  min_index, checkBackslash);
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
    while (count >= CHUNK_SIZE) {
        uint8x16_t chunk = vld1q_u8(src);
        uint16x8_t low = vmovl_u8(vget_low_u8(chunk));
        uint16x8_t high = vmovl_u8(vget_high_u8(chunk));
        vst1q_u16(reinterpret_cast<uint16_t *>(p), low);
        vst1q_u16(reinterpret_cast<uint16_t *>(p + UTF16_CHUNK_SIZE), high);
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
    while (count >= UTF16_CHUNK_SIZE) {
        uint16x8_t chunk = vld1q_u16(src);
        uint8x8_t packed = vmovn_u16(chunk);
        vst1_u8(reinterpret_cast<uint8_t *>(p), packed);
        src += UTF16_CHUNK_SIZE;
        p += UTF16_CHUNK_SIZE;
        count -= UTF16_CHUNK_SIZE;
    }
    while (count-- != 0) {
        *p++ = static_cast<Char>(*src++);
    }
}
