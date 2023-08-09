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

#include "ecmascript/ts_types/global_ts_type_ref.h"

namespace panda::ecmascript::kungfu {
class GateType {
public:
    constexpr explicit GateType(uint32_t type = 0) : type_(type)
    {
    }

    explicit GateType(GlobalTSTypeRef gt) : type_(0)
    {
        type_ |= gt.GetType();
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
        GlobalTSTypeRef r(0, static_cast<int>(TSPrimitiveType::ANY));
        return GateType(r);
    }

    static GateType NumberType()
    {
        GlobalTSTypeRef r(0, static_cast<int>(TSPrimitiveType::NUMBER));
        return GateType(r);
    }

    static GateType DoubleType()
    {
        GlobalTSTypeRef r(0, static_cast<int>(TSPrimitiveType::DOUBLE));
        return GateType(r);
    }

    static GateType BooleanType()
    {
        GlobalTSTypeRef r(0, static_cast<int>(TSPrimitiveType::BOOLEAN));
        return GateType(r);
    }

    static GateType VoidType()
    {
        GlobalTSTypeRef r(0, static_cast<int>(TSPrimitiveType::VOID_TYPE));
        return GateType(r);
    }

    static GateType StringType()
    {
        GlobalTSTypeRef r(0, static_cast<int>(TSPrimitiveType::STRING));
        return GateType(r);
    }

    static GateType SymbolType()
    {
        GlobalTSTypeRef r(0, static_cast<int>(TSPrimitiveType::SYMBOL));
        return GateType(r);
    }

    static GateType NullType()
    {
        GlobalTSTypeRef r(0, static_cast<int>(TSPrimitiveType::NULL_TYPE));
        return GateType(r);
    }

    static GateType UndefinedType()
    {
        GlobalTSTypeRef r(0, static_cast<int>(TSPrimitiveType::UNDEFINED));
        return GateType(r);
    }

    static GateType IntType()
    {
        GlobalTSTypeRef r(0, static_cast<int>(TSPrimitiveType::INT));
        return GateType(r);
    }

    static GateType BigIntType()
    {
        GlobalTSTypeRef r(0, static_cast<int>(TSPrimitiveType::BIG_INT));
        return GateType(r);
    }

    bool IsAnyType() const
    {
        GlobalTSTypeRef r = GetGTRef();
        uint32_t m = r.GetModuleId();
        uint32_t l = r.GetLocalId();
        return (m == 0) && (l == static_cast<uint32_t>(TSPrimitiveType::ANY));
    }

    bool IsNumberType() const
    {
        GlobalTSTypeRef r = GetGTRef();
        uint32_t m = r.GetModuleId();
        uint32_t l = r.GetLocalId();
        return (m == 0) && ((l == static_cast<uint32_t>(TSPrimitiveType::NUMBER)) ||
                            (l == static_cast<uint32_t>(TSPrimitiveType::INT))    ||
                            (l == static_cast<uint32_t>(TSPrimitiveType::DOUBLE)));
    }

    bool IsIntType() const
    {
        GlobalTSTypeRef r = GetGTRef();
        uint32_t m = r.GetModuleId();
        uint32_t l = r.GetLocalId();
        return (m == 0) && (l == static_cast<uint32_t>(TSPrimitiveType::INT));
    }

    bool IsDoubleType() const
    {
        GlobalTSTypeRef r = GetGTRef();
        uint32_t m = r.GetModuleId();
        uint32_t l = r.GetLocalId();
        return (m == 0) && (l == static_cast<uint32_t>(TSPrimitiveType::DOUBLE));
    }

    bool IsStringType() const
    {
        GlobalTSTypeRef r = GetGTRef();
        uint32_t m = r.GetModuleId();
        uint32_t l = r.GetLocalId();
        return (m == 0) && (l == static_cast<uint32_t>(TSPrimitiveType::STRING));
    }

    bool IsNullType() const
    {
        GlobalTSTypeRef r = GetGTRef();
        uint32_t m = r.GetModuleId();
        uint32_t l = r.GetLocalId();
        return (m == 0) && (l == static_cast<uint32_t>(TSPrimitiveType::NULL_TYPE));
    }

    bool IsUndefinedType() const
    {
        GlobalTSTypeRef r = GetGTRef();
        uint32_t m = r.GetModuleId();
        uint32_t l = r.GetLocalId();
        return (m == 0) && (l == static_cast<uint32_t>(TSPrimitiveType::UNDEFINED));
    }

    bool IsBooleanType() const
    {
        GlobalTSTypeRef r = GetGTRef();
        uint32_t m = r.GetModuleId();
        uint32_t l = r.GetLocalId();
        return (m == 0) && (l == static_cast<uint32_t>(TSPrimitiveType::BOOLEAN));
    }

    bool IsBigIntType() const
    {
        GlobalTSTypeRef r = GetGTRef();
        uint32_t m = r.GetModuleId();
        uint32_t l = r.GetLocalId();
        return (m == 0) && (l == static_cast<uint32_t>(TSPrimitiveType::BIG_INT));
    }

    bool IsNJSValueType() const
    {
        return type_ == NJS_VALUE;
    }

    bool IsDigitablePrimitiveType() const
    {
        return IsNumberType() || IsNullType() || IsUndefinedType() || IsBooleanType() || IsBigIntType();
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

    GlobalTSTypeRef GetGTRef() const
    {
        uint32_t r = type_ & (~MIR_TYPE_MASK);
        return GlobalTSTypeRef(r);
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

enum class ValueType : uint8_t {
    BOOL,
    INT32,
    UINT32,
    FLOAT64,
    TAGGED_BOOLEAN,
    TAGGED_INT,
    TAGGED_DOUBLE,
    TAGGED_NUMBER,
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
