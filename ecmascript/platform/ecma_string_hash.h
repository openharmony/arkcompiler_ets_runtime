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
    class EcmaStringHash {
    public:
        static constexpr size_t BLOCK_SIZE = 4;
        static constexpr size_t SWITCH_UNROLLING_SIZE = 32;
        static constexpr size_t MIN_SIZE_FOR_UNROLLING = 128;

        static constexpr uint32_t HASH_SHIFT = 5;
        static constexpr uint32_t HASH_MULTIPLY = 31;
        static constexpr uint32_t BLOCK_MULTIPLY = 31 * 31 * 31 * 31;

        static constexpr uint32_t MULTIPLIER[BLOCK_SIZE] = { 31 * 31 * 31, 31 * 31, 31, 1 };

        static constexpr size_t SIZE_1 = 1;
        static constexpr size_t SIZE_2 = 2;
        static constexpr size_t SIZE_3 = 3;
        static constexpr size_t SIZE_4 = 4;
        static constexpr size_t SIZE_5 = 5;
        static constexpr size_t SIZE_6 = 6;
        static constexpr size_t SIZE_7 = 7;
        static constexpr size_t SIZE_8 = 8;
        static constexpr size_t SIZE_9 = 9;
        static constexpr size_t SIZE_10 = 10;
        static constexpr size_t SIZE_11 = 11;
        static constexpr size_t SIZE_12 = 12;
        static constexpr size_t SIZE_13 = 13;
        static constexpr size_t SIZE_14 = 14;
        static constexpr size_t SIZE_15 = 15;
        static constexpr size_t SIZE_16 = 16;
        static constexpr size_t SIZE_17 = 17;
        static constexpr size_t SIZE_18 = 18;
        static constexpr size_t SIZE_19 = 19;
        static constexpr size_t SIZE_20 = 20;
        static constexpr size_t SIZE_21 = 21;
        static constexpr size_t SIZE_22 = 22;
        static constexpr size_t SIZE_23 = 23;
        static constexpr size_t SIZE_24 = 24;
        static constexpr size_t SIZE_25 = 25;
        static constexpr size_t SIZE_26 = 26;
        static constexpr size_t SIZE_27 = 27;
        static constexpr size_t SIZE_28 = 28;
        static constexpr size_t SIZE_29 = 29;
        static constexpr size_t SIZE_30 = 30;
        static constexpr size_t SIZE_31 = 31;
        static constexpr size_t SIZE_32 = 32;
    };
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_ECMA_STRING_HASH_H
