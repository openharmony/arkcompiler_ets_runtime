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
}  // panda::ecmascript::base

#endif  // ECMASCRIPT_BASE_MATH_HELPER_H
