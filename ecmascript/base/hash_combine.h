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

#ifndef ECMASCRIPT_BASE_HASH_COMBINE_H
#define ECMASCRIPT_BASE_HASH_COMBINE_H

#include <cstddef>
#include <cstdint>
#include "libpandabase/macros.h"

namespace panda::ecmascript::base {
class HashCombiner {
public:
    static uint64_t HashCombine(uint64_t seed, uint64_t value)
    {
        // In the meantime, we're not considering 32-bit systems
        ASSERT(sizeof(void *) == 8); // 8 : make sure the void* pointer is 8 bytes in size
        const uint64_t m = uint64_t{0xC6A4A7935BD1E995};
        const uint32_t r = 47;

        value *= m;
        value ^= value >> r;
        value *= m;

        seed ^= value;
        seed *= m;
        return seed;
    }
};
}
#endif  //HASH_COMBINE_H
