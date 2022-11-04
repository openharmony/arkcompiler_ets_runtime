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

#ifndef ECMASCRIPT_COMPILER_GATE_BITFIELD_ACCESSOR_H
#define ECMASCRIPT_COMPILER_GATE_BITFIELD_ACCESSOR_H

#include "libpandabase/utils/bit_field.h"

namespace panda::ecmascript::kungfu {
using BitField = uint64_t;
class GateBitFieldAccessor {
public:
    static constexpr unsigned START_BIT = 32;
    static constexpr unsigned MASK = (1LLU << START_BIT) - 1; // 1: mask

    static uint64_t GetStateCount(BitField bitfield)
    {
        return bitfield;
    }

    static uint64_t GetDependCount(BitField bitfield)
    {
        return bitfield;
    }

    static uint32_t GetInValueCount(BitField bitfield)
    {
        return static_cast<uint32_t>(bitfield & MASK);
    }

    static uint32_t GetBytecodeIndex(BitField bitfield)
    {
        return static_cast<uint32_t>(bitfield >> START_BIT);
    }

    // 32 bit inValue, 32 bit bcIndex
    static BitField ConstructJSBytecode(uint32_t inValue, uint32_t bcIndex)
    {
        return (static_cast<uint64_t>(bcIndex) << START_BIT) | inValue;
    }
};

}
#endif  // ECMASCRIPT_COMPILER_GATE_BITFIELD_ACCESSOR_H
