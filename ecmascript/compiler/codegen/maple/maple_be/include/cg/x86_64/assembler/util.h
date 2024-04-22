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

#ifndef X64_ASSEMBLER_UTIL_H
#define X64_ASSEMBLER_UTIL_H

namespace assembler {
using uint8 = uint8_t;
using uint16 = uint16_t;
using uint32 = uint32_t;
using uint64 = uint64_t;
using int8 = int8_t;
using int16 = int16_t;
using int32 = int32_t;
using int64 = int64_t;
using uintptr = uintptr_t;

/* InsnSize is in byte. */
enum InsnSize : uint8 {
    kB = 1,
    kW = 2,
    kL = 4,
    kQ = 8,
};

/* size in bytes */
static const uint8 k1Byte = 1;
static const uint8 k2Bytes = 2;
static const uint8 k4Bytes = 4;
static const uint8 k8Bytes = 8;

/* size in bits */
static const uint8 k8Bits = 8;
static const uint8 k16Bits = 16;
static const uint8 k32Bits = 32;
static const uint8 k64Bits = 64;
static const uint8 k128Bits = 128;

inline bool Is8Bits(uint32 val)
{
    return val >= 0xFFFFFF80 || val <= 0x7F;
}

inline bool Is16Bits(uint32 val)
{
    return val >= 0xFFFF8000 || val <= 0x7FFF;
}

inline bool Is32Bits(uint64 val)
{
    return val >= ~uint64(0x7FFFFFFFU) || val <= 0x7FFFFFFFU;
}

inline bool Is64Bits(uint64 val)
{
    return val >= ~uint64(0x7FFFFFFFFFFFFFFFU) || val <= 0x7FFFFFFFFFFFFFFFU;
}

inline int64 CalculateLabelSymIdx(uint32 funcUniqueId, uint32 labelIdx)
{
    /* 32: make sure stIdx is large enough to be unique */
    const int kLeftShiftBits = 32;
    /* -1: BBLabel's stIdx is negative */
    return static_cast<int64>((static_cast<uint64>(funcUniqueId) << kLeftShiftBits) + labelIdx) * (-1);
}

inline int64 CalculateStrLabelSymIdx(uint64 size, int64 labelIdx, size_t strTableSize = 0)
{
    const int kLeftShiftBits = 8;
    return (static_cast<int64>(size + strTableSize) * kLeftShiftBits + labelIdx);
}
} /* namespace assembler */

#endif /* X64_ASSEMBLER_UTIL_H */