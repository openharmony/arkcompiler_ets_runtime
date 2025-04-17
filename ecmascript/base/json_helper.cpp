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
constexpr uint8_t CODE_SPACE = 0x20;
constexpr uint8_t ZERO_FIRST = 0xc0; // \u0000 => c0 80
constexpr uint8_t ALONE_SURROGATE_3B_FIRST = 0xed;
constexpr uint8_t ALONE_SURROGATE_3B_SECOND_START = 0xa0;
constexpr uint8_t ALONE_SURROGATE_3B_SECOND_END = 0xbf;
constexpr uint8_t ALONE_SURROGATE_3B_THIRD_START = 0x80;
constexpr uint8_t ALONE_SURROGATE_3B_THIRD_END = 0xbf;

namespace {

constexpr int K_JSON_ESCAPE_TABLE_ENTRY_SIZE = 8;

// Table for escaping Latin1 characters.
// Table entries start at a multiple of 8 with the first byte indicating length.
constexpr const char* const JSON_ESCAPE_TABLE =
    "\6\\u0000 \6\\u0001 \6\\u0002 \6\\u0003 \6\\u0004 \6\\u0005 \6\\u0006 \6\\u0007 "
    "\2\\b     \2\\t     \2\\n     \6\\u000b \2\\f     \2\\r     \6\\u000e \6\\u000f "
    "\6\\u0010 \6\\u0011 \6\\u0012 \6\\u0013 \6\\u0014 \6\\u0015 \6\\u0016 \6\\u0017 "
    "\6\\u0018 \6\\u0019 \6\\u001a \6\\u001b \6\\u001c \6\\u001d \6\\u001e \6\\u001f "
    "\1       \1!      \2\\\"     \1#      \1$      \1%      \1&      \1'      "
    "\1(      \1)      \1*      \1+      \1,      \1-      \1.      \1/      "
    "\1""0      \1""1      \1""2      \1""3      \1""4      \1""5      \1""6      \1""7      "
    "\18      \19      \1:      \1;      \1<      \1=      \1>      \1?      "
    "\1@      \1A      \1B      \1C      \1D      \1E      \1F      \1G      "
    "\1H      \1I      \1J      \1K      \1L      \1M      \1N      \1O      "
    "\1P      \1Q      \1R      \1S      \1T      \1U      \1V      \1W      "
    "\1X      \1Y      \1Z      \1[      \2\\\\     \1]      \1^      \1_      ";

// excluding 0xc0, 0xed
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
    0, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 0, 1, 1,
    1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};

constexpr bool DoNotEscape(uint8_t c)
{
    return JSON_DO_NOT_ESCAPE_FLAG_TABLE[c];
}

inline void AppendCString(CString& output, const char* s)
{
    output.append(s + 1, *s);
}

}  // namespace

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

bool JsonHelper::IsFastValueToQuotedString(const Span<const uint8_t>& sp)
{
    for (const auto ch : sp) {
        if (!DoNotEscape(ch)) {
            return false;
        }
    }
    return true;
}

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

void JsonHelper::AppendValueToQuotedString(const Span<const uint8_t>& sp, CString& output)
{
    output += "\"";
    bool isFast = IsFastValueToQuotedString(sp); // fast mode
    if (isFast) {
        output.append(reinterpret_cast<const char*>(sp.data()), sp.size());
        output += "\"";
        return;
    }
    for (uint32_t i = 0; i < sp.size(); ++i) {
        const auto ch = sp[i];
        if (DoNotEscape(ch)) {
            output += ch;
        } else if (ch == ZERO_FIRST) {
            output += "\\u0000";
            ++i;
        } else if (ch == ALONE_SURROGATE_3B_FIRST) {
            if (i + 2 < sp.size() && // 2: Check 2 more characters
                sp[i + 1] >= ALONE_SURROGATE_3B_SECOND_START && // 1: 1st character after ch
                sp[i + 1] <= ALONE_SURROGATE_3B_SECOND_END && // 1: 1st character after ch
                sp[i + 2] >= ALONE_SURROGATE_3B_THIRD_START && // 2: 2nd character after ch
                sp[i + 2] <= ALONE_SURROGATE_3B_THIRD_END) {   // 2: 2nd character after ch
                auto unicodeRes = utf_helper::ConvertUtf8ToUnicodeChar(sp.data() + i, 3); // 3: Parse 3 characters
                ASSERT(unicodeRes.first != utf_helper::INVALID_UTF8);
                AppendUnicodeEscape(static_cast<uint32_t>(unicodeRes.first), output);
                i += 2; // 2 : Skip 2 characters
                } else {
                    output += ch;
                }
        } else {
            AppendCString(output, &JSON_ESCAPE_TABLE[ch * K_JSON_ESCAPE_TABLE_ENTRY_SIZE]);
        }
    }
    output += "\"";
}
} // namespace panda::ecmascript::base