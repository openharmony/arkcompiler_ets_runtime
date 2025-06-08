/*
 * Copyright (c) 2023 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_JS_TAGGED_VALUE_INTERNALS_H
#define ECMASCRIPT_JS_TAGGED_VALUE_INTERNALS_H

#include "ecmascript/base/bit_helper.h"

namespace panda::ecmascript {

using JSTaggedType = uint64_t;

//  Every double with all of its exponent bits set and its highest mantissa bit set is a quiet NaN.
//  That leaves 51 bits unaccounted for. We’ll avoid one of those so that we don’t step on Intel’s
//  “QNaN Floating-Point Indefinite” value, leaving us 50 bits. Those remaining bits can be anything.
//  so we use a special quietNaN as TaggedInt tag(highest 16bits as 0xFFFF), and need to encode double
//  to the value will begin with a 16-bit pattern within the range 0x0001..0xFFFE.

//  Nan-boxing pointer is used and the first four bytes are used as tag:
//    Object:             [0x0000] [48 bit direct pointer]
//    WeakRef:            [0x0000] [47 bits direct pointer] | 1
//                   /    [0x0001] [48 bit any value]
//    TaggedDouble:       ......
//                   \    [0xFFFE] [48 bit any value]
//    TaggedInt:          [0xFFFF] [0x0000] [32 bit signed integer]
//
//  There are some special markers of Object:
//    False:       [56 bits 0] | 0x06          // 0110
//    True:        [56 bits 0] | 0x07          // 0111
//    Undefined:   [56 bits 0] | 0x02          // 0010
//    Null:        [56 bits 0] | 0x03          // 0011
//    Hole:        [56 bits 0] | 0x05          // 0101
//    Optimized:   [56 bits 0] | 0x0C          // 1100

class JSTaggedValueInternals {
public:
    static constexpr size_t BIT_PER_BYTE = 8;
    static constexpr size_t TAG_BITS_SIZE = 16;  // 16 means bit numbers of 0xFFFF
    static constexpr size_t TAG_BITS_SHIFT = base::BitNumbers<JSTaggedType>() - TAG_BITS_SIZE;
    static_assert((TAG_BITS_SHIFT + TAG_BITS_SIZE) == sizeof(JSTaggedType) * BIT_PER_BYTE, "Insufficient bits!");
    static constexpr JSTaggedType TAG_MARK = 0xFFFFULL << TAG_BITS_SHIFT;
    // int tag
    static constexpr JSTaggedType TAG_INT = TAG_MARK;
    static constexpr JSTaggedType TAG_INT32_INC_MAX = INT32_MAX + 1ULL;
    static constexpr JSTaggedType TAG_INT32_DEC_MIN = INT32_MIN - 1ULL;
    // object tag
    static constexpr JSTaggedType TAG_OBJECT = 0x0000ULL << TAG_BITS_SHIFT;
    // weak object tag
    static constexpr JSTaggedType TAG_WEAK = TAG_OBJECT | 0x01ULL;
    // special tag
    static constexpr JSTaggedType TAG_NULL = 0x01ULL;
    static constexpr JSTaggedType TAG_SPECIAL = 0x02ULL;
    static constexpr JSTaggedType TAG_BOOLEAN = 0x04ULL;
    static constexpr JSTaggedType TAG_EXCEPTION = 0x08ULL;
    static constexpr JSTaggedType TAG_OPTIMIZED_OUT = 0x12ULL;
    // tag mask
    static constexpr JSTaggedType TAG_SPECIAL_MASK = TAG_MARK | TAG_SPECIAL;
    static constexpr JSTaggedType TAG_BOOLEAN_MASK = TAG_SPECIAL | TAG_BOOLEAN;
    static constexpr JSTaggedType TAG_HEAPOBJECT_MASK = TAG_MARK | TAG_SPECIAL | TAG_BOOLEAN;
    static constexpr JSTaggedType TAG_WEAK_MASK = TAG_HEAPOBJECT_MASK | TAG_WEAK;
    // special value
    static constexpr JSTaggedType VALUE_HOLE = 0x05ULL;
    static constexpr JSTaggedType VALUE_NULL = TAG_OBJECT | TAG_SPECIAL | TAG_NULL;
    static constexpr JSTaggedType VALUE_FALSE = TAG_BOOLEAN_MASK | static_cast<JSTaggedType>(false);
    static constexpr JSTaggedType VALUE_TRUE = TAG_BOOLEAN_MASK | static_cast<JSTaggedType>(true);
    static constexpr JSTaggedType VALUE_UNDEFINED = TAG_SPECIAL;
    static constexpr JSTaggedType VALUE_EXCEPTION = TAG_SPECIAL | TAG_EXCEPTION;
    static constexpr JSTaggedType VALUE_ZERO = TAG_INT | 0x00ULL;
    static constexpr JSTaggedType VALUE_OPTIMIZED_OUT = TAG_SPECIAL | TAG_OPTIMIZED_OUT;
    // ReadBarrier DFX tag
    static constexpr size_t TAG_READ_BARRIER_SHIFT = 47;
    static constexpr JSTaggedType TAG_READ_BARRIER_DFX = 0x1ULL << TAG_READ_BARRIER_SHIFT;
    static constexpr JSTaggedType TAG_READ_BARRIER_DFX_MASK = TAG_HEAPOBJECT_MASK | TAG_READ_BARRIER_DFX;

    static constexpr size_t INT_SIGN_BIT_OFFSET = 31;
    static constexpr size_t DOUBLE_ENCODE_OFFSET_BIT = 48;
    static constexpr JSTaggedType DOUBLE_ENCODE_OFFSET = 1ULL << DOUBLE_ENCODE_OFFSET_BIT;
    // Tagged +0.0 = IEEE754 representation of +0.0 + offset
    static constexpr JSTaggedType VALUE_POSITIVE_ZERO = 0x0000'0000'0000'0000uLL + DOUBLE_ENCODE_OFFSET;
    // Tagged -0.0 = IEEE754 representation of -0.0 + offset
    static constexpr JSTaggedType VALUE_NEGATIVE_ZERO = 0x8000'0000'0000'0000uLL + DOUBLE_ENCODE_OFFSET;
};
} // namespace panda::ecmascript
#endif // ECMASCRIPT_JS_TAGGED_VALUE_INTERNALS_H
