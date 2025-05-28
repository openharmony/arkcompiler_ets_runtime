/*
 * Copyright (c) 2023-2024 Huawei Device Co., Ltd.
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

#include "ecmascript/base/json_helper.h"
#include "ecmascript/base/utf_helper.h"


namespace panda::ecmascript::base {

#if ENABLE_NEXT_OPTIMIZATION
constexpr int K_JSON_ESCAPE_TABLE_ENTRY_SIZE = 8;

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
    return (c >= 0x20 && c <= 0x21) ||
           (c >= 0x23 && c != 0x5C && (c < 0xD800 || c > 0xDFFF));
}

bool JsonHelper::IsFastValueToQuotedString(const Span<const uint8_t> &sp)
{
    for (const auto utf8Ch : sp) {
        if (!DoNotEscape(utf8Ch)) {
            return false;
        }
    }
    return true;
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

#if ENABLE_NEXT_OPTIMIZATION
void JsonHelper::AppendQuotedValueToC16String(const Span<const uint16_t> &sp, uint32_t &index, C16String &output)
{
    auto ch = sp[index];
    if (utf_helper::IsUTF16Surrogate(ch)) {
        // utf-16 to quoted string
        if (ch <= utf_helper::DECODE_LEAD_HIGH) {
            if (index + 1 < sp.size() && utf_helper::IsUTF16LowSurrogate(sp[index + 1])) {
                AppendChar(output, ch);
                AppendChar(output, sp[index + 1]);
                ++index;
            } else {
                AppendUnicodeEscape(static_cast<uint32_t>(ch), output);
            }
        } else {
            AppendUnicodeEscape(static_cast<uint32_t>(ch), output);
        }
    } else {
        ASSERT(ch < 0x60);
        AppendString(output, &JSON_ESCAPE_TABLE[ch * K_JSON_ESCAPE_TABLE_ENTRY_SIZE]);
    }
}

template <typename SrcType, typename DstType>
void JsonHelper::AppendValueToQuotedString(const Span<const SrcType> &sp, DstType &output)
{
    static_assert(sizeof(typename DstType::value_type) >= sizeof(SrcType));
    AppendString(output, "\"");
    if constexpr (sizeof(SrcType) == 1) {
        if (IsFastValueToQuotedString(sp)) {
            AppendString(output, reinterpret_cast<const char *>(sp.data()), sp.size());
            AppendString(output, "\"");
            return;
        }
    }
    uint32_t len = sp.size();
    for (uint32_t i = 0; i < len; ++i) {
        auto ch = sp[i];
        if (DoNotEscape(ch)) {
            AppendChar(output, ch);
        } else if constexpr (sizeof(SrcType) != 1) {
            AppendQuotedValueToC16String(sp, i, output);
        } else {
            ASSERT(ch < 0x60);
            AppendString(output, &JSON_ESCAPE_TABLE[ch * K_JSON_ESCAPE_TABLE_ENTRY_SIZE]);
        }
    }
    AppendString(output, "\"");
}
template void JsonHelper::AppendValueToQuotedString<uint8_t, CString>(
    const Span<const uint8_t> &sp, CString &output);
template void JsonHelper::AppendValueToQuotedString<uint8_t, C16String>(
    const Span<const uint8_t> &sp, C16String &output);
template void JsonHelper::AppendValueToQuotedString<uint16_t, C16String>(
    const Span<const uint16_t> &sp, C16String &output);

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
                    auto unicodeRes = utf_helper::ConvertUtf8ToUnicodeChar(
                        reinterpret_cast<const uint8_t*>(str.c_str() + i), 3); // 3: Parse 3 characters
                    ASSERT(unicodeRes.first != utf_helper::INVALID_UTF8);
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