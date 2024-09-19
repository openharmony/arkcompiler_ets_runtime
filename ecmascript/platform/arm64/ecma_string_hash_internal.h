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

#ifndef ECMASCRIPT_PLATFORM_ECMA_STRING_HASH_ARM64_H
#define ECMASCRIPT_PLATFORM_ECMA_STRING_HASH_ARM64_H

#include <cstdint>

#include <arm_neon.h>

#include "ecmascript/platform/ecma_string_hash.h"

namespace panda::ecmascript {
class EcmaStringHashInternal {
friend class EcmaStringHashHelper;
private:
    template <typename T>
    static uint32_t ComputeHashForDataOfLongString(const T *data, size_t size,
                                                   uint32_t hashSeed)
    {
        constexpr uint32_t hashShift = static_cast<uint32_t>(EcmaStringHash::HASH_SHIFT);
        constexpr uint32_t blockSize = static_cast<size_t>(EcmaStringHash::BLOCK_SIZE);
        uint32_t hash[blockSize] = {0};
        uint32_t index = 0;
        uint32x4_t hashVec = vld1q_u32(hash);
        uint32x4_t multiplier_vec = vdupq_n_u32(static_cast<uint32_t>(EcmaStringHash::HASH_MULTIPLY));
        uint32x4_t dataVec;
        for (; index + blockSize <= size; index += blockSize) {
            dataVec[0] = data[index];
            dataVec[1] = data[index + 1]; // 1: the second element of the block
            dataVec[2] = data[index + 2]; // 2: the third element of the block
            dataVec[3] = data[index + 3]; // 3: the fourth element of the block
            hashVec = vaddq_u32(vmulq_u32(hashVec, multiplier_vec), dataVec);
        }
        vst1q_u32(hash, hashVec);
        for (; index < size; ++index) {
            hash[0] = (hash[0] << hashShift) - hash[0] + data[index];
        }
        uint32_t totalHash = hashSeed;
        for (uint32_t i = 0; i < blockSize; ++i) {
            totalHash = (totalHash << hashShift) - totalHash + hash[i];
        }
        return totalHash;
    }
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_ECMA_STRING_HASH_ARM64_H