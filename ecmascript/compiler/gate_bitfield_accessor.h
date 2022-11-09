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
#include "libpandafile/bytecode_instruction-inl.h"

namespace panda::ecmascript::kungfu {
using BitField = uint64_t;
using EcmaOpcode = BytecodeInstruction::Opcode;

class GateBitFieldAccessor {
public:
    static constexpr unsigned IN_VALUE_NUM_BITS = 16;
    static constexpr unsigned OPCODE_NUM_BITS = 16;
    static constexpr unsigned BC_INDEX_NUM_BITS = 32;

    using InValueBits = panda::BitField<uint32_t, 0, IN_VALUE_NUM_BITS>;
    using OpcodeBits = InValueBits::NextField<EcmaOpcode, OPCODE_NUM_BITS>;
    using BcIndexBits = OpcodeBits::NextField<uint32_t, BC_INDEX_NUM_BITS>;


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
        return InValueBits::Decode(bitfield);
    }

    static uint32_t GetBytecodeIndex(BitField bitfield)
    {
        return BcIndexBits::Decode(bitfield);
    }

    static EcmaOpcode GetByteCodeOpcode(BitField bitfield)
    {
        return OpcodeBits::Decode(bitfield);
    }

    // 32 bit inValue, 32 bit bcIndex
    static BitField ConstructJSBytecode(uint16_t inValue, EcmaOpcode opcode, uint32_t bcIndex)
    {
        uint64_t value = InValueBits::Encode(inValue) | OpcodeBits::Encode(opcode) |
            BcIndexBits::Encode(bcIndex);
        return value;
    }

    static uint64_t GetConstantValue(BitField bitfield)
    {
        return bitfield;
    }
};

}
#endif  // ECMASCRIPT_COMPILER_GATE_BITFIELD_ACCESSOR_H
