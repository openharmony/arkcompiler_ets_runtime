/*
 * Copyright (c) 2021 Huawei Device Co., Ltd.
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

#ifndef ECMASCRIPT_COMPILER_TYPE_H
#define ECMASCRIPT_COMPILER_TYPE_H

#include "ecmascript/base/config.h"
#include "ecmascript/ts_types/global_ts_type_ref.h"

#define VALUE_TYPE_LIST(V)  \
    V(BOOL)                 \
    V(INT32)                \
    V(UINT32)               \
    V(FLOAT64)              \
    V(TAGGED_BOOLEAN)       \
    V(TAGGED_INT)           \
    V(TAGGED_DOUBLE)        \
    V(TAGGED_NUMBER)        \
    V(CHAR)                 \
    V(ECMA_STRING)          \
    V(UNDEFINED)            \
    V(HOLE_INT)             \
    V(HOLE_DOUBLE)          \
    V(TAGGED_NULL)

enum class ValueType : uint8_t {
#define DECLARE_VALUE_TYPE(TYPE) TYPE,
    VALUE_TYPE_LIST(DECLARE_VALUE_TYPE)
#undef DECLARE_VALUE_TYPE
};

namespace panda::ecmascript::kungfu {
#define PRIMITIVE_TYPE_LIST(V)         \
    V(ANY, AnyType)                    \
    V(NUMBER, NumberType)              \
    V(BOOLEAN, BooleanType)            \
    V(VOID_TYPE, VoidType)             \
    V(STRING, StringType)              \
    V(INTERN_STRING, InternStringType) \
    V(SYMBOL, SymbolType)              \
    V(NULL_TYPE, NullType)             \
    V(UNDEFINED, UndefinedType)        \
    V(INT, IntType)                    \
    V(BIG_INT, BigIntType)             \
    V(DOUBLE, DoubleType)              \

// ParamType: Prediction type from PGO, bound to MetaData in GATE
#define PARAM_TYPE_LIST(V)             \
    V(INT_OVERFLOW, IntOverflowType)   \

class ParamType {
public:
    explicit ParamType(uint32_t type = 0) : type_(type) {}

    ParamType(uint32_t jsType, [[maybe_unused]] bool isBuiltinType)
    {
        // only builtins type uses two-param constructor
        ASSERT(isBuiltinType);
        type_ = BUILTIN_TYPE | jsType;
    }

#define DEFINE_TYPE_CONSTRUCTOR(type, name) \
    static ParamType name() { return ParamType(static_cast<uint32_t>(type)); }

    PRIMITIVE_TYPE_LIST(DEFINE_TYPE_CONSTRUCTOR)
    PARAM_TYPE_LIST(DEFINE_TYPE_CONSTRUCTOR)
#undef DEFINE_TYPE_CONSTRUCTOR

#define DEFINE_JUDGE_METHOD(type, name) \
    bool Is##name() const { return type_ == static_cast<uint32_t>(type); }

    PRIMITIVE_TYPE_LIST(DEFINE_JUDGE_METHOD)
    PARAM_TYPE_LIST(DEFINE_JUDGE_METHOD)
#undef DEFINE_JUDGE_METHOD

    uint32_t Value() const
    {
        return type_;
    }

    bool HasNumberType() const
    {
        return IsNumberType() || IsIntType() || IsIntOverflowType() || IsDoubleType();
    }

    bool IsBuiltinType() const
    {
        return type_ & BUILTIN_TYPE;
    }

    uint32_t GetBuiltinType() const
    {
        return type_ & ~BUILTIN_TYPE;
    }

    bool operator ==(const ParamType &other) const
    {
        return type_ == other.type_;
    }

    bool operator !=(const ParamType &other) const
    {
        return !(*this == other);
    }

private:
    enum : uint8_t {
#define DECLARE_TYPE(type, name) type,
        PRIMITIVE_TYPE_LIST(DECLARE_TYPE)
        PARAM_TYPE_LIST(DECLARE_TYPE)
#undef DECLARE_TYPE
    };

    static constexpr uint32_t BUILTIN_TYPE = (1 << 31); // 31 : the 31-th bit is set implies builtin type

    uint32_t type_ {0};
};
#undef PARAM_TYPE_LIST

// GateType: Trusted type, directly bound to Gate
class GateType {
public:
    constexpr explicit GateType(uint32_t type = 0)
    {
        type_ |= type;
    }

    explicit GateType([[maybe_unused]]GlobalTSTypeRef gt)
    {
        // linxiang shoult remove in part3
        type_ = EMPTY;
    }

    ~GateType() = default;

    uint32_t Value() const
    {
        return type_;
    }

    static GateType NJSValue()
    {
        return GateType(NJS_VALUE);
    }

    static GateType TaggedValue()
    {
        return GateType(TAGGED_VALUE);
    }

    static GateType TaggedPointer()
    {
        return GateType(TAGGED_POINTER);
    }

    static GateType TaggedNPointer()
    {
        return GateType(TAGGED_NPOINTER);
    }

    static GateType Empty()
    {
        return GateType(EMPTY);
    }

    static GateType AnyType()
    {
        return GateType(static_cast<uint32_t>(TSPrimitiveType::ANY));
    }

    static GateType NumberType()
    {
        return GateType(static_cast<uint32_t>(TSPrimitiveType::NUMBER));
    }

    static GateType DoubleType()
    {
        return GateType(static_cast<uint32_t>(TSPrimitiveType::DOUBLE));
    }

    static GateType BooleanType()
    {
        return GateType(static_cast<uint32_t>(TSPrimitiveType::BOOLEAN));
    }

    static GateType VoidType()
    {
        return GateType(static_cast<uint32_t>(TSPrimitiveType::VOID_TYPE));
    }

    static GateType StringType()
    {
        return GateType(static_cast<uint32_t>(TSPrimitiveType::STRING));
    }

    static GateType SymbolType()
    {
        return GateType(static_cast<uint32_t>(TSPrimitiveType::SYMBOL));
    }

    static GateType NullType()
    {
        return GateType(static_cast<uint32_t>(TSPrimitiveType::NULL_TYPE));
    }

    static GateType UndefinedType()
    {
        return GateType(static_cast<uint32_t>(TSPrimitiveType::UNDEFINED));
    }

    static GateType IntType()
    {
        return GateType(static_cast<uint32_t>(TSPrimitiveType::INT));
    }

    static GateType BigIntType()
    {
        return GateType(static_cast<uint32_t>(TSPrimitiveType::BIG_INT));
    }

    bool IsAnyType() const
    {
        uint32_t type = GetType();
        return type == static_cast<uint32_t>(TSPrimitiveType::ANY);
    }

    bool IsNumberType() const
    {
        uint32_t type = GetType();
        return ((type == static_cast<uint32_t>(TSPrimitiveType::NUMBER)) ||
               (type == static_cast<uint32_t>(TSPrimitiveType::INT))    ||
               (type == static_cast<uint32_t>(TSPrimitiveType::DOUBLE)));
    }

    bool IsIntType() const
    {
        uint32_t type = GetType();
        return type == static_cast<uint32_t>(TSPrimitiveType::INT);
    }

    bool IsDoubleType() const
    {
        uint32_t type = GetType();
        return type == static_cast<uint32_t>(TSPrimitiveType::DOUBLE);
    }

    bool IsStringType() const
    {
        uint32_t type = GetType();
        return type == static_cast<uint32_t>(TSPrimitiveType::STRING);
    }

    bool IsNullType() const
    {
        uint32_t type = GetType();
        return type == static_cast<uint32_t>(TSPrimitiveType::NULL_TYPE);
    }

    bool IsUndefinedType() const
    {
        uint32_t type = GetType();
        return type == static_cast<uint32_t>(TSPrimitiveType::UNDEFINED);
    }

    bool IsBooleanType() const
    {
        uint32_t type = GetType();
        return type == static_cast<uint32_t>(TSPrimitiveType::BOOLEAN);
    }

    bool IsBigIntType() const
    {
        uint32_t type = GetType();
        return type == static_cast<uint32_t>(TSPrimitiveType::BIG_INT);
    }

    bool IsNJSValueType() const
    {
        return type_ == NJS_VALUE;
    }

    bool IsDigitablePrimitiveType() const
    {
        return IsNumberType() || IsNullType() || IsUndefinedType() || IsBooleanType() || IsBigIntType();
    }

    bool IsSymbolType() const
    {
        uint32_t type = GetType();
        return type == static_cast<uint32_t>(TSPrimitiveType::SYMBOL);
    }

    // In addition to normal number types, null, undefined, boolean types will be converted to numeric types when doing
    // some numerical computation.
    bool IsPrimitiveNumberType() const
    {
        return IsNumberType() || IsNullType() || IsUndefinedType() || IsBooleanType();
    }

    bool IsPrimitiveIntType() const
    {
        return IsIntType() || IsNullType() || IsBooleanType();
    }

    bool IsGCRelated() const
    {
        return (type_ & (~GateType::GC_MASK)) == 0;
    }

    bool operator ==(const GateType &other) const
    {
        return type_ == other.type_;
    }

    bool operator !=(const GateType &other) const
    {
        return type_ != other.type_;
    }

    bool operator <(const GateType &other) const
    {
        return type_ < other.type_;
    }

    bool operator <=(const GateType &other) const
    {
        return type_ <= other.type_;
    }

    bool operator >(const GateType &other) const
    {
        return type_ > other.type_;
    }

    bool operator >=(const GateType &other) const
    {
        return type_ >= other.type_;
    }

    uint32_t GetType() const
    {
        return type_ & (~MIR_TYPE_MASK);
    }

    GlobalTSTypeRef GetGTRef() const
    {
        // linxiang shoult remove in part3
        GlobalTSTypeRef empty;
        return empty;
    }

private:
    static constexpr uint32_t GC_MASK = ~(1 << 30); // 30 : the 30-th bit is unset implies GC-related type
    static constexpr uint32_t NO_GC_MASK = ~(1 << 29); // 29 : the 29-th bit is unset implies NO-GC-related type
    // 31 : the 31-st bit is set implies MIR type
    static constexpr uint32_t MIR_BASE_BITS = (1 << 31) | (1 << 30) | (1 << 29);
    static constexpr uint32_t EMPTY_TYPE = 1 << 28; // 1 : means offset of empty type
    static constexpr uint32_t MIR_TYPE_MASK = MIR_BASE_BITS | EMPTY_TYPE;

    static constexpr uint32_t NJS_VALUE = MIR_BASE_BITS;                           // (1110)
    static constexpr uint32_t TAGGED_VALUE = MIR_BASE_BITS & GC_MASK & NO_GC_MASK; // (1000)
    static constexpr uint32_t TAGGED_POINTER = MIR_BASE_BITS & GC_MASK;            // (1010)
    static constexpr uint32_t TAGGED_NPOINTER = MIR_BASE_BITS & NO_GC_MASK;        // (1100)
    static constexpr uint32_t EMPTY = NJS_VALUE + EMPTY_TYPE;                      // (1111)
    static constexpr uint32_t SIZE_BITS = 4;

    static constexpr uint32_t VALID_BITS = sizeof(uint32_t) * 8;
    static_assert((SIZE_BITS + GlobalTSTypeRef::GetSizeBits()) <= VALID_BITS);

    uint32_t type_ {0};
};

enum class ConvertSupport : uint8_t {
    ENABLE,
    // Not support conversion from srcType to dstType. It is necessary to use 'deopt' to ensure semantic correctness.
    DISABLE
};

class Type {
public:
    explicit Type(GateType payload);
    [[nodiscard]] bool IsBitset() const;
    ~Type();

private:
    GateType payload;
};
}  // namespace panda::ecmascript::kungfu

#endif  // ECMASCRIPT_COMPILER_TYPE_H
