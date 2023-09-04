/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#include "ecmascript/base/json_helper.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

namespace panda::ecmascript::base {
constexpr unsigned char CODE_SPACE = 0x20;
constexpr int FOUR_HEX = 4;
constexpr char ZERO_FIRST = static_cast<char>(0xc0); // \u0000 => c0 80

bool JsonHelper::IsFastValueToQuotedString(const char *value)
{
    if (strpbrk(value, "\"\\\b\f\n\r\t") != nullptr) {
        return false;
    }
    while (*value != '\0') {
        if ((*value > 0 && *value < CODE_SPACE) || *value == ZERO_FIRST) {
            return false;
        }
        value++;
    }
    return true;
}

CString JsonHelper::ValueToQuotedString(CString str)
{
    CString product;
    const char *value = str.c_str();
    // fast mode
    bool isFast = IsFastValueToQuotedString(value);
    if (isFast) {
        product += "\"";
        product += str;
        product += "\"";
        return product;
    }
    // 1. Let product be code unit 0x0022 (QUOTATION MARK).
    product += "\"";
    // 2. For each code unit C in value
    for (const char *c = value; *c != 0; ++c) {
        switch (*c) {
            /*
             * a. If C is 0x0022 (QUOTATION MARK) or 0x005C (REVERSE SOLIDUS), then
             * i. Let product be the concatenation of product and code unit 0x005C (REVERSE SOLIDUS).
             * ii. Let product be the concatenation of product and C.
             */
            case '\"':
                product += "\\\"";
                break;
            case '\\':
                product += "\\\\";
                break;
            /*
             * b. Else if C is 0x0008 (BACKSPACE), 0x000C (FORM FEED), 0x000A (LINE FEED), 0x000D (CARRIAGE RETURN),
             * or 0x000B (LINE TABULATION), then
             * i. Let product be the concatenation of product and code unit 0x005C (REVERSE SOLIDUS).
             * ii. Let abbrev be the String value corresponding to the value of C as follows:
             * BACKSPACE "b"
             * FORM FEED (FF) "f"
             * LINE FEED (LF) "n"
             * CARRIAGE RETURN (CR) "r"
             * LINE TABULATION "t"
             * iii. Let product be the concatenation of product and abbrev.
             */
            case '\b':
                product += "\\b";
                break;
            case '\f':
                product += "\\f";
                break;
            case '\n':
                product += "\\n";
                break;
            case '\r':
                product += "\\r";
                break;
            case '\t':
                product += "\\t";
                break;
            case ZERO_FIRST:
                product += "\\u0000";
                ++c;
                break;
            default:
                // c. Else if C has a code unit value less than 0x0020 (SPACE), then
                if (*c > 0 && *c < CODE_SPACE) {
                    /*
                     * i. Let product be the concatenation of product and code unit 0x005C (REVERSE SOLIDUS).
                     * ii. Let product be the concatenation of product and "u".
                     * iii. Let hex be the string result of converting the numeric code unit value of C to a String of
                     * four hexadecimal digits. Alphabetic hexadecimal digits are presented as lowercase Latin letters.
                     * iv. Let product be the concatenation of product and hex.
                     */
                    std::ostringstream oss;
                    oss << "\\u" << std::hex << std::setfill('0') << std::setw(FOUR_HEX) << static_cast<int>(*c);
                    product += oss.str();
                } else {
                    // Else,
                    // i. Let product be the concatenation of product and C.
                    product += *c;
                }
        }
    }
    // 3. Let product be the concatenation of product and code unit 0x0022 (QUOTATION MARK).
    product += "\"";
    // Return product.
    return product;
}
}  // namespace panda::ecmascript::base