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

#include "libpandabase/utils/bit_utils.h"

namespace panda::ecmascript::base::math {
constexpr uint32_t GetIntLog2(const uint32_t X)
{
    ASSERT((X > 0) && !(X & (X - 1U)));
    return static_cast<uint32_t>(panda_bit_utils_ctz(X));
}

constexpr uint64_t GetIntLog2(const uint64_t X)
{
    ASSERT((X > 0) && !(X & (X - 1U)));
    return static_cast<uint64_t>(panda_bit_utils_ctzll(X));
}
}  // panda::ecmascript::base

#endif  // PANDA_LIBBASE_UTILS_MATH_HELPERS_H_
