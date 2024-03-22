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

#ifndef ECMASCRIPT_MEM_MEM_COMMON_H
#define ECMASCRIPT_MEM_MEM_COMMON_H

#include <cstddef>
#include <cstdint>

namespace panda::ecmascript {
template <class T>
inline uintptr_t ToUintPtr(T *val)
{
    return reinterpret_cast<uintptr_t>(val);
}

inline uintptr_t ToUintPtr(std::nullptr_t)
{
    return reinterpret_cast<uintptr_t>(nullptr);
}

inline void *ToVoidPtr(uintptr_t val)
{
    return reinterpret_cast<void *>(val);
}

/*
    uint64_t return type usage in memory literals for giving
    compile-time error in case of integer overflow
*/
constexpr uint64_t SHIFT_KB = 10ULL;
constexpr uint64_t SHIFT_MB = 20ULL;
constexpr uint64_t SHIFT_GB = 30ULL;

constexpr uint64_t operator"" _KB(long double count)
{
    return count * (1ULL << SHIFT_KB);
}

// NOLINTNEXTLINE(google-runtime-int)
constexpr uint64_t operator"" _KB(unsigned long long count)
{
    return count * (1ULL << SHIFT_KB);
}

constexpr uint64_t operator"" _MB(long double count)
{
    return count * (1ULL << SHIFT_MB);
}

// NOLINTNEXTLINE(google-runtime-int)
constexpr uint64_t operator"" _MB(unsigned long long count)
{
    return count * (1ULL << SHIFT_MB);
}

constexpr uint64_t operator"" _GB(long double count)
{
    return count * (1ULL << SHIFT_GB);
}

// NOLINTNEXTLINE(google-runtime-int)
constexpr uint64_t operator"" _GB(unsigned long long count)
{
    return count * (1ULL << SHIFT_GB);
}
}  // namespace panda::ecmascript

#endif  // ECMASCRIPT_MEM_MEM_COMMON_H
