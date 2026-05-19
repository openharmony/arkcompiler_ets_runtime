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

#ifndef LINXKIT_PARSE_H
#define LINXKIT_PARSE_H

#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>

#include "libpandabase/utils/span.h"
#include "ecmascript/base/json_parser.h"

#ifdef __cplusplus
extern "C++" {
#endif

template <typename T>
bool __attribute__((always_inline)) LinxkitParseStringLength(
    size_t &length, bool &isAscii, bool inObjOrArrOrMap, const T *&end_, const T *current_, const T *range_)
{
    if constexpr (sizeof(T) == 2) {
        bool res = false;
        asm volatile(
            "mov        x0, %[len]\n"
            "mov        x2, %[curr]\n"
            "mov        w9, %w[isO]\n"
            "cmp        w9, #1\n"
            "csel       x3, %[range], %[end], eq\n"
            "movi       v1.8h, #34\n"  // double quote 0x22
            "movi       v2.8h, #92\n"  // backslash 0x5c
            "cbz        w9, 5f\n"

            "1:\n"  // 1 is_objOrArrorMap
            "add        x4, x2, #16\n"
            "cmp        x4, x3\n"
            "b.ls       4f\n"  // remaining >= 8 uint16_t chars, jump to simd_loop
            "sub        x4, x4, #8\n"
            "cmp        x4, x3\n"
            "b.ls       3f\n"  // remaining >= 4 uint16_t chars

            "2:\n"  // 2 is_objOrArrorMap_scalar
            "cmp        x2, x3\n"
            "b.ge       18f\n"  // 18f
            "ldrh       w6, [x2], #2\n"
            "cmp        w6, 0x22\n"  // double quote 0x22
            "b.eq       16f\n"       // inObjOrArrOrMap update end_ before ret true
            "cmp        w6, 0x5c\n"
            "b.eq       9f\n"  // check double quote
            "cmp        w6, 0x20\n"
            "b.lo       18f\n"  // 18f
            "cmp        w6, 0x7f\n"
            "b.hi       14f\n"  // 10 14f
            "add        x0, x0, #1\n"
            "b          2b\n"

            "3:\n"  // remaining chars >= 4 and < 8, is_objOrArrorMap_half
            "ld1        {v0.4h}, [x2]\n"
            "cmeq       v5.4h, v1.4h, v0.4h\n"
            "cmeq       v6.4h, v2.4h, v0.4h\n"
            "orr        v7.8b, v5.8b, v6.8b\n"
            "umaxv      h8, v7.4h\n"
            "fmov       w4, s8\n"
            "cbnz       w4, 2b\n"  // double quote or backslash in 4h, need special handling
            "uminv      h8, v0.4h\n"
            "fmov       w4, s8\n"
            "cmp        w4, #32\n"  // less than space is invalid
            "b.lt       18f\n"
            "add        x0, x0, #3\n"
            "add        x2, x2, #8\n"
            "umaxv      h8, v0.4h\n"
            "fmov       w4, s8\n"
            "cmp        w4, #127\n"  // greater than 127 is non-ASCII
            "b.gt       14f\n"
            "add        x0, x0, #1\n"
            "b          2b\n"

            "4:\n"  // is_objOrArrorMap_loop
            "ld1        {v0.8h}, [x2]\n"
            "cmeq       v5.8h, v1.8h, v0.8h\n"
            "cmeq       v6.8h, v2.8h, v0.8h\n"
            "orr        v7.16b, v5.16b, v6.16b\n"
            "umaxv      h8, v7.8h\n"
            "fmov       w4, s8\n"
            "cbnz       w4, 2b\n"  // double quote or backslash in 8h, need special handling
            "uminv      h8, v0.8h\n"
            "fmov       w4, s8\n"
            "cmp        w4, #32\n"  // less than space is invalid
            "b.lt       18f\n"
            "add        x0, x0, #7\n"
            "add        x2, x2, #16\n"
            "umaxv      h8, v0.8h\n"
            "fmov       w4, s8\n"
            "cmp        w4, #127\n"  // greater than 127 is non-ASCII
            "b.gt       14f\n"
            "add        x0, x0, #1\n"
            "b          1b\n"

            "5:\n"  // 5 not_objOrArrorMap
            "add        x4, x2, #16\n"
            "cmp        x4, x3\n"
            "b.ls       8f\n"  // remaining chars >= 8
            "sub        x4, x4, #8\n"
            "cmp        x4, x3\n"
            "b.ls       7f\n"  // 8 > remaining chars >= 4

            "6:\n"  // 6 not_objOrArrorMap_scalar
            "cmp        x2, x3\n"
            "b.ge       17f\n"
            "ldrh       w6, [x2], #2\n"
            "cmp        w6, #92\n"
            "b.eq       9f\n"
            "cmp        w6, #32\n"
            "b.lo       18f\n"
            "cmp        w6, #127\n"
            "b.hi       14f\n"
            "add        x0, x0, #1\n"
            "b          6b\n"

            "7:\n"  // 7 not_objOrArrorMap_half
            "ld1        {v0.4h}, [x2]\n"
            "cmeq       v5.4h, v2.4h, v0.4h\n"
            "umaxv      h8, v5.4h\n"
            "fmov       w4, s8\n"
            "cbnz       w4, 6b\n"  // backslash in 4h, need special handling
            "uminv      h8, v0.4h\n"
            "fmov       w4, s8\n"
            "cmp        w4, #32\n"  // less than space is invalid
            "b.lt       18f\n"
            "add        x0, x0, #3\n"
            "add        x2, x2, #8\n"
            "umaxv      h8, v0.4h\n"
            "fmov       w4, s8\n"
            "cmp        w4, #127\n"  // greater than 127 is non-ASCII
            "b.gt       14f\n"
            "add        x0, x0, #1\n"
            "b          6b\n"

            "8:\n"  // 8 simd_loop_notObj
            "ld1        {v0.8h}, [x2]\n"
            "cmeq       v5.8h, v2.8h, v0.8h\n"
            "umaxv      h8, v5.8h\n"
            "fmov       w4, s8\n"
            "cbnz       w4, 6b\n"  // backslash in 8h, need special handling
            "uminv      h8, v0.8h\n"
            "fmov       w4, s8\n"
            "cmp        w4, #32\n"  // less than space is invalid
            "b.lt       18f\n"
            "add        x0, x0, #7\n"
            "add        x2, x2, #16\n"
            "umaxv      h8, v0.8h\n"
            "fmov       w4, s8\n"
            "cmp        w4, #127\n"  // greater than 127 is non-ASCII
            "b.gt       14f\n"
            "add        x0, x0, #1\n"
            "b          5b\n"

            "9:\n"  // 9 CheckBackslash
            "cmp        x2, x3\n"
            "b.ge       18f\n"
            "ldrh       w6, [x2], #2\n"
            "cmp        w6, #34\n"
            "b.eq       15f\n"
            "cmp        w6, #92\n"
            "b.eq       15f\n"
            "cmp        w6, #47\n"
            "b.eq       15f\n"
            "cmp        w6, #98\n"
            "b.eq       15f\n"
            "cmp        w6, #102\n"
            "b.eq       15f\n"
            "cmp        w6, #110\n"
            "b.eq       15f\n"
            "cmp        w6, #114\n"
            "b.eq       15f\n"
            "cmp        w6, #116\n"
            "b.eq       15f\n"
            "cmp        w6, #117\n"  // case_u
            "b.ne       18f\n"
            "add        x5, x2, #8\n"
            "cmp        x5, x3\n"
            "b.hi       18f\n"
            "mov        w5, #0\n"  // w5: res
            "mov        w6, #4\n"

            "10:\n"  // 10 loop_unicode_entry
            "ldrh       w7, [x2], #2\n"
            "sub        w8, w7, #48\n"  // between 0~9?
            "cmp        w8, #10\n"
            "b.hs       11f\n"  // not_0_to_9
            "add        w5, w8, w5, lsl #4\n"
            "subs       w6, w6, #1\n"
            "b.ne       10b\n"
            "b          13f\n"

            "11:\n"                     // 11 not_0_to_9
            "sub        w8, w7, #97\n"  // between a~f?
            "cmp        w8, #6\n"
            "b.hs       12f\n"
            "sub        w7, w7, #87\n"  // proc_a_to_f
            "add        w5, w7, w5, lsl #4\n"
            "subs       w6, w6, #1\n"
            "b.ne       10b\n"
            "b          13f\n"

            "12:\n"                     // 12 not_a_to_f
            "sub        w8, w7, #65\n"  // between A~F?
            "cmp        w8, #6\n"
            "b.hs       18f\n"
            "sub        w7, w7, #55\n"  // proc_A_to_F
            "add        w5, w7, w5, lsl #4\n"
            "subs       w6, w6, #1\n"
            "b.ne       10b\n"

            "13:\n"  // 13 case_u_str
            "sub        w5, w5, #1\n"
            "cmp        w5, #126\n"
            "b.ls       15f\n"
            "14:\n"  // 14 ascii_false
            "mov        %w[isAscii], #0\n"

            "15:\n"  // 15 check_pass
            "add        x0, x0, #1\n"
            "cbz        w9, 5b\n"
            "b          1b\n"

            "16:\n"  // 16 inObjOrArrOrMap update end_ before ret true
            "sub        x2, x2, #2\n"
            "mov        %[end], x2\n"  // *end_ = double quote
            "17:\n"                    // 17 ret_true
            "mov        %w[res], #1\n"
            "18:\n"  // 18 ret_false
            "mov        %[len], x0\n"
            : [len] "+r"(length), [isAscii] "+r"(isAscii), [end] "+r"(end_), [res] "+r"(res)
            : [curr] "r"(current_), [isO] "r"(inObjOrArrOrMap), [range] "r"(range_)
            : "memory", "cc", "x0", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "w9", "v0",
            "v1", "v2", "v5", "v6", "v7", "v8"
        );
        return res;
    } else {
        static_assert(sizeof(T) == sizeof(uint8_t), "T must be 1 or 2 bytes in size");
        bool res = false;
        asm volatile(
            "mov        x0, %[len]\n"
            "mov        x2, %[curr]\n"
            "mov        w9, %w[isO]\n"
            "cmp        w9, #1\n"
            "csel       x3, %[range], %[end], eq\n"
            "movi       v1.8b, #34\n"  // double quote 0x22
            "movi       v2.8b, #92\n"  // backslash 0x5c

            "cbz        w9, 3f\n"
            "1:\n"  // 1 is_objOrArrorMap
            "add        x4, x2, #8\n"
            "cmp        x4, x3\n"
            "b.hi       2f\n"  // remaining < 8 uint8_t chars
            "ld1        {v0.8b}, [x2]\n"
            "cmeq       v5.8b, v1.8b, v0.8b\n"
            "cmeq       v6.8b, v2.8b, v0.8b\n"
            "orr        v7.8b, v5.8b, v6.8b\n"
            "umaxv      b8, v7.8b\n"
            "fmov       w4, s8\n"
            "cbnz       w4, 2f\n"  // double quote or backslash in 8h, need special handling
            // check if any char < 32
            "uminv      b8, v0.8b\n"
            "fmov       w4, s8\n"
            "cmp        w4, #32\n"
            "b.lt       14f\n"

            "add        x0, x0, #7\n"
            "add        x2, x2, #8\n"
            // check if any char > 127
            "umaxv      b8, v0.8b\n"
            "fmov       w4, s8\n"
            "cmp        w4, #127\n"
            "b.gt       10f\n"
            "add        x0, x0, #1\n"
            "b          1b\n"

            "2:\n"  // 2      is_objOrArrorMap_scalar
            "cmp        x2, x3\n"
            "b.ge       14f\n"
            "ldrb       w6, [x2], #1\n"
            "cmp        w6, 0x22\n"
            "b.eq       12f\n"
            "cmp        w6, 0x5c\n"
            "b.eq       5f\n"
            "cmp        w6, 0x20\n"
            "b.lo       14f\n"
            "cmp        w6, 0x7f\n"
            "b.hi       10f\n"
            "add        x0, x0, #1\n"
            "b          2b\n"

            "3:\n"  // 3 not_objOrArrorMap
            "add        x4, x2, #8\n"
            "cmp        x4, x3\n"
            "b.hi       4f\n"  // remaining < 8 uint8_t chars
            "ld1        {v0.8b}, [x2]\n"
            "cmeq       v5.8b, v2.8b, v0.8b\n"
            "umaxv      b8, v5.8b\n"
            "fmov       w4, s8\n"
            "cbnz       w4, 4f\n"  // backslash in 8h, need special handling
            // check if any char < 32
            "uminv      b8, v0.8b\n"
            "fmov       w4, s8\n"
            "cmp        w4, #32\n"
            "b.lt       14f\n"

            "add        x0, x0, #7\n"
            "add        x2, x2, #8\n"
            // check if any char > 127
            "umaxv      b8, v0.8b\n"
            "fmov       w4, s8\n"
            "cmp        w4, #127\n"
            "b.gt       10f\n"
            "add        x0, x0, #1\n"
            "b          3b\n"

            "4:\n"  // 4 not_objOrArrorMap_scalar
            "cmp        x2, x3\n"
            "b.ge       13f\n"
            "ldrb       w6, [x2], #1\n"
            "cmp        w6, 0x5c\n"
            "b.eq       5f\n"
            "cmp        w6, 0x20\n"
            "b.lo       14f\n"
            "cmp        w6, 0x7f\n"
            "b.hi       10f\n"
            "add        x0, x0, #1\n"
            "b          4b\n"

            "5:\n"  // 5 CheckBackslash
            "cmp        x2, x3\n"
            "b.ge       14f\n"
            "ldrb       w6, [x2], #1\n"
            "cmp        w6, #34\n"
            "b.eq       11f\n"
            "cmp        w6, #92\n"
            "b.eq       11f\n"
            "cmp        w6, #47\n"
            "b.eq       11f\n"
            "cmp        w6, #98\n"
            "b.eq       11f\n"
            "cmp        w6, #102\n"
            "b.eq       11f\n"
            "cmp        w6, #110\n"
            "b.eq       11f\n"
            "cmp        w6, #114\n"
            "b.eq       11f\n"
            "cmp        w6, #116\n"
            "b.eq       11f\n"
            "cmp        w6, #117\n"  // case_u
            "b.ne       14f\n"
            "add        x5, x2, #4\n"
            "cmp        x5, x3\n"
            "b.hi       14f\n"
            "mov        w5, #0\n"  // w5: res
            "mov        w6, #4\n"

            "6:\n"  // 6 loop_unicode_entry
            "ldrb       w7, [x2], #1\n"
            "sub        w8, w7, #48\n"  // between 0~9?
            "cmp        w8, #10\n"
            "b.hs       7f\n"
            "add        w5, w8, w5, lsl #4\n"
            "subs       w6, w6, #1\n"
            "b.ne       6b\n"
            "b          9f\n"

            "7:\n"                      // 7 not_0_to_9
            "sub        w8, w7, #97\n"  // between a~f?
            "cmp        w8, #6\n"
            "b.hs       8f\n"
            // "proc_a_to_f:\n"
            "sub        w7, w7, #87\n"
            "add        w5, w7, w5, lsl #4\n"
            "subs       w6, w6, #1\n"
            "b.ne       6b\n"
            "b          9f\n"

            "8:\n"                      // 8 not_a_to_f
            "sub        w8, w7, #65\n"  // between A~F?
            "cmp        w8, #6\n"
            "b.hs       14f\n"
            // "proc_A_to_F:\n"
            "sub        w7, w7, #55\n"
            "add        w5, w7, w5, lsl #4\n"
            "subs       w6, w6, #1\n"
            "b.ne       6b\n"

            "9:\n"  // 9 case_u_str
            "sub        w5, w5, #1\n"
            "cmp        w5, #126\n"
            "b.ls       11f\n"
            "10:\n"  // 10 ascii_false
            "mov        %w[isAscii], #0\n"

            "11:\n"  // 11 check_pass
            "add        x0, x0, #1\n"
            "cbz        w9, 4b\n"
            "b          1b\n"

            "12:\n"  // inObjOrArrOrMap update end_ before ret true
            "sub        x2, x2, #1\n"
            "mov        %[end], x2\n"  // *end_ = double quote
            "13:\n"                    // 13 ret_true
            "mov        %w[res], #1\n"
            "14:\n"  // 14 ret_false
            "mov        %[len], x0\n"
            : [len] "+r"(length), [isAscii] "+r"(isAscii), [end] "+r"(end_), [res] "+r"(res)
            : [curr] "r"(current_), [isO] "r"(inObjOrArrOrMap), [range] "r"(range_)
            : "memory", "cc", "x0", "x2", "x3", "x4", "x5", "x6", "x7", "x8", "w9", "v0",
            "v1", "v2", "v5", "v6", "v7", "v8"
        );
        return res;
    }
}

template <typename T>
void __attribute__((always_inline)) LinxkitCopyCharWithBackslashTo16(uint16_t *&p, const T *end_, const T *&current_)
{
    if (current_ > end_) {
        return;
    }
    if constexpr (sizeof(T) == 2) {
        asm volatile(
            "mov        x21, %[dst_ptr]\n"
            "mov        x8, %[cur]\n"
            "mov        x9, %[end]\n"
            "sub        x10, x9, x8\n"
            "lsr        x10, x10, #1\n"
            "add        x10, x10, #1\n"
            "movi       v1.8h, #92\n"

            "1:\n"  // loop_entry
            "cmp        x10, #8\n"
            "b.lo       2f\n"
            "ld1        {v0.8h}, [x8]\n"
            "cmeq       v2.8h, v1.8h, v0.8h\n"
            "umaxv      h3, v2.8h\n"
            "fmov       w4, s3\n"
            "cbnz       w4, 2f\n"
            "add        x8, x8, #16\n"
            "st1        {v0.8h}, [x21], #16\n"
            "subs       x10, x10, #8\n"
            "b.eq       16f\n"
            "b          1b\n"

            "2:\n"  // regv_slash_proc: at least one backslash or end terminator in 8h starting from w8
            "ldrh       w23, [x8], #2\n"
            "cmp        w23, #92\n"
            "b.eq       3f\n"  // backslash found
            "strh       w23, [x21], #2\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          2b\n"

            "3:\n"                       // slash_proc
            "sub        x10, x10, #1\n"  // backslash recognized, x10-1
            "ldrh       w23, [x8], #2\n"
            "sub        w22, w23, #34\n"
            "cmp        w22, #58\n"
            "b.hi       4f\n"              // not_34_to_92
            "strh       w23, [x21], #2\n"  // if 34 47 92, copy directly
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"

            "4:\n"  // not_34_to_92
            "sub        w22, w23, #98\n"
            "cmp        w22, #4\n"
            "b.hi       5f\n"
            "add        w23, w22, #8\n"  // if 98 102, subtract 90 and convert then copy
            "strh       w23, [x21], #2\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"

            "5:\n"  // not_34_to_92
            "cmp        w23, #110\n"
            "b.ne       6f\n"
            "mov        w23, #10\n"  // 110->10
            "strh       w23, [x21], #2\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"
            "6:\n"
            "cmp        w23, #114\n"
            "b.ne       7f\n"  // 114->13
            "mov        w23, #13\n"
            "strh       w23, [x21], #2\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"
            "7:\n"
            "cmp        w23, #116\n"
            "b.ne       8f\n"
            "mov        w23, #9\n"  // 116->9
            "strh       w23, [x21], #2\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"

            "8:\n"
            "cmp        w23, #117\n"  // if w23 is u
            "b.ne       15f\n"
            "sub        x10, x10, #1\n"
            "mov        w24, #0\n"  // w24: res
            "mov        x22, #4\n"

            "9:\n"
            "ldrh       w23, [x8], #2\n"
            "sub        w11, w23, #48\n"  // between 0~9?
            "cmp        w11, #10\n"
            "b.hs       10f\n"
            "add        w24, w11, w24, lsl #4\n"
            "subs       x22, x22, #1\n"
            "b.ne       9b\n"
            "b          14f\n"

            "10:\n"
            "sub        w11, w23, #97\n"  // between a~f?
            "cmp        w11, #6\n"
            "b.hs       12f\n"
            "11:\n"  // proc_a_to_f
            "sub        w23, w23, #87\n"
            "add        w24, w23, w24, lsl #4\n"
            "subs       x22, x22, #1\n"
            "b.ne       9b\n"
            "b          14f\n"

            "12:\n"                       // not_a_to_f
            "sub        w11, w23, #65\n"  // between A~F?
            "cmp        w11, #6\n"
            "b.hs       15f\n"
            "13:\n"  // proc_A_to_F
            "sub        w23, w23, #55\n"
            "add        w24, w23, w24, lsl #4\n"
            "subs       x22, x22, #1\n"
            "b.ne       9b\n"

            "14:\n"  // case_u_str
            "strh       w24, [x21], #2\n"
            "subs       x10, x10, #4\n"
            "b.eq       16f\n"
            "b          1b\n"

            "15:\n"                         // unreachable_
            "16:\n"                         // final_
            "mov        %[dst_ptr], x21\n"  // destination pointer
            "mov        %[cur], x8\n"

            : [dst_ptr] "+r"(p), [cur] "+r"(current_)
            : [end] "r"(end_)
            : "memory", "cc", "x8", "x9", "x10", "x11", "x21", "x22", "x23", "x24",
            "v0", "v1", "v2", "v3", "x4"
        );
    } else {
        static_assert(sizeof(T) == sizeof(uint8_t), "T must be 1 or 2 bytes in size");
        asm volatile(
            "mov        x21, %[dst_ptr]\n"
            "mov        x8, %[cur]\n"
            "mov        x9, %[end]\n"
            "sub        x10, x9, x8\n"          // remaining bytes
            "add        x10, x10, #1\n"
            "movi       v1.16b, #92\n"          // backslash '\\'

            "1:\n"  // loop_entry
            "cmp        x10, #16\n"             // process 16 bytes at a time
            "b.lo       2f\n"
            "ld1        {v0.16b}, [x8]\n"
            "cmeq       v2.16b, v1.16b, v0.16b\n"
            "umaxv      b3, v2.16b\n"
            "fmov       w4, s3\n"
            "cbnz       w4, 2f\n"
            "add        x8, x8, #16\n"
            "uxtl       v4.8h, v0.8b\n"         // low 8 bytes -> 8 halfwords
            "uxtl2      v5.8h, v0.16b\n"        // high 8 bytes -> 8 halfwords
            "st1        {v4.16b, v5.16b}, [x21], #32\n" // store 16 halfwords (32 bytes)
            "subs       x10, x10, #16\n"
            "b.eq       16f\n"
            "b          1b\n"

            "2:\n"  // regv_slash_proc: at least one backslash or end terminator in 16 bytes
            "ldrb       w23, [x8], #1\n"
            "cmp        w23, #92\n"
            "b.eq       3f\n"                  // backslash found
            "strh       w23, [x21], #2\n"      // zero-extend to 16-bit and store
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          2b\n"

            "3:\n"                       // slash_proc
            "subs       x10, x10, #1\n"  // backslash recognized, x10-1
            "ldrb       w23, [x8], #1\n"
            "sub        w22, w23, #34\n"
            "cmp        w22, #58\n"
            "b.hi       4f\n"            // not_34_to_92
            "strh       w23, [x21], #2\n" // if 34 47 92, copy directly
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"

            "4:\n"  // not_34_to_92
            "sub        w22, w23, #98\n"
            "cmp        w22, #4\n"
            "b.hi       5f\n"
            "add        w23, w22, #8\n"  // if 98 102, subtract 90 and convert then copy
            "strh       w23, [x21], #2\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"

            "5:\n"  // not_34_to_92
            "cmp        w23, #110\n"
            "b.ne       6f\n"
            "mov        w23, #10\n"      // 110->10 (newline)
            "strh       w23, [x21], #2\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"
            "6:\n"
            "cmp        w23, #114\n"
            "b.ne       7f\n"
            "mov        w23, #13\n"      // 114->13 (carriage return)
            "strh       w23, [x21], #2\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"
            "7:\n"
            "cmp        w23, #116\n"
            "b.ne       8f\n"
            "mov        w23, #9\n"       // 116->9 (tab)
            "strh       w23, [x21], #2\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"

            "8:\n"
            "cmp        w23, #117\n"     // if w23 is 'u' (Unicode escape)
            "b.ne       15f\n"
            "subs       x10, x10, #1\n"
            "mov        w24, #0\n"       // w24: result
            "mov        x22, #4\n"       // 4 hex digits to process

            "9:\n"
            "ldrb       w23, [x8], #1\n"
            "sub        w11, w23, #48\n" // between 0~9?
            "cmp        w11, #10\n"
            "b.hs       10f\n"
            "add        w24, w11, w24, lsl #4\n"
            "subs       x22, x22, #1\n"
            "b.ne       9b\n"
            "b          14f\n"

            "10:\n"
            "sub        w11, w23, #97\n" // between a~f?
            "cmp        w11, #6\n"
            "b.hs       12f\n"
            "11:\n"  // proc_a_to_f
            "sub        w23, w23, #87\n"
            "add        w24, w23, w24, lsl #4\n"
            "subs       x22, x22, #1\n"
            "b.ne       9b\n"
            "b          14f\n"

            "12:\n"                       // not_a_to_f
            "sub        w11, w23, #65\n"  // between A~F?
            "cmp        w11, #6\n"
            "b.hs       15f\n"
            "13:\n"  // proc_A_to_F
            "sub        w23, w23, #55\n"
            "add        w24, w23, w24, lsl #4\n"
            "subs       x22, x22, #1\n"
            "b.ne       9b\n"

            "14:\n"  // case_u_str
            "strh       w24, [x21], #2\n"
            "subs       x10, x10, #4\n"
            "b.eq       16f\n"
            "b          1b\n"

            "15:\n"                         // unreachable_
            "16:\n"                         // final_
            "mov        %[dst_ptr], x21\n"  // update destination pointer
            "mov        %[cur], x8\n"       // update current pointer

            : [dst_ptr] "+r"(p), [cur] "+r"(current_)
            : [end] "r"(end_)
            : "memory", "cc", "x8", "x9", "x10", "x11", "x21", "x22", "x23", "x24",
            "v0", "v1", "v2", "v3", "x4", "v4", "v5"
        );
    }
}

template <typename T>
void __attribute__((always_inline)) LinxkitCopyCharWithBackslashTo8(uint8_t *&p, const T *end_, const T *&current_)
{
    if (current_ > end_) {
        return;
    }
    if constexpr (sizeof(T) == 2) {
        asm volatile(
            "mov        x21, %[dst_ptr]\n"  // destination pointer
            "mov        x8, %[cur]\n"
            "mov        x9, %[end]\n"
            "sub        x10, x9, x8\n"   // x10: byte difference
            "lsr        x10, x10, #1\n"  // convert bytes to char count
            "add        x10, x10, #1\n"
            "movi       v1.8h, #92\n"

            "1:\n"  // loop_entry
            "cmp        x10, #8\n"
            "b.lo       2f\n"
            "ld1        {v0.8h}, [x8]\n"
            "cmeq       v2.8h, v1.8h, v0.8h\n"
            "umaxv      h3, v2.8h\n"
            "fmov       w4, s3\n"
            "cbnz       w4, 2f\n"
            "add        x8, x8, #16\n"
            "xtn        v0.8b, v0.8h\n"
            "st1        {v0.8b}, [x21], #8\n"
            "sub        x10, x10, #8\n"
            "cbz        x10, 16f\n"
            "b          1b\n"

            "2:\n"  // regv_slash_proc: at least one backslash or end terminator in 8h starting from w8
            "ldrh       w23, [x8], #2\n"
            "cmp        w23, #92\n"
            "b.eq       3f\n"  // backslash found
            "strb       w23, [x21], #1\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          2b\n"

            "3:\n"                       // slash_proc
            "sub        x10, x10, #1\n"  // backslash recognized, x10-1
            "ldrh       w23, [x8], #2\n"
            "sub        w22, w23, #34\n"
            "cmp        w22, #58\n"
            "b.hi       4f\n"              // not_34_to_92
            "strb       w23, [x21], #1\n"  // if 34 47 92, copy directly
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"

            "4:\n"  // not_34_to_92
            "sub        w22, w23, #98\n"
            "cmp        w22, #4\n"
            "b.hi       5f\n"
            "add        w23, w22, #8\n"    // if 98 102, subtract 90 and convert then copy
            "strb       w23, [x21], #1\n"  // if 34 47 92, copy directly
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"

            "5:\n"  // not_34_to_92
            "cmp        w23, #110\n"
            "b.ne       6f\n"
            "mov        w23, #10\n"  // 110->10
            "strb       w23, [x21], #1\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"
            "6:\n"
            "cmp        w23, #114\n"
            "b.ne       7f\n"  // 114->13
            "mov        w23, #13\n"
            "strb       w23, [x21], #1\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"
            "7:\n"
            "cmp        w23, #116\n"
            "b.ne       8f\n"
            "mov        w23, #9\n"  // 116->9
            "strb       w23, [x21], #1\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"

            "8:\n"
            "cmp        w23, #117\n"  // if w23 is u
            "b.ne       15f\n"
            "sub        x10, x10, #1\n"
            "mov        w24, #0\n"  // w24: res
            "mov        x22, #4\n"

            "9:\n"
            "ldrh       w23, [x8], #2\n"
            "sub        w11, w23, #48\n"  // between 0~9?
            "cmp        w11, #10\n"
            "b.hs       10f\n"
            "add        w24, w11, w24, lsl #4\n"
            "subs       x22, x22, #1\n"
            "b.ne       9b\n"
            "b          14f\n"

            "10:\n"
            "sub        w11, w23, #97\n"  // between a~f?
            "cmp        w11, #6\n"
            "b.hs       12f\n"
            "11:\n"  // proc_a_to_f
            "sub        w23, w23, #87\n"
            "add        w24, w23, w24, lsl #4\n"
            "subs       x22, x22, #1\n"
            "b.ne       9b\n"
            "b          14f\n"

            "12:\n"                       // not_a_to_f
            "sub        w11, w23, #65\n"  // between A~F?
            "cmp        w11, #6\n"
            "b.hs       15f\n"
            "13:\n"  // proc_A_to_F
            "sub        w23, w23, #55\n"
            "add        w24, w23, w24, lsl #4\n"
            "subs       x22, x22, #1\n"
            "b.ne       9b\n"

            "14:\n"  // case_u_str
            "strb       w24, [x21], #1\n"
            "subs       x10, x10, #4\n"
            "b.eq       16f\n"
            "b          1b\n"

            "15:\n"                         // unreachable_
            "16:\n"                         // final_
            "mov        %[dst_ptr], x21\n"  // destination pointer
            "mov        %[cur], x8\n"

            : [dst_ptr] "+r"(p), [cur] "+r"(current_)
            : [end] "r"(end_)
            : "memory", "cc", "x8", "x9", "x10", "x11", "x21", "x22", "x23", "x24",
            "v0", "v1", "v2", "v3", "w4"
        );
    } else {
        static_assert(sizeof(T) == sizeof(uint8_t), "T must be 1 or 2 bytes in size");
        asm volatile(
            "mov        x21, %[dst_ptr]\n"  // destination pointer
            "mov        x8, %[cur]\n"
            "mov        x9, %[end]\n"
            "sub        x10, x9, x8\n"  // x10: byte difference
            "add        x10, x10, #1\n"
            "movi       v1.16b, #92\n"

            "1:\n"  // loop_entry
            "cmp        x10, #16\n"
            "b.lo       2f\n"
            "ld1        {v0.16b}, [x8]\n"
            "cmeq       v2.16b, v1.16b, v0.16b\n"
            "umaxv      b3, v2.16b\n"
            "fmov       w4, s3\n"
            "cbnz       w4, 2f\n"
            "add        x8, x8, #16\n"
            "st1        {v0.16b}, [x21], #16\n"
            "sub        x10, x10, #16\n"
            "cbz        x10, 16f\n"
            "b          1b\n"

            "2:\n"  // regv_slash_proc: at least one backslash or end terminator in 16b starting from w8
            "ldrb       w23, [x8], #1\n"
            "cmp        w23, #92\n"
            "b.eq       3f\n"  // backslash found
            "strb       w23, [x21], #1\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          2b\n"

            "3:\n"                       // slash_proc
            "sub        x10, x10, #1\n"  // backslash recognized, x10-1
            "ldrb       w23, [x8], #1\n"
            "sub        w22, w23, #34\n"
            "cmp        w22, #58\n"
            "b.hi       4f\n"              // not_34_to_92
            "strb       w23, [x21], #1\n"  // if 34 47 92, copy directly
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"

            "4:\n"  // not_34_to_92
            "sub        w22, w23, #98\n"
            "cmp        w22, #4\n"
            "b.hi       5f\n"
            "add        w23, w22, #8\n"    // if 98 102, subtract 90 and convert then copy
            "strb       w23, [x21], #1\n"  // if 34 47 92, copy directly
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"

            "5:\n"  // not_34_to_92
            "cmp        w23, #110\n"
            "b.ne       6f\n"
            "mov        w23, #10\n"  // 110->10
            "strb       w23, [x21], #1\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"
            "6:\n"
            "cmp        w23, #114\n"
            "b.ne       7f\n"  // 114->13
            "mov        w23, #13\n"
            "strb       w23, [x21], #1\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"
            "7:\n"
            "cmp        w23, #116\n"
            "b.ne       8f\n"
            "mov        w23, #9\n"  // 116->9
            "strb       w23, [x21], #1\n"
            "subs       x10, x10, #1\n"
            "b.eq       16f\n"
            "b          1b\n"

            "8:\n"
            "cmp        w23, #117\n"  // if w23 is u
            "b.ne       15f\n"
            "sub        x10, x10, #1\n"
            "mov        w24, #0\n"  // w24: res
            "mov        x22, #4\n"

            "9:\n"
            "ldrb       w23, [x8], #1\n"
            "sub        w11, w23, #48\n"  // between 0~9?
            "cmp        w11, #10\n"
            "b.hs       10f\n"
            "add        w24, w11, w24, lsl #4\n"
            "subs       x22, x22, #1\n"
            "b.ne       9b\n"
            "b          14f\n"

            "10:\n"
            "sub        w11, w23, #97\n"  // between a~f?
            "cmp        w11, #6\n"
            "b.hs       12f\n"
            "11:\n"  // proc_a_to_f
            "sub        w23, w23, #87\n"
            "add        w24, w23, w24, lsl #4\n"
            "subs       x22, x22, #1\n"
            "b.ne       9b\n"
            "b          14f\n"

            "12:\n"                       // not_a_to_f
            "sub        w11, w23, #65\n"  // between A~F?
            "cmp        w11, #6\n"
            "b.hs       15f\n"
            "13:\n"  // proc_A_to_F
            "sub        w23, w23, #55\n"
            "add        w24, w23, w24, lsl #4\n"
            "subs       x22, x22, #1\n"
            "b.ne       9b\n"

            "14:\n"  // case_u_str
            "strb       w24, [x21], #1\n"
            "subs       x10, x10, #4\n"
            "b.eq       16f\n"
            "b          1b\n"

            "15:\n"                         // unreachable_
            "16:\n"                         // final_
            "mov        %[dst_ptr], x21\n"  // destination pointer
            "mov        %[cur], x8\n"

            : [dst_ptr] "+r"(p), [cur] "+r"(current_)
            : [end] "r"(end_)
            : "memory", "cc", "x8", "x9", "x10", "x11", "x21", "x22", "x23",
            "x24", "v0", "v1", "v2", "v3", "w4"
        );
    }
}

#ifdef __cplusplus
}
#endif
#endif
