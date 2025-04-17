/*
* Copyright (c) 2025 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_BASE_HASH_HELPER_H
#define ECMASCRIPT_BASE_HASH_HELPER_H

#include <array>

#include "ecmascript/platform/ecma_string_hash.h"

namespace panda::ecmascript::base {

    template <size_t N>
    constexpr std::array<uint32_t, N> ComputePowerOf31()
    {
        std::array<uint32_t, N> result{};
        result[0] = 1;
        for (size_t i = 1; i < N; i++) {
            result[i] = result[i - 1] * EcmaStringHash::HASH_MULTIPLY;
        }
        return result;
    }

    constexpr auto POWER_OF_31_TABLE =
        ComputePowerOf31<EcmaStringHash::SWITCH_UNROLLING_SIZE + 1>();
}

#endif // ECMASCRIPT_BASE_HASH_HELPER_H
