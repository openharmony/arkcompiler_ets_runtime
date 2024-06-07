/**
 * Copyright (c) 2021-2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BASE_MATH_HELPER_H
#define ECMASCRIPT_BASE_MATH_HELPER_H

#include <cstdint>
#include <cmath>

#include "ecmascript/base/bit_helper.h"

#define panda_bit_utils_ctz __builtin_ctz      // NOLINT(cppcoreguidelines-macro-usage)
#define panda_bit_utils_ctzll __builtin_ctzll  // NOLINT(cppcoreguidelines-macro-usage)

namespace panda::ecmascript::base {
class MathHelper {
public:
    static constexpr uint32_t GetIntLog2(const uint32_t X)
    {
        return static_cast<uint32_t>(panda_bit_utils_ctz(X));
    }

    static constexpr uint64_t GetIntLog2(const uint64_t X)
    {
        return static_cast<uint64_t>(panda_bit_utils_ctzll(X));
    }

    static double Asinh(double input)
    {
#if defined(PANDA_TARGET_WINDOWS)
        if (input == 0 && !std::signbit(input)) {
            // +0.0(double) is the special case for std::asinh() function compiled in linux for windows.
            return +0.0;
        }
#endif
        return std::asinh(input);
    }

    static inline double Atanh(double input)
    {
#if defined(PANDA_TARGET_WINDOWS)
        if (input == 0 && std::signbit(input)) {
            // -0.0(double) is the special case for std::atanh() function compiled in linux for windows.
            return -0.0;
        }
#endif
        return std::atanh(input);
    }
};

template <typename T, typename = typename std::enable_if<std::is_integral<T>::value>::type>
inline constexpr int WhichPowerOfTwo(T value)
{
    // Ensure the size of the integer is no more than 8 bytes (64 bits).
    static_assert(sizeof(T) <= 8);
    // Use __builtin_ctzll for 8 bytes (64 bits) and __builtin_ctz for 32-bit integers.
    return sizeof(T) == 8 ? __builtin_ctzll(static_cast<uint64_t>(value)) : __builtin_ctz(static_cast<uint32_t>(value));
}


inline int32_t SignedDiv32(int32_t lhs, int32_t rhs)
{
    if (rhs == 0) {
        return 0;
    }
    if (rhs == -1) {
        return lhs == std::numeric_limits<int32_t>::min() ? lhs : -lhs;
    }
    return lhs / rhs;
}

inline int64_t SignedDiv64(int64_t lhs, int64_t rhs)
{
    if (rhs == 0) {
        return 0;
    }
    if (rhs == -1) {
        return lhs == std::numeric_limits<int64_t>::min() ? lhs : -lhs;
    }
    return lhs / rhs;
}

inline int32_t SignedMod32(int32_t lhs, int32_t rhs)
{
    if (rhs == 0 || rhs == -1) {
        return 0;
    }
    return lhs % rhs;
}


inline bool SignedAddOverflow32(int32_t lhs, int32_t rhs, int32_t *val)
{
    uint32_t res = static_cast<uint32_t>(lhs) + static_cast<uint32_t>(rhs);
    *val = base::bit_cast<int32_t>(res);
    // Check for overflow by examining the sign bit.(bit 31 in a 32-bit integer)
    return ((res ^ static_cast<uint32_t>(lhs)) & (res ^ static_cast<uint32_t>(rhs)) & (1U << 31)) != 0;
}


inline bool SignedSubOverflow32(int32_t lhs, int32_t rhs, int32_t *val)
{
    uint32_t res = static_cast<uint32_t>(lhs) - static_cast<uint32_t>(rhs);
    *val = base::bit_cast<int32_t>(res);
    // Check for overflow by examining the sign bit.(bit 31 in a 32-bit integer)
    return ((res ^ static_cast<uint32_t>(lhs)) & (res ^ ~static_cast<uint32_t>(rhs)) & (1U << 31)) != 0;
}

inline bool SignedMulOverflow32(int32_t lhs, int32_t rhs, int32_t *val)
{
    int64_t result = int64_t{lhs} * int64_t{rhs};
    *val = static_cast<int32_t>(result);
    using limits = std::numeric_limits<int32_t>;
    return result < limits::min() || result > limits::max();
}


// Returns the quotient x/y, avoiding C++ undefined behavior if y == 0.
template <typename T>
inline T Divide(T x, T y)
{
    if (y != 0) {
        return x / y;
    }
    if (x == 0 || x != x) {
        return std::numeric_limits<T>::quiet_NaN();
    }
    if ((x >= 0) == (std::signbit(y) == 0)) {
        return std::numeric_limits<T>::infinity();
    }
    return -std::numeric_limits<T>::infinity();
}


template <typename SignedType>
inline SignedType AddWithWraparound(SignedType a, SignedType b)
{
    static_assert(std::is_integral<SignedType>::value && std::is_signed<SignedType>::value,
        "use this for signed integer types");
    using UnsignedType = typename std::make_unsigned<SignedType>::type;
    UnsignedType aUnsigned = static_cast<UnsignedType>(a);
    UnsignedType bUnsigned = static_cast<UnsignedType>(b);
    UnsignedType result = aUnsigned + bUnsigned;
    return static_cast<SignedType>(result);
}

template <typename SignedType>
inline SignedType SubWithWraparound(SignedType a, SignedType b)
{
    static_assert(std::is_integral<SignedType>::value && std::is_signed<SignedType>::value,
        "use this for signed integer types");
    using UnsignedType = typename std::make_unsigned<SignedType>::type;
    UnsignedType aUnsigned = static_cast<UnsignedType>(a);
    UnsignedType bUnsigned = static_cast<UnsignedType>(b);
    UnsignedType result = aUnsigned - bUnsigned;
    return static_cast<SignedType>(result);
}

template <typename SignedType>
inline SignedType MulWithWraparound(SignedType a, SignedType b)
{
    static_assert(std::is_integral<SignedType>::value && std::is_signed<SignedType>::value,
        "use this for signed integer types");
    using UnsignedType = typename std::make_unsigned<SignedType>::type;
    UnsignedType aUnsigned = static_cast<UnsignedType>(a);
    UnsignedType bUnsigned = static_cast<UnsignedType>(b);
    UnsignedType result = aUnsigned * bUnsigned;
    return static_cast<SignedType>(result);
}

template <typename SignedType>
inline SignedType ShlWithWraparound(SignedType a, SignedType b)
{
    using UnsignedType = typename std::make_unsigned<SignedType>::type;
    const UnsignedType kMask = (sizeof(a) * 8) - 1;
    return static_cast<SignedType>(static_cast<UnsignedType>(a) << (static_cast<UnsignedType>(b) & kMask));
}

template <typename SignedType>
inline SignedType NegateWithWraparound(SignedType a)
{
    static_assert(std::is_integral<SignedType>::value && std::is_signed<SignedType>::value,
        "use this for signed integer types");
    if (a == std::numeric_limits<SignedType>::min()) {
        return a;
    }
    return -a;
}
}  // panda::ecmascript::base

#endif  // ECMASCRIPT_BASE_MATH_HELPER_H
