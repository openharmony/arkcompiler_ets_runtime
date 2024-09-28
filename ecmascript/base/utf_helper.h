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

#ifndef ECMASCRIPT_BASE_UTF_HELPER_H
#define ECMASCRIPT_BASE_UTF_HELPER_H

#include <cstdint>
#include <vector>

#include "libpandabase/utils/utf.h"
#include "ecmascript/common.h"

namespace panda::ecmascript::base::utf_helper {

static constexpr size_t CONST_2 = 2;
static constexpr size_t CONST_3 = 3;
static constexpr size_t CONST_4 = 4;
static constexpr size_t MASK1 = 0x80;
static constexpr size_t MASK2 = 0x20;
static constexpr size_t MASK3 = 0x10;
static constexpr size_t LOW_3BITS = 0x7;
static constexpr size_t LOW_4BITS = 0xF;
static constexpr size_t LOW_5BITS = 0x1F;
static constexpr size_t LOW_6BITS = 0x3F;
static constexpr size_t L_SURROGATE_START = 0xDC00;
static constexpr size_t H_SURROGATE_START = 0xD800;
static constexpr size_t SURROGATE_RAIR_START = 0x10000;
static constexpr size_t OFFSET_18POS = 18;
static constexpr size_t OFFSET_12POS = 12;
static constexpr size_t OFFSET_10POS = 10;
static constexpr size_t OFFSET_6POS = 6;
static constexpr uint16_t DECODE_LEAD_LOW = 0xD800;
static constexpr uint16_t DECODE_LEAD_HIGH = 0xDBFF;
static constexpr uint16_t DECODE_TRAIL_LOW = 0xDC00;
static constexpr uint16_t DECODE_TRAIL_HIGH = 0xDFFF;
static constexpr uint32_t DECODE_FIRST_FACTOR = 0x400;
static constexpr uint32_t DECODE_SECOND_FACTOR = 0x10000;
static constexpr uint32_t UTF8_OFFSET = 6;
static constexpr uint32_t UTF16_OFFSET = 10;
static constexpr uint16_t SURROGATE_MASK = 0xF800;
static constexpr uint16_t UTF16_REPLACEMENT_CHARACTER = 0xFFFD;

static constexpr uint8_t BIT_MASK_1 = 0x80;
static constexpr uint8_t BIT_MASK_2 = 0xC0;
static constexpr uint8_t BIT_MASK_3 = 0xE0;
static constexpr uint8_t BIT_MASK_4 = 0xF0;
static constexpr uint8_t BIT_MASK_5 = 0xF8;
static constexpr uint8_t BIT_MASK_FF = 0xFF;
static constexpr uint16_t BIT16_MASK = 0x3FF;

static constexpr uint8_t UTF8_1B_MAX = 0x7f;

static constexpr uint16_t UTF8_2B_MAX = 0x7ff;
static constexpr uint8_t UTF8_2B_FIRST = 0xc0;
static constexpr uint8_t UTF8_2B_SECOND = 0x80;
static constexpr uint8_t UTF8_2B_THIRD = 0x3f;
static constexpr uint8_t UTF8_2B_FIRST_MIN = 0xc2;  // the minimum for 2 bytes is 128, which is 0xc280

static constexpr uint16_t UTF8_3B_MAX = 0xffff;
static constexpr uint8_t UTF8_3B_FIRST = 0xe0;
static constexpr uint8_t UTF8_3B_SECOND = 0x80;
static constexpr uint8_t UTF8_3B_THIRD = 0x80;
static constexpr uint8_t UTF8_3B_SECOND_MIN = 0xa0;  // the minimum for 3 bytes is 2048, which is 0xe0a080
static constexpr uint8_t UTF8_3B_RESERVED_FIRST = 0xED;
static constexpr uint8_t UTF8_3B_RESERVED_SECOND_MIN = 0xA0;
static constexpr uint8_t UTF8_3B_RESERVED_SECOND_MAX = 0xBF; // U+D800~U+DFFF is reserved for UTF-16 surrogate pairs

static constexpr uint8_t UTF8_4B_FIRST = 0xf0;
static constexpr uint8_t UTF8_4B_SECOND_MIN = 0x90;  // the minimum for 4 bytes is 65536, which is 0xf0908080
static constexpr uint8_t UTF8_4B_FIRST_MAX = 0xF4; // the maximum for 4 bytes is 1114111, which is 0x10FFFF
static constexpr uint8_t UTF8_4B_SECOND_MAX = 0x8F;

static constexpr uint8_t byteMask = 0xbf;
static constexpr uint8_t byteMark = 0x80;

static constexpr uint8_t latin1Limit = 0xFF;

static constexpr int32_t INVALID_UTF8 = -1;

enum UtfLength : uint8_t { ONE = 1, TWO = 2, THREE = 3, FOUR = 4 };
enum UtfOffset : uint8_t { SIX = 6, TEN = 10, TWELVE = 12, EIGHTEEN = 18 };

static constexpr size_t MAX_BYTES = 4;
struct Utf8Char {
    size_t n;
    std::array<uint8_t, MAX_BYTES> ch;
};

static const unsigned char firstByteMark[7] = {0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC};

uint32_t DecodeUTF16(uint16_t const *utf16, size_t len, size_t *index, bool cesu8 = false);

size_t EncodeUTF8(uint32_t codepoint, uint8_t* utf8, size_t len, size_t index);

uint32_t UTF16Decode(uint16_t lead, uint16_t trail);

bool IsValidUTF8(const std::vector<uint8_t> &data);

Utf8Char ConvertUtf16ToUtf8(uint16_t d0, uint16_t d1, bool modify, bool isWriteBuffer = false);

size_t Utf16ToUtf8Size(const uint16_t *utf16, uint32_t length, bool modify = true,
                       bool isGetBufferSize = false, bool cesu8 = false);

size_t PUBLIC_API ConvertRegionUtf16ToUtf8(const uint16_t *utf16In, uint8_t *utf8Out, size_t utf16Len,
                                           size_t utf8Len, size_t start, bool modify = true,
                                           bool isWriteBuffer = false, bool cesu = false);

size_t DebuggerConvertRegionUtf16ToUtf8(const uint16_t *utf16In, uint8_t *utf8Out, size_t utf16Len, size_t utf8Len,
                                        size_t start, bool modify = true, bool isWriteBuffer = false);

uint32_t HandleAndDecodeInvalidUTF16(uint16_t const *utf16, size_t len, size_t *index);

std::pair<uint32_t, size_t> ConvertUtf8ToUtf16Pair(const uint8_t *data, bool combine = false);

size_t Utf8ToUtf16Size(const uint8_t *utf8, size_t utf8Len);

size_t ConvertRegionUtf8ToUtf16(const uint8_t *utf8In, uint16_t *utf16Out, size_t utf8Len, size_t utf16Len);

size_t ConvertRegionUtf16ToLatin1(const uint16_t *utf16In, uint8_t *latin1Out, size_t utf16Len, size_t latin1Len);

static inline uint32_t CombineTwoU16(uint16_t d0, uint16_t d1)
{
    uint32_t codePoint = d0 - utf::HI_SURROGATE_MIN;
    codePoint <<= UtfOffset::TEN;
    codePoint |= d1 - utf::LO_SURROGATE_MIN;
    codePoint += utf::LO_SUPPLEMENTS_MIN;
    return codePoint;
}

std::pair<int32_t, size_t> ConvertUtf8ToUnicodeChar(const uint8_t *utf8, size_t maxLen);

static inline bool IsHexDigits(uint16_t ch)
{
    return (ch >= '0' && ch <= '9') || (ch >= 'A' && ch <= 'F') || (ch >= 'a' && ch <= 'f');
}

}  // namespace panda::ecmascript::base::utf_helper

#endif  // ECMASCRIPT_BASE_UTF_HELPER_H