/*
 * Copyright (c) 2022 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BASE_BIT_HELPER_H
#define ECMASCRIPT_BASE_BIT_HELPER_H

#include <cstdint>
#include <cstring>
#include <limits>
#include <type_traits>

namespace panda::ecmascript::base {
constexpr uint64_t pureNaN = 0x7FF8ULL << 48U; // Be sure return the NaN that is safe.
template <class S, class R>
union Data {
    S src;
    R dst;
};

bool CheckBufferBounds(void* addr, void* buffer, size_t bufferSize, const char* name);

template <typename T>
inline T ReadBuffer(void **buffer, size_t offset)
{
    T result = *(reinterpret_cast<T *>(*buffer));
    *buffer = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(*buffer) + offset);
    return result;
}

template <typename T>
inline T ReadBuffer(void **buffer)
{
    T result = *(reinterpret_cast<T *>(*buffer));
    *buffer = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(*buffer) + result.offset_);
    return result;
}

inline char *ReadBuffer(void **buffer)
{
    auto result = reinterpret_cast<char *>(*buffer);
    *buffer = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(*buffer) + strlen(result) + 1);
    return result;
}

template <typename T>
inline T *ReadBufferInSize(void **buffer)
{
    T *result = reinterpret_cast<T *>(*buffer);
    *buffer = reinterpret_cast<void *>(reinterpret_cast<uintptr_t>(*buffer) + result->Size());
    return result;
}

template <typename T>
inline constexpr uint32_t CountLeadingZeros(T value)
{
    constexpr size_t RADIX = 2;
    static_assert(std::is_integral<T>::value, "T must be integral");
    static_assert(std::is_unsigned<T>::value, "T must be unsigned");
    static_assert(std::numeric_limits<T>::radix == RADIX, "Unexpected radix!");
    static_assert(sizeof(T) == sizeof(uint64_t) || sizeof(T) <= sizeof(uint32_t), "Unsupported sizeof(T)");
    if (value == 0) {
        return sizeof(T) * 8; // 8: Each byte has 8 bits
    }
    if (sizeof(T) == sizeof(uint64_t)) {
        return __builtin_clzll(static_cast<uint64_t>(value));
    }
    // 32 : 32 mean the bits of type T is less than 32
    return __builtin_clz(static_cast<uint32_t>(value)) - (32 - std::numeric_limits<T>::digits);
}

inline constexpr uint32_t CountLeadingZeros32(uint32_t value)
{
    return CountLeadingZeros(value);
}

inline constexpr uint32_t CountLeadingZeros64(uint64_t value)
{
    return CountLeadingZeros(value);
}

inline constexpr uint32_t CountLeadingOnes32(uint32_t value)
{
    return CountLeadingZeros(~value);
}

inline constexpr uint32_t CountLeadingOnes64(uint64_t value)
{
    return CountLeadingZeros(~value);
}

template <typename T>
inline constexpr uint32_t CountTrailingZeros(T value)
{
    constexpr size_t RADIX = 2;
    static_assert(std::is_integral<T>::value, "T must be integral");
    static_assert(std::is_unsigned<T>::value, "T must be unsigned");
    static_assert(std::numeric_limits<T>::radix == RADIX, "Unexpected radix!");
    static_assert(sizeof(T) == sizeof(uint64_t) || sizeof(T) <= sizeof(uint32_t), "Unsupported sizeof(T)");
    if (value == 0) {
        return sizeof(T) * 8; // 8: Each byte has 8 bits
    }
    if (sizeof(T) == sizeof(uint64_t)) {
        return __builtin_ctzll(static_cast<uint64_t>(value));
    }
    return __builtin_ctz(static_cast<uint32_t>(value));
}

inline constexpr unsigned CountTrailingZeros32(uint32_t value)
{
    return CountTrailingZeros(value);
}

inline constexpr unsigned CountTrailingZeros64(uint64_t value)
{
    return CountTrailingZeros(value);
}

inline constexpr unsigned CountTrailingOnes32(uint32_t value)
{
    return CountTrailingZeros(~value);
}

inline constexpr unsigned CountTrailingOnes64(uint64_t value)
{
    return CountTrailingZeros(~value);
}

/// isMask_64 - This function returns true if the argument is a non-empty
/// sequence of ones starting at the least significant bit with the remainder
/// zero (64 bit version).
constexpr inline bool IsMask_64(uint64_t Value)
{
    return Value && ((Value + 1) & Value) == 0;
}

/// isShiftedMask_64 - This function returns true if the argument contains a
/// non-empty sequence of ones with the remainder zero (64 bit version.)
constexpr inline bool IsShiftedMask_64(uint64_t Value)
{
    return Value && IsMask_64((Value - 1) | Value);
}

template <typename T>
constexpr T RoundUp(T x, size_t n)
{
    static_assert(std::is_integral<T>::value, "T must be integral");
    return (static_cast<size_t>(x) + n - 1U) & (-n);
}

template <class To, class From>
inline To bit_cast(const From &src) noexcept  // NOLINT(readability-identifier-naming)
{
    static_assert(sizeof(To) == sizeof(From), "size of the types must be equal");
    // The use of security functions 'memcpy_s' here will have a greater impact on performance
    Data<From, To> data;
    data.src = src;
    return data.dst;
}

template <typename T>
inline constexpr uint32_t BitNumbers()
{
    constexpr int BIT_NUMBER_OF_CHAR = 8;
    return sizeof(T) * BIT_NUMBER_OF_CHAR;
}
}  // panda::ecmascript::base
#endif
