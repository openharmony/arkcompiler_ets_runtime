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

#ifndef ECMASCRIPT_BASE_JSON_HELPER_H
#define ECMASCRIPT_BASE_JSON_HELPER_H

#include "ecmascript/js_handle.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/property_attributes.h"
#include "objects/utils/span.h"

namespace panda::ecmascript::base {
constexpr int HEX_DIGIT_MASK = 0xF;
constexpr int HEX_SHIFT_THREE = 12;
constexpr int HEX_SHIFT_TWO = 8;
constexpr int HEX_SHIFT_ONE = 4;
constexpr uint8_t CODE_SPACE = 0x20;
#if !ENABLE_LATEST_OPTIMIZATION
constexpr uint8_t ZERO_FIRST = 0xc0; // \u0000 => c0 80
constexpr uint8_t ALONE_SURROGATE_3B_FIRST = 0xed;
constexpr uint8_t ALONE_SURROGATE_3B_SECOND_START = 0xa0;
constexpr uint8_t ALONE_SURROGATE_3B_SECOND_END = 0xbf;
constexpr uint8_t ALONE_SURROGATE_3B_THIRD_START = 0x80;
constexpr uint8_t ALONE_SURROGATE_3B_THIRD_END = 0xbf;
#endif

class JsonHelper {
public:
    enum class TransformType : uint32_t {
        SENDABLE = 1,
        NORMAL = 2,
        BIGINT = 3
    };

    enum class BigIntMode : uint8_t {
        DEFAULT,
        PARSE_AS_BIGINT,
        ALWAYS_PARSE_AS_BIGINT
    };

    enum class ParseReturnType : uint8_t {
        OBJECT,
        MAP
    };

    struct ParseOptions {
        BigIntMode bigIntMode = BigIntMode::DEFAULT;
        ParseReturnType returnType = ParseReturnType::OBJECT;

        ParseOptions() = default;
        ParseOptions(BigIntMode bigIntMode, ParseReturnType returnType)
        {
            this->bigIntMode = bigIntMode;
            this->returnType = returnType;
        }
    };

    static inline bool IsTypeSupportBigInt(TransformType type)
    {
        return type == TransformType::SENDABLE || type == TransformType::BIGINT;
    }

#if ENABLE_LATEST_OPTIMIZATION
    template <typename T>
    static void AppendUnicodeEscape(uint32_t ch, T& output)
    {
        static constexpr char HEX_DIGIT[] = "0123456789abcdef";
        using CharType = typename T::value_type;

        CharType char0 = HEX_DIGIT[(ch >> HEX_SHIFT_THREE) & HEX_DIGIT_MASK];
        CharType char1 = HEX_DIGIT[(ch >> HEX_SHIFT_TWO) & HEX_DIGIT_MASK];
        CharType char2 = HEX_DIGIT[(ch >> HEX_SHIFT_ONE) & HEX_DIGIT_MASK];
        CharType char3 = HEX_DIGIT[ch & HEX_DIGIT_MASK];

        auto* dst = output.GetBuffer() + output.GetLength();
        dst[0] = '\\';      // 0: escape character
        dst[1] = 'u';       // 1: escape character
        dst[2] = char0;     // 2: char0
        dst[3] = char1;     // 3: char1
        dst[4] = char2;     // 4: char2
        dst[5] = char3;     // 5: char3
        output.SetLength(output.GetLength() + 6);   // 6: total length
    }
#else
    template <typename T>
    static inline void AppendUnicodeEscape(uint32_t ch, T& output)
    {
        static constexpr char HEX_DIGIT[] = "0123456789abcdef";
        AppendString(output, "\\u");
        AppendChar(output, HEX_DIGIT[(ch >> HEX_SHIFT_THREE) & HEX_DIGIT_MASK]);
        AppendChar(output, HEX_DIGIT[(ch >> HEX_SHIFT_TWO) & HEX_DIGIT_MASK]);
        AppendChar(output, HEX_DIGIT[(ch >> HEX_SHIFT_ONE) & HEX_DIGIT_MASK]);
        AppendChar(output, HEX_DIGIT[ch & HEX_DIGIT_MASK]);
    }
#endif

    static inline bool IsJsonFormatCharacter(uint32_t ch)
    {
        // JSON structural characters that define the format
        if (ch == '{' || ch == '}' || ch == '[' || ch == ']' ||
            ch == ':' || ch == ',' || ch == '"') {
            return true;
        }
        // Whitespace characters that are part of JSON format
        if (ch == ' ' || ch == '\t' || ch == '\n' || ch == '\r') {
            return true;
        }
        return false;
    }

    static std::pair<std::string, std::uint32_t> AnonymizeJsonString(JSThread *thread, JSHandle<JSTaggedValue> str,
                                                                     uint32_t position, uint32_t width);

    static std::pair<std::string, std::uint32_t> AnonymizeJsonStringUtf16(const uint16_t *inData, size_t len,
                                                                          uint32_t position, uint32_t width);

#if ENABLE_LATEST_OPTIMIZATION
    static bool IsFastValueToQuotedString(const common::Span<const uint8_t> &sp);
#else
    static bool IsFastValueToQuotedString(const CString &str);
#endif

    // String values are wrapped in QUOTATION MARK (") code units. The code units " and \ are escaped with \ prefixes.
    // Control characters code units are replaced with escape sequences \uHHHH, or with the shorter forms,
    // \b (BACKSPACE), \f (FORM FEED), \n (LINE FEED), \r (CARRIAGE RETURN), \t (CHARACTER TABULATION).
#if ENABLE_LATEST_OPTIMIZATION
    template <typename SrcType, typename BuilderType>
    static bool AppendValueToQuotedString(const common::Span<const SrcType> &sp, BuilderType &output);
#else
    static void AppendValueToQuotedString(const CString& str, CString& output);
#endif

    static inline bool CompareKey(const std::pair<JSHandle<JSTaggedValue>, PropertyAttributes> &a,
                                  const std::pair<JSHandle<JSTaggedValue>, PropertyAttributes> &b)
    {
        return a.second.GetDictionaryOrder() < b.second.GetDictionaryOrder();
    }

    static inline bool CompareNumber(const JSHandle<JSTaggedValue> &a, const JSHandle<JSTaggedValue> &b)
    {
        return a->GetNumber() < b->GetNumber();
    }
};

}  // namespace panda::ecmascript::base

#endif  // ECMASCRIPT_BASE_UTF_JSON_H