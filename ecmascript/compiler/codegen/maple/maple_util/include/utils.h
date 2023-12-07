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

#ifndef MAPLE_UTIL_INCLUDE_UTILS_H
#define MAPLE_UTIL_INCLUDE_UTILS_H
#include <type_traits>
#include <limits>
#include <vector>
#include "mpl_logging.h"

namespace maple {
namespace utils {
// Operations on char
constexpr bool IsDigit(char c)
{
    return (c >= '0' && c <= '9');
}

constexpr bool IsLower(char c)
{
    return (c >= 'a' && c <= 'z');
}

constexpr bool IsUpper(char c)
{
    return (c >= 'A' && c <= 'Z');
}

constexpr bool IsAlpha(char c)
{
    return (IsLower(c) || IsUpper(c));
}

constexpr bool IsAlnum(char c)
{
    return (IsAlpha(c) || IsDigit(c));
}

namespace __ToDigitImpl {
template <uint8_t Scale, typename T>
struct ToDigitImpl {
};

constexpr uint8_t kOctalNum = 8;
constexpr uint8_t kDecimalNum = 10;
constexpr uint8_t kHexadecimalNum = 16;

template <typename T>
struct ToDigitImpl<kDecimalNum, T> {
    static T DoIt(char c)
    {
        if (utils::IsDigit(c)) {
            return c - '0';
        }
        return std::numeric_limits<T>::max();
    }
};

template <typename T>
struct ToDigitImpl<kOctalNum, T> {
    static T DoIt(char c)
    {
        if (c >= '0' && c < '8') {
            return c - '0';
        }
        return std::numeric_limits<T>::max();
    }
};

template <typename T>
struct ToDigitImpl<kHexadecimalNum, T> {
    static T DoIt(char c)
    {
        if (utils::IsDigit(c)) {
            return c - '0';
        }
        if (c >= 'a' && c <= 'f') {
            return c - 'a' + kDecimalNum;
        }
        if (c >= 'A' && c <= 'F') {
            return c - 'A' + kDecimalNum;
        }
        return std::numeric_limits<T>::max();
    }
};
}  // namespace __ToDigitImpl

template <uint8_t Scale = __ToDigitImpl::kDecimalNum, typename T = uint8_t>
constexpr T ToDigit(char c)
{
    return __ToDigitImpl::ToDigitImpl<Scale, T>::DoIt(c);
}

// Operations on pointer
template <typename T>
inline T &ToRef(T *ptr)
{
    CHECK_NULL_FATAL(ptr);
    return *ptr;
}

template <typename T, typename = decltype(&T::get)>
inline typename T::element_type &ToRef(T &ptr)
{
    return ToRef(ptr.get());
}

template <size_t pos, typename = std::enable_if_t<pos<32>> struct bit_field {
    enum { value = 1U << pos };
};

template <size_t pos>
constexpr uint32_t bit_field_v = bit_field<pos>::value;

template <size_t pos, typename = std::enable_if_t<pos<64>> struct lbit_field {
    enum { value = 1ULL << pos };
};

template <size_t pos>
constexpr uint64_t lbit_field_v = bit_field<pos>::value;

template <typename T>
bool Contains(const std::vector<T> &container, const T &data)
{
    return (std::find(std::begin(container), std::end(container), data) != container.end());
}

}  // namespace utils
}  // namespace maple
#endif  // MAPLE_UTIL_INCLUDE_UTILS_H
