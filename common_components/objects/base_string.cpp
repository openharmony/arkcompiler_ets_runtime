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

#include <codecvt>
#include <locale>

#include "common_components/base/utf_helper.h"
#include "common_interfaces/objects/base_string.h"

#include "common_components/platform/string_hash.h"
#include "common_components/platform/string_hash_helper.h"

namespace common {
    constexpr size_t LOW_3BITS = 0x7;
    constexpr size_t LOW_4BITS = 0xF;
    constexpr size_t LOW_5BITS = 0x1F;
    constexpr size_t LOW_6BITS = 0x3F;
    constexpr size_t L_SURROGATE_START = 0xDC00;
    constexpr size_t H_SURROGATE_START = 0xD800;
    constexpr size_t SURROGATE_RAIR_START = 0x10000;
    constexpr size_t OFFSET_18POS = 18;
    constexpr size_t OFFSET_12POS = 12;
    constexpr size_t OFFSET_10POS = 10;
    constexpr size_t OFFSET_6POS = 6;

    size_t UtfUtils::DebuggerConvertRegionUtf16ToUtf8(const uint16_t* utf16In, uint8_t* utf8Out, size_t utf16Len,
                                                      size_t utf8Len, size_t start, bool modify, bool isWriteBuffer)
    {
        return common::utf_helper::DebuggerConvertRegionUtf16ToUtf8(utf16In, utf8Out, utf16Len, utf8Len,
                                                                    start, modify, isWriteBuffer);
    }

    size_t UtfUtils::Utf8ToUtf16Size(const uint8_t* utf8, size_t utf8Len)
    {
        return common::utf_helper::Utf8ToUtf16Size(utf8, utf8Len);
    }

    size_t UtfUtils::Utf16ToUtf8Size(const uint16_t* utf16, uint32_t length, bool modify, bool isGetBufferSize,
                                     bool cesu8)
    {
        return common::utf_helper::Utf16ToUtf8Size(utf16, length, modify, isGetBufferSize, cesu8);
    }

    size_t UtfUtils::ConvertRegionUtf8ToUtf16(const uint8_t* utf8In, uint16_t* utf16Out, size_t utf8Len,
                                              size_t utf16Len)
    {
        return common::utf_helper::ConvertRegionUtf8ToUtf16(utf8In, utf16Out, utf8Len, utf16Len);
    }

    size_t UtfUtils::ConvertRegionUtf16ToLatin1(const uint16_t* utf16In, uint8_t* latin1Out, size_t utf16Len,
                                                size_t latin1Len)
    {
        return common::utf_helper::ConvertRegionUtf16ToLatin1(utf16In, latin1Out, utf16Len, latin1Len);
    }

    size_t UtfUtils::ConvertRegionUtf16ToUtf8(const uint16_t* utf16In, uint8_t* utf8Out, size_t utf16Len,
                                              size_t utf8Len, size_t start, bool modify, bool isWriteBuffer, bool cesu)
    {
        return common::utf_helper::ConvertRegionUtf16ToUtf8(
            utf16In, utf8Out, utf16Len, utf8Len, start, modify, isWriteBuffer, cesu);
    }


    // To change the hash algorithm of BaseString, please modify BaseString::CalculateConcatHashCode
    // and BaseStringHashHelper::ComputeHashForDataPlatform simultaneously!!
    template<typename T>
    uint32_t ComputeHashForDataInternal(const T *data, size_t size, uint32_t hashSeed)
    {
        if (size <= static_cast<size_t>(StringHash::MIN_SIZE_FOR_UNROLLING)) {
            uint32_t hash = hashSeed;
            for (uint32_t i = 0; i < size; i++) {
                hash = (hash << static_cast<uint32_t>(StringHash::HASH_SHIFT)) - hash + data[i];
            }
            return hash;
        }
        return StringHashHelper::ComputeHashForDataPlatform(data, size, hashSeed);
    }

    PUBLIC_API uint32_t BaseString::ComputeHashForData(const uint8_t *data, size_t size, uint32_t hashSeed)
    {
        return ComputeHashForDataInternal(data, size, hashSeed);
    }

    PUBLIC_API uint32_t BaseString::ComputeHashForData(const uint16_t *data, size_t size, uint32_t hashSeed)
    {
        return ComputeHashForDataInternal(data, size, hashSeed);
    }

    uint32_t BaseString::ComputeHashcodeUtf8(const uint8_t *utf8Data, size_t utf8Len, bool canBeCompress)
    {
        if (canBeCompress) {
            uint32_t mixHash = 0;
            // String using UTF8 encoding, and length smaller than 10, try to compute integer hash.
            if (utf8Len < MAX_ELEMENT_INDEX_LEN && HashIntegerString(utf8Data, utf8Len, &mixHash, 0)) {
                return mixHash;
            }
            uint32_t hash = ComputeHashForData(utf8Data, utf8Len, 0);
            return MixHashcode(hash, NOT_INTEGER);
        }
        auto utf16Len = UtfUtils::Utf8ToUtf16Size(utf8Data, utf8Len);
        std::vector<uint16_t> tmpBuffer(utf16Len);
        [[maybe_unused]] auto len = UtfUtils::ConvertRegionUtf8ToUtf16(utf8Data, tmpBuffer.data(), utf8Len,
                                                                       utf16Len);
        DCHECK_CC(len == utf16Len);
        uint32_t hash = ComputeHashForData(tmpBuffer.data(), utf16Len, 0);
        return MixHashcode(hash, NOT_INTEGER);
    }

    /* static */
    uint32_t BaseString::ComputeHashcodeUtf16(const uint16_t *utf16Data, uint32_t length)
    {
        uint32_t mixHash = 0;
        // String length smaller than 10, try to compute integer hash.
        if (length < MAX_ELEMENT_INDEX_LEN && HashIntegerString(utf16Data, length, &mixHash, 0)) {
            return mixHash;
        }
        uint32_t hash = ComputeHashForData(utf16Data, length, 0);
        return MixHashcode(hash, NOT_INTEGER);
    }


    // drop the tail bytes if the remain length can't fill the length it represents.
    static size_t FixUtf8Len(const uint8_t* utf8, size_t utf8Len)
    {
        constexpr size_t TWO_BYTES_LENGTH = 2;
        constexpr size_t THREE_BYTES_LENGTH = 3;
        size_t trimSize = 0;
        if (utf8Len >= 1 && utf8[utf8Len - 1] >= 0xC0) {
            // The last one char claim there are more than 1 byte next to it, it's invalid, so drop the last one.
            trimSize = 1;
        }
        if (utf8Len >= TWO_BYTES_LENGTH && utf8[utf8Len - TWO_BYTES_LENGTH] >= 0xE0) {
            // The second to last char claim there are more than 2 bytes next to it, it's invalid, so drop the last two.
            trimSize = TWO_BYTES_LENGTH;
        }
        if (utf8Len >= THREE_BYTES_LENGTH && utf8[utf8Len - THREE_BYTES_LENGTH] >= 0xF0) {
            // The third to last char claim there are more than 3 bytes next to it, it's invalid, so drop the last
            // three.
            trimSize = THREE_BYTES_LENGTH;
        }
        return utf8Len - trimSize;
    }

    /* static */
    bool BaseString::IsUtf8EqualsUtf16(const uint8_t* utf8Data, size_t utf8Len,
                                       const uint16_t* utf16Data, uint32_t utf16Len)
    {
        size_t safeUtf8Len = FixUtf8Len(utf8Data, utf8Len);
        const uint8_t* utf8End = utf8Data + utf8Len;
        const uint8_t* utf8SafeEnd = utf8Data + safeUtf8Len;
        const uint16_t* utf16End = utf16Data + utf16Len;
        while (utf8Data < utf8SafeEnd && utf16Data < utf16End) {
            uint8_t src = *utf8Data;
            switch (src & 0xF0) {
                case 0xF0:
                    {
                        const uint8_t c2 = *(++utf8Data);
                        const uint8_t c3 = *(++utf8Data);
                        const uint8_t c4 = *(++utf8Data);
                        uint32_t codePoint = ((src & LOW_3BITS) << OFFSET_18POS) | ((c2 & LOW_6BITS) << OFFSET_12POS) |
                            ((c3 & LOW_6BITS) << OFFSET_6POS) | (c4 & LOW_6BITS);
                        if (codePoint >= SURROGATE_RAIR_START) {
                            if (utf16Data >= utf16End - 1) {
                                return false;
                            }
                            codePoint -= SURROGATE_RAIR_START;
                            if (*utf16Data++ != static_cast<uint16_t>((codePoint >> OFFSET_10POS) |
                                H_SURROGATE_START)) {
                                return false;
                            } else if (*utf16Data++ != static_cast<uint16_t>((codePoint & 0x3FF) | L_SURROGATE_START)) {
                                return false;
                            }
                        } else {
                            if (*utf16Data++ != static_cast<uint16_t>(codePoint)) {
                                return false;
                            }
                        }
                        utf8Data++;
                        break;
                    }
                case 0xE0:
                    {
                        const uint8_t c2 = *(++utf8Data);
                        const uint8_t c3 = *(++utf8Data);
                        if (*utf16Data++ != static_cast<uint16_t>(((src & LOW_4BITS) << OFFSET_12POS) |
                            ((c2 & LOW_6BITS) << OFFSET_6POS) | (c3 & LOW_6BITS))) {
                            return false;
                        }
                        utf8Data++;
                        break;
                    }
                case 0xD0:
                case 0xC0:
                    {
                        const uint8_t c2 = *(++utf8Data);
                        if (*utf16Data++ != static_cast<uint16_t>(((src & LOW_5BITS) << OFFSET_6POS) | (c2 &
                            LOW_6BITS))) {
                            return false;
                        }
                        utf8Data++;
                        break;
                    }
                default:
                    do {
                        if (*utf16Data++ != static_cast<uint16_t>(*utf8Data++)) {
                            return false;
                        }
                    }
                    while (utf8Data < utf8SafeEnd && utf16Data < utf16End && *utf8Data < 0x80);
                    break;
            }
        }
        // The remain chars should be treated as single byte char.
        while (utf8Data < utf8End && utf16Data < utf16End) {
            if (*utf16Data++ != static_cast<uint16_t>(*utf8Data++)) {
                return false;
            }
        }
        return utf8Data == utf8End && utf16Data == utf16End;
    }

    // static
    template <typename T1, typename T2>
    uint32_t BaseString::CalculateDataConcatHashCode(const T1* dataFirst, size_t sizeFirst,
                                                     const T2* dataSecond, size_t sizeSecond)
    {
        uint32_t totalHash = ComputeHashForData(dataFirst, sizeFirst, 0);
        totalHash = ComputeHashForData(dataSecond, sizeSecond, totalHash);
        return MixHashcode(totalHash, NOT_INTEGER);
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


    bool BaseString::CanBeCompressed(const BaseString* string)
    {
        DCHECK_CC(string->IsLineString());
        if (string->IsUtf8()) {
            return CanBeCompressed(string->GetDataUtf8(), string->GetLength());
        }
        return CanBeCompressed(string->GetDataUtf16(), string->GetLength());
    }

    // static
    bool BaseString::CanBeCompressed(const uint8_t* utf8Data, uint32_t utf8Len)
    {
        uint32_t index = 0;
        for (; index + 4 <= utf8Len; index += 4) {
            // 4: process the data in chunks of 4 elements to improve speed
            // Check if all four characters in the current block are ASCII characters
            if (!IsASCIICharacter(utf8Data[index]) ||
                !IsASCIICharacter(utf8Data[index + 1]) || // 1: the second element of the block
                !IsASCIICharacter(utf8Data[index + 2]) || // 2: the third element of the block
                !IsASCIICharacter(utf8Data[index + 3])) {
                // 3: the fourth element of the block
                return false;
            }
        }
        // Check remaining characters if they are ASCII
        for (; index < utf8Len; ++index) {
            if (!IsASCIICharacter(utf8Data[index])) {
                return false;
            }
        }
        return true;
    }

    /* static */
    bool BaseString::CanBeCompressed(const uint16_t* utf16Data, uint32_t utf16Len)
    {
        uint32_t index = 0;
        for (; index + 4 <= utf16Len; index += 4) {
            // 4: process the data in chunks of 4 elements to improve speed
            // Check if all four characters in the current block are ASCII characters
            if (!IsASCIICharacter(utf16Data[index]) ||
                !IsASCIICharacter(utf16Data[index + 1]) || // 1: the second element of the block
                !IsASCIICharacter(utf16Data[index + 2]) || // 2: the third element of the block
                !IsASCIICharacter(utf16Data[index + 3])) {
                // 3: the fourth element of the block
                return false;
            }
        }
        // Check remaining characters if they are ASCII
        for (; index < utf16Len; ++index) {
            if (!IsASCIICharacter(utf16Data[index])) {
                return false;
            }
        }
        return true;
    }


    bool BaseString::IsASCIICharacter(uint16_t data)
    {
        if (data == 0) {
            return false;
        }
        // \0 is not considered ASCII in Ecma-Modified-UTF8 [only modify '\u0000']
        return data <= UtfUtils::UTF8_1B_MAX;
    }


    /* static */
    template <typename T1, typename T2>
    int32_t BaseString::IndexOf(Span<const T1>& lhsSp, Span<const T2>& rhsSp, int32_t pos, int32_t max)
    {
        DCHECK_CC(rhsSp.size() > 0);
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

    template
    int32_t BaseString::IndexOf<uint8_t, uint8_t>(Span<const uint8_t>& lhsSp, Span<const uint8_t>& rhsSp, int32_t pos,
                                                  int32_t max);
    template
    int32_t BaseString::IndexOf<uint16_t, uint16_t>(Span<const uint16_t>& lhsSp, Span<const uint16_t>& rhsSp,
                                                    int32_t pos, int32_t max);

    template
    int32_t BaseString::IndexOf<uint8_t, uint16_t>(Span<const uint8_t>& lhsSp, Span<const uint16_t>& rhsSp, int32_t pos,
                                                   int32_t max);

    template
    int32_t BaseString::IndexOf<uint16_t, uint8_t>(Span<const uint16_t>& lhsSp, Span<const uint8_t>& rhsSp, int32_t pos,
                                                   int32_t max);


    template <typename T1, typename T2>
    int32_t BaseString::LastIndexOf(Span<const T1>& lhsSp, Span<const T2>& rhsSp, int32_t pos)
    {
        int rhsSize = static_cast<int>(rhsSp.size());
        DCHECK_CC(rhsSize > 0);
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

    template
    int32_t BaseString::LastIndexOf<uint8_t, uint8_t>(Span<const uint8_t>& lhsSp, Span<const uint8_t>& rhsSp,
                                                      int32_t pos);
    template
    int32_t BaseString::LastIndexOf<uint16_t, uint16_t>(Span<const uint16_t>& lhsSp, Span<const uint16_t>& rhsSp,
                                                        int32_t pos);
    template
    int32_t BaseString::LastIndexOf<uint8_t, uint16_t>(Span<const uint8_t>& lhsSp, Span<const uint16_t>& rhsSp,
                                                       int32_t pos);
    template
    int32_t BaseString::LastIndexOf<uint16_t, uint8_t>(Span<const uint16_t>& lhsSp, Span<const uint8_t>& rhsSp,
                                                       int32_t pos);


    template <typename T1, typename T2>
    int32_t CompareStringSpan(Span<T1>& lhsSp, Span<T2>& rhsSp, int32_t count)
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

    template
    int32_t CompareStringSpan<const uint8_t, const uint8_t>(Span<const uint8_t>& lhsSp, Span<const uint8_t>& rhsSp,
                                                            int32_t count);
    template
    int32_t CompareStringSpan<const uint16_t, const uint16_t>(Span<const uint16_t>& lhsSp, Span<const uint16_t>& rhsSp,
                                                              int32_t count);
    template
    int32_t CompareStringSpan<const uint8_t, const uint16_t>(Span<const uint8_t>& lhsSp, Span<const uint16_t>& rhsSp,
                                                             int32_t count);
    template
    int32_t CompareStringSpan<const uint16_t, const uint8_t>(Span<const uint16_t>& lhsSp, Span<const uint8_t>& rhsSp,
                                                             int32_t count);


    template <typename T1, typename T2>
    bool IsSubStringAtSpan(Span<T1>& lhsSp, Span<T2>& rhsSp, uint32_t offset)
    {
        int rhsSize = static_cast<int>(rhsSp.size());
        DCHECK_CC(rhsSize + offset <= lhsSp.size());
        for (int i = 0; i < rhsSize; ++i) {
            auto left = static_cast<int32_t>(lhsSp[offset + static_cast<uint32_t>(i)]);
            auto right = static_cast<int32_t>(rhsSp[i]);
            if (left != right) {
                return false;
            }
        }
        return true;
    }

    template
    bool IsSubStringAtSpan<const uint8_t, const uint8_t>(Span<const uint8_t>& lhsSp, Span<const uint8_t>& rhsSp,
                                                         uint32_t offset);
    template
    bool IsSubStringAtSpan<const uint16_t, const uint16_t>(Span<const uint16_t>& lhsSp, Span<const uint16_t>& rhsSp,
                                                           uint32_t offset);
    template
    bool IsSubStringAtSpan<const uint8_t, const uint16_t>(Span<const uint8_t>& lhsSp, Span<const uint16_t>& rhsSp,
                                                          uint32_t offset);
    template
    bool IsSubStringAtSpan<const uint16_t, const uint8_t>(Span<const uint16_t>& lhsSp, Span<const uint8_t>& rhsSp,
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
}  // namespace common
