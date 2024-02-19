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

#define TYPED_BIN_OP_LIST(V)    \
    V(TYPED_ADD)                \
    V(TYPED_SUB)                \
    V(TYPED_MUL)                \
    V(TYPED_DIV)                \
    V(TYPED_MOD)                \
    V(TYPED_LESS)               \
    V(TYPED_LESSEQ)             \
    V(TYPED_GREATER)            \
    V(TYPED_GREATEREQ)          \
    V(TYPED_EQ)                 \
    V(TYPED_NOTEQ)              \
    V(TYPED_STRICTEQ)           \
    V(TYPED_STRICTNOTEQ)        \
    V(TYPED_SHL)                \
    V(TYPED_SHR)                \
    V(TYPED_ASHR)               \
    V(TYPED_AND)                \
    V(TYPED_OR)                 \
    V(TYPED_XOR)                \
    V(TYPED_EXP)

#define TYPED_UN_OP_LIST(V) \
    V(TYPED_NEG)            \
    V(TYPED_NOT)            \
    V(TYPED_INC)            \
    V(TYPED_DEC)            \
    V(TYPED_ISFALSE)        \
    V(TYPED_ISTRUE)

#define TYPED_JUMP_OP_LIST(V)   \
    V(TYPED_JEQZ)               \
    V(TYPED_JNEZ)

#define TYPED_LOAD_OP_LIST(V)           \
    V(ARRAY_LOAD_INT_ELEMENT)           \
    V(ARRAY_LOAD_DOUBLE_ELEMENT)        \
    V(ARRAY_LOAD_OBJECT_ELEMENT)        \
    V(ARRAY_LOAD_TAGGED_ELEMENT)        \
    V(ARRAY_LOAD_HOLE_TAGGED_ELEMENT)   \
    V(ARRAY_LOAD_HOLE_INT_ELEMENT)      \
    V(ARRAY_LOAD_HOLE_DOUBLE_ELEMENT)   \
    V(INT8ARRAY_LOAD_ELEMENT)           \
    V(UINT8ARRAY_LOAD_ELEMENT)          \
    V(UINT8CLAMPEDARRAY_LOAD_ELEMENT)   \
    V(INT16ARRAY_LOAD_ELEMENT)          \
    V(UINT16ARRAY_LOAD_ELEMENT)         \
    V(INT32ARRAY_LOAD_ELEMENT)          \
    V(UINT32ARRAY_LOAD_ELEMENT)         \
    V(FLOAT32ARRAY_LOAD_ELEMENT)        \
    V(FLOAT64ARRAY_LOAD_ELEMENT)        \
    V(STRING_LOAD_ELEMENT)

#define TYPED_STORE_OP_LIST(V)          \
    V(ARRAY_STORE_ELEMENT)              \
    V(ARRAY_STORE_INT_ELEMENT)          \
    V(ARRAY_STORE_DOUBLE_ELEMENT)       \
    V(INT8ARRAY_STORE_ELEMENT)          \
    V(UINT8ARRAY_STORE_ELEMENT)         \
    V(UINT8CLAMPEDARRAY_STORE_ELEMENT)  \
    V(INT16ARRAY_STORE_ELEMENT)         \
    V(UINT16ARRAY_STORE_ELEMENT)        \
    V(INT32ARRAY_STORE_ELEMENT)         \
    V(UINT32ARRAY_STORE_ELEMENT)        \
    V(FLOAT32ARRAY_STORE_ELEMENT)       \
    V(FLOAT64ARRAY_STORE_ELEMENT)

#define TYPED_CALL_TARGET_CHECK_OP_LIST(V)  \
    V(JSCALL_IMMEDIATE_AFTER_FUNC_DEF)      \
    V(JSCALL)                               \
    V(JSCALL_FAST)                          \
    V(JSCALLTHIS)                           \
    V(JSCALLTHIS_FAST)                      \
    V(JSCALLTHIS_NOGC)                      \
    V(JSCALLTHIS_FAST_NOGC)

enum class TypedBinOp : uint8_t {
#define DECLARE_TYPED_BIN_OP(OP) OP,
    TYPED_BIN_OP_LIST(DECLARE_TYPED_BIN_OP)
#undef DECLARE_TYPED_BIN_OP
};

enum class TypedUnOp : uint8_t {
#define DECLARE_TYPED_UN_OP(OP) OP,
    TYPED_UN_OP_LIST(DECLARE_TYPED_UN_OP)
#undef DECLARE_TYPED_UN_OP
};

enum class TypedJumpOp : uint8_t {
#define DECLARE_TYPED_JUMP_OP(OP) OP,
    TYPED_JUMP_OP_LIST(DECLARE_TYPED_JUMP_OP)
#undef DECLARE_TYPED_JUMP_OP
};

enum class TypedLoadOp : uint8_t {
#define DECLARE_TYPED_LOAD_OP(OP) OP,
    TYPED_LOAD_OP_LIST(DECLARE_TYPED_LOAD_OP)
#undef DECLARE_TYPED_LOAD_OP
    TYPED_ARRAY_FIRST = INT8ARRAY_LOAD_ELEMENT,
    TYPED_ARRAY_LAST = FLOAT64ARRAY_LOAD_ELEMENT,
};

enum class TypedStoreOp : uint8_t {
#define DECLARE_TYPED_STORE_OP(OP) OP,
    TYPED_STORE_OP_LIST(DECLARE_TYPED_STORE_OP)
#undef DECLARE_TYPED_STORE_OP
    TYPED_ARRAY_FIRST = INT8ARRAY_STORE_ELEMENT,
    TYPED_ARRAY_LAST = FLOAT64ARRAY_STORE_ELEMENT,
};

enum class TypedCallTargetCheckOp : uint8_t {
#define DECLARE_TYPED_CALL_TARGET_CHECK_OP(OP) OP,
    TYPED_CALL_TARGET_CHECK_OP_LIST(DECLARE_TYPED_CALL_TARGET_CHECK_OP)
#undef DECLARE_TYPED_CALL_TARGET_CHECK_OP
};

enum class BranchKind : uint8_t {
    NORMAL_BRANCH = 0,
    TRUE_BRANCH,
    FALSE_BRANCH,
    STRONG_TRUE_BRANCH,
    STRONG_FALSE_BRANCH,
};

enum class TypedOpKind : uint8_t {
    TYPED_BIN_OP,
    TYPED_CALL_TARGET_CHECK_OP,
    TYPED_UN_OP,
    TYPED_JUMP_OP,
    TYPED_STORE_OP,
    TYPED_LOAD_OP,
};

enum class MemoryType : uint8_t {
    ELEMENT_TYPE = 0,
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

    bool equal(const GateMetaData &other) const override
    {
        if (!OneParameterMetaData::equal(other)) {
            return false;
        }
        auto cast_other = static_cast<const TypedCallMetaData *>(&other);
        if (noGC_ == cast_other->noGC_) {
            return true;
        }
        return false;
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

    bool equal(const GateMetaData &other) const override
    {
        if (!OneParameterMetaData::equal(other)) {
            return false;
        }
        auto cast_other =
            static_cast<const TypedCallTargetCheckMetaData *>(&other);
        if (checkOp_ == cast_other->checkOp_) {
            return true;
        }
        return false;
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
    TypedBinaryMetaData(uint64_t value, TypedBinOp binOp, PGOTypeRef type)
        : OneParameterMetaData(OpCode::TYPED_BINARY_OP, GateFlags::NO_WRITE, 1, 1, 2, value), // 2: valuesIn
        binOp_(binOp), type_(type)
    {
        SetKind(GateMetaData::Kind::TYPED_BINARY_OP);
    }

    bool equal(const GateMetaData &other) const override
    {
        if (!OneParameterMetaData::equal(other)) {
            return false;
        }
        auto cast_other = static_cast<const TypedBinaryMetaData *>(&other);
        if (binOp_ == cast_other->binOp_ && type_ == cast_other->type_) {
            return true;
        }
        return false;
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

    PGOTypeRef GetType() const
    {
        return type_;
    }

    std::string Str() const
    {
        return GateMetaData::Str(binOp_);
    }

    static TypedBinOp GetRevCompareOp(TypedBinOp op);
    static TypedBinOp GetSwapCompareOp(TypedBinOp op);
private:
    TypedBinOp binOp_;
    PGOTypeRef type_;
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

class MemoryOrder {
public:
    MemoryOrder() = default;
    ~MemoryOrder() = default;
    explicit MemoryOrder(uint32_t v) : value_(v) {}

    enum Order {
        NOT_ATOMIC = 0,
        MEMORY_ORDER_RELEASE
    };

    enum Barrier {
        NEED_BARRIER = 0,
        NO_BARRIER
    };

    static MemoryOrder Default()
    {
        return Create(NOT_ATOMIC);
    }

    static MemoryOrder Create(Order order, Barrier barrier = NO_BARRIER)
    {
        uint32_t value = OrderField::Encode(order) | BarrierField::Encode(barrier);
        return MemoryOrder(value);
    }

    void SetBarrier(Barrier barrier)
    {
        BarrierField::Set<uint32_t>(barrier, &value_);
    }

    Barrier GetBarrier() const
    {
        return BarrierField::Get(value_);
    }

    void SetOrder(Order order)
    {
        OrderField::Set<uint32_t>(order, &value_);
    }

    Order GetOrder() const
    {
        return OrderField::Get(value_);
    }

    uint32_t Value() const
    {
        return value_;
    }

private:
    static constexpr uint32_t ORDER_BITS = 8;
    static constexpr uint32_t BARRIER_BITS = 8;
    using OrderField = panda::BitField<Order, 0, ORDER_BITS>;
    using BarrierField = OrderField::NextField<Barrier, BARRIER_BITS>;

    uint32_t value_;
};

class LoadStoreAccessor {
public:
    static constexpr int MEMORY_ORDER_BITS = 32;
    explicit LoadStoreAccessor(uint64_t value) : bitField_(value) {}

    MemoryOrder GetMemoryOrder() const
    {
        return MemoryOrder(MemoryOrderBits::Get(bitField_));
    }

    static uint64_t ToValue(MemoryOrder order)
    {
        return MemoryOrderBits::Encode(order.Value());
    }
private:
    using MemoryOrderBits = panda::BitField<uint32_t, 0, MEMORY_ORDER_BITS>;

    uint64_t bitField_;
};

class LoadStoreConstOffsetAccessor {
public:
    static constexpr int OPRAND_OFFSET_BITS = 32;
    static constexpr int MEMORY_ORDER_BITS = 32;
    explicit LoadStoreConstOffsetAccessor(uint64_t value) : bitField_(value) {}

    MemoryOrder GetMemoryOrder() const
    {
        return MemoryOrder(MemoryOrderBits::Get(bitField_));
    }

    size_t GetOffset() const
    {
        return static_cast<size_t>(OprandOffsetBits::Get(bitField_));
    }

    static uint64_t ToValue(size_t offset, MemoryOrder order)
    {
        return OprandOffsetBits::Encode(static_cast<uint32_t>(offset)) |
               MemoryOrderBits::Encode(order.Value());
    }
private:
    using OprandOffsetBits = panda::BitField<uint32_t, 0, OPRAND_OFFSET_BITS>;
    using MemoryOrderBits = OprandOffsetBits::NextField<uint32_t, MEMORY_ORDER_BITS>;

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
