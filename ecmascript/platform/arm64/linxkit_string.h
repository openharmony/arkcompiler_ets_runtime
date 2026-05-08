/*
 * Copyright (c) 2026 Huawei Device Co., Ltd.
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

#ifndef LINXKIT_STRING_H
#define LINXKIT_STRING_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "libpandabase/utils/span.h"
#include "ecmascript/platform/string_hash.h"

#ifdef __cplusplus
extern "C++" {
#endif

template <typename T>
uint32_t __attribute__((always_inline)) LinxkitComputeHashForData(const T *data, size_t size, uint32_t hashSeed)
{
    if (size == 0) {
        return hashSeed;
    }
    const uint16_t values[4] = {29791, 961, 31, 1};  // stored in memory
    if constexpr (sizeof(T) == 2) {
        asm volatile(
            "mov    x3, %[data]\n"     // data pointer
            "mov    w4, %w[size]\n"    // data length
            "mov    w15, %w[hash]\n"
            "ld1    {v0.4h}, [%[a]]\n"  // load multipliers v0.4h = [29791, 961, 31, 1]
            "ldr    w7, =923521\n"      // w7 = 31^4

            "1:\n"  // safe_load_mix
            "cmp    w4, #8\n"
            "b.lt   2f\n"
            
            // hash = ((hash * 31^4 + low_sum) * 31^4 + high_sum)
            "ldr    q1, [x3], #16\n"    // v1.8h
            "umull  v2.4s, v1.4h, v0.4h\n"
            "ext    v1.16b, v1.16b, v1.16b, #8\n"
            "addv   s2, v2.4s\n"
            "umull  v1.4s, v1.4h, v0.4h\n"
            "fmov   w11, s2\n"
            "addv   s1, v1.4s\n"
            "madd   w15, w15, w7, w11\n"  // hash = hash * 31^4 + low_sum
            "fmov   w12, s1\n"
            "madd   w15, w15, w7, w12\n"  // hash = hash * 31^4 + high_sum
            "subs   w4, w4, #8\n"
            "cbz    w4, 4f\n"          // finish_mix
            "b      1b\n"              // back to loop 1

            "2:\n"  // compute_less8_mix
            "cmp    w4, #4\n"
            "b.ge   3f\n"              // 8 > data >= 4, jump to 3

            // Accelerate tail processing when less than 4
            "subs   w4, w4, #1\n"
            "mov    w7, #31\n"
            "ldrh   w11, [x3]\n"
            "madd   w15, w15, w7, w11\n"
            "cbz    w4, 4f\n"
            "subs   w4, w4, #1\n"
            "ldrh   w11, [x3, #2]\n"
            "madd   w15, w15, w7, w11\n"
            "cbz    w4, 4f\n"
            "ldrh   w11, [x3, #4]\n"
            "madd   w15, w15, w7, w11\n"
            "b      4f\n"

            "3:\n"  // compute_greater4_mix vectorized processing of 4 bytes
            "ld1    {v1.4h}, [x3], #8\n"
            "umull  v2.4s, v1.4h, v0.4h\n"
            "addv   s2, v2.4s\n"
            "fmov   w11, s2\n"
            "madd   w15, w15, w7, w11\n"
            "subs   w4, w4, #4\n"
            "cbz    w4, 4f\n"          // all processed
            "b      2b\n"              // data < 4, jump to 2

            "4:\n"  // finish_mix
            "mov    %w[hash], w15\n"

            : [hash] "+r"(hashSeed)
            : [size] "r"(size), [data] "r"(data), [a] "r"(values)
            : "memory", "cc", "x3", "x4", "x7", "x11", "x12", "x15", "v0", "v1", "v2"
        );
    } else {
        ASSERT(sizeof(T) == sizeof(uint8_t));
        asm volatile(
            "mov    x3, %[data]\n"    // data pointer
            "mov	w4, %w[size]\n"   // data length
            "mov    w15, %w[hash]\n"
            "ld1    {v0.4h}, [%[a]]\n"  // load multipliers v0.4h = [29791, 961, 31, 1]
            "ldr    w7, =923521\n"      // 31^4

            "1:\n"      // safe_load_mix
            "cmp    w4, #16\n"
            "b.lt   2f\n"       // jump to 2 if less than 16

            // hash = ((hash * 31^4 + sum_0_3) * 31^4 + sum_4_7) * 31^4 + sum_8_11) * 31^4 + sum_12_15
            // = hash * 31^16 + b0*31^15 + b1*31^14 + ... + b15*31^0
            "ldr	q1, [x3], #16\n"
            "ushll	v2.8h, v1.8b, #0\n"         // v2.8h = [b0,b1,...,b7]
            "ext	v1.16b, v1.16b, v1.16b, #8\n"   // Rotate v1 = [b8,b9,...,b15,b0,b1,...,b7]
            "umull	v3.4s, v2.4h, v0.4h\n"      // v3.4s = [b0*29791, b1*961, b2*31, b3*1]
            "ext	v2.16b, v2.16b, v2.16b, #8\n"
            "ushll	v1.8h, v1.8b, #0\n"     // Extend high 8 bytes to halfwords: v1.8h = [b8,b9,...,b15]
            "addv	s3, v3.4s\n"
            "umull	v2.4s, v2.4h, v0.4h\n"
            "umull	v4.4s, v1.4h, v0.4h\n"
            "ext	v1.16b, v1.16b, v1.16b, #8\n"   // Rotate v1 again to align bytes 12-15 for next multiplication
            "fmov	w14, s3\n"
            "addv	s2, v2.4s\n"
            "addv	s3, v4.4s\n"
            "umull	v1.4s, v1.4h, v0.4h\n"
            "madd	w15, w15, w7, w14\n"    // Hash = hash * 31^4 + sum_0_3
            "fmov	w14, s2\n"
            "madd	w15, w15, w7, w14\n"    // Hash = (hash * 31^4 + sum_0_3) * 31^4 + sum_4_7
            "fmov	w14, s3\n"
            "madd	w15, w15, w7, w14\n"
            "addv	s1, v1.4s\n"
            "fmov	w14, s1\n"      // Extract sum for bytes 12-15
            "madd	w15, w15, w7, w14\n"
            "subs   w4, w4, #16\n"
            "cbz    w4, 5f\n"
            "b      1b\n"       // safe_load_mix

            "2:\n"      // compute_less16_mix remaining characters less than 16
            "cmp    w4, #8\n"
            "b.ge   4f\n"  // compute_ge8_less16

            "3:\n"  // compute_less8_mix
            "mov    w7, #31\n"
            "ldrb   w10, [x3], #1\n"
            "madd   w15, w15, w7, w10\n"
            "subs   w4, w4, #1\n"
            "b.ne   3b\n"
            "b      5f\n"

            "4:\n"  // compute_ge8_less16
            "ld1    {v1.8b}, [x3], #8\n"
            // process low 4 bytes (v1.b[0]~v1.b[3])
            "uxtl   v2.8h, v1.8b\n"     // extend 8 bytes -> 8 halfwords
            "umull  v3.4s, v0.4h, v2.4h\n"   // low 4 halfwords multiply v0.4h
            "addv   s4, v3.4s\n"
            "fmov   w5, s4\n"
            "madd   w15, w15, w7, w5\n"      // hash = hash*31^4 + sum_low
            // process high 4 bytes (v1.b[4]~v1.b[7])
            "ext    v1.16b, v1.16b, v1.16b, #4\n"   // move high 4 bytes to low 4 bytes position
            "uxtl   v2.8h, v1.8b\n"
            "umull  v3.4s, v0.4h, v2.4h\n"
            "addv   s4, v3.4s\n"
            "fmov   w5, s4\n"
            "madd   w15, w15, w7, w5\n"      // hash = hash*31^4 + sum_high

            "sub    w4, w4, #8\n"            // remaining bytes minus 8
            "cbnz   w4, 3b\n"

            "5:\n"  // finish_mix
            "mov    %w[hash], w15\n"
            : [hash] "+r"(hashSeed)
            : [size] "r"(size), [data] "r"(data), [a] "r"(values)
            : "memory", "cc", "x3", "x4", "x5", "x7", "x10", "x14", "x15", "v0", "v1", "v2", "v3", "v4"
        );
    }
    return hashSeed;
}

#ifdef __cplusplus
}
#endif
#endif
