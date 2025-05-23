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
#include "ecmascript/base/config.h"
#include "ecmascript/platform/ecma_string_hash.h"

namespace panda::ecmascript {
class EcmaStringHashInternal {
friend class EcmaStringHashHelper;
private:
#if ENABLE_NEXT_OPTIMIZATION
    template <typename T>
    static uint32_t ComputeHashForDataOfLongString(const T *data, size_t size,
                                                   uint32_t hashSeed)
    {
        /**
         *  process the first {remainder} items of data[] and hashSeed
         *  for example, if remainder = 2,
         *  then hash[2] = data[0] * 31^1, hash[3] = data[1] * 31^0;
         *  hash[0] = hashSeed * 31^{remainder}
         *
         *  the rest elements in data[] will be processed with for loop as follows
         *  hash[0]: hash[0] * 31^4 + data[i] * 31^3
         *  hash[1]: hash[1] * 31^4 + data[i+1] * 31^2
         *  hash[2]: hash[2] * 31^4 + data[i+2] * 31^1
         *  hash[3]: hash[3] * 31^4 + data[i+3] * 31^0
         *  i starts at {remainder} and every time += 4,
         *  at last, totolHash = hash[0] + hash[1] + hash[2] + hash[3];
         */
        static_assert(std::is_same_v<T, uint8_t> || std::is_same_v<T, uint16_t>);
        constexpr size_t blockSize = EcmaStringHash::BLOCK_SIZE;
        constexpr size_t loopSize = EcmaStringHash::SIMD_U8_LOOP_SIZE;
        uint32_t hash[blockSize] = {};
        uint32_t index = 0;
        uint32_t remainder = size & (blockSize - 1);
        switch (remainder) {
#define CASE(N) case (N): \
    hash[blockSize - (N)] = data[index++] * EcmaStringHash::MULTIPLIER[blockSize - (N)]; [[fallthrough]]
            CASE(EcmaStringHash::SIZE_3);
            CASE(EcmaStringHash::SIZE_2);
            CASE(EcmaStringHash::SIZE_1);
#undef CASE
            default:
                break;
        }
        hash[0] += hashSeed * EcmaStringHash::MULTIPLIER[blockSize - 1 - remainder];

        uint32x4_t dataVec;
        uint32x4_t hashVec;
        uint32x4_t multiplierVec = vld1q_u32(EcmaStringHash::MULTIPLIER);
        uint32x4_t scaleVec = vdupq_n_u32(EcmaStringHash::BLOCK_MULTIPLY);

        if constexpr (std::is_same_v<T, uint8_t>) {
            // process 4 elements with for loop if (size-index) % 8 = 4
            if ((size - index) % loopSize == blockSize) {
                for (size_t i = 0; i < blockSize; i++) {
                    hash[i] = hash[i] * EcmaStringHash::BLOCK_MULTIPLY + data[index++] * EcmaStringHash::MULTIPLIER[i];
                }
            }
            hashVec = vld1q_u32(hash);
            for (; index < size; index += loopSize) {
                uint8x8_t dataVec8 = vld1_u8(data + index);
                uint16x8_t dataVec16 = vmovl_u8(dataVec8);
                dataVec = vmovl_u16(vget_low_u16(dataVec16));
                hashVec = vaddq_u32(vmulq_u32(hashVec, scaleVec), vmulq_u32(dataVec, multiplierVec));
                dataVec = vmovl_u16(vget_high_u16(dataVec16));
                hashVec = vaddq_u32(vmulq_u32(hashVec, scaleVec), vmulq_u32(dataVec, multiplierVec));
            }
        } else {
            hashVec = vld1q_u32(hash);
            for (; index < size; index += blockSize) {
                dataVec = vmovl_u16(vld1_u16(data + index));
                hashVec = vaddq_u32(vmulq_u32(hashVec, scaleVec), vmulq_u32(dataVec, multiplierVec));
            }
        }
        return vaddvq_u32(hashVec);
    }
#else
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
#endif
};
}  // namespace panda::ecmascript
#endif  // ECMASCRIPT_PLATFORM_ECMA_STRING_HASH_ARM64_H