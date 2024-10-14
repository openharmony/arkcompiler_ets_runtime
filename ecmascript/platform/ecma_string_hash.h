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

#ifndef ECMASCRIPT_PLATFORM_ECMA_STRING_HASH_H
#define ECMASCRIPT_PLATFORM_ECMA_STRING_HASH_H

#include <cstdint>

namespace panda::ecmascript {
    enum class EcmaStringHash : uint32_t {
        BLOCK_SIZE = 4,
        HASH_SHIFT = 5,
        HASH_MULTIPLY = 31,
        MIN_SIZE_FOR_UNROLLING = 128,
    };
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_ECMA_STRING_HASH_H
