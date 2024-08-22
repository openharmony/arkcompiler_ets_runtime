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
    V(JSCALL)                               \
    V(JSCALL_FAST)                          \
    V(JSCALLTHIS)                           \
    V(JSCALLTHIS_FAST)                      \
    V(JSCALLTHIS_NOGC)                      \
    V(JSCALLTHIS_FAST_NOGC)                 \
    V(JS_NEWOBJRANGE)

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

class NewConstructMetaData : public OneParameterMetaData {
public:
    static constexpr int NEED_PUSH_ARGV_BIT_SIZE = 1;
    NewConstructMetaData(OpCode opcode, GateFlags flags, uint32_t statesIn,
        uint16_t dependsIn, uint32_t valuesIn, uint64_t value, bool needPushArgv)
        : OneParameterMetaData(opcode, flags, statesIn, dependsIn, valuesIn, value)
    {
        bitField_ = NeedPushArgvBit::Encode(needPushArgv);
    }
    
    static const NewConstructMetaData* Cast(const GateMetaData* meta)
    {
        meta->AssertKind(GateMetaData::Kind::CALL_NEW);
        return static_cast<const NewConstructMetaData*>(meta);
    }

    bool NeedPushArgv() const
    {
        return NeedPushArgvBit::Get(bitField_);
    }

    uint64_t GetValue() const
    {
        return bitField_;
    }

private:
    using NeedPushArgvBit = panda::BitField<bool, 0, NEED_PUSH_ARGV_BIT_SIZE>;

    uint64_t bitField_;
};

class TypedUnaryAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    explicit TypedUnaryAccessor(uint64_t value) : bitField_(value) {}

    ParamType GetParamType() const
    {
        return ParamType(TypedValueBits::Get(bitField_));
    }

    TypedUnOp GetTypedUnOp() const
    {
        return TypedUnOpBits::Get(bitField_);
    }

    static uint64_t ToValue(ParamType paramType, TypedUnOp unaryOp)
    {
        return TypedValueBits::Encode(paramType.Value()) | TypedUnOpBits::Encode(unaryOp);
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

    ParamType GetParamType() const
    {
        return ParamType(TypedValueBits::Get(bitField_));
    }

    TypedBinOp GetTypedBinOp() const
    {
        return TypedBinOpBits::Get(bitField_);
    }

    static uint64_t ToValue(ParamType operandType, TypedBinOp binOp)
    {
        return TypedValueBits::Encode(operandType.Value()) | TypedBinOpBits::Encode(binOp);
    }

private:
    using TypedValueBits = panda::BitField<uint32_t, 0, OPRAND_TYPE_BITS>;
    using TypedBinOpBits = TypedValueBits::NextField<TypedBinOp, OPRAND_TYPE_BITS>;

    uint64_t bitField_;
};

class TypedCallTargetCheckAccessor {
public:
    // type bits shift
    static constexpr int CALLTARGETCHECK_OP_BITS = 32;
    explicit TypedCallTargetCheckAccessor(uint64_t value) : bitField_(value) {}

    TypedCallTargetCheckOp GetCallTargetCheckOp() const
    {
        return CallTargetCheckOpBits::Get(bitField_);
    }

    static uint64_t ToValue(TypedCallTargetCheckOp op)
    {
        return CallTargetCheckOpBits::Encode(op);
    }

private:
    using CallTargetCheckOpBits = panda::BitField<TypedCallTargetCheckOp, 0, CALLTARGETCHECK_OP_BITS>;

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

class MemoryAttribute {
public:
    MemoryAttribute() = default;
    ~MemoryAttribute() = default;
    explicit MemoryAttribute(uint32_t v) : value_(v) {}

    enum Order {
        NOT_ATOMIC = 0,
        MEMORY_ORDER_RELEASE
    };

    enum Barrier {
        NEED_BARRIER = 0,
        NO_BARRIER,
        UNKNOWN_BARRIER
    };

    enum ShareFlag {
        UNKNOWN = 0,
        NON_SHARE,
        SHARED
    };

    static MemoryAttribute Default()
    {
        return Create(NOT_ATOMIC);
    }

    static MemoryAttribute NeedBarrier()
    {
        return Create(NOT_ATOMIC, NEED_BARRIER);
    }

    static MemoryAttribute UnknownBarrier()
    {
        return Create(NOT_ATOMIC, UNKNOWN_BARRIER);
    }

    static MemoryAttribute DefaultWithShareBarrier()
    {
        return Create(NOT_ATOMIC, UNKNOWN_BARRIER, SHARED);
    }

    static MemoryAttribute NeedNotShareBarrier()
    {
        return Create(NOT_ATOMIC, NEED_BARRIER, NON_SHARE);
    }

    static MemoryAttribute NeedBarrierAndAtomic()
    {
        return Create(MEMORY_ORDER_RELEASE, NEED_BARRIER);
    }

    static MemoryAttribute NoBarrier()
    {
        return Create(NOT_ATOMIC, NO_BARRIER);
    }

    void SetBarrier(Barrier barrier)
    {
        BarrierField::Set<uint32_t>(barrier, &value_);
    }

    Barrier GetBarrier() const
    {
        return BarrierField::Get(value_);
    }

    void SetShare(ShareFlag share)
    {
        ShareField::Set<uint32_t>(share, &value_);
    }

    ShareFlag GetShare() const
    {
        return ShareField::Get(value_);
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
    static MemoryAttribute Create(Order order, Barrier barrier = UNKNOWN_BARRIER, ShareFlag share = UNKNOWN)
    {
        uint32_t value = OrderField::Encode(order) | BarrierField::Encode(barrier) | ShareField::Encode(share);
        return MemoryAttribute(value);
    }

    static constexpr uint32_t ORDER_BITS = 8;
    static constexpr uint32_t BARRIER_BITS = 8;
    static constexpr uint32_t SHARE_BITS = 8;
    using OrderField = panda::BitField<Order, 0, ORDER_BITS>;
    using BarrierField = OrderField::NextField<Barrier, BARRIER_BITS>;
    using ShareField = BarrierField::NextField<ShareFlag, SHARE_BITS>;

    uint32_t value_;
};

class LoadStoreAccessor {
public:
    static constexpr int MEMORY_ORDER_BITS = 32;
    explicit LoadStoreAccessor(uint64_t value) : bitField_(value) {}

    MemoryAttribute GetMemoryAttribute() const
    {
        return MemoryAttribute(MemoryAttributeBits::Get(bitField_));
    }

    static uint64_t ToValue(MemoryAttribute mAttr)
    {
        return MemoryAttributeBits::Encode(mAttr.Value());
    }
private:
    using MemoryAttributeBits = panda::BitField<uint32_t, 0, MEMORY_ORDER_BITS>;

    uint64_t bitField_;
};

class LoadStoreConstOffsetAccessor {
public:
    static constexpr int OPRAND_OFFSET_BITS = 32;
    static constexpr int MEMORY_ATTRIBUTE_BITS = 32;
    explicit LoadStoreConstOffsetAccessor(uint64_t value) : bitField_(value) {}

    MemoryAttribute GetMemoryAttribute() const
    {
        return MemoryAttribute(MemoryAttributeBits::Get(bitField_));
    }

    size_t GetOffset() const
    {
        return static_cast<size_t>(OprandOffsetBits::Get(bitField_));
    }

    static uint64_t ToValue(size_t offset, MemoryAttribute mAttr)
    {
        return OprandOffsetBits::Encode(static_cast<uint32_t>(offset)) |
               MemoryAttributeBits::Encode(mAttr.Value());
    }
private:
    using OprandOffsetBits = panda::BitField<uint32_t, 0, OPRAND_OFFSET_BITS>;
    using MemoryAttributeBits = OprandOffsetBits::NextField<uint32_t, MEMORY_ATTRIBUTE_BITS>;

    uint64_t bitField_;
};

class TypedJumpAccessor {
public:
    // type bits shift
    static constexpr int OPRAND_TYPE_BITS = 32;
    static constexpr int JUMP_OP_BITS = 8;
    explicit TypedJumpAccessor(uint64_t value) : bitField_(value) {}

    ParamType GetParamType() const
    {
        return ParamType(TypedValueBits::Get(bitField_));
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

    static uint64_t ToValue(ParamType paramType, TypedJumpOp jumpOp, uint32_t weight)
    {
        return TypedValueBits::Encode(paramType.Value())
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
