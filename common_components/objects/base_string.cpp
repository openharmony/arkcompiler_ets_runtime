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
#include "common_interfaces/objects/string/base_string.h"

#include "common_interfaces/objects/string/base_string-inl.h"
#include <codecvt>
#include <locale>


namespace panda {
uint32_t utf_utils::HandleAndDecodeInvalidUTF16(uint16_t const* utf16, size_t len, size_t* index)
{
    uint16_t first = utf16[*index];
    // A valid surrogate pair should always start with a High Surrogate
    if (IsUTF16LowSurrogate(first)) {
        return UTF16_REPLACEMENT_CHARACTER;
    }
    if (IsUTF16HighSurrogate(first) || (first & SURROGATE_MASK) == DECODE_LEAD_LOW) {
        if (*index == len - 1) {
            // A High surrogate not paired with another surrogate
            return UTF16_REPLACEMENT_CHARACTER;
        }
        uint16_t second = utf16[*index + 1];
        if (!IsUTF16LowSurrogate(second)) {
            // A High surrogate not followed by a low surrogate
            return UTF16_REPLACEMENT_CHARACTER;
        }
        // A valid surrogate pair, decode normally
        (*index)++;
        return ((first - DECODE_LEAD_LOW) << UTF16_OFFSET) + (second - DECODE_TRAIL_LOW) + DECODE_SECOND_FACTOR;
    }
    // A unicode not fallen into the range of representing by surrogate pair, return as it is
    return first;
}

size_t utf_utils::DebuggerConvertRegionUtf16ToUtf8(const uint16_t* utf16In, uint8_t* utf8Out, size_t utf16Len,
                                                   size_t utf8Len, size_t start, bool modify, bool isWriteBuffer)
{
    if (utf16In == nullptr || utf8Out == nullptr || utf8Len == 0) {
        return 0;
    }
    size_t utf8Pos = 0;
    size_t end = start + utf16Len;
    for (size_t i = start; i < end; ++i) {
        uint32_t codepoint = HandleAndDecodeInvalidUTF16(utf16In, end, &i);
        if (codepoint == 0) {
            if (isWriteBuffer) {
                utf8Out[utf8Pos++] = 0x00U;
                continue;
            }
            if (modify) {
                // special case for \u0000 ==> C080 - 1100'0000 1000'0000
                utf8Out[utf8Pos++] = UTF8_2B_FIRST;
                utf8Out[utf8Pos++] = UTF8_2B_SECOND;
            }
            continue;
        }
        size_t size = UTF8Length(codepoint);
        if (utf8Pos + size > utf8Len) {
            break;
        }
        utf8Pos += EncodeUTF8(codepoint, utf8Out, utf8Pos, size);
    }
    return utf8Pos;
}


size_t utf_utils::ConvertRegionUtf16ToLatin1(const uint16_t* utf16In, uint8_t* latin1Out, size_t utf16Len,
                                             size_t latin1Len)
{
    if (utf16In == nullptr || latin1Out == nullptr || latin1Len == 0) {
        return 0;
    }
    size_t latin1Pos = 0;
    size_t end = utf16Len;
    for (size_t i = 0; i < end; ++i) {
        if (latin1Pos == latin1Len) {
            break;
        }
        uint32_t codepoint = DecodeUTF16(utf16In, end, &i);
        uint8_t latin1Code = static_cast<uint8_t>(codepoint & LATIN1_LIMIT);
        latin1Out[latin1Pos++] = latin1Code;
    }
    return latin1Pos;
}

// static
template <typename T1, typename T2>
uint32_t BaseString::CalculateDataConcatHashCode(const T1* dataFirst, size_t sizeFirst,
                                                 const T2* dataSecond, size_t sizeSecond)
{
    uint32_t totalHash = ComputeHashForData(dataFirst, sizeFirst, 0);
    totalHash = ComputeHashForData(dataSecond, sizeSecond, totalHash);
    return totalHash;
}

template
uint32_t BaseString::CalculateDataConcatHashCode<uint8_t, uint8_t>(const uint8_t* dataFirst, size_t sizeFirst,
                                                                   const uint8_t* dataSecond, size_t sizeSecond);
template
uint32_t BaseString::CalculateDataConcatHashCode<uint16_t, uint16_t>(const uint16_t* dataFirst, size_t sizeFirst,
                                                                     const uint16_t* dataSecond, size_t sizeSecond);
template
uint32_t BaseString::CalculateDataConcatHashCode<uint8_t, uint16_t>(const uint8_t* dataFirst, size_t sizeFirst,
                                                                    const uint16_t* dataSecond, size_t sizeSecond);
template
uint32_t BaseString::CalculateDataConcatHashCode<uint16_t, uint8_t>(const uint16_t* dataFirst, size_t sizeFirst,
                                                                    const uint8_t* dataSecond, size_t sizeSecond);


template <typename T1, typename T2>
bool IsSubStringAtSpan(common::Span<T1>& lhsSp, common::Span<T2>& rhsSp, uint32_t offset)
{
    size_t rhsSize = rhsSp.size();
    ASSERT_COMMON(rhsSize + offset <= lhsSp.size());
    for (size_t i = 0; i < rhsSize; ++i) {
        auto left = static_cast<int32_t>(lhsSp[offset + static_cast<uint32_t>(i)]);
        auto right = static_cast<int32_t>(rhsSp[i]);
        if (left != right) {
            return false;
        }
    }
    return true;
}

template
bool IsSubStringAtSpan<const uint8_t, const uint8_t>(common::Span<const uint8_t>& lhsSp,
                                                     common::Span<const uint8_t>& rhsSp,
                                                     uint32_t offset);
template
bool IsSubStringAtSpan<const uint16_t, const uint16_t>(common::Span<const uint16_t>& lhsSp,
                                                       common::Span<const uint16_t>& rhsSp,
                                                       uint32_t offset);
template
bool IsSubStringAtSpan<const uint8_t, const uint16_t>(common::Span<const uint8_t>& lhsSp,
                                                      common::Span<const uint16_t>& rhsSp,
                                                      uint32_t offset);
template
bool IsSubStringAtSpan<const uint16_t, const uint8_t>(common::Span<const uint16_t>& lhsSp,
                                                      common::Span<const uint8_t>& rhsSp,
                                                      uint32_t offset);


std::u16string Utf16ToU16String(const uint16_t* utf16Data, uint32_t dataLen)
{
    auto* char16tData = reinterpret_cast<const char16_t*>(utf16Data);
    std::u16string u16str(char16tData, dataLen);
    return u16str;
}

std::u16string Utf8ToU16String(const uint8_t* utf8Data, uint32_t dataLen)
{
    auto* charData = reinterpret_cast<const char*>(utf8Data);
    std::string str(charData, dataLen);
    std::u16string u16str = std::wstring_convert<std::codecvt_utf8_utf16<char16_t>, char16_t>{}.from_bytes(str);
    return u16str;
}
} // namespace panda::ecmascript
