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

#ifndef ECMASCRIPT_BASE_JSON_HELPER_H
#define ECMASCRIPT_BASE_JSON_HELPER_H

#include "ecmascript/js_handle.h"
#include "ecmascript/mem/c_string.h"
#include "ecmascript/property_attributes.h"
#include "libpandabase/utils/span.h"

namespace panda::ecmascript::base {
constexpr int HEX_DIGIT_MASK = 0xF;
constexpr int HEX_SHIFT_THREE = 12;
constexpr int HEX_SHIFT_TWO = 8;
constexpr int HEX_SHIFT_ONE = 4;
constexpr uint8_t CODE_SPACE = 0x20;
#if !ENABLE_NEXT_OPTIMIZATION
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

#if ENABLE_NEXT_OPTIMIZATION
    static bool IsFastValueToQuotedString(const Span<const uint8_t> &sp);
#else
    static bool IsFastValueToQuotedString(const CString &str);
#endif

    // String values are wrapped in QUOTATION MARK (") code units. The code units " and \ are escaped with \ prefixes.
    // Control characters code units are replaced with escape sequences \uHHHH, or with the shorter forms,
    // \b (BACKSPACE), \f (FORM FEED), \n (LINE FEED), \r (CARRIAGE RETURN), \t (CHARACTER TABULATION).
#if ENABLE_NEXT_OPTIMIZATION
    static void AppendQuotedValueToC16String(const Span<const uint16_t> &sp, uint32_t &index, C16String &output);
    template <typename SrcType, typename DstType>
    static void AppendValueToQuotedString(const Span<const SrcType> &sp, DstType &output);
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