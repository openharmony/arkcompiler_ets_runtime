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

#ifndef ECMASCRIPT_MEM_C_STRING_H
#define ECMASCRIPT_MEM_C_STRING_H

#include <sstream>
#include <string>
#include <string_view>

#include "ecmascript/common.h"
#include "ecmascript/mem/caddress_allocator.h"

namespace panda::ecmascript {
class EcmaString;
class JSTaggedValue;

using CString = std::basic_string<char, std::char_traits<char>, CAddressAllocator<char>>;
using C16String = std::basic_string<char16_t, std::char_traits<char16_t>, CAddressAllocator<char16_t>>;
using CStringStream = std::basic_stringstream<char, std::char_traits<char>, CAddressAllocator<char>>;

struct CStringHash {
    using argument_type = panda::ecmascript::CString;
    using result_type = std::size_t;

    size_t operator()(const CString &str) const noexcept
    {
        return std::hash<std::string_view>()(std::string_view(str.data(), str.size()));
    }
};

constexpr int DEC_BASE = 10;

// PRINT will skip '\0' in utf16 during conversion of utf8
enum StringConvertedUsage { PRINT, LOGICOPERATION };

long CStringToL(const CString &str);
int64_t CStringToLL(const CString &str);
uint64_t CStringToULL(const CString &str);
float CStringToF(const CString &str);
double CStringToD(const CString &str);

CString ConvertToString(const std::string &str);
std::string PUBLIC_API ConvertToStdString(const CString &str);

// '\u0000' is skip according to holdZero
// cesu8 means non-BMP1 codepoints should encode as 1 utf8 string
CString PUBLIC_API ConvertToString(const ecmascript::EcmaString *s,
    StringConvertedUsage usage = StringConvertedUsage::PRINT, bool cesu8 = false);

#if ENABLE_NEXT_OPTIMIZATION
template <typename DstType>
bool AppendSpecialDouble(DstType &str, double d);
template <typename DstType>
void AppendDoubleToString(DstType &str, double d);
template <typename DstType>
void AppendIntToCString(DstType &str, int number);
template <typename DstType>
void ConvertNumberToCStringAndAppend(DstType &str, JSTaggedValue num);
void ConvertQuotedAndAppendToCString(CString &str, const EcmaString *s);
void ConvertQuotedAndAppendToC16String(C16String &str, const EcmaString *s);
#endif

CString ConvertToString(ecmascript::JSTaggedValue key);

// append char to CString to C16String.
// char16_t to CString needs to be converted, which is time-consuming. So this way is not allowed.
template <typename DstType>
inline void AppendString(DstType &dst, const char *src, uint32_t len)
{
    if constexpr (sizeof(typename DstType::value_type) == 1) {
        dst.append(src, len);
    } else {
        for (uint32_t i = 0; i < len; ++i) {
            dst += static_cast<char16_t>(src[i]);
        }
    }
}

template <typename DstType>
inline void AppendString(DstType &dst, const char *src)
{
    if constexpr (sizeof(typename DstType::value_type) == 1) {
        dst.append(src);
    } else {
        while (*src != '\0') {
            dst += static_cast<char16_t>(*src);
            ++src;
        }
    }
}

template <typename DstType, typename SrcType>
inline void AppendChar(DstType &dst, const SrcType src)
{
    auto c = static_cast<typename DstType::value_type>(src);
    dst += c;
}

template <typename DstType, typename SrcType>
inline void AppendCString(DstType &dst, SrcType &src)
{
    static_assert(sizeof(typename DstType::value_type) >= sizeof(typename SrcType::value_type));
    if constexpr (sizeof(typename DstType::value_type) == 1) {
        dst += src;
    } else {
        if constexpr (sizeof(typename SrcType::value_type) == 1) {
            for (const auto &c : src) {
                dst += static_cast<char16_t>(c);
            }
        } else {
            dst += src;
        }
    }
}

template<class T>
std::enable_if_t<std::is_floating_point_v<T>, CString> FloatToCString(T number)
{
    CStringStream strStream;
    strStream << number;
    return strStream.str();
}

template<class T>
std::enable_if_t<std::is_integral_v<T>, CString> ToCString(T number)
{
    static constexpr uint32_t BUFF_SIZE = std::numeric_limits<T>::digits10 + 3;  // 3: Reserved for sign bit and '\0'.
    int64_t n = static_cast<int64_t>(number);
    char buf[BUFF_SIZE];
    uint32_t position = BUFF_SIZE - 1;
    buf[position] = '\0';
    bool isNeg = true;
    if (n >= 0) {
        n = -n;
        isNeg = false;
    }
    do {
        buf[--position] = static_cast<int8_t>('0' - (n % DEC_BASE));
        n /= DEC_BASE;
    } while (n);
    if (isNeg) {
        buf[--position] = '-';
    }
    return CString(&buf[position]);
}
}  // namespace panda::ecmascript

namespace std {
template <>
struct hash<panda::ecmascript::CString> {
    using argument_type = panda::ecmascript::CStringHash::argument_type;
    using result_type = panda::ecmascript::CStringHash::result_type;

    size_t operator()(const panda::ecmascript::CString &str) const noexcept
    {
        return panda::ecmascript::CStringHash()(str);
    }
};
}  // namespace std

#endif  // ECMASCRIPT_MEM_C_STRING_H
