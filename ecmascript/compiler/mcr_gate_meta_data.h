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

#ifndef ECMASCRIPT_COMPILER_MCR_GATE_META_DATA_H
#define ECMASCRIPT_COMPILER_MCR_GATE_META_DATA_H

#include <string>

#include "ecmascript/compiler/bytecodes.h"
#include "ecmascript/compiler/type.h"
#include "ecmascript/mem/chunk.h"
#include "ecmascript/mem/chunk_containers.h"

#include "ecmascript/elements.h"
#include "ecmascript/pgo_profiler/types/pgo_profiler_type.h"
#include "libpandabase/macros.h"

#include "ecmascript/compiler/share_gate_meta_data.h"

namespace panda::ecmascript::kungfu {

enum class BranchKind : uint8_t {
    NORMAL_BRANCH = 0,
    TRUE_BRANCH,
    FALSE_BRANCH,
    STRONG_TRUE_BRANCH,
    STRONG_FALSE_BRANCH,
};

enum class TypedBinOp : uint8_t {
    TYPED_ADD = 0,
    TYPED_SUB,
    TYPED_MUL,
    TYPED_DIV,
    TYPED_MOD,
    TYPED_LESS,
    TYPED_LESSEQ,
    TYPED_GREATER,
    TYPED_GREATEREQ,
    TYPED_EQ,
    TYPED_NOTEQ,
    TYPED_STRICTEQ,
    TYPED_SHL,
    TYPED_SHR,
    TYPED_ASHR,
    TYPED_AND,
    TYPED_OR,
    TYPED_XOR,
    TYPED_EXP,
};

enum class TypedOpKind : uint8_t {
    TYPED_BIN_OP,
    TYPED_CALL_TARGET_CHECK_OP,
    TYPED_UN_OP,
    TYPED_JUMP_OP,
    TYPED_STORE_OP,
    TYPED_LOAD_OP,
};

enum class TypedStoreOp : uint8_t {
    ARRAY_STORE_ELEMENT = 0,
    INT8ARRAY_STORE_ELEMENT,
    UINT8ARRAY_STORE_ELEMENT,
    UINT8CLAMPEDARRAY_STORE_ELEMENT,
    INT16ARRAY_STORE_ELEMENT,
    UINT16ARRAY_STORE_ELEMENT,
    INT32ARRAY_STORE_ELEMENT,
    UINT32ARRAY_STORE_ELEMENT,
    FLOAT32ARRAY_STORE_ELEMENT,
    FLOAT64ARRAY_STORE_ELEMENT,

    TYPED_ARRAY_FIRST = INT8ARRAY_STORE_ELEMENT,
    TYPED_ARRAY_LAST = FLOAT64ARRAY_STORE_ELEMENT,
};

enum class MemoryType : uint8_t {
    ELEMENT_TYPE = 0,
};

enum class TypedLoadOp : uint8_t {
    ARRAY_LOAD_INT_ELEMENT = 0,
    ARRAY_LOAD_DOUBLE_ELEMENT,
    ARRAY_LOAD_OBJECT_ELEMENT,
    ARRAY_LOAD_TAGGED_ELEMENT,
    ARRAY_LOAD_HOLE_TAGGED_ELEMENT,
    INT8ARRAY_LOAD_ELEMENT,
    UINT8ARRAY_LOAD_ELEMENT,
    UINT8CLAMPEDARRAY_LOAD_ELEMENT,
    INT16ARRAY_LOAD_ELEMENT,
    UINT16ARRAY_LOAD_ELEMENT,
    INT32ARRAY_LOAD_ELEMENT,
    UINT32ARRAY_LOAD_ELEMENT,
    FLOAT32ARRAY_LOAD_ELEMENT,
    FLOAT64ARRAY_LOAD_ELEMENT,
    STRING_LOAD_ELEMENT,

    TYPED_ARRAY_FIRST = INT8ARRAY_LOAD_ELEMENT,
    TYPED_ARRAY_LAST = FLOAT64ARRAY_LOAD_ELEMENT,
};

enum class TypedCallTargetCheckOp : uint8_t {
    JSCALL_IMMEDIATE_AFTER_FUNC_DEF = 0,
    JSCALL,
    JSCALL_FAST,
    JSCALLTHIS,
    JSCALLTHIS_FAST,
    JSCALLTHIS_NOGC,
    JSCALLTHIS_FAST_NOGC,
};

enum class TypedUnOp : uint8_t {
    TYPED_NEG = 0,
    TYPED_NOT,
    TYPED_INC,
    TYPED_DEC,
    TYPED_ISFALSE,
    TYPED_ISTRUE,
};

enum class TypedJumpOp : uint8_t {
    TYPED_JEQZ = 0,
    TYPED_JNEZ,
};

class TypedCallMetaData : public OneParameterMetaData {
public:
    TypedCallMetaData(OpCode opcode, GateFlags flags, uint32_t statesIn,
        uint16_t dependsIn, uint32_t valuesIn, uint64_t value, bool noGC)
        : OneParameterMetaData(opcode, flags, statesIn, dependsIn, valuesIn, value),
        noGC_(noGC)
    {
        SetKind(GateMetaData::Kind::TYPED_CALL);
    }

    static const TypedCallMetaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::Kind::TYPED_CALL);
        return static_cast<const TypedCallMetaData*>(meta);
    }

    bool IsNoGC() const
    {
        return noGC_;
    }
private:
    bool noGC_;
};

class TypedCallTargetCheckMetaData : public OneParameterMetaData {
public:
    TypedCallTargetCheckMetaData(uint32_t valuesIn, uint64_t value, TypedCallTargetCheckOp checkOp)
        : OneParameterMetaData(OpCode::TYPED_CALLTARGETCHECK_OP, GateFlags::CHECKABLE, 1, 1, valuesIn, value),
        checkOp_(checkOp)
    {
        SetKind(GateMetaData::Kind::TYPED_CALLTARGETCHECK_OP);
    }

    static const TypedCallTargetCheckMetaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::Kind::TYPED_CALLTARGETCHECK_OP);
        return static_cast<const TypedCallTargetCheckMetaData*>(meta);
    }

    TypedCallTargetCheckOp GetTypedCallTargetCheckOp() const
    {
        return checkOp_;
    }
private:
    TypedCallTargetCheckOp checkOp_;
};


class TypedBinaryMetaData : public OneParameterMetaData {
public:
    TypedBinaryMetaData(uint64_t value, TypedBinOp binOp, PGOSampleType type)
        : OneParameterMetaData(OpCode::TYPED_BINARY_OP, GateFlags::NO_WRITE, 1, 1, 2, value), // 2: valuesIn
        binOp_(binOp), type_(type)
    {
        SetKind(GateMetaData::Kind::TYPED_BINARY_OP);
    }

    static const TypedBinaryMetaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::Kind::TYPED_BINARY_OP);
        return static_cast<const TypedBinaryMetaData*>(meta);
    }

    TypedBinOp GetTypedBinaryOp() const
    {
        return binOp_;
    }

    PGOSampleType GetType() const
    {
        return type_;
    }

    static TypedBinOp GetRevCompareOp(TypedBinOp op);
    static TypedBinOp GetSwapCompareOp(TypedBinOp op);
private:
    TypedBinOp binOp_;
    PGOSampleType type_;
};

class TypedUnaryAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    explicit TypedUnaryAccessor(uint64_t value) : bitField_(value) {}

    GateType GetTypeValue() const
    {
        return GateType(TypedValueBits::Get(bitField_));
    }

    TypedUnOp GetTypedUnOp() const
    {
        return TypedUnOpBits::Get(bitField_);
    }

    static uint64_t ToValue(GateType typeValue, TypedUnOp unaryOp)
    {
        return TypedValueBits::Encode(typeValue.Value())
            | TypedUnOpBits::Encode(unaryOp);
    }

private:
    using TypedValueBits = panda::BitField<uint32_t, 0, OPRAND_TYPE_BITS>;
    using TypedUnOpBits = TypedValueBits::NextField<TypedUnOp, OPRAND_TYPE_BITS>;

    uint64_t bitField_;
};

class TypedBinaryAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    explicit TypedBinaryAccessor(uint64_t value) : bitField_(value) {}
    explicit TypedBinaryAccessor(GateType gate, TypedBinOp binOp)
    {
        bitField_ = TypedValueBits::Encode(gate.Value()) | TypedBinOpBits::Encode(binOp);
    }

    GateType GetTypeValue() const
    {
        return GateType(TypedValueBits::Get(bitField_));
    }

    TypedBinOp GetTypedBinOp() const
    {
        return TypedBinOpBits::Get(bitField_);
    }

    uint64_t ToValue() const
    {
        return bitField_;
    }

private:
    using TypedValueBits = panda::BitField<uint32_t, 0, OPRAND_TYPE_BITS>;
    using TypedBinOpBits = TypedValueBits::NextField<TypedBinOp, OPRAND_TYPE_BITS>;

    uint64_t bitField_;
};

class BranchAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    explicit BranchAccessor(uint64_t value) : bitField_(value) {}

    int32_t GetTrueWeight() const
    {
        return TrueWeightBits::Get(bitField_);
    }

    int32_t GetFalseWeight() const
    {
        return FalseWeightBits::Get(bitField_);
    }

    static uint64_t ToValue(uint32_t trueWeight, uint32_t falseWeight)
    {
        return TrueWeightBits::Encode(trueWeight)
            | FalseWeightBits::Encode(falseWeight);
    }
private:
    using TrueWeightBits = panda::BitField<uint32_t, 0, OPRAND_TYPE_BITS>;
    using FalseWeightBits = TrueWeightBits::NextField<uint32_t, OPRAND_TYPE_BITS>;

    uint64_t bitField_;
};

class TypedJumpAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    static constexpr int JUMP_OP_BITS = 8;
    explicit TypedJumpAccessor(uint64_t value) : bitField_(value) {}

    GateType GetTypeValue() const
    {
        return GateType(TypedValueBits::Get(bitField_));
    }

    TypedJumpOp GetTypedJumpOp() const
    {
        return TypedJumpOpBits::Get(bitField_);
    }

    uint32_t GetTrueWeight() const
    {
        return TrueWeightBits::Get(bitField_);
    }

    uint32_t GetFalseWeight() const
    {
        return FalseWeightBits::Get(bitField_);
    }

    static uint64_t ToValue(GateType typeValue, TypedJumpOp jumpOp, uint32_t weight)
    {
        return TypedValueBits::Encode(typeValue.Value())
            | TypedJumpOpBits::Encode(jumpOp)
            | WeightBits::Encode(weight);
    }

private:
    using TypedValueBits = panda::BitField<uint32_t, 0, OPRAND_TYPE_BITS>;
    using TypedJumpOpBits = TypedValueBits::NextField<TypedJumpOp, JUMP_OP_BITS>;
    using WeightBits = TypedJumpOpBits::NextField<uint32_t, PGOSampleType::WEIGHT_BITS + PGOSampleType::WEIGHT_BITS>;
    using FalseWeightBits = TypedJumpOpBits::NextField<uint32_t, PGOSampleType::WEIGHT_BITS>;
    using TrueWeightBits = FalseWeightBits::NextField<uint32_t, PGOSampleType::WEIGHT_BITS>;

    uint64_t bitField_;
};

}

#endif  // ECMASCRIPT_COMPILER_MCR_GATE_META_DATA_H
