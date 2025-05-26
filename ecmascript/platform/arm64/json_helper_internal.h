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
    static bool ReadJsonStringRangeForUtf8(bool &isFastString, const uint8_t *current,
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
                return false;
            }
        }
        return false;
    }
};
}
#endif //ECMASCRIPT_PLATFORM_JSON_HELPER_INTERNAL_ARM64_H
