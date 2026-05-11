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

#ifndef ECMASCRIPT_PLATFORM_JSON_HELPER_INTERNAL_ARM64_H
#define ECMASCRIPT_PLATFORM_JSON_HELPER_INTERNAL_ARM64_H
#include <arm_neon.h>
#include "ecmascript/base/config.h"

namespace panda::ecmascript::base {
class JsonHelperInternal {
friend class JsonPlatformHelper;
private:
    enum class ParseStepResult : uint8_t {
        CONTINUE,
        RETURN_TRUE,
        RETURN_FALSE,
    };

    static constexpr uint8x16_t QUOTE_INDEX_MASK_DATA = {
        0x01, 0x11, 0x21, 0x31, 0x41, 0x51, 0x61, 0x71, 0x81, 0x91, 0xA1, 0xB1, 0xC1, 0xD1, 0xE1, 0xF1
    };
    static constexpr uint8x16_t BACKSLASH_INDEX_MASK_DATA = {
        0x02, 0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72, 0x82, 0x92, 0xA2, 0xB2, 0xC2, 0xD2, 0xE2, 0xF2
    };
    static constexpr uint8x16_t CONTROL_INDEX_MASK_DATA = {
        0x04, 0x14, 0x24, 0x34, 0x44, 0x54, 0x64, 0x74, 0x84, 0x94, 0xA4, 0xB4, 0xC4, 0xD4, 0xE4, 0xF4
    };
    static constexpr uint8x16_t QUOTE_VECTOR = {
        '"', '"', '"', '"', '"', '"', '"', '"', '"', '"', '"', '"', '"', '"', '"', '"'
    };
    static constexpr uint8x16_t BACKSLASH_VECTOR = {
        '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\', '\\'
    };
    static constexpr uint8x16_t SPACE_VECTOR = {
        ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' ', ' '
    };
    static constexpr uint8x16_t ZERO_VECTOR = {
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };
    static constexpr uint8x16_t COMPARE_VECTOR = {
        0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF, 0xFF
    };
    static constexpr uint8x16_t NON_ASCII_INDEX_MASK_DATA = {
        0x08, 0x18, 0x28, 0x38, 0x48, 0x58, 0x68, 0x78,
        0x88, 0x98, 0xA8, 0xB8, 0xC8, 0xD8, 0xE8, 0xF8
    };

    static constexpr uint8x16_t ASCII_END_VECTOR = {
        0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F,
        0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F, 0x7F
    };

    static constexpr uint8_t QUOTE_MASK = 0x01;
    static constexpr uint8_t BACKSLASH_MASK = 0x02;
    static constexpr uint8_t CONTROL_MASK = 0x04;
    static constexpr uint8_t INDEX_MASK = 0xF0;
    static constexpr uint8_t INDEX_SHIFT = 4;
    static constexpr uint8_t QUOTE_CHAR = '"';
    static constexpr uint8_t BACKSLASH_CHAR = '\\';
    static constexpr uint8_t SPACE_CHAR = ' ';
    static constexpr uint8_t COMPARE_MASK = 0xFF;
    static constexpr size_t CHUNK_SIZE = 16;
    static constexpr uint8_t NON_ASCII_MASK = 0x08;
    static constexpr uint8_t ASCII_END = 0x7F;

    static constexpr uint16x8_t UTF16_QUOTE_INDEX_MASK_DATA = {
        0x0001, 0x0011, 0x0021, 0x0031, 0x0041, 0x0051, 0x0061, 0x0071
    };
    static constexpr uint16x8_t UTF16_BACKSLASH_INDEX_MASK_DATA = {
        0x0002, 0x0012, 0x0022, 0x0032, 0x0042, 0x0052, 0x0062, 0x0072
    };
    static constexpr uint16x8_t UTF16_CONTROL_INDEX_MASK_DATA = {
        0x0004, 0x0014, 0x0024, 0x0034, 0x0044, 0x0054, 0x0064, 0x0074
    };
    static constexpr uint16x8_t UTF16_NON_ASCII_INDEX_MASK_DATA = {
        0x0008, 0x0018, 0x0028, 0x0038, 0x0048, 0x0058, 0x0068, 0x0078
    };
    static constexpr uint16x8_t UTF16_QUOTE_VECTOR = {
        0x0022, 0x0022, 0x0022, 0x0022, 0x0022, 0x0022, 0x0022, 0x0022
    };
    static constexpr uint16x8_t UTF16_BACKSLASH_VECTOR = {
        0x005C, 0x005C, 0x005C, 0x005C, 0x005C, 0x005C, 0x005C, 0x005C
    };
    static constexpr uint16x8_t UTF16_SPACE_VECTOR = {
        0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020, 0x0020
    };
    static constexpr uint16x8_t UTF16_ASCII_END_VECTOR = {
        0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F, 0x007F
    };
    static constexpr uint16x8_t UTF16_ZERO_VECTOR = {
        0, 0, 0, 0, 0, 0, 0, 0
    };
    static constexpr uint16x8_t UTF16_COMPARE_VECTOR = {
        0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF, 0xFFFF
    };
    static constexpr uint16_t UTF16_QUOTE_MASK = 0x0001;
    static constexpr uint16_t UTF16_BACKSLASH_MASK = 0x0002;
    static constexpr uint16_t UTF16_CONTROL_MASK = 0x0004;
    static constexpr uint16_t UTF16_NON_ASCII_MASK = 0x0008;
    static constexpr uint16_t UTF16_INDEX_MASK = 0x0070;
    static constexpr uint16_t UTF16_INDEX_SHIFT = 4;
    static constexpr uint16_t UTF16_QUOTE_CHAR = 0x0022;
    static constexpr uint16_t UTF16_BACKSLASH_CHAR = 0x005C;
    static constexpr uint16_t UTF16_SPACE_CHAR = 0x0020;
    static constexpr uint16_t UTF16_ASCII_END = 0x007F;
    static constexpr uint16_t UTF16_COMPARE_MASK = 0xFFFF;
    static constexpr size_t UTF16_CHUNK_SIZE = 8;

    #include "ecmascript/platform/arm64/json_helper_internal_helpers.inl"

    /**
    * origin:          [   x,   x,'\\',   x, '"',   x,   x,'\t',   x,   x,'\\','\b',   x,   x, '"',   x]
    * first step:
    *      quote:      [   0,   0,   0,   0,0x41,   0,   0,   0,   0,   0,   0,   0,   0,   0,0xE1,   0]
    *      backslash:  [   0,   0,0x22,   0,   0,   0,   0,   0,   0,   0,0xA2,   0,   0,   0,   0,   0]
    *      control:    [   0,   0,   0,   0,   0,   0,   0,0x74,   0,   0,   0,0xB4,   0,   0,   0,   0]
    * second step:
    *      combine:    [   0,   0,0x22,   0,0x41,   0,   0,0x74,   0,   0,0xA2,0xB4,   0,   0,0xE1,   0]
    *      final:      [0xFF,0xFF,0x22,0xFF,0x41,0xFF,0xFF,0x74,0xFF,0xFF,0xA2,0xB4,0xFF,0xFF,0xE1,0xFF]
    * min_index: 0x22 =>  0x22 & 0x02 > 0  =>  backslash
    */
    static bool ReadJsonStringRangeForUtf8(bool &isFastString, const uint8_t *&current,
                                       const uint8_t *range, const uint8_t *&end)
    {
        const uint8_t* cur = current;
        for (; cur + CHUNK_SIZE <= range; cur += CHUNK_SIZE) {
            uint8x16_t chunk = vld1q_u8(cur);
            uint8x16_t quote_mask = vandq_u8(QUOTE_INDEX_MASK_DATA, vceqq_u8(chunk, QUOTE_VECTOR));
            uint8x16_t backslash_mask = vandq_u8(BACKSLASH_INDEX_MASK_DATA, vceqq_u8(chunk, BACKSLASH_VECTOR));
            uint8x16_t control_mask = vandq_u8(CONTROL_INDEX_MASK_DATA, vcltq_u8(chunk, SPACE_VECTOR));
            uint8x16_t combined_mask = vorrq_u8(quote_mask, vorrq_u8(backslash_mask, control_mask));
            uint8x16_t zero_mask = vceqq_u8(combined_mask, ZERO_VECTOR);
            uint8x16_t indices = vbslq_u8(zero_mask, COMPARE_VECTOR, combined_mask);
            uint8_t min_index = vminvq_u8(indices);
            if (min_index != COMPARE_MASK) { // 0xFF means no special characters
                if ((min_index & QUOTE_MASK) > 0) {
                    end = cur + ((min_index & INDEX_MASK) >> INDEX_SHIFT);
                    return true;
                }
                if (UNLIKELY((min_index & BACKSLASH_MASK) > 0)) {
                    isFastString = false;
                    return true;
                }
                if (UNLIKELY((min_index & CONTROL_MASK) > 0)) {
                    current = cur + ((min_index & INDEX_MASK) >> INDEX_SHIFT);
                    return false;
                }
            }
        }
        for (; cur != range; ++cur) {
            if (*cur == QUOTE_CHAR) {
                end = cur;
                return true;
            }
            if (UNLIKELY(*cur == BACKSLASH_CHAR)) {
                isFastString = false;
                return true;
            }
            if (UNLIKELY(*cur < SPACE_CHAR)) {
                current = cur;
                return false;
            }
        }
        current = range;
        return false;
    }

    static bool ReadJsonStringRangeForUtf16(bool &isFastString, bool &isAscii,
                                            const uint16_t *&current,
                                            const uint16_t *range,
                                            const uint16_t *&end)
    {
        const uint16_t *cur = current;
        for (; cur + UTF16_CHUNK_SIZE <= range; cur += UTF16_CHUNK_SIZE) {
            uint16x8_t chunk = vld1q_u16(cur);

            uint16x8_t quote_mask =
                vandq_u16(UTF16_QUOTE_INDEX_MASK_DATA, vceqq_u16(chunk, UTF16_QUOTE_VECTOR));
            uint16x8_t backslash_mask =
                vandq_u16(UTF16_BACKSLASH_INDEX_MASK_DATA, vceqq_u16(chunk, UTF16_BACKSLASH_VECTOR));
            uint16x8_t control_mask =
                vandq_u16(UTF16_CONTROL_INDEX_MASK_DATA, vcltq_u16(chunk, UTF16_SPACE_VECTOR));
            uint16x8_t non_ascii_mask =
                vandq_u16(UTF16_NON_ASCII_INDEX_MASK_DATA, vcgtq_u16(chunk, UTF16_ASCII_END_VECTOR));

            uint16x8_t combined_mask =
                vorrq_u16(vorrq_u16(quote_mask, backslash_mask),
                          vorrq_u16(control_mask, non_ascii_mask));
            uint16x8_t zero_mask = vceqq_u16(combined_mask, UTF16_ZERO_VECTOR);
            uint16x8_t indices = vbslq_u16(zero_mask, UTF16_COMPARE_VECTOR, combined_mask);
            uint16_t min_index = vminvq_u16(indices);
            if (min_index == UTF16_COMPARE_MASK) {
                continue;
            }

            ParseStepResult result = HandleUtf16StringRangeChunk(isFastString, isAscii, cur, min_index,
                                                                 current, end);
            if (result == ParseStepResult::RETURN_TRUE) {
                return true;
            }
            if (result == ParseStepResult::RETURN_FALSE) {
                return false;
            }
        }

        ParseStepResult result = ScanJsonStringRangeUtf16Tail(isFastString, isAscii, cur, range, current, end);
        if (result == ParseStepResult::RETURN_TRUE) {
            return true;
        }
        if (result == ParseStepResult::RETURN_FALSE) {
            return false;
        }

        current = range;
        return false;
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

        while (cur + CHUNK_SIZE <= last) {
            ParseStepResult result = ProcessUtf8LengthChunk(length, isAscii, inObjOrArrOrMap, cur, last,
                                                            current, end, checkBackslash);
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

        while (cur + UTF16_CHUNK_SIZE <= last) {
            ParseStepResult result = ProcessUtf16LengthChunk(length, isAscii, inObjOrArrOrMap, cur, last,
                                                             current, end, checkBackslash);
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

    template <typename Char>
    static void CopyPlainUtf8Chars(Char *&p, const uint8_t *src, size_t count)
    {
        if constexpr (sizeof(Char) == sizeof(uint8_t)) {
            CopyPlainUtf8CharsSameWidth(p, src, count);
        } else {
            CopyPlainUtf8CharsExpand(p, src, count);
        }
    }

    template <typename Char>
    static void CopyPlainUtf16Chars(Char *&p, const uint16_t *src, size_t count)
    {
        if constexpr (sizeof(Char) == sizeof(uint16_t)) {
            CopyPlainUtf16CharsSameWidth(p, src, count);
        } else {
            CopyPlainUtf16CharsTruncate(p, src, count);
        }
    }

    template <typename Char, typename ParseBackslashFunc>
    static void CopyCharWithBackslashForUtf8(Char *&p,
                                            const uint8_t *&current,
                                            const uint8_t *end,
                                            ParseBackslashFunc parseBackslash)
    {
        const uint8_t *cur = current;
        const uint8_t *last = end + 1;

        while (cur + CHUNK_SIZE <= last) {
            uint8x16_t chunk = vld1q_u8(cur);
            uint8x16_t backslash_mask =
                vandq_u8(BACKSLASH_INDEX_MASK_DATA, vceqq_u8(chunk, BACKSLASH_VECTOR));
            uint8x16_t zero_mask = vceqq_u8(backslash_mask, ZERO_VECTOR);
            uint8x16_t indices = vbslq_u8(zero_mask, COMPARE_VECTOR, backslash_mask);
            uint8_t min_index = vminvq_u8(indices);
            if (min_index == COMPARE_MASK) {
                CopyPlainUtf8Chars(p, cur, CHUNK_SIZE);
                cur += CHUNK_SIZE;
                continue;
            }

            uint8_t offset = (min_index & INDEX_MASK) >> INDEX_SHIFT;

            CopyPlainUtf8Chars(p, cur, offset);
            cur += offset;

            current = cur;
            parseBackslash(p);

            // Match scalar CopyCharWithBackslash:
            // ParseBackslash leaves current at the escaped char / final unicode digit,
            // then the outer loop calls Advance().
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

        while (cur + UTF16_CHUNK_SIZE <= last) {
            uint16x8_t chunk = vld1q_u16(cur);
            uint16x8_t backslash_mask =
                vandq_u16(UTF16_BACKSLASH_INDEX_MASK_DATA, vceqq_u16(chunk, UTF16_BACKSLASH_VECTOR));
            uint16x8_t zero_mask = vceqq_u16(backslash_mask, UTF16_ZERO_VECTOR);
            uint16x8_t indices = vbslq_u16(zero_mask, UTF16_COMPARE_VECTOR, backslash_mask);
            uint16_t min_index = vminvq_u16(indices);
            if (min_index == UTF16_COMPARE_MASK) {
                CopyPlainUtf16Chars(p, cur, UTF16_CHUNK_SIZE);
                cur += UTF16_CHUNK_SIZE;
                continue;
            }

            uint16_t offset = (min_index & UTF16_INDEX_MASK) >> UTF16_INDEX_SHIFT;

            CopyPlainUtf16Chars(p, cur, offset);
            cur += offset;

            current = cur;
            parseBackslash(p);

            // Match scalar CopyCharWithBackslash:
            // ParseBackslash leaves current at the escaped char / final unicode digit,
            // then the outer loop calls Advance().
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
}
#endif //ECMASCRIPT_PLATFORM_JSON_HELPER_INTERNAL_ARM64_H
