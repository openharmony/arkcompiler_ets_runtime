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
#include "aarch64_imm_valid.h"
#include <iomanip>
#include <set>

namespace maplebe {
const std::set<uint64> ValidBitmaskImmSet = {
#include "valid_bitmask_imm.txt"
};
constexpr uint32 kMaxBitTableSize = 5;
constexpr std::array<uint64, kMaxBitTableSize> kBitmaskImmMultTable = {
    0x0000000100000001UL, 0x0001000100010001UL, 0x0101010101010101UL, 0x1111111111111111UL, 0x5555555555555555UL,
};
namespace aarch64 {
bool IsBitmaskImmediate(uint64 val, uint32 bitLen)
{
    DEBUG_ASSERT(val != 0, "IsBitmaskImmediate() don's accept 0 or -1");
    DEBUG_ASSERT(static_cast<int64>(val) != -1, "IsBitmaskImmediate() don's accept 0 or -1");
    if ((bitLen == k32BitSize) && (static_cast<int32>(val) == -1)) {
        return false;
    }
    uint64 val2 = val;
    if (bitLen == k32BitSize) {
        val2 = (val2 << k32BitSize) | (val2 & ((1ULL << k32BitSize) - 1));
    }
    bool expectedOutcome = (ValidBitmaskImmSet.find(val2) != ValidBitmaskImmSet.end());

    if ((val & 0x1) != 0) {
        /*
         * we want to work with
         * 0000000000000000000000000000000000000000000001100000000000000000
         * instead of
         * 1111111111111111111111111111111111111111111110011111111111111111
         */
        val = ~val;
    }

    if (bitLen == k32BitSize) {
        val = (val << k32BitSize) | (val & ((1ULL << k32BitSize) - 1));
    }

    /* get the least significant bit set and add it to 'val' */
    uint64 tmpVal = val + (val & static_cast<uint64>(UINT64_MAX - val + 1));

    /* now check if tmp is a power of 2 or tmpVal==0. */
    DEBUG_ASSERT(tmpVal >= 0, "tmpVal -1 should be unsigned");
    tmpVal = tmpVal & (tmpVal - 1);
    if (tmpVal == 0) {
        if (!expectedOutcome) {
#if defined(DEBUG) && DEBUG
            LogInfo::MapleLogger() << "0x" << std::hex << std::setw(static_cast<int>(k16ByteSize)) << std::setfill('0')
                                   << static_cast<uint64>(val) << "\n";
#endif
            return false;
        }
        DEBUG_ASSERT(expectedOutcome, "incorrect implementation: not valid value but returning true");
        /* power of two or zero ; return true */
        return true;
    }

    int32 p0 = __builtin_ctzll(val);
    int32 p1 = __builtin_ctzll(tmpVal);
    int64 diff = p1 - p0;

    /* check if diff is a power of two; return false if not. */
    CHECK_FATAL(static_cast<uint64>(diff) >= 1, "value overflow");
    if ((static_cast<uint64>(diff) & (static_cast<uint64>(diff) - 1)) != 0) {
        DEBUG_ASSERT(!expectedOutcome, "incorrect implementation: valid value but returning false");
        return false;
    }

    uint32 logDiff = static_cast<uint32>(__builtin_ctzll(static_cast<uint64>(diff)));
    uint64 pattern = val & ((1ULL << static_cast<uint64>(diff)) - 1);
#if defined(DEBUG) && DEBUG
    bool ret = (val == pattern * kBitmaskImmMultTable[kMaxBitTableSize - logDiff]);
    DEBUG_ASSERT(expectedOutcome == ret, "incorrect implementation: return value does not match expected outcome");
    return ret;
#else
    return val == pattern * kBitmaskImmMultTable[kMaxBitTableSize - logDiff];
#endif
}
}  // namespace aarch64
}  // namespace maplebe
